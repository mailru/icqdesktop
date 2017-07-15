#pragma once

namespace core
{

    enum class profile_state
    {
        min = 0,

        online,
        dnd,
        invisible,
        away,
        offline,

        max,
    };

    // don't change texts in this func - it uses for sending to server
    inline std::ostream& operator<<(std::ostream &oss, const profile_state arg)
    {
        assert(arg > profile_state::min);
        assert(arg < profile_state::max);

        switch(arg)
        {
            case profile_state::online:    oss << "online"; break;
            case profile_state::dnd:       oss << "dnd"; break;
            case profile_state::invisible: oss << "invisible"; break;
            case profile_state::away:      oss << "away"; break;

        default:
            assert(!"unknown core::profile_state value");
            break;
        }

        return oss;
    }

    enum class message_type
    {
        min,


        undefined,
        base,
        file_sharing,
        sms,
        sticker,
        chat_event,
        voip_event,

        max
    };

    enum class file_sharing_function
    {
        min,

        download_file,
        download_file_metainfo,
        download_preview_metainfo,
        check_local_copy_exists,

        max
    };

    enum class file_sharing_content_type
    {
        min,

        undefined,
        image,
        gif,
        video,
        snap_image,
        snap_gif,
        snap_video,
        ptt,

        max
    };

    enum class typing_status
    {
        min,
        
        none,
        typing,
        typed,
        
        max,
    };
    
    inline bool is_snap_file_sharing_content_type(const file_sharing_content_type _type)
    {
        assert(_type > file_sharing_content_type::min);
        assert(_type < file_sharing_content_type::max);

        return ((_type == file_sharing_content_type::snap_gif) ||
                (_type == file_sharing_content_type::snap_image) ||
                (_type == file_sharing_content_type::snap_video));
    }

    enum class sticker_size
    {
        min,

        small,
        medium,
        large,

        max
    };

    enum class chat_event_type
    {
        // the codes are stored in a db thus do not reorder the items below

        invalid,

        min,

        added_to_buddy_list,

        mchat_add_members,
        mchat_invite,
        mchat_leave,
        mchat_del_members,
        mchat_kicked,

        chat_name_modified,

        buddy_reg,
        buddy_found,

        birthday,

        avatar_modified,

        generic,

        chat_description_modified,

        message_deleted,

        chat_rules_modified,

        max
    };

    enum class voip_event_type
    {
        invalid,

        min,

        missed_call,
        call_ended,
        accept,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const message_type arg)
    {
        switch(arg)
        {
        case message_type::base:
            oss << "base";
            break;

        case message_type::file_sharing:
            oss << "file_sharing";
            break;

        case message_type::sticker:
            oss << "sticker";
            break;

        case message_type::sms:
            oss << "sms";
            break;

        default:
            assert(!"unknown core::message_type value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_function arg)
    {
        assert(arg > file_sharing_function::min);
        assert(arg < file_sharing_function::max);

        switch(arg)
        {
        case file_sharing_function::check_local_copy_exists:
            oss << "check_local_copy_exists";
            break;

        case file_sharing_function::download_file:
            oss << "download_file";
            break;

        case file_sharing_function::download_file_metainfo:
            oss << "download_file_metainfo";
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << "download_preview_metainfo";
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << "small";
            break;

        case sticker_size::medium:
            oss << "medium";
            break;

        case sticker_size::large:
            oss << "large";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    inline std::wostream& operator<<(std::wostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << L"small";
            break;

        case sticker_size::medium:
            oss << L"medium";
            break;

        case sticker_size::large:
            oss << L"large";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    enum class group_chat_info_errors
    {
        min = 1,
        network_error = 2,
        not_in_chat = 3,
        blocked = 4,
        max,
    };

    namespace stats
    {
        // NOTE: don't change old numbers, add new one if necessary
        enum class stats_event_names
        {
            min = 0,

            // absolete part : TODO : delete it

            absolete_service_session_start = 1,
            absolete_start_session = 2,

            absolete_reg_page_phone = 3,
            absolete_reg_login_phone = 4,
            absolete_reg_page_uin = 5,
            absolete_reg_login_uin = 6,
            absolete_reg_edit_country = 7,
            absolete_reg_error_phone = 8,
            absolete_reg_sms_send = 9,
            absolete_reg_sms_resend = 10,
            absolete_reg_error_code = 11,
            absolete_reg_edit_phone = 12,
            absolete_reg_error_uin = 13,
            absolete_reg_error_other = 14,
            absolete_login_forgot_password = 224,

            absolete_main_window_fullscreen = 15,
            absolete_main_window_resize = 16,

            absolete_groupchat_from_create_button = 17,
            absolete_groupchat_from_sidebar = 18,
            absolete_groupchat_created = 19,
            absolete_groupchat_create_members_count = 21,
            absolete_groupchat_members_count = 22,
            absolete_groupchat_leave = 23,
            absolete_groupchat_rename = 25,
            absolete_groupchat_avatar_changed = 182,
            absolete_groupchat_add_member_sidebar = 183,
            absolete_groupchat_add_member_dialog = 184,

            absolete_filesharing_sent_count = 26,
            absolete_filesharing_sent = 27,
            absolete_filesharing_sent_success = 28,
            absolete_filesharing_count = 29,
            absolete_filesharing_filesize = 30,
            absolete_filesharing_dnd_recents = 31,
            absolete_filesharing_dnd_dialog = 32,
            absolete_filesharing_dialog = 33,
            absolete_filesharing_cancel = 34,
            absolete_filesharing_download_file = 37,
            absolete_filesharing_download_cancel = 38,
            absolete_filesharing_download_success = 39,

            absolete_smile_sent_picker = 40,
            absolete_smile_sent_from_recents = 41,
            absolete_sticker_sent_from_picker = 42,
            absolete_sticker_sent_from_recents = 43,
            absolete_picker_cathegory_click = 45,
            absolete_picker_tab_click = 46,

            absolete_alert_click = 48,
            absolete_alert_viewall = 49,
            absolete_alert_close = 50,

            absolete_spam_cl_menu = 51,
            absolete_spam_auth_widget = 52,
            absolete_spam_sidebar = 53,
            
            absolete_recents_close = 55,
            absolete_recents_read = 56,
            absolete_recents_readall = 57,
            absolete_mute_recents_menu = 58,
            absolete_mute_sidebar = 59,
            absolete_unmute = 60,

            absolete_ignore_recents_menu = 61,
            absolete_ignore_cl_menu = 62,
            absolete_ignore_auth_widget = 63,
            absolete_ignore_sidebar = 64,
            absolete_ignorelist_open = 66,
            absolete_ignorelist_remove = 67,

            absolete_cl_empty_write_msg = 68,
            absolete_cl_empty_android = 69,
            absolete_cl_empty_ios = 70,
            absolete_cl_empty_find_friends = 71,
            absolete_cl_search = 72,
            absolete_cl_load = 73,
            absolete_cl_search_openmessage = 208,
            absolete_cl_search_dialog = 209,
            absolete_cl_search_dialog_openmessage = 210,
            absolete_cl_search_nohistory = 211,

            absolete_myprofile_edit = 74,
            absolete_myprofile_open = 75,
            absolete_myprofile_online = 76,
            absolete_myprofile_invisible = 77,
            absolete_myprofile_dnd = 78,
            absolete_myprofile_away = 161,

            absolete_profile_cl = 79,
            absolete_profile_auth_widget = 81,
            absolete_profile_avatar = 82,
            absolete_profile_search_results = 83,
            absolete_profile_members_list = 84,
            absolete_profile_call = 85,
            absolete_profile_video_call = 86,
            absolete_profile_sidebar = 191,

            absolete_add_user_profile_page = 87,
            absolete_add_user_auth_widget = 89,
            absolete_add_user_sidebar = 164,
            absolete_delete_auth_widget = 91,
            absolete_delete_sidebar = 92,
            absolete_delete_cl_menu = 93,
            absolete_delete_profile_page = 163,
            absolete_rename_contact = 154,

            absolete_search_open_page = 94,
            absolete_search_no_results = 99,
            absolete_add_user_search_results = 100,
            absolete_search = 101,

            absolete_message_send_button = 102,
            absolete_message_enter_button = 103,

            absolete_open_chat_recents = 104,
            absolete_open_chat_search_recents = 105,
            absolete_open_chat_cl = 106,

            absolete_message_pending = 113,
            absolete_message_delete_my = 155,
            absolete_message_delete_all = 156,

            absolete_history_preload = 118,
            absolete_history_delete = 157,

            absolete_open_popup_livechat = 158,

            absolete_gui_load = 119,

            absolete_feedback_show = 120,
            absolete_feedback_sent = 121,
            absolete_feedback_error = 122,

            absolete_call_from_chat = 123,
            absolete_videocall_from_chat = 124,
            absolete_call_from_cl_menu = 127,
            absolete_call_from_search_results = 130,

            absolete_voip_callback = 131,
            absolete_voip_incoming_call = 132,
            absolete_voip_accept = 133,
            absolete_voip_accept_video = 134,
            absolete_voip_declined = 135,
            absolete_voip_started = 136,
            absolete_voip_finished = 137,

            absolete_voip_chat = 138,
            absolete_voip_camera_off = 139,
            absolete_voip_camera_on = 140,
            absolete_voip_microphone_off = 141,
            absolete_voip_microphone_on = 142,
            absolete_voip_dynamic_off = 143,
            absolete_voip_dynamic_on = 144,
            absolete_voip_sound_off = 145,
            absolete_voip_settings = 146,
            absolete_voip_fullscreen = 147,
            absolete_voip_camera_change = 148,
            absolete_voip_microphone_change = 149,
            absolete_voip_dynamic_change = 150,
            absolete_voip_aspectratio_change = 151,

            absolete_settings_about_show = 152,
            absolete_client_settings = 153,

            absolete_proxy_open = 159,
            absolete_proxy_set = 160,

            absolete_strange_event = 162,

            absolete_promo_skip  = 170,
            absolete_promo_next  = 171,
            absolete_promo_switch  = 172,

            absolete_favorites_set = 173,
            absolete_favorites_unset = 174,
            absolete_favorites_load = 175,

            absolete_livechats_page_open = 176,
            absolete_livechat_profile_open = 177,
            absolete_livechat_join_fromprofile = 178,
            absolete_livechat_join_frompopup = 179,
            absolete_livechat_admins = 180,
            absolete_livechat_blocked = 181,

            absolete_profile_avatar_changed = 186,
            absolete_introduce_name_set = 187,
            absolete_introduce_avatar_changed = 188,
            absolete_introduce_avatar_fail = 189,

            absolete_masks_open = 192,
            absolete_masks_select = 193,
            
            absolete_chats_create_open = 201,
            absolete_chats_create_public = 202,
            absolete_chats_create_approval = 203,
            absolete_chats_create_readonly = 204,
            absolete_chats_create_rename = 205,
            absolete_chats_create_avatar = 206,
            absolete_chats_created = 207,

            absolete_quotes_send_alone = 212,
            absolete_quotes_send_answer = 213,
            absolete_quotes_messagescount = 214,

            absolete_forward_send_frommenu = 215,
            absolete_forward_send_frombutton = 216,
            absolete_forward_send_preview = 217,
            absolete_forward_messagescount = 218,

            // end of absolete part

            service_session_start = 1001,
            start_session = 1002,
            start_session_params_loaded = 1003,

            reg_page_phone = 2001,
            reg_login_phone = 2002,
            reg_page_uin = 2003,
            reg_login_uin = 2004,
            reg_edit_country = 2005,
            reg_error_phone = 2006,
            reg_sms_send = 2007,
            reg_sms_resend = 2008,
            reg_error_code = 2009,
            reg_edit_phone = 2010,
            reg_error_uin = 2011,
            reg_error_other = 2012,
            login_forgot_password = 2013,

            main_window_fullscreen = 3001,
            main_window_resize = 3002,

            groupchat_from_create_button = 4001,
            groupchat_from_sidebar = 4002,
            groupchat_created = 4003,
            groupchat_create_members_count = 4005,
            groupchat_members_count = 4006,
            groupchat_leave = 4007,
            groupchat_rename = 4009,
            groupchat_avatar_changed = 4010,
            groupchat_add_member_sidebar = 4011,
            groupchat_add_member_dialog = 4012,

            filesharing_sent_count = 5001,
            filesharing_sent = 5002,
            filesharing_sent_success = 5003,
            filesharing_count = 5004,
            filesharing_filesize = 5005,
            filesharing_dnd_recents = 5006,
            filesharing_dnd_dialog = 5007,
            filesharing_dialog = 5008,
            filesharing_cancel = 5009,
            filesharing_download_file = 5010,
            filesharing_download_cancel = 5011,
            filesharing_download_success = 5012,

            smile_sent_picker = 6001,
            smile_sent_from_recents = 6002,
            sticker_sent_from_picker = 6003,
            sticker_sent_from_recents = 6004,
            picker_cathegory_click = 6005,
            picker_tab_click = 6006,

            alert_click = 7001,
            alert_viewall = 7002,
            alert_close = 7003,
            alert_mail_common = 7004,
            alert_mail_letter = 7005,
            tray_mail = 7006,

            spam_cl_menu = 8001,
            spam_auth_widget = 8002,
            spam_sidebar = 8003,
            spam_members_list = 8004,
            spam_chat_avatar = 8005,

            recents_close = 9001,
            recents_read = 9002,
            recents_readall = 9003,
            mute_recents_menu = 9004,
            mute_sidebar = 9005,
            unmute = 9006,

            ignore_recents_menu = 10001,
            ignore_cl_menu = 10002,
            ignore_auth_widget = 10003,
            ignore_sidebar = 10004,
            ignorelist_open = 10006,
            ignorelist_remove = 10007,

            cl_empty_write_msg = 11001,
            cl_empty_android = 11002,
            cl_empty_ios = 11003,
            cl_empty_find_friends = 11004,
            cl_search = 11005,
            cl_load = 11006,
            cl_search_openmessage = 11007,
            cl_search_dialog = 11008,
            cl_search_dialog_openmessage = 11009,
            cl_search_nohistory = 11010,

            myprofile_edit = 12001,
            myprofile_open = 12002,
            myprofile_online = 12003,
            myprofile_invisible = 12004,
            myprofile_dnd = 12005,
            myprofile_away = 12006,

            profile_cl = 13001,
            profile_auth_widget = 13002,
            profile_avatar = 13003,
            profile_search_results = 13004,
            profile_members_list = 13005,
            profile_call = 13006,
            profile_video_call = 13007,
            profile_sidebar = 13008,
            profile_write_message = 13009,

            add_user_profile_page = 14001,
            add_user_auth_widget = 14002,
            add_user_sidebar = 14004,
            delete_auth_widget = 14005,
            delete_sidebar = 14006,
            delete_cl_menu = 14007,
            delete_profile_page = 14008,
            rename_contact = 14009,

            search_open_page = 15001,
            search_no_results = 15002,
            add_user_search_results = 15003,
            search = 15004,

            message_send_button = 16001,
            message_enter_button = 16002,

            open_chat_recents = 17001,
            open_chat_search_recents = 17002,
            open_chat_cl = 17003,

            message_pending = 18001,
            message_delete_my = 18002,
            message_delete_all = 18003,

            history_preload = 19002,
            history_delete = 19003,

            open_popup_livechat = 20001,

            gui_load = 21001,

            feedback_show = 22001,
            feedback_sent = 22002,
            feedback_error = 22003,

            call_from_chat = 23001,
            videocall_from_chat = 23002,
            call_from_cl_menu = 23005,
            call_from_search_results = 23008,
            voip_callback = 23009,
            voip_incoming_call = 23010,
            voip_accept = 23011,
            voip_accept_video = 23012,
            voip_declined = 23013,
            voip_started = 23014,
            voip_finished = 23015,

            voip_chat = 24001,
            voip_camera_off = 24002,
            voip_camera_on = 24003,
            voip_microphone_off = 24004,
            voip_microphone_on = 24005,
            voip_dynamic_off = 24006,
            voip_dynamic_on = 24007,
            voip_sound_off = 24008,
            voip_settings = 24009,
            voip_fullscreen = 24010,
            voip_camera_change = 24011,
            voip_microphone_change = 24012,
            voip_dynamic_change = 24013,
            voip_aspectratio_change = 24014,

            settings_about_show = 25001,
            client_settings = 25002,

            proxy_open = 26001,
            proxy_set = 26002,

            strange_event = 27001,

            promo_skip  = 28001,
            promo_next  = 28002,
            promo_switch  = 28003,

            favorites_set = 29001,
            favorites_unset = 29002,
            favorites_load = 29003,

            livechats_page_open = 30001,
            livechat_profile_open = 30002,
            livechat_join_fromprofile = 30003,
            livechat_join_frompopup = 30004,
            livechat_admins = 30005,
            livechat_blocked = 30006,

            profile_avatar_changed = 31001,
            introduce_name_set = 31002,
            introduce_avatar_changed = 31003,
            introduce_avatar_fail = 31004,

            masks_open = 32001,
            masks_select = 32002,
            
            chats_create_open = 33001,
            chats_create_public = 33002,
            chats_create_approval = 33003,
            chats_create_readonly = 33004,
            chats_create_rename = 33005,
            chats_create_avatar = 33006,
            chats_created = 33007,

            quotes_send_alone = 34001,
            quotes_send_answer = 34002,
            quotes_messagescount = 34003,

            forward_send_frommenu = 35001,
            forward_send_frombutton = 35002,
            forward_send_preview = 35003,
            forward_messagescount = 35004,

            merge_accounts = 35100,

            unknowns_add_user = 36000,
            unknowns_close = 36001,
            unknowns_closeall = 36002,

			chat_down_button = 36003,

            max = 36004,
        };

        inline std::ostream& operator<<(std::ostream &oss, const stats_event_names arg)
        {
            assert(arg > stats_event_names::min);
            assert(arg < stats_event_names::max);

            switch(arg)
            {
            case stats_event_names::start_session :
            case stats_event_names::absolete_start_session : oss << "Start_Session [session enable]"; break;

            case stats_event_names::start_session_params_loaded : oss << "Start_Session_Params_Loaded"; break;

            case stats_event_names::service_session_start :
            case stats_event_names::absolete_service_session_start : assert(false); break;

            // registration
            case stats_event_names::reg_page_phone :
            case stats_event_names::absolete_reg_page_phone : oss << "Reg_Page_Phone"; break;
            case stats_event_names::reg_login_phone :
            case stats_event_names::absolete_reg_login_phone : oss << "Reg_Login_Phone"; break;
            case stats_event_names::reg_page_uin :
            case stats_event_names::absolete_reg_page_uin : oss << "Reg_Page_Uin"; break;
            case stats_event_names::reg_login_uin :
            case stats_event_names::absolete_reg_login_uin : oss << "Reg_Login_UIN"; break;
            case stats_event_names::reg_edit_country :
            case stats_event_names::absolete_reg_edit_country : oss << "Reg_Edit_Country"; break;
            case stats_event_names::reg_error_phone :
            case stats_event_names::absolete_reg_error_phone : oss << "Reg_Error_Phone"; break;
            case stats_event_names::reg_sms_send :
            case stats_event_names::absolete_reg_sms_send : oss << "Reg_Sms_Send"; break;
            case stats_event_names::reg_sms_resend :
            case stats_event_names::absolete_reg_sms_resend : oss << "Reg_Sms_Resend"; break;
            case stats_event_names::reg_error_code :
            case stats_event_names::absolete_reg_error_code : oss << "Reg_Error_Code"; break;
            case stats_event_names::reg_edit_phone :
            case stats_event_names::absolete_reg_edit_phone : oss << "Reg_Edit_Phone"; break;
            case stats_event_names::reg_error_uin :
            case stats_event_names::absolete_reg_error_uin : oss << "Reg_Error_UIN"; break;
            case stats_event_names::reg_error_other :
            case stats_event_names::absolete_reg_error_other : oss << "Reg_Error_Other"; break;
            case stats_event_names::login_forgot_password:
            case stats_event_names::absolete_login_forgot_password: oss << "Login_Forgot_Password"; break;

            // main window
            case stats_event_names::main_window_fullscreen :
            case stats_event_names::absolete_main_window_fullscreen : oss << "Mainwindow_Fullscreen"; break;
            case stats_event_names::main_window_resize :
            case stats_event_names::absolete_main_window_resize : oss << "Mainwindow_Resize"; break;

            // group chat
            case stats_event_names::groupchat_from_create_button :
            case stats_event_names::absolete_groupchat_from_create_button : oss << "Groupchat_FromCreateButton"; break;
            case stats_event_names::groupchat_from_sidebar :
            case stats_event_names::absolete_groupchat_from_sidebar : oss << "Groupchat_FromSidebar"; break;
            case stats_event_names::groupchat_created :
            case stats_event_names::absolete_groupchat_created : oss << "Groupchat_Created"; break;
            case stats_event_names::groupchat_members_count :
            case stats_event_names::absolete_groupchat_members_count : oss << "Groupchat_MembersCount"; break;
            case stats_event_names::groupchat_leave :
            case stats_event_names::absolete_groupchat_leave : oss << "Groupchat_Leave"; break;
            case stats_event_names::groupchat_rename :
            case stats_event_names::absolete_groupchat_rename : oss << "Groupchat_Rename"; break;
            case stats_event_names::groupchat_avatar_changed :
            case stats_event_names::absolete_groupchat_avatar_changed : oss << "Groupchat_Avatar_Changed"; break;
            case stats_event_names::groupchat_add_member_sidebar :
            case stats_event_names::absolete_groupchat_add_member_sidebar : oss << "Groupchat_Add_member_Sidebar"; break;
            case stats_event_names::groupchat_add_member_dialog :
            case stats_event_names::absolete_groupchat_add_member_dialog : oss << "Groupchat_Add_member_Dialog"; break;

            // filesharing
            case stats_event_names::filesharing_sent :
            case stats_event_names::absolete_filesharing_sent : oss << "Filesharing_Sent"; break;
            case stats_event_names::filesharing_sent_count :
            case stats_event_names::absolete_filesharing_sent_count : oss << "Filesharing_Sent_Count"; break;
            case stats_event_names::filesharing_sent_success :
            case stats_event_names::absolete_filesharing_sent_success : oss << "Filesharing_Sent_Success"; break;
            case stats_event_names::filesharing_count :
            case stats_event_names::absolete_filesharing_count : oss << "Filesharing_Count"; break;
            case stats_event_names::filesharing_filesize :
            case stats_event_names::absolete_filesharing_filesize : oss << "Filesharing_Filesize"; break;
            case stats_event_names::filesharing_dnd_recents :
            case stats_event_names::absolete_filesharing_dnd_recents : oss << "Filesharing_DNDRecents"; break;
            case stats_event_names::filesharing_dnd_dialog :
            case stats_event_names::absolete_filesharing_dnd_dialog : oss << "Filesharing_DNDDialog"; break;
            case stats_event_names::filesharing_dialog :
            case stats_event_names::absolete_filesharing_dialog : oss << "Filesharing_Dialog"; break;
            case stats_event_names::filesharing_cancel :
            case stats_event_names::absolete_filesharing_cancel : oss << "Filesharing_Cancel"; break;
            case stats_event_names::filesharing_download_file :
            case stats_event_names::absolete_filesharing_download_file : oss << "Filesharing_Download_File"; break;
            case stats_event_names::filesharing_download_cancel :
            case stats_event_names::absolete_filesharing_download_cancel : oss << "Filesharing_Download_Cancel"; break;
            case stats_event_names::filesharing_download_success :
            case stats_event_names::absolete_filesharing_download_success : oss << "Filesharing_Download_Success"; break;

            // smiles and stickers
            case stats_event_names::smile_sent_picker :
            case stats_event_names::absolete_smile_sent_picker : oss << "Smile_Sent_Picker"; break;
            case stats_event_names::smile_sent_from_recents :
            case stats_event_names::absolete_smile_sent_from_recents : oss << "Smile_Sent_From_Recents"; break;
            case stats_event_names::sticker_sent_from_picker :
            case stats_event_names::absolete_sticker_sent_from_picker : oss << "Sticker_Sent_From_Picker"; break;
            case stats_event_names::sticker_sent_from_recents :
            case stats_event_names::absolete_sticker_sent_from_recents : oss << "Sticker_Sent_From_Recents"; break;
            case stats_event_names::picker_cathegory_click :
            case stats_event_names::absolete_picker_cathegory_click : oss << "Picker_Cathegory_Click"; break;
            case stats_event_names::picker_tab_click :
            case stats_event_names::absolete_picker_tab_click : oss << "Picker_Tab_Click"; break;

            // alerts
            case stats_event_names::alert_click :
            case stats_event_names::absolete_alert_click : oss << "Alert_Click"; break;
            case stats_event_names::alert_viewall :
            case stats_event_names::absolete_alert_viewall : oss << "Alert_ViewAll"; break;
            case stats_event_names::alert_close :
            case stats_event_names::absolete_alert_close : oss << "Alert_Close"; break;
            case stats_event_names::alert_mail_common:  oss << "Alert_Mail_Common"; break;
            case stats_event_names::alert_mail_letter: oss << "Alert_Mail_Letter"; break;
            case stats_event_names::tray_mail: oss << "Tray_Mail"; break;
            // spam
            case stats_event_names::spam_cl_menu :
            case stats_event_names::absolete_spam_cl_menu : oss << "Spam_CL_Menu"; break;
            case stats_event_names::spam_auth_widget :
            case stats_event_names::absolete_spam_auth_widget : oss << "Spam_Auth_Widget"; break;
            case stats_event_names::spam_sidebar :
            case stats_event_names::absolete_spam_sidebar : oss << "Spam_Sidebar"; break;
            case stats_event_names::spam_members_list: oss << "Spam_Members_List"; break;
            case stats_event_names::spam_chat_avatar: oss << "Spam_Chat_Avatar"; break;

            // cl
            case stats_event_names::recents_close :
            case stats_event_names::absolete_recents_close: oss << "Recents_Close"; break;
            case stats_event_names::recents_read :
            case stats_event_names::absolete_recents_read : oss << "Recents_Read"; break;
            case stats_event_names::recents_readall :
            case stats_event_names::absolete_recents_readall : oss << "Recents_Readall"; break;

            case stats_event_names::mute_recents_menu :
            case stats_event_names::absolete_mute_recents_menu : oss << "Mute_Recents_Menu"; break;
            case stats_event_names::mute_sidebar :
            case stats_event_names::absolete_mute_sidebar: oss << "Mute_Sidebar"; break;
            case stats_event_names::unmute :
            case stats_event_names::absolete_unmute : oss << "Unmute"; break;

            case stats_event_names::ignore_recents_menu :
            case stats_event_names::absolete_ignore_recents_menu : oss << "Ignore_Recents_Menu"; break;
            case stats_event_names::ignore_cl_menu :
            case stats_event_names::absolete_ignore_cl_menu : oss << "Ignore_CL_Menu"; break;
            case stats_event_names::ignore_auth_widget :
            case stats_event_names::absolete_ignore_auth_widget : oss << "Ignore_Auth_Widget"; break;
            case stats_event_names::ignore_sidebar :
            case stats_event_names::absolete_ignore_sidebar: oss << "Ignore_Sidebar"; break;
            case stats_event_names::ignorelist_open :
            case stats_event_names::absolete_ignorelist_open : oss << "Ignorelist_Open"; break;
            case stats_event_names::ignorelist_remove :
            case stats_event_names::absolete_ignorelist_remove : oss << "Ignorelist_Remove"; break;


            case stats_event_names::cl_empty_write_msg :
            case stats_event_names::absolete_cl_empty_write_msg : oss << "CL_Empty_Write_Msg"; break;
            case stats_event_names::cl_empty_android :
            case stats_event_names::absolete_cl_empty_android : oss << "CL_Empty_Android"; break;
            case stats_event_names::cl_empty_ios :
            case stats_event_names::absolete_cl_empty_ios : oss << "CL_Empty_IOS"; break;
            case stats_event_names::cl_empty_find_friends :
            case stats_event_names::absolete_cl_empty_find_friends : oss << "CL_Empty_Find_Friends"; break;

            case stats_event_names::rename_contact :
            case stats_event_names::absolete_rename_contact : oss << "rename_contact"; break;

            case stats_event_names::cl_search :
            case stats_event_names::absolete_cl_search : oss << "CL_Search"; break;
            case stats_event_names::cl_load :
            case stats_event_names::absolete_cl_load : oss << "CL_Load"; break;
            case stats_event_names::cl_search_openmessage :
            case stats_event_names::absolete_cl_search_openmessage: oss << "CL_Search_OpenMessage"; break;
            case stats_event_names::cl_search_dialog :
            case stats_event_names::absolete_cl_search_dialog: oss << "CL_Search_Dialog"; break;
            case stats_event_names::cl_search_dialog_openmessage:
            case stats_event_names::absolete_cl_search_dialog_openmessage: oss << "CL_Search_Dialog_OpenMessage"; break;
            case stats_event_names::cl_search_nohistory :
            case stats_event_names::absolete_cl_search_nohistory: oss << "CL_Search_NoHistory"; break;

            // profile
            case stats_event_names::myprofile_open :
            case stats_event_names::absolete_myprofile_open : oss << "Myprofile_Open"; break;
            case stats_event_names::myprofile_online :
            case stats_event_names::absolete_myprofile_online: oss << "Myprofile_Online"; break;
            case stats_event_names::myprofile_invisible :
            case stats_event_names::absolete_myprofile_invisible : oss << "Myprofile_Invisible"; break;
            case stats_event_names::myprofile_dnd :
            case stats_event_names::absolete_myprofile_dnd : oss << "Myprofile_DND"; break;
            case stats_event_names::myprofile_away :
            case stats_event_names::absolete_myprofile_away : oss << "Myprofile_Away"; break;

            case stats_event_names::profile_cl :
            case stats_event_names::absolete_profile_cl : oss << "Profile_CL"; break;
            case stats_event_names::profile_auth_widget :
            case stats_event_names::absolete_profile_auth_widget : oss << "Profile_Auth_Widget"; break;
            case stats_event_names::profile_avatar :
            case stats_event_names::absolete_profile_avatar : oss << "Profile_Avatar"; break;
            case stats_event_names::profile_search_results :
            case stats_event_names::absolete_profile_search_results : oss << "Profile_Search_Results"; break;
            case stats_event_names::profile_members_list :
            case stats_event_names::absolete_profile_members_list : oss << "Profile_Members_List"; break;
            case stats_event_names::profile_call :
            case stats_event_names::absolete_profile_call : oss << "Profile_Call"; break;
            case stats_event_names::profile_video_call :
            case stats_event_names::absolete_profile_video_call : oss << "Profile_Video_Call"; break;
            case stats_event_names::profile_sidebar :
            case stats_event_names::absolete_profile_sidebar : oss << "Profile_Sidebar"; break;
            case stats_event_names::profile_write_message: oss << "Profile_Write_Message"; break;

            case stats_event_names::add_user_profile_page :
            case stats_event_names::absolete_add_user_profile_page : oss << "Add_User_Profile_Page"; break;
            case stats_event_names::myprofile_edit :
            case stats_event_names::absolete_myprofile_edit : oss << "Myprofile_Edit"; break;


            // search, adding and auth
            case stats_event_names::add_user_auth_widget :
            case stats_event_names::absolete_add_user_auth_widget : oss << "Add_User_Auth_Widget"; break;
            case stats_event_names::add_user_sidebar :
            case stats_event_names::absolete_add_user_sidebar: oss << "Add_User_Sidebar"; break;
            case stats_event_names::unknowns_add_user: oss << "Unknowns_Add_User"; break;


            case stats_event_names::delete_auth_widget :
            case stats_event_names::absolete_delete_auth_widget : oss << "Delete_Auth_Widget"; break;
            case stats_event_names::delete_sidebar :
            case stats_event_names::absolete_delete_sidebar: oss << "Delete_Sidebar"; break;
            case stats_event_names::delete_cl_menu :
            case stats_event_names::absolete_delete_cl_menu : oss << "Delete_CL_Menu"; break;
            case stats_event_names::delete_profile_page :
            case stats_event_names::absolete_delete_profile_page: oss << "Delete_Profile_Page"; break;
            case stats_event_names::unknowns_close: oss << "Unknowns_Close"; break;
            case stats_event_names::unknowns_closeall: oss << "Unknowns_CloseAll"; break;

            case stats_event_names::search_open_page :
            case stats_event_names::absolete_search_open_page : oss << "Search_Open_Page"; break;
            case stats_event_names::search_no_results :
            case stats_event_names::absolete_search_no_results : oss << "Search_No_Results"; break;
            case stats_event_names::add_user_search_results :
            case stats_event_names::absolete_add_user_search_results : oss << "Add_User_Search_Results"; break;
            case stats_event_names::search :
            case stats_event_names::absolete_search : oss << "Search"; break;


            // messaging
            case stats_event_names::message_send_button :
            case stats_event_names::absolete_message_send_button : oss << "Message_Send_Button"; break;
            case stats_event_names::message_enter_button :
            case stats_event_names::absolete_message_enter_button : oss << "Message_Enter_Button"; break;
            case stats_event_names::open_chat_recents :
            case stats_event_names::absolete_open_chat_recents : oss << "Open_Chat_Recents"; break;
            case stats_event_names::open_chat_search_recents :
            case stats_event_names::absolete_open_chat_search_recents : oss << "Open_Chat_Search_Recents"; break;
            case stats_event_names::open_chat_cl :
            case stats_event_names::absolete_open_chat_cl : oss << "Open_Chat_CL"; break;

            case stats_event_names::message_pending :
            case stats_event_names::absolete_message_pending : oss << "Message_Pending"; break;
            case stats_event_names::message_delete_my :
            case stats_event_names::absolete_message_delete_my : oss << "message_delete_my"; break;
            case stats_event_names::message_delete_all :
            case stats_event_names::absolete_message_delete_all : oss << "message_delete_all"; break;
            case stats_event_names::open_popup_livechat :
            case stats_event_names::absolete_open_popup_livechat : oss << "open_popup_livechat"; break;

            case stats_event_names::history_preload :
            case stats_event_names::absolete_history_preload : oss << "History_Preload"; break;
            case stats_event_names::history_delete :
            case stats_event_names::absolete_history_delete : oss << "history_delete"; break;

            case stats_event_names::feedback_show :
            case stats_event_names::absolete_feedback_show : oss << "Feedback_Show"; break;
            case stats_event_names::feedback_sent :
            case stats_event_names::absolete_feedback_sent : oss << "Feedback_Sent"; break;
            case stats_event_names::feedback_error :
            case stats_event_names::absolete_feedback_error : oss << "Feedback_Error"; break;

            case stats_event_names::call_from_chat :
            case stats_event_names::absolete_call_from_chat: oss << "Call_From_Chat"; break;
            case stats_event_names::videocall_from_chat:
            case stats_event_names::absolete_videocall_from_chat: oss << "Videocall_From_Chat"; break;
            case stats_event_names::call_from_cl_menu :
            case stats_event_names::absolete_call_from_cl_menu : oss << "Call_From_CL_Meu"; break;
            case stats_event_names::call_from_search_results :
            case stats_event_names::absolete_call_from_search_results : oss << "Call_From_Search_Results"; break;
            case stats_event_names::voip_callback :
            case stats_event_names::absolete_voip_callback : oss << "Voip_Callback"; break;
            case stats_event_names::voip_incoming_call :
            case stats_event_names::absolete_voip_incoming_call : oss << "Voip_Incoming_Call"; break;
            case stats_event_names::voip_accept :
            case stats_event_names::absolete_voip_accept : oss << "Voip_Accept"; break;
            case stats_event_names::voip_accept_video :
            case stats_event_names::absolete_voip_accept_video : oss << "Voip_Accept_Video"; break;
            case stats_event_names::voip_declined :
            case stats_event_names::absolete_voip_declined : oss << "Voip_Declined"; break;
            case stats_event_names::voip_started :
            case stats_event_names::absolete_voip_started : oss << "Voip_Started"; break;
            case stats_event_names::voip_finished :
            case stats_event_names::absolete_voip_finished : oss << "Voip_Finished"; break;

            case stats_event_names::voip_chat :
            case stats_event_names::absolete_voip_chat : oss << "Voip_Chat"; break;
            case stats_event_names::voip_camera_off :
            case stats_event_names::absolete_voip_camera_off : oss << "Voip_Camera_Off"; break;
            case stats_event_names::voip_camera_on :
            case stats_event_names::absolete_voip_camera_on : oss << "Voip_Camera_On"; break;
            case stats_event_names::voip_microphone_off :
            case stats_event_names::absolete_voip_microphone_off : oss << "Voip_Microphone_Off"; break;
            case stats_event_names::voip_microphone_on :
            case stats_event_names::absolete_voip_microphone_on : oss << "Voip_Microphone_On"; break;
            case stats_event_names::voip_dynamic_off :
            case stats_event_names::absolete_voip_dynamic_off : oss << "Voip_Dynamic_Off"; break;
            case stats_event_names::voip_dynamic_on :
            case stats_event_names::absolete_voip_dynamic_on : oss << "Voip_Dynamic_On"; break;
            case stats_event_names::voip_sound_off :
            case stats_event_names::absolete_voip_sound_off : oss << "Voip_Sound_Off"; break;
            case stats_event_names::voip_settings :
            case stats_event_names::absolete_voip_settings : oss << "Voip_Settings"; break;
            case stats_event_names::voip_fullscreen :
            case stats_event_names::absolete_voip_fullscreen : oss << "Voip_Fullscreen"; break;
            case stats_event_names::voip_camera_change :
            case stats_event_names::absolete_voip_camera_change : oss << "Voip_Camera_Change"; break;
            case stats_event_names::voip_microphone_change :
            case stats_event_names::absolete_voip_microphone_change : oss << "Voip_Microphone_Change"; break;
            case stats_event_names::voip_dynamic_change :
            case stats_event_names::absolete_voip_dynamic_change : oss << "Voip_Dynamic_Change"; break;
            case stats_event_names::voip_aspectratio_change :
            case stats_event_names::absolete_voip_aspectratio_change : oss << "Voip_AspectRatio_Change"; break;

            case stats_event_names::gui_load :
            case stats_event_names::absolete_gui_load : oss << "Gui_Load"; break;

            case stats_event_names::settings_about_show :
            case stats_event_names::absolete_settings_about_show : oss << "Settings_About_Show"; break;
            case stats_event_names::client_settings :
            case stats_event_names::absolete_client_settings : oss << "Client_Settings"; break;

            case stats_event_names::proxy_open :
            case stats_event_names::absolete_proxy_open : oss << "proxy_open"; break;
            case stats_event_names::proxy_set :
            case stats_event_names::absolete_proxy_set : oss << "proxy_set"; break;

            case stats_event_names::promo_skip :
            case stats_event_names::absolete_promo_skip : oss << "Promo_Skip"; break;
            case stats_event_names::promo_next :
            case stats_event_names::absolete_promo_next : oss << "Promo_Next"; break;
            case stats_event_names::promo_switch :
            case stats_event_names::absolete_promo_switch : oss << "Promo_Switch"; break;

            case stats_event_names::favorites_set :
            case stats_event_names::absolete_favorites_set : oss << "favorites_set"; break;
            case stats_event_names::favorites_unset :
            case stats_event_names::absolete_favorites_unset : oss << "favorites_unset"; break;
            case stats_event_names::favorites_load :
            case stats_event_names::absolete_favorites_load : oss << "favorites_load"; break;

            case stats_event_names::livechats_page_open :
            case stats_event_names::absolete_livechats_page_open : oss << "Livechats_Page_Open"; break;
            case stats_event_names::livechat_profile_open :
            case stats_event_names::absolete_livechat_profile_open : oss << "Livechat_Profile_Open"; break;
            case stats_event_names::livechat_join_fromprofile :
            case stats_event_names::absolete_livechat_join_fromprofile : oss << "Livechat_Join_FromProfile"; break;
            case stats_event_names::livechat_join_frompopup :
            case stats_event_names::absolete_livechat_join_frompopup : oss << "Livechat_Join_FromPopup"; break;
            case stats_event_names::livechat_admins :
            case stats_event_names::absolete_livechat_admins : oss << "Livechat_Admins"; break;
            case stats_event_names::livechat_blocked :
            case stats_event_names::absolete_livechat_blocked : oss << "Livechat_Blocked"; break;


            case stats_event_names::profile_avatar_changed :
            case stats_event_names::absolete_profile_avatar_changed : oss << "Profile_Avatar_Changed"; break;
            case stats_event_names::introduce_name_set :
            case stats_event_names::absolete_introduce_name_set : oss << "Introduce_Name_Set"; break;
            case stats_event_names::introduce_avatar_changed :
            case stats_event_names::absolete_introduce_avatar_changed : oss << "Introduce_Avatar_Changed"; break;
            case stats_event_names::introduce_avatar_fail :
            case stats_event_names::absolete_introduce_avatar_fail : oss << "Introduce_Avatar_Fail"; break;

            // Masks 
            case stats_event_names::masks_open :
            case stats_event_names::absolete_masks_open: oss << "Masks_Open"; break;
            case stats_event_names::masks_select :
            case stats_event_names::absolete_masks_select: oss << "Masks_Select"; break;

            // Create chat
            case stats_event_names::chats_create_open :
            case stats_event_names::absolete_chats_create_open : oss << "Chats_Create_Open"; break;
            case stats_event_names::chats_create_public :
            case stats_event_names::absolete_chats_create_public : oss << "Chats_Create_Public"; break;
            case stats_event_names::chats_create_approval :
            case stats_event_names::absolete_chats_create_approval : oss << "Chats_Create_Approval"; break;
            case stats_event_names::chats_create_readonly :
            case stats_event_names::absolete_chats_create_readonly : oss << "Chats_Create_Readonly"; break;
            case stats_event_names::chats_create_rename :
            case stats_event_names::absolete_chats_create_rename : oss << "Chats_Create_Rename"; break;
            case stats_event_names::chats_create_avatar :
            case stats_event_names::absolete_chats_create_avatar : oss << "Chats_Create_Avatar"; break;
            case stats_event_names::chats_created :
            case stats_event_names::absolete_chats_created : oss << "Chats_Created"; break;

            case stats_event_names::quotes_send_alone :
            case stats_event_names::absolete_quotes_send_alone : oss << "Quotes_Send_Alone"; break;
            case stats_event_names::quotes_send_answer :
            case stats_event_names::absolete_quotes_send_answer : oss << "Quotes_Send_Answer"; break;
            case stats_event_names::quotes_messagescount :
            case stats_event_names::absolete_quotes_messagescount : oss << "Quotes_MessagesCount"; break;

            case stats_event_names::forward_send_frommenu :
            case stats_event_names::absolete_forward_send_frommenu : oss << "Forward_Send_FromMenu"; break;
            case stats_event_names::forward_send_frombutton :
            case stats_event_names::absolete_forward_send_frombutton : oss << "Forward_Send_FromButton"; break;
            case stats_event_names::forward_send_preview :
            case stats_event_names::absolete_forward_send_preview : oss << "Forward_Send_Preview"; break;
            case stats_event_names::forward_messagescount :
            case stats_event_names::absolete_forward_messagescount : oss << "Forward_MessagesCount"; break;

            case stats_event_names::merge_accounts: oss << "Merge_Accounts"; break;

			case stats_event_names::chat_down_button: oss << "Chat_DownButton"; break;

            default:
                assert(!"unknown core::stats_event_names ");
                oss << "Unknown event " << (int)arg; break;
            }

            return oss;
        }
    }

    inline stats::stats_event_names stateToStatisticsEvent(const profile_state _state)
    {
        assert(_state > profile_state::min);
        assert(_state < profile_state::max);

        switch (_state)
        {
            case profile_state::online:    return stats::stats_event_names::myprofile_online;
            case profile_state::dnd:       return stats::stats_event_names::myprofile_dnd;
            case profile_state::invisible: return stats::stats_event_names::myprofile_invisible;
            case profile_state::away:      return stats::stats_event_names::myprofile_away;

            default:
                assert(!"unknown core::proxy_types value");
                return stats::stats_event_names::strange_event;
            break;
        }
    }

    enum class proxy_types
    {
        min = -100,
        auto_proxy = -1,

        // positive values are reserved for curl_proxytype
        http_proxy = 0,
        socks4 = 4,
        socks5 = 5,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const proxy_types arg)
    {
        assert(arg > proxy_types::min);
        assert(arg < proxy_types::max);

        switch(arg)
        {
            case proxy_types::auto_proxy: oss << "Auto"; break;
            case proxy_types::http_proxy: oss << "HTTP Proxy"; break;
            case proxy_types::socks4: oss << "SOCKS4"; break;
            case proxy_types::socks5: oss << "SOCKS5"; break;

            default:
                assert(!"unknown core::proxy_types value");
                break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_content_type arg)
    {
        assert(arg > file_sharing_content_type::min);
        assert(arg < file_sharing_content_type::max);

        switch (arg)
        {
            case file_sharing_content_type::gif: return (oss << "gif");
            case file_sharing_content_type::image: return (oss << "image");
            case file_sharing_content_type::ptt: return (oss << "ptt");
            case file_sharing_content_type::snap_gif: return (oss << "snap_gif");
            case file_sharing_content_type::snap_image: return (oss << "snap_image");
            case file_sharing_content_type::snap_video: return (oss << "snap_video");
            case file_sharing_content_type::undefined: return (oss << "undefined");
            case file_sharing_content_type::video: return (oss << "video");
            default:
                assert(!"unexpected file sharing content type");
        }

        return oss;
    }

}
