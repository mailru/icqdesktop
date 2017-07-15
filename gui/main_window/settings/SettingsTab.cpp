#include "stdafx.h"
#include "SettingsTab.h"

#include "../contact_list/Common.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CustomButton.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/log/log.h"

namespace
{
    const int LEFT_OFFSET = 16;
    const int BUTTON_WIDTH = 28;
    const int BACK_WIDTH = 52;
    const int BACK_HEIGHT = 48;
}

namespace Ui
{
    class SettingsTab::UI
    {
    private:
        QWidget *settingsView;
        QLabel *topicLabel;
        QWidget* topWidget_;
        CustomButton *myProfileButton;
        CustomButton *generalButton;
        CustomButton *voipButton;
        CustomButton *notificationsButton;
        CustomButton *themesButton;

        friend class SettingsTab;

    public:
        void init(QWidget* _p, QLayout* _pl)
        {
            auto back = new CustomButton(_p, ":/resources/basic_elements/contr_basic_back_100.png");
            back->setFixedSize(QSize(Utils::scale_value(BACK_WIDTH), Utils::scale_value(BACK_HEIGHT)));
            back->setStyleSheet("background: transparent; border-style: none;");
            back->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            back->setAlign(Qt::AlignLeft);
            topWidget_ = new QWidget(_p);
            auto hLayout = Utils::emptyHLayout(topWidget_);
            hLayout->setContentsMargins(0, 0, Utils::scale_value(LEFT_OFFSET + BACK_WIDTH), 0);
            hLayout->addWidget(back);
            topicLabel = new QLabel(_p);
            Utils::grabTouchWidget(topicLabel);
            topicLabel->setObjectName("settings_topic_label");
            topicLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            hLayout->addWidget(topicLabel);
            _pl->addWidget(topWidget_);

            back->setCursor(QCursor(Qt::PointingHandCursor));
            connect(back, &QAbstractButton::clicked,[] () 
            { 
                emit Utils::InterConnector::instance().myProfileBack(); 
            });

            const auto flatted = true;
            const auto checkable = true;
            
            myProfileButton = new CustomButton(settingsView, ":/resources/settings/settings_profile_100.png");
            myProfileButton->setActiveImage(":/resources/settings/settings_profile_100_active.png");
            Testing::setAccessibleName(myProfileButton, "settings_my_profile_button");
            Utils::grabTouchWidget(myProfileButton);
            myProfileButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            myProfileButton->setMouseTracking(true);
            myProfileButton->setFlat(flatted);
            myProfileButton->setCheckable(checkable);
            myProfileButton->setAlign(Qt::AlignLeft);
            myProfileButton->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            myProfileButton->setFocusPolicy(Qt::NoFocus);
            myProfileButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(myProfileButton);

            generalButton = new CustomButton(settingsView, ":/resources/settings/settings_general_100.png");
            generalButton->setActiveImage(":/resources/settings/settings_general_100_active.png");
            Utils::grabTouchWidget(generalButton);
            generalButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            generalButton->setMouseTracking(true);
            generalButton->setFlat(flatted);
            generalButton->setCheckable(checkable);
            generalButton->setAlign(Qt::AlignLeft);
            generalButton->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            generalButton->setFocusPolicy(Qt::NoFocus);
            generalButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(generalButton);

            themesButton = new CustomButton(settingsView, ":/resources/settings/settings_themes_100.png");
            themesButton->setActiveImage(":/resources/settings/settings_themes_100_active.png");
            themesButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            themesButton->setMouseTracking(true);
            themesButton->setFlat(flatted);
            themesButton->setCheckable(checkable);
            themesButton->setAlign(Qt::AlignLeft);
            themesButton->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            themesButton->setFocusPolicy(Qt::NoFocus);
            themesButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(themesButton);

            notificationsButton = new CustomButton(settingsView, ":/resources/settings/settings_notify_100.png");
            notificationsButton->setActiveImage(":/resources/settings/settings_notify_100_active.png");
            Utils::grabTouchWidget(notificationsButton);
            notificationsButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            notificationsButton->setMouseTracking(true);
            notificationsButton->setFlat(flatted);
            notificationsButton->setCheckable(checkable);
            notificationsButton->setAlign(Qt::AlignLeft);
            notificationsButton->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            notificationsButton->setFocusPolicy(Qt::NoFocus);
            notificationsButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(notificationsButton);

            voipButton = new CustomButton(settingsView, ":/resources/settings/settings_video_100.png");
            voipButton->setActiveImage(":/resources/settings/settings_video_100_active.png");
            Utils::grabTouchWidget(voipButton);
            voipButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
            voipButton->setMouseTracking(true);
            voipButton->setFlat(flatted);
            voipButton->setCheckable(checkable);
            voipButton->setAlign(Qt::AlignLeft);
            voipButton->setOffsets(Utils::scale_value(LEFT_OFFSET), 0);
            voipButton->setFocusPolicy(Qt::NoFocus);
            voipButton->setCursor(QCursor(Qt::PointingHandCursor));
            _pl->addWidget(voipButton);

            topicLabel->setText(QT_TRANSLATE_NOOP("settings_pages","Settings"));
            myProfileButton->setText(QT_TRANSLATE_NOOP("settings_pages", "My Profile"));
            generalButton->setText(QT_TRANSLATE_NOOP("settings_pages", "General"));
            themesButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
            voipButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
            notificationsButton->setText(QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
        }
    };

    SettingsTab::SettingsTab(QWidget* _parent):
        QWidget(_parent),
        ui_(new UI()),
        currentSettingsItem_(Utils::CommonSettingsType::CommonSettingsType_None),
        isCompact_(false)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/settings/settings_tab.qss"));

        auto scrollArea = CreateScrollAreaAndSetTrScrollBar(this);
        scrollArea->setObjectName("scroll_area");
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::grabTouchWidget(scrollArea->viewport(), true);

        auto scrollAreaContent = new QWidget(scrollArea);
        scrollAreaContent->setObjectName("settings_view");
        Utils::grabTouchWidget(scrollAreaContent);

        auto scrollAreaContentLayout = Utils::emptyVLayout(scrollAreaContent);
        scrollAreaContentLayout->setAlignment(Qt::AlignTop);

        scrollArea->setWidget(scrollAreaContent);

        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(scrollArea);

        ui_->init(scrollAreaContent, scrollAreaContentLayout);

        connect(ui_->myProfileButton, SIGNAL(toggled(bool)), this, SLOT(settingsProfileClicked()), Qt::QueuedConnection);
        connect(ui_->generalButton, SIGNAL(toggled(bool)), this, SLOT(settingsGeneralClicked()), Qt::QueuedConnection);
        connect(ui_->voipButton, SIGNAL(toggled(bool)), this, SLOT(settingsVoiceVideoClicked()), Qt::QueuedConnection);
        connect(ui_->notificationsButton, SIGNAL(toggled(bool)), this, SLOT(settingsNotificationsClicked()), Qt::QueuedConnection);
        connect(ui_->themesButton, SIGNAL(toggled(bool)), this, SLOT(settingsThemesClicked()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &SettingsTab::compactModeChanged, Qt::QueuedConnection);
        
#ifdef STRIP_VOIP
        ui_->voipButton->hide();
#endif //STRIP_VOIP
    }

    SettingsTab::~SettingsTab() throw()
    {
        //
    }

    void SettingsTab::cleanSelection()
    {
        emit Utils::InterConnector::instance().popPagesToRoot();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_None;
        updateSettingsState();
    }

    void SettingsTab::setCompactMode(bool isCompact, bool force)
    {
        if (isCompact == isCompact_ && !force)
            return;

        isCompact_ = isCompact;

        int compact_offset =
            (ContactList::ItemWidth(false, false, true) - Utils::scale_value(BUTTON_WIDTH)) / 2;

        ui_->topWidget_->setVisible(!isCompact_);
        ui_->myProfileButton->setText(isCompact_ ? QString() : QT_TRANSLATE_NOOP("settings_pages", "My Profile"));
        ui_->myProfileButton->setOffsets(isCompact_ ? compact_offset : Utils::scale_value(LEFT_OFFSET), 0);
        ui_->generalButton->setText(isCompact_ ? QString() : QT_TRANSLATE_NOOP("settings_pages", "General"));
        ui_->generalButton->setOffsets(isCompact_ ? compact_offset : Utils::scale_value(LEFT_OFFSET), 0);
        ui_->themesButton->setText(isCompact_ ? QString() : QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
        ui_->themesButton->setOffsets(isCompact_ ? compact_offset : Utils::scale_value(LEFT_OFFSET), 0);
        ui_->voipButton->setText(isCompact_ ? QString() : QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
        ui_->voipButton->setOffsets(isCompact_ ? compact_offset : Utils::scale_value(LEFT_OFFSET), 0);
        ui_->notificationsButton->setText(isCompact_ ? QString() : QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
        ui_->notificationsButton->setOffsets(isCompact_ ? compact_offset : Utils::scale_value(LEFT_OFFSET), 0);
    }

    void SettingsTab::settingsProfileClicked()
    {
        if (currentSettingsItem_ != Utils::CommonSettingsType::CommonSettingsType_Profile
            && currentSettingsItem_ != Utils::CommonSettingsType::CommonSettingsType_None)
            emit Utils::InterConnector::instance().generalSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Profile;
        updateSettingsState();

        emit Utils::InterConnector::instance().profileSettingsShow("");
        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "My profile"));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::myprofile_open);
    }

    void SettingsTab::settingsGeneralClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_General;
        updateSettingsState();

        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
    }

    void SettingsTab::settingsVoiceVideoClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_VoiceVideo;
        updateSettingsState();

        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Voice and video"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }

    void SettingsTab::settingsNotificationsClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Notifications;
        updateSettingsState();

        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Notifications"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Notifications);
    }

    void SettingsTab::settingsThemesClicked()
    {
        if (currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile)
            emit Utils::InterConnector::instance().profileSettingsBack();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_Themes;
        updateSettingsState();

        emit Utils::InterConnector::instance().showHeader(QT_TRANSLATE_NOOP("main_page", "Wallpaper"));
        emit Utils::InterConnector::instance().themesSettingsShow(false, "");
    }

    void SettingsTab::updateSettingsState()
    {
        ui_->myProfileButton->blockSignals(true);
        ui_->generalButton->blockSignals(true);
        ui_->voipButton->blockSignals(true);
        ui_->notificationsButton->blockSignals(true);
        ui_->themesButton->blockSignals(true);

        ui_->myProfileButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile);
        ui_->myProfileButton->setDisabled(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile);
        ui_->myProfileButton->setActive(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile);
        ui_->myProfileButton->setTextColor(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Profile ? "#ffffff" : "#454545");

        ui_->generalButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_General);
        ui_->generalButton->setDisabled(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_General);
        ui_->generalButton->setActive(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_General);
        ui_->generalButton->setTextColor(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_General ? "#ffffff" : "#454545");

        ui_->voipButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        ui_->voipButton->setDisabled(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        ui_->voipButton->setActive(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        ui_->voipButton->setTextColor(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo ? "#ffffff" : "#454545");

        ui_->notificationsButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Notifications);
        ui_->notificationsButton->setDisabled(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Notifications);
        ui_->notificationsButton->setActive(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Notifications);
        ui_->notificationsButton->setTextColor(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Notifications ? "#ffffff" : "#454545");

        ui_->themesButton->setChecked(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Themes);
        ui_->themesButton->setDisabled(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Themes);
        ui_->themesButton->setActive(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Themes);
        ui_->themesButton->setTextColor(currentSettingsItem_ == Utils::CommonSettingsType::CommonSettingsType_Themes ? "#ffffff" : "#454545");
        
        
        ui_->myProfileButton->blockSignals(false);
        ui_->generalButton->blockSignals(false);
        ui_->voipButton->blockSignals(false);
        ui_->notificationsButton->blockSignals(false);
        ui_->themesButton->blockSignals(false);
    }

    void SettingsTab::compactModeChanged()
    {
        setCompactMode(isCompact_, true);
    }
}
