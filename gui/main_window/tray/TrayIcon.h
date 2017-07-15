#pragma once

#include "../../types/message.h"

class QSystemTrayIcon;

namespace Logic
{
    class RecentItemDelegate;
}

namespace Ui
{
#ifdef _WIN32
    class ToastManager;
#else
    class NotificationCenterManager;
#endif //_WIN32
    class RecentMessagesAlert;
    class MainWindow;
    class ContextMenu;

    enum TrayPosition
    {
        TOP_LEFT = 0,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTOOM_RIGHT,
    };

    class TrayIcon : public QObject
    {
        Q_OBJECT

    public:
        TrayIcon(MainWindow* parent);
        ~TrayIcon();

        void Hide();
        void forceUpdateIcon();

        void setVisible(bool visible);

    private Q_SLOTS:
        void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
        void newMail(QString, QString, QString, QString);
        void mailStatus(QString, unsigned, bool);
        void messageClicked(QString, QString);
        void clearNotifications(QString);
        void updateIcon();
        void activated(QSystemTrayIcon::ActivationReason);
        void loggedIn();
        void loggedOut(const bool _is_auth_error);
        void menuStateOnline();
        void menuStateDoNotDisturb();
        void menuStateInvisible();
        void myInfo();
        void updateAlertsPosition();
        void onEmailIconClick(QSystemTrayIcon::ActivationReason);
        void mailBoxOpened();
        void logout();
    private:
        void init();
        void initEMailIcon();
        void showEmailIcon();
        void hideEmailIcon();

        bool canShowNotifications(bool isMail) const;
        void openMailBox(const QString& _mailId);
#ifdef _WIN32
        bool toastSupported();
#else
        bool ncSupported();
#endif //_WIN32
        void showMessage(const Data::DlgState& state);
        TrayPosition getTrayPosition() const;
        void markShowed(const QString&);
        bool canShowNotificationsWin() const;

        void updateStatus();
        void setMacIcon();

        QAction * onlineAction_;
        QAction * dndAction_;
        QAction * invAction_;

    private:
        
        QSystemTrayIcon* systemTrayIcon_;
        QSystemTrayIcon* emailSystemTrayIcon_;

        ContextMenu* Menu_;
        RecentMessagesAlert* MessageAlert_;
        RecentMessagesAlert* MailAlert_;
        QHash<QString, qint64> ShowedMessages_;
        QStringList Notifications_;
        MainWindow* MainWindow_;

        bool first_start_;

        QIcon Base_;
        QIcon Unreads_;
        QIcon TrayBase_;
        QIcon TrayUnreads_;
        QIcon emailIcon_;

        QString Email_;
        QTimer* InitMailStatusTimer_;
        int UnreadsCount_;

#if defined (_WIN32)
        std::unique_ptr<ToastManager> ToastManager_;
        ITaskbarList3 *ptbl;
#elif defined(__APPLE__)
        std::unique_ptr<NotificationCenterManager> NotificationCenterManager_;
#endif //_WIN32
    };
}
