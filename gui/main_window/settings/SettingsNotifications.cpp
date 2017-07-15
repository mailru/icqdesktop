#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../my_info.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initNotifications(NotificationSettings* _parent, std::map<std::string, Synchronizator>& _collector)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(36), 0, Utils::scale_value(36), Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->addWidget(scrollArea);

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Notifications"));
    {
        _parent->sounds_ = GeneralCreator::addSwitcher(
            &_collector,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Play sounds"),
            get_gui_settings()->get_value<bool>(settings_sounds_enabled, true),
            [](bool enabled) -> QString
        {
            GetDispatcher()->getVoipController().setMuteSounds(!enabled);
            if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != enabled)
                get_gui_settings()->set_value<bool>(settings_sounds_enabled, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        auto outgoingSoundWidgets = GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Outgoing messages sound"),
            get_gui_settings()->get_value<bool>(settings_outgoing_message_sound_enabled, false),
            [](bool enabled) -> QString
        {
            if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != enabled)
                get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        if (!get_gui_settings()->get_value(settings_sounds_enabled, true))
        {
            outgoingSoundWidgets.check_->setChecked(false);
            outgoingSoundWidgets.check_->setEnabled(false);
            outgoingSoundWidgets.text_->setEnabled(false);
        }
        connect(_parent->sounds_.check_, &QCheckBox::toggled, scrollArea, [_parent, outgoingSoundWidgets]()
        {
            bool c = _parent->sounds_.check_->isChecked();
            if (!c)
            {
                outgoingSoundWidgets.check_->setChecked(false);
                outgoingSoundWidgets.check_->setEnabled(false);
                outgoingSoundWidgets.text_->setEnabled(false);
                if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) != c)
                    get_gui_settings()->set_value(settings_outgoing_message_sound_enabled, c);
            }
            else
            {
                outgoingSoundWidgets.check_->setEnabled(true);
                outgoingSoundWidgets.text_->setEnabled(true);
            }
        });
        auto enable_notification = GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Show notifications"),
            get_gui_settings()->get_value<bool>(settings_notify_new_messages, true),
            [](bool enabled) -> QString
        {
            if (get_gui_settings()->get_value<bool>(settings_notify_new_messages, true) != enabled)
                get_gui_settings()->set_value<bool>(settings_notify_new_messages, enabled);
            return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
        if (Ui::MyInfo()->haveConnectedEmail())
        {
            auto enable_mail_notifications = GeneralCreator::addSwitcher(
                0,
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings_pages", "Show mail notifications"),
                get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true),
                [](bool enabled) -> QString
            {
                if (get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true) != enabled)
                    get_gui_settings()->set_value<bool>(settings_notify_new_mail_messages, enabled);
                return (enabled ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });

            connect(enable_notification.check_, &QCheckBox::toggled, scrollArea, [enable_notification, enable_mail_notifications]()
            {
                bool c = enable_notification.check_->isChecked();
                if (!c)
                {
                    enable_mail_notifications.check_->setChecked(false);
                    enable_mail_notifications.check_->setEnabled(false);
                    enable_mail_notifications.text_->setEnabled(false);
                    if (get_gui_settings()->get_value(settings_notify_new_mail_messages, false) != c)
                        get_gui_settings()->set_value(settings_notify_new_mail_messages, c);
                }
                else
                {
                    enable_mail_notifications.check_->setEnabled(true);
                    enable_mail_notifications.text_->setEnabled(true);
                }
            });
        }
    }
}

