#include "stdafx.h"
#include "DetachedVideoWnd.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../utils/utils.h"
#include "VideoPanelHeader.h"
#include "VideoWindow.h"

#ifdef _WIN32
    #include "QDesktopWidget"
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif


#define SHOW_HEADER_PANEL false

Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* parent)
    : AspectRatioResizebleWnd()
    , parent_(parent)
    , closedManualy_(false)
    , showPanelTimer_(this)
    , videoPanelHeaderEffect_(NULL)
	, shadow_ (new Ui::ShadowWindowParent(this))
{
        this->resize(400, 300);
        horizontalLayout_ = Utils::emptyHLayout(this);
        horizontalLayout_->setAlignment(Qt::AlignVCenter);
        QMetaObject::connectSlotsByName(this);
        
#ifdef _WIN32
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::SubWindow);
#else
        // We have problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
        // If video window is showing with this flags,, we cannot activate main ICQ window.
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window /*| Qt::WindowDoesNotAcceptFocus*/);
        //setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif

        if (SHOW_HEADER_PANEL)
        {
            videoPanelHeader_.reset(new VideoPanelHeader(this, kVPH_ShowNone));
        }

        if (!!videoPanelHeader_)
        {
            videoPanelHeaderEffect_ = new UIEffects(*videoPanelHeader_.get());
        }
#ifndef __linux__
        std::vector<Ui::BaseVideoPanel*> panels;
        if (!!videoPanelHeader_)
        {
            panels.push_back(videoPanelHeader_.get());
        }
        rootWidget_ = platform_specific::GraphicsPanel::create(this, panels, false);
        rootWidget_->setContentsMargins(0, 0, 0, 0);
        rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
        rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        layout()->addWidget(rootWidget_);
#endif //__linux__

		std::vector<BaseVideoPanel*> videoPanels;

        if (!!videoPanelHeader_)
        {
			videoPanels.push_back(videoPanelHeader_.get());
        }

        eventFilter_ = new ResizeEventFilter(videoPanels, shadow_->getShadowWidget(), this);
        installEventFilter(eventFilter_);

        setAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_ShowWithoutActivating);

        connect(&showPanelTimer_, SIGNAL(timeout()), this, SLOT(checkPanelsVis()), Qt::QueuedConnection);
        showPanelTimer_.setInterval(1500);

        if (!!videoPanelHeader_)
        {
            videoPanelHeader_->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            videoPanelHeader_->setAttribute(Qt::WA_NoSystemBackground, true);
            videoPanelHeader_->setAttribute(Qt::WA_TranslucentBackground, true);

            connect(videoPanelHeader_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
            connect(videoPanelHeader_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);

            connect(videoPanelHeader_.get(), SIGNAL(onClose()), this, SLOT(onPanelClickedClose()), Qt::QueuedConnection);
            connect(videoPanelHeader_.get(), SIGNAL(onMinimize()), this, SLOT(onPanelClickedMinimize()), Qt::QueuedConnection);
            connect(videoPanelHeader_.get(), SIGNAL(onMaximize()), this, SLOT(onPanelClickedMaximize()), Qt::QueuedConnection);
        }

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);

        setMinimumSize(Utils::scale_value(320), Utils::scale_value(80));
        
        QDesktopWidget dw;
        const auto screenRect = dw.screenGeometry(dw.primaryScreen());
        const auto detachedWndRect = rect();

        auto detachedWndPos = screenRect.topRight();
        detachedWndPos.setX(detachedWndPos.x() - detachedWndRect.width() - 0.01f * screenRect.width());
        detachedWndPos.setY(detachedWndPos.y() + 0.05f * screenRect.height());

        const QRect rc(detachedWndPos.x(), detachedWndPos.y(), detachedWndRect.width(), detachedWndRect.height());
        setGeometry(rc);
}

void Ui::DetachedVideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count <= 1)
    { // in this moment destroyed call is active, e.a. call_count + 1
        closedManualy_ = false;
    }
}

void Ui::DetachedVideoWindow::onPanelClickedClose()
{
    closedManualy_ = true;
    hide();
}
void Ui::DetachedVideoWindow::onPanelClickedMinimize()
{
    //showMinimized();
}
void Ui::DetachedVideoWindow::onPanelClickedMaximize()
{
    //if (_parent) {
    //_parent->showNormal();
    //_parent->activateWindow();
    //hide();
    //}
}

void Ui::DetachedVideoWindow::onPanelMouseEnter()
{
    showPanelTimer_.stop();
    if (videoPanelHeaderEffect_)
    {
        videoPanelHeaderEffect_->fadeIn(500);
    }
}

void Ui::DetachedVideoWindow::onPanelMouseLeave()
{
    const QPoint p = mapFromGlobal(QCursor::pos());
    if (!rect().contains(p))
    {
        showPanelTimer_.start();
    }
}

void Ui::DetachedVideoWindow::mousePressEvent(QMouseEvent* _e)
{
    posDragBegin_ = _e->pos();
}

void Ui::DetachedVideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    if (_e->buttons() & Qt::LeftButton)
    {
        QPoint diff = _e->pos() - posDragBegin_;
        QPoint newpos = this->pos() + diff;

        this->move(newpos);
    }
}

void Ui::DetachedVideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    showPanelTimer_.stop();
    if (videoPanelHeaderEffect_)
    {
        videoPanelHeaderEffect_->fadeIn(500);
    }
}

void Ui::DetachedVideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    showPanelTimer_.start();
}

void Ui::DetachedVideoWindow::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
    if (parent_)
    {
        if (parent_->isMinimized())
        {
            parent_->showNormal();
        }
        parent_->raise();
        parent_->activateWindow();
        //hide();
    }
}

quintptr Ui::DetachedVideoWindow::getContentWinId()
{
    return (quintptr)rootWidget_->frameId();
}

void Ui::DetachedVideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        hide();
    }
}

void Ui::DetachedVideoWindow::onVoipWindowAddComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        showNormal();
    }
}

void Ui::DetachedVideoWindow::showFrame()
{
#ifdef _WIN32
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), false, false, 0);		
    }
	shadow_->showShadow();
#endif
}

void Ui::DetachedVideoWindow::hideFrame()
{
#ifdef _WIN32
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());		
    }
	shadow_->hideShadow();
#endif
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    if (!!videoPanelHeader_)
    {
        videoPanelHeader_->show();
    }
    QWidget::showEvent(_e);
    showPanelTimer_.stop();

    if (videoPanelHeaderEffect_)
    {
        videoPanelHeaderEffect_->fadeIn(500);
    }
    showPanelTimer_.start();
}

bool Ui::DetachedVideoWindow::closedManualy()
{
    return closedManualy_;
}

void Ui::DetachedVideoWindow::hideEvent(QHideEvent* _e)
{
    if (!!videoPanelHeader_)
    {
        videoPanelHeader_->hide();
    }
    QWidget::hideEvent(_e);
}

quintptr Ui::DetachedVideoWindow::getVideoFrameId() const
{
    return (quintptr)rootWidget_->frameId();
}

Ui::DetachedVideoWindow::~DetachedVideoWindow()
{
    removeEventFilter(eventFilter_);
    delete eventFilter_;
}

void Ui::DetachedVideoWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
}

void Ui::DetachedVideoWindow::checkPanelsVis()
{
    showPanelTimer_.stop();
    if (videoPanelHeaderEffect_)
    {
        videoPanelHeaderEffect_->fadeOut(500);
    }
}
