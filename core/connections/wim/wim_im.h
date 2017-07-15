#pragma once

#include "../base_im.h"
#include "../../async_task.h"
#include "../../archive/dlg_state.h"
#include "../../archive/history_message.h"
#include "../../archive/opened_dialog.h"
#include "events/fetch_event_diff.h"


#ifdef _DEBUG
#define	ROBUSTO_THREAD_COUNT	1
#else
#define	ROBUSTO_THREAD_COUNT	3
#endif //_DEBUG

namespace voip_manager{
    struct VoipProtoMsg;
}

CORE_TOOLS_NS_BEGIN

struct url;

typedef std::vector<url> url_vector_t;

CORE_TOOLS_NS_END

namespace core
{
    class async_task;
    class login_info;
    class phone_info;

    enum class file_sharing_content_type;
    
    enum class typing_status;

    class masks;

    namespace statistic
    {
        class imstat;
    }

    namespace tools
    {
        class binary_stream;
    }

    namespace archive
    {
        class face;

        class not_sent_message;
        typedef std::shared_ptr<not_sent_message> not_sent_message_sptr;

        class history_message;
        typedef std::shared_ptr<history_message> history_message_sptr;

        typedef std::vector<std::shared_ptr<history_message>> history_block;
        typedef std::shared_ptr<history_block> history_block_sptr;

        typedef std::list<message_header> headers_list;
        typedef std::shared_ptr<headers_list> headers_list_sptr;

        typedef std::vector<std::pair<std::string, int64_t>> contact_and_msgs;
        typedef std::vector<std::pair<std::pair<std::string, std::shared_ptr<int64_t>>, std::shared_ptr<int64_t>>> contact_and_offsets;

        struct coded_term;
    }

    namespace themes
    {
        class theme;
    }


    namespace wim
    {
        namespace preview_proxy
        {
            class link_meta;
        }

        typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> timepoint;

        typedef std::unordered_set<std::string> ignorelist_cache;

        class fetch_event_buddy_list;
        class fetch_event_presence;
        class fetch_event_dlg_state;
        class fetch_event_hidden_chat;
        class fetch_event_diff;
        class fetch_event_my_info;
        class fetch_event_user_added_to_buddy_list;
        class fetch_event_typing;
        class fetch_event_permit;
        class fetch_event_imstate;
        class fetch_event_notification;
        class fetch_event_snaps;

        struct auth_parameters;
        struct fetch_parameters;
        struct wim_packet_params;
        struct robusto_packet_params;
        class contactlist;
        class avatar_loader;
        class wim_packet;
        class robusto_packet;
        struct get_history_params;
        struct set_dlg_state_params;
        class active_dialogs;
        class favorites;
        class my_info_cache;
        class loader;
        class async_loader;
        class send_message;
        class fetch;
        class chat_params;
        class mailbox_storage;
        class snaps_storage;

        namespace holes
        {
            class request;
            class failed_requests;
        }


        class send_message_handler
        {
        public:

            std::function<void(int32_t _error, const send_message& _packet)> on_result;
        };

        //////////////////////////////////////////////////////////////////////////
        // wim_send_thread
        //////////////////////////////////////////////////////////////////////////
        class wim_send_thread : public async_executer, public std::enable_shared_from_this<wim_send_thread>
        {
            struct task_and_params
            {
                std::shared_ptr<wim_packet> task_;
                std::function<void(int32_t)> error_handler_;
                std::shared_ptr<async_task_handlers> callback_handlers_;

                task_and_params(
                    std::shared_ptr<wim_packet> _task,
                    std::function<void(int32_t)> _error_handler,
                    std::shared_ptr<async_task_handlers> _callback_handlers)
                    :
                task_(_task),
                    error_handler_(_error_handler),
                    callback_handlers_(_callback_handlers) {}
            };

            bool is_packet_execute_;
            std::list<task_and_params> packets_queue_;

            std::chrono::system_clock::time_point cancel_packets_time_;

            void execute_packet_from_queue();

            std::shared_ptr<async_task_handlers> post_packet(
                std::shared_ptr<wim_packet> _packet,
                std::function<void(int32_t)> _error_handler,
                std::shared_ptr<async_task_handlers> _handlers);

        public:

            std::shared_ptr<async_task_handlers> post_packet(std::shared_ptr<wim_packet> _packet, std::function<void(int32_t)> _error_handler);
            void clear();

            wim_send_thread();
            virtual ~wim_send_thread();
        };



        //////////////////////////////////////////////////////////////////////////
        // robusto_thread
        //////////////////////////////////////////////////////////////////////////


        class robusto_thread : public async_executer, public std::enable_shared_from_this<robusto_thread>
        {
            struct task_and_params
            {
                std::shared_ptr<robusto_packet>			packet_;
                std::shared_ptr<async_task_handlers>	handlers_;

                task_and_params(
                    std::shared_ptr<robusto_packet> _packet,
                    std::shared_ptr<async_task_handlers> _handlers)
                    :
                packet_(_packet),
                    handlers_(_handlers)
                {
                }
            };

            bool	is_get_robusto_token_in_process_;

            std::list<std::shared_ptr<task_and_params>>	packets_queue_;

        public:

            robusto_thread() :
                async_executer(ROBUSTO_THREAD_COUNT),
                is_get_robusto_token_in_process_(false) {}
            virtual ~robusto_thread() {}

            bool is_get_robusto_token_in_process()					{ return is_get_robusto_token_in_process_; }
            void set_robusto_token_in_process(bool _is_in_process)	{ is_get_robusto_token_in_process_ = _is_in_process; }

            void push_packet_to_queue(
                std::shared_ptr<robusto_packet> _packet,
                std::shared_ptr<async_task_handlers> _handlers);

            std::shared_ptr<task_and_params> get_front_packet_from_queue();
        };


        //////////////////////////////////////////////////////////////////////////
        // fetch_thread
        //////////////////////////////////////////////////////////////////////////
        class fetch_thread : public async_executer
        {

        };



        //////////////////////////////////////////////////////////////////////////
        // search_data
        //////////////////////////////////////////////////////////////////////////

        struct search_data
        {
            search_data()
                : req_id(0)
                , count_of_free_threads(0)
            {
            }

            std::list<std::pair<std::pair<std::string, std::shared_ptr<int64_t>>, std::shared_ptr<int64_t>>> contact_and_offset;
            std::chrono::time_point<std::chrono::system_clock> start_time;
            std::chrono::time_point<std::chrono::system_clock> last_send_time;
            int64_t req_id;
            std::map<int64_t, int32_t, std::greater<int64_t>> top_messages_ids;
            int32_t count_of_free_threads;
            int32_t count_of_sent_msgs;
            std::vector<std::shared_ptr<::core::archive::searched_msg>> top_messages;
            int32_t count_of_yet_no_sent_msgs;
        };

        //////////////////////////////////////////////////////////////////////////
        // class im
        //////////////////////////////////////////////////////////////////////////
        class im : public base_im , public std::enable_shared_from_this<im>
        {
            friend class core::masks;

            // stop signal
            struct stop_objects
            {
                std::mutex stop_mutex_;
                uint64_t active_session_id_;
                std::condition_variable condition_poll_;

                stop_objects() : active_session_id_(0) {}
            };

            std::shared_ptr<stop_objects> stop_objects_;
            std::weak_ptr<stop_objects> stop_objects_weak_; 
            std::shared_ptr<wim::contactlist> contact_list_;
            std::shared_ptr<wim::active_dialogs> active_dialogs_;
            std::shared_ptr<wim::my_info_cache> my_info_cache_;
            std::shared_ptr<wim::favorites> favorites_;
            std::shared_ptr<wim::mailbox_storage> mailbox_storage_;
            std::shared_ptr<wim::snaps_storage> snaps_storage_;

            // authorization parameters
            std::shared_ptr<auth_parameters> auth_params_;
            std::shared_ptr<auth_parameters> attached_auth_params_;

            // wim fetch parameters
            std::shared_ptr<fetch_parameters> fetch_params_;

            // temporary for phone registration
            std::shared_ptr<phone_info> phone_registration_data_;
            std::shared_ptr<phone_info> attached_phone_registration_data_;

            // threads
            std::shared_ptr<wim_send_thread> wim_send_thread_;
            std::shared_ptr<fetch_thread> fetch_thread_;
            std::shared_ptr<async_executer> async_tasks_;
            std::shared_ptr<robusto_thread> robusto_threads_;

            // files loader/uploader
            std::shared_ptr<loader> files_loader_;
            std::shared_ptr<async_loader> async_loader_;

            // avatar loader
            std::shared_ptr<avatar_loader> avatar_loader_;

            // timers
            uint32_t store_timer_id_;
            uint32_t stat_timer_id_;
            uint32_t hosts_config_timer_id_;
            uint32_t dlg_state_timer_;

            bool im_created_;

            // archive
            std::shared_ptr<archive::face> archive_;
            std::shared_ptr<archive::face> get_archive();

            // opened dialog, posted from gui
            std::map<std::string, archive::opened_dialog> opened_dialogs_;

            std::shared_ptr<holes::failed_requests> failed_holes_requests_;

            bool sent_pending_messages_active_;

            // statistic
            std::unique_ptr<statistic::imstat> imstat_;

            // search
            std::shared_ptr<async_executer> history_searcher_;
            search_data search_data_;

            // need for start session
            timepoint start_session_time_;

            int64_t prefetch_uid_;

            // post messages timer
            int32_t post_messages_timer_;

            int32_t mute_chats_timer_;

            std::chrono::system_clock::time_point last_success_network_post_;
            std::chrono::system_clock::time_point last_check_alt_scheme_reset_;

            // dlg_state cache
            bool dlg_state_agregate_mode_;
            std::chrono::system_clock::time_point agregate_start_time_;
            std::chrono::system_clock::time_point last_network_activity_time_;
            std::list<core::icollection*> cached_dlg_states_;

            const robusto_packet_params make_robusto_params();

            std::shared_ptr<async_task_handlers> post_wim_packet(std::shared_ptr<wim_packet> _packet);
            std::shared_ptr<async_task_handlers> post_robusto_packet(std::shared_ptr<robusto_packet> packet);
            std::shared_ptr<async_task_handlers> post_robusto_packet_internal(std::shared_ptr<robusto_packet> _packet, std::shared_ptr<async_task_handlers> _handlers,
                uint32_t recursion_count, int32_t _last_error);
            void post_robusto_packet_to_server(std::shared_ptr<async_task_handlers> _handlers, std::shared_ptr<robusto_packet> _packet, uint32_t _recursion_count);

            void store_auth_parameters();
            void load_auth_and_fetch_parameters();
            void store_fetch_parameters();
            std::wstring get_auth_parameters_filename();
            std::wstring get_auth_parameters_filename_exported();
            std::wstring get_auth_parameters_filename_merge();
            std::wstring get_fetch_parameters_filename();
            std::wstring get_exported_muted_chats_filename();

            void save_my_info();
            void save_contact_list();
            void save_active_dialogs();
            void save_favorites();
            void save_mailboxes();
            void save_snaps_storage();

            std::shared_ptr<async_task_handlers> load_active_dialogs();
            std::shared_ptr<async_task_handlers> load_contact_list();
            std::shared_ptr<async_task_handlers> load_my_info();
            std::shared_ptr<async_task_handlers> load_favorites();
            std::shared_ptr<async_task_handlers> load_mailboxes();
            std::shared_ptr<async_task_handlers> load_snaps_storage();

            // stickers
            void load_stickers_data(int64_t _seq, const std::string _size);
            void download_stickers(int64_t _seq, const std::string _size);
            void download_stickers_metadata(int64_t _seq, const std::string _size);
            void download_stickers_metafile(int64_t _seq, const std::string& _size, const std::string& _md5);
            virtual void get_stickers_meta(int64_t _seq, const std::string& _size) override;
            virtual void get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, core::sticker_size _size) override;
            void post_stickers_meta_to_gui(int64_t _seq, const std::string& _size);

            virtual void get_chat_home(int64_t _seq, const std::string& _tag) override;
            virtual void get_chat_info(int64_t _seq, const std::string& _aimid, const std::string& _stamp, int32_t _limit) override;
            virtual void get_chat_blocked(int64_t _seq, const std::string& _aimid) override;
            virtual void get_chat_pending(int64_t _seq, const std::string& _aimid) override;
            virtual void resolve_pending(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _contact, bool _approve) override;

            virtual void create_chat(int64_t _seq, const std::string& _aimid, const std::string& _name, const std::vector<std::string>& _members, core::wim::chat_params *&_params) override;

            virtual void mod_chat_params(int64_t _seq, const std::string& _aimid, core::wim::chat_params *&_params) override;
            virtual void mod_chat_name(int64_t _seq, const std::string& _aimid, const std::string& _name) override;
            virtual void mod_chat_about(int64_t _seq, const std::string& _aimid, const std::string& _about) override;
            virtual void mod_chat_public(int64_t _seq, const std::string& _aimid, bool _public) override;
            virtual void mod_chat_join(int64_t _seq, const std::string& _aimid, bool _approved) override;
            virtual void mod_chat_link(int64_t _seq, const std::string& _aimid, bool _link) override;
            virtual void mod_chat_ro(int64_t _seq, const std::string& _aimid, bool _ro) override;
            virtual void mod_chat_age(int64_t _seq, const std::string& _aimid, bool _age) override;

            virtual void block_chat_member(int64_t _seq, const std::string& _aimid, const std::string& _contact, bool _block) override;
            virtual void set_chat_member_role(int64_t _seq, const std::string& _aimid, const std::string& _contact, const std::string& _role) override;

            virtual void get_themes_meta(int64_t _seq, const ThemesScale themes_value_) override;
            void get_theme(int64_t _seq, int32_t _theme_id) override;
            themes::theme* get_theme_from_cache(int32_t _theme_id) override;
            void load_themes_meta(int64_t _seq);
            void download_themes_meta(int64_t _seq);
            void download_themes(int64_t _seq);

            void load_cached_objects();
            void save_cached_objects();

            void post_my_info_to_gui();
            void post_contact_list_to_gui();
            void post_ignorelist_to_gui(int64_t _seq);
            void post_active_dialogs_to_gui();
            void post_active_dialogs_are_empty_to_gui();

            virtual std::string get_login() override;
            virtual void logout(std::function<void()> _on_result) override;

            void login_by_password(
                int64_t _seq, 
                const std::string& login, 
                const std::string& password, 
                const bool save_auth_data, 
                const bool start_session,
                const bool _from_export_login);

            void login_by_agent_token(
                const std::string& _login, 
                const std::string& _token, 
                const std::string& _guid);

            void login_by_password_and_attach_uin(
                int64_t _seq, 
                const std::string& login, 
                const std::string& password, 
                const wim_packet_params& _from_params);

            void start_session(bool _is_ping = false) override;

            void phoneinfo(int64_t seq, const std::string &phone, const std::string &gui_locale) override;

            void cancel_requests();
            bool is_session_valid(uint64_t _session_id);
            void poll(bool _is_first, bool _after_network_error, int32_t _failed_network_error_count = 0);

            void dispatch_events(std::shared_ptr<fetch> _fetch_packet, std::function<void(int32_t)> _on_complete = [](int32_t){});

            void schedule_store_timer();
            void stop_store_timer();

            loader& get_loader();
            async_loader& get_async_loader();

            // statistic
            void schedule_stat_timer();
            void stop_stat_timer();
            void send_statistic_if_needed();

            void schedule_hosts_config_timer();
            void stop_hosts_config_timer();

            void on_im_created();

            virtual std::wstring get_im_path() const override;

            std::shared_ptr<avatar_loader> get_avatar_loader();

            virtual void get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size, bool _force) override;
            virtual void show_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size) override;

            virtual void send_message_to_contact(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _message,
                const core::message_type _type,
                const std::string& _internal_id,
                const core::archive::quotes_vec& _quotes) override;
                
            virtual void send_message_typing(const int64_t _seq, const std::string& _contact, const core::typing_status& _status) override;

            virtual void send_feedback(const int64_t, const std::string &url, const std::map<std::string, std::string> &fields, const std::vector<std::string> &files) override;

            virtual void set_state(const int64_t _seq, const core::profile_state _state) override;

            void post_pending_messages(const bool _recurcive = false);

            std::shared_ptr<async_task_handlers> send_pending_message_async(
                const int64_t _seq,
                const archive::not_sent_message_sptr& _message);

            std::shared_ptr<send_message_handler> send_message_async(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _message,
                const std::string& _internal_id,
                const core::message_type _type,
                core::archive::quotes_vec _quotes);

            void post_not_sent_message_to_gui(int64_t _seq, const archive::not_sent_message_sptr& _message);

            virtual void connect() override;

            // login functions
            virtual void login(int64_t _seq, const login_info& _info) override;
            virtual void login_get_sms_code(int64_t seq, const phone_info& _info, bool _is_login) override;
            virtual void login_by_phone(int64_t _seq, const phone_info& _info) override;

            virtual void start_attach_phone(int64_t _seq, const phone_info& _info) override;
            virtual void start_attach_uin(int64_t _seq, const login_info& _info, const wim_packet_params& _from_params) override;

            virtual void erase_auth_data() override;

            virtual void sign_url(int64_t _seq, const std::string& unsigned_url) override;

            std::shared_ptr<async_task_handlers> get_robusto_token();

            // history functions
            void get_archive_images(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count) override;
            void repair_archive_images(int64_t _seq, const std::string& _contact) override;
            void get_archive_index(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, int32_t _recursion, std::function<void(int64_t)> last_message_catcher);
            void get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, int32_t _recursion, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher);
            std::shared_ptr<async_task_handlers> get_archive_messages_get_messages(int64_t _seq, const std::string& _contact
                , int64_t _from, int64_t _count_early, int64_t _count_later, int32_t _recursion, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher);
            virtual void get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher) override;
            virtual void get_archive_index(int64_t _seq_, const std::string& _contact, int64_t _from, int64_t _count, std::function<void(int64_t)> last_message_catcher) override;
            virtual void get_archive_messages_buddies(int64_t _seq, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids) override;

            std::shared_ptr<async_task_handlers> get_history_from_server(const get_history_params& _params, std::function<void(int64_t)> last_message_catcher);
            std::shared_ptr<async_task_handlers> set_dlg_state(const set_dlg_state_params& _params);
            virtual void add_opened_dialog(const std::string& _contact) override;
            virtual void remove_opened_dialog(const std::string& _contact) override;
            virtual void set_first_message(const std::string& _contact, int64_t _message) override;
            bool has_opened_dialogs(const std::string& _contact) const;
            int64_t get_first_message(const std::string& _contact) const;
            virtual void set_last_read(const std::string& _contact, int64_t _message) override;
            virtual void hide_dlg_state(const std::string& _contact) override;
            virtual void delete_archive_messages(const int64_t _seq, const std::string &_contact_aimid, const std::vector<int64_t> &_ids, const bool _for_all) override;
            virtual void delete_archive_messages_from(const int64_t _seq, const std::string &_contact_aimid, const int64_t _from_id) override;

            virtual void history_search_in_history(const std::string& search_patterns, const std::vector<std::string>& _aimids) override;
            virtual void history_search_in_cl(const std::vector<std::vector<std::string>>& search_patterns, int64_t _req_id, unsigned fixed_patterns_count, const std::string& pattern) override;
            virtual void setup_search_params(int64_t _req_id) override;
            virtual void clear_search_params() override;

            void download_failed_holes();
			void download_holes(const std::string& _contact, std::function<void(int64_t)> last_message_catcher);
			void download_holes(const std::string& _contact, int64_t _depth = -1, std::function<void(int64_t)> last_message_catcher = [](int64_t){});
            void download_holes(const std::string& _contact, int64_t _from, int64_t _depth = -1, int32_t _recursion = 0, std::function<void(int64_t)> last_message_catcher = [](int64_t){});

            virtual std::string _get_protocol_uid() override;

            void update_active_dialogs(const std::string& _aimid, archive::dlg_state& _state);
            void hide_chat_async(const std::string& _contact, const int64_t _last_msg_id, std::function<void(int32_t)> _on_result = [](int32_t){});
            virtual void hide_chat(const std::string& _contact) override;
            virtual void mute_chat(const std::string& _contact, bool _mute) override;
            std::shared_ptr<async_task_handlers> mute_chat_async(const std::string& _contact, bool _mute);
            void mute_chats(std::shared_ptr<std::list<std::string>> _chats);
            virtual void add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message) override;
            virtual void remove_contact(int64_t _seq, const std::string& _aimid) override;
            virtual void rename_contact(int64_t _seq, const std::string& _aimid, const std::string& _friendly) override;
            virtual void spam_contact(int64_t _seq, const std::string& _aimid) override;
            virtual void ignore_contact(int64_t _seq, const std::string& _aimid, bool ignore) override;
            virtual void get_ignore_list(int64_t _seq) override;
            virtual void favorite(const std::string& _contact) override;
            virtual void unfavorite(const std::string& _contact) override;

            std::shared_ptr<async_task_handlers> post_dlg_state_to_gui(
                const std::string _contact, 
                const bool _add_to_active_dialogs = false, 
                const bool _serialize_message = true, 
                const bool _force = false /*need for favorites*/);

            void post_dlg_state_to_gui(core::icollection* _dlg_state);
            void post_cached_dlg_states_to_gui();
            void stop_dlg_state_timer();
            void check_need_agregate_dlg_state();


            std::shared_ptr<async_task_handlers> post_history_search_result_msg_to_gui(const std::string _contact
                , bool _serialize_message
                , bool _from_search
                , int64_t _req_id
                , bool _is_contact
                , std::shared_ptr<::core::archive::history_message> _msg
                , std::string term
                , int32_t _priority);

            coll_helper serialize_history_search_result_msg(const std::string _contact
                , const archive::dlg_state& _state
                , bool _serialize_message
                , bool _from_search
                , int64_t _req_id
                , bool _is_contact
                , const std::string& term
                , int32_t _priority);

            // ------------------------------------------------------------------------------
            // files functions
            virtual void upload_file_sharing(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _file_name,
                std::shared_ptr<core::tools::binary_stream> _data,
                const std::string& _extension) override;

            virtual void get_file_sharing_preview_size(
                const int64_t _seq,
                const std::string& _file_url) override;

            virtual void download_file_sharing_metainfo(
                const int64_t _seq,
                const std::string& _file_url) override;

            virtual void download_file_sharing(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _file_url,
                const bool _force_request_metainfo,
                const std::string& _filename,
                const std::string& _download_dir,
                bool _raise_priority) override;

            virtual void download_image(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const std::string& _image_url,
                const std::string& _forced_path,
                const bool _download_preview,
                const int32_t _preview_width,
                const int32_t _preview_height,
                const bool _raise_priority) override;

            virtual void download_link_preview(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const std::string& _url,
                const int32_t _preview_width,
                const int32_t _preview_height) override;

            virtual void cancel_loader_task(const std::string& _url) override;

            virtual void abort_file_sharing_download(const std::string& _url) override;

            virtual void abort_file_sharing_upload(
                const int64_t _seq,
                const std::string & _contact,
                const std::string &_process_seq) override;

            virtual void raise_download_priority(
                const int64_t _task_id) override;

            virtual void contact_switched(const std::string &_contact_aimid) override;

            void download_link_preview_image(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const preview_proxy::link_meta &_meta,
                const int32_t _preview_width,
                const int32_t _preview_height);

            void download_link_preview_favicon(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const preview_proxy::link_meta &_meta);

            void request_snap_metainfo(const int64_t _seq, const std::string &_uri, const std::string& _ttl_id);

            virtual void set_played(const std::string& url, bool played) override;
            virtual void speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale) override;

            void upload_file_sharing_internal(const archive::not_sent_message_sptr& _not_sent);

            void resume_failed_network_requests();
            void resume_file_sharing_uploading(const archive::not_sent_message_sptr &_not_sent);
            void resume_all_file_sharing_uploading();
            void resume_download_stickers();
            void resume_download_masks();
            void resume_failed_avatars();
            // ------------------------------------------------------------------------------

            // group chat
            virtual void remove_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_remove) override;
            virtual void add_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_add) override;
            virtual void add_chat(int64_t _seq, const std::string& _m_chat_name, const std::vector<std::string>& _m_chat_members) override;
            virtual void modify_chat(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_name) override;

            // mrim
            virtual void get_mrim_key(int64_t _seq, const std::string& _email) override;

            static void on_created_groupchat(std::shared_ptr<diffs_map> _diff);

            void on_history_patch_version_changed(const std::string& _aimid, const std::string& _history_patch_version);
            void on_del_up_to_changed(const std::string& _aimid, const int64_t _del_up_to, const auto_callback_sptr& _on_complete);

            void merge_account();
            void attach_uin(int64_t _seq);
            void attach_phone(int64_t _seq, const auth_parameters& auth_params, const phone_info& _info);

            void apply_exported_settings();
            void apply_exported_muted_chats();
            void apply_exported_muted_chats_internal();

            void attach_uin_finished();
            void attach_phone_finished();

            virtual void update_profile(int64_t _seq, const std::vector<std::pair<std::string, std::string>>& _field) override;

            // alpha chats
            virtual void join_live_chat(int64_t _seq, const std::string& _stamp, const int _age) override;

            std::shared_ptr<async_task_handlers> send_timezone();

            // prefetching

            void history_search_one_batch(std::shared_ptr<archive::coded_term> _cterm, std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data, int64_t _seq
                , int64_t _min_id);

            void prefetch_last_dialog_messages(const std::string &_dlg_aimid, const char* const _reason);

            void post_unignored_contact_to_gui(const std::string& _aimid);

            void load_hosts_config();

            void check_for_change_hosts_scheme(int32_t _error);

            void need_update_search_cache();

            bool wait_function(int32_t _timeout);

        public:

            virtual void post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) override;
            virtual void post_voip_alloc_to_server(const std::string& data) override;

            virtual std::shared_ptr<wim::wim_packet> prepare_voip_msg(const std::string& data) override;
            virtual std::shared_ptr<wim::wim_packet> prepare_voip_pac(const voip_manager::VoipProtoMsg& data) override;

            void login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login) override;
            std::string get_contact_friendly_name(const std::string& contact_login) override;

            // events
            void on_login_result(int64_t _seq, int32_t err, bool _from_exported_data);
            void on_login_result_attach_uin(int64_t _seq, int32_t err, const auth_parameters& auth_params, const wim_packet_params& _from_params);

            void handle_net_error(int32_t err) override;

            void on_event_buddies_list(fetch_event_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_presence(fetch_event_presence* _event, std::shared_ptr<auto_callback> _on_complete);

            void on_event_dlg_state(fetch_event_dlg_state* _event, auto_callback_sptr _on_complete);
            void on_event_dlg_state_local_state_loaded(
                const auto_callback_sptr& _on_complete,
                const std::string& _aimid,
                const archive::dlg_state& _local_dlg_state,
                archive::dlg_state _server_dlg_state,
                const archive::history_block_sptr& _messages
                );
            void on_event_dlg_state_process_messages(
                const archive::history_block& _messages,
                const std::string& _aimid,
                InOut archive::dlg_state& _server_dlg_state
                );
            void on_event_dlg_state_local_state_updated(
                const auto_callback_sptr& _on_complete,
                const archive::history_block_sptr& _messages,
                const std::string& _aimid,
                const archive::dlg_state& _local_dlg_state,
                const bool _patch_version_changed,
                const bool _del_up_to_changed
                );
            void on_event_dlg_state_history_updated(
                const auto_callback_sptr& _on_complete,
                const bool _patch_version_changed,
                const archive::dlg_state& _local_dlg_state,
                const std::string& _aimid,
                const int64_t _last_message_id,
                const int32_t _count,
                const bool _del_up_to_changed
                );

            void on_event_hidden_chat(fetch_event_hidden_chat* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_diff(fetch_event_diff* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_my_info(fetch_event_my_info* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_user_added_to_buddy_list(fetch_event_user_added_to_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_typing(fetch_event_typing* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_permit(fetch_event_permit* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_imstate(fetch_event_imstate* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_notification(fetch_event_notification* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_snaps(fetch_event_snaps* _event, std::shared_ptr<auto_callback> _on_complete);

            virtual void search_contacts(int64_t _seq, const std::string& keyword, const std::string& phonenumber, const std::string& tag) override;
            virtual void get_profile(int64_t _seq, const std::string& _contact) override;

            virtual const wim_packet_params make_wim_params() override;
            virtual const wim_packet_params make_wim_params_general(bool _is_current_auth_params) override;
            const wim_packet_params make_wim_params_from_auth(const auth_parameters& _auth_params);

            im(const im_login_id& _login, std::shared_ptr<voip_manager::VoipManager> _voip_manager);
            virtual ~im();
            virtual void load_flags(const int64_t _seq) override;
            virtual void set_avatar(const int64_t _seq, tools::binary_stream image, const std::string& _aimId, const bool _chat) override;

            virtual void save_auth_to_export(std::function<void()> _on_result) override;

            virtual void read_snap(const uint64_t _snap_id, const std::string& _aimId, const bool _mark_prev_snaps_read, const bool _refresh_storage) override;
            virtual void delete_snap(const uint64_t _snap_id) override;
            virtual void download_snap_metainfo(const int64_t _seq, const std::string& _contact_aimid, const std::string &_ttl_id, bool _raise_priority) override;

            virtual void get_mask_id_list(int64_t _seq) override;
            virtual void get_mask_preview(int64_t _seq, const std::string& mask_id) override;
            virtual void get_mask_model(int64_t _seq) override;
            virtual void get_mask(int64_t _seq, const std::string& mask_id) override;

            virtual void refresh_snaps_storage() override;
            virtual void refresh_user_snaps(const std::string& _aimId) override;
            virtual void remove_from_snaps_storage(const std::string& _aimId) override;

            virtual bool has_valid_login() const override;
        };
    }
}
