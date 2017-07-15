#include "stdafx.h"
#include "VideoSettings.h"
#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"

Ui::VideoSettings::VideoSettings(QWidget* _parent)
    : QWidget(_parent)
{
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);
    cbVideoCapture_ = new QComboBox(this);
    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(255);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(cbVideoCapture_->sizePolicy().hasHeightForWidth());
    cbVideoCapture_->setSizePolicy(sizePolicy1);
    cbAudioPlayback_ = new QComboBox(this);
    sizePolicy1.setHeightForWidth(cbAudioPlayback_->sizePolicy().hasHeightForWidth());
    cbAudioPlayback_->setSizePolicy(sizePolicy1);
    cbAudioCapture_ = new QComboBox(this);
    sizePolicy1.setHeightForWidth(cbAudioCapture_->sizePolicy().hasHeightForWidth());
    cbAudioCapture_->setSizePolicy(sizePolicy1);
    buttonAudioPlaybackSet_ = new QPushButton(this);
    buttonAudioCaptureSet_ = new QPushButton(this);
    buttonAudioPlaybackSet_->setText(QApplication::translate("voip_pages", "Settings", 0));
    QMetaObject::connectSlotsByName(this);
    

    connect(cbAudioCapture_, SIGNAL(currentIndexChanged(int)), this, SLOT(audioCaptureDevChanged(int)), Qt::QueuedConnection);
    connect(cbAudioPlayback_, SIGNAL(currentIndexChanged(int)), this, SLOT(audioPlaybackDevChanged(int)), Qt::QueuedConnection);
    connect(cbVideoCapture_, SIGNAL(currentIndexChanged(int)), this, SLOT(videoCaptureDevChanged(int)), Qt::QueuedConnection);

    connect(buttonAudioPlaybackSet_, SIGNAL(clicked()), this, SLOT(audioPlaybackSettings()), Qt::QueuedConnection);
    connect(buttonAudioCaptureSet_, SIGNAL(clicked()), this, SLOT(audioCaptureSettings()), Qt::QueuedConnection);

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), this, SLOT(onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>&)), Qt::DirectConnection);
    Ui::GetDispatcher()->getVoipController().setRequestSettings();
}

Ui::VideoSettings::~VideoSettings()
{

}

void Ui::VideoSettings::onVoipDeviceListUpdated(const std::vector<voip_proxy::device_desc>& _devices)
{
    devices_ = _devices;

    bool videoCaUpd = false;
    bool audioPlUpd = false;
    bool audioCaUpd = false;

    using namespace voip_proxy;
    for (unsigned ix = 0; ix < devices_.size(); ix++)
    {
        const device_desc& desc = devices_[ix];

        QComboBox* comboBox  = NULL;
        bool* flagPtr = NULL;

        switch (desc.dev_type)
        {
        case kvoipDevTypeAudioCapture:
            comboBox = cbAudioCapture_;
            flagPtr = &audioCaUpd;
            break;

        case kvoipDevTypeAudioPlayback:
            comboBox = cbAudioPlayback_;
            flagPtr = &audioPlUpd;
            break;

        case  kvoipDevTypeVideoCapture:
            comboBox = cbVideoCapture_;
            flagPtr = &videoCaUpd;
            break;

        case kvoipDevTypeUndefined:
        default:
            assert(false);
            continue;
        }

        assert(comboBox && flagPtr);
        if (!comboBox || !flagPtr)
        {
            continue;
        }

        if (!*flagPtr)
        {
            *flagPtr = true;
            comboBox->clear();
        }
        comboBox->addItem(
            QIcon (build::is_icq()
                ? ":/resources/main_window/appicon.ico"
                : ":/resources/main_window/appicon_agent.ico"),
            desc.name.c_str(), desc.uid.c_str());
    }
}

void Ui::VideoSettings::audioPlaybackSettings()
{
#ifdef _WIN32
    SHELLEXECUTEINFOA shellInfo;
    memset(&shellInfo, 0, sizeof(shellInfo));

    shellInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    shellInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shellInfo.hwnd   = (HWND)winId();
    shellInfo.lpVerb = "";
    shellInfo.lpFile = "mmsys.cpl";
    shellInfo.lpParameters = ",0";
    shellInfo.lpDirectory  = NULL;
    shellInfo.nShow        = SW_SHOWDEFAULT;

    BOOL ret = ShellExecuteExA(&shellInfo);
    if (!ret)
    {
        assert(ret);
    }
#else//WIN32
    #warning audioPlaybackSettings
#endif//WIN32
}

void Ui::VideoSettings::audioCaptureSettings()
{
#ifdef _WIN32
    SHELLEXECUTEINFOA shellInfo;
    memset(&shellInfo, 0, sizeof(shellInfo));

    shellInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    shellInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
    shellInfo.hwnd   = (HWND)winId();
    shellInfo.lpVerb = "";
    shellInfo.lpFile = "mmsys.cpl";
    shellInfo.lpParameters = ",1";
    shellInfo.lpDirectory  = NULL;
    shellInfo.nShow        = SW_SHOWDEFAULT;

    BOOL ret = ShellExecuteExA(&shellInfo);
    if (!ret)
    {
        assert(false);
    }
    
#else//WIN32
    #warning audioPlaybackSettings
#endif//WIN32
}

void Ui::VideoSettings::_onComboBoxItemChanged(QComboBox& _cb, int _ix, const std::string& _devType)
{
    assert(_ix >= 0 && _ix < _cb.count());
    if (_ix < 0 || _ix >= _cb.count())
    {
        return;
    }

    auto var = _cb.itemData(_ix);
    assert(var.type() == QVariant::String);
    if ((var.type() != QVariant::String))
    {
        return;
    }

    std::string uid = var.toString().toUtf8().constData();
    assert(!uid.empty());
    if (uid.empty())
    {
        return;
    }

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", _devType);
    collection.set_value_as_string("uid", uid);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void Ui::VideoSettings::audioCaptureDevChanged(int _ix)
{
    assert(cbAudioCapture_);
    if (cbAudioCapture_)
    {
        _onComboBoxItemChanged(*cbAudioCapture_, _ix, "audio_capture");
    }
}

void Ui::VideoSettings::audioPlaybackDevChanged(int _ix)
{
    assert(cbAudioPlayback_);
    if (cbAudioPlayback_)
    {
        _onComboBoxItemChanged(*cbAudioPlayback_, _ix, "audio_playback");
    }
}

void Ui::VideoSettings::videoCaptureDevChanged(int _ix)
{
    assert(cbVideoCapture_);
    if (cbVideoCapture_)
    {
        _onComboBoxItemChanged(*cbVideoCapture_, _ix, "video_capture");
    }
}
