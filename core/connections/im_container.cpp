#include "stdafx.h"
#include "im_container.h"
#include "login_info.h"
#include "im_login.h"
#include "wim/chat_params.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "wim/wim_im.h"
#include "wim/wim_packet.h"
#include "../Voip/VoipManager.h"

#include "../core.h"
#include "../tools/system.h"
#include "../utils.h"
#include "../archive/contact_archive.h"
#include "../archive/history_message.h"
#include "../statistics.h"
#include "../themes/themes.h"
#include "../proxy_settings.h"

#include "../configuration/app_config.h"

using namespace core;

voip_manager::BitmapDescription get_bitmap_from_stream(core::istream* stream, int w, int h)
{
    voip_manager::BitmapDescription res;
    res.size = stream->size();
    res.data = (void*)stream->read(stream->size());
    res.w = w;
    res.h = h;

    return res;
}

im_container::im_container(std::shared_ptr<voip_manager::VoipManager> voip_manager)
    :   voip_manager_(voip_manager),
        logins_(new im_login_list(utils::get_product_data_path() + L"/settings/ims.stg"))
{
    REGISTER_IM_MESSAGE("login_by_password", on_login_by_password);
    REGISTER_IM_MESSAGE("login_by_password_for_attach_uin", on_login_by_password_for_attach_uin);
    REGISTER_IM_MESSAGE("login_get_sms_code", on_login_get_sms_code);
    REGISTER_IM_MESSAGE("login_by_phone", on_login_by_phone);
    REGISTER_IM_MESSAGE("logout", on_logout);
    REGISTER_IM_MESSAGE("connect_after_migration", on_connect_after_migration);
    REGISTER_IM_MESSAGE("avatars/get", on_get_contact_avatar);
    REGISTER_IM_MESSAGE("avatars/show", on_show_contact_avatar);
    REGISTER_IM_MESSAGE("send_message", on_send_message);
    REGISTER_IM_MESSAGE("message/typing", on_message_typing);
    REGISTER_IM_MESSAGE("feedback/send", on_feedback);
    REGISTER_IM_MESSAGE("set_state", on_set_state);
    REGISTER_IM_MESSAGE("archive/images/get", on_get_archive_images);
    REGISTER_IM_MESSAGE("archive/images/repair", on_repair_archive_images);
    REGISTER_IM_MESSAGE("archive/index/get", on_get_archive_index);
    REGISTER_IM_MESSAGE("archive/buddies/get", on_get_archive_messages_buddies);
    REGISTER_IM_MESSAGE("archive/messages/get", on_get_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete", on_delete_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete_from", on_delete_archive_messages_from);

    REGISTER_IM_MESSAGE("history_search", on_history_search);
    REGISTER_IM_MESSAGE("history_search_ended", on_history_search_ended);

    REGISTER_IM_MESSAGE("dialogs/add", on_add_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/remove", on_remove_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/set_first_message", on_set_first_message);
    REGISTER_IM_MESSAGE("dialogs/hide", on_hide_chat);
    REGISTER_IM_MESSAGE("dialogs/mute", on_mute_chat);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read", on_set_last_read);
    REGISTER_IM_MESSAGE("voip_call", on_voip_call_message);
    REGISTER_IM_MESSAGE("files/upload", on_upload_file_sharing);
    REGISTER_IM_MESSAGE("files/upload/abort", on_abort_file_sharing_uploading);
    REGISTER_IM_MESSAGE("files/download/preview_size", on_get_file_sharing_preview_size);
    REGISTER_IM_MESSAGE("files/download/metainfo", on_download_file_sharing_metainfo);
    REGISTER_IM_MESSAGE("files/download", on_download_file);
    REGISTER_IM_MESSAGE("files/download/abort", on_abort_file_downloading);
    REGISTER_IM_MESSAGE("image/download", on_download_image);
    REGISTER_IM_MESSAGE("image/download/cancel", on_cancel_image_downloading);
    REGISTER_IM_MESSAGE("link_metainfo/download", on_download_link_preview);
    REGISTER_IM_MESSAGE("download/raise_priority", on_download_raise_priority);
    REGISTER_IM_MESSAGE("stickers/meta/get", on_get_stickers_meta);
    REGISTER_IM_MESSAGE("stickers/sticker/get", on_get_sticker);
    REGISTER_IM_MESSAGE("chats/info/get", on_get_chat_info);
    REGISTER_IM_MESSAGE("chats/blocked/get", on_get_chat_blocked);
    REGISTER_IM_MESSAGE("chats/pending/get", on_get_chat_pending);
    REGISTER_IM_MESSAGE("chats/home/get", on_get_chat_home);
    REGISTER_IM_MESSAGE("chats/pending/resolve", on_resolve_pending);
    REGISTER_IM_MESSAGE("contacts/search", on_search_contacts);
    REGISTER_IM_MESSAGE("contacts/profile/get", on_profile);
    REGISTER_IM_MESSAGE("contacts/add", on_add_contact);
    REGISTER_IM_MESSAGE("contacts/remove", on_remove_contact);
    REGISTER_IM_MESSAGE("contacts/rename", on_rename_contact);
    REGISTER_IM_MESSAGE("contacts/block", on_spam_contact);
    REGISTER_IM_MESSAGE("contacts/ignore", on_ignore_contact);
    REGISTER_IM_MESSAGE("contacts/get_ignore", on_get_ignore_contacts);
    REGISTER_IM_MESSAGE("contact/switched", on_contact_switched);
    REGISTER_IM_MESSAGE("dlg_state/hide", on_hide_dlg_state);
    REGISTER_IM_MESSAGE("remove_members", on_remove_members);
    REGISTER_IM_MESSAGE("add_members", on_add_members);
    REGISTER_IM_MESSAGE("add_chat", on_add_chat);
    REGISTER_IM_MESSAGE("modify_chat", on_modify_chat);
    REGISTER_IM_MESSAGE("sign_url", on_sign_url);
    REGISTER_IM_MESSAGE("stats", on_stats);
    REGISTER_IM_MESSAGE("themes/meta/get", on_get_themes_meta);
    REGISTER_IM_MESSAGE("themes/theme/get", on_get_theme);
    REGISTER_IM_MESSAGE("files/set_url_played", on_url_played);
    REGISTER_IM_MESSAGE("files/speech_to_text", on_speech_to_text);
    REGISTER_IM_MESSAGE("favorite", on_favorite);
    REGISTER_IM_MESSAGE("unfavorite", on_unfavorite);
    REGISTER_IM_MESSAGE("load_flags", on_get_flags);
    REGISTER_IM_MESSAGE("update_profile", on_update_profile);
    REGISTER_IM_MESSAGE("set_user_proxy_settings", on_set_user_proxy);
    REGISTER_IM_MESSAGE("livechat/join", on_join_livechat);
    REGISTER_IM_MESSAGE("set_locale", on_set_locale);
    REGISTER_IM_MESSAGE("set_avatar", on_set_avatar);

    REGISTER_IM_MESSAGE("chats/create", on_create_chat);

    REGISTER_IM_MESSAGE("chats/mod/params", on_mod_chat_params);
    REGISTER_IM_MESSAGE("chats/mod/name", on_mod_chat_name);
    REGISTER_IM_MESSAGE("chats/mod/about", on_mod_chat_about);
    REGISTER_IM_MESSAGE("chats/mod/public", on_mod_chat_public);
    REGISTER_IM_MESSAGE("chats/mod/join", on_mod_chat_join);
    REGISTER_IM_MESSAGE("chats/mod/link", on_mod_chat_link);
    REGISTER_IM_MESSAGE("chats/mod/ro", on_mod_chat_ro);
    REGISTER_IM_MESSAGE("chats/mod/age", on_mod_chat_age);
    
    REGISTER_IM_MESSAGE("chats/block", on_block_chat_member);
    REGISTER_IM_MESSAGE("chats/role/set", on_set_chat_member_role);
    REGISTER_IM_MESSAGE("close_promo", on_close_promo);
    REGISTER_IM_MESSAGE("phoneinfo", on_phoneinfo);
    REGISTER_IM_MESSAGE("snap/mark_as_read", on_snap_read);
    REGISTER_IM_MESSAGE("snap/delete", on_delete_read);
    REGISTER_IM_MESSAGE("snap/get_metainfo", on_snap_download_metainfo);
    REGISTER_IM_MESSAGE("masks/get_id_list", on_get_mask_id_list);
    REGISTER_IM_MESSAGE("masks/preview/get", on_get_mask_preview);
    REGISTER_IM_MESSAGE("masks/model/get", on_get_mask_model);
    REGISTER_IM_MESSAGE("masks/get", on_get_mask);
    REGISTER_IM_MESSAGE("mrim/get_key", on_mrim_get_key);
    REGISTER_IM_MESSAGE("snaps/refresh", on_snaps_refresh);
    REGISTER_IM_MESSAGE("snaps/refresh_user_snaps", on_refresh_user_snaps);
    REGISTER_IM_MESSAGE("snaps/remove_from_cache", on_remove_from_snaps_storage);
}


im_container::~im_container()
{
}

void core::im_container::create()
{
    if (create_ims())
    {
        connect_ims();
    }
}

void im_container::connect_ims()
{
    for (auto iter = ims_.begin(); iter < ims_.end(); iter++)
        (*iter)->connect();
}

bool im_container::update_login(im_login_id& _login)
{
    return logins_->update(_login);
}

void im_container::replace_uin_in_login(im_login_id& old_login, im_login_id& new_login)
{
    logins_->replace_uin(old_login, new_login);
}

bool core::im_container::create_ims()
{
    if (logins_->load())
    {
    }

    im_login_id login("", default_im_id);

    if (logins_->get_first_login(login))
    {
    }

    ims_.push_back(std::make_shared<wim::im>(login, voip_manager_));

    return !ims_.empty();
}

std::string core::im_container::get_first_login() const
{
    im_login_id login("", default_im_id);
    logins_->get_first_login(login);
    return login.get_login();
}

void core::im_container::on_message_from_gui(const char * _message, int64_t _seq, coll_helper& _params)
{
    auto iter_handler = messages_map_.find(_message);
    if (iter_handler == messages_map_.end())
    {
        assert(!"unknown message type");
        return;
    }

    iter_handler->second(_seq, _params);
}

void core::im_container::fromInternalProxySettings2Voip(const core::proxy_settings& proxySettings, voip_manager::VoipProxySettings& voipProxySettings) {
    using namespace voip_manager;

    if (!proxySettings.use_proxy_) {
        voipProxySettings.type          = VoipProxySettings::kProxyType_None;
    } else {
        switch (proxySettings.proxy_type_) {
        case 0:  
            voipProxySettings.type = VoipProxySettings::kProxyType_Http;
            break;

        case 4:  
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks4;
            break;

        case 5:  
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks5;
            break;

        case 6:  
            voipProxySettings.type = VoipProxySettings::kProxyType_Socks4a;
            break;

        default: 
            voipProxySettings.type = VoipProxySettings::kProxyType_None;
        }
    }

    voipProxySettings.serverUrl    = proxySettings.proxy_server_;
    voipProxySettings.userName     = proxySettings.login_;
    voipProxySettings.userPassword = proxySettings.password_;
}

void core::im_container::on_voip_call_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    const std::string type = _params.get_value_as_string("type");
    if (!im && type != "voip_reset") {
        return;
    }

    if (type == "voip_call_start") {
        const std::string call_type = _params.get_value_as_string("call_type");
        const std::string mode      = _params.get_value_as_string("mode");
        const std::string contact   = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_start(contact, call_type == "video", mode == "attach");
        };
    } else if (type == "voip_add_window") {
        voip_manager::WindowParams windowParams = {};
        windowParams.hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        windowParams.isPrimary = _params.get_value_as_bool("mode");
        windowParams.isSystem  = _params.get_value_as_bool("system_mode");
        windowParams.scale     = (float)_params.get_value_as_double("scale");

        std::vector<core::istream*> steramsForFree;

        // Function to get bitmap from params.
        auto getBitmapDescription = [&_params, &steramsForFree](const std::string& prefix, voip_manager::BitmapDescription& outBitmap) -> bool {
            bool res = false;
            if (_params.is_value_exist(prefix.c_str())) {
                core::istream* statusStream = _params.get_value_as_stream(prefix.c_str());
                const auto h = _params.get_value_as_int((prefix + "_height").c_str());
                const auto w = _params.get_value_as_int((prefix + "_width").c_str());
                assert(statusStream);
                if (statusStream) {
                    const auto stream_size = statusStream->size();
                    assert(stream_size);
                    if (stream_size > 0 && h > 0 && w > 0) {
                        outBitmap = get_bitmap_from_stream(statusStream, w, h);
                        res = true;
                    }

                    steramsForFree.push_back(statusStream);
                }
            }

            return res;
        };

        if (getBitmapDescription("camera_status", windowParams.cameraStatus.bitmap))
        {
            windowParams.cameraStatus.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("calling", windowParams.calling.bitmap))
        {
            windowParams.calling.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("watermark", windowParams.watermark.bitmap))
        {
            windowParams.watermark.hwnd = windowParams.hwnd;
        }

        std::string buttonsParams[] = { "normalButton" , "disabledButton", "pressedButton" , "highlightedButton" };
        voip_manager::BitmapDescription* buttons[] = {&windowParams.normalButton, &windowParams.disabledButton, 
            &windowParams.pressedButton,  &windowParams.highlightedButton};

        for (int i = 0; i < sizeof(buttonsParams) / sizeof(buttonsParams[0]); i ++)
        {
            getBitmapDescription(buttonsParams[i], *buttons[i]);
        }

        im->on_voip_add_window(windowParams);

        for (auto stream : steramsForFree)
        {
            if (stream) {
                stream->reset();
            }
        }
    } else if (type == "voip_remove_window") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        im->on_voip_remove_window(hwnd);
    } else if (type == "voip_call_stop") {
        im->on_voip_call_stop();
    } else if (type == "voip_call_volume_change") {
        const int vol = _params.get_value_as_int("volume");
        im->on_voip_volume_change(vol);
    } else if (type == "audio_playback_mute_switch") {
        im->on_voip_mute_switch();
    } else if (type == "voip_call_media_switch") {
        const bool video = _params.get_value_as_bool("video");
        im->on_voip_switch_media(video);
    } else if (type == "voip_sounds_mute") {
        const bool mute = _params.get_value_as_bool("mute");
        im->on_voip_mute_incoming_call_sounds(mute);
    } else if (type == "voip_call_decline") {
        const std::string mode = _params.get_value_as_string("mode");
        im->on_voip_call_end(_params.get_value_as_string("contact"), mode == "busy");
    } else if (type == "converted_avatar") {
        on_voip_avatar_msg(im, _params);
    } else if (type == "backgroung_update") {
        on_voip_background_msg(im, _params);
    } else if (type == "voip_call_accept") {
        const std::string mode    = _params.get_value_as_string("mode");
        const std::string contact = _params.get_value_as_string("contact");

        assert(!contact.empty());
        if (!contact.empty()) {
            core::proxy_settings            proxySettings = g_core->get_proxy_settings();
            voip_manager::VoipProxySettings voipProxySettings;
            fromInternalProxySettings2Voip(proxySettings, voipProxySettings);

            im->on_voip_call_set_proxy(voipProxySettings);
            im->on_voip_call_accept(contact, mode == "video");
        }
    } else if (type == "device_change") {
        const std::string dev_type = _params.get_value_as_string("dev_type");
        const std::string uid = _params.get_value_as_string("uid");
        im->on_voip_device_changed(dev_type, uid);
    } else if (type == "request_calls") {
        im->on_voip_call_request_calls();
    } else if (type == "update") {
        im->on_voip_update();
    } else if (type == "voip_set_window_offsets") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        auto l = _params.get_value_as_int("left");
        auto t = _params.get_value_as_int("top");
        auto r = _params.get_value_as_int("right");
        auto b = _params.get_value_as_int("bottom");

        im->on_voip_window_set_offsets(hwnd, l, t, r, b);
    } else if (type == "voip_reset") {
        if (!im) {
#ifndef STRIP_VOIP
            voip_manager_->get_voip_manager()->reset();
#endif
        } else {
            im->on_voip_reset();
        }
    }
	else if (type == "audio_playback_mute")
	{
		const std::string mode    = _params.get_value_as_string("mute");
		im->on_voip_set_mute(mode == "on");
	}
	else if (type == "voip_minimal_bandwidth_switch")
	{
		im->on_voip_minimal_bandwidth_switch();
	}
	else if (type == "voip_load_mask")
	{
		const std::string maskPath = _params.get_value_as_string("path");
		im->on_voip_load_mask(maskPath);
	}
	else if (type == "window_set_primary")
	{
		void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
		const std::string contact = _params.get_value_as_string("contact");
		im->on_voip_window_set_primary(hwnd, contact);
	}
    else if (type == "voip_init_mask_engine")
    {
        im->on_voip_init_mask_engine();
    }
    else if (type == "voip_window_set_conference_layout")
    {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        const int vol = _params.get_value_as_int("layout");

        im->on_voip_window_set_conference_layout(hwnd, (voip_manager::ConferenceLayout)vol);
    }
	else 
	{
        assert(false);
    }
}

void core::im_container::on_voip_background_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    core::istream* stream = _params.get_value_as_stream("background");
    const auto h          = _params.get_value_as_int("background_height");
    const auto w          = _params.get_value_as_int("background_width");
    void* hwnd            = (void*)(uintptr_t)_params.get_value_as_int64("window_handle");

    assert(stream);
    if (stream) {
        const auto stream_size = stream->size();

        assert(stream_size);
        if (stream_size > 0 && h > 0 && w > 0) {
            im->on_voip_window_update_background(hwnd, stream->read(stream->size()), stream_size, w, h);
            stream->reset();
        }
    }
}

void core::im_container::on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    typedef void (base_im::*__loader_func)(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w);

    auto __load_avatar = [&_params, &im] (const std::string& prefix, __loader_func func) {
        const std::string contact = _params.get_value_as_string("contact");
        const auto size = _params.get_value_as_int("size");

        assert(!contact.empty() && size);
        if (contact.empty() || !size) {
            return;
        }

        assert(!!im);
        if (!im) {
            return;
        }

        if (!_params.is_value_exist((prefix + "avatar").c_str())) {
            return;
        }

        core::istream* stream = _params.get_value_as_stream((prefix + "avatar").c_str());
        const auto h          = _params.get_value_as_int((prefix + "avatar_height").c_str());
        const auto w          = _params.get_value_as_int((prefix + "avatar_width").c_str());

        assert(stream);
        if (stream) {
            const auto stream_size = stream->size();

            assert(stream_size);
            if (stream_size > 0) {
                core::base_im& ptr = *im.get();
                (ptr.*func)(contact, stream->read(stream->size()), stream_size, h, w);
                stream->reset();
            }
        }
    };

    __load_avatar("",                     &base_im::on_voip_user_update_avatar);
    __load_avatar("rem_camera_offline_",  &base_im::on_voip_user_update_avatar_no_video);
    __load_avatar("sign_normal_",         &base_im::on_voip_user_update_avatar_text);
    __load_avatar("sign_header_",         &base_im::on_voip_user_update_avatar_text_header);
    __load_avatar("loc_camera_offline_",  &base_im::on_voip_user_update_avatar_camera_off);
    __load_avatar("loc_camera_disabled_", &base_im::on_voip_user_update_avatar_no_camera);
    __load_avatar("background_",          &base_im::on_voip_user_update_avatar_background);
}

void im_container::on_get_archive_images(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contact = _params.get_value_as_string("contact");
    const auto from = _params.get_value_as_int64("from");
    const auto count = _params.get_value_as_int64("count");

    im->get_archive_images(_seq, contact, from, count);
}

void im_container::on_repair_archive_images(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contact = _params.get_value_as_string("contact");

    im->repair_archive_images(_seq, contact);
}

void im_container::on_get_archive_index(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

	auto last_message_catcher = [](int64_t id)
	{
		assert(0);
	};

    im->get_archive_index(_seq, _params.get_value_as_string("contact"), -1, -1, last_message_catcher);
}


void im_container::on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto ids_list = std::make_shared<core::archive::msgids_list>();

    core::ifptr<core::ihheaders_list> val_headers(_params.get_value_as_hheaders("ids"), false);

    for (auto hdr = val_headers->first(); hdr; hdr = val_headers->next())
        ids_list->push_back(hdr->id_);


    im->get_archive_messages_buddies(_seq, _params.get_value_as_string("contact"), ids_list);
}


void im_container::on_get_archive_messages(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

	std::string contact = _params.get_value_as_string("contact");
	int64_t last_read_msg = _params.get_value_as_int64("last_read_msg");

	auto last_message_catcher = [last_read_msg, contact](int64_t id)
	{
		if (last_read_msg == id)
		{
			coll_helper cl_coll(g_core->create_collection(), true);

			cl_coll.set<std::string>("contact", contact);
			cl_coll.set<int64_t>("last_read_msg", last_read_msg);
			g_core->post_message_to_gui("history_update", 0, cl_coll.get());
		}
	};

    im->get_archive_messages(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_int64("from"),
        _params.get_value_as_int64("count_early"),
        _params.get_value_as_int64("count_later"),
        _params.get_value_as_bool("need_prefetch"),
		last_message_catcher);
}

void core::im_container::on_message_typing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    im->send_message_typing(_seq, _params.get_value_as_string("contact"), (core::typing_status)_params.get_value_as_int("status"));
}

void core::im_container::on_send_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto type = message_type::base;

    std::string message;
    if (_params->is_value_exist("message"))
        message = _params.get_value_as_string("message");

    if (core::configuration::get_app_config().is_crash_enabled_ && message == "!crash")
    {
        throw new std::runtime_error("artificial crash");
    }
    
    bool is_sms = false;
    if (_params->is_value_exist("is_sms"))
    {
        is_sms = _params.get_value_as_bool("is_sms");
        type = message_type::sms;
    }

    bool is_sticker = false;
    if (_params->is_value_exist("sticker"))
    {
        is_sticker = _params.get_value_as_bool("sticker");
        if (is_sticker)
        {
            std::stringstream ss_message;
            ss_message << "ext:" << _params.get_value_as_int("sticker/set_id") << ":" << "sticker:" << _params.get_value_as_int("sticker/id");

            message = ss_message.str();
        }

        type = message_type::sticker;
    }

    core::archive::quotes_vec quotes;
    if (_params->is_value_exist("quotes"))
    {
        core::iarray* array = _params.get_value_as_array("quotes");
        for (auto i = 0; i < array->size(); ++i)
        {
            core::archive::quote q;
            q.unserialize(array->get_at(i)->get_as_collection());
            quotes.push_back(q);
        }
    }

    assert(!(is_sms && is_sticker));

    im->send_message_to_contact(
        _seq,
        _params.get_value_as_string("contact"),
        message,
        type,
        "",
        quotes);
}

void core::im_container::on_feedback(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string url;
    std::map<std::string, std::string> fields;
    std::vector<std::string> files;

    url = _params.get_value_as_string("url");

    fields.insert(std::make_pair("fb.screen_resolution", _params.get_value_as_string("screen_resolution")));
    fields.insert(std::make_pair("fb.referrer", _params.get_value_as_string("referrer")));
    fields.insert(std::make_pair((build::is_icq() ? "fb.question.3004" : "fb.question.7780"), _params.get_value_as_string("version")));
    fields.insert(std::make_pair("fb.question.159", _params.get_value_as_string("os_version")));
    fields.insert(std::make_pair("fb.question.178", _params.get_value_as_string("build_type")));
    fields.insert(std::make_pair((build::is_icq() ? "fb.question.3005" : "fb.question.7782"), _params.get_value_as_string("platform")));
    fields.insert(std::make_pair("fb.user_name", _params.get_value_as_string("user_name")));
    fields.insert(std::make_pair("fb.message", _params.get_value_as_string("message")));
    fields.insert(std::make_pair("fb.communication_email", _params.get_value_as_string("communication_email")));
    fields.insert(std::make_pair("Lang", _params.get_value_as_string("language")));
    fields.insert(std::make_pair("attachements_count", _params.get_value_as_string("attachements_count")));

    if (_params.is_value_exist("attachement"))
    {
        core::iarray* array = _params.get_value_as_array("attachement");
        for (auto i = 0; i < array->size(); ++i)
        {
            files.push_back(array->get_at(i)->get_as_string());
        }
    }

    im->send_feedback(_seq, url, fields, files);
}

void core::im_container::on_phoneinfo(int64_t seq, coll_helper& _params)
{
    std::shared_ptr<base_im> im = get_im(_params);
    if (!im)
    {
        im = std::make_shared<wim::im>(im_login_id(""), voip_manager_);
        ims_.push_back(im);
    }

    std::string phone = _params.get_value_as_string("phone");
    std::string gui_locale = _params.get_value_as_string("gui_locale");

    im->phoneinfo(seq, phone, gui_locale);
}

void core::im_container::on_set_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto state = profile_state::online;
    std::string sstate = _params.get_value_as_string("state");
    if (sstate == "dnd")
        state = profile_state::dnd;
    else if (sstate == "away")
        state = profile_state::away;
    else if (sstate == "invisible")
        state = profile_state::invisible;
    else if (sstate == "offline")
        state = profile_state::offline;

    im->set_state(_seq, state);
}

void core::im_container::on_remove_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_remove"));
}

void core::im_container::on_add_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_members(
        _seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("m_chat_members_to_add"));
}

void core::im_container::on_add_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::string> members;

    auto member_array = _params.get_value_as_array("m_chat_members");
    for (auto i = 0; member_array && i < member_array->size(); ++i)
    {
        members.push_back(member_array->get_at(i)->get_as_string());
    }

    im->add_chat(
        _seq,
        _params.get_value_as_string("m_chat_name"),
        members);
}

void core::im_container::on_modify_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->modify_chat(
        _seq,
        _params.get_value_as_string("aimid"), _params.get_value_as_string("m_chat_name"));
}

void core::im_container::on_mrim_get_key(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mrim_key(
        _seq,
        _params.get_value_as_string("email"));
}

void core::im_container::on_login_by_password(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));

    std::shared_ptr<base_im> im = std::make_shared<wim::im>(im_login_id(""), voip_manager_);
    ims_.clear();
    im->login(_seq, info);
    ims_.push_back(im);
}

void core::im_container::on_login_by_password_for_attach_uin(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));

    auto from_im = get_im(_params);
    if (from_im)
        from_im->start_attach_uin(_seq, info, from_im->make_wim_params());
}

void core::im_container::on_login_get_sms_code(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));

    std::shared_ptr<base_im> im;

    auto is_login = _params.get_value_as_bool("is_login");
    if (is_login)
    {
        ims_.clear();
        im = std::make_shared<wim::im>(im_login_id(""), voip_manager_);
    }
    else
    {
        im = get_im(_params);
    }

    im->login_normalize_phone(_seq, _params.get_value_as_string("country"), _params.get_value_as_string("phone"), _params.get_value_as_string("locale"), is_login);

    if (is_login)
        ims_.push_back(im);
}

void core::im_container::on_login_by_phone(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));
    info.set_sms_code(_params.get_value_as_string("sms_code"));

    if (ims_.empty())
    {
        assert(!"ims empty");
        return;
    }

    bool is_login = _params.get_value_as_bool("is_login");
    if (is_login)
        (*ims_.begin())->login_by_phone(_seq, info);
    else
        (*ims_.begin())->start_attach_phone(_seq, info);
}

void core::im_container::on_snap_read(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const auto contact_aimid = _params.get<std::string>("contact_aimid");
    assert(!contact_aimid.empty());

    const auto snap_id = _params.get<uint64_t>("snap_id");
    assert(snap_id > 0);

    const auto mark_prev_snaps_read = _params.get<bool>("mark_prev_snaps_read");
    const auto refresh_storage = _params.get<bool>("refresh_storage");

    im->read_snap(snap_id, contact_aimid, mark_prev_snaps_read, refresh_storage);
}

void core::im_container::on_delete_read(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const auto snap_id = _params.get<uint64_t>("snap_id");
    assert(snap_id > 0);

    im->delete_snap(snap_id);
}

void core::im_container::on_snap_download_metainfo(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const auto ttl_id = _params.get<std::string>("ttl_id");
    assert(!ttl_id.empty());

    const auto contact_aimid = _params.get<std::string>("contact");
    bool raise_priority = _params.get<bool>("raise", false);

    im->download_snap_metainfo(_seq, contact_aimid, ttl_id, raise_priority);
}

void core::im_container::on_connect_after_migration(int64_t _seq, coll_helper& _params)
{
    create();
}

void core::im_container::on_logout(int64_t _seq, coll_helper& _params)
{
    assert(!ims_.empty());
    if (ims_.empty())
        return;

    std::weak_ptr<core::im_container> wr_this(shared_from_this());

    auto __onlogout = [wr_this] ()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->start_session_stats(false /* delayed */);

        (*ptr_this->ims_.begin())->erase_auth_data();

        ptr_this->ims_.clear();
    };

    g_core->post_message_to_gui("need_login", 0, nullptr);

#ifndef STRIP_VOIP
    if (voip_manager_->get_call_manager()->call_get_count())
#else
    if (0)
#endif
    {
        auto __doLogout = [wr_this, __onlogout] ()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }
            (*ptr_this->ims_.begin())->logout(__onlogout);
        };
#ifndef STRIP_VOIP
        voip_manager_->get_call_manager()->call_stop_smart(__doLogout);
#endif //STRIP_VOIP
    }
    else
    {
        (*ims_.begin())->logout(__onlogout);
    }
}

void core::im_container::logout(const bool _is_auth_error)
{
    assert(!ims_.empty());
    if (ims_.empty())
        return;

    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_bool("is_auth_error", _is_auth_error);

    g_core->post_message_to_gui("need_login", 0, coll.get());

    ims_.clear();
}

void core::im_container::on_sign_url(int64_t _seq, coll_helper& _params)
{
    (*ims_.begin())->sign_url(_seq, _params.get_value_as_string("url"));
}

void core::im_container::on_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    core::iarray* prop_array = _params.get_value_as_array("props");
    for (auto i = 0; prop_array && i < prop_array->size(); ++i)
    {
        core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
        auto prop_name = value.get_value_as_string("name");
        auto prop_value = value.get_value_as_string("value");
        props.push_back(std::make_pair(prop_name, prop_value));
    }

    g_core->insert_event((core::stats::stats_event_names)_params.get_value_as_int("event"), props);
}

void core::im_container::on_url_played(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_played(_params.get_value_as_string("url"), _params.get_value_as_bool("played"));
}

void core::im_container::on_speech_to_text(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->speech_to_text(_seq, _params.get_value_as_string("url"), _params.get_value_as_string("locale"));
}

std::shared_ptr<base_im> core::im_container::get_im(coll_helper& _params) const
{
    // temporary, for many im
    return get_im_by_id(0);
}

std::shared_ptr<base_im> core::im_container::get_im_by_id(int32_t _id) const
{
    if (ims_.empty())
    {
        return std::shared_ptr<base_im>();
    }

    return (*ims_.begin());
}

void core::im_container::on_get_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    bool force = false;
    if (_params.is_value_exist("force"))
        force = _params.get_value_as_bool("force");

    im->get_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"), force);
}

void core::im_container::on_show_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->show_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"));
}

void core::im_container::on_delete_archive_messages(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const auto contact_aimid = _params.get<std::string>("contact_aimid");
    assert(!contact_aimid.empty());

    const auto messages_ids = _params.get_value_as_array("messages_ids");
    assert(messages_ids);

    const auto for_all = _params.get<bool>("for_all");

    const auto ids_number = messages_ids->size();
    assert(ids_number > 0);

    std::vector<int64_t> ids;
    ids.reserve(ids_number);

    for (auto index = 0; index < ids_number; ++index)
    {
        const auto id = messages_ids->get_at(index)->get_as_int64();
        assert(id > 0);

        ids.push_back(id);
    }

    im->delete_archive_messages(_seq, contact_aimid, ids, for_all);
}

void core::im_container::on_delete_archive_messages_from(int64_t _seq, coll_helper& _params)
{
    assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const auto contact_aimid = _params.get<std::string>("contact_aimid");
    assert(!contact_aimid.empty());

    const auto from_id = _params.get<int64_t>("from_id");
    assert(from_id >= 0);

    im->delete_archive_messages_from(_seq, contact_aimid, from_id);
}

void core::im_container::on_add_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_remove_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_set_first_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_first_message(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void core::im_container::on_history_search(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::vector<std::string>> searchSymbolsPatterns;
    std::string pattern;
    if (_params.is_value_exist("symbols_array"))
    {
        core::iarray* searchSymbolsArray = _params.get_value_as_array("symbols_array");
        for (auto i = 0; i < searchSymbolsArray->size(); ++i)
        {
            std::vector<std::string> patternForSymbol;
            core::coll_helper symbol_value(searchSymbolsArray->get_at(i)->get_as_collection(), false);

            auto symbols_patterns = symbol_value.get_value_as_array("symbols_patterns");
            for (auto i = 0; i < symbols_patterns->size(); ++i)
            {
                core::coll_helper value(symbols_patterns->get_at(i)->get_as_collection(), false);
                patternForSymbol.push_back(value.get_value_as_string("symbol_pattern"));
            }
            searchSymbolsPatterns.push_back(patternForSymbol);
        }
    }
    else
    {
        pattern = _params.get_value_as_string("init_pattern");
    }

    for (auto symbol_iter = searchSymbolsPatterns.begin(); symbol_iter != searchSymbolsPatterns.end(); ++symbol_iter)
    {
        for (auto iter = symbol_iter->begin(); iter != symbol_iter->end(); ++iter)
            *iter = ::core::tools::system::to_upper(*iter);
    }

    im->history_search_in_cl(searchSymbolsPatterns, _seq, _params.get_value_as_uint("fixed_patterns_count"), pattern);

    if (!_params.get_value_as_bool("search_in_history"))
        return;

    if (!searchSymbolsPatterns.empty() || !pattern.empty()) // && searchSymbolsPatterns.size() > 2)
    {
        im->setup_search_params(_seq);

        std::string word = _params.get_value_as_string("init_pattern");
        /*for (auto i  = 0u; i < searchSymbolsPatterns.size(); ++i)
        {
            word.append(::core::tools::system::to_lower(searchSymbolsPatterns[i][0]));
        }*/

        std::vector<std::string> aimids;
        if (_params.is_value_exist("aimid"))
        {
            aimids.push_back(_params.get_value_as_string("aimid"));
        }

        im->history_search_in_history(word, aimids);
    }
}

void core::im_container::on_history_search_ended(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clear_search_params();
}

void core::im_container::on_set_last_read(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_hide_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_chat(_params.get_value_as_string("contact"));
}

void im_container::on_mute_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mute_chat(_params.get_value_as_string("contact"), _params.get_value_as_bool("mute"));
}

void im_container::on_upload_file_sharing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::shared_ptr<core::tools::binary_stream> data = std::make_shared<core::tools::binary_stream>();
    std::string file_name, extension;
    if (_params.is_value_exist("file_stream"))
    {
        auto stream = _params.get_value_as_stream("file_stream");
        auto size = stream->size();
        data->write((char*)(stream->read(size)), size);
    }

    if (_params->is_value_exist("file"))
        file_name = _params.get_value_as_string("file");

    if (_params->is_value_exist("ext"))
        extension = _params.get_value_as_string("ext");

    im->upload_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        file_name,
        data,
        extension);
}

void im_container::on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->abort_file_sharing_upload(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("process_seq"));
}

void im_container::on_get_file_sharing_preview_size(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_file_sharing_preview_size(
        _seq,
        _params.get<std::string>("url"));
}

void im_container::on_download_file_sharing_metainfo(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing_metainfo(
        _seq,
        _params.get<std::string>("url"));
}

void im_container::on_download_file(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing(
        _seq,
        _params.get<std::string>("contact"),
        _params.get<std::string>("url"),
        _params.get<bool>("force_request_metainfo"),
        _params.get<std::string>("filename"),
        _params.get<std::string>("download_dir"),
        _params.get<bool>("raise", false));
}

void im_container::on_download_image(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_image(
        _seq,
        _params.get<std::string>("contact"),
        _params.get<std::string>("uri"),
        _params.get<std::string>("destination", ""),
        _params.get<bool>("is_preview"),
        _params.get<int32_t>("preview_width", 0),
        _params.get<int32_t>("preview_height", 0),
        _params.get<bool>("raise", false));
}

void im_container::on_cancel_image_downloading(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get<std::string>("url");

    im->cancel_loader_task(url);
}

void im_container::on_download_link_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_link_preview(
        _seq,
        _params.get<std::string>("contact"),
        _params.get<std::string>("uri"),
        _params.get<int32_t>("preview_width", 0),
        _params.get<int32_t>("preview_height", 0));
}

void im_container::on_abort_file_downloading(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get_value_as_string("url");

    im->abort_file_sharing_download(url);
}

void im_container::on_download_raise_priority(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto proc_id = _params.get<int64_t>("proc_id");
    assert(proc_id > 0);

    im->raise_download_priority(proc_id);
}

void im_container::on_contact_switched(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contact_aimid = _params.get<std::string>("contact");

    im->contact_switched(contact_aimid);
}

void im_container::on_get_stickers_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_meta(_seq, _params.get_value_as_string("size"));
}

void im_container::on_get_themes_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_themes_meta(_seq, (ThemesScale)_params.get_value_as_int("themes_scale"));
}

void im_container::on_get_theme(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme(_seq, _params.get_value_as_int("theme_id"));
}

void im_container::on_get_sticker(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_sticker(_seq,
        _params.get<int32_t>("set_id"),
        _params.get<int32_t>("sticker_id"),
        _params.get<core::sticker_size>("size"));
}

void im_container::on_get_chat_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string aimid, stamp;

    if (_params->is_value_exist("stamp"))
    {
        stamp = _params.get<std::string>("stamp");
    }
    else if (_params->is_value_exist("aimid"))
    {
        aimid = _params.get<std::string>("aimid");
    }
    else
    {
        assert(false);
    }

    im->get_chat_info(_seq, aimid, stamp, _params.get<int32_t>("limit"));
}

void im_container::on_get_chat_blocked(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_chat_blocked(_seq, _params.get<std::string>("aimid"));
}

void im_container::on_get_chat_pending(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_chat_pending(_seq, _params.get<std::string>("aimid"));
}

void im_container::on_get_chat_home(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;


    std::string tag;
    if (_params.is_value_exist("tag"))
        tag = _params.get<std::string>("tag");
    im->get_chat_home(_seq, tag);
}

void im_container::on_resolve_pending(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;


    std::vector<std::string> contacts;
    if (_params.is_value_exist("contacts"))
    {
        auto contacts_array = _params.get_value_as_array("contacts");
        for (auto i = 0; contacts_array && i < contacts_array->size(); ++i)
            contacts.push_back(contacts_array->get_at(i)->get_as_string());
    }

    im->resolve_pending(_seq, _params.get_value_as_string("aimid"), contacts, _params.get_value_as_bool("approve"));
}

void im_container::on_search_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    im->search_contacts(_seq, _params.get_value_as_string("keyword"), _params.get_value_as_string("phonenumber"), _params.get_value_as_string("tag"));
}

void im_container::on_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_profile(_seq, _params.get_value_as_string("aimid"));
}

void im_container::on_hide_dlg_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_dlg_state(_params.get_value_as_string("aimid"));
}

void im_container::on_add_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("group"),
        _params.get_value_as_string("message"));
}

void im_container::on_remove_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_rename_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->rename_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("friendly")
        );
}


void im_container::on_spam_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->spam_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_ignore_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->ignore_contact(
        _seq,
        _params.get_value_as_string("contact"), _params.get_value_as_bool("ignore"));
}

void im_container::on_get_ignore_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ignore_list(_seq);
}

void im_container::on_favorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->favorite(_params.get_value_as_string("contact"));
}

void im_container::on_unfavorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->unfavorite(_params.get_value_as_string("contact"));
}

themes::theme* im_container::get_theme(coll_helper& _params)
{
    auto im = get_im(_params);
    auto theme_id = _params.get_value_as_int("id");
    themes::theme *the_theme = im->get_theme_from_cache(theme_id);
    return the_theme;
}

void im_container::on_get_flags(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->load_flags(_seq);
}

void core::im_container::on_update_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::pair<std::string, std::string>> fields;

    auto field_array = _params.get_value_as_array("fields");
    for (auto i = 0; field_array && i < field_array->size(); ++i)
    {
        core::coll_helper value(field_array->get_at(i)->get_as_collection(), false);
        auto field_name = value.get_value_as_string("field_name");
        auto field_value = value.get_value_as_string("field_value");
        fields.push_back(std::make_pair(field_name, field_value));
    }

    im->update_profile(_seq, fields);
}

void core::im_container::on_join_livechat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string stamp = _params.get_value_as_string("stamp");
    int age = _params.is_value_exist("age") ? _params.get_value_as_int("age") : -1;

    im->join_live_chat(_seq, stamp, age);
}

void core::im_container::on_set_locale(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string locale = _params.get_value_as_string("locale");

    g_core->set_locale(locale);
}

void core::im_container::on_set_user_proxy(int64_t _seq, coll_helper& _params)
{
    proxy_settings user_proxy;

    user_proxy.proxy_type_ = (int32_t)_params.get_value_as_enum<proxy_types>("settings_proxy_type");

    assert(user_proxy.proxy_type_ >= 0 || user_proxy.proxy_type_ == -1);

    user_proxy.use_proxy_ = user_proxy.proxy_type_ != -1;
    user_proxy.proxy_server_ = tools::from_utf8(_params.get_value_as_string("settings_proxy_server"));
    user_proxy.proxy_port_ = _params.get_value_as_int("settings_proxy_port");
    user_proxy.login_ =  tools::from_utf8(_params.get_value_as_string("settings_proxy_username"));
    user_proxy.password_ =  tools::from_utf8(_params.get_value_as_string("settings_proxy_password"));
    user_proxy.need_auth_ = _params.get_value_as_bool("settings_proxy_need_auth");

    g_core->set_user_proxy_settings(user_proxy);
}

void core::im_container::on_set_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    tools::binary_stream bs_data;

    core::istream* stream = _params.get_value_as_stream("avatar");
    if (!stream)
    {
        assert(!"_params.get_value_as_stream(\"avatar\")");
        return;
    }

    uint32_t size = stream->size();
    if (stream && size)
    {
        bs_data.write((const char*) stream->read(size), size);
        stream->reset();
    }

    im->set_avatar(_seq, bs_data, _params.is_value_exist("aimid") ? _params.get_value_as_string("aimid") : std::string(), _params.is_value_exist("chat") ? _params.get_value_as_bool("chat") : false);
}

void core::im_container::on_create_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    
    auto params = core::wim::chat_params::create(_params);
    std::vector<std::string> members;
    {
        auto member_array = _params.get_value_as_array("members");
        for (auto i = 0; member_array && i < member_array->size(); ++i)
            members.push_back(member_array->get_at(i)->get_as_string());
    }
    im->create_chat(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"), members, params);
}

void core::im_container::on_mod_chat_params(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    
    auto params = core::wim::chat_params::create(_params);
    im->mod_chat_params(_seq, _params.get_value_as_string("aimid"), params);
}

void core::im_container::on_mod_chat_name(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_name(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"));
}

void core::im_container::on_mod_chat_about(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_about(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("about"));
}

void core::im_container::on_mod_chat_public(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_public(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("public"));
}

void core::im_container::on_mod_chat_join(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_join(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("approved"));
}

void core::im_container::on_mod_chat_link(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    
    im->mod_chat_link(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("link"));
}

void core::im_container::on_mod_chat_ro(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    
    im->mod_chat_ro(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("ro"));
}

void core::im_container::on_mod_chat_age(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;
    
    im->mod_chat_age(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_bool("age"));
}

void core::im_container::on_block_chat_member(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->block_chat_member(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_bool("block"));
}

void core::im_container::on_set_chat_member_role(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_chat_member_role(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_string("role"));
}

void core::im_container::on_close_promo(int64_t _seq, coll_helper& _params)
{
    g_core->set_need_show_promo(false);
    on_connect_after_migration(_seq, _params);
}

void im_container::on_get_mask_id_list(int64_t _seq, coll_helper& _params)
{
#ifndef STRIP_VOIP
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_id_list(_seq);
#endif //STRIP_VOIP
}

void im_container::on_get_mask_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_preview(_seq, _params.get_value_as_string("mask_id"));
}

void im_container::on_get_mask_model(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask_model(_seq);
}

void im_container::on_get_mask(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mask(_seq, _params.get_value_as_string("mask_id"));
}

void im_container::on_snaps_refresh(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->refresh_snaps_storage();
}

void im_container::on_refresh_user_snaps(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->refresh_user_snaps(_params.get_value_as_string("aimid"));
}

void im_container::on_remove_from_snaps_storage(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_from_snaps_storage(_params.get_value_as_string("aimid"));
}

bool im_container::has_valid_login() const
{
    auto im = get_im_by_id(0);
    if (!im)
        return false;

    return im->has_valid_login();
}
