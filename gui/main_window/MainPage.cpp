#include "stdafx.h"
#include "MainPage.h"

#include "ContactDialog.h"
#include "GroupChatOperations.h"
#include "IntroduceYourself.h"
#include "TitleBar.h"
#include "livechats/LiveChatsHome.h"
#include "livechats/LiveChatProfile.h"
#include "contact_list/ChatMembersModel.h"
#include "contact_list/Common.h"
#include "contact_list/ContactList.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/SearchModelDLG.h"
#include "contact_list/UnknownsModel.h"
#include "contact_list/TopPanel.h"
#include "contact_list/SelectionContactsForGroupChat.h"
#include "contact_list/SnapItemDelegate.h"
#include "livechats/LiveChatsModel.h"
#include "history_control/MessagesModel.h"
#include "search_contacts/SearchContactsWidget.h"
#include "settings/GeneralSettingsWidget.h"
#include "settings/SettingsTab.h"
#include "sidebar/Sidebar.h"
#include "settings/SettingsThemes.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../my_info.h"
#include "../controls/BackButton.h"
#include "../controls/CommonStyle.h"
#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/WidgetsNavigator.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/HorScrollableView.h"
#include "../controls/TransparentScrollBar.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../utils/log/log.h"
#include "../voip/IncomingCallWindow.h"
#include "../voip/VideoWindow.h"
#include "../cache/snaps/SnapStorage.h"
#include "MainWindow.h"
#include "SnapsPage.h"
#include "MainMenu.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const int balloon_size = 20;
    const int unreads_padding = 6;
    const int counter_padding = 4;
    const int back_spacing = 16;
    const int unreads_minimum_extent = balloon_size;
    const int min_step = 0;
    const int max_step = 100;
    const int BACK_WIDTH = 52;
    const int BACK_HEIGHT = 48;
    const int MENU_WIDTH = 240;
    const int CL_POPUP_WIDTH = 360;
    const int ANIMATION_DURATION = 200;
    const int SNAP_VIEW_HEIGHT = 116;
    const int TOP_WIDGET_HEIGHT = 52;
}

namespace Ui
{
    UnreadsCounter::UnreadsCounter(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(Logic::getRecentsModel(), SIGNAL(updated()), this, SLOT(update()), Qt::QueuedConnection);
    }

    void UnreadsCounter::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        const auto borderColor = Ui::CommonStyle::getTopPanelColor();
        const auto bgColor = QColor("#579e1c");
        const auto textColor = QColor("#ffffff");
        Utils::drawUnreads(
            &p,
            Fonts::appFontScaled(13, Fonts::FontWeight::Medium),
            &bgColor,
            &textColor,
            &borderColor,
            Logic::getRecentsModel()->totalUnreads(),
            Utils::scale_value(balloon_size), 0, 0
            );
    }

    HeaderBack::HeaderBack(QWidget* _parent)
        : QPushButton(_parent)
    {

    }

    void HeaderBack::paintEvent(QPaintEvent* e)
    {
        QPushButton::paintEvent(e);
        QPainter p(this);
        QPixmap pix(Utils::parse_image_name(":/resources/basic_elements/contr_basic_back_100.png"));
        Utils::check_pixel_ratio(pix);
        double ratio = Utils::scale_bitmap(1);
        
        p.drawPixmap(QPoint(0, height() / 2 - pix.height() / 2 / ratio), pix);
    }
    
    void HeaderBack::resizeEvent(QResizeEvent* e)
    {
        emit resized();
        QPushButton::resizeEvent(e);
    }

    UnknownsHeader::UnknownsHeader(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    UnknownsHeader::~UnknownsHeader()
    {
    }

    MainPage* MainPage::_instance = NULL;
    MainPage* MainPage::instance(QWidget* _parent)
    {
        assert(_instance || _parent);

        if (!_instance)
            _instance = new MainPage(_parent);

        return _instance;
    }

    void MainPage::reset()
    {
        if (_instance)
        {
            delete _instance;
            _instance = 0;
        }
    }

    QString MainPage::getMainWindowQss()
    {
        auto style = Utils::LoadStyle(":/main_window/main_window.qss");
        return style;
    }

    MainPage::MainPage(QWidget* _parent)
        : QWidget(_parent)
        , unknownsHeader_(nullptr)
        , searchWidget_(new SearchWidget(this))
        , contactDialog_(new ContactDialog(this))
        , videoWindow_(nullptr)
        , pages_(new WidgetsNavigator(this))
        , searchContacts_(nullptr)
        , generalSettings_(new GeneralSettingsWidget(this))
        , liveChatsPage_(new LiveChatHome(this))
        , snapsPage_(new SnapsPage(this))
        , themesSettings_(new ThemesSettingsWidget(this))
        , noContactsYetSuggestions_(nullptr)
        , contactListWidget_(new ContactList(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
        , settingsTimer_(new QTimer(this))
        , introduceYourselfSuggestions_(nullptr)
        , needShowIntroduceYourself_(false)
        , liveChats_(new LiveChats(this))
        , recvMyInfo_(false)
        , animCLWidth_(new QPropertyAnimation(this, "clWidth"))
        , clSpacer_(new QWidget())
        , clHostLayout_(new QHBoxLayout())
        , leftPanelState_(LeftPanelState::spreaded)
        , NeedShowUnknownsHeader_(false)
        , myTopWidget_(0)
        , currentTab_(RECENTS)
        , mainMenu_(new MainMenu(this))
        , semiWindow_(new SemitransparentWindowAnimated(this, ANIMATION_DURATION))
        , anim_(min_step)
        , snapsView_(new HorScrollableView(this))
        , headerWidget_(new QWidget(this))
        , headerLabel_(new QLabel(this))
        , menuVisible_(false)
    {
        animTopPanelHeight = new QPropertyAnimation(this, "topPanelHeight");
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPlaceholder, this, &MainPage::showPlaceholder);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::liveChatSelected, this, &MainPage::liveChatSelected, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &MainPage::compactModeChanged, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::contacts, this, &MainPage::contactsClicked, Qt::QueuedConnection);

        setStyleSheet(getMainWindowQss());

        this->setProperty("Invisible", true);
        horizontalLayout_ = Utils::emptyHLayout(this);
        QMetaObject::connectSlotsByName(this);

        originalLayout = qobject_cast<QHBoxLayout*>(layout());

        contactsWidget_ = new QWidget();
        contactsWidget_->setObjectName("contactsWidgetStyle");
        contactsLayout = Utils::emptyVLayout(contactsWidget_);

        myTopWidget_ = new TopPanelWidget(this, searchWidget_);
        myTopWidget_->setFixedHeight(Utils::scale_value(TOP_WIDGET_HEIGHT));
        contactsLayout->addWidget(myTopWidget_);

        mainMenu_->setFixedWidth(Utils::scale_value(MENU_WIDTH));
        mainMenu_->hide();

        snapsPage_->hide();

        semiWindow_->hide();

        connect(snapsPage_, SIGNAL(close()), this, SLOT(snapsClose()), Qt::QueuedConnection);

        connect(myTopWidget_, SIGNAL(back()), this, SLOT(openRecents()), Qt::QueuedConnection);
        connect(myTopWidget_, SIGNAL(burgerClicked()), this, SLOT(showMainMenu()), Qt::QueuedConnection);
        connect(myTopWidget_, SIGNAL(discoverClicked()), this, SLOT(discoverClicked()), Qt::QueuedConnection);

        {
            CustomButton *back = nullptr;

            unknownsHeader_ = new UnknownsHeader(this);
            int height = ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).unknownsItemHeight();
            unknownsHeader_->setFixedHeight(height);
            unknownsHeader_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
            unknownsHeader_->setVisible(false);
            {
                int horOffset = ::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).itemHorPadding();

                back = new CustomButton(this, ":/resources/basic_elements/contr_basic_back_100.png");
                back->setFixedSize(QSize(Utils::scale_value(BACK_WIDTH), Utils::scale_value(BACK_HEIGHT)));
                back->setStyleSheet("background: transparent; border-style: none;");
                back->setCursor(Qt::PointingHandCursor);
                back->setOffsets(horOffset, 0);
                back->setAlign(Qt::AlignLeft);

                auto layout = Utils::emptyHLayout(unknownsHeader_);
                layout->setContentsMargins(0, 0, horOffset + BACK_WIDTH, 0);
                layout->addWidget(back);

                auto unknownBackButtonLabel = new QLabel(this);
                unknownBackButtonLabel->setObjectName("unknownsLabel");
                unknownBackButtonLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                unknownBackButtonLabel->setText(QT_TRANSLATE_NOOP("contact_list", "Unknown contacts"));

                layout->addWidget(unknownBackButtonLabel);
            }
            contactsLayout->addWidget(unknownsHeader_);
            connect(back, &QPushButton::clicked, this, [=]()
            {
                emit Utils::InterConnector::instance().unknownsGoBack();
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &MainPage::changeCLHeadToUnknownSlot);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, &MainPage::changeCLHeadToSearchSlot);
        }

        auto hlayout = Utils::emptyHLayout();
        searchButton_ = new Ui::CustomButton(this, ":/resources/search_big_100.png");
        searchButton_->setFixedSize(Utils::scale_value(28), Utils::scale_value(40));
        searchButton_->setStyleSheet("QPushButton:focus { border: none; outline: none; } QPushButton:flat { border: none; outline: none; }");
        searchButton_->setCursor(QCursor(Qt::PointingHandCursor));
        hlayout->addWidget(searchButton_);
        contactsLayout->addLayout(hlayout);

        auto mailLayout = Utils::emptyHLayout();
        mailLayout->setContentsMargins(Utils::scale_value(6), 0, 0, 0);
        contactsLayout->addLayout(mailLayout);
        connect(searchButton_, SIGNAL(clicked()), this, SLOT(setSearchFocus()), Qt::QueuedConnection);
        searchButton_->hide();
        
        auto snapSize = Logic::SnapItemDelegate::getSnapItemSize();

        snapsView_->setModel(Logic::GetSnapStorage());
        snapsView_->setItemDelegate(new Logic::SnapItemDelegate(this));
        snapsView_->setShowGrid(false);
        snapsView_->verticalHeader()->hide();
        snapsView_->horizontalHeader()->hide();
        snapsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        snapsView_->verticalHeader()->setDefaultSectionSize(snapSize.height());
        snapsView_->verticalScrollBar()->setDisabled(true);
        snapsView_->horizontalHeader()->setDefaultSectionSize(snapSize.width());
        snapsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        snapsView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        snapsView_->setAutoScroll(false);
        snapsView_->setFocusPolicy(Qt::NoFocus);
        snapsView_->setCursor(QCursor(Qt::PointingHandCursor));
        snapsView_->setFixedHeight(Utils::scale_value(SNAP_VIEW_HEIGHT));
        snapsView_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        snapsView_->setFrameStyle(QFrame::NoFrame);
        snapsView_->setStyleSheet(Utils::ScaleStyle("QTableView { margin-left: 0dip; margin-right: 0dip; }", Utils::getScaleCoefficient()));
        snapsView_->hide();

        contactListWidget_->setSnaps(snapsView_);
        connect(snapsView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(snapClicked(const QModelIndex&)), Qt::QueuedConnection);
        connect(Logic::GetSnapStorage(), SIGNAL(indexChanged()), this, SLOT(snapsChanged()), Qt::QueuedConnection);

        contactsLayout->addWidget(contactListWidget_);

        connect(contactListWidget_, SIGNAL(tabChanged(int)), this, SLOT(tabChanged(int)), Qt::QueuedConnection);

        QSpacerItem* contactsLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        contactsLayout->addSpacerItem(contactsLayoutSpacer);

        pagesLayout_ = Utils::emptyVLayout();
        pagesLayout_->addWidget(headerWidget_);
        headerWidget_->setFixedHeight(Ui::CommonStyle::getTopPanelHeight());
        headerWidget_->setObjectName("header");
        auto l = Utils::emptyHLayout(headerWidget_);
        l->addSpacerItem(new QSpacerItem(Utils::scale_value(back_spacing), 0, QSizePolicy::Fixed));
        headerBack_ = new HeaderBack(headerWidget_);
        headerBack_->setText(QT_TRANSLATE_NOOP("main_page", "Back to chats"));
        headerBack_->adjustSize();
        headerBack_->setObjectName("header_back");
        headerBack_->setCursor(Qt::PointingHandCursor);
        connect(headerBack_, SIGNAL(clicked()), this, SLOT(headerBack()));
        l->addWidget(headerBack_);
        headerLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        headerLabel_->setObjectName("header_label");
        headerLabel_->setText("label");
        l->addWidget(headerLabel_);
        auto s = new QSpacerItem(headerBack_->width() + Utils::scale_value(back_spacing), 0, QSizePolicy::Fixed);
        l->addSpacerItem(s);
        headerWidget_->hide();
        counter_ = new UnreadsCounter(headerWidget_);
        counter_->setFixedSize(Utils::scale_value(balloon_size * 2), Utils::scale_value(balloon_size));
        connect(headerBack_, &HeaderBack::resized, [this]() {
            counter_->move(headerBack_->width() + Utils::scale_value(counter_padding + back_spacing), headerWidget_->height() / 2 - Utils::scale_value(balloon_size) / 2);});

        connect(&Utils::InterConnector::instance(), SIGNAL(showHeader(QString)), this, SLOT(showHeader(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(myProfileBack()), this, SLOT(headerBack()));

        pagesLayout_->addWidget(pages_);

        {
            auto pc = pages_->count();
            pages_->addWidget(contactDialog_);
            pages_->addWidget(generalSettings_);
            pages_->addWidget(liveChatsPage_);
            pages_->addWidget(themesSettings_);
            if (!pc)
                pages_->push(contactDialog_);
        }
        originalLayout->addWidget(clSpacer_);

        clHostLayout_->addWidget(contactsWidget_);
        originalLayout->addLayout(clHostLayout_);
        originalLayout->addLayout(pagesLayout_);

        QSpacerItem* originalLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
        originalLayout->addSpacerItem(originalLayoutSpacer);
        originalLayout->setAlignment(Qt::AlignLeft);
        setFocus();

        connect(contactListWidget_, &ContactList::itemSelected, contactDialog_, &ContactDialog::onContactSelected, Qt::QueuedConnection);
        connect(contactDialog_, SIGNAL(sendMessage(QString)), contactListWidget_, SLOT(onSendMessage(QString)), Qt::QueuedConnection);

        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::onContactSelected, Qt::QueuedConnection);
        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::hideRecentsPopup, Qt::QueuedConnection);
        connect(contactListWidget_, &ContactList::addContactClicked, this, &MainPage::onAddContactClicked, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsShow(QString)), this, SLOT(onProfileSettingsShow(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsShow(bool,QString)), this, SLOT(onThemesSettingsShow(bool,QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsShow(int)), this, SLOT(onGeneralSettingsShow(int)), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(profileSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(generalSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(themesSettingsBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachPhoneBack()), pages_, SLOT(pop()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(attachUinBack()), pages_, SLOT(pop()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(showSnapsChanged()), this, SLOT(showSnapsChanged()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(popPagesToRoot()), this, SLOT(popPagesToRoot()), Qt::DirectConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, this, &MainPage::startSearhInDialog, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setSearchFocus, this, &MainPage::setSearchFocus, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::themesSettingsOpen, this, &MainPage::themesSettingsOpen, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchEnd, this, &MainPage::searchEnd, Qt::QueuedConnection);

        connect(searchWidget_, SIGNAL(searchBegin()), this, SLOT(searchBegin()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchEnd()), this, SLOT(searchEnd()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(inputEmpty()), this, SLOT(searchInputClear()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(search(QString)), Logic::getSearchModelDLG(), SLOT(searchPatternChanged(QString)), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(enterPressed()), contactListWidget_, SLOT(searchResult()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(upPressed()), contactListWidget_, SLOT(searchUpPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(downPressed()), contactListWidget_, SLOT(searchDownPressed()), Qt::QueuedConnection);
        connect(searchWidget_, SIGNAL(searchIconClicked()), this, SLOT(spreadCL()), Qt::QueuedConnection);
        connect(contactListWidget_, SIGNAL(searchEnd()), searchWidget_, SLOT(searchCompleted()), Qt::QueuedConnection);

        connect(
            &Utils::InterConnector::instance(),
            &Utils::InterConnector::historyControlReady,
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::contactChanged,
            Qt::QueuedConnection);

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipShowVideoWindow(bool)), this, SLOT(onVoipShowVideoWindow(bool)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncoming(const std::string&, const std::string&)), this, SLOT(onVoipCallIncoming(const std::string&, const std::string&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallIncomingAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);

        QTimer::singleShot(Ui::period_for_start_stats_settings_ms, this, SLOT(post_stats_with_settings()));
        QObject::connect(settingsTimer_, SIGNAL(timeout()), this, SLOT(post_stats_with_settings()));
        settingsTimer_->start(Ui::period_for_stats_settings_ms);

        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &MainPage::myInfo, Qt::UniqueConnection);

        connect(contactDialog_, &ContactDialog::clicked, this, &MainPage::hideRecentsPopup);
        connect(searchWidget_, &SearchWidget::activeChanged, this, &MainPage::searchActivityChanged);
        connect(Ui::GetDispatcher(), &core_dispatcher::historyUpdate, contactDialog_, &ContactDialog::onContactSelectedToLastMessage, Qt::QueuedConnection);

        animBurger_ = new QPropertyAnimation(this, "anim");
        animBurger_->setDuration(ANIMATION_DURATION);
        connect(animBurger_, SIGNAL(finished()), this, SLOT(animFinished()), Qt::QueuedConnection);

        connect(mainMenu_, SIGNAL(createGroupChat()), this, SLOT(createGroupChat()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(addContact()), this, SLOT(onAddContactClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(contacts()), this, SLOT(contactsClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(settings()), this, SLOT(settingsClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(discover()), this, SLOT(discoverClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(stories()), this, SLOT(storiesClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(myProfile()), this, SLOT(myProfileClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(about()), this, SLOT(aboutClicked()), Qt::QueuedConnection);
        connect(mainMenu_, SIGNAL(contactUs()), this, SLOT(contactUsClicked()), Qt::QueuedConnection);
    }

    MainPage::~MainPage()
    {
        if (videoWindow_)
        {
            delete videoWindow_;
            videoWindow_ = nullptr;
        }
    }

    void MainPage::setCLWidth(int _val)
    {
        auto compact_width = ::ContactList::ItemWidth(false, false, true);
        auto normal_width = ::ContactList::ItemWidth(false, false, false);
        contactsWidget_->setFixedWidth(_val);
        contactListWidget_->setItemWidth(_val);
        contactListWidget_->setFixedWidth(_val);
        unknownsHeader_->setFixedWidth(_val);
        myTopWidget_->setFixedWidth(_val);
        bool isCompact = (_val == compact_width);
        contactListWidget_->setPictureOnlyView(isCompact);
        searchButton_->setVisible(isCompact && currentTab_ == RECENTS && !NeedShowUnknownsHeader_);

        TopPanelWidget::Mode m = TopPanelWidget::NORMAL;
        if (isCompact)
            m = TopPanelWidget::COMPACT;
        else if (leftPanelState_ == LeftPanelState::spreaded || _val != normal_width)
            m = TopPanelWidget::SPREADED;
        myTopWidget_->setMode(m);

        if (leftPanelState_ == LeftPanelState::picture_only && _val == compact_width && clHostLayout_->count() == 0)
        {
            clSpacer_->setFixedWidth(0);
            clHostLayout_->addWidget(contactsWidget_);
        }
    }

    int MainPage::getCLWidth() const
    {
        return contactListWidget_->width();
    }

    void MainPage::animateVisibilityCL(int _newWidth, bool _withAnimation)
    {
        int startValue = getCLWidth();
        int endValue = _newWidth;

        int duration = _withAnimation ? 200 : 0;

        animCLWidth_->stop();
        animCLWidth_->setDuration(duration);
        animCLWidth_->setStartValue(startValue);
        animCLWidth_->setEndValue(endValue);
        animCLWidth_->setEasingCurve(QEasingCurve::InQuad);
        animCLWidth_->start();
    }

    void MainPage::resizeEvent(QResizeEvent*)
    {
        auto cl_width = ::ContactList::ItemWidth(false, false, leftPanelState_ == LeftPanelState::picture_only);
        if (leftPanelState_ == LeftPanelState::spreaded
            || (::ContactList::IsPictureOnlyView() != (leftPanelState_ == LeftPanelState::picture_only)))
        {
            setLeftPanelState(::ContactList::IsPictureOnlyView() ? LeftPanelState::picture_only : LeftPanelState::normal, false);
            cl_width = ::ContactList::ItemWidth(false, false, leftPanelState_ == LeftPanelState::picture_only);
        }
        else
        {
            animateVisibilityCL(cl_width, false);
        }

        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            sidebar->setFixedWidth(width() - cl_width);

        myTopWidget_->setFixedWidth(cl_width);
        snapsView_->setFixedWidth(cl_width - Utils::scale_value(1));

        mainMenu_->setFixedHeight(height());
        semiWindow_->setFixedSize(size());
        snapsPage_->setFixedSize(size());
    }

    void MainPage::hideMenu()
    {
        if (!menuVisible_)
            return;
        
        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        semiWindow_->Hide();
    }

    bool MainPage::isMenuVisible() const
    {
        return mainMenu_->isVisible() && animBurger_->state() != QVariantAnimation::Running;
    }
    
    bool MainPage::isMenuVisibleOrOpening() const
    {
        return menuVisible_;
    }

    int MainPage::getContactDialogWidth(int _mainPageWidth)
    {
        return _mainPageWidth - ::ContactList::ItemWidth(false, false, false);
    }

    void MainPage::setAnim(int _val)
    {
        anim_ = _val;
        auto w = -1 * mainMenu_->width() + mainMenu_->width() * (anim_ / (double)max_step);
        mainMenu_->move(w, 0);
    }

    int MainPage::getAnim() const
    {
        return anim_;
    }

    void MainPage::animFinished()
    {
        if (anim_ == min_step)
            mainMenu_->hide();
    }

    void MainPage::snapsChanged()
    {
        snapsView_->setRowHidden(Logic::GetSnapStorage()->getFeaturedRow(), true);
        int friends = Logic::GetSnapStorage()->getFriendsSnapsCount();
        int featured = Logic::GetSnapStorage()->getFeaturedSnapsCount();

        auto offset = Logic::SnapItemDelegate::getFirstAndLastOffset();
        auto s = Logic::SnapItemDelegate::getSnapItemSize();
        for (auto i = 0; i < friends; ++i)
        {
            if (i == 0 || i == friends - 1)
                snapsView_->setColumnWidth(i, s.width() + offset);
            else
                snapsView_->setColumnWidth(i, s.width());
        }

        while (friends < featured)
        {
            snapsView_->setColumnWidth(friends, 0);
            ++friends;
        }

        showSnapsChanged();
    }

    void MainPage::showSnapsChanged()
    {
        if (leftPanelState_ != LeftPanelState::normal)
            return;

        if (contactListWidget_->currentTab() != RECENTS)
            return;

        if (unknownsHeader_->isVisible())
            return;

        bool show = get_gui_settings()->get_value<bool>(settings_show_snaps, false) && Logic::GetSnapStorage()->getFriendsSnapsCount() != 0;
        snapsView_->setVisible(show);
        Logic::getRecentsModel()->setSnapsVisible(show);
    }

    void MainPage::snapsClose()
    {
        snapsPage_->stop();
        semiWindow_->Hide();
        snapsPage_->hide();
    }

    void MainPage::snapClicked(const QModelIndex& index)
    {
        if (index.isValid())
        {
            semiWindow_->setFixedSize(size());
            semiWindow_->raise();
            semiWindow_->Show();

            Logic::GetSnapStorage()->startTv(index.row(), index.column());
            snapsPage_->show();
            snapsPage_->resize(size());
            snapsPage_->raise();
        }
    }

    void MainPage::setSearchFocus()
    {
        auto curPage = pages_->currentWidget();

        if (curPage != contactDialog_)
        {
            pages_->push(contactDialog_);
            contactListWidget_->changeTab(RECENTS, true);
            currentTab_ = RECENTS;
            myTopWidget_->setBack(false);
        }

        if (::ContactList::IsPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true, true);
        }
        searchWidget_->setFocus();
    }

    void MainPage::onProfileSettingsShow(QString _uin)
    {
        if (!_uin.isEmpty())
        {
            showSidebar(_uin, profile_page);
            return;
        }

        contactDialog_->takeSidebar();
        Sidebar* sidebar = contactDialog_->getSidebar();
        sidebar->preparePage(_uin, profile_page);
        sidebar->setSidebarWidth(Utils::scale_value(428));
        pages_->addWidget(sidebar);
        pages_->push(sidebar);
        sidebar->setFixedWidth(width() - ::ContactList::ItemWidth(false, false, leftPanelState_ == LeftPanelState::picture_only || leftPanelState_ == LeftPanelState::spreaded));
    }

    void MainPage::raiseVideoWindow()
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
        }

        if (!!videoWindow_ && !videoWindow_->isHidden())
        {
            videoWindow_->showNormal();
            videoWindow_->activateWindow();
        }
    }

    void MainPage::nextChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
            Logic::getContactListModel()->select(Logic::getRecentsModel()->nextAimId(Logic::getContactListModel()->selectedContact()), -1);
        else if (contactListWidget_->currentTab() == ALL)
            Logic::getContactListModel()->next();
    }

    void MainPage::prevChat()
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty())
            return;

        if (contactListWidget_->currentTab() == RECENTS)
            Logic::getContactListModel()->select(Logic::getRecentsModel()->prevAimId(Logic::getContactListModel()->selectedContact()), -1);
        else if (contactListWidget_->currentTab() == ALL)
            Logic::getContactListModel()->prev();
    }

    void MainPage::leftTab()
    {
        int tab = contactListWidget_->currentTab();
        if (tab != RECENTS)
        {
            contactListWidget_->changeTab((CurrentTab)(++tab));
        }
    }

    void MainPage::rightTab()
    {
        int tab = contactListWidget_->currentTab();
        if (tab != SETTINGS && tab != SEARCH)
        {
            contactListWidget_->changeTab((CurrentTab)(++tab));
        }
    }

    void MainPage::onGeneralSettingsShow(int _type)
    {
        pages_->push(generalSettings_);
        generalSettings_->setType(_type);
    }

    void MainPage::onThemesSettingsShow(bool _showBackButton, QString _aimId)
    {
        themesSettings_->setBackButton(_showBackButton);
        themesSettings_->setTargetContact(_aimId);
        pages_->push(themesSettings_);
    }

    void MainPage::clearSearchMembers()
    {
        contactListWidget_->update();
    }

    void MainPage::cancelSelection()
    {
        assert(contactDialog_);
        contactDialog_->cancelSelection();
    }

    void MainPage::createGroupChat()
    {
        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        QStringList empty_list;
        auto oldModel = Logic::getChatMembersModel();
        Logic::setChatMembersModel(NULL);
        Ui::createGroupChat(empty_list);
        Logic::setChatMembersModel(oldModel);

        semiWindow_->Hide();
    }

    void MainPage::myProfileClicked()
    {
        searchButton_->hide();
        contactListWidget_->changeTab(SETTINGS);
        auto settingsTab = contactListWidget_->getSettingsTab();
        if (settingsTab)
            settingsTab->settingsProfileClicked();

        emit Utils::InterConnector::instance().profileSettingsShow("");
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "My profile"));
    }

    void MainPage::aboutClicked()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_About);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "About app"));
    }

    void MainPage::contactUsClicked()
    {
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_ContactUs);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Contact Us"));
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        assert(contactDialog_);
        return contactDialog_;
    }

    HistoryControlPage* MainPage::getHistoryPage(const QString& _aimId) const
    {
        return contactDialog_->getHistoryPage(_aimId);
    }

    void MainPage::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        contactDialog_->insertTopWidget(_aimId, _widget);
    }

    void MainPage::removeTopWidget(const QString& _aimId)
    {
        contactDialog_->removeTopWidget(_aimId);
    }

    void MainPage::showSidebar(const QString& _aimId, int _page)
    {
        if (searchContacts_ && pages_->currentWidget() == searchContacts_)
        {
            contactDialog_->takeSidebar();
            Sidebar* sidebar = contactDialog_->getSidebar();
            sidebar->preparePage(_aimId, (SidebarPages)_page);
            sidebar->setSidebarWidth(Utils::scale_value(428));
            sidebar->setFixedWidth(pages_->width());
            pages_->addWidget(sidebar);
            pages_->push(sidebar);
        }
        else
        {
            contactDialog_->showSidebar(_aimId, _page);
        }
    }

    bool MainPage::isSidebarVisible() const
    {
        return contactDialog_->isSidebarVisible();
    }

    void MainPage::setSidebarVisible(bool _show)
    {
        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
        {
            pages_->pop();
            pages_->removeWidget(sidebar);
            if (!_show)
                contactDialog_->setSidebarVisible(_show);
        }
        else
        {
            contactDialog_->setSidebarVisible(_show);
        }
    }

    void MainPage::restoreSidebar()
    {
        auto cont = Logic::getContactListModel()->selectedContact();
        if (isSidebarVisible() && !cont.isEmpty())
            showSidebar(cont, menu_page);
        else
            setSidebarVisible(false);
    }

    bool MainPage::isContactDialog() const
    {
        if (pages_ == 0)
            return false;

        return pages_->currentWidget() == contactDialog_;
    }

    bool MainPage::isSnapsPageVisible() const
    {
        return snapsPage_->isVisible();
    }

    void MainPage::onContactSelected(QString _contact)
    {
        pages_->poproot();
        headerWidget_->hide();

        if (searchContacts_)
        {
            pages_->removeWidget(searchContacts_);
            delete searchContacts_;
            searchContacts_ = nullptr;
        }

        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
    }

    void MainPage::onAddContactClicked()
    {
        if (!searchContacts_)
        {
            searchContacts_ = new SearchContactsWidget(this);
            connect(searchContacts_, &SearchContactsWidget::active, this, &MainPage::hideRecentsPopup);
            pages_->addWidget(searchContacts_);
        }
        pages_->push(searchContacts_);
        searchContacts_->onFocus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::search_open_page);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Add contact"));
    }

    void MainPage::contactsClicked()
    {        
        menuVisible_ = false;
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        SelectContactsWidget contacts(nullptr, Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, QT_TRANSLATE_NOOP("popup_window", "Contacts"), QString(), QString(), Ui::MainPage::instance());
        emit Utils::InterConnector::instance().searchEnd();
        contacts.setFixedWidth(Utils::scale_value(CL_POPUP_WIDTH));
        Logic::getContactListModel()->updatePlaceholders();
        contacts.show();

        semiWindow_->Hide();
    }

    void MainPage::settingsClicked()
    {
        searchButton_->hide();
        contactListWidget_->changeTab(SETTINGS);
        auto settingsTab = contactListWidget_->getSettingsTab();
        if (settingsTab)
            settingsTab->settingsGeneralClicked();

        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
    }

    void MainPage::discoverClicked()
    {
        contactListWidget_->changeTab(LIVE_CHATS);
        Logic::GetLiveChatsModel()->initIfNeeded();
        emit Utils::InterConnector::instance().liveChatsShow();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechats_page_open);
    }

    void MainPage::searchBegin()
    {
        contactListWidget_->setSearchMode(true);
        snapsView_->hide();
    }

    void MainPage::storiesClicked()
    {
        animBurger_->stop();
        animBurger_->setStartValue(max_step);
        animBurger_->setEndValue(min_step);
        animBurger_->start();

        menuVisible_ = false;

        Logic::GetSnapStorage()->startTv(0, 0);
        snapsPage_->show();
        snapsPage_->resize(size());
        snapsPage_->raise();
    }

    void MainPage::searchEnd()
    {
        if (NeedShowUnknownsHeader_)
            changeCLHead(true);

        searchWidget_->clearInput();

        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();
        emit Utils::InterConnector::instance().disableSearchInDialog();

        contactListWidget_->setSearchMode(false);
        contactListWidget_->setSearchInDialog(false);

        showSnapsChanged();
    }

    void MainPage::searchInputClear()
    {
        emit Utils::InterConnector::instance().hideNoSearchResults();
        emit Utils::InterConnector::instance().hideSearchSpinner();

        if (!contactListWidget_->getSearchInDialog())
        {
            contactListWidget_->setSearchMode(false);
            Logic::getRecentsModel()->setSnapsVisible(false);
        }
    }

    void MainPage::onVoipCallIncoming(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            std::string callId = _account + "#" + _contact;

            auto it = incomingCallWindows_.find(callId);
            if (incomingCallWindows_.end() == it || !it->second)
            {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(_account, _contact));
                assert(!!window);

                if (!!window)
                {
                    window->showFrame();
                    incomingCallWindows_[callId] = window;
                }
            }
            else
            {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }
        }
    }

    void MainPage::destroyIncomingCallWindow(const std::string& _account, const std::string& _contact)
    {
        assert(!_account.empty());
        assert(!_contact.empty());

        if (!_account.empty() && !_contact.empty())
        {
            std::string call_id = _account + "#" + _contact;
            auto it = incomingCallWindows_.find(call_id);
            if (incomingCallWindows_.end() != it)
            {
                auto window = it->second;
                assert(!!window);

                if (!!window)
                {
                    window->hideFrame();
                }
            }
        }
    }

    void MainPage::onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);

        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.account, _contactEx.contact.contact);
    }

    void MainPage::showVideoWindow()
    {
        if (videoWindow_)
        {
            if (videoWindow_->isMinimized())
            {
                videoWindow_->showNormal();
            }

            videoWindow_->activateWindow();
#ifndef _WIN32
            videoWindow_->raise();
#endif
        }
    }

    void MainPage::notifyApplicationWindowActive(const bool isActive)
    {
        if (contactDialog_)
            contactDialog_->notifyApplicationWindowActive(isActive);

        if (mainMenu_ && mainMenu_->isVisible())
            mainMenu_->notifyApplicationWindowActive(isActive);

        if (snapsPage_ && snapsPage_->isVisible())
            snapsPage_->notifyApplicationWindowActive(isActive);
    }

    void MainPage::recentsTabActivate(bool _selectUnread)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->changeTab(RECENTS);

            if (_selectUnread)
            {
                QString aimId = Logic::getRecentsModel
                ()->nextUnreadAimId();
                if (aimId.isEmpty())
                    aimId = Logic::getUnknownsModel()->nextUnreadAimId();
                if (!aimId.isEmpty())
                    contactListWidget_->select(aimId);

            }
        }
    }

    void MainPage::selectRecentChat(QString _aimId)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            if (_aimId.length() > 0)
            {
                contactListWidget_->select(_aimId);
            }
        }
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType _item)
    {
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->settingsClicked();

            switch (_item)
            {
            case Utils::CommonSettingsType::CommonSettingsType_General:
            case Utils::CommonSettingsType::CommonSettingsType_VoiceVideo:
            case Utils::CommonSettingsType::CommonSettingsType_Notifications:
            case Utils::CommonSettingsType::CommonSettingsType_Themes:
            case Utils::CommonSettingsType::CommonSettingsType_About:
            case Utils::CommonSettingsType::CommonSettingsType_ContactUs:
            case Utils::CommonSettingsType::CommonSettingsType_AttachPhone:
            case Utils::CommonSettingsType::CommonSettingsType_AttachUin:
                Utils::InterConnector::instance().generalSettingsShow((int)_item);
                break;
            case Utils::CommonSettingsType::CommonSettingsType_Profile:
                Utils::InterConnector::instance().profileSettingsShow("");
                break;
            default:
                break;
            }

            if (_item == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo && contactListWidget_)
                contactListWidget_->selectSettingsVoipTab();
        }
    }

    void MainPage::onVoipShowVideoWindow(bool _enabled)
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

        if (!!videoWindow_)
        {
            if (_enabled)
            {
                videoWindow_->showFrame();
            }
            else
            {
                videoWindow_->hideFrame();

                bool wndMinimized = false;
                bool wndHiden = false;
                if (QWidget* parentWnd = window())
                {
                    wndHiden = !parentWnd->isVisible();
                    wndMinimized = parentWnd->isMinimized();
                }

                if (!Utils::foregroundWndIsFullscreened() && !wndMinimized && !wndHiden)
                {
                    raise();
                }
            }
        }
    }

    void MainPage::hideInput()
    {
        contactDialog_->hideInput();
    }

    QWidget* MainPage::showNoContactsYetSuggestions(QWidget* _parent, std::function<void()> _addNewContactsRoutine)
    {
        if (!noContactsYetSuggestions_)
        {
            noContactsYetSuggestions_ = new QWidget(_parent);
            noContactsYetSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            noContactsYetSuggestions_->setStyleSheet("background-color: #ffffff;");
            {
                auto l = Utils::emptyVLayout(noContactsYetSuggestions_);
                l->setAlignment(Qt::AlignCenter);
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto logoWidget = new QWidget(p);
                        logoWidget->setObjectName(build::is_icq() ? "logoWidget" : "logoWidgetAgent");
                        logoWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        logoWidget->setFixedSize(Utils::scale_value(80), Utils::scale_value(80));
                        pl->addWidget(logoWidget);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), CommonStyle::getTextCommonColor(), Utils::scale_value(44));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(
                            build::is_icq()? 
                            QT_TRANSLATE_NOOP("placeholders", "Install ICQ on mobile")
                            : QT_TRANSLATE_NOOP("placeholders", "Install Mail.Ru Agent on mobile")
                        );
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(24), CommonStyle::getTextCommonColor(), Utils::scale_value(30));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "to synchronize your contacts"));
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = new QHBoxLayout(p);
                    pl->setContentsMargins(0, Utils::scale_value(28), 0, 0);
                    pl->setSpacing(Utils::scale_value(8));
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto appStoreButton = new QPushButton(p);
                        appStoreButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        appStoreButton->setFlat(true);

                        auto appStoreImage = QString(":/resources/placeholders/content_badge_appstore_%1_100.png").
                            arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());

                        auto appStoreImageStyle = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                            .arg(appStoreImage)
                            .arg(appStoreImage);

                        Utils::ApplyStyle(appStoreButton, appStoreImageStyle);

                        appStoreButton->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        appStoreButton->setCursor(Qt::PointingHandCursor);

                        _parent->connect(appStoreButton, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(build::is_icq() ?
                                QUrl("https://app.appsflyer.com/id302707408?pid=icq_win")
                                : QUrl("https://app.appsflyer.com/id335315530?pid=agent_win")
                            );
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_ios);
                        });
                        pl->addWidget(appStoreButton);

                        auto googlePlayWidget = new QPushButton(p);
                        googlePlayWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                        googlePlayWidget->setFlat(true);

                        auto googlePlayImage = QString(":/resources/placeholders/content_badge_gplay_%1_100.png")
                            .arg(Ui::get_gui_settings()->get_value(settings_language, QString("")).toUpper());
                        auto googlePlayStyle = QString("QPushButton { border-image: url(%1); } QPushButton:hover { border-image: url(%2); } QPushButton:focus { border: none; outline: none; }")
                            .arg(googlePlayImage)
                            .arg(googlePlayImage);

                        Utils::ApplyStyle(googlePlayWidget, googlePlayStyle);

                        googlePlayWidget->setFixedSize(Utils::scale_value(152), Utils::scale_value(44));
                        googlePlayWidget->setCursor(Qt::PointingHandCursor);
                        _parent->connect(googlePlayWidget, &QPushButton::clicked, []()
                        {
                            QDesktopServices::openUrl(build::is_icq() ? 
                                QUrl("https://app.appsflyer.com/com.icq.mobile.client?pid=icq_win")
                                : QUrl("https://app.appsflyer.com/ru.mail?pid=agent_win")
                            );
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_android);
                        });
                        pl->addWidget(googlePlayWidget);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w1 = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(18), CommonStyle::getTextCommonColor(), Utils::scale_value(46));
                        w1->setSizePolicy(QSizePolicy::Policy::Preferred, w1->sizePolicy().verticalPolicy());
                        w1->setText(QT_TRANSLATE_NOOP("placeholders", "or "));
                        pl->addWidget(w1);

                        auto w2 = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(18), CommonStyle::getLinkColor(), Utils::scale_value(46));
                        w2->setSizePolicy(QSizePolicy::Policy::Preferred, w2->sizePolicy().verticalPolicy());
                        w2->setText(QT_TRANSLATE_NOOP("placeholders", "find friends"));
                        w2->setCursor(Qt::PointingHandCursor);
                        _parent->connect(w2, &Ui::TextEmojiWidget::clicked, _addNewContactsRoutine);
                        pl->addWidget(w2);
                    }
                    l->addWidget(p);
                }
                {
                    auto p = new QWidget(noContactsYetSuggestions_);
                    p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    auto pl = Utils::emptyHLayout(p);
                    pl->setAlignment(Qt::AlignCenter);
                    {
                        auto w = new Ui::TextEmojiWidget(p, Fonts::appFontScaled(15), QColor("#767676"), Utils::scale_value(20));
                        w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
                        w->setText(build::is_icq() ?
                            QT_TRANSLATE_NOOP("placeholders", "by phone number or UIN")
                            : QT_TRANSLATE_NOOP("placeholders", "by phone number or Email")
                        );
                        pl->addWidget(w);
                    }
                    l->addWidget(p);
                }

            }
        }
        return noContactsYetSuggestions_;
    }

    void MainPage::showPlaceholder(Utils::PlaceholdersType _placeholdersType)
    {
        switch(_placeholdersType)
        {
        case Utils::PlaceholdersType::PlaceholdersType_HideFindFriend:
            if (noContactsYetSuggestions_)
            {
                noContactsYetSuggestions_->setHidden(true);
                pages_->removeWidget(noContactsYetSuggestions_);
            }
            if (pages_->currentWidget() != contactDialog_->getSidebar())
                pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_FindFriend:
            pages_->insertWidget(1, showNoContactsYetSuggestions(pages_, [this]()
            {
                onAddContactClicked();
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_find_friends);
            }));
            pages_->poproot();
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself:
            needShowIntroduceYourself_ = true;
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself:
            if (needShowIntroduceYourself_)
            {
                if (introduceYourselfSuggestions_)
                {
                    introduceYourselfSuggestions_->setHidden(true);
                    pages_->removeWidget(introduceYourselfSuggestions_);
                }

                if (pages_->currentWidget() != contactDialog_->getSidebar())
                    pages_->poproot();

                needShowIntroduceYourself_ = false;
            }
            break;

        case Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself:
            if (!needShowIntroduceYourself_)
                break;

            pages_->insertWidget(0, showIntroduceYourselfSuggestions(pages_));
            pages_->poproot();
            break;

        default:
            break;
        }
    }

    QWidget* MainPage::showIntroduceYourselfSuggestions(QWidget* _parent)
    {
        if (!introduceYourselfSuggestions_)
        {
            introduceYourselfSuggestions_ = new IntroduceYourself(MyInfo()->aimId(), MyInfo()->friendlyName(), _parent);
            introduceYourselfSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            introduceYourselfSuggestions_->setStyleSheet("border: 20px solid red;");
        }
        return introduceYourselfSuggestions_;
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::getStatsSender()->trySendStats();
    }

    void MainPage::myInfo()
    {
        if (MyInfo()->friendlyName().isEmpty())
        {
            if (!recvMyInfo_)
            {
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself);
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself);
            }
        }
        else
        {
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        recvMyInfo_ = true;
    }

    void MainPage::openCreatedGroupChat()
    {
        auto connect_id = connect(GetDispatcher(), &core_dispatcher::openChat, this, [=](QString _aimId) { QTimer::singleShot(500, [=](){ contactListWidget_->select(_aimId); } ); });
        QTimer::singleShot(3000, [=] { disconnect(connect_id); } );
    }

    void MainPage::popPagesToRoot()
    {
        Sidebar* sidebar = contactDialog_->getSidebar();
        if (pages_->currentWidget() == sidebar)
            setSidebarVisible(false);

        pages_->poproot();
    }

    void MainPage::liveChatSelected()
    {
        pages_->push(liveChatsPage_);
    }

    void MainPage::spreadCL()
    {
        if (::ContactList::IsPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true, true);
            searchWidget_->setFocus();
        }
    }

    void MainPage::hideRecentsPopup()
    {
        if (leftPanelState_ == LeftPanelState::spreaded)
            setLeftPanelState(LeftPanelState::picture_only, true);
    }


    void MainPage::setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search, bool _force)
    {
        assert(_newState > LeftPanelState::min && _newState < LeftPanelState::max);

        if (leftPanelState_ == _newState && !_force)
            return;

        int new_cl_width = 0;
        leftPanelState_ = _newState;
        if (leftPanelState_ == LeftPanelState::normal)
        {
            clSpacer_->setFixedWidth(0);
            if (clHostLayout_->count() == 0)
                clHostLayout_->addWidget(contactsWidget_);

            new_cl_width = ::ContactList::ItemWidth(false, false, false);
            contactListWidget_->setPictureOnlyView(false);
            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);

            showSnapsChanged();
        }
        else if (leftPanelState_ == LeftPanelState::picture_only)
        {
            new_cl_width = ::ContactList::ItemWidth(false, false, true);
            searchWidget_->clearInput();
            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);

            if (currentTab_ != RECENTS && currentTab_ != SETTINGS)
                contactListWidget_->changeTab(RECENTS);

            snapsView_->hide();
            Logic::getRecentsModel()->setSnapsVisible(false);
        }
        else if (leftPanelState_ == LeftPanelState::spreaded)
        {
            clSpacer_->setFixedWidth(::ContactList::ItemWidth(false, false, true));
            clHostLayout_->removeWidget(contactsWidget_);

            contactsLayout->setParent(contactsWidget_);
            new_cl_width = Utils::scale_value(360);

            contactListWidget_->setItemWidth(new_cl_width);

            unknownsHeader_->setVisible(NeedShowUnknownsHeader_ && leftPanelState_ != LeftPanelState::picture_only);
            animateVisibilityCL(new_cl_width, _withAnimation);

            snapsView_->hide();
            Logic::getRecentsModel()->setSnapsVisible(false);
        }
        else
        {
            assert(false && "Left Panel state does not exist.");
        }
    }

    void MainPage::searchActivityChanged(bool _isActive)
    {
        if (!_isActive && leftPanelState_ == LeftPanelState::spreaded)
            hideRecentsPopup();

        myTopWidget_->searchActivityChanged(_isActive);
    }

    bool MainPage::isVideoWindowActive()
    {
        return (videoWindow_ && videoWindow_->isActiveWindow());
    }

    void MainPage::setFocusOnInput()
    {
        if (contactDialog_)
            contactDialog_->setFocusOnInputWidget();
    }

    void MainPage::clearSearchFocus()
    {
        searchWidget_->clearFocus();
    }

    void MainPage::onSendMessage(const QString& contact)
    {
        if (contactDialog_)
            contactDialog_->onSendMessage(contact);
    }

    void MainPage::startSearhInDialog(QString _aimid)
    {
        changeCLHead(false);

        setSearchFocus();

        Logic::getSearchModelDLG()->setSearchInDialog(_aimid);

        searchWidget_->clearInput();
        contactListWidget_->setSearchMode(true);
        contactListWidget_->setSearchInDialog(true);
    }

    void MainPage::changeCLHead(bool _showUnknownHeader)
    {
        if (currentTab_ == SETTINGS)
            return;
        
        unknownsHeader_->setVisible(_showUnknownHeader && leftPanelState_ != LeftPanelState::picture_only);

        if (_showUnknownHeader)
            snapsView_->hide();
        else
            showSnapsChanged();
    }

    void MainPage::changeCLHeadToSearchSlot()
    {
        NeedShowUnknownsHeader_ = false;
        changeCLHead(false);
        myTopWidget_->setBack(false);
        searchButton_->setVisible(leftPanelState_ == LeftPanelState::picture_only && currentTab_ == RECENTS && !NeedShowUnknownsHeader_);
    }

    void MainPage::changeCLHeadToUnknownSlot()
    {
        NeedShowUnknownsHeader_ = true;
        changeCLHead(true);
        myTopWidget_->setBack(true);
        searchButton_->hide();
    }

    void MainPage::openRecents()
    {
        emit Utils::InterConnector::instance().unknownsGoBack();
        contactListWidget_->changeTab(RECENTS);
    }

    void MainPage::showMainMenu()
     {
        if (menuVisible_)
            return;
         
        searchWidget_->clearFocus();
        searchEnd();
         
        menuVisible_ = true;
        semiWindow_->setFixedSize(size());
        semiWindow_->raise();
        semiWindow_->Show();
        mainMenu_->setFixedHeight(height());
        mainMenu_->raise();
        mainMenu_->show();

        animBurger_->stop();
        animBurger_->setStartValue(min_step);
        animBurger_->setEndValue(max_step);
        animBurger_->start();
    }
    
    void MainPage::compactModeChanged()
    {
        setLeftPanelState(leftPanelState_, false, false, true);
    }

    void MainPage::tabChanged(int tab)
    {
        if (tab != currentTab_ && leftPanelState_ == LeftPanelState::spreaded)
        {
            setLeftPanelState(LeftPanelState::picture_only, false);
        }

        if (tab == RECENTS)
            showSnapsChanged();
        else
            snapsView_->hide();

        currentTab_ = tab;
        searchButton_->setVisible(leftPanelState_ == LeftPanelState::picture_only && tab == RECENTS && !NeedShowUnknownsHeader_);
    }

    void MainPage::themesSettingsOpen()
    {
        contactListWidget_->openThemeSettings();
    }

    void MainPage::headerBack()
    {
        headerWidget_->hide();
        pages_->poproot();   
        contactListWidget_->changeTab(RECENTS);
    }

    void MainPage::showHeader(QString text)
    {
        headerLabel_->setText(text);
        headerWidget_->show();
    }

    void MainPage::nextSnap()
    {
        if (snapsPage_)
            snapsPage_->nextSnap();
    }

    void MainPage::nextUserSnap()
    {
        if (snapsPage_)
            snapsPage_->nextUser();
    }

    void MainPage::prevUserSnap()
    {
        if (snapsPage_)
            snapsPage_->prevUser();
    }
}
