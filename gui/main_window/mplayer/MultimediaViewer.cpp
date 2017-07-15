#include "stdafx.h"
#include "MultimediaViewer.h"
#include "VideoPlayer.h"
#include "../../utils/utils.h"
#include "../sounds/SoundsManager.h"

namespace Ui
{


    MultimediaViewer::MultimediaViewer(QWidget* _parent)
        :   QWidget(_parent), closed_(false)//, videoPlayer_(0)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/mplayer/mstyles.qss"));

        setAttribute(Qt::WA_TranslucentBackground, true);
                
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

        QVBoxLayout* rootLayout = Utils::emptyVLayout();
        setLayout(rootLayout);

        QWidget* rootWidget = new QWidget(this);

        rootWidget->setStyleSheet("background: rgba(0, 0, 0, 65%);");
        rootLayout->addWidget(rootWidget);

        setMouseTracking(true);
        grabKeyboard();

        setFocusPolicy(Qt::FocusPolicy::WheelFocus);
    }

    MultimediaViewer::~MultimediaViewer()
    {
        //delete videoPlayer_;
    }

    void MultimediaViewer::paintEvent(QPaintEvent* _e)
    {
        QWidget::paintEvent(_e);
    }

    void MultimediaViewer::keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Escape)
        {
            releaseKeyboardAndClose();
        }
    }

    void MultimediaViewer::mousePressEvent(QMouseEvent*)
    {
        releaseKeyboardAndClose();
    }

    void MultimediaViewer::resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);
    }

    void MultimediaViewer::releaseKeyboardAndClose()
    {
        closed_ = true;

        showNormal();
        
        releaseKeyboard();
        close();
        
//         if (videoPlayer_)
//         {
//             videoPlayer_->close();
//         }

        emit closed();
    }

    void MultimediaViewer::closeViewer()
    {
        if (closed_)
            return;

        releaseKeyboardAndClose();
    }
    
    void MultimediaViewer::showWindow()
    {
        show();

        //videoPlayer_ = new VideoPlayer(this);

//         connect(videoPlayer_, &VideoPlayer::signalClose, this, [this]()
//         {
//             releaseKeyboardAndClose();
//         });
// 
//         connect(videoPlayer_, &VideoPlayer::mediaLoaded, this, [this]()
//         {
//             emit mediaLoaded();
//         });



//         videoPlayer_->show();
//         videoPlayer_->moveToTop();
//         videoPlayer_->setFocus();
    }

    void MultimediaViewer::playMedia(const QString& _mediaSource)
    {
//         if (videoPlayer_)
//         {
//             videoPlayer_->play(_mediaSource);
//         }
    }
    
    void MultimediaViewer::changeEvent(QEvent* _e)
    {
        /*if (_e->type() == QEvent::ActivationChange)
        {
            if (videoPlayer_ && isActiveWindow())
            {
                videoPlayer_->moveToTop();
                qDebug() << "move to top";
            }
        }*/
    }
}
