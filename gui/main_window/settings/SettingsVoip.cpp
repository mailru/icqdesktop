#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initVoiceVideo(QWidget* _parent, VoiceAndVideoOptions& _voiceAndVideo)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(36), 0, 0, Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->addWidget(scrollArea);

    auto __deviceChanged = [&_voiceAndVideo, _parent](const int ix, const voip_proxy::EvoipDevTypes dev_type)
    {
        assert(ix >= 0);
        if (ix < 0) { return; }

        std::vector<DeviceInfo>* devList = NULL;
        switch (dev_type) {
        case voip_proxy::kvoipDevTypeAudioPlayback: { devList = &_voiceAndVideo.aPlaDeviceList; break; }
        case voip_proxy::kvoipDevTypeAudioCapture: {  devList = &_voiceAndVideo.aCapDeviceList; break; }
        case voip_proxy::kvoipDevTypeVideoCapture: {  devList = &_voiceAndVideo.vCapDeviceList; break; }
        case voip_proxy::kvoipDevTypeUndefined:
        default:
            assert(!"unexpected device type");
            return;
        };

        assert(devList);
        if (devList->empty()) { return; }

        assert(ix < (int)devList->size());
        const DeviceInfo& info = (*devList)[ix];

        GeneralSettingsWidget* settingsWidget = qobject_cast<GeneralSettingsWidget*>(_parent->parent());

        if (settingsWidget)
        {
            voip_proxy::device_desc description;
            description.name = info.name;
            description.uid = info.uid;
            description.dev_type = dev_type;

            settingsWidget->setActiveDevice(description);
        }
    };

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Voice and video"));
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Microphone:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioCapture);
        },
            [](bool) -> QString { return ""; });

        _voiceAndVideo.audioCaptureDevices = di.menu;
        _voiceAndVideo.aCapSelected = di.currentSelected;
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Speakers:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeAudioPlayback);
        },
            [](bool) -> QString { return ""; });

        _voiceAndVideo.audioPlaybackDevices = di.menu;
        _voiceAndVideo.aPlaSelected = di.currentSelected;
    }
    {
        std::vector< QString > vs;
        const auto di = GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Webcam:"),
            vs,
            0,
            -1,
            [__deviceChanged](QString v, int ix, TextEmojiWidget*)
        {
            __deviceChanged(ix, voip_proxy::kvoipDevTypeVideoCapture);
        },
            [](bool) -> QString { return ""; });

        _voiceAndVideo.videoCaptureDevices = di.menu;
        _voiceAndVideo.vCapSelected = di.currentSelected;
    }
}