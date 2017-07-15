#include "stdafx.h"
#include "VoipProxy.h"

#include "MaskManager.h"

#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/sounds/SoundsManager.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../../core/Voip/VoipSerialization.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../controls/TextEmojiWidget.h"
#include "../my_info.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

#define INCLUDE_USER_NAME         1
#define INCLUDE_USER_NAME_IN_CONF 1
#define INCLUDE_REMOTE_CAMERA_OFF 0
#define INCLUDE_AVATAR            1
#define INCLUDE_BLUR_BACKGROUND   1
#define INCLUDE_WATERMARK         1
#define INCLUDE_CAMERA_OFF_STATUS 1

#define AVATAR_VIDE_SIZE          360

namespace
{
    inline const size_t __hash(const std::wstring& _str)
    {
        std::hash<std::wstring> h;
        return h(_str);
    }
    inline const size_t __hash(const std::string& _str)
    {
        std::hash<std::string> h;
        return h(_str);
    }
}

void addImageToCollection(Ui::gui_coll_helper& collection, const std::string& dir, const std::string& name, const std::string& filenamePrefix, const std::string& filenamePostfix, int outputWidth = 0, int outputHeight = 0)
{
    std::stringstream resourceStr;
    resourceStr << ":/resources/" + (!dir.empty() ? dir + "/" : "") << filenamePrefix << Utils::scale_bitmap_with_value(100) << filenamePostfix << ".png";
    QImage image(resourceStr.str().c_str());

    if (outputHeight > 0 && outputWidth > 0)
    {
        QImage bigImage(outputWidth, outputHeight, QImage::Format_ARGB32);
        bigImage.fill(Qt::transparent);

        {
            QPainter painter(&bigImage);

            QSize position = (bigImage.size() - image.size()) / 2;
            painter.drawImage(QPoint(position.width(), position.height()), image);
        }

        image = bigImage.copy();
    }

    auto dataSize = image.byteCount();
    auto dataPtr = image.bits();
    auto sz = image.size();

    assert(dataSize && dataPtr);
    if (dataSize && dataPtr)
    {
        core::ifptr<core::istream> stream(collection->create_stream());
        stream->write(dataPtr, dataSize);

        collection.set_value_as_stream(name, stream.get());
        collection.set_value_as_int(name + "_height", sz.height());
        collection.set_value_as_int(name + "_width", sz.width());
    }
}

void addTextToCollection(Ui::gui_coll_helper& collection, const std::string& name, const std::string& textString)
{
    QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(14));
    font.setStyleStrategy(QFont::PreferAntialias);

    const QString text = QT_TRANSLATE_NOOP("voip_pages", textString.c_str());

    QFontMetrics fm(font);
    const QSize textSz = fm.size(Qt::TextSingleLine, text);

    const int nickW = Utils::scale_bitmap_with_value(textSz.width());
    const int nickH = Utils::scale_bitmap_with_value(textSz.height());

    assert(nickW && nickH);
    if (nickW && nickH)
    {
        QPainter painter;
        painter.save();

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPixmap pm(nickW, nickH);
        pm.fill(Qt::transparent);

        const QPen pen(QColor("#767676"));

        painter.begin(&pm);
        painter.setPen(pen);
        painter.setFont(font);

        if (textSz.width() > nickW)
        {
            painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, text);
        }
        else
        {
            painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignHCenter | Qt::AlignVCenter, text);
        }
        painter.end();

        QImage image = pm.toImage();

        auto dataSize = image.byteCount();
        auto dataPtr = image.bits();
        auto sz = image.size();

        if (dataSize && dataPtr)
        {
            core::ifptr<core::istream> stream(collection->create_stream());
            stream->write(dataPtr, dataSize);

            collection.set_value_as_stream(name.c_str(), stream.get());
            collection.set_value_as_int(name + "_height", sz.height());
            collection.set_value_as_int(name + "_width",  sz.width());
        }

        painter.restore();
    }
}

#define STRING_ID(x) __hash((x))

voip_proxy::VoipController::VoipController(Ui::core_dispatcher& _dispatcher)
    : dispatcher_(_dispatcher)
    , callTimeElapsed_(0)
    , callTimeTimer_(this)
    , haveEstablishedConnection_(false)
    , iTunesWasPaused_(false)
    , maskEngineEnable_(false)
	, voipIsKilled_(false)
{
    cipherState_.state= voip_manager::CipherState::kCipherStateUnknown;

    connect(&callTimeTimer_, SIGNAL(timeout()), this, SLOT(_updateCallTime()), Qt::QueuedConnection);
    callTimeTimer_.setInterval(1000);

    std::vector<unsigned> codePoints;
    codePoints.reserve(32);

    codePoints.push_back(0xF09F8EA9);
    codePoints.push_back(0xF09F8FA0);
    codePoints.push_back(0xF09F92A1);
    codePoints.push_back(0xF09F9AB2);
    codePoints.push_back(0xF09F8C8D);
    codePoints.push_back(0xF09F8D8C);
    codePoints.push_back(0xF09F8D8F);
    codePoints.push_back(0xF09F909F);
    codePoints.push_back(0xF09F90BC);
    codePoints.push_back(0xF09F928E);
    codePoints.push_back(0xF09F98BA);
    codePoints.push_back(0xF09F8CB2);
    codePoints.push_back(0xF09F8CB8);
    codePoints.push_back(0xF09F8D84);
    codePoints.push_back(0xF09F8D90);
    codePoints.push_back(0xF09F8E88);
    codePoints.push_back(0xF09F90B8);
    codePoints.push_back(0xF09F9180);
    codePoints.push_back(0xF09F9184);
    codePoints.push_back(0xF09F9191);
    codePoints.push_back(0xF09F9193);
    codePoints.push_back(0xF09F9280);
    codePoints.push_back(0xF09F9494);
    codePoints.push_back(0xF09F94A5);
    codePoints.push_back(0xF09F9A97);
    codePoints.push_back(0xE2AD90);
    codePoints.push_back(0xE28FB0);
    codePoints.push_back(0xE29ABD);
    codePoints.push_back(0xE29C8F);
    codePoints.push_back(0xE29882);
    codePoints.push_back(0xE29C82);
    codePoints.push_back(0xE29DA4);

    voipEmojiManager_.addMap(64, 64, ":/emoji/secure_emoji_64.png", codePoints, 64);
    voipEmojiManager_.addMap(80, 80, ":/emoji/secure_emoji_80.png", codePoints, 80);
    voipEmojiManager_.addMap(96, 96, ":/emoji/secure_emoji_96.png", codePoints, 96);
    voipEmojiManager_.addMap(128, 128, ":/emoji/secure_emoji_128.png", codePoints, 128);

	connect(this, SIGNAL(onVoipCallIncoming(const std::string&, const std::string&)),
		this, SLOT(updateUserAvatar(const std::string&, const std::string&)));

	connect(this, SIGNAL(onVoipCallConnected(const voip_manager::ContactEx&)),
		this, SLOT(updateUserAvatar(const voip_manager::ContactEx&)));

    connect(this, SIGNAL(onVoipMaskEngineEnable(bool)),
        this, SLOT(updateVoipMaskEngineEnable(bool)));

    connect(this, SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)),
        this, SLOT(onCallCreate(const voip_manager::ContactEx&)));

    resetMaskManager();
}

voip_proxy::VoipController::~VoipController()
{
    callTimeTimer_.stop();
}

void voip_proxy::VoipController::_updateCallTime()
{
    callTimeElapsed_ += 1;
    emit onVoipCallTimeChanged(callTimeElapsed_, true);
}

void voip_proxy::VoipController::setAvatars(int _size, const char*_contact, bool forPreview)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    _loadUserBitmaps(collection, _contact, _size);
    if (forPreview)
    {
        collection.set_value_as_string("contact", PREVIEW_RENDER_NAME);
    }
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

std::string voip_proxy::VoipController::formatCallName(const std::vector<std::string>& _names, const char* _clip)
{
    std::string name;
    for (unsigned ix = 0; ix < _names.size(); ix++)
    {
        const std::string& nt = _names[ix];
        assert(!nt.empty());
        if (nt.empty())
        {
            continue;
        }

        if (name.empty())
        {
            // ------------------------------
        } else if (ix == _names.size() - 1 && _clip)
        {
            name += " ";
            name += _clip;
            name += " ";
        }
        else
        {
            name += ", ";
        }
        name += nt;
    }

    assert(!name.empty() || _names.empty());
    return name;
}

void voip_proxy::VoipController::setRequestSettings()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "update");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_loadUserBitmaps(Ui::gui_coll_helper& _collection, const std::string& _contact, int _size)
{

    bool temp = false;
	auto realAvatar = Logic::GetAvatarStorage()->Get(
		QString(_contact.c_str()),
		"",
		_size,//Utils::scale_bitmap(_size),
		!Logic::getContactListModel()->isChat(_contact.c_str()),
		temp,
		false
	);

    assert(!_contact.empty() && _size);
    if (_contact.empty() || !_size)
    {
        return;
    }

    _collection.set_value_as_string("type", "converted_avatar");
    _collection.set_value_as_string("contact", _contact);
    _collection.set_value_as_int("size", _size);

    auto packAvatar = [&_collection](const QImage& image, const std::string& prefix)
    {
        const auto dataSize = image.byteCount();
        const auto dataPtr = image.bits();

        assert(dataSize && dataPtr);
        if (dataSize && dataPtr)
        {
            core::ifptr<core::istream> avatar_stream(_collection->create_stream());
            avatar_stream->write(dataPtr, dataSize);
            _collection.set_value_as_stream(prefix + "avatar", avatar_stream.get());
            _collection.set_value_as_int(prefix + "avatar_height", image.height());
            _collection.set_value_as_int(prefix + "avatar_width", image.width());
        }
    };

    if (realAvatar)
    {
        const auto rawAvatar = realAvatar->toImage();
        if (INCLUDE_AVATAR)
        {// simple avatar
            const int avatarSize = Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize);

            QImage image(QSize(avatarSize, avatarSize), QImage::Format_ARGB32);
            image.fill(Qt::transparent);

            QPainter painter(&image);

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);

            QPainterPath path(QPointF(0, 0));
            path.addEllipse(0, 0, avatarSize, avatarSize);
            painter.setClipPath(path);

            painter.drawImage(QRect(0, 0, avatarSize, avatarSize), rawAvatar);
            packAvatar(image, "");
        }

        if (INCLUDE_BLUR_BACKGROUND) {
            const int avatarSize = Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize);

            // In conference we have 3/4 aspect.
            QImage image(QSize(avatarSize, avatarSize * 3.0f / 4.0f), QImage::Format_ARGB32);
            image.fill(Qt::transparent);

            QPainter painter(&image);

            QGraphicsBlurEffect* blur = new QGraphicsBlurEffect();
            blur->setBlurRadius(8);

            QGraphicsScene scene;
            QGraphicsPixmapItem item;
            int cropHeight = (rawAvatar.height() - rawAvatar.width() * 3.0f / 4.0f);
            item.setPixmap(QPixmap::fromImage(rawAvatar.copy(0, cropHeight / 2, rawAvatar.width(), rawAvatar.height() - cropHeight)));
            item.setGraphicsEffect(blur);
            scene.addItem(&item);
            scene.render(&painter, QRectF(), QRectF());

            painter.fillRect(rawAvatar.rect(), QBrush(QColor(255, 255, 255, 255 / 10)));

            packAvatar(image, "background_");
        }

        if (INCLUDE_REMOTE_CAMERA_OFF) {// no remote video avatar
            const int avatarNoRemoteVideoSz = Utils::scale_bitmap_with_value(160);
            const int xyOffset = (Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize) - avatarNoRemoteVideoSz) / 2;

            QImage image(Utils::scale_bitmap_with_value(QSize(voip_manager::kAvatarDefaultSize, voip_manager::kAvatarDefaultSize)), QImage::Format_ARGB32);
            image.fill(Qt::transparent);

            QPainter painter(&image);

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);

            QPainterPath path(QPointF(0, 0));
            path.addEllipse(xyOffset, xyOffset, avatarNoRemoteVideoSz, avatarNoRemoteVideoSz);
            painter.setClipPath(path);

            painter.drawImage(QRect(xyOffset, xyOffset, avatarNoRemoteVideoSz, avatarNoRemoteVideoSz), rawAvatar);
            QColor noVideoColor("#000000");
            noVideoColor.setAlphaF(0.5);
            painter.fillRect(QRect(0, 0, Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize),
                Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize)), noVideoColor);
            packAvatar(image, "rem_camera_offline_");
        }
    }

    {// signs
		QString tmpContact;

		if (Ui::MyInfo()->aimId() == QString::fromStdString(_contact))
		{
			tmpContact = Ui::MyInfo()->friendlyName();
		}
		else
		{
			tmpContact = (Logic::getContactListModel()->getDisplayName(_contact.c_str()));
		}
        if (tmpContact.isEmpty())
        {
            tmpContact = _contact.c_str();
        }

        if (INCLUDE_USER_NAME)
        {// normal
            QPainter painter;

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            const auto fontSize = 15;
            QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(fontSize));
            font.setStyleStrategy(QFont::PreferAntialias);

            QFontMetrics fm(font);
            const QSize textSz = fm.size(Qt::TextSingleLine, tmpContact);
            const int nickW = Utils::scale_bitmap_with_value(voip_manager::kNickTextW);
            const int nickH = Utils::scale_bitmap_with_value(voip_manager::kNickTextH);

            QPixmap pm(nickW, nickH);
            pm.fill(Qt::transparent);

            const QPen pen(QColor("#000000"));

            painter.begin(&pm);
            painter.setPen(pen);
            painter.setFont(font);

            Ui::CompiledText compiledText;
            Ui::CompiledText::compileText(tmpContact, compiledText, false, false);

            // Align to center.
            int xOffset = std::max((nickW - compiledText.width(fm)) / 2, 0);
            int yOffset = std::max(nickH - Utils::scale_bitmap_with_value(fontSize) / 2, 0);
            compiledText.draw(painter, xOffset, yOffset, nickW);

            painter.end();

            packAvatar(pm.toImage(), "sign_normal_");
        }

        if (INCLUDE_USER_NAME_IN_CONF)
        {// header
            QPainter painter;

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            const QSize border = QSize(Utils::scale_bitmap_with_value(10), 0);

            QFont font = Fonts::appFont(Utils::scale_bitmap_with_value(15));
            font.setStyleStrategy(QFont::PreferAntialias);
            QColor confUserName("#000000");
            const QPen pen(confUserName);

            QSize textSz;
            { // measure text size
                QPixmap pm(1, 1);
                painter.begin(&pm);
                painter.setPen(pen);
                painter.setFont(font);

                QFontMetrics fMetrics = painter.fontMetrics();
                textSz = Utils::scale_bitmap(fMetrics.size( Qt::TextSingleLine, tmpContact));
                painter.end();
            }

            { // draw
                // Make too long header.
                QImage image(textSz.width() + border.width() * 2 + Utils::scale_bitmap(1920), Utils::scale_bitmap_with_value(32), QImage::Format_ARGB32);
                
                //pm.fill(QColor(255, 255, 255, 255));

                QColor headerBackground("#ffffff");
                headerBackground.setAlphaF(0.7);

                //pm.fill(headerBackground);
                painter.begin(&image);

                painter.setCompositionMode(QPainter::CompositionMode_Clear);
                painter.fillRect(image.rect(), Qt::black);
                painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                painter.fillRect(image.rect(), headerBackground);
                
                painter.setPen(pen);
                painter.setFont(font);

                painter.drawText(QRect(border.width(), border.height(), textSz.width(), image.height()), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, tmpContact);
                painter.end();

                packAvatar(image, "sign_header_");
            }
        }
    }
}

void voip_proxy::VoipController::handlePacket(core::coll_helper& _collParams)
{
    const std::string sigType = _collParams.get_value_as_string("sig_type");
    if (sigType == "video_window_show")
    {
        const bool enable = _collParams.get_value_as_bool("param");

        emit onVoipShowVideoWindow(enable);
    }
    else if (sigType == "device_vol_changed")
    {
        int volumePercent = _collParams.get_value_as_int("volume_percent");
        const std::string deviceType = _collParams.get_value_as_string("device_type");

        volumePercent = std::min(100, std::max(volumePercent, 0));
        emit onVoipVolumeChanged(deviceType, volumePercent);
    }
    else if (sigType == "voip_reset_complete")
    {
		voipIsKilled_ = true;
        emit onVoipResetComplete();
    }
    else if (sigType == "device_muted")
    {
        const bool muted = _collParams.get_value_as_bool("muted");
        const std::string deviceType = _collParams.get_value_as_string("device_type");

        emit onVoipMuteChanged(deviceType, muted);
    }
    else if (sigType == "call_invite")
    {
        const std::string account = _collParams.get_value_as_string("account");
        const std::string contact = _collParams.get_value_as_string("contact");

        emit onVoipCallIncoming(account, contact);
    }
    else if (sigType == "call_peer_list_changed")
    {
        voip_manager::ContactsList contacts;
        contacts << _collParams;

        if (contacts.isActive)
        {
            activePeerList_ = contacts;
        }
        emit onVoipCallNameChanged(contacts);
    }
    else if (sigType == "voip_window_add_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;

        emit onVoipWindowAddComplete(hwnd);
    }
    else if (sigType == "voip_window_remove_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;

        emit onVoipWindowRemoveComplete(hwnd);
    }
    else if (sigType == "call_in_accepted")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        emit onVoipCallIncomingAccepted(contactEx);
    }
    else if (sigType == "device_list_changed")
    {
        const int deviceCount = _collParams.get_value_as_int("count");
        const voip2::DeviceType deviceType = (voip2::DeviceType)_collParams.get_value_as_int("type");

        auto& devices = devices_[deviceType];

        devices.clear();
        devices.reserve(deviceCount);

        if (deviceCount > 0)
        {
            for (int ix = 0; ix < deviceCount; ix++)
            {
                std::stringstream ss;
                ss << "device_" << ix;

                auto device = _collParams.get_value_as_collection(ss.str().c_str());
                assert(device);
                if (device)
                {
                    core::coll_helper device_helper(device, false);

                    device_desc desc;
                    desc.name = device_helper.get_value_as_string("name");
                    desc.uid  = device_helper.get_value_as_string("uid");
                    desc.isActive = device_helper.get_value_as_bool("is_active");

                    const std::string type = device_helper.get_value_as_string("device_type");
                    desc.dev_type = type == "audio_playback" ? kvoipDevTypeAudioPlayback :
                        type == "video_capture"  ? kvoipDevTypeVideoCapture :
                        type == "audio_capture"  ? kvoipDevTypeAudioCapture :
                        kvoipDevTypeUndefined;

                    assert(desc.dev_type != kvoipDevTypeUndefined);
                    if (desc.dev_type != kvoipDevTypeUndefined)
                    {
                        devices.push_back(desc);
                    }
                }
            }
        }

        assert((deviceCount > 0 && !devices.empty()) || (!deviceCount && devices.empty()));
        emit onVoipDeviceListUpdated((EvoipDevTypes)(deviceType + 1), devices);
    }
    else if (sigType == "media_loc_a_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("param");

        emit onVoipMediaLocalAudio(enabled);
    }
    else if (sigType == "media_loc_v_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("param");

        emit onVoipMediaLocalVideo(enabled);
    }
    else if (sigType == "mouse_tap")
    {
        const quintptr hwnd = _collParams.get_value_as_int64("hwnd");
        const std::string tapType(_collParams.get_value_as_string("mouse_tap_type"));

        emit onVoipMouseTapped(hwnd, tapType);
    }
    else if (sigType == "button_tap")
    {
        voip_manager::ButtonTap button;
        button << _collParams;

        if (button.type == voip2::ButtonType_Close)
        {
            setDecline(button.contact.c_str(), false);
        }

        emit onVoipButtonTapped(button);
    }
    else if (sigType == "call_created")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        // We use sound_enabled settings for voip ring.
        // Set sound to 1.0.
        if (!haveEstablishedConnection_)
        {
            onStartCall(!contactEx.incoming);
        }

        haveEstablishedConnection_ = true;

        emit onVoipCallCreated(contactEx);
        Ui::GetSoundsManager()->callInProgress(true);
    }
    else if (sigType == "call_destroyed")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        if (contactEx.call_count <= 1)
        {
            callTimeTimer_.stop();
            callTimeElapsed_ = 0;
            haveEstablishedConnection_ = false;
            onEndCall();

            emit onVoipCallTimeChanged(callTimeElapsed_, false);
            Ui::GetSoundsManager()->callInProgress(false);
        }

        connectedPeerList_.remove(contactEx.contact);
        emit onVoipCallDestroyed(contactEx);
    }
    else if (sigType == "call_connected")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        // TODO: When we will have video conference, we need to update this code.
        // On reconnect we will not reset timer.
        auto it = std::find (connectedPeerList_.begin(), connectedPeerList_.end(), contactEx.contact);
        if (it == connectedPeerList_.end())
        {
            callTimeElapsed_ = 0;
        }
        emit onVoipCallTimeChanged(callTimeElapsed_, true);

        if (!callTimeTimer_.isActive())
        {
            // Disable mute, if it was set for ring.
            setAPlaybackMute(false);
            callTimeTimer_.start();
        }

        connectedPeerList_.push_back(contactEx.contact);
        emit onVoipCallConnected(contactEx);
    }
    else if (sigType == "frame_size_changed")
    {
        voip_manager::FrameSize fs;
        fs << _collParams;

        emit onVoipFrameSizeChanged(fs);
    }
    else if (sigType == "media_rem_v_changed")
    {
        voip_manager::VideoEnable videoEnable;
        videoEnable << _collParams;
        emit onVoipMediaRemoteVideo(videoEnable);
    }
    else if (sigType == "voip_cipher_state_changed")
    {
        cipherState_ << _collParams;
        emit onVoipUpdateCipherState(cipherState_);
    }
    else if (sigType == "call_out_accepted")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        emit onVoipCallOutAccepted(contactEx);
    }
    else if (sigType == "voip_minimal_bandwidth_state_changed")
    {
        const bool enabled = _collParams.get_value_as_bool("enable");
        emit onVoipMinimalBandwidthChanged(enabled);
    }
	else if (sigType == "voip_mask_engine_enable")
	{
		const bool enabled = _collParams.get_value_as_bool("enable");
		emit onVoipMaskEngineEnable(enabled);
	}
	else if (sigType == "voip_load_mask")
	{
		const bool result       = _collParams.get_value_as_bool("result");
		const std::string name = _collParams.get_value_as_string("name");
		emit onVoipLoadMaskResult(name, result);
	}
    else if (sigType == "layout_changed")
    {
        bool bTray = _collParams.get_value_as_bool("tray");
        std::string layout = _collParams.get_value_as_string("layout_type");
        intptr_t hwnd = _collParams.get_value_as_int64("hwnd");
        
        emit onVoipChangeWindowLayout(hwnd, bTray, layout);
    }
	else if (sigType == "voip_main_video_layout_changed")
	{
		voip_manager::MainVideoLayout mainLayout;
		mainLayout << _collParams;

		if (mainLayout.type == voip_manager::MVL_OUTGOING)
		{
			callTimeTimer_.stop();
			callTimeElapsed_ = 0;
			emit onVoipCallTimeChanged(callTimeElapsed_, false);
		}

		emit onVoipMainVideoLayoutChanged(mainLayout);
	}
}

void voip_proxy::VoipController::updateActivePeerList()
{
    emit onVoipCallNameChanged(activePeerList_);
}

void voip_proxy::VoipController::setHangup()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_stop");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchAPlaybackMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute_switch");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setVolumeAPlayback(int volume)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_volume_change");
    collection.set_value_as_int("volume", std::min(100, std::max(0, volume)));
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchACaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", false);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchVCaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", true);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setMuteSounds(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_sounds_mute");
    collection.set_value_as_bool("mute", _mute);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setActiveDevice(const voip_proxy::device_desc& _description)
{
    assert(!_description.uid.empty());
    if (_description.uid.empty())
    {
        return;
    }

    std::string devType;
    switch (_description.dev_type)
    {
    case kvoipDevTypeAudioPlayback:
    {
        devType = "audio_playback";
        break;
    }
    case kvoipDevTypeAudioCapture:
    {
        devType = "audio_capture";
        break;
    }
    case kvoipDevTypeVideoCapture:
    {
        devType = "video_capture";
        break;
    }
    case kvoipDevTypeUndefined:

    default:
        assert(!"unexpected device type");
        return;
    };

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", devType);
    collection.set_value_as_string("uid", _description.uid);

    Ui::GetDispatcher()->post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptA(const char* _contact)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_accept");
    collection.set_value_as_string("mode", "audio");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptV(const char* _contact)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_accept");
    collection.set_value_as_string("mode", "video");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::voipReset()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_reset");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setDecline(const char* _contact, bool _busy)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_decline");
    collection.set_value_as_string("mode", _busy ? "busy" : "not_busy");
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setStartA(const char* _contact, bool _attach)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    if (_attach)
    {
        setAvatars(AVATAR_VIDE_SIZE, _contact);
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_string("call_type", "audio");
    collection.set_value_as_string("mode", _attach ? "attach" : "not attach");

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setStartV(const char* _contact, bool _attach)
{
    assert(_contact);
    if (!_contact)
    {
        return;
    }

    if (_attach)
    {
        setAvatars(AVATAR_VIDE_SIZE, _contact);
    }

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_string("call_type", "video");
    collection.set_value_as_string("mode", _attach ? "attach" : "not attach");

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowOffsets(quintptr _hwnd, int _l, int _t, int _r, int _b)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_set_window_offsets");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_int("left", _l);
    collection.set_value_as_int("top", _t);
    collection.set_value_as_int("right", _r);
    collection.set_value_as_int("bottom", _b);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowAdd(quintptr _hwnd, bool _primaryWnd, bool _systemWnd, int _panelHeight)
{
    if (_hwnd)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
        {// window
            collection.set_value_as_string("type", "voip_add_window");
            collection.set_value_as_double("scale", Utils::getScaleCoefficient() * Utils::scale_bitmap(1));
            collection.set_value_as_bool("mode", _primaryWnd);
            collection.set_value_as_bool("system_mode", _systemWnd);
            collection.set_value_as_int64("handle", _hwnd);
        }

        if (INCLUDE_CAMERA_OFF_STATUS && _primaryWnd)
        {
            addTextToCollection(collection, "camera_status", "turned off the camera");
            addTextToCollection(collection, "calling", "calling");

        }

        if (INCLUDE_WATERMARK && _primaryWnd)
        {
            addImageToCollection(collection, "video_panel", "watermark", "icq_logo_watermark_", "");
        }

        // Added buttons.
        {
            int w = Utils::scale_bitmap_with_value(32);
            int h = Utils::scale_bitmap_with_value(32);
            addImageToCollection(collection, "basic_elements", "normalButton", "close_a_", "",       w, h);
            addImageToCollection(collection, "basic_elements", "highlightedButton", "close_b_" , "", w, h);
            addImageToCollection(collection, "basic_elements", "pressedButton", "close_b_", "",      w, h); // TODO
            addImageToCollection(collection, "basic_elements", "disabledButton", "close_b_", "",     w, h);
        }

        dispatcher_.post_message_to_core("voip_call", collection.get());

        if (!_systemWnd)
        {
            setWindowOffsets(_hwnd, Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(_panelHeight) * Utils::scale_bitmap(1));
        }

		setWindowBackground(_hwnd, (char)-1, (char)-1, (char)-1, (char)-1);
    }
}

void voip_proxy::VoipController::setWindowRemove(quintptr _hwnd)
{
    if (_hwnd)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
        collection.set_value_as_string("type", "voip_remove_window");
        collection.set_value_as_int64("handle", _hwnd);
        dispatcher_.post_message_to_core("voip_call", collection.get());
    }
}

void voip_proxy::VoipController::getSecureCode(voip_manager::CipherState& _state) const
{
    _state.state      = cipherState_.state;
    _state.secureCode = cipherState_.secureCode;
}

void voip_proxy::VoipController::setAPlaybackMute(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute");
    collection.set_value_as_string("mute", _mute ? "on" : "off");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::loadPlaybackVolumeFromSettings()
{
    bool soundEnabled = Ui::get_gui_settings()->get_value<bool>(settings_sounds_enabled, true);
    setAPlaybackMute(!soundEnabled);
}

void voip_proxy::VoipController::_checkIgnoreContact(QString _contact)
{
    std::string contactName = _contact.toStdString();

    if (
        std::find_if(
            activePeerList_.contacts.begin(), activePeerList_.contacts.end(), [&contactName](const voip_manager::Contact& _contact)
            {
                return _contact.contact == contactName;
            }
            ) != activePeerList_.contacts.end()
        )
    {
        setHangup();
    }
}

void voip_proxy::VoipController::onStartCall(bool _bOutgoing)
{
    setVolumeAPlayback(100);

    // Change volume from application settings only for incomming calls.
    if (!_bOutgoing)
    {
        loadPlaybackVolumeFromSettings();
    }
#ifdef __APPLE__
        // Pause iTunes if we have call.
    iTunesWasPaused_ = iTunesWasPaused_ || platform_macos::pauseiTunes();
#endif

    connect(Logic::getContactListModel(), SIGNAL(ignore_contact(QString)), this, SLOT(_checkIgnoreContact(QString)));
    connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)));

}

void voip_proxy::VoipController::onEndCall()
{
#ifdef __APPLE__
    // Resume iTunes if we paused it.
    if (iTunesWasPaused_)
    {
        platform_macos::playiTunes();
        iTunesWasPaused_ = false;
    }
#endif

    disconnect(Logic::getContactListModel(), SIGNAL(ignore_contact(QString)), this, SLOT(_checkIgnoreContact(QString)));
    disconnect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)));
}

void voip_proxy::VoipController::switchMinimalBandwithMode()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_minimal_bandwidth_switch");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowBackground(quintptr _hwnd, char r, char g, char b, char a)
{
	Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);

	collection.set_value_as_string("type", "backgroung_update");

	core::ifptr<core::istream> background_stream(collection->create_stream());
	const char pixel[] = {r, g, b, a};
	background_stream->write((const uint8_t*)pixel, 4);

	collection.set_value_as_stream("background", background_stream.get());
	collection.set_value_as_int("background_height", 1);
	collection.set_value_as_int("background_width", 1);
	collection.set_value_as_int64("window_handle", _hwnd);

	dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::updateUserAvatar(const voip_manager::ContactEx& _contactEx)
{
	setAvatars(AVATAR_VIDE_SIZE, _contactEx.contact.contact.c_str());
}

void voip_proxy::VoipController::updateUserAvatar(const std::string& _account, const std::string& _contact)
{
	setAvatars(AVATAR_VIDE_SIZE, _contact.c_str());
}

void voip_proxy::VoipController::loadMask(const std::string& maskPath)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_load_mask");
    collection.set_value_as_string("path", maskPath);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowSetPrimary(quintptr _hwnd, const char* _contact)
{
	Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
	collection.set_value_as_string("type", "window_set_primary");
	collection.set_value_as_int64("handle", _hwnd);
	collection.set_value_as_string("contact", _contact);

	dispatcher_.post_message_to_core("voip_call", collection.get());
}

voip_masks::MaskManager* voip_proxy::VoipController::getMaskManager() const
{
    return maskManager_.get();
}

void voip_proxy::VoipController::updateVoipMaskEngineEnable(bool _enable)
{
    maskEngineEnable_ = _enable;
}

bool voip_proxy::VoipController::isMaskEngineEnabled() const
{
    return maskEngineEnable_;
}

void voip_proxy::VoipController::initMaskEngine() const
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_init_mask_engine");

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::resetMaskManager()
{
    maskManager_.reset(new voip_masks::MaskManager(dispatcher_, this));
}

const std::vector<voip_proxy::device_desc>& voip_proxy::VoipController::deviceList(EvoipDevTypes type)
{
    auto arrayIndex = type - 1;
    if (arrayIndex >= 0 && arrayIndex < sizeof(devices_) / sizeof(devices_[0]))
    {
        return devices_[arrayIndex];
    }

    assert(false && "Wrong device type");

    static std::vector<voip_proxy::device_desc> emptyList;

    return emptyList;
}


void voip_proxy::VoipController::setWindowConferenceLayout(quintptr _hwnd, voip_manager::ConferenceLayout& layout)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_window_set_conference_layout");
    collection.set_value_as_int64("handle", _hwnd);
    collection.set_value_as_int("layout", layout);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::onCallCreate(const voip_manager::ContactEx& _contactEx)
{
    setAvatars(360, _contactEx.contact.account.c_str(), true);
}

void voip_proxy::VoipController::avatarChanged(QString aimId)
{
    auto currentContact = aimId.toStdString();
    auto foundContact = std::find_if(activePeerList_.contacts.begin(), activePeerList_.contacts.end(), [&currentContact](const voip_manager::Contact& contact) {
        return currentContact == contact.contact;
    });
    
    // Update only own avatar.
    if (foundContact != activePeerList_.contacts.end() && (activePeerList_.contacts.size() > 1 || !connectedPeerList_.empty()))
    {
        setAvatars(360, currentContact.c_str(), false);
    }
    else if (aimId == Ui::MyInfo()->aimId())
    {
        setAvatars(360, currentContact.c_str(), true);
    }
}

const std::vector<voip_manager::Contact>& voip_proxy::VoipController::currentCallContacts()
{
    return activePeerList_.contacts;
}

bool voip_proxy::VoipController::isVoipKilled()
{
	return voipIsKilled_;
}

bool voip_proxy::VoipController::hasEstablishCall()
{
    return !connectedPeerList_.empty();
}


voip_proxy::VoipEmojiManager::VoipEmojiManager()
: activeMapId_(0)
{

}

voip_proxy::VoipEmojiManager::~VoipEmojiManager()
{

}

bool voip_proxy::VoipEmojiManager::addMap(const unsigned _sw, const unsigned _sh, const std::string& _path, const std::vector<unsigned>& _codePoints, const size_t _size)
{
    assert(!_codePoints.empty());

    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    const size_t id = STRING_ID(_path);
    bool existsSameSize = false;
    bool exactlySame    = false;

    if (it != codeMaps_.end())
    {
        existsSameSize = (it->codePointSize == _size);
        exactlySame    = existsSameSize && (it->id == id);
    }

    if (exactlySame)
    {
        return true;
    }

    if (existsSameSize)
    {
        codeMaps_.erase(it);
    }

    CodeMap codeMap;
    codeMap.id            = id;
    codeMap.path          = _path;
    codeMap.codePointSize = _size;
    codeMap.sw            = _sw;
    codeMap.sh            = _sh;
    codeMap.codePoints    = _codePoints;

    codeMaps_.push_back(codeMap);
    std::sort(
        codeMaps_.begin(), codeMaps_.end(), [] (const CodeMap& l, const CodeMap& r)
        {
            return l.codePointSize < r.codePointSize;
        }
    );
    return true;
}

bool voip_proxy::VoipEmojiManager::getEmoji(const unsigned _codePoint, const size_t _size, QImage& _image)
{
    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    if (it == codeMaps_.end())
    {
        if (codeMaps_.empty())
        {
            return false;
        }
        it = codeMaps_.end();
    }

    const CodeMap& codeMap = *it;
    if (codeMap.id != activeMapId_)
    { // reload code map
        activeMap_.load(codeMap.path.c_str());
        activeMapId_ = codeMap.id;
    }

    const size_t mapW = activeMap_.width();
    const size_t mapH = activeMap_.height();
    assert(mapW != 0 && mapH != 0);

    if (mapW == 0 || mapH == 0)
    {
        return false;
    }

    const size_t rows = mapH / codeMap.sh;
    const size_t cols = mapW / codeMap.sw;

    for (size_t ix = 0; ix < codeMap.codePoints.size(); ++ix)
    {
        const unsigned code = codeMap.codePoints[ix];
        if (code == _codePoint)
        {
            const size_t targetRow = ix / cols;
            const size_t targetCol = ix - (targetRow * cols);

            if (targetRow >= rows)
            {
                return false;
            }

            const QRect r(targetCol * codeMap.sw, targetRow * codeMap.sh, codeMap.sw, codeMap.sh);
            _image = activeMap_.copy(r);
            return true;
        }
    }

    assert(false);
    return false;
}
