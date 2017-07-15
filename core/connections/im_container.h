#ifndef __IM_CONTAINER_H__
#define __IM_CONTAINER_H__

#pragma once
#include <memory>

namespace voip_manager {
    struct VoipProxySettings;
    class VoipManager;
}

namespace core
{
    struct proxy_settings;
    class base_im;
    class login_info;
    class phone_info;
    class coll_helper;
    class im_login_list;
    class im_login_id;
    typedef std::vector<std::shared_ptr<base_im>> ims_list;
    namespace themes
    {
        class theme;
    }

    typedef std::function<void(int64_t, coll_helper&)> message_function;

    #define REGISTER_IM_MESSAGE(_message_string, _callback)                                             \
        messages_map_.emplace(                                                                          \
            _message_string,                                                                            \
            std::bind(&im_container::_callback, this, std::placeholders::_1, std::placeholders::_2));

    class im_container : public std::enable_shared_from_this<im_container>
    {
        std::unordered_map<std::string, message_function> messages_map_;

        std::unique_ptr<im_login_list> logins_;
        ims_list ims_;

        std::shared_ptr<voip_manager::VoipManager> voip_manager_;

        bool create_ims();
        void connect_ims();

        // gui message handlers
        void on_login_by_password(int64_t _seq, coll_helper& _params);
        void on_login_by_password_for_attach_uin(int64_t _seq, coll_helper& _params);

        void on_login_get_sms_code(int64_t _seq, coll_helper& _params);
        void on_login_by_phone(int64_t _seq, coll_helper& _params);
        void on_logout(int64_t _seq, coll_helper& _params);
        void on_phoneinfo(int64_t _seq, coll_helper& _params);
        void on_snap_read(int64_t _seq, coll_helper& _params);
        void on_delete_read(int64_t _seq, coll_helper& _params);
        void on_snap_download_metainfo(int64_t _seq, coll_helper& _params);
        void on_connect_after_migration(int64_t _seq, coll_helper& _params);
        void on_get_contact_avatar(int64_t _seq, coll_helper& _params);
        void on_show_contact_avatar(int64_t _seq, coll_helper& _params);
        void on_send_message(int64_t _seq, coll_helper& _params);
        void on_message_typing(int64_t _seq, coll_helper& _params);
        void on_feedback(int64_t _seq, coll_helper& _params);
        void on_set_state(int64_t _seq, coll_helper& _params);

        void on_history_search(int64_t _seq, coll_helper& _params);
        void on_history_search_ended(int64_t _seq, coll_helper& _params);

        void on_get_archive_images(int64_t _seq, coll_helper& _params);
        void on_repair_archive_images(int64_t _seq, coll_helper& _params);
        void on_get_archive_index(int64_t _seq, coll_helper& _params);
        void on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params);
        void on_get_archive_messages(int64_t _seq, coll_helper& _params);
        void on_delete_archive_messages(int64_t _seq, coll_helper& _params);
        void on_delete_archive_messages_from(int64_t _seq, coll_helper& _params);
        void on_add_opened_dialog(int64_t _seq, coll_helper& _params);
        void on_remove_opened_dialog(int64_t _seq, coll_helper& _params);
        void on_set_first_message(int64_t _seq, coll_helper& _params);
        void on_set_last_read(int64_t _seq, coll_helper& _params);
        void on_hide_chat(int64_t _seq, coll_helper& _params);
        void on_mute_chat(int64_t _seq, coll_helper& _params);
        void on_upload_file_sharing(int64_t _seq, coll_helper& _params);
        void on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params);

        void on_get_file_sharing_preview_size(int64_t _seq, coll_helper& _params);
        void on_download_file_sharing_metainfo(int64_t _seq, coll_helper& _params);
        void on_download_file(int64_t _seq, coll_helper& _params);

        void on_download_image(int64_t _seq, coll_helper& _params);
        void on_cancel_image_downloading(int64_t _seq, coll_helper& _params);
        void on_download_link_preview(int64_t _seq, coll_helper& _params);
        void on_download_raise_priority(int64_t _seq, coll_helper& _params);
        void on_contact_switched(int64_t _seq, coll_helper& _params);
        void on_abort_file_downloading(int64_t _seq, coll_helper& _params);
        void on_get_stickers_meta(int64_t _seq, coll_helper& _params);
        void on_get_themes_meta(int64_t _seq, coll_helper& _params);
        void on_get_theme(int64_t _seq, coll_helper& _params);
        void on_get_sticker(int64_t _seq, coll_helper& _params);
        void on_get_chat_info(int64_t _seq, coll_helper& _params);
        void on_get_chat_blocked(int64_t _seq, coll_helper& _params);
        void on_get_chat_pending(int64_t _seq, coll_helper& _params);
        void on_get_chat_home(int64_t _seq, coll_helper& _params);
        void on_resolve_pending(int64_t _seq, coll_helper& _params);
        void on_search_contacts(int64_t _seq, coll_helper& _params);
        void on_profile(int64_t _seq, coll_helper& _params);
        void on_hide_dlg_state(int64_t _seq, coll_helper& _params);
        void on_add_contact(int64_t _seq, coll_helper& _params);
        void on_remove_contact(int64_t _seq, coll_helper& _params);
        void on_rename_contact(int64_t _seq, coll_helper& _params);
        void on_spam_contact(int64_t _seq, coll_helper& _params);
        void on_url_played(int64_t _seq, coll_helper& _params);
        void on_speech_to_text(int64_t _seq, coll_helper& _params);
        void on_ignore_contact(int64_t _seq, coll_helper& _params);
        void on_get_ignore_contacts(int64_t _seq, coll_helper& _params);
        void on_favorite(int64_t _seq, coll_helper& _params);
        void on_unfavorite(int64_t _seq, coll_helper& _params);

        void on_create_chat(int64_t _seq, coll_helper& _params);
        
        void on_mod_chat_params(int64_t _seq, coll_helper& _params);
        void on_mod_chat_name(int64_t _seq, coll_helper& _params);
        void on_mod_chat_about(int64_t _seq, coll_helper& _params);
        void on_mod_chat_public(int64_t _seq, coll_helper& _params);
        void on_mod_chat_join(int64_t _seq, coll_helper& _params);
        void on_mod_chat_link(int64_t _seq, coll_helper& _params);
        void on_mod_chat_ro(int64_t _seq, coll_helper& _params);
        void on_mod_chat_age(int64_t _seq, coll_helper& _params);
        
        void on_block_chat_member(int64_t _seq, coll_helper& _params);
        void on_set_chat_member_role(int64_t _seq, coll_helper& _params);

        void on_set_user_proxy(int64_t _seq, coll_helper& _params);
        void on_join_livechat(int64_t _seq, coll_helper& _params);
        void on_set_locale(int64_t _seq, coll_helper& _params);
        void on_set_avatar(int64_t _seq, coll_helper& _params);

        void on_voip_call_message(int64_t _seq, coll_helper& _params);
        void on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params);
        void on_voip_background_msg(std::shared_ptr<base_im> im, coll_helper& _params);

        // group chat
        void on_remove_members(int64_t _seq, coll_helper& _params);
        void on_add_members(int64_t _seq, coll_helper& _params);
        void on_add_chat(int64_t _seq, coll_helper& _params);
        void on_modify_chat(int64_t _seq, coll_helper& _params);

        //mrim
        void on_mrim_get_key(int64_t _seq, coll_helper& _params);

        // tools
        void on_sign_url(int64_t _seq, coll_helper& _params);
        void on_stats(int64_t _seq, coll_helper& _params);

        //snaps
        void on_snaps_refresh(int64_t _seq, coll_helper& _params);
        void on_refresh_user_snaps(int64_t _seq, coll_helper& _params);
        void on_remove_from_snaps_storage(int64_t _seq, coll_helper& _params);

        std::shared_ptr<base_im> get_im(coll_helper& _params) const;
        void on_get_flags(int64_t _seq, coll_helper& _params);

        void on_update_profile(int64_t _seq, coll_helper& _params);

        void fromInternalProxySettings2Voip(const core::proxy_settings& proxySettings, voip_manager::VoipProxySettings& voipProxySettings);

        void on_close_promo(int64_t _seq, coll_helper& _params);

        // masks
        void on_get_mask_id_list(int64_t _seq, coll_helper& _params);
        void on_get_mask_preview(int64_t _seq, coll_helper& _params);
        void on_get_mask_model(int64_t _seq, coll_helper& _params);
        void on_get_mask(int64_t _seq, coll_helper& _params);

    public:

        void on_message_from_gui(const char * _message, int64_t _seq, coll_helper& _params);
        std::shared_ptr<base_im> get_im_by_id(int32_t _id) const;
        bool update_login(im_login_id& _login);
        void replace_uin_in_login(im_login_id& old_login, im_login_id& new_login);
        void logout(const bool _is_auth_error);

        bool has_valid_login() const;

        im_container(std::shared_ptr<voip_manager::VoipManager> voip_manager);
        virtual ~im_container();

        void create();
        std::string get_first_login() const;
        themes::theme* get_theme(coll_helper& _params);
    };
}

#endif //__IM_CONTAINER_H__
