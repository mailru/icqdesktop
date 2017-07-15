#include "stdafx.h"
#include "TrayIcon.h"

#include "RecentMessagesAlert.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../sounds/SoundsManager.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"


#ifdef _WIN32
#   include "toast_notifications/win32/ToastManager.h"
typedef HRESULT (__stdcall *QueryUserNotificationState)(QUERY_USER_NOTIFICATION_STATE *pquns);
typedef BOOL (__stdcall *QuerySystemParametersInfo)(__in UINT uiAction, __in UINT uiParam, __inout_opt PVOID pvParam, __in UINT fWinIni);
#else

#ifdef __APPLE__
#   include "notification_center/macos/NotificationCenterManager.h"
#   include "../../utils/macos/mac_support.h"
#endif

#endif //_WIN32

#ifdef _WIN32
#   include <Shobjidl.h>
extern HICON qt_pixmapToWinHICON(const QPixmap &p);
QString pinPath = "\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\ICQ.lnk";
#endif // _WIN32

namespace
{
    int init_mail_timeout = 5000;

#ifdef _WIN32
    HICON createHIconFromQIcon(const QIcon &icon, int xSize, int ySize) {
        if (!icon.isNull()) {
            const QPixmap pm = icon.pixmap(icon.actualSize(QSize(xSize, ySize)));
            if (!pm.isNull()) {
                return qt_pixmapToWinHICON(pm);
            }
        }
        return nullptr;
    }
#endif //_WIN32
}

namespace Ui
{
    TrayIcon::TrayIcon(MainWindow* parent)
        : QObject(parent)
        , MainWindow_(parent)
        , systemTrayIcon_(new QSystemTrayIcon(this))
        , emailSystemTrayIcon_(nullptr)
        , Menu_(new ContextMenu(0))
        , MessageAlert_(new RecentMessagesAlert(new Logic::RecentItemDelegate(this), false))
        , MailAlert_(new  RecentMessagesAlert(new Logic::RecentItemDelegate(this), true))
        , Base_(build::is_icq() ?
            ":/resources/main_window/appicon.ico" :
            ":/resources/main_window/appicon_agent.ico")
        , Unreads_(build::is_icq() ?
            ":/resources/main_window/appicon_unread.ico" :
            ":/resources/main_window/appicon_unread_agent.ico")
#ifdef _WIN32
        , TrayBase_(build::is_icq() ?
            ":/resources/main_window/appicon_tray.ico" :
            ":/resources/main_window/appicon_tray_agent.ico")
        , TrayUnreads_(build::is_icq() ? 
            ":/resources/main_window/appicon_tray_unread.ico" :
            ":/resources/main_window/appicon_tray_unread_agent.ico")
#else
        , TrayBase_(build::is_icq() ?
            ":/resources/main_window/appicon.ico":
            ":/resources/main_window/appicon_agent.ico")
        , TrayUnreads_(build::is_icq() ?
            ":/resources/main_window/appicon_unread.ico" :
            ":/resources/main_window/appicon_unread_agent.ico")
#endif //_WIN32
#ifdef _WIN32
        , ptbl(0)
        , first_start_(true)
        , UnreadsCount_(0)
#endif //_WIN32
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
            if (FAILED(hr))
                ptbl = 0;
        }
#endif //_WIN32
        init();

        InitMailStatusTimer_ = new QTimer(this);
        InitMailStatusTimer_->setInterval(init_mail_timeout);
        InitMailStatusTimer_->setSingleShot(true);

        connect(Ui::GetDispatcher(), SIGNAL(im_created()), this, SLOT(loggedIn()), Qt::QueuedConnection);
        connect(MessageAlert_, SIGNAL(messageClicked(QString, QString)), this, SLOT(messageClicked(QString, QString)), Qt::QueuedConnection);
        connect(MailAlert_, SIGNAL(messageClicked(QString, QString)), this, SLOT(messageClicked(QString, QString)), Qt::QueuedConnection);
        connect(MessageAlert_, SIGNAL(changed()), this, SLOT(updateAlertsPosition()), Qt::QueuedConnection);
        connect(MailAlert_, SIGNAL(changed()), this, SLOT(updateAlertsPosition()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contactChanged(QString)), this, SLOT(updateIcon()), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(myInfo()), this, SLOT(myInfo()));
        connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &TrayIcon::loggedOut, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(historyControlPageFocusIn(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(selectedContactChanged(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), SIGNAL(readStateChanged(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), SIGNAL(readStateChanged(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(activeDialogHide(QString)), this, SLOT(clearNotifications(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(mailStatus(QString, unsigned, bool)), this, SLOT(mailStatus(QString, unsigned, bool)), Qt::QueuedConnection);

#ifdef __APPLE__
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, [this]()
        {
            clearNotifications("mail");
        });
#endif

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, &TrayIcon::mailBoxOpened, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &TrayIcon::logout, Qt::QueuedConnection);
#ifdef __APPLE__
        ncSupported(); //setup notification manager
        setVisible(get_gui_settings()->get_value(settings_show_in_menubar, true));
#endif //__APPLE__

    }

    TrayIcon::~TrayIcon()
    {
        disconnect(get_gui_settings());
    }

    void TrayIcon::openMailBox(const QString& _mailId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("email", Email_);

        Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [this, _mailId](core::icollection* _collection)
        {
            Utils::openMailBox(Email_, Ui::gui_coll_helper(_collection, false).get_value_as_string("key"), _mailId);
        });
    }

    void TrayIcon::Hide()
    {
        MessageAlert_->hide();
        MessageAlert_->markShowed();
        forceUpdateIcon();
    }

    void TrayIcon::forceUpdateIcon()
    {
        UnreadsCount_ = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
#ifdef _WIN32
        if (ptbl)
        {
            QIcon iconOverlay;
            if (UnreadsCount_ > 0)
            {
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(16, UnreadsCount_, QColor("#f23c34"), Qt::white)));
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(32, UnreadsCount_, QColor("#f23c34"), Qt::white)));
            }
            ptbl->SetOverlayIcon((HWND)MainWindow_->winId(), UnreadsCount_ > 0 ? createHIconFromQIcon(iconOverlay, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) : HICON(), L"");
        }
        else
        {
            MainWindow_->setWindowIcon(UnreadsCount_ > 0 ? Unreads_ : Base_);
        }
#endif //_WIN32

        if (!platform::is_apple())
        {
            if (UnreadsCount_ > 0)
            {
                QIcon iconOverlay;
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(16, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(16, 16)).toImage())));
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(32, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(32, 32)).toImage())));
                systemTrayIcon_->setIcon(iconOverlay);
            }
            else
            {
                systemTrayIcon_->setIcon(TrayBase_);
            }
        }
        else
        {
            setMacIcon();
        }

        updateStatus();
    }

    void TrayIcon::setVisible(bool visible)
    {
        systemTrayIcon_->setVisible(visible);
    }

    void TrayIcon::setMacIcon()
    {
#ifdef __APPLE__
        QString state = MyInfo()->state().toLower();

        if (state != "dnd" &&
            state != "invisible" &&
            state != "offline")

        {
            state = "online";
        }


        bool unreads = (Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads()) != 0;
        QString iconResource(QString(":/resources/main_window/mac_tray/icq_osxlogo_%1_%2%3_100.png").
            arg(state).arg(MacSupport::currentTheme()).
            arg(unreads && (state != "offline") ? "_unread" : ""));

        QIcon icon(Utils::parse_image_name(iconResource));
        systemTrayIcon_->setIcon(icon);
#endif
    }

    void TrayIcon::myInfo()
    {
        setMacIcon();
        updateStatus();
    }

    void TrayIcon::updateAlertsPosition()
    {
        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = screenMarginX;
        int screenMarginYMail = screenMarginY;
        if (MessageAlert_->isVisible())
            screenMarginYMail += (MessageAlert_->height() - Utils::scale_value(12));

        switch (pos)
        {
        case Ui::TOP_RIGHT:
            MessageAlert_->move(availableGeometry.topRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            MailAlert_->move(availableGeometry.topRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginYMail);
            break;

        case Ui::BOTTOM_LEFT:
            MessageAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MessageAlert_->height() - screenMarginY);
            MailAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case Ui::BOTOOM_RIGHT:
            MessageAlert_->move(availableGeometry.bottomRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MessageAlert_->height() - screenMarginY);
            MailAlert_->move(availableGeometry.bottomRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case Ui::TOP_LEFT:
        default:
            MessageAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            MailAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginYMail);
            break;
        }
    }

    void TrayIcon::updateStatus()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }

        onlineAction_->setCheckable(true);
        dndAction_->setCheckable(true);
        invAction_->setCheckable(true);

        auto state = MyInfo()->state().toLower();
        if (state == "invisible")
        {
            onlineAction_->setChecked(false);
            dndAction_->setChecked(false);
            invAction_->setChecked(true);
        }
        else if (state == "dnd")
        {
            onlineAction_->setChecked(false);
            dndAction_->setChecked(true);
            invAction_->setChecked(false);
        }
        else
        {
            onlineAction_->setChecked(true);
            dndAction_->setChecked(false);
            invAction_->setChecked(false);
        }
    }

    void TrayIcon::updateIcon()
    {
        auto count = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
        if (count != UnreadsCount_)
        {
            UnreadsCount_ = count;
#ifdef _WIN32
            if (ptbl)
            {
                QIcon iconOverlay;
                if (UnreadsCount_ > 0)
                {
                    iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(16, UnreadsCount_, QColor("#f23c34"), Qt::white)));
                    iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(32, UnreadsCount_, QColor("#f23c34"), Qt::white)));
                }
                ptbl->SetOverlayIcon((HWND)MainWindow_->winId(), UnreadsCount_ > 0 ? createHIconFromQIcon(iconOverlay, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) : HICON(), L"");
            }
            else
            {
                MainWindow_->setWindowIcon(UnreadsCount_ > 0 ? Unreads_ : Base_);
            }
#endif //_WIN32

            if (!platform::is_apple())
            {
                if (UnreadsCount_ > 0)
                {
                    QIcon iconOverlay;
                    iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(16, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(16, 16)).toImage())));
                    iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(32, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(32, 32)).toImage())));
                    systemTrayIcon_->setIcon(iconOverlay);
                }
                else
                {
                    systemTrayIcon_->setIcon(TrayBase_);
                }
            }
            else
            {
                setMacIcon();
            }
        }

        updateStatus();
    }

    void TrayIcon::clearNotifications(QString aimId)
    {
        if (aimId.isEmpty() || !Notifications_.contains(aimId))
            return;
        
        Notifications_.removeAll(aimId);
#if defined (_WIN32)
        if (toastSupported())
            ToastManager_->HideNotifications(aimId);
#elif defined (__APPLE__)
        if (ncSupported())
        {
            NotificationCenterManager_->HideNotifications(aimId);
        }
#endif //_WIN32
    }

    void TrayIcon::dlgStates(std::shared_ptr<QList<Data::DlgState>> _states)
    {
        for (auto _state : *_states)
        {
            bool canNotify = _state.Visible_ && (!ShowedMessages_.contains(_state.AimId_) || (_state.LastMsgId_ != -1 && ShowedMessages_[_state.AimId_] < _state.LastMsgId_));
            if (_state.GetText().isEmpty())
                canNotify = false;

            if (Logic::getUnknownsModel()->contactIndex(_state.AimId_).isValid())
                canNotify = false;

            if (!_state.Outgoing_)
            {
                if (_state.UnreadCount_ != 0 && canNotify && !Logic::getContactListModel()->isMuted(_state.AimId_))
                {
                    ShowedMessages_[_state.AimId_] = _state.LastMsgId_;
                    if (canShowNotifications(_state.AimId_ == "mail"))
                        showMessage(_state);
#ifdef __APPLE__
                    if (!MainWindow_->isActive() || MacSupport::previewIsShown())
#else
                    if (!MainWindow_->isActive())
#endif //__APPLE__
                        GetSoundsManager()->playIncomingMessage();
                }
            }

            if (_state.Visible_)
                markShowed(_state.AimId_);

        }

        updateIcon();

#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads());
#endif

    }

    void TrayIcon::newMail(QString email, QString from, QString subj, QString id)
    {
        Email_ = email;
        if (canShowNotifications(true))
        {
            Data::DlgState state;
            state.AimId_ = "mail";
            state.Friendly_ = from;
            state.Email_ = Email_;
            state.MailId_ = id;
            state.SetText(subj);
            showMessage(state);
            GetSoundsManager()->playIncomingMail();
        }

        showEmailIcon();
    }

    void TrayIcon::mailStatus(QString email, unsigned count, bool init)
    {
        Email_ = email;

        if (first_start_)
        {
            bool visible = !!count;
            if (visible)
            {
                showEmailIcon();
            }
            else
            {
                hideEmailIcon();
            }

            first_start_ = false;
        }
        else if (!count)
        {
            hideEmailIcon();
        }

        if (canShowNotifications(true))
        {
            if (!init && !InitMailStatusTimer_->isActive() && !MailAlert_->isVisible())
                return;

            if (count == 0 && init)
            {
                InitMailStatusTimer_->start();
                return;
            }

            Data::DlgState state;
            state.AimId_ = "mail";
            state.Email_ = Email_;
            state.Friendly_ = QT_TRANSLATE_NOOP("tray_menu", "New email");
            state.SetText(QVariant(count).toString() + Utils::GetTranslator()->getNumberString(count, 
                QT_TRANSLATE_NOOP3("tray_menu", " new email", "1"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "2"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "5"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "21")
                ));

            if (MailAlert_->updateMailStatusAlert(state))
            {
                if (count == 0)
                    MailAlert_->hide();
                return;
            }
            else if (MailAlert_->isVisible())
            {
                return;
            }

            showMessage(state);
        }
    }

    void TrayIcon::messageClicked(QString aimId, QString mailId)
    {
#if defined (_WIN32)
        if (!toastSupported())
#elif defined (__APPLE__)
        if (!ncSupported())
#endif //_WIN32
        {
            if (aimId == "mail")
            {
                MailAlert_->hide();
                MailAlert_->markShowed();
            }
            else
            {
                MessageAlert_->hide();
                MessageAlert_->markShowed();
            }
        }

        if (!aimId.isEmpty())
        {
            auto w = MainWindow_->getMainPage();
            if (w)
                w->snapsClose();

            if (aimId == "mail")
            {
                GetDispatcher()->post_stats_to_core(mailId.isEmpty() ? core::stats::stats_event_names::alert_mail_common : core::stats::stats_event_names::alert_mail_letter);

                openMailBox(mailId);

                return;

            }
            if (Logic::getContactListModel()->selectedContact() != aimId)
                Utils::InterConnector::instance().getMainWindow()->skipRead();
            Logic::getContactListModel()->setCurrent(aimId, -1, true, true);
        }

        MainWindow_->activateFromEventLoop();
        MainWindow_->hideMenu();
        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_click);
    }

    void TrayIcon::showMessage(const Data::DlgState& state)
    {
        Notifications_ << state.AimId_;
#if defined (_WIN32)
        if (toastSupported())
        {
            ToastManager_->DisplayToastMessage(state.AimId_, state.GetText());
            return;
        }
#elif defined (__APPLE__)
        if (ncSupported())
        {
            auto displayName = state.AimId_ == "mail" ? state.Friendly_ : Logic::getContactListModel()->getDisplayName(state.AimId_);
            NotificationCenterManager_->DisplayNotification(state.AimId_, state.senderNick_, state.GetText(), state.MailId_, displayName);
            return;
        }
#endif //_WIN32
        const auto isMail = state.AimId_ == "mail";
        auto alert = isMail ? MailAlert_ : MessageAlert_;
        alert->addAlert(state);

        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        if (isMail && MessageAlert_->isVisible())
            screenMarginY += (MessageAlert_->height() - Utils::scale_value(12));

        switch (pos)
        {
        case Ui::TOP_RIGHT:
            alert->move(availableGeometry.topRight().x() - alert->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            break;

        case Ui::BOTTOM_LEFT:
            alert->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - alert->height() - screenMarginY);
            break;

        case Ui::BOTOOM_RIGHT:
            alert->move(availableGeometry.bottomRight().x() - alert->width() - screenMarginX, availableGeometry.bottomRight().y() - alert->height() - screenMarginY);
            break;

        case Ui::TOP_LEFT:
        default:
            alert->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            break;
        }

        alert->show();
        if (Menu_->isVisible())
            Menu_->raise();
    }

    TrayPosition TrayIcon::getTrayPosition() const
    {
        QRect availableGeometry = QDesktopWidget().availableGeometry();
        QRect iconGeometry = systemTrayIcon_->geometry();

        QString ag = QString("availableGeometry x: %1, y: %2, w: %3, h: %4 ").arg(availableGeometry.x()).arg(availableGeometry.y()).arg(availableGeometry.width()).arg(availableGeometry.height());
        QString ig = QString("iconGeometry x: %1, y: %2, w: %3, h: %4").arg(iconGeometry.x()).arg(iconGeometry.y()).arg(iconGeometry.width()).arg(iconGeometry.height());
        Log::trace("tray", ag + ig);

#ifdef __linux__
        if (iconGeometry.isEmpty())
            return TOP_RIGHT;
#endif //__linux__

        bool top = abs(iconGeometry.y() - availableGeometry.topLeft().y()) < abs(iconGeometry.y() - availableGeometry.bottomLeft().y());
        if (abs(iconGeometry.x() - availableGeometry.topLeft().x()) < abs(iconGeometry.x() - availableGeometry.topRight().x()))
            return top ? TOP_LEFT : BOTTOM_LEFT;
        else
            return top ? TOP_RIGHT : BOTOOM_RIGHT;
    }

    void TrayIcon::initEMailIcon()
    {
#ifdef __APPLE__
        emailIcon_ = QIcon(Utils::parse_image_name(":/resources/main_window/mac_tray/email_100.png"));
#else
        emailIcon_ = QIcon(":/resources/main_window/tray_email.ico");
#endif //__APPLE__
    }

    void TrayIcon::showEmailIcon()
    {
        if (platform::is_apple() || emailSystemTrayIcon_ || !canShowNotifications(true))
            return;

        emailSystemTrayIcon_ = new QSystemTrayIcon(this);
        emailSystemTrayIcon_->setIcon(emailIcon_);
        emailSystemTrayIcon_->setVisible(true);
        connect(emailSystemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::onEmailIconClick, Qt::QueuedConnection);
    }

    void TrayIcon::hideEmailIcon()
    {
        if (platform::is_apple() || !emailSystemTrayIcon_)
            return;

        emailSystemTrayIcon_->setVisible(false);
        delete emailSystemTrayIcon_;
        emailSystemTrayIcon_ = nullptr;
    }

    void TrayIcon::init()
    {
        MessageAlert_->hide();
        MailAlert_->hide();

        MainWindow_->setWindowIcon(Base_);
        if (!platform::is_apple())
        {
            if (UnreadsCount_ > 0)
            {
                QIcon iconOverlay;
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(16, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(16, 16)).toImage())));
                iconOverlay.addPixmap(QPixmap::fromImage(Utils::iconWithCounter(32, UnreadsCount_, QColor("#f23c34"), Qt::white, TrayBase_.pixmap(QSize(32, 32)).toImage())));
                systemTrayIcon_->setIcon(iconOverlay);
            }
            else
            {
                systemTrayIcon_->setIcon(TrayBase_);
            }
            systemTrayIcon_->setToolTip(build::is_icq() ? "ICQ" : "Mail.Ru Agent");
        }
        else
        {
            setMacIcon();
        }

        initEMailIcon();

        onlineAction_ = dndAction_ = invAction_ = NULL;


#ifdef __APPLE__

        onlineAction_ = Menu_->addAction(
            QIcon(build::is_icq()
            ? ":/resources/statuses/status_online_200.png"
            : ":/resources/statuses/status_online_agent_200.png"),
            QT_TRANSLATE_NOOP("tray_menu", "Online"), this, SLOT(menuStateOnline()));
        dndAction_ = Menu_->addAction(
            QIcon(build::is_icq()
            ? ":/resources/statuses/status_dnd_200.png"
            : ":/resources/statuses/status_dnd_agent_200.png"),
            QT_TRANSLATE_NOOP("tray_menu", "Do not disturb"), this, SLOT(menuStateDoNotDisturb()));
        invAction_ = Menu_->addAction(
            QIcon(build::is_icq()
            ? ":/resources/statuses/status_invisible_200.png"
            : ":/resources/statuses/status_invisible_agent_200.png"),
            QT_TRANSLATE_NOOP("tray_menu", "Invisible"), this, SLOT(menuStateInvisible()));

        updateStatus();

        Menu_->addSeparator();

        //#ifdef UPDATES
        //        Menu_->addAction(QIcon(), QT_TRANSLATE_NOOP("context_menu", "Check for updates..."), parent(), SLOT(checkForUpdates()));
        //        Menu_->addSeparator();
        //#endif
        Menu_->addAction(QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#else
#ifdef __linux__
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Open"), parent(), SLOT(activate()));
        Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#else
        Menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/quit_100.png")), QT_TRANSLATE_NOOP("tray_menu","Quit"), parent(), SLOT(exit()));
#endif //__linux__
#endif

        systemTrayIcon_->setContextMenu(Menu_);
        connect(systemTrayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activated(QSystemTrayIcon::ActivationReason)), Qt::QueuedConnection);
        systemTrayIcon_->show();
    }

    void TrayIcon::markShowed(const QString& aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", aimId);
        Ui::GetDispatcher()->post_message_to_core("dlg_state/hide", collection.get());
    }

    bool TrayIcon::canShowNotificationsWin() const
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
        {
            static QueryUserNotificationState query;
            if (!query)
            {
                HINSTANCE shell32 = LoadLibraryW(L"shell32.dll");
                if (shell32)
                {
                    query = (QueryUserNotificationState)GetProcAddress(shell32, "SHQueryUserNotificationState");
                }
            }

            if (query)
            {
                QUERY_USER_NOTIFICATION_STATE state;
                if (query(&state) == S_OK && state != QUNS_ACCEPTS_NOTIFICATIONS)
                    return false;
            }
        }
#endif //_WIN32
        return true;
    }

    void TrayIcon::mailBoxOpened()
    {
        hideEmailIcon();
    }

    void TrayIcon::logout()
    {
        first_start_ = true;

        hideEmailIcon();
    }

    void TrayIcon::onEmailIconClick(QSystemTrayIcon::ActivationReason)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::tray_mail);

        openMailBox(QString());
    }

    bool TrayIcon::canShowNotifications(bool isMail) const
    {
        // TODO: must be based on the type of notification - is it message, birthday-notify or contact-coming-notify.
        if (!get_gui_settings()->get_value<bool>(isMail ? settings_notify_new_mail_messages : settings_notify_new_messages, true))  return false;

#ifdef _WIN32
        if (QSysInfo().windowsVersion() == QSysInfo::WV_XP)
        {
            static QuerySystemParametersInfo query;
            if (!query)
            {
                HINSTANCE user32 = LoadLibraryW(L"user32.dll");
                if (user32)
                {
                    query = (QuerySystemParametersInfo)GetProcAddress(user32, "SystemParametersInfoW");
                }
            }

            if (query)
            {
                BOOL result = FALSE;      
                if (query(SPI_GETSCREENSAVERRUNNING, 0, &result, 0) && result)
                    return false;
            }
        }
        else
        {
            if (!canShowNotificationsWin())
                return false;
        }

        if (isMail)
            return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages();

#endif //_WIN32
#ifdef __APPLE__
        return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages() && (!MainWindow_->isActive() || MacSupport::previewIsShown());
#else
        return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages() && !MainWindow_->isActive();
#endif
    }

    void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
    {
        if (platform::is_windows())
        {
            if (reason == QSystemTrayIcon::Trigger)
                MainWindow_->activate();
        }

        updateStatus();
    }

    void TrayIcon::loggedIn()
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &TrayIcon::dlgStates, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &TrayIcon::updateIcon, Qt::QueuedConnection);        
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &TrayIcon::dlgStates, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, &TrayIcon::updateIcon, Qt::QueuedConnection);       
        connect(Ui::GetDispatcher(), SIGNAL(newMail(QString, QString, QString, QString)), this, SLOT(newMail(QString, QString, QString, QString)), Qt::QueuedConnection);
    }

    void TrayIcon::loggedOut(const bool _is_auth_error)
    {
#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(0);
#endif
    }

#if defined (_WIN32)
    bool TrayIcon::toastSupported()
    {
        return false;
        /*
        if (QSysInfo().windowsVersion() > QSysInfo::WV_WINDOWS8_1)
        {
        if (!ToastManager_.get())
        {
        ToastManager_.reset(new ToastManager());
        connect(ToastManager_.get(), SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
        }
        return true;
        }
        return false;
        */
    }
#elif defined (__APPLE__)
    bool TrayIcon::ncSupported()
    {
        if (QSysInfo().macVersion() > QSysInfo::MV_10_7)
        {
            if (!NotificationCenterManager_.get())
            {
                NotificationCenterManager_.reset(new NotificationCenterManager());
                connect(NotificationCenterManager_.get(), SIGNAL(messageClicked(QString, QString)), this, SLOT(messageClicked(QString, QString)), Qt::QueuedConnection);
                connect(NotificationCenterManager_.get(), SIGNAL(osxThemeChanged()), this, SLOT(myInfo()), Qt::QueuedConnection);
            }
            return true;
        }
        return false;
    }
#endif //_WIN32

    void TrayIcon::menuStateOnline()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }

        dndAction_->setChecked(false);
        invAction_->setChecked(false);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "online");
        collection.set_value_as_string("aimid",  MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
    void TrayIcon::menuStateDoNotDisturb()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }

        onlineAction_->setChecked(false);
        dndAction_->setChecked(false);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "dnd");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
    void TrayIcon::menuStateInvisible()
    {
        if (!onlineAction_ ||
            !dndAction_ ||
            !invAction_)
        {
            return;
        }

        onlineAction_->setChecked(false);
        dndAction_->setChecked(false);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("state", "invisible");
        collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
        Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    }
}
