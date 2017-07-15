#include "stdafx.h"
#include "MainMenu.h"
#include "MainWindow.h"
#include "MainPage.h"
#include "sidebar/SidebarUtils.h"
#include "mplayer/FFMpegPlayer.h"
#include "../cache/snaps/SnapStorage.h"
#include "../controls/CustomButton.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/TextEditEx.h"
#include "../controls/LabelEx.h"
#include "../controls/TransparentScrollBar.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/Text2DocConverter.h"
#include "../fonts.h"
#include "../my_info.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"

namespace
{
    const int ITEM_HEIGHT = 40;
    const int CLOSE_REGION_SIZE = 48;
    const int HOR_PADDING = 16;
    const int LINE_TOP_MARGIN = 4;
    const int VER_PADDING = 16;
    const int TEXT_OFFSET = 12;
    const int BACKGROUND_HEIGHT_MAX = 148;
    const int BACKGROUND_HEIGHT_MIN = 116;
    const int SNAPS_HEIGHT_MAX = 208;
    const int SNAPS_HEIGHT_MIN = 120;
    const int SNAPS_WIDTH = 240;
    const int CLOSE_TOP_PADDING = 16;
    const int CLOSE_WIDTH = 16;
    const int CLOSE_HEIGHT = 16;
    const int AVATAR_SIZE = 52;
    const int AVATAR_BOTTOM_PADDING = 16;
    const int NAME_PADDING = 12;
    const int CHECKBOX_WIDTH = 44;
    const int CHECKBOX_HEIGHT = 24;
    const int IMAGE_DURATION = 4000;
    const int MAX_PLAYER_QUEUE_SIZE = 3;
    const int NAME_TOP_PADDING = 6;
    const int STATUS_TOP_PADDING = 8;
}

namespace Ui
{
    PreviewImageWidget::PreviewImageWidget(QWidget* parent)
        : QWidget(parent)
    {
    }

    PreviewImageWidget::~PreviewImageWidget()
    {
    }

    void PreviewImageWidget::paintEvent(QPaintEvent* e)
    {
        if (Preview_.isNull())
            return;

        QPainter p(this);
        p.drawPixmap(rect(), Preview_, Preview_.rect());
    }


    BackWidget::BackWidget(QWidget* _parent)
        : QWidget(_parent)
        , SnapsCount_(0)
        , Preview_(new PreviewImageWidget(this))
        , Overlay_(new PreviewImageWidget(this))
    {
        auto layout = Utils::emptyVLayout(this);
        Player_ = new FFMpegPlayer(this, false, true);
        Player_->setImageDuration(IMAGE_DURATION);
        Player_->setFillClient(true);
        Player_->setMute(true);
        Player_->hide();
        layout->addWidget(Player_);

        Player_->stackUnder(Preview_);
        Preview_->raise();
        Preview_->hide();

        Player_->stackUnder(Overlay_);
        Preview_->stackUnder(Overlay_);
        Overlay_->raise();
        Overlay_->hide();

        connect(Logic::GetSnapStorage(), SIGNAL(playUserSnap(QString, QString, QString, QString, qint64, bool)), this, SLOT(playSnap(QString, QString, QString, QString, qint64, bool)), Qt::QueuedConnection);
        connect(Logic::GetSnapStorage(), SIGNAL(previewChanged(QString)), this, SLOT(previewChanged(QString)), Qt::QueuedConnection);
        connect(Logic::GetSnapStorage(), SIGNAL(removed(QString)), this, SLOT(userSnapsRemoved(QString)), Qt::QueuedConnection);
        connect(Logic::GetSnapStorage(), SIGNAL(snapRemoved(QString, qint64, QString)), this, SLOT(snapRemoved(QString, qint64, QString)), Qt::QueuedConnection);
        connect(Player_, SIGNAL(mediaChanged(qint32)), this, SLOT(mediaChanged(qint32)), Qt::QueuedConnection);
        connect(Player_, SIGNAL(dataReady()), this, SLOT(hidePreview()), Qt::QueuedConnection);

        setMinimumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MIN));
        setMaximumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MAX));
    }

    void BackWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        emit clicked();
        return QWidget::mouseReleaseEvent(e);
    }

    void BackWidget::initSnaps()
    {
        auto snaps = Logic::GetSnapStorage()->getSnapsCount(MyInfo()->aimId());

        Overlay_->setVisible(snaps != 0 && !Preview_->isEmpty());

        bool needPreview = (SnapsCount_ == 0);
        if (SnapsCount_ && Player_->state() == QMovie::Paused)
            Player_->play(true);

        SnapsCount_ = snaps;

        if (SnapsCount_)
        {
            Logic::GetSnapStorage()->startUserSnaps();
            setMinimumHeight(Utils::scale_value(SNAPS_HEIGHT_MIN));
            setMaximumHeight(Utils::scale_value(SNAPS_HEIGHT_MAX));
        }
        else
        {
            Player_->stop();
            setMinimumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MIN));
            setMaximumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MAX));
        }

        if (SnapsCount_ && Player_->state() != QMovie::Running)
            showPreview();
    }

    void BackWidget::play()
    {
        if (Player_->state() == QMovie::Paused)
            Player_->play(true);
    }

    void BackWidget::pause()
    {
        if (Player_->state() == QMovie::Running)
            Player_->pause();
    }

    void BackWidget::playSnap(QString _path, QString, QString, QString, qint64 id, bool _first)
    {
        auto w = Utils::InterConnector::instance().getMainWindow();
        if (w && w->getMainPage() && !w->getMainPage()->isMenuVisibleOrOpening())
            return;
        
        if (Snaps_.contains(id))
            return;

        if (_first)
        {
            connect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()), Qt::UniqueConnection);
            Player_->clearQueue();
            Snaps_.clear();
        }

        if (!Snaps_.isEmpty() && Ids_[Player_->getMedia()] == Snaps_.last().path_)
            Player_->removeFromQueue(Snaps_.last().mediaId_);

        QFileInfo f(_path);
        Player_->openMedia(_path, Utils::is_image_extension_not_gif(f.suffix()));
        SnapItem s;
        s.path_ = _path;
        s.mediaId_ = Player_->getLastMedia();
        Snaps_[id] = s;
        Ids_[s.mediaId_] = s.path_;
    }

    void BackWidget::fileLoaded()
    {
        disconnect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()));
        Player_->play(true);
    }

    void BackWidget::mediaChanged(qint32 id)
    {
        if ((!Snaps_.isEmpty() && Snaps_.last().mediaId_ == id) || id == -1)
        {
            QMap<qint64, SnapItem>::iterator iter = Snaps_.begin();
            while (iter != Snaps_.end())
            {
                QFileInfo f(iter->path_);
                Player_->openMedia(iter->path_, Utils::is_image_extension_not_gif(f.suffix()));
                iter->mediaId_ = Player_->getLastMedia();
                Ids_[iter->mediaId_] = iter->path_;
                ++iter;
            }
        }
        Ids_.remove(id - 1);
        if (id == -1 && !Snaps_.isEmpty())
            connect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()), Qt::UniqueConnection);
    }

    void BackWidget::hidePreview()
    {
        Preview_->hide();
        Player_->show();
    }

    void BackWidget::showPreview()
    {
        Preview_->show();
        Player_->hide();
    }

    void BackWidget::previewChanged(QString aimid)
    {
        if (MyInfo()->aimId() != aimid)
            return;

        auto preview = Logic::GetSnapStorage()->getFirstUserPreview();
        if (preview.isNull())
        {
            Overlay_->hide();
            return;
        }

        QImage resultImg(QSize(Utils::scale_value(SNAPS_WIDTH), Utils::scale_value(SNAPS_HEIGHT_MAX)), QImage::Format_ARGB32);
        QPainter painter(&resultImg);
        painter.setRenderHint(QPainter::Antialiasing);

        if (preview.width() > preview.height())
            preview = preview.scaledToHeight(resultImg.height(), Qt::SmoothTransformation);
        else
            preview = preview.scaledToWidth(resultImg.width(), Qt::SmoothTransformation);
        QRect sourceRect(0, 0, resultImg.width(), resultImg.height());
        if (preview.width() > resultImg.rect().width())
        {
            auto diff = preview.width() / 2 - resultImg.rect().width() / 2;
            sourceRect.moveLeft(diff);
        }

        if (preview.height() > resultImg.rect().height())
        {
            auto diff = preview.height() / 2 - resultImg.rect().height() / 2;
            sourceRect.moveTop(diff);
        }
        painter.drawPixmap(resultImg.rect(), preview, sourceRect);
        Preview_->setPreview(QPixmap::fromImage(resultImg));
    }

    void BackWidget::userSnapsRemoved(QString aimId)
    {
        if (aimId == MyInfo()->aimId())
        {
            Player_->stop();
            Player_->hide();
            setMinimumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MIN));
            setMaximumHeight(Utils::scale_value(BACKGROUND_HEIGHT_MAX));
            Preview_->hide();
            Overlay_->hide();
            Snaps_.clear();
        }
    }

    void BackWidget::snapRemoved(QString aimid, qint64 id, QString path)
    {
        if (aimid != MyInfo()->aimId())
            return;

        auto s = Snaps_[id];
        auto media = Player_->getMedia();
        if (media == s.mediaId_ || (Ids_.contains(media) && Ids_[media] == path))
        {
            Player_->stop();
            Player_->loadFromQueue();
            auto w = Utils::InterConnector::instance().getMainWindow();
            if (w && w->isActive() && w->getMainPage()->isMenuVisible())
                Player_->play(true);
        }

        Player_->removeFromQueue(s.mediaId_);

        Snaps_.remove(id);
    }

    void BackWidget::prepareOverlay()
    {
        QImage resultImg(size(), QImage::Format_ARGB32);
        resultImg.fill(qRgba(0,0,0,0));
        QPainter painter(&resultImg);
        painter.setRenderHint(QPainter::Antialiasing);

        QPixmap pix(Utils::parse_image_name(":/resources/gradient_menu_100.png"));
        pix = pix.scaledToHeight(resultImg.rect().height() / 2, Qt::SmoothTransformation);
        QBrush b(pix);
        painter.fillRect(QRect(0, 0, resultImg.rect().width(), resultImg.rect().height() / 2), b);
        QMatrix m;
        m.rotate(180);
        b.setMatrix(m);
        painter.fillRect(QRect(0, resultImg.rect().height() / 2, resultImg.rect().width(), resultImg.rect().height() / 2), b);

        Overlay_->setPreview(QPixmap::fromImage(resultImg));
    }

    void BackWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(rect(), QColor("#84b858"));
    }

    void BackWidget::resizeEvent(QResizeEvent *e)
    {
        QPainterPath p;
        p.addRect(rect());
        Player_->setClippingPath(p);

        Preview_->setFixedSize(size());
        Preview_->move(0, 0);

        prepareOverlay();
        Overlay_->setFixedSize(size());
        Overlay_->move(0, 0);

        emit resized();
        return QWidget::resizeEvent(e);
    }

    MainMenu::MainMenu(QWidget* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
    {
        setObjectName("menu");
        setStyleSheet(Utils::LoadStyle(":/main_window/main_menu.qss"));

        ScrollArea_ = CreateScrollAreaAndSetTrScrollBar(this);
        ScrollArea_->setContentsMargins(0, 0, 0, 0);
        ScrollArea_->setWidgetResizable(true);

        MainWidget_ = new QWidget(ScrollArea_);
        ScrollArea_->setWidget(MainWidget_);

        MainWidget_->setStyleSheet("background: transparent; border: none;");
        ScrollArea_->setStyleSheet("background: white; border: none;");

        auto mainLayout = Utils::emptyVLayout(MainWidget_);
        mainLayout->setAlignment(Qt::AlignLeft);

        Background_ = new BackWidget(MainWidget_);
        mainLayout->addWidget(Background_, 1);
        connect(Background_, SIGNAL(resized()), this, SLOT(resize()), Qt::QueuedConnection);

        Close_ = new CustomButton(MainWidget_, ":/resources/basic_elements/close_big_b_100.png");
        Close_->setFixedSize(Utils::scale_value(CLOSE_WIDTH), Utils::scale_value(CLOSE_HEIGHT));
        Close_->setAttribute(Qt::WA_TransparentForMouseEvents);

        Avatar_ = new ContactAvatarWidget(MainWidget_, MyInfo()->aimId(), MyInfo()->friendlyName(), Utils::scale_value(AVATAR_SIZE), true);
        Avatar_->setAttribute(Qt::WA_TransparentForMouseEvents);
        Avatar_->SetOutline(true);
        
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(Utils::scale_value(2));
        shadow->setXOffset(0);
        shadow->setYOffset(Utils::scale_value(1));
        shadow->setColor(QColor(0, 0, 0, 77));
        Avatar_->setGraphicsEffect(shadow);

        Name_ = new TextEditEx(MainWidget_, Fonts::appFontScaled(17, Fonts::FontWeight::Medium), QColor("#ffffff"), false, false);
        Name_->setFrameStyle(QFrame::NoFrame);
        Name_->setStyleSheet("background: transparent;");
        Name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Name_->setAttribute(Qt::WA_TransparentForMouseEvents);

        Status_ = new TextEditEx(MainWidget_, Fonts::appFontScaled(14, Fonts::FontWeight::Normal), QColor("#ffffff"), false, false);
        Status_->setFrameStyle(QFrame::NoFrame);
        Status_->setStyleSheet("background: transparent;");
        Status_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Status_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Status_->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto buttonsLayout = Utils::emptyVLayout(MainWidget_);
        buttonsLayout->setAlignment(Qt::AlignLeft);

        CreateGroupchat_ = new ActionButton(MainWidget_, ":/resources/create_chat_100.png", QT_TRANSLATE_NOOP("main_menu", "Create group chat"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        CreateGroupchat_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(CreateGroupchat_);

        AddContact_ = new ActionButton(MainWidget_, ":/resources/add_contact_100.png", QT_TRANSLATE_NOOP("main_menu", "Add contact"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        AddContact_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(AddContact_);

        Contacts_ = new ActionButton(MainWidget_, ":/resources/contacts_100.png", QT_TRANSLATE_NOOP("main_menu", "Contacts"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Contacts_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Contacts_);

        Settings_ = new ActionButton(MainWidget_, ":/resources/settings/settings_general_100.png", QT_TRANSLATE_NOOP("main_menu", "Settings"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Settings_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Settings_);

        auto horLayout = Utils::emptyHLayout();
        SoundsButton_ = new CustomButton(MainWidget_, ":/resources/settings/settings_notify_100.png");
        SoundsButton_->setOffsets(Utils::scale_value(TEXT_OFFSET), 0);
        SoundsButton_->setText(QT_TRANSLATE_NOOP("main_menu", "Sounds"));
        SoundsButton_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        SoundsButton_->setAlign(Qt::AlignLeft);
        SoundsButton_->setFocusPolicy(Qt::NoFocus);
        SoundsButton_->setFixedHeight(Utils::scale_value(ITEM_HEIGHT));
        SoundsButton_->adjustSize();
        SoundsButton_->setTextColor("#454545");
        horLayout->addWidget(SoundsButton_);
        SoundsCheckbox_ = new QCheckBox(MainWidget_);
        SoundsCheckbox_->setObjectName("greenSwitcher");
        SoundsCheckbox_->adjustSize();
        SoundsCheckbox_->setCursor(QCursor(Qt::PointingHandCursor));
        SoundsCheckbox_->setFixedSize(Utils::scale_value(CHECKBOX_WIDTH), Utils::scale_value(CHECKBOX_HEIGHT));
        SoundsCheckbox_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
        SoundsCheckbox_->setStyleSheet("outline: none;");
        horLayout->addWidget(SoundsCheckbox_);
        horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        buttonsLayout->addLayout(horLayout);

        Line_ = new LineWidget(MainWidget_, Utils::scale_value(HOR_PADDING), Utils::scale_value(LINE_TOP_MARGIN), Utils::scale_value(HOR_PADDING), Utils::scale_value(LINE_TOP_MARGIN));
        buttonsLayout->addWidget(Line_);

        Discover_ = new ActionButton(MainWidget_, ":/resources/explore_100.png", QT_TRANSLATE_NOOP("main_menu", "Discover"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Discover_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Discover_);
        Discover_->hide();

        Stories_ = new ActionButton(MainWidget_, ":/resources/snaps_100.png", QT_TRANSLATE_NOOP("main_menu", "Stories"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        Stories_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(Stories_);
        Stories_->hide();

        SignOut_ = new ActionButton(MainWidget_, ":/resources/settings/settings_signout_100.png", QT_TRANSLATE_NOOP("main_menu", "Sign out"), Utils::scale_value(ITEM_HEIGHT), 0, Utils::scale_value(TEXT_OFFSET));
        SignOut_->setCursor(QCursor(Qt::PointingHandCursor));
        buttonsLayout->addWidget(SignOut_);
        buttonsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
        mainLayout->addLayout(buttonsLayout);

        auto hLayout = Utils::emptyHLayout();
        hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed));

        auto f = Fonts::appFont(Utils::scale_value(13), Fonts::FontFamily::SEGOE_UI, Fonts::FontWeight::Normal);
        auto p = QPalette();
        p.setColor(QPalette::WindowText, QColor("#999999"));

        About_ = new LabelEx(MainWidget_);
        About_->setText(QT_TRANSLATE_NOOP("main_menu", "About app"));
        About_->setCursor(Qt::PointingHandCursor);
        About_->setFont(f);
        About_->setPalette(p);
        hLayout->addWidget(About_);

        hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(HOR_PADDING), 0, QSizePolicy::Fixed));

        ContactUs_ = new LabelEx(MainWidget_);
        ContactUs_->setText(QT_TRANSLATE_NOOP("main_menu", "Contact Us"));
        ContactUs_->setCursor(Qt::PointingHandCursor);
        ContactUs_->setFont(f);
        ContactUs_->setPalette(p);
        hLayout->addWidget(ContactUs_);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        mainLayout->addLayout(hLayout);
        mainLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(VER_PADDING), QSizePolicy::Preferred, QSizePolicy::Fixed));

        auto l = Utils::emptyVLayout(this);
        l->addWidget(ScrollArea_);

        connect(&Utils::InterConnector::instance(), SIGNAL(closeAnySemitransparentWindow()), this, SLOT(Hide()), Qt::QueuedConnection);
        connect(MyInfo(), SIGNAL(received()), this, SLOT(myInfoUpdated()), Qt::QueuedConnection);
        connect(SoundsCheckbox_, SIGNAL(stateChanged(int)), this, SLOT(soundsChecked(int)), Qt::QueuedConnection);
        connect(get_gui_settings(), SIGNAL(changed(QString)), this, SLOT(guiSettingsChanged(QString)), Qt::QueuedConnection);

        connect(CreateGroupchat_, SIGNAL(clicked()), this, SIGNAL(createGroupChat()), Qt::QueuedConnection);
        connect(AddContact_, SIGNAL(clicked()), this, SIGNAL(addContact()), Qt::QueuedConnection);
        connect(Contacts_, SIGNAL(clicked()), this, SIGNAL(contacts()), Qt::QueuedConnection);
        connect(Settings_, SIGNAL(clicked()), this, SIGNAL(settings()), Qt::QueuedConnection);
        connect(Discover_, SIGNAL(clicked()), this, SIGNAL(discover()), Qt::QueuedConnection);
        connect(Stories_, SIGNAL(clicked()), this, SIGNAL(stories()), Qt::QueuedConnection);
        connect(About_, SIGNAL(clicked()), this, SIGNAL(about()), Qt::QueuedConnection);
        connect(ContactUs_, SIGNAL(clicked()), this, SIGNAL(contactUs()), Qt::QueuedConnection);
        connect(SignOut_, SIGNAL(clicked()), this, SLOT(signOut()), Qt::QueuedConnection);

        connect(AddContact_, SIGNAL(clicked()), this, SLOT(Hide()), Qt::QueuedConnection);
        connect(Settings_, SIGNAL(clicked()), this, SLOT(Hide()), Qt::QueuedConnection);
        connect(Discover_, SIGNAL(clicked()), this, SLOT(Hide()), Qt::QueuedConnection);
        connect(About_, SIGNAL(clicked()), this, SLOT(Hide()), Qt::QueuedConnection);
        connect(ContactUs_, SIGNAL(clicked()), this, SLOT(Hide()), Qt::QueuedConnection);

        connect(Logic::GetSnapStorage(), SIGNAL(indexChanged()), this, SLOT(snapsChanged()), Qt::QueuedConnection);
    }

    void MainMenu::notifyApplicationWindowActive(const bool isActive)
    {
        if (isActive)
            Background_->play();
        else
            Background_->pause();
    }

    void MainMenu::updateState()
    {
        QFontMetrics f(Status_->font());
        auto elidedState = f.elidedText(MyInfo()->state(), Qt::ElideRight, width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));

        auto &doc = *Status_->document();
        doc.clear();

        QTextCursor cursor = Status_->textCursor();
        Logic::Text2Doc(elidedState, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, f.height());
        Status_->adjustHeight(f.width(elidedState) + Utils::scale_value(HOR_PADDING));
    }

    void MainMenu::Hide()
    {
        auto w = Utils::InterConnector::instance().getMainWindow();
        if (w)
            w->hideMenu();

        Background_->pause();
    }

    void MainMenu::myInfoUpdated()
    {
        Avatar_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendlyName());
        QFontMetrics f(Name_->font());
        auto elidedName = f.elidedText(MyInfo()->friendlyName(), Qt::ElideRight, width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));

        auto &doc = *Name_->document();
        doc.clear();

        QTextCursor cursor = Name_->textCursor();
        Logic::Text2Doc(elidedName, cursor, Logic::Text2DocHtmlMode::Pass, false);
        Logic::FormatDocument(doc, f.height());
        Name_->adjustHeight(f.width(elidedName) + Utils::scale_value(HOR_PADDING));

        updateState();
    }

    void MainMenu::resizeEvent(QResizeEvent *e)
    {
        resize();

        return QWidget::resizeEvent(e);
    }

    void MainMenu::resize()
    {
        Background_->setFixedWidth(width());
        CreateGroupchat_->setFixedWidth(width());
        AddContact_->setFixedWidth(width());
        Contacts_->setFixedWidth(width());
        Settings_->setFixedWidth(width());
        Discover_->setFixedWidth(width());
        Stories_->setFixedWidth(width());

        SoundsButton_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + CHECKBOX_WIDTH));
        Close_->move(Utils::scale_value(HOR_PADDING), Utils::scale_value(CLOSE_TOP_PADDING));
        Avatar_->move(Utils::scale_value(HOR_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE));
        Name_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));
        Name_->move(Utils::scale_value(HOR_PADDING + AVATAR_SIZE + NAME_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE - NAME_TOP_PADDING));
        Line_->setLineWidth(width() - Utils::scale_value(HOR_PADDING * 2));

        Status_->setFixedWidth(width() - Utils::scale_value(HOR_PADDING + AVATAR_SIZE + HOR_PADDING + NAME_PADDING));
        Status_->move(Utils::scale_value(HOR_PADDING + AVATAR_SIZE + NAME_PADDING), Background_->height() - Utils::scale_value(AVATAR_BOTTOM_PADDING + AVATAR_SIZE - STATUS_TOP_PADDING) + Name_->height());
    }

    void MainMenu::snapsChanged()
    {
        Stories_->setVisible(Logic::GetSnapStorage()->getFriendsSnapsCount() != 0);
    }

    void MainMenu::showEvent(QShowEvent *e)
    {
        updateState();
        Background_->initSnaps();
        return QWidget::showEvent(e);
    }

    void MainMenu::hideEvent(QHideEvent *e)
    {
        return QWidget::hideEvent(e);
    }

    void MainMenu::mouseReleaseEvent(QMouseEvent *e)
    {
        static auto closeRect = QRect(0, 0, Utils::scale_value(CLOSE_REGION_SIZE), Utils::scale_value(CLOSE_REGION_SIZE));
        if (closeRect.contains(e->pos()))
        {
            Hide();
            return;
        }

        auto profileRect = QRect(0, Avatar_->y(), width(), Avatar_->height());
        if (profileRect.contains(e->pos()))
        {
            emit myProfile();
            Hide();
            return;
        }

        return QWidget::mouseReleaseEvent(e);
    }

    void MainMenu::soundsChecked(int value)
    {
        get_gui_settings()->set_value<bool>(settings_sounds_enabled, value != Qt::Unchecked);
    }

    void MainMenu::guiSettingsChanged(QString value)
    {
        if (value != settings_sounds_enabled)
            return;

        SoundsCheckbox_->blockSignals(true);
        SoundsCheckbox_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
        SoundsCheckbox_->blockSignals(false);
    }

    void MainMenu::signOut()
    {
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Sign out"),
            NULL);

        if (confirm)
        {
            get_gui_settings()->set_value(settings_feedback_email, QString(""));
            GetDispatcher()->post_message_to_core("logout", nullptr);
            emit Utils::InterConnector::instance().logout();
        }
    }

    MainMenu::~MainMenu()
    {
    }
}
