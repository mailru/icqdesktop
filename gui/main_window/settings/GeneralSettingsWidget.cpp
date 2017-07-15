#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/TextEmojiWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    namespace { bool initialized_ = false; }

    GeneralSettingsWidget::GeneralSettingsWidget(QWidget* _parent):
        QStackedWidget(_parent),
        general_(nullptr),
        notifications_(nullptr),
        about_(nullptr),
        contactus_(nullptr)
    {
        initialized_ = false;

        voiceAndVideo_.rootWidget = NULL;
        voiceAndVideo_.audioCaptureDevices = NULL;
        voiceAndVideo_.audioPlaybackDevices = NULL;
        voiceAndVideo_.videoCaptureDevices = NULL;
        voiceAndVideo_.aCapSelected = NULL;
        voiceAndVideo_.aPlaSelected = NULL;
        voiceAndVideo_.vCapSelected = NULL;

        voiceAndVideo_.rootWidget = new QWidget(this);
        voiceAndVideo_.rootWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initVoiceVideo(voiceAndVideo_.rootWidget, voiceAndVideo_);

        // Fill device list.
        for (int i = voip_proxy::kvoipDevTypeAudioCapture; i<= voip_proxy::kvoipDevTypeVideoCapture; i += 1)
        {
            voip_proxy::EvoipDevTypes type = (voip_proxy::EvoipDevTypes)i;
            onVoipDeviceListUpdated(type, Ui::GetDispatcher()->getVoipController().deviceList(type));
        }

        addWidget(voiceAndVideo_.rootWidget);

        auto setActiveDevice = [this] (const voip_proxy::EvoipDevTypes& type)
        {
            QString settingsName;
            switch (type)
            {
                case voip_proxy::kvoipDevTypeAudioPlayback: { settingsName = settings_speakers;   break; }
                case voip_proxy::kvoipDevTypeAudioCapture:  { settingsName = settings_microphone; break; }
                case voip_proxy::kvoipDevTypeVideoCapture:  { settingsName = settings_webcam;     break; }
                case voip_proxy::kvoipDevTypeUndefined:
                default:
                    assert(!"unexpected device type");
                    return;
            };

            QString val = get_gui_settings()->get_value<QString>(settingsName, "");
            if (val != "")
            {
                voip_proxy::device_desc description;
                description.uid      = std::string(val.toUtf8());
                description.dev_type = type;

                user_selected_device_[type] = description.uid;
                Ui::GetDispatcher()->getVoipController().setActiveDevice(description);
            }
        };

        setActiveDevice(voip_proxy::kvoipDevTypeAudioPlayback);
        setActiveDevice(voip_proxy::kvoipDevTypeAudioCapture);
        setActiveDevice(voip_proxy::kvoipDevTypeVideoCapture);

        QObject::connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipDeviceListUpdated, this, &GeneralSettingsWidget::onVoipDeviceListUpdated, Qt::DirectConnection);
    }

    GeneralSettingsWidget::~GeneralSettingsWidget()
    {
        //
    }

    void GeneralSettingsWidget::initialize()
    {
        if (initialized_)
            return;

        initialized_ = true;

        setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss"));
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        QHBoxLayout* mainLayout;
        mainLayout = Utils::emptyHLayout();

        std::map<std::string, Synchronizator> collector;

        general_ = new GeneralSettings(this);
        general_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initGeneral(general_, collector);
        addWidget(general_);

        notifications_ = new NotificationSettings(this);
        notifications_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initNotifications(notifications_, collector);
        addWidget(notifications_);

        themes_ = new QWidget(this);
        themes_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        addWidget(themes_);

        about_ = new QWidget(this);
        about_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAbout(about_, collector);
        addWidget(about_);

        contactus_ = new QWidget(this);
        contactus_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initContactUs(contactus_, collector);
        addWidget(contactus_);

        attachUin_ = new QWidget(this);
        attachUin_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachUin(attachUin_, collector);
        addWidget(attachUin_);

        attachPhone_ = new QWidget(this);
        attachPhone_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        Creator::initAttachPhone(attachPhone_, collector);
        addWidget(attachPhone_);

        setCurrentWidget(general_);

        for (auto cs: collector)
        {
            auto &s = cs.second;
            for (size_t si = 0, sz = s.widgets_.size(); sz > 1 && si < (sz - 1); ++si)
                for (size_t sj = si + 1; sj < sz; ++sj)
                    connect(s.widgets_[si], s.signal_, s.widgets_[sj], s.slot_),
                    connect(s.widgets_[sj], s.signal_, s.widgets_[si], s.slot_);
        }

        general_->recvUserProxy();
        QObject::connect(Ui::GetDispatcher(), &core_dispatcher::getUserProxy, general_, &GeneralSettings::recvUserProxy, Qt::DirectConnection);
    }

    void GeneralSettingsWidget::setType(int _type)
    {
        initialize();

        Utils::CommonSettingsType type = (Utils::CommonSettingsType)_type;

        if (type == Utils::CommonSettingsType::CommonSettingsType_General)
        {
            setCurrentWidget(general_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_VoiceVideo)
        {
            setCurrentWidget(voiceAndVideo_.rootWidget);
            if (devices_.empty())
                Ui::GetDispatcher()->getVoipController().setRequestSettings();
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Notifications)
        {
            setCurrentWidget(notifications_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_Themes)
        {
            setCurrentWidget(themes_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_About)
        {
            setCurrentWidget(about_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settings_about_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_ContactUs)
        {
            setCurrentWidget(contactus_);
            emit Utils::InterConnector::instance().generalSettingsContactUsShown();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::feedback_show);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachPhone)
        {
            setCurrentWidget(attachPhone_);
        }
        else if (type == Utils::CommonSettingsType::CommonSettingsType_AttachUin)
        {
            setCurrentWidget(attachUin_);
            emit Utils::InterConnector::instance().updateFocus();
        }
    }

    void GeneralSettingsWidget::onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const std::vector< voip_proxy::device_desc >& _devices)
    {
        devices_ = _devices;

        QMenu* menu = NULL;
        std::vector<DeviceInfo>* deviceList = NULL;
        TextEmojiWidget* currentSelected = NULL;

        switch (deviceType)
        {
        case voip_proxy::kvoipDevTypeAudioCapture:
            menu = voiceAndVideo_.audioCaptureDevices;
            deviceList = &voiceAndVideo_.aCapDeviceList;
            currentSelected = voiceAndVideo_.aCapSelected;
            break;

        case voip_proxy::kvoipDevTypeAudioPlayback:
            menu = voiceAndVideo_.audioPlaybackDevices;
            deviceList = &voiceAndVideo_.aPlaDeviceList;
            currentSelected = voiceAndVideo_.aPlaSelected;
            break;

        case voip_proxy::kvoipDevTypeVideoCapture:
            menu = voiceAndVideo_.videoCaptureDevices;
            deviceList = &voiceAndVideo_.vCapDeviceList;
            currentSelected = voiceAndVideo_.vCapSelected;
            break;

        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(false);
            return;
        }

        if (!menu || !deviceList)            
        {
            return;
        }

        deviceList->clear();
        menu->clear();
        if (currentSelected)
        {
            currentSelected->setText("");
        }

        using namespace voip_proxy;

        const device_desc* selectedDesc = nullptr;
        const device_desc* activeDesc   = nullptr;
        for (unsigned ix = 0; ix < devices_.size(); ix++)
        {
            const device_desc& desc = devices_[ix];
 
            DeviceInfo di;
            di.name = desc.name;
            di.uid  = desc.uid;

            menu->addAction(desc.name.c_str());
            deviceList->push_back(di);

            if (user_selected_device_.count(deviceType) > 0 && user_selected_device_[deviceType] == di.uid)
            {
                selectedDesc = &desc;
            }
            if (desc.isActive)
            {
                activeDesc = &desc;
            }
        }

        if (currentSelected)
        {
            // User selected item has most priority, then voip active device, then first element in list.
            const device_desc* desc = selectedDesc ? selectedDesc
                : (activeDesc ? activeDesc
                    : (!devices_.empty() ? &devices_[0] : nullptr));

            if (desc)
            {
                currentSelected->setText(desc->name.c_str());
                setActiveDevice(*desc);
            }
        }
    }

    void GeneralSettingsWidget::hideEvent(QHideEvent* _e)
    {
        QStackedWidget::hideEvent(_e);
    }

    void GeneralSettingsWidget::showEvent(QShowEvent* _e)
    {
        initialize();
        QStackedWidget::showEvent(_e);
    }

    void GeneralSettingsWidget::paintEvent(QPaintEvent* _event)
    {
        QStackedWidget::paintEvent(_event);

        QPainter painter(this);
        painter.setBrush(QBrush(QColor("#ffffff")));
        painter.drawRect(
            geometry().x() - 1,
            geometry().y() - 1,
            visibleRegion().boundingRect().width() + 2,
            visibleRegion().boundingRect().height() + 2
        );
    }

    void GeneralSettingsWidget::setActiveDevice(const voip_proxy::device_desc& _description)
    {
        Ui::GetDispatcher()->getVoipController().setActiveDevice(_description);

        QString settingsName;
        switch (_description.dev_type) {
        case voip_proxy::kvoipDevTypeAudioPlayback: { settingsName = settings_speakers;  break; }
        case voip_proxy::kvoipDevTypeAudioCapture: { settingsName = settings_microphone; break; }
        case voip_proxy::kvoipDevTypeVideoCapture: { settingsName = settings_webcam;     break; }
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(!"unexpected device type");
            return;
        };

        user_selected_device_[_description.dev_type] = _description.uid;

        if (get_gui_settings()->get_value<QString>(settingsName, "") != _description.uid.c_str())
            get_gui_settings()->set_value<QString>(settingsName, _description.uid.c_str());
    }


    GeneralSettings::GeneralSettings(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &GeneralSettings::value_changed);
    }

    void GeneralSettings::recvUserProxy()
    {
        auto userProxy = Utils::get_proxy_settings();
        std::ostringstream str;
        if (userProxy->type_ > core::proxy_types::min && userProxy->type_ < core::proxy_types::max)
            str << userProxy->type_;
        else
            str << core::proxy_types::auto_proxy;

        auto connectionTypeName = str.str();
        if (connectionTypeChooser_)
            connectionTypeChooser_->setText(QString(connectionTypeName.c_str()));
    }

    void GeneralSettings::value_changed(QString name)
    {
        if (name == settings_sounds_enabled)
        {
            sounds_.check_->blockSignals(true);
            sounds_.check_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
            sounds_.check_->blockSignals(false);
        }
    }

    NotificationSettings::NotificationSettings(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &NotificationSettings::value_changed);
    }

    void NotificationSettings::value_changed(QString name)
    {
        if (name == settings_sounds_enabled)
        {
            sounds_.check_->blockSignals(true);
            sounds_.check_->setChecked(get_gui_settings()->get_value<bool>(settings_sounds_enabled, true));
            sounds_.check_->blockSignals(false);
        }
    }
}
