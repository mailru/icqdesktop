#include "stdafx.h"
#include "local_peer.h"

#include <QtNetwork/QLocalSocket>

#include "../main_window/MainWindow.h"
#include "InterConnector.h"

LocalPeer::LocalPeer(QObject* parent, bool other)
    : QObject(parent)
    , other_(other)
    , wnd_(0)
{
    socket_name_ = Utils::get_crossprocess_pipe_name();
    if (!other_)
        server_ = new QLocalServer(this);
}

void LocalPeer::listen()
{
    server_->listen(socket_name_);
    QObject::connect(server_, SIGNAL(newConnection()), SLOT(receiveConnection()), Qt::QueuedConnection);
}

void LocalPeer::wait()
{
    server_->waitForNewConnection(5000);
}

unsigned int LocalPeer::get_hwnd_and_activate()
{
    unsigned int hwnd = 0;

#ifdef _WIN32
    QLocalSocket socket;
    bool connOk = false;
    int timeout = 5000;

    for (int i = 0; i < 2; ++i) 
    {
        socket.connectToServer(socket_name_);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;
        int ms = 250;
        Sleep(DWORD(ms));
    }

    if (!connOk)
        return false;

    QByteArray uMsg((QString(crossprocess_message_get_hwnd_activate)).toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());

    if (socket.waitForBytesWritten(timeout))
    {
        if (socket.waitForReadyRead(timeout))
        {
            QByteArray data_read = socket.readAll();
            if (data_read.size() == sizeof(hwnd))
            {
                hwnd = *(unsigned int*) data_read.data();
            }
        }
    }
#endif //_WIN32

    return hwnd;
}

void LocalPeer::send_url_command(const QString& _url_command)
{
#ifdef _WIN32
    QLocalSocket socket;
    bool connOk = false;
    int timeout = 5000;

    for (int i = 0; i < 2; ++i) 
    {
        socket.connectToServer(socket_name_);

        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;

        int ms = 250;
        ::Sleep(DWORD(ms));
    }

    if (!connOk)
        return;

    QByteArray uMsg((QString(crossprocess_message_execute_url_command) + ":" + _url_command).toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());

    if (socket.waitForBytesWritten(timeout))
    {
    }
#endif //_WIN32
}

void LocalPeer::set_main_window(Ui::MainWindow* _wnd)
{
    wnd_ = _wnd;
}

void LocalPeer::receiveConnection()
{
#ifdef _WIN32
    std::auto_ptr<QLocalSocket> socket(server_->nextPendingConnection());
    if (!socket.get())
        return;

    while (socket->bytesAvailable() < (int)sizeof(quint32))
        socket->waitForReadyRead();

    QDataStream ds(socket.get());
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;

    uMsg.resize(remaining);
    int got = 0;

    char* uMsgBuf = uMsg.data();

    do 
    {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } 
    while (remaining && got >= 0 && socket->waitForReadyRead(2000));

    if (got < 0) 
        return;
    
    QString message_execute_url_command(crossprocess_message_execute_url_command ":");

    QString message(QString::fromUtf8(uMsg));

    if (message == crossprocess_message_get_process_id)
    {
        unsigned int process_id = ::GetCurrentProcessId();
        socket->write((const char*) &process_id, sizeof(process_id));
    }
    else if (message == crossprocess_message_get_hwnd_activate)
    {
        unsigned int hwnd = 0;
        if (wnd_)
        {
            hwnd = (unsigned int) wnd_->winId();
            wnd_->activateFromEventLoop();
        }
        socket->write((const char*) &hwnd, sizeof(hwnd));
    }
    else if (message.length() > message_execute_url_command.length() && 
        message.mid(0, message_execute_url_command.length()) == message_execute_url_command)
    {
        QString command = message.mid(message_execute_url_command.length());

        emit Utils::InterConnector::instance().schemeUrlClicked(command);
    }
    else
    {
        socket->write("icq", qstrlen("icq"));
    }

    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000);
       

    if (message == crossprocess_message_shutdown_process)
    {
        QApplication::exit(0);
    }
#endif //_WIN32
}
