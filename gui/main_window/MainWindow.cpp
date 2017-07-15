#include "stdafx.h"
#include "MainWindow.h"

#include "ContactDialog.h"
#include "LoginPage.h"
#include "MainPage.h"
#include "PromoPage.h"
#include "TitleBar.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"
#include "history_control/HistoryControlPage.h"
#include "history_control/MessagesScrollArea.h"
#include "livechats/LiveChatsModel.h"
#include "mplayer/VideoPlayer.h"
#include "sounds/SoundsManager.h"
#include "tray/TrayIcon.h"
#include "../app_config.h"
#include "../gui_settings.h"
#include "../theme_settings.h"
#include "../controls/BackgroundWidget.h"
#include "../controls/CommonStyle.h"
#include "../previewer/GalleryWidget.h"
#include "../previewer/Previewer.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../cache/stickers/stickers.h"
#include "../cache/snaps/SnapStorage.h"
#include "../my_info.h"

#ifdef _WIN32
#include "../../common.shared/win32/crash_handler.h"
#include <windowsx.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")
#endif //_WIN32

#ifdef __APPLE__
#include "macos/AccountsPage.h"
#include "../utils/macos/mac_support.h"
#include "../utils/macos/mac_migration.h"
#include "../utils/macos/mac_toolbar.h"
#endif //__APPLE__

namespace
{
#ifdef _WIN32
    const int TITLE_CONTEXT_MENU_CMD = WM_USER + 0x100;
#endif

	const int SIZE_BOX_WIDTH = 4;
	enum PagesIndex
	{
		loginPage_INDEX = 0,
        mainPage_INDEX = 1,
	};

    int getMinWindowWidth()
    {
        return Utils::scale_value(500);
    }

    int getMinWindowHeight()
    {
        return Utils::scale_value(550);
    }

    QRect getDefaultWindowSize()
    {
        return QRect(0, 0, Utils::scale_value(1000), Utils::scale_value(600));
    }
}

namespace Ui
{
    ShadowWindow::ShadowWindow(QBrush brush, int shadowWidth)
        : QWidget(0, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , ShadowWidth_(shadowWidth)
        , Brush_(brush)
        , IsActive_(true)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void ShadowWindow::setActive(bool _value)
    {
        IsActive_ = _value;
        repaint();
    }

    void ShadowWindow::paintEvent(QPaintEvent * /*e*/)
    {
        QRect origin = rect();

        QRect right = QRect(QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1), QPoint(origin.width(), origin.height() - ShadowWidth_ - 1));
        QRect left = QRect(QPoint(origin.x(), origin.y() + ShadowWidth_ + 1), QPoint(origin.x() + ShadowWidth_, origin.height() - ShadowWidth_ - 1));
        QRect top = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.y()), QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_));
        QRect bottom = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_), QPoint(origin.width() - ShadowWidth_ - 1, origin.height()));

        QRect topLeft = QRect(origin.topLeft(), QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_));
        QRect topRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.y()), QPoint(origin.width(), origin.y() + ShadowWidth_));
        QRect bottomLeft = QRect(QPoint(origin.x(), origin.height() - ShadowWidth_), QPoint(origin.x() + ShadowWidth_, origin.height()));
        QRect bottomRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_), origin.bottomRight());

        QPainter p(this);

        QRect body = origin;
        body.setX(origin.x() + ShadowWidth_);
        body.setY(origin.y() + ShadowWidth_);
        body.setWidth(origin.width() - ShadowWidth_ * 2);
        body.setHeight(origin.height() - ShadowWidth_ * 2);
        p.fillRect(body, Brush_);

        QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
        setGradientColor(lg);
        p.fillRect(right, QBrush(lg));

        lg = QLinearGradient(left.topRight(), left.topLeft());
        setGradientColor(lg);
        p.fillRect(left, QBrush(lg));

        lg = QLinearGradient(top.bottomLeft(), top.topLeft());
        setGradientColor(lg);
        p.fillRect(top, QBrush(lg));

        lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
        setGradientColor(lg);
        p.fillRect(bottom, QBrush(lg));

        QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topLeft, QBrush(g));

        g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topRight, QBrush(g));

        g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomLeft, QBrush(g));

        g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomRight, QBrush(g));
    }

    void ShadowWindow::setGradientColor(QGradient& _gradient)
    {
        QColor windowGradientColor("#000000");
        windowGradientColor.setAlphaF(0.2);
        _gradient.setColorAt(0, windowGradientColor);
        windowGradientColor.setAlphaF(IsActive_ ? 0.08 : 0.04);
        _gradient.setColorAt(0.2, windowGradientColor);
        windowGradientColor.setAlphaF(0.02);
        _gradient.setColorAt(0.6, IsActive_ ? windowGradientColor : Qt::transparent);
        _gradient.setColorAt(1, Qt::transparent);
    }

	TitleWidgetEventFilter::TitleWidgetEventFilter(QObject* parent)
		: QObject(parent)
        , dragging_(false)
	{
        mainWindow_ = static_cast<MainWindow*>(parent);

        iconTimer_ = new QTimer(this);
        iconTimer_->setSingleShot(true);
        connect(iconTimer_, &QTimer::timeout, this, &Ui::TitleWidgetEventFilter::showLogoContextMenu);
	}
    
    void TitleWidgetEventFilter::addIgnoredWidget(QWidget* _widget)
    {
        ignoredWidgets_.insert(_widget);
    }

    void TitleWidgetEventFilter::showContextMenu(QPoint _pos)
    {
#ifdef _WIN32
        HWND handle = (HWND)mainWindow_->winId();
        HMENU sysMenu = GetSystemMenu(handle, FALSE);
        if (sysMenu)
        {
            UINT enabledState = MF_BYCOMMAND | MF_ENABLED;
            UINT disabledState = MF_BYCOMMAND | (MF_DISABLED | MF_GRAYED);
            EnableMenuItem(sysMenu, SC_MAXIMIZE,mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_SIZE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_MOVE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_RESTORE, mainWindow_->isMaximized()? enabledState: disabledState);            
            
            int flag = TrackPopupMenu(sysMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD, _pos.x(), _pos.y(), NULL, handle, 0);
            if(flag > 0)
                SendMessage(handle, WM_SYSCOMMAND, flag, TITLE_CONTEXT_MENU_CMD);
        }        
#endif        
    }

    void TitleWidgetEventFilter::showLogoContextMenu()
    {
        mainWindow_->activate();
        auto logo = mainWindow_->getWindowLogo();
        showContextMenu(logo->mapToGlobal(logo->rect().center()));
    }

	bool TitleWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
	{
        bool ignore = std::any_of(ignoredWidgets_.begin(),
                                  ignoredWidgets_.end(),
                                  [](QWidget* _wdg) { return _wdg->underMouse(); });
        if (!ignore)
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(_event);
            bool logoUnderMouse = mouseEvent && mainWindow_->getWindowLogo()->rect().contains(mouseEvent->pos());
            switch (_event->type())
		    {
		    case QEvent::MouseButtonDblClick:
                if (logoUnderMouse)
                {
                    iconTimer_->stop();
                    emit logoDoubleClick();
                }
                else
                    emit doubleClick();
                dragging_ = false;
			    break;

		    case QEvent::MouseButtonPress:   
                clickPos_ = mouseEvent->pos();
                dragging_ = true;
			    _event->accept();
			    break;

		    case QEvent::MouseMove:
                if (dragging_)
			        emit moveRequest(mouseEvent->globalPos() - clickPos_);
			    break;

            case QEvent::MouseButtonRelease:
                dragging_ = false;
                emit checkPosition();
                if (mouseEvent->button() == Qt::RightButton)
                    showContextMenu(mouseEvent->globalPos());
                else if (logoUnderMouse)
                    iconTimer_->start(qApp->doubleClickInterval());
                break;

		    default:
			    break;
		    }
        }
		return QObject::eventFilter(_obj, _event);
	}

    void MainWindow::hideTaskbarIcon()
    {
#ifdef _WIN32
        HWND parent = (HWND)::GetWindowLong((HWND) winId(), GWL_HWNDPARENT);
        if (!parent)
            ::SetWindowLong((HWND) winId(), GWL_HWNDPARENT, (LONG) fake_parent_window_);
#endif //_WIN32
        trayIcon_->forceUpdateIcon();
    }

    void MainWindow::showTaskbarIcon()
    {
#ifdef _WIN32
        ::SetWindowLong((HWND) winId(), GWL_HWNDPARENT, 0L);

        std::unique_ptr<QWidget> w(new QWidget(this));
        w->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        w->show();
        w->activateWindow();
        trayIcon_->forceUpdateIcon();
#endif //_WIN32
    }

    void MainWindow::showMenuBarIcon(bool _show)
    {
        trayIcon_->setVisible(_show);
    }
    
    void MainWindow::setFocusOnInput()
    {
        if (mainPage_)
            mainPage_->setFocusOnInput();
    }

    void MainWindow::onSendMessage(const QString& contact)
    {
        if (mainPage_)
            mainPage_->onSendMessage(contact);
    }

    int MainWindow::getTitleHeight() const
    {
#ifdef _WIN32
        return titleWidget_->height();
#else
        return 0;
#endif // _WIN32
    }

    void MainWindow::setTitleIconsVisible(bool _unreadMsgVisible, bool _unreadMailVisible)
    {
        unreadMsg_->setVisible(_unreadMsgVisible);
        unreadMail_->setVisible(_unreadMailVisible);
#ifdef __APPLE__
        getMacSupport()->toolbar()->setTitleIconsVisible(_unreadMsgVisible, _unreadMailVisible);
#endif
    }

    MainWindow::MainWindow(QApplication* app, const bool _has_valid_login)
        : gallery_(new Previewer::GalleryWidget(this))
		, mainPage_(nullptr)
		, loginPage_(nullptr)
        , promoPage_(nullptr)
#ifdef __APPLE__
        , accounts_page_(nullptr)
#endif //_APPLE__
		, app_(app)
		, eventFilter_(new TitleWidgetEventFilter(this))
		, trayIcon_(new TrayIcon(this))
        , backgroundPixmap_(QPixmap())
        , Shadow_(0)
        , SkipRead_(false)
        , TaskBarIconHidden_(false)
        , callPanelMainEx(NULL)
        , IsMaximized_(false)
        , ffplayer_(nullptr)
	{
        Utils::InterConnector::instance().setMainWindow(this);

#ifdef _WIN32
        auto productDataPath = ::common::get_user_profile() + L"/" + (build::is_icq() ? product_path_icq_w : product_path_agent_w);
        core::dump::crash_handler chandler("icq.desktop", productDataPath.c_str(), false);
        chandler.set_process_exception_handlers();
        chandler.set_thread_exception_handlers();
#endif //_WIN32

        setStyleSheet(MainPage::getMainWindowQss());
#ifdef __APPLE__
        getMacSupport()->enableMacCrashReport();
        getMacSupport()->listenSleepAwakeEvents();
#endif
        app_->installNativeEventFilter(this);
        app_->installEventFilter(this);

        this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        this->setLayoutDirection(Qt::LeftToRight);
        this->setAutoFillBackground(false);
        mainWidget_ = new QWidget(this);
        mainWidget_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        mainLayout_ = Utils::emptyVLayout(mainWidget_);
		mainLayout_->setSizeConstraint(QLayout::SetDefaultConstraint);
        titleWidget_ = new QWidget(mainWidget_);
        titleWidget_->setObjectName("titleWidget");
        titleWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        titleLayout_ = Utils::emptyHLayout(titleWidget_);
        logo_ = new QPushButton(titleWidget_);
        logo_->setObjectName("windowIcon");
        logo_->setProperty("agent", build::is_agent() ? true : false);
        logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        logo_->setFocusPolicy(Qt::NoFocus);
		titleLayout_->addWidget(logo_);
        title_ = new QLabel(titleWidget_);
        title_->setFocusPolicy(Qt::NoFocus);
        title_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Medium));
        title_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
		titleLayout_->addWidget(title_);
        titleLayout_->addItem(new QSpacerItem(Utils::scale_value(20), 20, QSizePolicy::Fixed, QSizePolicy::Minimum));
        unreadMsg_ = new UnreadMsgWidget(titleWidget_);        
        titleLayout_->addWidget(unreadMsg_);
        titleLayout_->addItem(new QSpacerItem(Utils::scale_value(4), 20, QSizePolicy::Fixed, QSizePolicy::Minimum));
        unreadMail_ = new UnreadMailWidget(titleWidget_);
        titleLayout_->addWidget(unreadMail_);
        spacer_ = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
		titleLayout_->addItem(spacer_);
        hideButton_ = new QPushButton(titleWidget_);
        Utils::ApplyStyle(hideButton_, Ui::CommonStyle::getMinimizeButtonStyle());
		titleLayout_->addWidget(hideButton_);
        maximizeButton_ = new QPushButton(titleWidget_);
        Utils::ApplyStyle(maximizeButton_, Ui::CommonStyle::getMaximizeButtonStyle());
		maximizeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		titleLayout_->addWidget(maximizeButton_);
        closeButton_ = new QPushButton(titleWidget_);
		closeButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Utils::ApplyStyle(closeButton_, Ui::CommonStyle::getCloseButtonStyle());
		titleLayout_->addWidget(closeButton_);
		mainLayout_->addWidget(titleWidget_);
        stackedWidget_ = new BackgroundWidget(mainWidget_, "");

        {// Call panel main ex
            CallPanelMainEx::CallPanelMainFormat format;
            format.topPartHeight = !platform::is_apple() ? Utils::scale_value(28) : 0;
            format.topPartFormat = kVPH_ShowAll;
            format.bottomPartHeight = Utils::scale_value(50);

            callPanelMainEx = new CallPanelMainEx(this, format);
            connect(callPanelMainEx, SIGNAL(onMinimize()), this, SLOT(minimize()), Qt::QueuedConnection);
            connect(callPanelMainEx, SIGNAL(onMaximize()), this, SLOT(maximize()), Qt::QueuedConnection);
            connect(callPanelMainEx, SIGNAL(onClose()), this, SLOT(hideWindow()), Qt::QueuedConnection);

            connect(callPanelMainEx, SIGNAL(onClickOpenChat(const std::string&)), this, SLOT(onOpenChat(const std::string&)), Qt::DirectConnection);
            connect(callPanelMainEx, SIGNAL(onBackToVideo()), this, SLOT(onShowVideoWindow()), Qt::DirectConnection);
        }

        if (callPanelMainEx)
        {
			mainLayout_->addWidget(callPanelMainEx);
            callPanelMainEx->installEventFilter(eventFilter_);

            callPanelMainEx->hide();
        }

        QPixmap p(":/resources/main_window/pat_100.png");
        setBackgroundPixmap(p, true);

        //Utils::InterConnector::instance().setMainWindow(this);
        get_qt_theme_settings()->setOrLoadDefaultTheme();
		mainLayout_->addWidget(stackedWidget_);
        this->setCentralWidget(mainWidget_);

        logo_->setText(QString());
		hideButton_->setText(QString());
		maximizeButton_->setText(QString());
		closeButton_->setText(QString());

		stackedWidget_->setCurrentIndex(-1);
        QMetaObject::connectSlotsByName(this);


        QFont f = QApplication::font();
        f.setStyleStrategy(QFont::PreferAntialias);
        QApplication::setFont(f);

#ifdef _WIN32
        if (get_common_settings()->get_need_show_promo())
        {
            showPromoPage();
        }
        else
        {
            if (!get_gui_settings()->get_value(settings_keep_logged_in, true) || !_has_valid_login)// || !get_gui_settings()->contains_value(settings_keep_logged_in))
            {
                showLoginPage(false);
            }
            else
            {
                showMainPage();
            }
        }
#else
        if (!get_gui_settings()->get_value(settings_keep_logged_in, true))// || !get_gui_settings()->contains_value(settings_keep_logged_in))
        {
            showLoginPage(false);
        }
        else
        {
            showMainPage();
        }
#endif // _WIN32

        eventFilter_->addIgnoredWidget(unreadMsg_);
        eventFilter_->addIgnoredWidget(unreadMail_);
        eventFilter_->addIgnoredWidget(hideButton_);
        eventFilter_->addIgnoredWidget(maximizeButton_);
        eventFilter_->addIgnoredWidget(closeButton_);
		titleWidget_->installEventFilter(eventFilter_);
        connect(eventFilter_, &Ui::TitleWidgetEventFilter::logoDoubleClick, this, &Ui::MainWindow::hideWindow, Qt::QueuedConnection);
        
        title_->setText(build::is_icq() ? QT_TRANSLATE_NOOP("title", "ICQ") : QT_TRANSLATE_NOOP("title", "Mail.Ru Agent"));
        title_->setAttribute(Qt::WA_TransparentForMouseEvents);
		logo_->setAttribute(Qt::WA_TransparentForMouseEvents);

		setWindowTitle(title_->text());
#ifdef _WIN32
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowSystemMenuHint);
        fake_parent_window_ = Utils::createFakeParentWindow();
#else
        titleWidget_->hide();
#endif
      
		title_->setMouseTracking(true);

		connect(hideButton_, SIGNAL(clicked()), this, SLOT(minimize()), Qt::QueuedConnection);
		connect(maximizeButton_, SIGNAL(clicked()), this, SLOT(maximize()), Qt::QueuedConnection);
		connect(closeButton_, SIGNAL(clicked()), this, SLOT(hideWindow()), Qt::QueuedConnection);

        unreadMsg_->setCursor(Qt::PointingHandCursor);
        unreadMail_->setCursor(Qt::PointingHandCursor);
		hideButton_->setCursor(Qt::PointingHandCursor);
		maximizeButton_->setCursor(Qt::PointingHandCursor);
		closeButton_->setCursor(Qt::PointingHandCursor);

		connect(eventFilter_, SIGNAL(doubleClick()), this, SLOT(maximize()), Qt::QueuedConnection);
		connect(eventFilter_, SIGNAL(moveRequest(QPoint)), this, SLOT(moveRequest(QPoint)), Qt::QueuedConnection);
        connect(eventFilter_, SIGNAL(checkPosition()), this, SLOT(checkPosition()), Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipResetComplete()), this, SLOT(onVoipResetComplete()), Qt::QueuedConnection);

		connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &MainWindow::showLoginPage, Qt::DirectConnection);
		connect(&Utils::InterConnector::instance(), SIGNAL(showIconInTaskbar(bool)), this, SLOT(showIconInTaskbar(bool)), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(activateNextUnread()), this, SLOT(activateNextUnread()), Qt::QueuedConnection);

        connect(this, SIGNAL(needActivate()), this, SLOT(activate()), Qt::QueuedConnection);

        connect(get_gui_settings(), SIGNAL(changed(QString)), this, SLOT(guiSettingsChanged(QString)), Qt::QueuedConnection);

        if (platform::is_windows())
        {
            int shadowWidth = get_gui_settings()->get_shadow_width();
            QBrush b = stackedWidget_->palette().background();
            QMatrix m;
            m.translate(shadowWidth, titleWidget_->height() + shadowWidth);
            b.setMatrix(m);
            Shadow_ = new ShadowWindow(b, shadowWidth);
            QPoint pos = mapToGlobal(QPoint(rect().x(), rect().y()));
            Shadow_->move(pos.x() - shadowWidth, pos.y() - shadowWidth);
            Shadow_->resize(rect().width() + 2 * shadowWidth, rect().height() + 2 * shadowWidth);

            Shadow_->setActive(true);
            Shadow_->show();
        }

        initSettings();
#ifdef _WIN32
        DragAcceptFiles((HWND)winId(), TRUE);
#endif //_WIN32

        if (!get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
			hideTaskbarIcon();

#ifdef __APPLE__
        getMacSupport()->enableMacUpdater();
        getMacSupport()->enableMacPreview(winId());

        getMacSupport()->toolbar()->setup();
        getMacSupport()->toolbar()->setTitleText(title_->text());
        getMacSupport()->toolbar()->setUnreadMsgWidget(unreadMsg_);
        getMacSupport()->toolbar()->setUnreadMailWidget(unreadMail_);
#endif
        setTitleIconsVisible(false, false);
        connect(MyInfo(), &my_info::received, this, &MainWindow::onMyInfoReceived, Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);

        setFocus(Qt::ActiveWindowFocusReason);
	}

	MainWindow::~MainWindow()
	{
        app_->removeNativeEventFilter(this);
        app_->removeEventFilter(this);

#ifdef _WIN32
        if (fake_parent_window_)
            ::DestroyWindow(fake_parent_window_);
#endif
	}

    void MainWindow::onOpenChat(const std::string& _contact)
    {
        raise();
        activate();
        
        hideMenu();

        if (!_contact.empty())
        {
            Logic::getContactListModel()->setCurrent(_contact.c_str(), -1, true, true);
        }
    }

	void MainWindow::activate()
	{
        setVisible(true);
        trayIcon_->Hide();
        activateWindow();
#ifdef _WIN32
        ShowWindow((HWND)winId(), SW_SHOW);
        isMaximized() ? showMaximized() : showNormal();
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
#ifdef __APPLE__
        getMacSupport()->activateWindow(winId());
        getMacSupport()->updateMainMenu();
#endif //__APPLE__

        auto currentHistoryPage = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
        if (currentHistoryPage)
            currentHistoryPage->updateItems();
	}



    void MainWindow::openGallery(const QString& _aimId, const Data::Image& _image, const QString& _localPath)
    {
        gallery_->openGallery(_aimId, _image, _localPath);
    }

    void MainWindow::closeGallery()
    {
        gallery_->closeGallery();
    }

    void MainWindow::activateFromEventLoop()
    {
        emit needActivate();
    }

    bool MainWindow::isActive() const
    {
#ifdef _WIN32
        return GetForegroundWindow() == (HWND)winId();
#else
        return isActiveWindow();
#endif //_WIN32
    }

    bool MainWindow::isMainPage() const
    {
        if (mainPage_ == 0)
            return false;

        return mainPage_->isContactDialog();
    }

    int MainWindow::getScreen() const
    {
        return QApplication::desktop()->screenNumber(this);
    }

    void MainWindow::skipRead()
    {
        SkipRead_ = true;
    }

    void MainWindow::hideMenu()
    {
        if (mainPage_)
            mainPage_->hideMenu();
    }

    void MainWindow::closePopups()
    {
        hideMenu();
        if (mainPage_)
            mainPage_->snapsClose();
        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow();
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        emit Utils::InterConnector::instance().themesSettingsBack();
    }

    HistoryControlPage* MainWindow::getHistoryPage(const QString& _aimId) const
    {
        if (mainPage_)
            return mainPage_->getHistoryPage(_aimId);
        else
            return nullptr;
    }

    MainPage* MainWindow::getMainPage() const
    {
        assert(mainPage_);
        return mainPage_;
    }

    QPushButton* MainWindow::getWindowLogo() const
    {
        return logo_;
    }

    void MainWindow::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        mainPage_->insertTopWidget(_aimId, _widget);
    }

    void MainWindow::removeTopWidget(const QString& _aimId)
    {
        if (mainPage_)
            mainPage_->removeTopWidget(_aimId);
    }

    void MainWindow::showSidebar(const QString& _aimId, int _page)
    {
        mainPage_->showSidebar(_aimId, _page);
    }

    void MainWindow::setSidebarVisible(bool _show)
    {
        mainPage_->setSidebarVisible(_show);
    }

    bool MainWindow::isSidebarVisible() const
    {
        return mainPage_->isSidebarVisible();
    }

    void MainWindow::restoreSidebar()
    {
        mainPage_->restoreSidebar();
    }

	bool MainWindow::nativeEventFilter(const QByteArray& _data, void* _message, long* _result)
	{
#ifdef _WIN32
		MSG* msg = (MSG*)(_message);
		if (msg->message == WM_NCHITTEST)
		{
			if (msg->hwnd != (HANDLE)winId())
			{
				return false;
			}

			int boxWidth = Utils::scale_value(SIZE_BOX_WIDTH);
			if (isMaximized())
			{
				*_result = HTCLIENT;
				return true;
			}

			int x = GET_X_LPARAM(msg->lParam);
			int y = GET_Y_LPARAM(msg->lParam);

			QPoint topLeft = QWidget::mapToGlobal(rect().topLeft());
			QPoint bottomRight = QWidget::mapToGlobal(rect().bottomRight());

			if (x <= topLeft.x() + boxWidth)
			{
				if (y <= topLeft.y() + boxWidth)
					*_result = HTTOPLEFT;
				else if (y >= bottomRight.y() - boxWidth)
					*_result = HTBOTTOMLEFT;
				else
					*_result = HTLEFT;
			}
			else if (x >= bottomRight.x() - boxWidth)
			{
				if (y <= topLeft.y() + boxWidth)
					*_result = HTTOPRIGHT;
				else if (y >= bottomRight.y() - boxWidth)
					*_result = HTBOTTOMRIGHT;
				else
					*_result = HTRIGHT;
			}
			else
			{
				if (y <= topLeft.y() + boxWidth)
					*_result = HTTOP;
				else if (y >= bottomRight.y() - boxWidth)
					*_result = HTBOTTOM;
				else
					*_result = HTCLIENT;
			}
			return true;
		}
		else if ((msg->message == WM_SYSCOMMAND && msg->wParam == SC_RESTORE && msg->hwnd == (HWND)winId()) || (msg->message == WM_SHOWWINDOW && msg->hwnd == (HWND)winId() && msg->wParam == TRUE))
		{
			setVisible(true);
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            trayIcon_->Hide();
            if (!SkipRead_ && isMainPage())
            {
			    Logic::getRecentsModel()->sendLastRead();
                Logic::getUnknownsModel()->sendLastRead();
            }
            if (isMainPage() && mainPage_)
            {
                mainPage_->notifyApplicationWindowActive(isActiveWindow());
            }
            if (!TaskBarIconHidden_)
                SkipRead_ = false;
            TaskBarIconHidden_ = false;

            if (msg->lParam == TITLE_CONTEXT_MENU_CMD)
                maximize();
		}
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_CLOSE)
        {
            hideWindow();
            return true;
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam  == SC_MINIMIZE)
        {
            minimize();
            return true;
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam  == SC_MAXIMIZE)
        {
            maximize();
            return true;
        }
        else if (msg->message == WM_WINDOWPOSCHANGING || msg->message == WM_WINDOWPOSCHANGED)
        {
            if (msg->hwnd != (HANDLE)winId())
            {
                return false;
            }

            WINDOWPOS* pos = (WINDOWPOS*)msg->lParam;
            if (pos->flags == 0x8170 || pos->flags == 0x8130)
            {
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                return false;
            }
            if (Shadow_)
            {
                if (!(pos->flags & SWP_NOSIZE) && !(pos->flags & SWP_NOMOVE) && !(pos->flags & SWP_DRAWFRAME))
                {
                    int shadowWidth = get_gui_settings()->get_shadow_width();
                    SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), pos->x - shadowWidth, pos->y - shadowWidth, pos->cx + shadowWidth * 2, pos->cy + shadowWidth * 2, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
                }
                else if (!(pos->flags & SWP_NOZORDER))
                {
                    UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE;
                    if (pos->flags & SWP_SHOWWINDOW)
                        flags |= SWP_SHOWWINDOW;
                    if (pos->flags & SWP_HIDEWINDOW)
                        flags |= SWP_HIDEWINDOW;

                    SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, flags);
                }
            }
        }
        else if (msg->message == WM_ACTIVATE)
        {
            if (!Shadow_)
            {
                return false;
            }

            const auto isInactivate = (msg->wParam == WA_INACTIVE);
            const auto isShadowWindow = (msg->hwnd == (HWND)Shadow_->winId());
            if (isShadowWindow && !isInactivate)
            {
                activate();
                return false;
            }
        }
        else if (msg->message == WM_DEVICECHANGE)
        {
            GetSoundsManager()->reinit();
        }
        else if (msg->message == WM_DISPLAYCHANGE)
        {
            checkPosition();
        }
        if (msg->message == WM_POWERBROADCAST)
        {
            if (msg->wParam == PBT_APMSUSPEND)
            {
                gotoSleep();
            }

            if (msg->wParam == PBT_APMRESUMESUSPEND)
            {
                gotoWake();
            }
        }
        if (msg->message == WM_WTSSESSION_CHANGE)
        {
            if (msg->wParam == WTS_SESSION_LOCK)
                gotoSleep();
            if (msg->wParam == WTS_SESSION_UNLOCK)
                gotoWake();
        }
        if (msg->message == WM_NCLBUTTONDOWN && msg->hwnd == (HWND)winId())
        {
            BYTE bFlag = 0;
            switch (msg->wParam)
            {
            case HTTOP:
                bFlag = WMSZ_TOP;
                break;

            case HTTOPLEFT:
                bFlag = WMSZ_TOPLEFT;
                break;

            case HTTOPRIGHT:
                bFlag = WMSZ_TOPRIGHT;
                break;

            case HTLEFT:
                bFlag = WMSZ_LEFT;
                break;

            case HTRIGHT:
                bFlag = WMSZ_RIGHT;
                break;

            case HTBOTTOM:
                bFlag = WMSZ_BOTTOM;
                break;

            case HTBOTTOMLEFT:
                bFlag = WMSZ_BOTTOMLEFT;
                break;

            case HTBOTTOMRIGHT:
                bFlag = WMSZ_BOTTOMRIGHT;
                break;
            }

            if (bFlag)
            {
                DefWindowProc(msg->hwnd, WM_SYSCOMMAND, (SC_SIZE | bFlag), msg->lParam);
                return true;
            }
        }
        if (msg->message == WM_SETCURSOR  && msg->hwnd == (HWND)winId())
        {
            LPCTSTR pszCur = NULL;
            switch (LOWORD(msg->lParam))
            {
            case HTTOP:
            case HTBOTTOM:
                pszCur = IDC_SIZENS;
                break;

            case HTTOPLEFT:
            case HTBOTTOMRIGHT:
                pszCur = IDC_SIZENWSE;
                break;

            case HTLEFT:
            case HTRIGHT:
                pszCur = IDC_SIZEWE;
                break;

            case HTTOPRIGHT:
            case HTBOTTOMLEFT:
                pszCur = IDC_SIZENESW;
                break;
            }

            if (pszCur)
            {
                SetCursor(LoadCursor(NULL, pszCur));
                return true;
            }
        }
#else

#ifdef __APPLE__
        return MacSupport::nativeEventFilter(_data, _message, _result);
#endif

#endif //_WIN32
		return false;
	}

    bool MainWindow::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
			if (stackedWidget_->currentWidget() == mainPage_)
			{
				auto keyEvent = (QKeyEvent*)(event);
				if (keyEvent && (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab))
				{
					if (keyEvent->modifiers() == Qt::NoModifier)
					{
						emit Utils::InterConnector::instance().searchEnd();

						if (mainPage_)
						{
							mainPage_->clearSearchFocus();
							mainPage_->setFocusOnInput();
						}

						return true;
					}
					else if (keyEvent->modifiers() == Qt::ShiftModifier)
					{
						return true;
					}
				}
			}
        }
        return false;
    }

    void MainWindow::enterEvent(QEvent* _event)
    {
        QMainWindow::enterEvent(_event);

#ifdef __APPLE__
        if (qApp->activeWindow() != this && (mainPage_ == nullptr || !mainPage_->isVideoWindowActive()))
        {
            qApp->setActiveWindow(this);
        }
#endif

        emit Utils::InterConnector::instance().historyControlPageFocusIn(Logic::getContactListModel()->selectedContact());
    }

    void MainWindow::leaveEvent(QEvent* _event)
    {
        QMainWindow::leaveEvent(_event);

#ifdef __APPLE__
//        if (qApp->activeWindow() == this)
//            qApp->setActiveWindow(nullptr);
#endif
    }

    void MainWindow::resizeEvent(QResizeEvent* _event)
	{
        if (isMaximized())
        {
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow();

            get_gui_settings()->set_value(settings_window_maximized, true);
        }
        else
        {
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow();

            QRect rc = Ui::get_gui_settings()->get_value(settings_main_window_rect, QRect());
            rc.setWidth(_event->size().width());
            rc.setHeight(_event->size().height());

            get_gui_settings()->set_value(settings_main_window_rect, rc);
            get_gui_settings()->set_value(settings_window_maximized, false);
        }
        // TODO : limit call this stats
        // GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::main_window_resize);

#ifdef __APPLE__
        getMacSupport()->updateMainMenu();
#endif
	}

	void MainWindow::moveEvent(QMoveEvent* _event)
	{
        if (!isMaximized())
        {
		    auto rc = Ui::get_gui_settings()->get_value<QRect>(settings_main_window_rect, QRect());
            rc.moveTo(pos());
            Ui::get_gui_settings()->set_value<QRect>(settings_main_window_rect, rc);
        }
	}

	void MainWindow::changeEvent(QEvent* _event)
	{
        if (_event->type() == QEvent::ActivationChange)
        {
            if (isActiveWindow())
            {
                trayIcon_->Hide();
                if (!SkipRead_ && isMainPage())
                {
                    Logic::getRecentsModel()->sendLastRead();
                    Logic::getUnknownsModel()->sendLastRead();
                }
                SkipRead_ = false;
            }

            if (Shadow_)
            {
                Shadow_->setActive(isActiveWindow());
            }

            if (mainPage_)
            {
                mainPage_->notifyApplicationWindowActive(isActiveWindow());
            }
        }

        if (_event->type() == QEvent::ApplicationStateChange)
        {
            if (Shadow_)
            {
                Shadow_->setActive(isActiveWindow());
            }
        }

        QMainWindow::changeEvent(_event);
	}

	void MainWindow::closeEvent(QCloseEvent* _event)
	{
        Previewer::CloseMedia();

        if (!platform::is_windows())
        {
            if (_event->spontaneous())
            {
                _event->ignore();

                hideWindow();
            }
        }
	}

    void MainWindow::keyPressEvent(QKeyEvent* _event)
    {
        QWidget* w = stackedWidget_->currentWidget();
        if (w && qobject_cast<MainPage*>(w) && mainPage_ && _event->matches(QKeySequence::Find))
        {
            mainPage_->setSearchFocus();
        }

#ifndef __APPLE__
        if (w && qobject_cast<MainPage*>(w) && _event->key() == Qt::Key_Escape)
        {
            if (mainPage_ && mainPage_->isMenuVisible())
                mainPage_->hideMenu();
            else if (mainPage_ && mainPage_->isSnapsPageVisible())
                mainPage_->snapsClose();
            else
                minimize();
        }
#else
        if (w && qobject_cast<MainPage*>(w) && _event->key() == Qt::Key_Escape)
        {
            if (mainPage_ && mainPage_->isMenuVisible())
                mainPage_->hideMenu();
            else if (mainPage_ && mainPage_->isSnapsPageVisible())
                mainPage_->snapsClose();
        }
#endif
        
#ifdef __linux__
        if (w && qobject_cast<MainPage*>(w) && _event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_Q)
            exit();
#endif //__linux__
        
        if (w && qobject_cast<MainPage*>(w) && _event->key() == Qt::Key_Space)
        {
            if (mainPage_ && mainPage_->isSnapsPageVisible())
                mainPage_->nextSnap();
        }
        
        if (w && qobject_cast<MainPage*>(w) && _event->key() == Qt::Key_Left)
        {
            if (mainPage_ && mainPage_->isSnapsPageVisible())
                mainPage_->prevUserSnap();
        }
        
        if (w && qobject_cast<MainPage*>(w) && _event->key() == Qt::Key_Right)
        {
            if (mainPage_ && mainPage_->isSnapsPageVisible())
                mainPage_->nextUserSnap();
        }

#ifdef _WIN32
        if (w && qobject_cast<MainPage*>(w) && mainPage_ && _event->matches(QKeySequence::NextChild))
        {
            mainPage_->nextChat();
        }

        if (w && qobject_cast<MainPage*>(w) && mainPage_ && _event->matches(QKeySequence::PreviousChild))
        {
            mainPage_->prevChat();
        }

        if (w && qobject_cast<MainPage*>(w) && _event->modifiers() == Qt::CTRL && _event->key() == Qt::Key_F4)
        {
            Logic::getRecentsModel()->hideChat(Logic::getContactListModel()->selectedContact());
        }

        if (mainPage_ && _event->key() == Qt::Key_Left && _event->modifiers() == Qt::AltModifier)
        {
            mainPage_->leftTab();
            setFocus(Qt::MouseFocusReason);
        }

        if (mainPage_ && _event->key() == Qt::Key_Right && _event->modifiers() == Qt::AltModifier)
        {
            mainPage_->rightTab();
            setFocus(Qt::MouseFocusReason);
        }

#endif //_WIN32

        QMainWindow::keyPressEvent(_event);
    }

    void MainWindow::setBackgroundPixmap(QPixmap& _pixmap, const bool _tiling)
    {
        Utils::check_pixel_ratio(_pixmap);
        if (_pixmap.isNull())
        {
            _pixmap = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_100.png"));
        }

		stackedWidget_->setImage(_pixmap, _tiling);
    }

    void MainWindow::initSizes()
    {
        auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());

        const int screenCount = QDesktopWidget().screenCount();

        bool intersects = false;

        for (int i = 0; i < screenCount; ++i)
        {
            QRect screenGeometry = QDesktopWidget().screenGeometry(i);

            QRect intersectedRect = screenGeometry.intersected(mainRect);

            if (intersectedRect.width() > Utils::scale_value(50) && intersectedRect.height() > Utils::scale_value(50))
            {
                intersects = true;
                break;
            }
        }

        if (!intersects)
        {
            mainRect = getDefaultWindowSize();
        }

        resize(mainRect.width(), mainRect.height());

        setMinimumHeight((qApp->desktop()->screenGeometry().height() >= Utils::scale_value(800)) ? getMinWindowHeight() : qApp->desktop()->screenGeometry().height() * 0.7);
        setMinimumWidth(getMinWindowWidth());

        

        if (mainRect.left() == 0 && mainRect.top() == 0)
        {
            QRect desktopRect = QDesktopWidget().availableGeometry(this);

            QPoint center = desktopRect.center();

            move(center.x() - width()*0.5, center.y()-height()*0.5);

            get_gui_settings()->set_value(settings_main_window_rect, geometry());
        }
        else
        {
            move(mainRect.left(), mainRect.top());
        }
    }

	void MainWindow::initSettings()
	{
        initSizes();

        bool isMaximized = get_gui_settings()->get_value<bool>(settings_window_maximized, false);
        
		isMaximized ? showMaximized() : show();
        Utils::ApplyStyle(maximizeButton_, isMaximized? Ui::CommonStyle::getRestoreButtonStyle() : Ui::CommonStyle::getMaximizeButtonStyle());
		maximizeButton_->setStyle(QApplication::style());

#ifdef _WIN32
        if (isMaximized)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

        WTSRegisterSessionNotification( (HWND)winId(), NOTIFY_FOR_THIS_SESSION);
#endif //_WIN32
	}

    void MainWindow::resize(int w, int h)
    {
        const auto screen = getScreen();
        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        if (screenGeometry.width() < w)
            w = screenGeometry.width();

        if (screenGeometry.height() < h)
            h = screenGeometry.height();

        return QMainWindow::resize(w, h);
    }

    void MainWindow::showMaximized()
    {
        QMainWindow::showNormal();
        IsMaximized_ = true;
        const auto screen = getScreen();
        const auto screenGeometry = QApplication::desktop()->availableGeometry(screen);
        setGeometry(screenGeometry);

        updateState();
    }

    void MainWindow::showNormal()
    {
        QMainWindow::showNormal();
        IsMaximized_ = false;
        auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());
        setGeometry(mainRect);

        updateState();
    }

    void MainWindow::updateState()
    {
        Utils::ApplyStyle(maximizeButton_, isMaximized() ? Ui::CommonStyle::getRestoreButtonStyle() : Ui::CommonStyle::getMaximizeButtonStyle());
        maximizeButton_->setStyle(QApplication::style());

        if (callPanelMainEx)
        {
            if (isMaximized())
            {
                callPanelMainEx->processOnWindowMaximized();
            }
            else
            {
                callPanelMainEx->processOnWindowNormalled();
            }
        }

        get_gui_settings()->set_value<bool>(settings_window_maximized, isMaximized());
    }

    bool MainWindow::isMaximized() const
    {
        return IsMaximized_;
    }

    void MainWindow::checkPosition()
    {
        QRect desktopRect;
        for (int i = 0; i < QDesktopWidget().screenCount(); ++i)
            desktopRect |= QDesktopWidget().availableGeometry(i);

        QRect main = rect();
        QPoint mainP = mapToGlobal(main.topLeft());
        main.moveTo(mainP);
        int y = main.y();
        if (y < desktopRect.y())
            maximize();
    }

    void MainWindow::gotoSleep()
    {
        GetDispatcher()->invokeStateAway();
    }

    void MainWindow::gotoWake()
    {
        GetDispatcher()->invokePreviousState();
    }

	void MainWindow::maximize()
	{
		if (isMaximized())
		{
			showNormal();
            auto mainRect = Ui::get_gui_settings()->get_value<QRect>(
                settings_main_window_rect,
                getDefaultWindowSize());

			resize(mainRect.width(), mainRect.height());
            move(mainRect.x(), mainRect.y() < 0 ? 0 : mainRect.y());
#ifdef _WIN32
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
		}
        else
        {
#ifdef _WIN32
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
			showMaximized();
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::main_window_fullscreen);
	}

    void MainWindow::minimize()
    {
        if (get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
        {
#ifdef _WIN32
            showMinimized();
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#elif __APPLE__
            MacSupport::minimizeWindow(winId());
#else
            showMinimized();
#endif //_WIN32
        }
        else
        {
            hideWindow();
        }

        if (mainPage_)
        {
            mainPage_->notifyApplicationWindowActive(false);
            mainPage_->hideMenu();
        }
    }

	void MainWindow::moveRequest(QPoint _point)
	{
		if (isMaximized())
			maximize();
		else
			move(_point);
	}

    void MainWindow::guiSettingsChanged(QString _key)
    {
        if (_key == settings_language || _key == settings_scale_coefficient)
        {
            //showLoginPage();
            //showMainPage();
        }
        else if (_key == settings_notify_new_mail_messages)
        {
            bool mailVisible = get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages,
                                                                   true)
                                && MyInfo()->haveConnectedEmail();
            setTitleIconsVisible(true, mailVisible);
        }
    }

    void MainWindow::clear_global_objects()
    {
        // delete main page
        if (mainPage_)
        {
			stackedWidget_->removeWidget(mainPage_);
            MainPage::reset();
            mainPage_ = 0;
        }

        Logic::ResetContactListModel();
        Logic::ResetRecentsModel();
        Logic::ResetUnknownsModel();
        Logic::ResetMessagesModel();
        Logic::ResetLiveChatsModel();
        Logic::GetSnapStorage()->clearStorage();

        Ui::GetDispatcher()->getVoipController().resetMaskManager();

        Ui::Stickers::resetCache();

        trayIcon_->forceUpdateIcon();
    }

    void MainWindow::showMigrateAccountPage(QString _accountId)
    {
#ifdef __APPLE__
        MacMigrationManager * manager = new MacMigrationManager(_accountId);

        if (manager->getProfiles().size() == 1)
        {
            manager->migrateProfile(manager->getProfiles()[0]);
            if (build::is_icq())
            {
                showMainPage();
            }
        }
        else
        {
            if (!accounts_page_)
            {
                accounts_page_ = new AccountsPage(this, manager);
                connect(accounts_page_, SIGNAL(account_selected()), this, SLOT(showMainPage()), Qt::QueuedConnection); // strict order
                accounts_page_->summon();
                stackedWidget_->addWidget(accounts_page_);
            }

			stackedWidget_->setCurrentWidget(accounts_page_);

            clear_global_objects();
        }

        delete manager;
#endif
    }

    void MainWindow::showLoginPage(const bool _is_auth_error)
    {
#ifdef __APPLE__
        getMacSupport()->createMenuBar(true);

        getMacSupport()->forceEnglishInputSource();

        if (!get_gui_settings()->get_value<bool>(settings_mac_accounts_migrated, false))
        {
            if (MacMigrationManager::canMigrateAccount() > 0)
            {
                mainPage_ = nullptr;
                MainPage::reset();
                showPromoPage();
                return;
            }
        }
#endif

        if (!loginPage_)
        {
            loginPage_ = new LoginPage(this, true /* is_login */);
            stackedWidget_->addWidget(loginPage_);

            connect(loginPage_, SIGNAL(loggedIn()), this, SLOT(showMainPage()), Qt::QueuedConnection);
        }

        setTitleIconsVisible(false, false);
        stackedWidget_->setCurrentWidget(loginPage_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_phone);

        if (_is_auth_error)
        {
            emit Utils::InterConnector::instance().authError(core::le_wrong_login);
        }

        clear_global_objects();
    }

	void MainWindow::showMainPage()
	{
        if (!mainPage_)
        {
            mainPage_ = MainPage::instance(this);
			stackedWidget_->addWidget(mainPage_);
        }
#ifdef __APPLE__
        getMacSupport()->createMenuBar(false);
#endif
		stackedWidget_->setCurrentWidget(mainPage_);
	}

    void MainWindow::showPromoPage()
    {
        initSizes();

        if (!promoPage_)
        {
            promoPage_ = new PromoPage(this);
			stackedWidget_->addWidget(promoPage_);
        }

#ifdef __APPLE__
        getMacSupport()->createMenuBar(true);
#endif
		stackedWidget_->setCurrentWidget(promoPage_);
    }

    void MainWindow::closePromoPage()
    {
#ifndef __APPLE__
        GetDispatcher()->post_message_to_core("close_promo", nullptr);
#endif

#ifdef __APPLE__
        const auto profiles = MacMigrationManager::canMigrateAccount();
        showMigrateAccountPage(profiles);
#endif
        
        promoPage_ = nullptr;
        
#ifdef __APPLE__
        if (!profiles.isEmpty())
        {
            return;
        }
#endif
        
        if (!get_gui_settings()->get_value(settings_keep_logged_in, true))// || !get_gui_settings()->contains_value(settings_keep_logged_in))
        {
            showLoginPage(false);
        }
        else
        {
            showMainPage();
        }
    }

    void MainWindow::checkForUpdates()
    {
#ifdef __APPLE__
        getMacSupport()->runMacUpdater();
#endif
    }

    void MainWindow::showIconInTaskbar(bool _show)
    {
        if (_show)
        {
            showTaskbarIcon();
        }
        else
        {
			hideTaskbarIcon();
        }
    }

    void MainWindow::copy()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            bool handled = false;

            if (platform::is_apple())
            {
                Ui::MessagesScrollArea * area = dynamic_cast<Ui::MessagesScrollArea*>(focused);

                if (area)
                {
                    QString text = area->getSelectedText();
#ifdef __APPLE__
                    MacSupport::replacePasteboard(text);
#endif

                    handled = true;
                }
            }

            if (!handled)
            {
                QApplication::postEvent( focused,
                                        new QKeyEvent( QEvent::KeyPress,
                                                      Qt::Key_C,
                                                      Qt::ControlModifier ));
                QApplication::postEvent( focused,
                                        new QKeyEvent( QEvent::KeyRelease,
                                                      Qt::Key_C,
                                                      Qt::ControlModifier ));
            }
        }
    }

    void MainWindow::cut()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_X,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_X,
                                                  Qt::ControlModifier ));
        }
    }

    void MainWindow::paste()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_V,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_V,
                                                  Qt::ControlModifier ));
        }
    }


    void MainWindow::undo()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier ));
        }
    }


    void MainWindow::redo()
    {
        QWidget* focused = QApplication::focusWidget();
        if( focused != 0 )
        {
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyPress,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier|Qt::ShiftModifier ));
            QApplication::postEvent( focused,
                                    new QKeyEvent( QEvent::KeyRelease,
                                                  Qt::Key_Z,
                                                  Qt::ControlModifier|Qt::ShiftModifier ));
        }
    }

    void MainWindow::activateSettings()
    {
        activate();
        mainPage_->hideMenu();
        mainPage_->settingsClicked();
    }

    void MainWindow::activateNextUnread()
    {
        activate();
        closePopups();
        mainPage_->recentsTabActivate(true);
    }

    void MainWindow::activateNextChat()
    {
        activate();
        mainPage_->recentsTabActivate(false);
        mainPage_->selectRecentChat(Logic::getRecentsModel()->nextAimId(Logic::getContactListModel()->selectedContact()));
    }

    void MainWindow::activatePrevChat()
    {
        activate();
        mainPage_->recentsTabActivate(false);
        mainPage_->selectRecentChat(Logic::getRecentsModel()->prevAimId(Logic::getContactListModel()->selectedContact()));
    }

    void MainWindow::activateContactSearch()
    {
        activate();
        mainPage_->hideMenu();
        mainPage_->onAddContactClicked();
    }

    void MainWindow::activateAbout()
    {
        activate();
        mainPage_->hideMenu();
        mainPage_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_About);
    }

    void MainWindow::activateProfile()
    {
        activate();
        mainPage_->hideMenu();
        mainPage_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Profile);
    }

    void MainWindow::closeCurrent()
    {
        activate();
        mainPage_->hideMenu();
        if (platform::is_apple())
        {
            auto selectedContact = Logic::getContactListModel()->selectedContact();
            if (selectedContact.isEmpty())
            {
                hideWindow();
            }
            else
            {
                Logic::getContactListModel()->setCurrent("", -1, true);
            }
        }
        else
        {
            Logic::getRecentsModel()->hideChat(Logic::getContactListModel()->selectedContact());
        }
    }

    void MainWindow::toggleFullScreen()
    {
#ifdef __APPLE__
        if (!Utils::InterConnector::instance().isDragOverlay())
            MacSupport::toggleFullScreen();
#endif
    }

    void MainWindow::updateMainMenu()
    {
#ifdef __APPLE__
        getMacSupport()->updateMainMenu();
#endif
    }

    void MainWindow::exit()
    {
#ifdef STRIP_VOIP
        QApplication::exit();
#else

#ifdef _WIN32
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif

        Ui::GetDispatcher()->getVoipController().voipReset();
#endif //STRIP_VOIP
    }

    void MainWindow::onVoipResetComplete()
    {
        QApplication::exit();
	}

    void MainWindow::hideWindow()
    {
        TaskBarIconHidden_ = true;
        hideMenu();

#if defined __APPLE__
        MacSupport::closeWindow(winId());
#elif defined (_WIN32)
        ShowWindow((HWND)winId(), SW_HIDE);
#else
        hide();
#endif

#ifdef _WIN32
        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
    }

    void MainWindow::pasteEmoji()
    {
        if (!Logic::getContactListModel()->selectedContact().isEmpty())
        {
#ifdef __APPLE__
            MacSupport::showEmojiPanel();
#else
            getMainPage()->getContactDialog()->onSmilesMenu();
#endif
        }
    }

    void MainWindow::onVoipCallIncomingAccepted(const voip_manager::ContactEx& /*contact_ex*/)
    {
        assert(callPanelMainEx);
        if (callPanelMainEx)
        {
            callPanelMainEx->show();
            if (titleWidget_)
            {
                titleWidget_->hide();
            }
        }
    }

    void MainWindow::onVoipCallCreated(const voip_manager::ContactEx& _cont)
    {
        if (!_cont.incoming && _cont.connection_count >= 1)
        {
            assert(callPanelMainEx);
            if (callPanelMainEx)
            {
                callPanelMainEx->show();
                if (titleWidget_)
                {
                    titleWidget_->hide();
                }
            }
        }
    }

    void MainWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        if (_contactEx.connection_count <= 1)
        { // in this moment destroyed call is active, e.a. call_count + 1
            assert(callPanelMainEx);
            if (callPanelMainEx)
            {
                callPanelMainEx->hide();
                if (titleWidget_ && !platform::is_apple())
                {
                    titleWidget_->show();
                }
            }
        }
    }

	void MainWindow::onShowVideoWindow()
	{
		if (mainPage_)
		{
			mainPage_->showVideoWindow();
		}
	}
    
    void MainWindow::onMyInfoReceived()
    {
        bool mailVisible = get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages,
                                                               true)
                            && MyInfo()->haveConnectedEmail();
        setTitleIconsVisible(true, mailVisible);
    }
    
#ifdef __APPLE__
    MacSupport* MainWindow::getMacSupport()
    {
        static MacSupport mac_support(this);
        
        return &mac_support;
    }
#endif //__APPPLE__
}
