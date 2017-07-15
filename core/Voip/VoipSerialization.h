#ifndef __VOIP_SERIALIZATION_H__
#define __VOIP_SERIALIZATION_H__

#include "VoipManagerDefines.h"
#include "../../corelib/collection_helper.h"

inline void operator>>(const voip2::ButtonType& type, core::coll_helper& coll) {
    switch (type) {
    case voip2::ButtonType_Close: coll.set_value_as_string("button_type", "close"); return;
    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::ButtonTap& button_tap, core::coll_helper& coll) {
    coll.set_value_as_string("account", button_tap.account.c_str());
    coll.set_value_as_string("contact", button_tap.contact.c_str());
    coll.set_value_as_int64("hwnd", (int64_t)button_tap.hwnd);
    coll.set_value_as_int("type", button_tap.type);
    button_tap.type >> coll;
}

inline void operator << (voip_manager::ButtonTap& button_tap, core::coll_helper& coll) {

    button_tap.account = coll.get_value_as_string("account");
    button_tap.contact = coll.get_value_as_string("contact");
    button_tap.hwnd = (void*)coll.get_value_as_int64("hwnd");
    button_tap.type = (voip2::ButtonType)coll.get_value_as_int("type");

    assert(!button_tap.account.empty());
    assert(!button_tap.contact.empty());

    coll.set_value_as_string("account", button_tap.account.c_str());
    coll.set_value_as_string("contact", button_tap.contact.c_str());
    coll.set_value_as_int64("hwnd", (int64_t)button_tap.hwnd);
    button_tap.type >> coll;
}

inline void operator>>(const std::vector<std::string>& contacts, core::coll_helper& coll) {
    for (unsigned ix = 0; ix < contacts.size(); ix++) {
        const std::string& contact = contacts[ix];
        if (contact.empty()) {
            assert(false);
            continue;
        }

        std::stringstream ss;
        ss << "contact_" << ix;

        coll.set_value_as_string(ss.str().c_str(), contact.c_str());
    }
}

inline void operator>>(const voip2::DeviceType& type, core::coll_helper& coll) {
    const char* name = "device_type";
    switch (type) {
    case voip2::VideoCapturing: coll.set_value_as_string(name, "video_capture"); return;
    case voip2::AudioRecording: coll.set_value_as_string(name, "audio_capture"); return;
    case voip2::AudioPlayback: coll.set_value_as_string(name, "audio_playback"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::LayoutType& type, core::coll_helper& coll) {
    const char* name = "layout_type";
    switch (type) {
    case voip2::LayoutType_One: coll.set_value_as_string(name, "square_with_detach_preview"); return;
    case voip2::LayoutType_Two: coll.set_value_as_string(name, "square_with_attach_preview"); return;
    case voip2::LayoutType_Three: coll.set_value_as_string(name, "primary_with_detach_preview"); return;
    case voip2::LayoutType_Four: coll.set_value_as_string(name, "primary_with_attach_preview"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::MouseTap& type, core::coll_helper& coll) {
    const char* name = "mouse_tap_type";
    switch (type) {
    case voip2::MouseTap_Single: coll.set_value_as_string(name, "single"); return;
    case voip2::MouseTap_Double: coll.set_value_as_string(name, "double"); return;
    case voip2::MouseTap_Long: coll.set_value_as_string(name, "long"); return;
    case voip2::MouseTap_Over: coll.set_value_as_string(name, "over"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip2::ViewArea& type, core::coll_helper& coll) {
    const char* name = "view_area_type";
    switch (type) {
    case voip2::ViewArea_Primary: coll.set_value_as_string(name, "primary"); return;
    case voip2::ViewArea_Detached: coll.set_value_as_string(name, "detached"); return;
    case voip2::ViewArea_Default: coll.set_value_as_string(name, "default"); return;
    case voip2::ViewArea_Background: coll.set_value_as_string(name, "background"); return;

    default: assert(false); break;
    }
}

inline void operator>>(const voip_manager::DeviceState& state, core::coll_helper& coll) {
    state.type >> coll;
    coll.set_value_as_bool("success", state.success);
    if (!state.uid.empty()) {
        coll.set_value_as_string("uid", state.uid.c_str());
    } else {
        assert(false);
    }
}

inline void operator>>(const intptr_t& ptr, core::coll_helper& coll) {
    coll.set_value_as_int64("pointer", (int64_t)ptr);
}

inline void operator<<(intptr_t& ptr, core::coll_helper& coll) {
    ptr = coll.get_value_as_int64("pointer");
}

inline void operator>>(const voip_manager::DeviceVol& vol, core::coll_helper& coll) {
    vol.type >> coll;
    coll.set_value_as_int("volume_percent", int(vol.volume * 100.0f));
}

inline void operator>>(const voip_manager::DeviceMute& mute, core::coll_helper& coll) {
    mute.type >> coll;
    coll.set_value_as_bool("muted", mute.mute);
}

inline void operator>>(const bool& param, core::coll_helper& coll) {
    coll.set_value_as_bool("param", param);
}

inline void operator>>(const voip_manager::DeviceInterrupt& inter, core::coll_helper& coll) {
    inter.type >> coll;
    coll.set_value_as_bool("interrupt", inter.interrupt);
}

inline void operator>>(const voip_manager::LayoutChanged& type, core::coll_helper& coll) {
    type.layout_type >> coll;
    coll.set_value_as_bool("tray", type.tray);    
    coll.set_value_as_int64("hwnd", (int64_t)type.hwnd);
}

inline void operator>>(const voip_manager::device_description& device, core::coll_helper& coll) {
    coll.set_value_as_string("name", device.name);
    coll.set_value_as_string("uid", device.uid);
    coll.set_value_as_bool("is_active", device.isActive);
    device.type >> coll;
}

inline void operator>>(const voip_manager::device_list& deviceList, core::coll_helper& coll) {
    coll.set_value_as_int("count", (int)deviceList.devices.size());
    coll.set_value_as_int("type",  (int)deviceList.type);

    for (unsigned ix = 0; ix < deviceList.devices.size(); ix++) {
        const voip_manager::device_description& desc = deviceList.devices[ix];
        core::coll_helper device_coll(coll->create_collection(), false);
        desc >> device_coll;

        std::stringstream sstream;
        sstream << "device_" << ix;

        coll.set_value_as_collection(sstream.str(), device_coll.get());
    }
}

inline void operator>>(const voip_manager::FrameSize& fs, core::coll_helper& coll) {
    coll.set_value_as_int64("wnd", fs.hwnd);
    coll.set_value_as_double("aspect_ratio", fs.aspect_ratio);
}

inline void operator>>(const voip_manager::MouseTap& tap, core::coll_helper& coll) {
    coll.set_value_as_string("account", tap.account.c_str());
    coll.set_value_as_string("contact", tap.contact.c_str());

    int64_t hwnd = (int64_t)tap.hwnd;
    coll.set_value_as_int64("hwnd", hwnd);

    tap.tap >> coll;
    tap.area >> coll;
}

inline void operator>>(const voip_manager::MissedCall& missed_call, core::coll_helper& coll) {
    coll.set_value_as_string("account", missed_call.account.c_str());
    coll.set_value_as_string("contact", missed_call.contact.c_str());
    coll.set_value_as_int("ts", missed_call.ts);
}

inline void operator>>(const std::string& param, core::coll_helper& coll) {
    coll.set_value_as_string("param", param.c_str());
}

inline void operator>>(const voip_manager::Contact& contact, core::coll_helper& coll) {
    coll.set_value_as_string("account", contact.account.c_str());
    coll.set_value_as_string("contact", contact.contact.c_str());
}

inline void operator<<(voip_manager::Contact& contact, core::coll_helper& coll) {
    contact.account = coll.get_value_as_string("account");
    contact.contact = coll.get_value_as_string("contact");

    assert(!contact.account.empty());
    assert(!contact.contact.empty());
}

inline void operator>>(const voip_manager::ContactEx& contact_ex, core::coll_helper& coll) {
    contact_ex.contact >> coll;
    coll.set_value_as_int("call_count", contact_ex.call_count);
    coll.set_value_as_int("connection_count", contact_ex.connection_count);
    coll.set_value_as_bool("incoming", contact_ex.incoming);
    coll.set_value_as_int("window_number", contact_ex.windows.size());
    for (int i = 0; i < contact_ex.windows.size(); i++)
    {
        char paramName[32];
        sprintf(paramName, "window_%d", i);
        coll.set_value_as_int64(paramName, (int64_t)contact_ex.windows[i]);
    }
}

inline void operator<<(voip_manager::ContactEx& contact_ex, core::coll_helper& coll) {
    contact_ex.contact << coll;
    contact_ex.call_count       = coll.get_value_as_int("call_count");
    contact_ex.connection_count = coll.get_value_as_int("connection_count");
    contact_ex.incoming = coll.get_value_as_bool("incoming");
    auto windowNumber = coll.get_value_as_int("window_number");
    for (int i = 0; i < windowNumber; i++)
    {
        char paramName[32];
        sprintf(paramName, "window_%d", i);
        contact_ex.windows.push_back((void*)coll.get_value_as_int64(paramName));
    }
}

inline void operator<<(voip_manager::FrameSize& fs, core::coll_helper& coll) {
    fs.aspect_ratio = coll.get_value_as_double("aspect_ratio");
    fs.hwnd = coll.get_value_as_int64("wnd");
}

inline void operator>>(const voip_manager::CipherState& val, core::coll_helper& coll) {
    coll.set_value_as_int("state", int(val.state));
    coll.set_value_as_string("secure_code", val.secureCode);
}

inline void operator<<(voip_manager::CipherState& val, core::coll_helper& coll) {
    val.state = (voip_manager::CipherState::State)coll.get_value_as_int("state");
    val.secureCode = coll.get_value_as_string("secure_code");
}

inline void operator>>(void* hwnd, core::coll_helper& coll) {
    coll.set_value_as_int64("hwnd", (int64_t)hwnd);
}

inline void operator << (void*& hwnd, core::coll_helper& coll) {
    hwnd = (void *)coll.get_value_as_int64("hwnd");
}


template <typename T> void appendVector (const std::vector<T>& vector, core::coll_helper& coll, const std::string& prefix) {

    coll.set_value_as_int(prefix + "Count", (int)vector.size());
    for (unsigned ix = 0; ix < vector.size(); ix++) {
        const T& cont = vector[ix];

        core::coll_helper contact_coll(coll->create_collection(), false);
        (T)cont >> contact_coll;

        std::stringstream sstream;
        sstream << prefix << ix;

        coll.set_value_as_collection(sstream.str(), contact_coll.get());
    }
}

template <typename T> void readVector (std::vector<T>& vector, core::coll_helper& coll, 
    const std::string& prefix){
    const auto count = coll.get_value_as_int((prefix + "Count").c_str());
    if (!count) {
        return;
    }
    vector.clear();
    vector.reserve(count);
    for (int ix = 0; ix < count; ix++) {
        std::stringstream ss;
        ss << prefix << ix;

        auto contact = coll.get_value_as_collection(ss.str().c_str());
        assert(contact);
        if (!contact) { continue; }

        core::coll_helper contact_helper(contact, false);
        T cont_desc;
        cont_desc << contact_helper;

        vector.push_back(cont_desc);
    }
}


inline void operator>>(const voip_manager::ContactsList& contactsList, core::coll_helper& coll) {

    auto contacts = contactsList.contacts;
    appendVector(contacts, coll, "contacts");
    

    auto windows = contactsList.windows;
    appendVector(windows, coll, "windows");

    coll.set_value_as_bool("isActive", contactsList.isActive);
}

inline void operator<<(voip_manager::ContactsList& contactsList/*std::vector<voip_manager::Contact>& contacts*/, 
    core::coll_helper& coll) {

    readVector(contactsList.contacts, coll, "contacts");
    readVector(contactsList.windows, coll, "windows");
    contactsList.isActive = coll.get_value_as_bool("isActive");
}

inline void operator>>(const voip_manager::EnableParams& mdw, core::coll_helper& coll) {
    coll.set_value_as_bool("enable", mdw.enable);
}

inline void operator >> (const voip_manager::NamedResult& value, core::coll_helper& coll) {
	coll.set_value_as_bool("result", value.result);
	coll.set_value_as_string("name", value.name);
}

inline void operator >> (const voip_manager::VideoEnable& value, core::coll_helper& coll) {
    coll.set_value_as_bool("enable", value.enable);
    value.contact >> coll;
}

inline void operator << (voip_manager::VideoEnable& value, core::coll_helper& coll) {
    value.enable = coll.get_value_as_bool("enable");
    value.contact << coll;
}

inline void operator >> (const voip_manager::MainVideoLayout& value, core::coll_helper& coll) {
	coll.set_value_as_int64("hwnd", (int64_t)value.hwnd);
	coll.set_value_as_int("type", (int)value.type);
}

inline void operator << (voip_manager::MainVideoLayout& value, core::coll_helper& coll) {
	value.hwnd = (void*)coll.get_value_as_int64("hwnd");
	value.type = (voip_manager::MainVideoLayoutType)coll.get_value_as_int("type");
}

inline void operator>>(const voip_manager::eNotificationTypes& type, core::coll_helper& coll) {
    const char* name = "sig_type";

    using namespace voip_manager;
    switch (type) {
    case kNotificationType_Undefined:   coll.set_value_as_string(name, "undefined");    return;
    case kNotificationType_CallCreated: coll.set_value_as_string(name, "call_created"); return;
    case kNotificationType_CallInvite:  coll.set_value_as_string(name, "call_invite");  return;
    case kNotificationType_CallOutAccepted: coll.set_value_as_string(name, "call_out_accepted"); return;
    case kNotificationType_CallInAccepted: coll.set_value_as_string(name, "call_in_accepted"); return;
    case kNotificationType_CallConnected: coll.set_value_as_string(name, "call_connected"); return;
    case kNotificationType_CallDisconnected: coll.set_value_as_string(name, "call_disconnected"); return;
    case kNotificationType_CallDestroyed: coll.set_value_as_string(name, "call_destroyed"); return;
    case kNotificationType_CallPeerListChanged: coll.set_value_as_string(name, "call_peer_list_changed"); return;

    case kNotificationType_QualityChanged: coll.set_value_as_string(name, "quality_changed"); return;

    case kNotificationType_MediaLocAudioChanged: coll.set_value_as_string(name, "media_loc_a_changed"); return;
    case kNotificationType_MediaLocVideoChanged: coll.set_value_as_string(name, "media_loc_v_changed"); return;
    case kNotificationType_MediaRemVideoChanged: coll.set_value_as_string(name, "media_rem_v_changed"); return;
    case kNotificationType_MediaRemAudioChanged: coll.set_value_as_string(name, "media_rem_a_changed"); return;

    case kNotificationType_DeviceListChanged: coll.set_value_as_string(name, "device_list_changed"); return;
    case kNotificationType_DeviceStarted: coll.set_value_as_string(name, "device_started"); return;
    case kNotificationType_DeviceMuted: coll.set_value_as_string(name, "device_muted"); return;
    case kNotificationType_DeviceVolChanged: coll.set_value_as_string(name, "device_vol_changed"); return;
    case kNotificationType_DeviceInterrupt: coll.set_value_as_string(name, "device_interrupt"); return;

    case kNotificationType_MouseTap: coll.set_value_as_string(name, "mouse_tap"); return;
    case kNotificationType_ButtonTap: coll.set_value_as_string(name, "button_tap"); return;
    case kNotificationType_LayoutChanged: coll.set_value_as_string(name, "layout_changed"); return;

    case kNotificationType_MissedCall: coll.set_value_as_string(name, "missed_call"); return;
    case kNotificationType_ShowVideoWindow: coll.set_value_as_string(name, "video_window_show"); return;
    case kNotificationType_FrameSizeChanged: coll.set_value_as_string(name, "frame_size_changed"); return;
    case kNotificationType_VoipResetComplete: coll.set_value_as_string(name, "voip_reset_complete"); return;
    case kNotificationType_VoipWindowRemoveComplete: coll.set_value_as_string(name, "voip_window_remove_complete"); return;
    case kNotificationType_VoipWindowAddComplete: coll.set_value_as_string(name, "voip_window_add_complete"); return;

    case kNotificationType_CipherStateChanged:  coll.set_value_as_string(name, "voip_cipher_state_changed"); return;

	case kNotificationType_MinimalBandwidthChanged: coll.set_value_as_string(name, "voip_minimal_bandwidth_state_changed"); return;
	case kNotificationType_MaskEngineEnable: coll.set_value_as_string(name, "voip_mask_engine_enable"); return;
	case kNotificationType_LoadMask: coll.set_value_as_string(name, "voip_load_mask"); return;

    case kNotificationType_ConnectionDestroyed: /* Nothing to do for now */ return;

	case kNotificationType_MainVideoLayoutChanged: coll.set_value_as_string(name, "voip_main_video_layout_changed"); return;

    default: assert(false); return;
    }
}

#endif//__VOIP_SERIALIZATION_H__