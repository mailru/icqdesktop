#include "stdafx.h"
#include "wim_im.h"
#include "wim_packet.h"
#include "../contact_profile.h"

// packets
#include "packets/client_login.h"
#include "packets/start_session.h"
#include "packets/fetch.h"
#include "packets/get_sms_code.h"
#include "packets/get_chat_info.h"
#include "packets/get_chat_home.h"
#include "packets/login_by_phone.h"
#include "packets/normalize_phone.h"
#include "packets/send_message.h"
#include "packets/get_history.h"
#include "packets/set_dlg_state.h"
#include "packets/wim_webrtc.h"
#include "packets/hide_chat.h"
#include "packets/get_stickers_index.h"
#include "packets/get_themes_index.h"
#include "packets/get_profile.h"
#include "packets/search_contacts.h"
#include "packets/add_buddy.h"
#include "packets/remove_buddy.h"
#include "packets/set_buddy_attribute.h"
#include "packets/mute_buddy.h"
#include "packets/remove_members.h"
#include "packets/add_members.h"
#include "packets/add_chat.h"
#include "packets/mrim_get_key.h"
#include "packets/modify_chat.h"
#include "packets/spam_report.h"
#include "packets/set_permit_deny.h"
#include "packets/get_permit_deny.h"
#include "packets/set_state.h"
#include "packets/end_session.h"
#include "packets/send_message_typing.h"
#include "packets/send_feedback.h"
#include "packets/speech_to_text.h"
#include "packets/del_message.h"
#include "packets/del_history.h"
#include "packets/attach_phone.h"
#include "packets/attach_uin.h"
#include "packets/merge_account.h"
#include "packets/get_flags.h"
#include "packets/update_profile.h"
#include "packets/join_chat_alpha.h"
#include "packets/set_timezone.h"
#include "packets/set_avatar.h"
#include "packets/get_chat_blocked.h"
#include "packets/get_chat_pending.h"
#include "packets/mod_chat_alpha.h"
#include "packets/block_chat_member.h"
#include "packets/mod_chat_member_alpha.h"
#include "packets/phoneinfo.h"
#include "packets/resolve_pending.h"
#include "packets/snap_viewed.h"
#include "packets/create_chat.h"
#include "packets/get_hosts_config.h"
#include "packets/get_snaps_home.h"
#include "packets/get_user_snaps.h"
#include "packets/get_user_snaps_patch.h"
#include "packets/del_snap.h"

#include "../../http_request.h"

// robusto packets
#include "packets/gen_robusto_token.h"
#include "packets/robusto_add_client.h"


//events
#include "events/fetch_event.h"
#include "events/fetch_event_buddy_list.h"
#include "events/fetch_event_presence.h"
#include "events/fetch_event_dlg_state.h"
#include "events/fetch_event_hidden_chat.h"
#include "events/fetch_event_diff.h"
#include "events/fetch_event_my_info.h"
#include "events/fetch_event_typing.h"
#include "events/fetch_event_permit.h"
#include "events/fetch_event_imstate.h"
#include "events/fetch_event_notification.h"
#include "events/fetch_event_snaps.h"

//avatars
#include "avatar_loader.h"

// cashed contactlist objects
#include "wim_contactlist_cache.h"
#include "active_dialogs.h"
#include "favorites.h"
#include "mailboxes.h"

#include "../../async_task.h"

#include "async_loader/async_loader.h"

#include "loader/loader.h"
#include "../../../common.shared/loader_errors.h"
#include "loader/loader_handlers.h"
#include "loader/web_file_info.h"
#include "loader/preview_proxy.h"
#include "loader/snap_metainfo.h"

// stickers
#include "../../stickers/stickers.h"

// masks
#include "../../masks/masks.h"

#include "auth_parameters.h"
#include "../../themes/themes.h"
#include "../../core.h"
#include "../../../corelib/collection_helper.h"
#include "../../../corelib/core_face.h"
#include "../../../corelib/enumerations.h"
#include "../../utils.h"
#include "../login_info.h"
#include "../im_login.h"
#include "../../archive/image_cache.h"
#include "../../archive/local_history.h"
#include "../../archive/contact_archive.h"
#include "../../archive/history_message.h"
#include "../../archive/archive_index.h"
#include "../../archive/not_sent_messages.h"
#include "../../archive/messages_data.h"
#include "stat/imstat.h"
#include "dialog_holes.h"
#include "../../configuration/hosts_config.h"

#include "../../log/log.h"

#include "../../configuration/app_config.h"
#include "../../../common.shared/url_parser/url_parser.h"
#include "../../../common.shared/version_info.h"

#include "../../tools/system.h"
#include "../../tools/file_sharing.h"

#include "../../configuration/hosts_config.h"

#include "chat_params.h"

#include "wim_im.h"

using namespace core;
using namespace wim;

namespace
{
    const auto send_message_timeout = std::chrono::milliseconds(500); //milliseconds

    const auto start_session_timeout = 1000;

    const uint32_t sent_repeat_count = 2;

    const int64_t PREFETCH_COUNT = 30;

    bool should_sign_url(const std::string &_url);

    const int32_t empty_timer_id = -1;

    const int32_t post_messages_rate = 1000; //milliseconds

    const auto rate_limit_timeout = std::chrono::milliseconds(30000); // 30

    const auto dlg_state_agregate_start_timeout = std::chrono::minutes(3);
    const auto dlg_state_agregate_period = std::chrono::seconds(60);
}

//////////////////////////////////////////////////////////////////////////
// send_thread class
//////////////////////////////////////////////////////////////////////////
core::wim::wim_send_thread::wim_send_thread()
    :   is_packet_execute_(false)
{
}

core::wim::wim_send_thread::~wim_send_thread()
{
}

std::shared_ptr<async_task_handlers> core::wim::wim_send_thread::post_packet(
    std::shared_ptr<wim_packet> _packet,
    std::function<void(int32_t)> _error_handler,
    std::shared_ptr<async_task_handlers> _handlers)
{
    std::weak_ptr<wim_send_thread> wr_this(shared_from_this());

    std::shared_ptr<async_task_handlers> callback_handlers = _handlers ? _handlers : std::make_shared<async_task_handlers>();

    const auto can_run_async = _packet->support_async_execution();

    if (is_packet_execute_ && !can_run_async)
    {
        packets_queue_.push_back(task_and_params(_packet, _error_handler, callback_handlers));
        return callback_handlers;
    }

    if (!can_run_async)
        is_packet_execute_ = true;

    const auto current_time = std::chrono::system_clock::now();

    // need wait for timeout (ratelimts)
    if (current_time < cancel_packets_time_)
    {
        run_async_function([]()->int32_t
        {
            return 0;

        })->on_result_ = [wr_this, callback_handlers](int32_t /*_error*/)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            callback_handlers->on_result_(wpie_error_request_canceled_wait_timeout);

            ptr_this->execute_packet_from_queue();
        };

        return callback_handlers;
    }

    if (can_run_async)
    {
        _packet->execute_async([wr_this, _packet, _error_handler, callback_handlers](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == wim_protocol_internal_error::wpie_network_error)
            {
                if ((_packet->get_repeat_count() < sent_repeat_count) && (!_packet->is_stopped()))
                {
                    if (_packet->can_change_hosts_scheme())
                        _packet->change_hosts_scheme();

                    ptr_this->run_async_function([]() { return 0; })->on_result_ = [wr_this, _packet, _error_handler, callback_handlers](int32_t /*_error*/)
                    {
                        auto ptr_this = wr_this.lock();
                        if (ptr_this)
                            ptr_this->post_packet(_packet, _error_handler, callback_handlers);
                    };

                    return;
                }
            }
            else
            {
                if (_packet->is_hosts_scheme_changed())
                    g_core->get_hosts_config().update_hosts(_packet->get_hosts_scheme());
            }

            if (_error == wpie_error_too_fast_sending)
            {
                ptr_this->cancel_packets_time_ = std::chrono::system_clock::now() + rate_limit_timeout;
            }

            callback_handlers->on_result_(_error);

            if (_error != 0)
            {
                if (_error_handler)
                {
                    _error_handler(_error);
                }
            }

            ptr_this->run_async_function([]() { return 0; })->on_result_ = [wr_this](int32_t /*_error*/)
            {
                auto ptr_this = wr_this.lock();
                if (ptr_this)
                    ptr_this->execute_packet_from_queue();
            };
        });

        return callback_handlers;
    }

    auto internal_handlers = run_async_function([_packet]()->int32_t
    {
        return _packet->execute();
    });

    internal_handlers->on_result_ = [wr_this, _packet, _error_handler, callback_handlers](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == wim_protocol_internal_error::wpie_network_error)
        {
            if ((_packet->get_repeat_count() < sent_repeat_count) && (!_packet->is_stopped()))
            {
                ptr_this->is_packet_execute_ = false;

                if (_packet->can_change_hosts_scheme())
                    _packet->change_hosts_scheme();

                ptr_this->post_packet(_packet, _error_handler, callback_handlers);

                return;
            }
        }
        else
        {
            if (_packet->is_hosts_scheme_changed())
                g_core->get_hosts_config().update_hosts(_packet->get_hosts_scheme());
        }

        if (_error == wpie_error_too_fast_sending)
        {
            ptr_this->cancel_packets_time_ = std::chrono::system_clock::now() + rate_limit_timeout;
        }

        callback_handlers->on_result_(_error);

        ptr_this->is_packet_execute_ = false;

        if (_error != 0)
        {
            if (_error_handler)
            {
                _error_handler(_error);
            }
        }

        ptr_this->execute_packet_from_queue();
    };

    return callback_handlers;
}


std::shared_ptr<async_task_handlers> core::wim::wim_send_thread::post_packet(std::shared_ptr<wim_packet> _packet, const std::function<void(int32_t)> _error_handler)
{
    return post_packet(_packet, _error_handler, nullptr);
}


void core::wim::wim_send_thread::execute_packet_from_queue()
{
    if (!packets_queue_.empty())
    {
        auto next_packet = packets_queue_.front();
        packets_queue_.pop_front();

        post_packet(next_packet.task_, next_packet.error_handler_, next_packet.callback_handlers_);
    }
}

void core::wim::wim_send_thread::clear()
{
    for (auto iter = packets_queue_.begin(); iter != packets_queue_.end(); ++iter)
        iter->callback_handlers_->on_result_(wpie_error_task_canceled);

    packets_queue_.clear();

}
//////////////////////////////////////////////////////////////////////////
// end send_thread class
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// robusto_thread
//////////////////////////////////////////////////////////////////////////
void robusto_thread::push_packet_to_queue(
    std::shared_ptr<robusto_packet> _packet,
    std::shared_ptr<async_task_handlers> _handlers)
{
    packets_queue_.push_back(std::make_shared<task_and_params>(_packet, _handlers));
}


std::shared_ptr<robusto_thread::task_and_params> robusto_thread::get_front_packet_from_queue()
{
    if (!packets_queue_.size())
        return nullptr;

    auto packet = packets_queue_.front();

    packets_queue_.pop_front();

    return packet;
}
//////////////////////////////////////////////////////////////////////////
// end robusto_thread class
//////////////////////////////////////////////////////////////////////////

std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
    std::shared_ptr<archive::face> _archive,
    const std::string& _contact,
    std::shared_ptr<archive::history_block> _messages);

const auto search_threads_count = 3;
const auto sending_search_results_interval_ms = std::chrono::milliseconds(500);

//////////////////////////////////////////////////////////////////////////
// im class
//////////////////////////////////////////////////////////////////////////
im::im(const im_login_id& _login, std::shared_ptr<voip_manager::VoipManager> _voip_manager)
    : base_im(_login, _voip_manager),
    wim_send_thread_(new wim_send_thread()),
    robusto_threads_(new robusto_thread()),
    fetch_thread_(new fetch_thread()),
    async_tasks_(new async_executer()),
    auth_params_(new auth_parameters()),
    attached_auth_params_(new auth_parameters()),
    fetch_params_(new fetch_parameters()),
    contact_list_(new contactlist()),
    active_dialogs_(new active_dialogs()),
    mailbox_storage_(new mailbox_storage()),
    snaps_storage_(new snaps_storage()),
    favorites_(new favorites()),
    store_timer_id_(0),
    stat_timer_id_(0),
    hosts_config_timer_id_(0),
    dlg_state_timer_(0),
    mute_chats_timer_(0),
    sent_pending_messages_active_(false),
    imstat_(new statistic::imstat()),
    my_info_cache_(new my_info_cache()),
    stop_objects_(new stop_objects()),
    failed_holes_requests_(new holes::failed_requests()),
    im_created_(false),
    start_session_time_(std::chrono::system_clock::now() - std::chrono::milliseconds(start_session_timeout)),
    prefetch_uid_(INT64_MAX),
    history_searcher_(new async_executer(search_threads_count)),
    post_messages_timer_(-1),
    last_success_network_post_(std::chrono::system_clock::now()),
    last_check_alt_scheme_reset_(std::chrono::system_clock::now()),
    dlg_state_agregate_mode_(false),
    last_network_activity_time_(std::chrono::system_clock::now() - dlg_state_agregate_start_timeout)
{
    stop_objects_weak_ = std::weak_ptr<stop_objects>(stop_objects_);
}


im::~im()
{
    stop_store_timer();
    stop_dlg_state_timer();
    save_cached_objects();
    cancel_requests();
    stop_stat_timer();
    stop_hosts_config_timer();
}

void im::schedule_store_timer()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    store_timer_id_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this) return;

        ptr_this->save_cached_objects();

    }, 10000);
}

void im::stop_store_timer()
{
    if ((store_timer_id_ > 0) && g_core)
    {
        g_core->stop_timer(store_timer_id_);
    }
}


void im::schedule_stat_timer()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    uint32_t timeout = (build::is_debug() ? (10 * 1000) : (60 * 1000));

    stat_timer_id_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->send_statistic_if_needed();

    }, timeout);
}

void im::stop_stat_timer()
{
    if ((stat_timer_id_ > 0) && g_core)
    {
        g_core->stop_timer(stat_timer_id_);
    }
}

void im::schedule_hosts_config_timer()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    hosts_config_timer_id_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->load_hosts_config();

    }, hosts_config::get_first_request_timeout_ms());
}

void im::stop_hosts_config_timer()
{
    if (g_core)
    {
        g_core->stop_timer(hosts_config_timer_id_);
    }
}


void im::connect()
{
    load_auth_and_fetch_parameters();

}

const robusto_packet_params im::make_robusto_params()
{
    static uint32_t rubusto_req_id = 0;

    return robusto_packet_params(
        auth_params_->robusto_token_,
        auth_params_->robusto_client_id_,
        ++rubusto_req_id);
}

const core::wim::wim_packet_params im::make_wim_params()
{
    return make_wim_params_general(true);
}

const core::wim::wim_packet_params im::make_wim_params_general(bool _is_current_auth_params)
{
    return make_wim_params_from_auth(_is_current_auth_params ? *auth_params_ : *attached_auth_params_);
}


const core::wim::wim_packet_params im::make_wim_params_from_auth(const auth_parameters& _auth_params)
{
    assert(g_core->is_core_thread());

    auto active_session = stop_objects_->active_session_id_;

    const std::weak_ptr<stop_objects> wr_stop(stop_objects_);

    auto stop_handler = [wr_stop, active_session]
    {
        auto ptr_stop = wr_stop.lock();
        if (!ptr_stop)
            return true;

        std::lock_guard<std::mutex> lock(ptr_stop->stop_mutex_);
        return(active_session != ptr_stop->active_session_id_);
    };

    auto proxy = g_core->get_proxy_settings();

    return wim_packet_params(
        stop_handler,
        _auth_params.a_token_,
        _auth_params.session_key_,
        _auth_params.dev_id_,
        _auth_params.aimsid_,
        g_core->get_uniq_device_id(),
        _auth_params.aimid_,
        _auth_params.time_offset_,
        proxy,
        core::configuration::get_app_config().full_log_,
        g_core->get_hosts_config().get_hosts());
}



void im::handle_net_error(int32_t err) {
    if (err == wpie_error_need_relogin) {
        start_session();
    }
}

std::shared_ptr<async_task_handlers> im::post_wim_packet(std::shared_ptr<wim_packet> _packet)
{
    assert(!std::dynamic_pointer_cast<robusto_packet>(_packet));

    std::weak_ptr<im> wr_this(shared_from_this());

    auto on_error = [wr_this, _packet](int32_t err)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->handle_net_error(err);
    };

    return wim_send_thread_->post_packet(_packet, on_error);
}

void im::login(int64_t _seq, const login_info& _info)
{
    switch (_info.get_login_type())
    {
    case login_type::lt_login_password:
        {
            login_by_password(_seq, _info.get_login(), _info.get_password(), _info.get_save_auth_data(), true /* start_session */, false);
        }
        break;
    default:
        {
            assert(!"unknown login type");
        }
        break;
    }
}

void im::start_attach_uin(int64_t _seq, const login_info& _info, const wim_packet_params& _from_params)
{
    switch (_info.get_login_type())
    {
    case login_type::lt_login_password:
        {
            login_by_password_and_attach_uin(_seq, _info.get_login(), _info.get_password(), _from_params);
        }
        break;
    default:
        {
            assert(!"unknown login type");
        }
        break;
    }
}

void im::login_by_agent_token(
    const std::string& _login, 
    const std::string& _token, 
    const std::string& _guid)
{
    auto packet = std::make_shared<client_login>(make_wim_params(), _login, _token);
    packet->set_product_guid_8x(_guid);

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [wr_this, packet](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->a_token_ = packet->get_a_token();
            ptr_this->auth_params_->session_key_ = packet->get_session_key();
            ptr_this->auth_params_->exipired_in_ = packet->get_expired_in();
            ptr_this->auth_params_->time_offset_ = packet->get_time_offset();

            ptr_this->auth_params_->serializable_ = true;

            ptr_this->on_login_result(0, 0, true);

            ptr_this->start_session();
        }
        else
        {
            ptr_this->on_login_result(0, _error, true);
        }
    };
}

void im::login_by_password(
    int64_t _seq, 
    const std::string& login, 
    const std::string& password, 
    const bool save_auth_data, 
    const bool start_session,
    const bool _from_export_login)
{
    auto packet = std::make_shared<client_login>(make_wim_params(), login, password);

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [wr_this, packet, save_auth_data, start_session, _seq, _from_export_login](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->a_token_ = packet->get_a_token();
            ptr_this->auth_params_->session_key_ = packet->get_session_key();
            ptr_this->auth_params_->exipired_in_ = packet->get_expired_in();
            ptr_this->auth_params_->time_offset_ = packet->get_time_offset();

            ptr_this->auth_params_->serializable_ = save_auth_data;

            ptr_this->on_login_result(_seq, 0, _from_export_login);
            ptr_this->start_session();
        }
        else
        {
            ptr_this->on_login_result(_seq, _error, _from_export_login);
        }
    };
}

void im::login_by_password_and_attach_uin(int64_t _seq, const std::string& login, const std::string& password, const wim_packet_params& _from_params)
{
    auto packet = std::make_shared<client_login>(make_wim_params(), login, password); // TODO : new_wim_params

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [wr_this, packet, _from_params, _seq](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto auth_params = ptr_this->attached_auth_params_;
            auth_params->a_token_ = packet->get_a_token();
            auth_params->session_key_ = packet->get_session_key();
            auth_params->exipired_in_ = packet->get_expired_in();
            auth_params->time_offset_ = packet->get_time_offset();

            // ptr_this->auth_params_->serializable_ = save_auth_data;

            ptr_this->on_login_result_attach_uin(_seq, 0, *(ptr_this->attached_auth_params_.get()), _from_params);
        }
        else
        {
            ptr_this->on_login_result_attach_uin(_seq, _error, *(ptr_this->attached_auth_params_.get()), _from_params);
        }
    };
}

std::wstring im::get_auth_parameters_filename()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(auth_file_name));
}

std::wstring im::get_fetch_parameters_filename()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(fetch_url_file_name));
}

std::wstring im::get_auth_parameters_filename_exported()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(auth_export_file_name));
}

std::wstring im::get_auth_parameters_filename_merge()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(auth_export_file_name_merge));
}

std::wstring im::get_exported_muted_chats_filename()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(muted_chats_export_file_name));
}


void im::erase_auth_data()
{
    auth_params_->clear();

    store_auth_parameters();
}

void im::store_auth_parameters()
{
    if (!auth_params_->serializable_)
        return;

    auto bstream = std::make_shared<core::tools::binary_stream>();
    auth_params_->serialize(*bstream);

    std::wstring file_name = get_auth_parameters_filename();

    async_tasks_->run_async_function([bstream, file_name]
    {
        if (!bstream->save_2_file(file_name))
            return -1;

        return 0;
    });
}

void im::save_auth_to_export(std::function<void()> _on_result)
{
    std::wstring export_file = get_auth_parameters_filename_exported();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auth_params_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    async_tasks_->run_async_function([export_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(export_file))
            return -1;

        return 0;

    })->on_result_ = [wr_this, _on_result](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            _on_result();
        }
    };
}

void im::read_snap(const uint64_t _snap_id, const std::string& _aimId, const bool _mark_prev_snaps_read, const bool _refresh_storage)
{
    assert(_snap_id > 0);
    assert(!_aimId.empty());

    auto packet = std::make_shared<snap_viewed>(make_wim_params(), _snap_id, _mark_prev_snaps_read, _aimId);

    post_robusto_packet(packet)->on_result_ =
        []
        (int32_t _error)
        {
        };

    if (_refresh_storage)
    {
        refresh_user_snaps(_aimId);
    }
}

void im::delete_snap(const uint64_t _snap_id)
{
    auto packet = std::make_shared<del_snap>(make_wim_params(), _snap_id);

    post_robusto_packet(packet)->on_result_ =
        []
    (int32_t _error)
    {
    };

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    auto aimId = auth_params_->aimid_;
    auto snaps_packet = std::make_shared<core::wim::get_user_snaps>(make_wim_params(), aimId);
    post_robusto_packet(snaps_packet)->on_result_ = [wr_this, snaps_packet, aimId](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        ptr_this->snaps_storage_->update_user_snaps(aimId, snaps_packet->result_, coll.get());
        g_core->post_message_to_gui("snaps/user_snaps", 0, coll.get());
    };
}

void im::download_snap_metainfo(const int64_t _seq, const std::string& _contact_aimid, const std::string &_ttl_id, bool _raise_priority)
{
    assert(_seq > 0);
    assert(!_ttl_id.empty());

    get_async_loader().download_snap_metainfo(_ttl_id, make_wim_params(), snap_meta_handler_t(
        [_seq, _ttl_id](loader_errors _error, const snap_meta_data_t& _data)
        {
            coll_helper cl_coll(g_core->create_collection(), true);

            const auto success = (_error == loader_errors::success);

            cl_coll.set<bool>("success", success);
            cl_coll.set<bool>("not_found", _error == loader_errors::http_error);
            if (_error == loader_errors::http_error)
                cl_coll.set<std::string>("ttl_id", _ttl_id);

            if (_data.additional_data_)
            {
                _data.additional_data_->serialize(Out cl_coll.get());
            }

            g_core->post_message_to_gui("snap/get_metainfo/result", _seq, cl_coll.get());
        }));
}

void im::get_mask_id_list(int64_t _seq)
{
    if (!masks_)
        create_masks(shared_from_this());

    masks_->get_mask_id_list(_seq);
}

void im::get_mask_preview(int64_t _seq, const std::string& mask_id)
{
    assert(masks_ && "call get_mask_id_list first");

    masks_->get_mask_preview(_seq, mask_id);
}

void im::get_mask_model(int64_t _seq)
{
    assert(masks_ && "call get_mask_id_list first");

    masks_->get_mask_model(_seq);
}

void im::get_mask(int64_t _seq, const std::string& mask_id)
{
    assert(masks_ && "call get_mask_id_list first");

    masks_->get_mask(_seq, mask_id);
}

void im::refresh_snaps_storage()
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto get_user_snaps = [wr_this](const std::string aimId)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto snaps_packet = std::make_shared<core::wim::get_user_snaps>(ptr_this->make_wim_params(), aimId);
        ptr_this->post_robusto_packet(snaps_packet)->on_result_ = [wr_this, snaps_packet, aimId](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            coll_helper coll(g_core->create_collection(), true);
            ptr_this->snaps_storage_->update_user_snaps(aimId, snaps_packet->result_, coll.get());
            coll.set_value_as_bool("from_refresh", true);
            g_core->post_message_to_gui("snaps/user_snaps", 0, coll.get());
        };
    };

    auto snaps_packet = std::make_shared<core::wim::get_snaps_home>(make_wim_params());
    post_robusto_packet(snaps_packet)->on_result_ = [wr_this, snaps_packet, get_user_snaps](int32_t _error)
    {
        if (_error != 0)
            return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->snaps_storage_ = snaps_packet->result_;

        auto snaps = snaps_packet->result_->get_snaps();
        for (auto s : snaps)
        {
            get_user_snaps(s.first);
        }
    };
}

void im::refresh_user_snaps(const std::string& _aimId)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    auto snaps_packet = std::make_shared<core::wim::get_user_snaps>(make_wim_params(), _aimId);
    post_robusto_packet(snaps_packet)->on_result_ = [wr_this, snaps_packet, _aimId](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        ptr_this->snaps_storage_->update_user_snaps(_aimId, snaps_packet->result_, coll.get());
        g_core->post_message_to_gui("snaps/user_snaps", 0, coll.get());
    };
}

void im::remove_from_snaps_storage(const std::string& _aimId)
{
    if (snaps_storage_)
        snaps_storage_->remove_user(_aimId);
}

void im::load_auth_and_fetch_parameters()
{
    std::wstring file_name = get_auth_parameters_filename();
    std::wstring file_name_exported = get_auth_parameters_filename_exported();
    std::wstring file_name_fetch = get_fetch_parameters_filename();

    auto auth_params = std::make_shared<auth_parameters>();
    auto fetch_params = std::make_shared<fetch_parameters>();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    core::tools::binary_stream bs_fetch;
    if (bs_fetch.load_from_file(file_name_fetch) && fetch_params->unserialize(bs_fetch))
    {
    }

    bool init_result = true;

    do 
    {
        core::tools::binary_stream bs_auth;
        if (bs_auth.load_from_file(file_name) && auth_params->unserialize(bs_auth))
            break;

        core::tools::binary_stream bstream_exported;
        if (bstream_exported.load_from_file(file_name_exported))
        {
            bstream_exported.write<char>('\0');

            rapidjson::Document doc;
            if (!doc.Parse(bstream_exported.read_available()).HasParseError())
            {
                if (auth_params->unserialize(doc))
                    break;
            }
        }

        init_result = false;
    }
    while (false);

    if (init_result && auth_params->is_valid())
    {
        auth_params_ = auth_params;
        fetch_params_ = fetch_params;

        async_tasks_->run_async_function([]
        {
            return 0;

        })->on_result_ = [wr_this, auth_params, fetch_params](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->load_cached_objects();
        };
    }
    else
    {
        g_core->unlogin(false);
    }
}


void im::store_fetch_parameters()
{
    auto bstream = std::make_shared<core::tools::binary_stream>();
    fetch_params_->serialize(*bstream);

    std::wstring file_name = get_fetch_parameters_filename();

    async_tasks_->run_async_function([bstream, file_name]
    {
        if (!bstream->save_2_file(file_name))
            return -1;

        return 0;
    });
}

std::shared_ptr<async_task_handlers> im::load_active_dialogs()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring active_dialogs_file = get_active_dilaogs_file_name();
    auto active_dlgs = std::make_shared<active_dialogs>();

    async_tasks_->run_async_function([active_dialogs_file, active_dlgs]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(active_dialogs_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return active_dlgs->unserialize(doc);

    })->on_result_ = [wr_this, active_dlgs, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->active_dialogs_ = active_dlgs;
            //	ptr_this->post_active_dialogs_to_gui();

            if (!active_dlgs->size())
                ptr_this->post_active_dialogs_are_empty_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_contact_list()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring contact_list_file = get_contactlist_file_name();
    auto contact_list = std::make_shared<contactlist>();

    async_tasks_->run_async_function([contact_list_file, contact_list]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(contact_list_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return contact_list->unserialize(doc);

    })->on_result_ = [wr_this, contact_list, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->contact_list_ = contact_list;
            ptr_this->post_contact_list_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_favorites()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring favorites_file = get_favorites_file_name();
    auto fvrts = std::make_shared<favorites>();

    async_tasks_->run_async_function([favorites_file, fvrts]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(favorites_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return fvrts->unserialize(doc);

    })->on_result_ = [wr_this, fvrts, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->favorites_ = fvrts;

            core::stats::event_props_type props;
            props.emplace_back("count", std::to_string(fvrts->size()));
            g_core->insert_event(core::stats::stats_event_names::favorites_load, props);

            for (auto fvrt : ptr_this->favorites_->contacts())
            {
                if (!ptr_this->contact_list_->is_ignored(fvrt.get_aimid()))
                {
                    ptr_this->post_dlg_state_to_gui(fvrt.get_aimid(), true, true, true);
                }
            }
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;

}

std::shared_ptr<async_task_handlers> im::load_mailboxes()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring mailboxes_file = get_mailboxes_file_name();
    auto mailboxes = std::make_shared<mailbox_storage>();

    async_tasks_->run_async_function([mailboxes_file, mailboxes]
    {
        return mailboxes->load(mailboxes_file);
    })->on_result_ = [wr_this, mailboxes, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->mailbox_storage_ = mailboxes;
            coll_helper coll(g_core->create_collection(), true);
            ptr_this->mailbox_storage_->serialize(coll);
            coll.set_value_as_bool("init", true);
            g_core->post_message_to_gui("mailboxes/status", 0, coll.get());
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_snaps_storage()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring snaps_storage_file = get_snaps_storage_filename();
    auto snaps = std::make_shared<snaps_storage>();

    async_tasks_->run_async_function([snaps_storage_file, snaps]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(snaps_storage_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return snaps->unserialize(doc);

    })->on_result_ = [wr_this, snaps, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->snaps_storage_ = snaps;
            coll_helper coll(g_core->create_collection(), true);
            ptr_this->snaps_storage_->serialize(coll.get());
            coll.set_value_as_bool("from_cache", true);
            g_core->post_message_to_gui("snaps/storage", 0, coll.get());
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_my_info()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto file_name = get_my_info_file_name();

    async_tasks_->run_async_function([wr_this, file_name]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 1;

        return ptr_this->my_info_cache_->load(file_name);

    })->on_result_ = [wr_this, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->post_my_info_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

void im::resume_download_stickers()
{
    if (get_stickers()->get_last_error() == loader_errors::network_error)
    {
        get_stickers()->set_last_error(loader_errors::success);

        const auto& rparams = get_stickers()->get_gui_request_params();

        switch (get_stickers()->get_failed_step())
        {
        case stickers::failed_step::download_metafile:
            {
                download_stickers_metafile(
                    rparams.seq_,
                    rparams.size_,
                    rparams.md5_);

                break;
            }
        case stickers::failed_step::download_metadata:
            {
                download_stickers_metadata(
                    rparams.seq_,
                    rparams.size_);

                break;
            }
        case stickers::failed_step::download_stickers:
            {
                download_stickers(
                    rparams.seq_,
                    rparams.size_);

                break;
            }
        default:
            {
                assert(false);
                break;
            }
        }
    }
}

void im::resume_download_masks()
{
    if (!masks_)
        create_masks(shared_from_this());

    masks_->on_connection_restored();
}

void im::post_stickers_meta_to_gui(int64_t _seq, const std::string& _size)
{
    coll_helper _coll(g_core->create_collection(), true);

    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->serialize_meta(_coll, _size)->on_result_ = [wr_this, _seq](coll_helper _coll)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->post_message_to_gui("stickers/meta/get/result", _seq, _coll.get());
    };
}

void im::download_stickers_metadata(int64_t _seq, const std::string _size)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->get_next_meta_task()->on_result_ = [wr_this, _size, _seq](bool _res, const stickers::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            if (!ptr_this->get_stickers()->is_up_to_date() || g_core->locale_was_changed())
                ptr_this->post_stickers_meta_to_gui(_seq, _size);

            ptr_this->download_stickers(_seq, _size);

            return;
        }

        ptr_this->get_async_loader().download_file(high_priority, _task.get_source_url(), _task.get_dest_file(), ptr_this->make_wim_params(), file_info_handler_t(
            [wr_this, _seq, _size, _task](loader_errors _error, const file_info_data_t& _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (loader_errors::network_error == _error)
                {
                    ptr_this->get_stickers()->set_last_error(loader_errors::network_error);
                    ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_metadata);
                    return;
                }

                ptr_this->get_stickers()->on_metadata_loaded(_task)->on_result_ = [wr_this, _seq, _size, _task](bool)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_stickers_metadata(_seq, _size);
                };
            }));
    };
}

void post_sticker_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, core::sticker_size _size, tools::binary_stream& _data)
{
    assert(_size > sticker_size::min);
    assert(_size < sticker_size::max);

    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_int("set_id", _set_id);
    coll.set_value_as_int("sticker_id", _sticker_id);

    const auto write_data =
        [&coll](tools::binary_stream &_data, const char *id)
    {
        if (_data.available() == 0)
        {
            return;
        }

        ifptr<istream> sticker_data(coll->create_stream(), true);
        const auto data_size = _data.available();
        sticker_data->write((const uint8_t*)_data.read(data_size), data_size);

        coll.set_value_as_stream(id, sticker_data.get());
    };

    if (_size == sticker_size::small)
    {
        write_data(_data, "data/small");
    }
    else if (_size == sticker_size::medium)
    {
        write_data(_data, "data/medium");
    }
    else if (_size == sticker_size::large)
    {
        write_data(_data, "data/large");
    }

    g_core->post_message_to_gui("stickers/sticker/get/result", _seq, coll.get());
}


void im::download_stickers(int64_t _seq, const std::string _size)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->get_next_sticker_task()->on_result_ = [wr_this, _size, _seq](bool _res, const stickers::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            ptr_this->get_stickers()->set_download_in_progress(false);
            return;
        }

        ptr_this->get_async_loader().download_file(high_priority, _task.get_source_url(), _task.get_dest_file(), ptr_this->make_wim_params(), file_info_handler_t(
            [wr_this, _seq, _size, _task](loader_errors _error, const file_info_data_t& _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == loader_errors::network_error)
                {
                    ptr_this->get_stickers()->set_last_error(loader_errors::network_error);
                    ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_stickers);
                    return;
                }

                int32_t set_id = _task.get_set_id(), sticker_id = _task.get_sticker_id(); sticker_size sz = _task.get_size();

                ptr_this->get_stickers()->on_sticker_loaded(_task)->on_result_ = [wr_this, _seq, _size, set_id, sticker_id, sz, _error]
                (bool _res, const std::list<int64_t>& _requests)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    if (_res && !_requests.empty() && _error == loader_errors::success)
                    {
                        ptr_this->get_stickers()->get_sticker(
                            _seq,
                            set_id,
                            sticker_id,
                            sz)->on_result_ = [_requests, set_id, sticker_id, sz](tools::binary_stream& _data)
                        {
                            for (auto seq : _requests)
                                post_sticker_2_gui(seq, set_id, sticker_id, sz, _data);
                        };
                    }

                    ptr_this->download_stickers(_seq, _size);
                };
            }));
    };
}


void im::load_stickers_data(int64_t _seq, const std::string _size)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_stickers()->make_download_tasks()->on_result_ = [wr_this, _seq, _size](bool)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_stickers_metadata(_seq, _size);
    };
}

void im::on_im_created()
{
    im_created_ = true;

    g_core->post_message_to_gui("im/created", 0, 0);

    schedule_store_timer();
    schedule_stat_timer();
    schedule_hosts_config_timer();
}

void im::load_cached_objects()
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto call_on_exit = std::make_shared<tools::auto_scope_bool>([wr_this](const bool _success)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_success)
        {
            ptr_this->poll(true, false);
        }
        else
        {
            if (ptr_this->auth_params_->is_valid_token())
            {
                ptr_this->start_session();
            }
            else if (ptr_this->auth_params_->is_valid_agent_token())
            {
                ptr_this->login_by_agent_token(
                    ptr_this->auth_params_->login_, 
                    ptr_this->auth_params_->agent_token_, 
                    ptr_this->auth_params_->product_guid_8x_);
            }
            else
            {
                ptr_this->login_by_password(
                    0, 
                    ptr_this->auth_params_->login_, 
                    ptr_this->auth_params_->password_md5_, 
                    true, 
                    true,
                    true);
            }

        }
    });

    tools::version_info info_this;

    const bool is_version_empty = auth_params_->version_.empty();

    bool is_new_version = true;

    if (!is_version_empty)
    {
        is_new_version = (tools::version_info(auth_params_->version_) < info_this);
    }

    const bool is_locale_changed = (g_core->get_locale() != auth_params_->locale_);

    if (is_version_empty || is_new_version || is_locale_changed)
    {
        return;
    }

    load_contact_list()->on_result_ = [wr_this, call_on_exit](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error != 0)
            return;

        ptr_this->load_snaps_storage()->on_result_ = [wr_this, call_on_exit](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->load_mailboxes()->on_result_ = [wr_this, call_on_exit](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->load_my_info()->on_result_ = [wr_this, call_on_exit](int32_t _error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    if (_error != 0)
                        return;

                    ptr_this->load_favorites()->on_result_ = [wr_this, call_on_exit](int32_t _error)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        ptr_this->load_active_dialogs()->on_result_ = [wr_this, call_on_exit](int32_t _error)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            if (_error == 0)
                            {
                                call_on_exit->set_success();

                                auto avatar_size = g_core->get_core_gui_settings().recents_avatars_size_;

                                ptr_this->active_dialogs_->enumerate([wr_this, avatar_size](const active_dialog& _dlg)
                                {
                                    auto ptr_this = wr_this.lock();
                                    if (!ptr_this)
                                        return;

                                    const auto &dlg_aimid = _dlg.get_aimid();

                                    if (!ptr_this->contact_list_->is_ignored(dlg_aimid))
                                    {
                                        if (avatar_size > 0)
                                            ptr_this->get_contact_avatar(-1, _dlg.get_aimid(), avatar_size, false);

                                        ptr_this->post_dlg_state_to_gui(dlg_aimid);
                                    }
                                });

                                ptr_this->post_ignorelist_to_gui(0);
                            }
                        };
                    };
                };
            };
        };
    };
}

void im::download_stickers_metafile(int64_t _seq, const std::string& _size, const std::string& _md5)
{
    get_stickers()->set_up_to_date(false);
    get_stickers()->set_download_in_progress(true);

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<get_stickers_index>(make_wim_params(), _md5);

    post_wim_packet(packet)->on_result_ = [wr_this, packet, _seq, _size](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto response = packet->get_response();

            ptr_this->get_stickers()->parse(response, false)->on_result_ = [response, wr_this, _seq, _size](const stickers::parse_result& _res)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_res.is_success())
                {
                    ptr_this->get_stickers()->set_up_to_date(_res.is_up_to_date());

                    if (!_res.is_up_to_date())
                    {
                        response->reset_out();

                        ptr_this->get_stickers()->save(response)->on_result_ = [wr_this, _seq, _size](bool _res)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            ptr_this->load_stickers_data(_seq, _size);
                        };
                    }
                    else
                    {
                        ptr_this->load_stickers_data(_seq, _size);
                    }
                }
            };
        }
        else
        {
            if (_error == wim_protocol_internal_error::wpie_network_error ||
                _error == wim_protocol_internal_error::wpie_error_task_canceled)
            {
                ptr_this->get_stickers()->set_last_error(loader_errors::network_error);
                ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_metafile);
            }
        }
    };
}

void im::get_stickers_meta(int64_t _seq, const std::string& _size)
{
    if (get_stickers()->is_meta_requested())
        return;

    get_stickers()->set_meta_requested();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::string stickers_size = _size;

    get_stickers()->load_meta_from_local()->on_result_ = [wr_this, stickers_size, _seq](const stickers::load_result& _res)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_stickers()->set_gui_request_params(stickers::gui_request_params(stickers_size, _seq, _res.get_md5()));

        if (_res.get_result() && !g_core->locale_was_changed())
            ptr_this->post_stickers_meta_to_gui(_seq, stickers_size);

        ptr_this->download_stickers_metafile(_seq, stickers_size, _res.get_md5());
    };
}


void im::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, core::sticker_size _size)
{
    assert(_size > sticker_size::min);
    assert(_size < sticker_size::max);

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_stickers()->get_sticker(_seq, _set_id, _sticker_id, _size)->on_result_ =
        [_seq, _set_id, _sticker_id, _size, wr_this](tools::binary_stream& _sticker_data)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_sticker_data.available() && !ptr_this->get_stickers()->is_download_in_progress())
        {
            std::stringstream ss_sticker_size;
            ss_sticker_size << _size;
            std::string size = ss_sticker_size.str();

            ptr_this->get_stickers()->get_md5()->on_result_ = [wr_this, size](const std::string& _md5)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_stickers_metafile(0, size, _md5);
            };
        }

        post_sticker_2_gui(_seq, _set_id, _sticker_id, _size, _sticker_data);
    };
}

void im::get_chat_info(int64_t _seq, const std::string& _aimid, const std::string& _stamp, int32_t _limit)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_chat_info_params params;
    params.aimid_ = _aimid;
    params.stamp_ = _stamp;
    params.members_limit_ = _limit;

    auto packet = std::make_shared<core::wim::get_chat_info>(make_wim_params(), params);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);
            packet->result_.serialize(coll);

            g_core->post_message_to_gui("chats/info/get/result", _seq, coll.get());
        }
        else
        {
            coll_helper coll(g_core->create_collection(), true);

            int32_t error_code = -1;
            switch (_error)
            {
            case wpie_error_robusto_you_are_not_chat_member:
                error_code = (int32_t)core::group_chat_info_errors::not_in_chat;
                break;
            case wpie_network_error:
                error_code = (int32_t)core::group_chat_info_errors::network_error;
                break;
            case  wpie_error_robusto_you_are_blocked:
                error_code = (int32_t)core::group_chat_info_errors::blocked;
                break;
            default:
                error_code = (int32_t)core::group_chat_info_errors::min;
            }

            coll.set_value_as_int("error", error_code);
            g_core->post_message_to_gui("chats/info/get/failed", _seq, coll.get());
        }
    };
}

void im::get_chat_blocked(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_chat_blocked_params params;
    params.aimid_ = _aimid;

    auto packet = std::make_shared<core::wim::get_chat_blocked>(make_wim_params(), params);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);

            ifptr<iarray> members_array(coll->create_array());

            if (!packet->result_.empty())
            {
                members_array->reserve((int32_t)packet->result_.size());
                for (const auto member : packet->result_)
                {
                    coll_helper coll_message(coll->create_collection(), true);
                    coll_message.set_value_as_string("aimid", member.aimid_);
                    coll_message.set_value_as_string("first_name", member.first_name_);
                    coll_message.set_value_as_string("last_name", member.last_name_);
                    coll_message.set_value_as_string("nick_name", member.nick_name_);
                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_collection(coll_message.get());
                    members_array->push_back(val.get());
                }
            }

            coll.set_value_as_array("members", members_array.get());
            g_core->post_message_to_gui("chats/blocked/result", _seq, coll.get());
        }
    };
}

void im::get_chat_pending(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::get_chat_pending>(make_wim_params(), _aimid);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);

            ifptr<iarray> members_array(coll->create_array());

            if (!packet->result_.empty())
            {
                members_array->reserve((int32_t)packet->result_.size());
                for (const auto member : packet->result_)
                {
                    coll_helper coll_message(coll->create_collection(), true);
                    coll_message.set_value_as_string("aimid", member.aimid_);
                    coll_message.set_value_as_string("first_name", member.first_name_);
                    coll_message.set_value_as_string("last_name", member.last_name_);
                    coll_message.set_value_as_string("nick_name", member.nick_name_);
                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_collection(coll_message.get());
                    members_array->push_back(val.get());
                }
            }

            coll.set_value_as_array("members", members_array.get());
            g_core->post_message_to_gui("chats/pending/result", _seq, coll.get());
        }
    };
}

void im::resolve_pending(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _contacts, bool _approve)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::resolve_pending>(make_wim_params(), _aimid, _contacts, _approve);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/pending/resolve/result", _seq, coll.get());
    };
}

void im::create_chat(int64_t _seq, const std::string& _aimid, const std::string& _name, const std::vector<std::string>& _members, core::wim::chat_params *&_params)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::create_chat>(make_wim_params(), _aimid, _name, _members);
    packet->set_chat_params(_params);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/create/result", _seq, coll.get());
        
        if (_error == 0)
        {
            core::stats::event_props_type props;
            auto members_count = packet->members_count();
            props.push_back(std::make_pair("Groupchat_Create_MembersCount", std::to_string(members_count)));
            g_core->insert_event(stats::stats_event_names::chats_created, props);
        }
    };
}

void im::mod_chat_params(int64_t _seq, const std::string& _aimid, core::wim::chat_params *&_params)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->set_chat_params(_params);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/params/result", _seq, coll.get());
    };
}

void im::mod_chat_name(int64_t _seq, const std::string& _aimid, const std::string& _name)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_name(_name);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/name/result", _seq, coll.get());
    };
}

void im::mod_chat_about(int64_t _seq, const std::string& _aimid, const std::string& _about)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_about(_about);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/about/result", _seq, coll.get());
    };
}

void im::mod_chat_public(int64_t _seq, const std::string& _aimid, bool _public)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_public(_public);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/public/result", _seq, coll.get());
    };
}

void im::mod_chat_join(int64_t _seq, const std::string& _aimid, bool _approved)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_join(_approved);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/public/result", _seq, coll.get());
    };
}

void im::mod_chat_link(int64_t _seq, const std::string& _aimid, bool _link)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_joiningByLink(_link);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/link/result", _seq, coll.get());
    };
}

void im::mod_chat_ro(int64_t _seq, const std::string& _aimid, bool _ro)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_readOnly(_ro);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/ro/result", _seq, coll.get());
    };
}

void im::mod_chat_age(int64_t _seq, const std::string& _aimid, bool _age)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_alpha>(make_wim_params(), _aimid);
    packet->get_chat_params()->set_ageGate(_age);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/mod/age/result", _seq, coll.get());
    };
}

void im::block_chat_member(int64_t _seq, const std::string& _aimid, const std::string& _contact, bool _block)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::block_chat_member>(make_wim_params(), _aimid, _contact, _block);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/block/result", _seq, coll.get());
    };
}

void im::set_chat_member_role(int64_t _seq, const std::string& _aimid, const std::string& _contact, const std::string& _role)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::mod_chat_member_alpha>(make_wim_params(), _aimid, _contact, _role);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("chats/role/set/result", _seq, coll.get());
    };
}

void im::get_chat_home(int64_t _seq, const std::string& _tag)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::get_chat_home>(make_wim_params(), _tag);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_string("new_tag", packet->result_tag_);
            coll.set_value_as_bool("need_restart", packet->need_restart_);
            coll.set_value_as_bool("finished", packet->finished_);
            ifptr<iarray> chats_array(coll->create_array());
            if (!packet->result_.empty())
            {
                chats_array->reserve((int32_t)packet->result_.size());
                for (const auto chat : packet->result_)
                {
                    coll_helper coll_message(coll->create_collection(), true);
                    chat.serialize(coll_message);
                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_collection(coll_message.get());
                    chats_array->push_back(val.get());
                }
            }
            coll.set_value_as_array("chats", chats_array.get());
            g_core->post_message_to_gui("chats/home/get/result", _seq, coll.get());
        }
        else
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_int("error", _error);
            g_core->post_message_to_gui("chats/home/get/failed", _seq, coll.get());
        }
    };
}

void im::save_my_info()
{
    if (my_info_cache_ && my_info_cache_->is_changed())
        my_info_cache_->save(get_my_info_file_name());
}

void im::save_contact_list()
{
    if (!contact_list_->is_changed())
        return;

    std::wstring contact_list_file = get_contactlist_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    contact_list_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    contact_list_->set_changed(false);

    auto handler = async_tasks_->run_async_function([contact_list_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(contact_list_file))
            return -1;

        return 0;
    });
}

void im::save_active_dialogs()
{
    if (!active_dialogs_->is_changed())
        return;

    std::wstring active_dialogs_file = get_active_dilaogs_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    active_dialogs_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    active_dialogs_->set_changed(false);

    auto handler = async_tasks_->run_async_function([active_dialogs_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(active_dialogs_file))
            return -1;

        return 0;
    });
}

void im::save_favorites()
{
    if (!favorites_->is_changed())
        return;

    std::wstring favorites_file = get_favorites_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    favorites_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    favorites_->set_changed(false);

    auto handler = async_tasks_->run_async_function([favorites_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(favorites_file))
            return -1;

        return 0;
    });
}

void im::save_mailboxes()
{
    if (!mailbox_storage_)
        return;

    mailbox_storage_->save(get_mailboxes_file_name());
}

void im::save_snaps_storage()
{
    if (!snaps_storage_->is_changed())
        return;

    std::wstring snaps_file = get_snaps_storage_filename();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    snaps_storage_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    snaps_storage_->set_changed(false);

    auto handler = async_tasks_->run_async_function([snaps_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(snaps_file))
            return -1;

        return 0;
    });
}

void im::save_cached_objects()
{
    save_my_info();
    save_contact_list();
    save_active_dialogs();
    save_favorites();
    save_mailboxes();
    save_snaps_storage();
}

void im::start_session(bool _is_ping)
{
    wim_send_thread_->clear();

    cancel_requests();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    std::string locale = g_core->get_locale();
    
    auto current_time = std::chrono::system_clock::now();
    auto timeout = (current_time < (start_session_time_ + std::chrono::milliseconds(start_session_timeout))) ? start_session_timeout : 0;
    start_session_time_ = current_time;
    auto packet = std::make_shared<core::wim::start_session>(make_wim_params(), _is_ping, g_core->get_uniq_device_id(), locale, timeout, std::bind(&im::wait_function, this, std::placeholders::_1));

    post_wim_packet(packet)->on_result_ = [wr_this, packet, _is_ping, locale](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->check_for_change_hosts_scheme(_error);

        if (_error == 0)
        {
            time_t time_offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - packet->get_ts();

            ptr_this->auth_params_->aimsid_ = packet->get_aimsid();
            ptr_this->auth_params_->time_offset_ = time_offset;
            ptr_this->auth_params_->aimid_ = packet->get_aimid();
            ptr_this->auth_params_->version_ = tools::version_info().get_version();
            ptr_this->auth_params_->locale_ = locale;

            ptr_this->fetch_params_->fetch_url_ = packet->get_fetch_url();

            im_login_id login(ptr_this->auth_params_->aimid_);
            g_core->update_login(login);
            ptr_this->set_id(login.get_id());

            ptr_this->store_auth_parameters();
            ptr_this->store_fetch_parameters();

            if (_is_ping)
            {
                ptr_this->poll(true, false);
                return;
            }

            ptr_this->load_favorites()->on_result_ = [wr_this](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (ptr_this->snaps_storage_->is_empty())
                {
                    ptr_this->refresh_snaps_storage();
                    ptr_this->poll(true, false);
                }
                else
                {
                    ptr_this->poll(true, false);
                }
            };

            ptr_this->apply_exported_settings();
        }
        else
        {
            if (_error != wim_protocol_internal_error::wpie_error_task_canceled)
            {
                if (packet->need_relogin())
                {
                    if (_is_ping)
                    {
                        ptr_this->start_session(false);
                    }
                    else
                    {
                        g_core->unlogin(true);
                    }
                }
                else if (packet->need_correct_ts())
                {
                    if (packet->get_ts() > 0)
                    {
                        time_t time_offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - packet->get_ts();
                        ptr_this->auth_params_->time_offset_ = time_offset;
                        ptr_this->store_auth_parameters();

                        ptr_this->start_session(_is_ping);
                    }
                    else
                    {
                        g_core->unlogin(false);
                    }
                }
                else
                {
                    ptr_this->start_session(_is_ping);
                }
            }
        }
    };
}

void im::cancel_requests()
{
    std::lock_guard<std::mutex> lock(stop_objects_->stop_mutex_);
    ++stop_objects_->active_session_id_;
}

bool im::is_session_valid(uint64_t _session_id)
{
    std::lock_guard<std::mutex> lock(stop_objects_->stop_mutex_);
    return (_session_id == stop_objects_->active_session_id_);
}

void im::post_pending_messages(const bool _recurcive)
{
    if (sent_pending_messages_active_ && !_recurcive)
        return;

    std::weak_ptr<im> wr_this = shared_from_this();

    sent_pending_messages_active_ = true;

    // stop post timer
    if (post_messages_timer_ != empty_timer_id)
    {
        g_core->stop_timer(post_messages_timer_);

        // reset timer id
        post_messages_timer_ = empty_timer_id;
    }

    // get first pending message grom queue
    get_archive()->get_pending_message()->on_result = [wr_this](archive::not_sent_message_sptr _message)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        // if message queue empty
        if (!_message)
        {
            ptr_this->sent_pending_messages_active_ = false;

            return;
        }

        auto current_time = std::chrono::system_clock::now();

        // if last post message time < 1 sec
        if (current_time - _message->get_post_time() < std::chrono::milliseconds(post_messages_rate))
        {
            // start timer
            assert(ptr_this->post_messages_timer_ == empty_timer_id);

            ptr_this->post_messages_timer_ = g_core->add_timer([wr_this]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->post_pending_messages(true);

            }, post_messages_rate);

            return;
        }

        ptr_this->get_archive()->update_message_post_time(_message->get_internal_id(), current_time)->on_result_ = [wr_this, _message](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->send_pending_message_async(0, _message)->on_result_ = [wr_this](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == 0)
                {
                    ptr_this->post_pending_messages(true);
                }
                else
                {
                    ptr_this->sent_pending_messages_active_ = false;
                }
            };
        };
    };
}

void im::check_for_change_hosts_scheme(int32_t _error)
{
    auto current_time = std::chrono::system_clock::now();

    const auto change_scheme_timeout = build::is_debug() ? std::chrono::minutes(5) : std::chrono::minutes(15);
    const auto check_alt_scheme_reset_timeout = build::is_debug() ? std::chrono::seconds(20) : std::chrono::minutes(60);

    if (g_core->get_hosts_config().is_alt_scheme())
    {
        if ((current_time - last_check_alt_scheme_reset_) > check_alt_scheme_reset_timeout)
        {
            g_core->get_hosts_config().change_scheme();

            last_success_network_post_ = current_time - change_scheme_timeout;
            last_check_alt_scheme_reset_ = current_time;

            return;
        }
    }

    if (_error == wpie_network_error)
    {
        if ((current_time - last_success_network_post_) > change_scheme_timeout)
        {
            g_core->get_hosts_config().change_scheme();
        }
    }
    else
    {
        last_success_network_post_ = current_time;
    }
}

void im::poll(bool _is_first, bool _after_network_error, int32_t _failed_network_error_count)
{
    if (!im_created_)
        on_im_created();

    if (!fetch_params_->is_valid())
        return start_session();

    const int32_t timeout = 60*1000;

    auto active_session_id = stop_objects_->active_session_id_;

    auto packet = std::make_shared<fetch>(make_wim_params(), fetch_params_->fetch_url_, 
        ((_is_first || _after_network_error) ? 1 : timeout), fetch_params_->next_fetch_time_, std::bind(&im::wait_function, this, std::placeholders::_1));

    std::weak_ptr<im> wr_this = shared_from_this();

    fetch_thread_->run_async_task(packet)->on_result_ = [_is_first, _after_network_error, active_session_id, packet, wr_this, _failed_network_error_count](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->check_for_change_hosts_scheme(_error);

        ptr_this->fetch_params_->next_fetch_time_ = packet->get_next_fetch_time();

        if (_error == 0)
        {
            ptr_this->dispatch_events(packet,[packet, wr_this, active_session_id, _is_first](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (packet->is_session_ended())
                    return;

                auto time_offset = packet->get_time_offset();
                auto time_offset_prev = ptr_this->auth_params_->time_offset_;

                ptr_this->fetch_params_->fetch_url_ = packet->get_next_fetch_url();
                ptr_this->fetch_params_->last_successful_fetch_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + time_offset;

                ptr_this->check_need_agregate_dlg_state();
                ptr_this->last_network_activity_time_ = std::chrono::system_clock::now();

                ptr_this->store_fetch_parameters();

                ptr_this->auth_params_->time_offset_ = time_offset;

                if (std::abs(time_offset - time_offset_prev) > 5*60)
                {
                    ptr_this->store_auth_parameters();
                }

                if (!ptr_this->is_session_valid(active_session_id))
                    return;

                ptr_this->poll(false, false);

                ptr_this->resume_failed_network_requests();

                if (_is_first)
                {
                    g_core->post_message_to_gui("login/complete", 0, 0);

                    ptr_this->send_timezone();
                }
            });
        }
        else
        {
            if (!ptr_this->is_session_valid(active_session_id))
                return;

            if (_error == wpie_network_error)
            {
                if (_failed_network_error_count > 2)
                {
                    if (_is_first && g_core->try_to_apply_alternative_settings())
                        ptr_this->poll(_is_first, true, 0);
                    else
                        ptr_this->start_session(true);
                }
                else
                    ptr_this->poll(_is_first, true, (_failed_network_error_count + 1));
            }
            else
            {
                ptr_this->start_session();
            }
        }
    };
}

void im::dispatch_events(std::shared_ptr<fetch> _fetch_packet, std::function<void(int32_t)> _on_complete)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    auto evt = _fetch_packet->pop_event();

    if (!evt)
    {
        _on_complete(0);

        return;
    }

    evt->on_im(shared_from_this(), std::make_shared<auto_callback>([_on_complete, wr_this, _fetch_packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->dispatch_events(_fetch_packet, _on_complete);
    }));
}


void im::logout(std::function<void()> _on_result)
{
    auto packet = std::make_shared<core::wim::end_session>(make_wim_params());

    post_wim_packet(packet)->on_result_ = [packet, _on_result](int32_t _error)
    {
        _on_result();
    };
}

void im::on_login_result(int64_t _seq, int32_t err, bool _from_exported_data)
{
    coll_helper cl_coll(g_core->create_collection(), true);

    core::login_error error_code = core::login_error::le_network_error;

    if (err != 0)
    {
        cl_coll.set_value_as_bool("result", false);

        switch ((wim_protocol_internal_error) err)
        {
            case wpie_wrong_login:
            case wpie_wrong_login_2x_factor:
            {
                error_code = core::login_error::le_wrong_login;
                if (wpie_wrong_login_2x_factor == (wim_protocol_internal_error) err)
                    error_code = core::login_error::le_wrong_login_2x_factor;

                if (_from_exported_data)
                {
                    coll_helper coll(g_core->create_collection(), true);

                    coll.set_value_as_bool("is_auth_error", true);

                    g_core->post_message_to_gui("need_login", 0, coll.get());

                    return;
                }

                break;
            }
            default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        cl_coll.set_value_as_int("error", (int32_t) error_code);
    }
    else
    {
        cl_coll.set_value_as_bool("result", true);
    }

    g_core->post_message_to_gui("login_result", _seq, cl_coll.get());
}

void im::on_login_result_attach_uin(int64_t _seq, int32_t err, const auth_parameters& auth_params, const wim_packet_params& _from_params)
{
    coll_helper cl_coll(g_core->create_collection(), true);

    core::login_error error_code = core::login_error::le_network_error;

    if (err != 0)
    {
        cl_coll.set_value_as_bool("result", false);

        switch ((wim_protocol_internal_error) err)
        {
        case wpie_wrong_login:
            {
                error_code = core::login_error::le_wrong_login;
                break;
            }
        case wpie_error_attach_busy_phone:
            {
                error_code = core::login_error::le_attach_error_busy_phone;
                break;
            }
        default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        cl_coll.set_value_as_int("error", (int32_t) error_code);
        g_core->post_message_to_gui("login_result_attach_uin", _seq, cl_coll.get());
    }
    else
    {
        attach_uin(_seq);
    }
}

void im::attach_phone(int64_t _seq, const auth_parameters& auth_params, const phone_info& _info)
{
    auto packet = std::make_shared<core::wim::attach_phone>(make_wim_params(), _info);
    std::weak_ptr<im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        core::login_error error_code = core::login_error::le_network_error;
        switch ((wim_protocol_internal_error) _error)
        {
        case wpie_wrong_login:
            {
                error_code = core::login_error::le_wrong_login;
                break;
            }
        case wpie_error_attach_busy_phone:
            {
                error_code = core::login_error::le_attach_error_busy_phone;
                break;
            }
        default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        coll_helper coll(g_core->create_collection(), true);
        if (_error != 0)
        {
            coll.set_value_as_bool("result", false);
            coll.set_value_as_int("error", error_code);
        }
        else
        {
            ptr_this->attach_phone_finished();
            coll.set_value_as_bool("result", true);
        }
        g_core->post_message_to_gui("login_result_attach_phone", _seq, coll.get());
    };
}

void im::merge_account()
{
    std::wstring merge_file = get_auth_parameters_filename_merge();

    std::weak_ptr<im> wr_this = shared_from_this();

    auto merge_params = std::make_shared<auth_parameters>();

    async_tasks_->run_async_function([merge_file, merge_params]()->int32_t
    {
        core::tools::binary_stream bs_merge;
        if (!bs_merge.load_from_file(merge_file))
        {
            return -1;
        }

        bs_merge.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse(bs_merge.read_available()).HasParseError())
        {
            return -1;
        }

        if (!merge_params->unserialize(doc))
        {
            return -1;
        }

        return 0;

    })->on_result_ = [wr_this, merge_params, merge_file](int32_t _error)
    {
        if (_error != 0)
            return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        
        auto packet = std::make_shared<core::wim::merge_account>(
            ptr_this->make_wim_params(), 
            ptr_this->make_wim_params_from_auth(*merge_params));

        g_core->insert_event(core::stats::stats_event_names::merge_accounts);

        ptr_this->post_wim_packet(packet)->on_result_ = [packet, wr_this, merge_file](int32_t _error)
        {
            if (_error != wim_protocol_internal_error::wpie_network_error)
            {
                tools::system::delete_file(merge_file);
            }
        };
    };
}


void im::apply_exported_settings()
{
    merge_account();

    apply_exported_muted_chats();
}

void im::apply_exported_muted_chats_internal()
{
    std::wstring merge_file = get_exported_muted_chats_filename();

    std::weak_ptr<im> wr_this = shared_from_this();

    auto merge_params = std::make_shared<auth_parameters>();

    auto chats_list = std::make_shared<std::list<std::string>>();

    async_tasks_->run_async_function([merge_file, merge_params, chats_list]()->int32_t
    {
        core::tools::binary_stream bs_merge;

        if (!bs_merge.load_from_file(merge_file))
        {
            return -1;
        }

        bs_merge.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse(bs_merge.read_available()).HasParseError() || !doc.IsArray())
        {
            return -1;
        }

        for (auto iter = doc.Begin(); iter != doc.End(); ++iter)
        {
            if (!iter->IsString())
                continue;

            chats_list->push_back(iter->GetString());
        }

        return 0;

    })->on_result_ = [wr_this, merge_params, merge_file, chats_list](int32_t _error)
    {
        if (!chats_list->size())
            return;

        if (_error != 0)
            return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->mute_chats(chats_list);
    };
}

void im::apply_exported_muted_chats()
{
    if (mute_chats_timer_ != 0)
        return;

    std::weak_ptr<im> wr_this = shared_from_this();

    mute_chats_timer_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->apply_exported_muted_chats_internal();

        g_core->stop_timer(ptr_this->mute_chats_timer_);

        ptr_this->mute_chats_timer_ = 0;

    }, 2000);
}

void im::attach_uin(int64_t _seq)
{
    cancel_requests();

    auto packet = std::make_shared<core::wim::attach_uin>(make_wim_params(), make_wim_params_general(false));
    std::weak_ptr<im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [packet, wr_this, _seq](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->attach_uin_finished();
        }

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("result", _error == 0);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("login_result_attach_uin", _seq, coll.get());
    };
    //g_core->post_message_to_gui("login_result_connect", 0, cl_coll.get());
}

void im::attach_uin_finished()
{
    im_login_id old_login(auth_params_->aimid_);
    old_login.set_id(get_id());
    im_login_id new_login(attached_auth_params_->aimid_);
    new_login.set_id(get_id());
    g_core->replace_uin_in_login(old_login, new_login);
    auth_params_ = attached_auth_params_;
}

void im::attach_phone_finished()
{
    phone_registration_data_ = attached_phone_registration_data_;
}

std::string im::get_contact_friendly_name(const std::string& contact_login) {
    assert(!!contact_list_);
    if (!!contact_list_) {
        return contact_list_->get_contact_friendly_name(contact_login);
    }
    return "";
}

void im::post_contact_list_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    contact_list_->serialize(cl_coll.get(), std::string());

    g_core->post_message_to_gui("contactlist", 0, cl_coll.get());

    core::stats::event_props_type props;

    int32_t group_count = contact_list_->get_groupchat_contacts_count();
    int32_t phone_count = contact_list_->get_phone_contacts_count();
    int32_t people_count = contact_list_->get_contacts_count() - group_count - phone_count;

    props.emplace_back("CL_Count", std::to_string(people_count));
    props.emplace_back("CL_Count_Groupchats", std::to_string(group_count));
    props.emplace_back("CL_Count_Phone", std::to_string(phone_count));
    g_core->insert_event(core::stats::stats_event_names::cl_load, props);
}


void im::post_ignorelist_to_gui(int64_t _seq)
{
    coll_helper coll(g_core->create_collection(), true);

    contact_list_->serialize_ignorelist(coll.get());

    g_core->post_message_to_gui("contacts/get_ignore/result", 0, coll.get());
}

void im::post_my_info_to_gui()
{
    coll_helper info_coll(g_core->create_collection(), true);
    my_info_cache_->get_info()->serialize(info_coll);

    g_core->post_message_to_gui("my_info", 0, info_coll.get());
}


void im::post_active_dialogs_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    active_dialogs_->serialize(cl_coll.get());

    g_core->post_message_to_gui("active_dialogs", 0, cl_coll.get());
}

void im::post_active_dialogs_are_empty_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    //
    g_core->post_message_to_gui("active_dialogs_are_empty", 0, cl_coll.get());
}

void im::on_event_buddies_list(fetch_event_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto contact_list = _event->get_contactlist();

    if (contact_list)
    {
        std::weak_ptr<im> wr_this(shared_from_this());

        get_archive()->sync_with_history()->on_result_ = [contact_list, wr_this, _on_complete](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->contact_list_->update_cl(*contact_list);
            ptr_this->need_update_search_cache();

            ptr_this->post_contact_list_to_gui();

            _on_complete->callback(_error);
        };
    }
}

void im::on_event_diff(fetch_event_diff* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto diff = _event->get_diff();

    std::weak_ptr<im> wr_this(shared_from_this());

    get_archive()->sync_with_history()->on_result_ = [diff, wr_this, _on_complete](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        for (auto iter = diff->begin(); iter != diff->end(); ++iter)
        {
            auto removed = std::make_shared<std::list<std::string>>();
            ptr_this->contact_list_->merge_from_diff(iter->first, iter->second, removed);
            ptr_this->need_update_search_cache();

            if (iter->first == "deleted" && removed->empty())
                continue;

            for (auto removedIter : *removed)
            {
                std::string contact = removedIter;

                ptr_this->active_dialogs_->remove(contact);
                ptr_this->unfavorite(contact);

                ptr_this->get_archive()->clear_dlg_state(contact)->on_result_ = [contact](int32_t _err)
                {
                    coll_helper cl_coll(g_core->create_collection(), true);
                    cl_coll.set_value_as_string("contact", contact);
                    g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
                };
            }

            ifptr<icollection> cl_coll(g_core->create_collection(), true);
            iter->second->serialize(cl_coll.get(), iter->first);
            g_core->post_message_to_gui("contactlist/diff", 0, cl_coll.get());
        }

        on_created_groupchat(diff);

        _on_complete->callback(_error);
    };
}

void im::on_created_groupchat(std::shared_ptr<diffs_map> _diff)
{
    for (auto iter = _diff->begin(); iter != _diff->end(); ++iter)
    {
        if (iter->first == "created")
        {
            auto group = iter->second->get_first_group();

            if (!group)
            {
                assert(false);
                continue;
            }

            if (!group->buddies_.empty())
            {
                auto buddy = *(group->buddies_.begin());

                if (!!buddy)
                {
                    coll_helper created_chat_coll(g_core->create_collection(), true);

                    std::string aimid = buddy.get()->aimid_;
                    created_chat_coll.set_value_as_string("aimId", aimid);
                    g_core->post_message_to_gui("open_created_chat", 0, created_chat_coll.get());

                    return;
                }
            }
        }
    }
}

void im::on_event_my_info(fetch_event_my_info* _event, std::shared_ptr<auto_callback> _on_complete)
{
    my_info_cache_->set_info(_event->get_info());
    post_my_info_to_gui();

    _on_complete->callback(0);
}

void im::on_event_user_added_to_buddy_list(fetch_event_user_added_to_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete)
{
    _on_complete->callback(0);
}

void im::on_event_typing(fetch_event_typing *_event, std::shared_ptr<auto_callback> _on_complete)
{
    if (!_event->aim_id().length())
    {
        _on_complete->callback(0);
        return;
    }

    coll_helper coll(g_core->create_collection(), true);
    coll.set_value_as_string("aimId", _event->aim_id());
    coll.set_value_as_string("chatterAimId", _event->chatter_aim_id());
    coll.set_value_as_string("chatterName", _event->chatter_name());
    if (_event->is_typing())
        g_core->post_message_to_gui("typing", 0, coll.get());
    else
        g_core->post_message_to_gui("typing/stop", 0, coll.get());

    _on_complete->callback(0);
}

void im::on_event_presence(fetch_event_presence* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto presence = _event->get_presence();
    auto aimid = _event->get_aimid();

    coll_helper cl_coll(g_core->create_collection(), true);
    cl_coll.set_value_as_string("aimId", aimid);
    presence->serialize(cl_coll.get());
    g_core->post_message_to_gui("contact_presence", 0, cl_coll.get());

    contact_list_->update_presence(aimid, presence);
    if (contact_list_->get_need_update_avatar(true))
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_string("aimid", aimid);
        g_core->post_message_to_gui("avatars/presence/updated", 0, coll.get());
    }
    
    _on_complete->callback(0);
}

std::shared_ptr<async_task_handlers> im::get_robusto_token()
{
    robusto_threads_->set_robusto_token_in_process(true);

    auto out_handlers = std::make_shared<async_task_handlers>();

    std::weak_ptr<im> wr_this = shared_from_this();
    auto packet_gen_token = std::make_shared<core::wim::gen_robusto_token>(make_wim_params());
    robusto_threads_->run_async_task(packet_gen_token)->on_result_ =
        [wr_this, out_handlers, packet_gen_token](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->robusto_token_ = packet_gen_token->get_token();

            auto add_client_packet = std::make_shared<core::wim::robusto_add_client>(ptr_this->make_wim_params());
            add_client_packet->set_robusto_params(ptr_this->make_robusto_params());

            ptr_this->robusto_threads_->run_async_task(add_client_packet)->on_result_ =
                [wr_this, out_handlers, add_client_packet](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->robusto_threads_->set_robusto_token_in_process(false);

                if (_error == 0)
                {
                    ptr_this->auth_params_->robusto_client_id_ = add_client_packet->get_client_id();
                    ptr_this->store_auth_parameters();
                }


                if (out_handlers->on_result_)
                    out_handlers->on_result_(_error);
            };
        }
        else
        {
            ptr_this->robusto_threads_->set_robusto_token_in_process(false);

            if (out_handlers->on_result_)
                out_handlers->on_result_(_error);
        }
    };

    return out_handlers;
}

void im::post_robusto_packet_to_server(std::shared_ptr<async_task_handlers> _handlers, std::shared_ptr<robusto_packet> _packet, uint32_t _recursion_count)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    // replace auth params
    _packet->set_robusto_params(make_robusto_params());

    robusto_threads_->run_async_task(_packet)->on_result_ = [_handlers, wr_this, _recursion_count, _packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            if (_handlers->on_result_)
                _handlers->on_result_(0);
        }
        else
        {
            if (_error == wpie_error_robusto_token_invalid)
            {
                if (ptr_this->robusto_threads_->is_get_robusto_token_in_process())
                {
                    ptr_this->robusto_threads_->push_packet_to_queue(_packet, _handlers);
                }
                else
                {
                    ptr_this->auth_params_->reset_robusto();
                    ptr_this->post_robusto_packet_internal(_packet, _handlers, _recursion_count + 1, _error);
                }
            }
            else
            {
                if (_handlers->on_result_)
                    _handlers->on_result_(_error);
            }
        }
    };
};

std::shared_ptr<async_task_handlers> im::post_robusto_packet_internal(
    std::shared_ptr<robusto_packet> _packet,
    std::shared_ptr<async_task_handlers> _handlers,
    uint32_t _recursion_count,
    int32_t _last_error)
{
    if (_recursion_count > 5)
    {
        if (_handlers->on_result_)
            _handlers->on_result_(_last_error);

        return _handlers;
    }

    if (!_handlers)
        _handlers = std::make_shared<async_task_handlers>();

    if (robusto_threads_->is_get_robusto_token_in_process())
    {
        robusto_threads_->push_packet_to_queue(_packet, _handlers);
        return _handlers;
    }

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    if (!auth_params_->is_robusto_valid())
    {
        get_robusto_token()->on_result_ = [_handlers, wr_this, _recursion_count, _packet](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0)
            {
                ptr_this->post_robusto_packet_to_server(_handlers, _packet, _recursion_count);

                while ( auto packet_and_handler = ptr_this->robusto_threads_->get_front_packet_from_queue() )
                {
                    ptr_this->post_robusto_packet_to_server(packet_and_handler->handlers_, packet_and_handler->packet_, _recursion_count + 1);
                }
            }
            else
            {
                if (_handlers->on_result_)
                    _handlers->on_result_(_error);
            }
        };
    }
    else
    {
        post_robusto_packet_to_server(_handlers, _packet, _recursion_count);
    }

    return _handlers;
}

std::shared_ptr<async_task_handlers> im::post_robusto_packet(std::shared_ptr<robusto_packet> _packet)
{
    return post_robusto_packet_internal(_packet, nullptr, 0, 0);
}

std::shared_ptr<archive::face> im::get_archive()
{
    if (!archive_)
        archive_.reset(new archive::face(get_im_data_path() + L"/" + L"archive"));

    return archive_;
}

void im::get_archive_images(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    get_archive()->get_images(_contact, _from, _count)->on_result =
        [wr_this, _seq](std::shared_ptr<archive::image_list> _images)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);

        ifptr<iarray> array(coll->create_array());
        array->reserve(static_cast<int32_t>(_images->size()));
        for (const auto& image : *_images)
        {
            coll_helper data(coll->create_collection(), true);
            data.set_value_as_int64("msgid", image.get_msgid());
            data.set_value_as_string("url", image.get_url());
            data.set_value_as_bool("is_filesharing", image.get_is_filesharing());

            ifptr<ivalue> val(coll->create_value());
            val->set_as_collection(data.get());

            array->push_back(val.get());
        }

        coll.set_value_as_array("images", array.get());

        g_core->post_message_to_gui("archive/images/get/result", _seq, coll.get());
    };
}

void im::repair_archive_images(int64_t _seq, const std::string& _contact)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    get_archive()->repair_images(_contact);
}

void im::get_archive_index(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, int32_t _recursion, std::function<void(int64_t)> last_message_catcher)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    get_archive()->get_dlg_state(_contact)->on_result =
        [wr_this, _seq, _contact, _from, _count, _recursion, last_message_catcher]
        (const archive::dlg_state &_dlg_state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            // load from local storage
            ptr_this->get_archive()->get_messages_index(_contact, _from, _count)->on_result =
                [_seq, wr_this, _contact, _recursion, _dlg_state, last_message_catcher](std::shared_ptr<archive::headers_list> _headers)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    // temporary always request history
                    if (_headers->empty())
                    {
                        const get_history_params params(_contact, -1, -1, -1, _dlg_state.get_history_patch_version("init"));

                        if (_recursion == 0)
                        {
                            // download from server and push to local storage
                            ptr_this->get_history_from_server(params, last_message_catcher)->on_result_ =
                                [_seq, wr_this, _contact](int32_t _error)
                            {
                            };

                            return;
                        }
                    }
                    else
                    {
                        coll_helper cl_coll(g_core->create_collection(), true);
                        archive::face::serialize(_headers, cl_coll);
                        cl_coll.set_value_as_string("contact", _contact);
                        g_core->post_message_to_gui("contact_history", _seq, cl_coll.get());
                    }

                    ptr_this->download_holes(_contact, last_message_catcher);
                }; // get_messages_index
        }; // get_dlg_state
}


void im::get_archive_index(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, std::function<void(int64_t)> last_message_catcher)
{
    get_archive_index(_seq, _contact, _from, _count, 0, last_message_catcher);
}


void serialize_messages_4_gui(const std::string& _value_name, std::shared_ptr<archive::history_block> _messages, icollection* _collection, const time_t _offset)
{
    coll_helper coll(_collection, false);
    ifptr<iarray> messages_array(coll->create_array());
    ifptr<iarray> deleted_array(coll->create_array());
    ifptr<iarray> modified_array(coll->create_array());

    if (!_messages->empty())
    {
        messages_array->reserve((int32_t)_messages->size());

        for (const auto &message : *_messages)
        {
            if (message->is_patch())
            {
                if (message->is_deleted())
                {
                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_int64(message->get_msgid());
                    deleted_array->push_back(val.get());
                }

                if (message->is_modified())
                {
                    coll_helper coll_modification(coll->create_collection(), true);
                    message->serialize(coll_modification.get(), _offset);

                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_collection(coll_modification.get());

                    modified_array->push_back(val.get());
                }

                continue;
            }

            __LOG(core::log::info("archive", boost::format("serialize message for gui, %1%") % message->get_text());)

                coll_helper coll_message(coll->create_collection(), true);
            message->serialize(coll_message.get(), _offset);
            ifptr<ivalue> val(coll->create_value());
            val->set_as_collection(coll_message.get());
            messages_array->push_back(val.get());
        }
    }

    coll.set_value_as_array(_value_name, messages_array.get());

    if (!deleted_array->empty())
    {
        coll.set_value_as_array("deleted", deleted_array.get());
    }

    if (!modified_array->empty())
    {
        coll.set_value_as_array("modified", modified_array.get());
    }
}


void im::get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, int32_t _recursion, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher)
{
    assert(!_contact.empty());
    assert(_from >= -1);
    /// assert(_count > 0);

    if (_contact.empty())
        return;

    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    get_archive_messages_get_messages(_seq, _contact, _from, _count_early, _count_later, _recursion, _need_prefetch, last_message_catcher)->on_result_ =
        [_seq, wr_this, _contact, _recursion, _from, _count_early, last_message_catcher]
        (int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            const auto is_first_request = (_from == -1);

            const auto skip_history_request = (
                !is_first_request ||
                !core::configuration::get_app_config().is_server_history_enabled_);

            if (skip_history_request)
            {
                return;
            }

            ptr_this->get_archive()->get_dlg_state(_contact)->on_result = [_seq, wr_this, _contact, _recursion, _from, _count_early, last_message_catcher](const archive::dlg_state& _state)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                const get_history_params hist_params(_contact, -1, -1, -1, _state.get_history_patch_version("init"), true);

                ptr_this->get_history_from_server(hist_params, last_message_catcher)->on_result_ =
                    [wr_this, _seq, _contact, _from, _count_early, _recursion, last_message_catcher](int32_t error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_holes(_contact, last_message_catcher);
                };
            };
        };
}

std::shared_ptr<async_task_handlers> im::get_archive_messages_get_messages(int64_t _seq, const std::string& _contact
                                                                           , int64_t _from, int64_t _count_early, int64_t _count_later, int32_t _recursion, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher)
{
    auto out_handler = std::make_shared<async_task_handlers>();
    const auto auto_handler = std::make_shared<tools::auto_scope>([out_handler] {out_handler->on_result_(0);});

    const auto is_first_request = (_from == -1);

    std::weak_ptr<im> wr_this = shared_from_this();

    // load from local storage

    /// const auto count_with_prefetch = _count_early + (_need_prefetch ? PREFETCH_COUNT : 0);

    get_archive()->get_messages(_contact, _from, _count_early, _count_later)->on_result =
        [_seq, wr_this, _contact, _recursion, _count_early, _count_later, is_first_request, auto_handler, last_message_catcher]
        (std::shared_ptr<archive::history_block> _messages)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

			for (auto val : *_messages)
			{
				last_message_catcher(val->get_msgid());
			}

            const auto messages_overflow = (_messages->size() > _count_early + _count_later);
            if (messages_overflow)
            {
                if (_count_early > 0)
                {
                    auto iter_to = _messages->end();
                    std::advance(iter_to, (int32_t)-_count_early);
                    _messages->erase(_messages->begin(), iter_to);
                }
                else if (_count_later > 0)
                {
                    auto iter_to = _messages->begin();
                    std::advance(iter_to, (int32_t)_count_later);
                    _messages->erase(iter_to, _messages->end());
                }
            }

            ptr_this->get_archive()->get_dlg_state(_contact)->on_result =
                [_seq, wr_this, _contact, _recursion, is_first_request, _messages, auto_handler]
                (const archive::dlg_state& _state)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                    {
                        return;
                    }

                    coll_helper coll(g_core->create_collection(), true);
                    coll.set<bool>("result", true);
                    coll.set<std::string>("contact", _contact);
                    serialize_messages_4_gui("messages", _messages, coll.get(), ptr_this->auth_params_->time_offset_);
                    coll.set<int64_t>("theirs_last_delivered", _state.get_theirs_last_delivered());
                    coll.set<int64_t>("theirs_last_read", _state.get_theirs_last_read());
                    coll.set<int64_t>("last_msg_in_index", _state.get_last_msgid());
                    coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);

                    const auto as = std::make_shared<tools::auto_scope>(
                        [coll, _seq]
                        {
                            g_core->post_message_to_gui("archive/messages/get/result", _seq, coll.get());
                        });

                    if (!is_first_request)
                    {
                        return;
                    }

                    ptr_this->get_archive()->get_not_sent_messages(_contact)->on_result =
                        [_seq, wr_this, _contact, as, coll, auto_handler]
                        (std::shared_ptr<archive::history_block> _pending_messages)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            if (!_pending_messages->empty())
                            {
                                serialize_messages_4_gui(
                                    "pending_messages",
                                    _pending_messages,
                                    coll.get(),
                                    ptr_this->auth_params_->time_offset_);
                            }
                        }; // get_not_sent_messages
                }; // get_dlg_state
        }; // get_messages

    return out_handler;
}

void im::get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, bool _need_prefetch, std::function<void(int64_t)> last_message_catcher)
{
	get_archive_messages(_seq, _contact, _from, _count_early, _count_later, 0, _need_prefetch, last_message_catcher);
}

void im::get_archive_messages_buddies(int64_t _seq, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids)
{
    __LOG(core::log::info("archive", boost::format("get messages buddies, contact=%1% messages count=%2%") % _contact % _ids->size());)

        std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_messages_buddies(_contact, _ids)->on_result = [_seq, wr_this, _contact, _ids](std::shared_ptr<archive::history_block> _messages)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_archive()->get_dlg_state(_contact)->on_result = [_seq, wr_this, _contact, _ids, _messages](const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_bool("result", _messages->size() == _ids->size() && !_ids->empty());
            coll.set_value_as_string("contact", _contact);
            serialize_messages_4_gui("messages", _messages, coll.get(), ptr_this->auth_params_->time_offset_);
            coll.set_value_as_int64("theirs_last_delivered", _state.get_theirs_last_delivered());
            coll.set_value_as_int64("theirs_last_read", _state.get_theirs_last_read());
            coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
            g_core->post_message_to_gui("archive/messages/get/result", _seq, coll.get());
        };
    };
}

std::shared_ptr<async_task_handlers> im::get_history_from_server(const get_history_params& _params, std::function<void(int64_t)> last_message_catcher)
{
    auto out_handler = std::make_shared<async_task_handlers>();
    auto packet = std::make_shared<core::wim::get_history>(make_wim_params(), _params, g_core->get_locale());

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::string contact = _params.aimid_;
    assert(!contact.empty());

    std::function<void(int32_t)> on_result =
        [out_handler](int32_t _error)
        {
            if (out_handler->on_result_)
                out_handler->on_result_(_error);
        };

    bool init = _params.init_;

    post_robusto_packet(packet)->on_result_ =
        [wr_this, packet, contact, init, on_result, last_message_catcher]
        (int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error != 0)
            {
                on_result(_error);
                return;
            }

            const auto dlg_state = packet->get_dlg_state();
            auto messages = packet->get_messages();

            __INFO(
                "delete_history",
                "processing incoming history from a server\n"
                "    contact=<%1%>\n"
                "    history-patch=<%2%>\n"
                "    messages-size=<%3%>",
                contact % dlg_state->get_history_patch_version() % messages->size());

            ptr_this->get_archive()->get_dlg_state(contact)->on_result = [wr_this, contact, messages, init, dlg_state, on_result, last_message_catcher](const archive::dlg_state& _state)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                dlg_state->set_official(_state.get_official());
                dlg_state->set_friendly(_state.get_friendly());

                ptr_this->get_archive()->set_dlg_state(contact, *dlg_state)->on_result =
                    [wr_this, contact, messages, init, on_result, last_message_catcher]
                    (const archive::dlg_state& _state, const archive::dlg_state_changes& _changes)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        if (messages->empty())
                        {
                            on_result(0);
                            return;
                        }

                        remove_messages_from_not_sent(ptr_this->get_archive(), contact, messages)->on_result_ =
                            [wr_this, contact, messages, init, on_result, _state, _changes, last_message_catcher]
                            (int32_t _error)
                            {
                                auto ptr_this = wr_this.lock();
                                if (!ptr_this)
                                {
                                    return;
                                }

                                const auto first_message = ptr_this->get_first_message(contact);
                                const auto is_dialog_empty = (first_message == -1);

                                auto actual_messages = std::make_shared<archive::history_block>();

                                std::copy_if(messages->begin(), messages->end(),
                                    std::back_inserter(*actual_messages),[first_message](std::shared_ptr<archive::history_message> item)
                                {
                                    return (item->get_msgid() > first_message);
                                });

                                const auto is_new_messages_arrived = !actual_messages->empty();
                                const auto is_dialog_opened = ptr_this->has_opened_dialogs(contact);

                                const auto post_messages_to_gui = (
                                    is_dialog_opened && (is_dialog_empty || is_new_messages_arrived)
                                );

                                if (post_messages_to_gui)
                                {
                                    coll_helper coll(g_core->create_collection(), true);
                                    coll.set<bool>("result", true);
                                    coll.set<std::string>("contact", contact);
                                    serialize_messages_4_gui("messages", actual_messages, coll.get(), ptr_this->auth_params_->time_offset_);
                                    coll.set<int64_t>("theirs_last_delivered", _state.get_theirs_last_delivered());
                                    coll.set<int64_t>("theirs_last_read", _state.get_theirs_last_read());
                                    coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
                                    g_core->post_message_to_gui(init ? "messages/received/init" : "messages/received/server", 0, coll.get());
                                }

                                ptr_this->get_archive()->update_history(contact, messages)->on_result =
                                    [wr_this, contact, on_result, _changes, messages, last_message_catcher]
                                    (std::shared_ptr<archive::headers_list> _inserted_messages, const archive::dlg_state&, const archive::dlg_state_changes& _changes_on_update)
                                    {
                                        for (auto val : *messages)
                                        {
                                            last_message_catcher(val->get_msgid());
                                        }

                                        auto ptr_this = wr_this.lock();
                                        if (!ptr_this)
                                            return;

                                        const auto last_message_changed = (_changes.last_message_changed_ && !_changes.initial_fill_);

                                        const auto last_message_changed_on_update = _changes_on_update.last_message_changed_;

                                        const auto skip_dlg_state = (!last_message_changed_on_update && !last_message_changed);
                                        if (skip_dlg_state)
                                        {
                                            on_result(0);
                                            return;
                                        }

                                        ptr_this->post_dlg_state_to_gui(contact)->on_result_ =
                                            [on_result](int32_t _error)
                                            {
                                                on_result(0);
                                            };
                                    };
                            }; // remove_messages_from_not_sent
                    }; // set_dlg_state
            }; //get_dlg_state
        }; // post_robusto_packet

    return out_handler;
}

std::shared_ptr<async_task_handlers> im::set_dlg_state(const set_dlg_state_params& _params)
{
    auto out_handler = std::make_shared<async_task_handlers>();

    auto packet = std::make_shared<core::wim::set_dlg_state>(make_wim_params(), _params);

    std::string contact = _params.aimid_;

    post_robusto_packet(packet)->on_result_ = [out_handler](int32_t _error)
    {
        if (out_handler->on_result_)
            out_handler->on_result_(_error);
    };

    return out_handler;
}

void im::download_failed_holes()
{
    while (auto req = failed_holes_requests_->get())
    {
        download_holes(
            req->get_contact(),
            req->get_from(),
            req->get_depth(),
            req->get_recursion());
    }
}

void im::download_holes(const std::string& _contact, std::function<void(int64_t)> last_message_catcher)
{
	im::download_holes(_contact, -1, last_message_catcher);
}

void im::download_holes(const std::string& _contact, int64_t _depth, std::function<void(int64_t)> last_message_catcher)
{
    assert(!_contact.empty());

    if (!core::configuration::get_app_config().is_server_history_enabled_)
    {
        return;
    }

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(_contact)->on_result =
        [wr_this, _contact, _depth, last_message_catcher](const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (!_state.has_last_msgid())
                return;

            ptr_this->download_holes(_contact, _state.get_last_msgid(), _depth, 0, last_message_catcher);
        };
}


void im::download_holes(const std::string& _contact, int64_t _from, int64_t _depth, int32_t _recursion, std::function<void(int64_t)> last_message_catcher)
{
    __INFO("archive", "im::download_holes, contact=%1%", _contact);

    std::weak_ptr<im> wr_this = shared_from_this();

    holes::request hole_request(_contact, _from, _depth, _recursion);

    get_archive()->get_next_hole(_contact, _from, _depth)->on_result =
    	[wr_this, _contact, _depth, _recursion, hole_request, last_message_catcher](std::shared_ptr<archive::archive_hole> _hole)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_hole)
            return;

        // fix for empty dialog
        if ((_recursion > 0) && (_hole->get_from() == -1))
            return;

        // on the off-chance
        const auto safety_limit_reached = (_recursion > 100);
        if (safety_limit_reached)
        {
            assert(!"archive logic bug");
            return;
        }

        ptr_this->get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _hole, _contact, _depth, _recursion, hole_request, last_message_catcher](const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            const auto is_from_obsolete = (_hole->has_from() && (_hole->get_from() <= _state.get_del_up_to()));
            const auto is_to_obsolete = (_hole->has_to() && (_hole->get_to() <= _state.get_del_up_to()));

            const auto is_request_obsolete = (is_from_obsolete || is_to_obsolete);
            if (is_request_obsolete)
            {
                __INFO(
                    "delete_history",
                    "obsolete holes request cancelled\n"
                    "    from=<%1%>\n"
                    "    from_obsolete=<%2%>\n"
                    "    to=<%3%>\n"
                    "    to_obsolete=<%4%>\n"
                    "    del_up_to=<%5%>",
                    _hole->get_from() % logutils::yn(is_from_obsolete) %
                    _hole->get_to() % logutils::yn(is_to_obsolete) %
                    _state.get_del_up_to()
                );

                return;
            }

            const auto count = (-1 * archive::history_block_size);
            const auto till_msg_id = (_hole->has_to() ? _hole->get_to() : -1);
            const auto patch_version = _state.get_history_patch_version("init");

            get_history_params hist_params(
            	_contact,
            	_hole->get_from(),
            	till_msg_id,
            	count,
            	patch_version,
            	false);

            int64_t depth_tail = -1;
            if (_depth != -1)
            {
                depth_tail = (_depth - _hole->get_depth());

                if (depth_tail <= 0)
                    return;
            }

            ptr_this->get_history_from_server(hist_params, last_message_catcher)->on_result_ = [wr_this, _contact, _hole, depth_tail, _recursion, hole_request, count, last_message_catcher](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_error == 0)
                {
                    ptr_this->get_archive()->validate_hole_request(_contact, *_hole, count)->on_result = [wr_this, _contact, depth_tail, _recursion, last_message_catcher](int64_t _from)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        ptr_this->download_holes(_contact, _from, depth_tail, (_recursion + 1), last_message_catcher);

                        return;

                    }; // validate_hole_request
                }

                if (_error == wim_protocol_internal_error::wpie_network_error || 
                    _error == wim_protocol_internal_error::wpie_error_task_canceled)
                {
                    ptr_this->failed_holes_requests_->add(hole_request);
                }

            }; // get_history_from_server

        }; // get_dlg_state

    }; // get_next_hole
}

void im::update_active_dialogs(const std::string& _aimid, archive::dlg_state& _state)
{
    active_dialog dlg(_aimid);

    active_dialogs_->update(dlg);
}

std::shared_ptr<async_task_handlers> im::post_dlg_state_to_gui(
    const std::string _contact, 
    const bool _add_to_active_dialogs, 
    const bool _serialize_message, 
    const bool _force)
{
    auto handler = std::make_shared<async_task_handlers>();

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(_contact)->on_result =
        [wr_this, handler, _contact, _add_to_active_dialogs, _serialize_message, _force]
        (const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_state.get_hidden_msg_id() >= _state.get_last_msgid() && !_force)
                return;

            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set<std::string>("contact", _contact);
            cl_coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
            cl_coll.set<bool>("is_chat", _contact.find("@chat.agent") != _contact.npos);

            if (ptr_this->favorites_->contains(_contact))
            {
                cl_coll.set<int64_t>("favorite_time", ptr_this->favorites_->get_time(_contact));
                if (_add_to_active_dialogs && !ptr_this->active_dialogs_->contains(_contact))
                {
                    active_dialog dlg(_contact);
                    ptr_this->active_dialogs_->update(dlg);
                }
            }

            _state.serialize(
                cl_coll.get(),
                ptr_this->auth_params_->time_offset_,
                ptr_this->fetch_params_->last_successful_fetch_,
                _serialize_message
            );

            ptr_this->post_dlg_state_to_gui(cl_coll.get());

            if (handler->on_result_)
                handler->on_result_(0);
        };

    return handler;
}

void im::check_need_agregate_dlg_state()
{
    const auto current_time = std::chrono::system_clock::now();

    if (!dlg_state_agregate_mode_ && (current_time - last_network_activity_time_) > dlg_state_agregate_start_timeout)
    {
        //ATLTRACE("agregate mode ON\r\n");

        dlg_state_agregate_mode_ = true;

        agregate_start_time_ = current_time;
    }
}

void im::post_dlg_state_to_gui(core::icollection* _dlg_state)
{
    //ATLTRACE("call post_dlg_state_to_gui\r\n");

    cached_dlg_states_.push_back(_dlg_state);

    _dlg_state->addref();

    check_need_agregate_dlg_state();

    if (dlg_state_agregate_mode_)
    {
        if (dlg_state_timer_ == 0)
        {
            std::weak_ptr<im> wr_this = shared_from_this();

            //ATLTRACE("add timer for agergate\r\n");

            dlg_state_timer_ = g_core->add_timer([wr_this]
            {
                //ATLTRACE("timer function called\r\n");

                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->post_cached_dlg_states_to_gui();

                const auto current_time = std::chrono::system_clock::now();

                if (current_time - ptr_this->agregate_start_time_ > dlg_state_agregate_period)
                {
                    //ATLTRACE("agregate mode OFF\r\n");

                    ptr_this->dlg_state_agregate_mode_ = false;

                    ptr_this->stop_dlg_state_timer();
                }

            }, 1000);
        }
    }
    else
    {
        //ATLTRACE("sync post dlg_state\r\n");

        post_cached_dlg_states_to_gui();
    }
}

void im::post_cached_dlg_states_to_gui()
{
    if (cached_dlg_states_.empty())
        return;

    coll_helper coll_dlgs(g_core->create_collection(), true);

    coll_dlgs.set_value_as_string("my_aimid", auth_params_->aimid_);
    

    ifptr<iarray> dlg_states_array(coll_dlgs->create_array());

    dlg_states_array->reserve((int32_t) cached_dlg_states_.size());

    for (auto _dlg_state_coll : cached_dlg_states_)
    {
        ifptr<ivalue> val(_dlg_state_coll->create_value());

        val->set_as_collection(_dlg_state_coll);

        dlg_states_array->push_back(val.get());

        _dlg_state_coll->release();
    }

    cached_dlg_states_.clear();

    coll_dlgs.set_value_as_array("dlg_states", dlg_states_array.get());

    g_core->post_message_to_gui("dlg_states", 0, coll_dlgs.get());
}

void im::stop_dlg_state_timer()
{
    if (dlg_state_timer_ != 0 && g_core)
    {
        g_core->stop_timer(dlg_state_timer_);

        dlg_state_timer_ = 0;
    }
}


std::shared_ptr<async_task_handlers> im::post_history_search_result_msg_to_gui(const std::string _contact
                                                               , bool _serialize_message, bool _from_search
                                                               , int64_t _req_id
                                                               , bool _is_contact
                                                               , std::shared_ptr<::core::archive::history_message> _msg
                                                               , std::string _term
                                                               , int32_t _priority)
{
    auto handler = std::make_shared<async_task_handlers>();

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(_contact)->on_result =
        [wr_this, handler, _contact, _serialize_message, _from_search, _msg, _req_id, _is_contact, _term, _priority]
        (const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            archive::dlg_state new_state = _state;
            new_state.set_last_message(*_msg);

            coll_helper cl_coll = ptr_this->serialize_history_search_result_msg(_contact, new_state, _serialize_message, _from_search, _req_id, _is_contact, _term, _priority);

            g_core->post_message_to_gui("history_search_result_msg", 0, cl_coll.get());

            if (handler->on_result_)
                handler->on_result_(0);
        };

    return handler;
}

coll_helper im::serialize_history_search_result_msg(const std::string _contact
                                                , const archive::dlg_state& _state
                                                , bool _serialize_message
                                                , bool _from_search
                                                , int64_t _req_id
                                                , bool _is_contact
                                                , const std::string& _term
                                                , int32_t _priority)
{
    coll_helper cl_coll(g_core->create_collection(), true);
    cl_coll.set<std::string>("contact", _contact);
    cl_coll.set<std::string>("my_aimid", auth_params_->aimid_);
    cl_coll.set<bool>("is_chat", _contact.find("@chat.agent") != _contact.npos);
    cl_coll.set<bool>("from_search", _from_search);
    cl_coll.set<bool>("is_contact", _is_contact);
    cl_coll.set<int64_t>("request_id", _req_id);
    cl_coll.set<std::string>("term", _term);
    cl_coll.set<int32_t>("priority", _priority);

    if (favorites_->contains(_contact))
        cl_coll.set<int64_t>("favorite_time", favorites_->get_time(_contact));

    _state.serialize(
        cl_coll.get(),
        auth_params_->time_offset_,
        fetch_params_->last_successful_fetch_,
        _serialize_message
        );

    return cl_coll;
}

std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
    std::shared_ptr<archive::face> _archive,
    const std::string& _contact,
    std::shared_ptr<archive::history_block> _messages)
{
    return _archive->remove_messages_from_not_sent(_contact, _messages);
}

void im::on_event_dlg_state(fetch_event_dlg_state* _event, std::shared_ptr<auto_callback> _on_complete)
{
    const auto &aimid = _event->get_aim_id();
    const auto &server_dlg_state = _event->get_dlg_state();
    const auto messages = _event->get_messages();

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(aimid)->on_result =
        [wr_this, aimid, _on_complete, server_dlg_state, messages]
        (const archive::dlg_state& _local_dlg_state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            ptr_this->on_event_dlg_state_local_state_loaded(
                _on_complete,
                aimid,
                _local_dlg_state,
                server_dlg_state,
                messages
            );
        };
}

void im::on_event_dlg_state_local_state_loaded(
    const auto_callback_sptr& _on_complete,
    const std::string& _aimid,
    const archive::dlg_state& _local_dlg_state,
    archive::dlg_state _server_dlg_state,
    const archive::history_block_sptr& _messages
)
{
    assert(_on_complete);
    assert(_messages);
    assert(!_aimid.empty());

    const auto &dlg_patch_version = _server_dlg_state.get_dlg_state_patch_version();
    const auto history_patch_version = _local_dlg_state.get_history_patch_version();
    const auto patch_version_changed = (
        !dlg_patch_version.empty() &&
        (dlg_patch_version != history_patch_version)
    );

    const auto dlg_del_up_to = _server_dlg_state.get_del_up_to();
    const auto local_del_up_to = _local_dlg_state.get_del_up_to();
    const auto del_up_to_changed = (dlg_del_up_to > local_del_up_to);

    __INFO(
        "delete_history",
        "compared stored and incoming dialog states\n"
        "    aimid=<%1%>\n"
        "    last-msg-id=<%2%>\n"
        "    dlg-patch=  <%3%>\n"
        "    history-patch=<%4%>\n"
        "    patch-changed=<%5%>\n"
        "    local-del= <%6%>\n"
        "    server-del=<%7%>\n"
        "    del-up-to-changed=<%8%>",
        _aimid % _server_dlg_state.get_last_msgid() %
        dlg_patch_version % history_patch_version % logutils::yn(patch_version_changed) %
        local_del_up_to % dlg_del_up_to % logutils::yn(del_up_to_changed));

    if (!_messages->empty())
    {
        on_event_dlg_state_process_messages(
            *_messages,
            _aimid,
            InOut _server_dlg_state
        );
    }

    std::weak_ptr<im> wr_this(shared_from_this());

    get_archive()->set_dlg_state(_aimid, _server_dlg_state)->on_result =
        [wr_this, _on_complete, _aimid, _local_dlg_state, _messages, patch_version_changed, del_up_to_changed]
        (const archive::dlg_state &_local_dlg_state, const archive::dlg_state_changes&)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            ptr_this->on_event_dlg_state_local_state_updated(
                _on_complete,
                _messages,
                _aimid,
                _local_dlg_state,
                patch_version_changed,
                del_up_to_changed
            );
        };
}

void im::on_event_dlg_state_process_messages(
    const archive::history_block& _messages,
    const std::string& _aimid,
    InOut archive::dlg_state& _server_dlg_state
)
{
    assert (!_messages.empty());

    for (archive::history_block::const_reverse_iterator msg = _messages.crbegin();
        msg != _messages.crend();
        ++msg)
    {
        if ((*msg)->get_msgid() == _server_dlg_state.get_last_msgid())
        {
            _server_dlg_state.set_last_message(**msg);
            break;
        }
    }

    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_string("aimid", _aimid);

    ifptr<iarray> senders_array(coll->create_array());
    senders_array->reserve((int32_t)_messages.size());
    for (const auto message: _messages)
    {
        if (!message)
        {
            continue;
        }

        std::string sender;
        if (message.get()->get_chat_data())
        {
            sender = message.get()->get_chat_data()->get_sender();
        }
        else if (!message.get()->get_sender_friendly().empty())
        {
            sender = _aimid;
        }

        if (!sender.empty())
        {
            ifptr<ivalue> val(coll->create_value());
            val->set_as_string(sender.c_str(), (int32_t)sender.length());
            senders_array->push_back(val.get());
        }
    }

    coll.set_value_as_array("senders", senders_array.get());

    g_core->post_message_to_gui("messages/received/senders", 0, coll.get());

    update_active_dialogs(_aimid, _server_dlg_state);
}

void im::on_event_dlg_state_local_state_updated(
    const auto_callback_sptr& _on_complete,
    const archive::history_block_sptr& _messages,
    const std::string& _aimid,
    const archive::dlg_state& _local_dlg_state,
    const bool _patch_version_changed,
    const bool _del_up_to_changed
)
{
    const auto serialize_message = (!_messages->empty() || _del_up_to_changed);
    post_dlg_state_to_gui(_aimid, false, serialize_message);

    if (_patch_version_changed)
    {
        on_history_patch_version_changed(
            _aimid,
            _local_dlg_state.get_history_patch_version("init"));
    }

    if (_messages->empty() && _del_up_to_changed)
    {
        on_del_up_to_changed(
            _aimid,
            _local_dlg_state.get_del_up_to(),
            _on_complete);

        return;
    }

    if (has_opened_dialogs(_aimid))
    {
        coll_helper coll(g_core->create_collection(), true);

        coll.set_value_as_bool("result", true);
        coll.set_value_as_string("contact", _aimid);
        serialize_messages_4_gui("messages", _messages, coll.get(), auth_params_->time_offset_);
        coll.set_value_as_int64("theirs_last_delivered", _local_dlg_state.get_theirs_last_delivered());
        coll.set_value_as_int64("theirs_last_read", _local_dlg_state.get_theirs_last_read());
        coll.set<std::string>("my_aimid", auth_params_->aimid_);

        g_core->post_message_to_gui("messages/received/dlg_state", 0, coll.get());
    }

    std::weak_ptr<im> wr_this(shared_from_this());

    remove_messages_from_not_sent(get_archive(), _aimid, _messages)->on_result_ =
        [wr_this, _on_complete, _aimid, _messages, _local_dlg_state, _patch_version_changed, _del_up_to_changed]
        (int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            int64_t last_message_id = _messages->empty() ? -1 : (*_messages->rbegin())->get_msgid();
            int32_t count = _messages->size();

            ptr_this->get_archive()->update_history(_aimid, _messages)->on_result = 
                [wr_this, _aimid, _local_dlg_state, _on_complete, _patch_version_changed, _del_up_to_changed, last_message_id, count]
                (archive::headers_list_sptr _inserted_messages, const archive::dlg_state&, const archive::dlg_state_changes&)

                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->on_event_dlg_state_history_updated(
                        _on_complete,
                        _patch_version_changed,
                        _local_dlg_state,
                        _aimid,
                        last_message_id,
                        count,
                        _del_up_to_changed
                    );
                };
        };
}

void im::on_event_dlg_state_history_updated(
    const auto_callback_sptr& _on_complete,
    const bool _patch_version_changed,
    const archive::dlg_state& _local_dlg_state,
    const std::string& _aimid,
    const int64_t _last_message_id,
    const int32_t _count,
    const bool _del_up_to_changed)
{
    std::weak_ptr<im> wr_this(shared_from_this());

    get_archive()->has_not_sent_messages(_aimid)->on_result =
        [wr_this, _aimid, _patch_version_changed, _del_up_to_changed, _local_dlg_state, _on_complete, _last_message_id, _count]
        (bool _has_not_sent_messages)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            if ((ptr_this->has_opened_dialogs(_aimid) || _has_not_sent_messages) && _count && _last_message_id > 0)
            {
                ptr_this->download_holes(
                	_aimid,
                	_last_message_id,
                	(_count + 1));

                if (_del_up_to_changed)
                {
                    ptr_this->on_del_up_to_changed(_aimid, _local_dlg_state.get_del_up_to(), _on_complete);

                    return;
                }

                _on_complete->callback(0);

                return;
            }

            if (_patch_version_changed)
            {
                ptr_this->on_history_patch_version_changed(
                    _aimid,
                    _local_dlg_state.get_history_patch_version("init")
                );
            }

            if (_del_up_to_changed)
            {
                ptr_this->on_del_up_to_changed(_aimid, _local_dlg_state.get_del_up_to(), _on_complete);

                return;
            }

            _on_complete->callback(0);
        };
}

void im::on_event_hidden_chat(fetch_event_hidden_chat* _event, std::shared_ptr<auto_callback> _on_complete)
{
    if (favorites_->contains(_event->get_aimid()))
    {
        _on_complete->callback(0);
        return;
    }

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    int64_t last_msg_id = _event->get_last_msg_id();
    std::string aimid = _event->get_aimid();

    get_archive()->get_dlg_state(aimid)->on_result =
        [wr_this,
        aimid,
        last_msg_id,
        _on_complete](const archive::dlg_state& _local_dlg_state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_local_dlg_state.get_last_msgid() <= last_msg_id)
            {
                archive::dlg_state updated_state = _local_dlg_state;
                updated_state.set_hidden_msg_id(last_msg_id);

                ptr_this->get_archive()->set_dlg_state(aimid, updated_state)->on_result =
                    [wr_this, aimid]
                (const archive::dlg_state&, const archive::dlg_state_changes&)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->active_dialogs_->remove(aimid);

                    coll_helper cl_coll(g_core->create_collection(), true);
                    cl_coll.set_value_as_string("contact", aimid);
                    g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
                };

            }

            _on_complete->callback(0);
        };
}

std::wstring im::get_im_path() const
{
    int32_t id = get_id();
    if (id <= 0)
        id = default_im_id;

    char buffer[20];
    sprintf(buffer, "%04d", id);

    return tools::from_utf8(buffer);
}


void im::login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login)
{
    if (_country.empty() || _raw_phone.empty())
    {
        assert(!"country or phone is empty");

        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set_value_as_bool("result", false);
        cl_coll.set_value_as_int("error", (uint32_t)  le_error_validate_phone);
        g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
    }

    auto packet = std::make_shared<core::wim::normalize_phone>(make_wim_params(), _country, _raw_phone);
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _locale, _is_login](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (0 == _error)
        {
            phone_info _info;
            _info.set_phone(packet->get_normalized_phone());
            _info.set_locale(_locale);

            ptr_this->login_get_sms_code(_seq, _info, _is_login);
        }
        else
        {
            login_error err = le_error_validate_phone;
            switch (_error)
            {
            case wim_protocol_internal_error::wpie_network_error:
            case wim_protocol_internal_error::wpie_http_error:
                err = login_error::le_network_error;
                break;
            case wim_protocol_internal_error::wpie_invalid_phone_number:
                err = login_error::le_error_validate_phone;
            }

            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", false);
            cl_coll.set_value_as_int("error", (uint32_t) err);

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
    };
}

void im::history_search_one_batch(std::shared_ptr<archive::coded_term> _cterm, std::shared_ptr<archive::contact_and_msgs> _thread_archive
                                  , std::shared_ptr<tools::binary_stream> data, int64_t _seq
                                  , int64_t _min_id)
{
    if (search_data_.req_id != _seq || search_data_.req_id == -1)
        return;

    std::shared_ptr<archive::contact_and_offsets> contact_and_offsets(new archive::contact_and_offsets());

    auto max_contacts_for_one_buffer = 100u;
    for (auto index = 0u; index < max_contacts_for_one_buffer && index < search_data_.contact_and_offset.size(); ++index)
    {
        auto item = search_data_.contact_and_offset.back();
        search_data_.contact_and_offset.pop_back();
        contact_and_offsets->push_back(item);
    }

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    get_archive()->get_history_block(contact_and_offsets, _thread_archive, data)->on_result =
        [_cterm, contact_and_offsets, wr_this, _seq, _min_id]
            (std::shared_ptr<archive::contact_and_msgs> _archive, std::shared_ptr<archive::contact_and_offsets> _remaining, std::shared_ptr<tools::binary_stream> _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (ptr_this->search_data_.req_id != _seq || ptr_this->search_data_.req_id == -1)
                    return;

                if (_remaining)
                {
                    for (auto item : *_remaining)
                    {
                        ptr_this->search_data_.contact_and_offset.push_back(item);
                    }
                }

                ptr_this->history_searcher_->run_t_async_function<std::vector<std::shared_ptr<::core::archive::searched_msg>>>(
                    [_cterm, _archive, contact_and_offsets, _seq, _min_id, _data]()->std::vector<std::shared_ptr<::core::archive::searched_msg>>
                        {
                            std::vector<std::shared_ptr<::core::archive::searched_msg>> messages_ids;

                            if (_archive->size() > 1)
                            {
                                archive::messages_data::search_in_archive(contact_and_offsets, _cterm, _archive, _data, messages_ids, _min_id);
                            }

                            return messages_ids;

                        })->on_result_ = [wr_this, contact_and_offsets, _archive, _seq, _min_id, _data, _cterm]
                        (std::vector<std::shared_ptr<::core::archive::searched_msg>> messages_ids)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            if (ptr_this->search_data_.req_id != _seq || ptr_this->search_data_.req_id == -1)
                                return;

                            for (auto item : *contact_and_offsets)
                            {
                                auto offset = item.second;
                                auto contact = item.first.first;
                                auto mode = item.first.second;
                                if (*mode == 0)
                                {
                                    ptr_this->search_data_.contact_and_offset.push_front(std::make_pair(std::make_pair(contact, std::make_shared<int64_t>(2)), std::make_shared<int64_t>(0)));
                                }
                                *mode = 1;

                                if (*offset > 0)
                                {
                                    ptr_this->search_data_.contact_and_offset.push_back(std::make_pair(std::make_pair(contact, mode), offset));
                                }
                            }

                            if (!ptr_this->search_data_.contact_and_offset.empty())
                            {
                                int64_t last_id = -1;
                                if (ptr_this->search_data_.top_messages.size() >= ::common::get_limit_search_results())
                                    last_id = ptr_this->search_data_.top_messages_ids.rbegin()->first;

                                ptr_this->history_search_one_batch(_cterm, _archive, _data, ptr_this->search_data_.req_id, last_id);
                            }
                            else
                            {
                                ++ptr_this->search_data_.count_of_free_threads;
                            }

                            for (auto item : messages_ids)
                            {
                                if (ptr_this->search_data_.top_messages_ids.count(item->id) != 0)
                                    continue;

                                if (ptr_this->search_data_.top_messages_ids.size() < ::common::get_limit_search_results())
                                {
                                    ptr_this->search_data_.top_messages.push_back(item);
                                    ptr_this->search_data_.top_messages_ids.insert(std::make_pair(item->id, ptr_this->search_data_.top_messages.size() - 1));
                                }
                                else
                                {
                                    auto greater = ptr_this->search_data_.top_messages_ids.upper_bound(item->id);

                                    if (greater != ptr_this->search_data_.top_messages_ids.end())
                                    {
                                        auto index = ptr_this->search_data_.top_messages_ids.rbegin()->second;
                                        auto min_id = ptr_this->search_data_.top_messages_ids.rbegin()->first;

                                        if (index == -1)
                                        {
                                            ptr_this->search_data_.top_messages.push_back(item);
                                            index = ptr_this->search_data_.top_messages.size() - 1;
                                        }
                                        else
                                        {
                                            ptr_this->search_data_.top_messages[index] = item;
                                        }

                                        ptr_this->search_data_.top_messages_ids.erase(min_id);
                                        ptr_this->search_data_.top_messages_ids.insert(std::make_pair(item->id, index));
                                    }
                                }
                            }

                            if (ptr_this->search_data_.count_of_free_threads == search_threads_count
                                    || (std::chrono::system_clock::now() > ptr_this->search_data_.last_send_time + sending_search_results_interval_ms))
                            {
                                auto ptr_this = wr_this.lock();
                                if (!ptr_this)
                                    return;

                                ptr_this->search_data_.count_of_yet_no_sent_msgs = ptr_this->search_data_.top_messages.size();
                                for (auto item : ptr_this->search_data_.top_messages)
                                {
                                    auto aimid = item->contact;
                                    auto msg_id = item->id;
                                    auto term = item->term;
                                    ptr_this->get_archive()->get_messages(aimid, msg_id, 0, 1)->on_result = [wr_this, aimid, term, msg_id, _seq]
                                    (std::shared_ptr<archive::history_block> _messages)
                                    {
                                        auto ptr_this = wr_this.lock();
                                        if (!ptr_this)
                                            return;

                                        --ptr_this->search_data_.count_of_yet_no_sent_msgs;

                                        if (ptr_this->search_data_.req_id != _seq
                                            || ptr_this->search_data_.req_id == -1
                                            || _messages->empty()
                                            || (*_messages)[0]->get_msgid() != msg_id
                                            || (*_messages)[0]->is_chat_event_deleted()
                                            || (*_messages)[0]->is_deleted())
                                        {
                                            ptr_this->search_data_.top_messages_ids.erase(msg_id);

                                            if (ptr_this->search_data_.count_of_free_threads == search_threads_count
                                                && ptr_this->search_data_.count_of_yet_no_sent_msgs == 0
                                                && ptr_this->search_data_.count_of_sent_msgs == 0)
                                            {
                                                coll_helper cl_coll(g_core->create_collection(), true);
                                                cl_coll.set<int64_t>("req_id", ptr_this->search_data_.req_id);
                                                g_core->post_message_to_gui("empty_search_results", 0, cl_coll.get());
                                                g_core->insert_event(stats::stats_event_names::cl_search_nohistory);
                                            }
                                        }
                                        else
                                        {
                                            ++ptr_this->search_data_.count_of_sent_msgs;
                                            ptr_this->post_history_search_result_msg_to_gui(aimid, true, true, ptr_this->search_data_.req_id
                                                , false /* is_contact */, (*_messages)[0], term, 0);
                                        }
                                    };

                                    ptr_this->search_data_.top_messages_ids[item->id] = -1;
                                }

                                ptr_this->search_data_.top_messages.clear();
                                ptr_this->search_data_.last_send_time = std::chrono::system_clock::now();
                            }

                            if (ptr_this->search_data_.count_of_free_threads == search_threads_count
                                && ptr_this->search_data_.top_messages_ids.empty())
                            {
                                coll_helper cl_coll(g_core->create_collection(), true);
                                cl_coll.set<int64_t>("req_id", ptr_this->search_data_.req_id);
                                g_core->post_message_to_gui("empty_search_results", 0, cl_coll.get());
                                g_core->insert_event(stats::stats_event_names::cl_search_nohistory);
                            }
                        };
            };
}

void im::history_search_in_cl(const std::vector<std::vector<std::string>>& search_patterns, int64_t _req_id, unsigned fixed_patterns_count, const std::string& pattern)
{
    auto post = [this, _req_id](const std::vector<std::string>& result)
    {
        coll_helper coll(g_core->create_collection(), true);
        ifptr<iarray> array(coll->create_array());
        for (auto contact : result)
        {
            archive::dlg_state fake_state;
            auto coll = serialize_history_search_result_msg(contact, fake_state, true, true, _req_id, true, std::string(), contact_list_->search_priority_[contact]);
            ifptr<ivalue> iv(coll->create_value());
            iv->set_as_collection(coll.get());
            array->push_back(iv.get());
        }
        coll.set_value_as_array("results", array.get());
        coll.set_value_as_int64("reqId", _req_id);

        g_core->post_message_to_gui("history_search_result_contacts", 0, coll.get());
    };

    if (fixed_patterns_count == 0)
        post(contact_list_->search(std::string(), true, 0, 0));

    if (!pattern.empty())
    {
        post(contact_list_->search(pattern, true, 0, fixed_patterns_count));
        if (g_core->end_search() == 0)
            contact_list_->last_search_patterns_ = pattern;
        return;
    }
    
    for (unsigned i = 0; i < fixed_patterns_count; ++i)
    {
        std::string pat;
        for (auto j = 0u; j < search_patterns.size(); j++)
            pat += search_patterns[j][i];

        post(contact_list_->search(pat, i == 0, i, fixed_patterns_count));
    }

    post(contact_list_->search(search_patterns, fixed_patterns_count));
}

void im::setup_search_params(int64_t _req_id)
{
    search_data_.start_time = std::chrono::system_clock::now();
    search_data_.last_send_time = std::chrono::system_clock::now() - std::chrono::milliseconds(2 * sending_search_results_interval_ms);
    search_data_.req_id = _req_id;
    search_data_.top_messages.clear();
    search_data_.count_of_yet_no_sent_msgs = 0;
    search_data_.count_of_sent_msgs = 0;
    search_data_.top_messages_ids.clear();
    search_data_.contact_and_offset.clear();
    search_data_.count_of_free_threads = search_threads_count;
}

void im::clear_search_params()
{
    setup_search_params(-1);
}

void im::history_search_in_history(const std::string& term, const std::vector<std::string>& _aimids)
{
    if (term.empty())
    {
        g_core->end_history_search();
        return;
    }

    if (_aimids.empty())
    {
        for (auto item : contact_list_->contacts_index_)
        {
            search_data_.contact_and_offset.push_back(std::make_pair(std::make_pair(item.second->aimid_, std::make_shared<int64_t>(0)), std::make_shared<int64_t>(0)));
        }
    }
    else
    {
        for (auto item : _aimids)
        {
            search_data_.contact_and_offset.push_back(std::make_pair(std::make_pair(item, std::make_shared<int64_t>(0)), std::make_shared<int64_t>(0)));
        }
    }

    std::shared_ptr<int32_t> last_symb_id(new int32_t(0));

    std::shared_ptr<archive::coded_term> cterm(new archive::coded_term());
    cterm->lower_term = ::tools::system::to_lower(term);
    cterm->coded_string = tools::convert_string_to_vector(term, last_symb_id, cterm->symbs, cterm->symb_indexes, cterm->symb_table);
    cterm->prefix = std::vector<int32_t>(tools::build_prefix(cterm->coded_string));

    auto started_contact_count = std::min<int64_t>(search_threads_count, search_data_.contact_and_offset.size());
    for (auto i = 0; i < started_contact_count; ++i)
    {
        std::shared_ptr<archive::contact_and_msgs> thread_archive(new archive::contact_and_msgs());

        std::shared_ptr<tools::binary_stream> data(new tools::binary_stream());
        data->reserve(1024 * 1024 * 10);

        --search_data_.count_of_free_threads;

        history_search_one_batch(cterm, thread_archive, data, search_data_.req_id, -1 /* _min_id */);
    }

    if (started_contact_count == 0)
    {
        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set<int64_t>("req_id", search_data_.req_id);
        g_core->post_message_to_gui("empty_search_results", 0, cl_coll.get());
        g_core->insert_event(stats::stats_event_names::cl_search_nohistory);
    }
}

void im::login_get_sms_code(int64_t _seq, const phone_info& _info, bool _is_login)
{
    auto packet = std::make_shared<core::wim::validate_phone>(make_wim_params(), _info.get_phone(), _info.get_locale());
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _is_login](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", true);
            cl_coll.set_value_as_int("code_length", packet->get_code_length());

            auto phone_data = std::make_shared<phone_info>();
            if (_is_login)
                ptr_this->phone_registration_data_ = phone_data;
            else
                ptr_this->attached_phone_registration_data_ = phone_data;

            phone_data->set_phone(packet->get_phone());
            phone_data->set_trans_id(packet->get_trans_id());
            phone_data->set_existing(packet->get_existing());

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
        else
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", false);

            login_error err = login_error::le_unknown_error;

            switch ((wim_protocol_internal_error)_error)
            {
            case wpie_network_error:
                err = login_error::le_network_error;
                break;
            case wpie_start_session_rate_limit:
                err = login_error::le_rate_limit;
                break;
            case wpie_phone_not_verified:
                err = login_error::le_invalid_sms_code;
                break;
            case wpie_http_error:
            case wpie_http_empty_response:
            case wpie_http_parse_response:
            case wpie_request_timeout:
                break;
            default:
                break;
            }

            cl_coll.set_value_as_int("result", (uint32_t) err);

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
    };
}

void im::login_by_phone(int64_t _seq, const phone_info& _info)
{
    if (!phone_registration_data_)
    {
        assert(!"im->phone_registration_data_ empty");
        return;
    }

    if (_info.get_sms_code().empty())
    {
        assert(!"_sms_code empty, validate it");
        return;
    }

    phone_info info_for_login(*phone_registration_data_);
    info_for_login.set_sms_code(_info.get_sms_code());

    auto packet = std::make_shared<core::wim::phone_login>(make_wim_params(), info_for_login);
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->a_token_ = packet->get_a_token();
            ptr_this->auth_params_->session_key_ = packet->get_session_key();
            ptr_this->auth_params_->exipired_in_ = packet->get_expired_in();
            ptr_this->auth_params_->time_offset_ = packet->get_time_offset();

            ptr_this->on_login_result(_seq, 0, false);
            ptr_this->start_session();
        }
        else
        {
            ptr_this->on_login_result(_seq, _error, false);
        }
    };
}

void im::start_attach_phone(int64_t _seq, const phone_info& _info)
{
    if (!attached_phone_registration_data_)
    {
        assert(!"im->phone_registration_data_ empty");
        return;
    }

    if (_info.get_sms_code().empty())
    {
        assert(!"_sms_code empty, validate it");
        return;
    }

    phone_info info_for_login(*attached_phone_registration_data_);
    info_for_login.set_sms_code(_info.get_sms_code());

    auth_parameters auth_params;
    auth_params.a_token_ = auth_params_->a_token_;
    auth_params.session_key_ =  auth_params_->session_key_;

    attach_phone(_seq, auth_params, info_for_login);
}

void im::sign_url(int64_t _seq, const std::string& url)
{
    auto es = [](std::string v) { return wim_packet::escape_symbols(v); };
    auto wim_params = make_wim_params();

    std::string unsigned_url = url;

    std::string signed_url;
    {
        const std::string host = "https://www.icq.com/karma_api/karma_client2web_login.php";

        std::map<std::string, std::string> params;

        params["ts"] = tools::from_int64(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - wim_params.time_offset_);
        params["owner"] = wim_params.aimid_;

        params["a"] = wim_packet::escape_symbols(wim_params.a_token_);
        params["k"] = wim_params.dev_id_;
        params["d"] = es(unsigned_url);
        std::string hash_data = ("GET&" + es(host) + "&" + es(wim_packet::create_query_from_map(params)));
        std::string sha256 = wim_packet::detect_digest(hash_data, wim_params.session_key_);
        params["sig_sha256"] = es(sha256);

        //params["a"] = es(params["a"]);
        //params["k"] = es(params["k"]);
        //params["d"] = es(params["d"]);
        //params["sig_sha256"] = es(params["sig_sha256"]);

        std::stringstream ss_url;
        ss_url << host << "?" << wim_packet::format_get_params(params);
        signed_url = ss_url.str();
    }

    auto icoll = g_core->create_collection();
    coll_helper coll(icoll, true);
    coll.set_value_as_string("url", signed_url);

    g_core->post_message_to_gui("signed_url", _seq, coll.get());
}

void im::resume_failed_network_requests()
{
    std::weak_ptr<wim::im> wr_this = shared_from_this();

    async_tasks_->run_async_function([]()->int32_t
    {
        return 0;

    })->on_result_ = [wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->post_pending_messages();
        ptr_this->resume_all_file_sharing_uploading();
        ptr_this->resume_failed_avatars();
        ptr_this->download_failed_holes();
        ptr_this->resume_download_stickers();
        ptr_this->resume_download_masks();

        ptr_this->get_async_loader().resume_suspended_tasks(ptr_this->make_wim_params());
    };
}

void im::resume_failed_avatars()
{
    get_avatar_loader()->resume(make_wim_params());
}


void im::get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size, bool _force)
{
    auto context = std::make_shared<avatar_context>(_avatar_size, _contact, get_im_data_path());
    context->force_ = _force;

    auto handler = get_avatar_loader()->get_contact_avatar_async(make_wim_params(), context);

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    handler->completed_ = handler->updated_ = [wr_this, _seq](std::shared_ptr<avatar_context> _context)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), false);
        ifptr<istream> avatar_stream(coll->create_stream());
        uint32_t size = _context->avatar_data_.available();
        avatar_stream->write((uint8_t*)_context->avatar_data_.read(size), size);
        coll.set_value_as_stream("avatar", avatar_stream.get());

        coll.set_value_as_bool("result", true);
        coll.set_value_as_string("contact", _context->contact_);
        coll.set_value_as_int("size", _context->avatar_size_);

        _context->avatar_data_.reset();

        const bool avatar_need_to_convert = ptr_this->on_voip_avatar_actual_for_voip(_context->contact_, _context->avatar_size_);

        coll.set_value_as_bool("avatar_need_to_convert", avatar_need_to_convert);

        g_core->post_message_to_gui("avatars/get/result", _seq, coll.get());
    };

    handler->failed_ = [_seq](std::shared_ptr<avatar_context> _context, int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("result", false);
        coll.set_value_as_string("contact", _context->contact_);
        coll.set_value_as_int("size", _context->avatar_size_);

        avatar_error av_err = avatar_error::ae_unknown_error;
        if (_error == wim_protocol_internal_error::wpie_network_error)
            av_err = avatar_error::ae_network_error;

        coll.set_value_as_int("error", av_err);
        coll.set_value_as_bool("avatar_need_to_convert", false);

        g_core->post_message_to_gui("avatars/get/result", _seq, coll.get());
    };
}

void im::show_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size)
{
    get_avatar_loader()->show_contact_avatar(_contact, _avatar_size);
}

std::shared_ptr<avatar_loader> im::get_avatar_loader()
{
    if (!avatar_loader_)
        avatar_loader_.reset(new avatar_loader());

    return avatar_loader_;
}

std::shared_ptr<send_message_handler> im::send_message_async(
    int64_t _seq,
    const std::string& _contact,
    const std::string& _message,
    const std::string& _internal_id,
    const core::message_type _type,
    core::archive::quotes_vec _quotes)
{
    assert(_type > message_type::min);
    assert(_type < message_type::max);

    auto handler = std::make_shared<send_message_handler>();

    auto packet = std::make_shared<core::wim::send_message>(make_wim_params(), _type, _internal_id, _contact, _message, _quotes);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [handler, packet, _seq, wr_this, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(_error, *packet);
    };

    return handler;
}

void im::send_message_to_contact(
    int64_t _seq,
    const std::string& _contact,
    const std::string& _message,
    const core::message_type _type,
    const std::string& _internal_id,
    const core::archive::quotes_vec& _quotes)
{
    assert(_type > message_type::min);
    assert(_type < message_type::max);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    core::archive::quotes_vec quotes = _quotes;
    for (auto q = quotes.begin(); q != quotes.end(); ++q)
    {
        int32_t time = q->get_time();
        time -= auth_params_->time_offset_;
        q->set_time(time);
    }

    archive::not_sent_message_sptr msg_not_sent;

    if (_type == core::message_type::file_sharing)
    {
        assert(!_message.empty());

        msg_not_sent = archive::not_sent_message::make_incoming_file_sharing(_contact, time, _message, _internal_id);
    }
    else
    {
        msg_not_sent = archive::not_sent_message::make(_contact, _message, _type, time, _internal_id);
    }

    msg_not_sent->attach_quotes(quotes);

    get_archive()->insert_not_sent_message(_contact, msg_not_sent);

    get_archive()->get_dlg_state(_contact)->on_result =
        [wr_this, _contact, _message, msg_not_sent]
        (const archive::dlg_state& _local_dlg_state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            archive::dlg_state new_dlg_state = _local_dlg_state;

            auto message = msg_not_sent->get_message();

            if (message->has_msgid())
            {
                new_dlg_state.set_last_msgid(message->get_msgid());
            }
            else
            {
                new_dlg_state.set_last_msgid(_local_dlg_state.get_last_msgid());
            }

            new_dlg_state.set_last_message(*message);
            new_dlg_state.set_unread_count(0);
            new_dlg_state.set_visible(true);
            new_dlg_state.set_official(_local_dlg_state.get_official());
            new_dlg_state.set_fake(true);

            ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result =
                [wr_this, _contact]
                (const archive::dlg_state&, const archive::dlg_state_changes&)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->post_dlg_state_to_gui(_contact);
                };
        };

    post_not_sent_message_to_gui(_seq, msg_not_sent);

    post_pending_messages();
}

void im::send_message_typing(int64_t _seq, const std::string& _contact, const core::typing_status& _status)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::send_message_typing>(make_wim_params(), _contact, _status);

    std::function<void(int32_t)> on_result = [](int32_t _error)
    {
        return;
    };

    post_wim_packet(packet)->on_result_ = on_result;

}

void im::send_feedback(const int64_t _seq, const std::string &url, const std::map<std::string, std::string> &fields, const std::vector<std::string> &files)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::send_feedback>(make_wim_params(), url, fields, files);

    std::function<void(int32_t)> on_result = [_seq](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("succeeded", (_error == 0));
        g_core->post_message_to_gui("feedback/sent", _seq, coll.get());

        if (_error == 0)
            g_core->insert_event(stats::stats_event_names::feedback_sent);
        else
            g_core->insert_event(stats::stats_event_names::feedback_error);
    };

    post_wim_packet(packet)->on_result_ = on_result;

}

void im::set_state(const int64_t _seq, const core::profile_state _state)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::set_state>(make_wim_params(), _state);

    std::function<void(int32_t)> on_result = [_seq, _state](int32_t _error)
    {
        return;
    };

    post_wim_packet(packet)->on_result_ = on_result;
}

void im::phoneinfo(int64_t seq, const std::string &phone, const std::string &gui_locale)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::phoneinfo>(make_wim_params(), phone, gui_locale);
    std::function<void(int32_t)> on_result = [seq, packet](int32_t _error)
    {
        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_string("operator", packet->info_operator_);
            coll.set_value_as_string("phone", packet->info_phone_);
            coll.set_value_as_string("iso_country", packet->info_iso_country_);
            if (!packet->printable_.empty())
            {
                ifptr<iarray> array(coll->create_array());
                array->reserve((int32_t)packet->printable_.size());
                for (const auto v: packet->printable_)
                {
                    ifptr<ivalue> iv(coll->create_value());
                    iv->set_as_string(v.c_str(), (int32_t)v.length());
                    array->push_back(iv.get());
                }
                coll.set_value_as_array("printable", array.get());
            }
            coll.set_value_as_string("status", packet->status_);
            coll.set_value_as_string("trunk_code", packet->trunk_code_);
            coll.set_value_as_string("modified_phone_number", packet->modified_phone_number_);
            if (!packet->remaining_lengths_.empty())
            {
                ifptr<iarray> array(coll->create_array());
                array->reserve((int32_t)packet->remaining_lengths_.size());
                for (const auto v: packet->remaining_lengths_)
                {
                    ifptr<ivalue> iv(coll->create_value());
                    iv->set_as_int64(v);
                    array->push_back(iv.get());
                }
                coll.set_value_as_array("remaining_lengths", array.get());
            }
            if (!packet->prefix_state_.empty())
            {
                ifptr<iarray> array(coll->create_array());
                array->reserve((int32_t)packet->prefix_state_.size());
                for (const auto v: packet->prefix_state_)
                {
                    ifptr<ivalue> iv(coll->create_value());
                    iv->set_as_string(v.c_str(), (int32_t)v.length());
                    array->push_back(iv.get());
                }
                coll.set_value_as_array("prefix_state", array.get());
            }
            coll.set_value_as_string("modified_prefix", packet->modified_prefix_);
            g_core->post_message_to_gui("phoneinfo/result", seq, coll.get());
        }
    };
    post_wim_packet(packet)->on_result_ = on_result;
}

void im::remove_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::remove_members>(make_wim_params(), _aimid, _m_chat_members);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
    };

    post_wim_packet(packet)->on_result_ = on_result;
}


void im::add_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_add)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::add_members>(make_wim_params(), _aimid, _m_chat_members_to_add);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
    };

    //if (contact_list_->exist(_aimid))
    post_wim_packet(packet)->on_result_ = on_result;
}


void im::add_chat(int64_t _seq, const std::string& _m_chat_name, const std::vector<std::string>& _m_chat_members)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::add_chat>(make_wim_params(), _m_chat_name, _m_chat_members);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            core::stats::event_props_type props;
            auto members_count = packet->get_members_count();
            props.push_back(std::make_pair("Groupchat_Create_MembersCount", std::to_string(members_count)));
            g_core->insert_event(stats::stats_event_names::groupchat_created, props);
        }
    };

}

void im::get_mrim_key(int64_t _seq, const std::string& _email)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::mrim_get_key>(make_wim_params(), _email);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        auto key = packet->get_mrim_key();
        if (_error == 0 && !key.empty())
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_string("key", key);
            g_core->post_message_to_gui("mrim/get_key/result", _seq, coll.get());
        }
    };

}

void im::on_history_patch_version_changed(const std::string& _aimid, const std::string& _history_patch_version)
{
    assert(!_aimid.empty());
    assert(!_history_patch_version.empty());

    const auto count = (-1 * core::archive::history_block_size);

    const get_history_params params(
        _aimid,
        -1,
        -1,
        count,
        _history_patch_version);

    __INFO(
        "delete_history",
        "history patch version changed, download history anew\n"
        "    contact=<%1%>\n"
        "    request-from=<%2%>\n"
        "    count=<%3%>\n"
        "    patch=<%4%>",
        _aimid % -1 % count % _history_patch_version);

	get_history_from_server(params, [](int64_t){});
}

void im::on_del_up_to_changed(const std::string& _aimid, const int64_t _del_up_to, const auto_callback_sptr& _on_complete)
{
    assert(!_aimid.empty());
    assert(_del_up_to > -1);
    assert(_on_complete);

    __INFO(
        "delete_history",
        "requesting history deletion\n"
        "    contact=<%1%>\n"
        "    up-to=<%2%>",
        _aimid % _del_up_to
    );

    get_archive()->delete_messages_up_to(_aimid, _del_up_to)->on_result_ =
        [_on_complete, _aimid, _del_up_to](int32_t error)
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set<std::string>("contact", _aimid);
            cl_coll.set<int64_t>("id", _del_up_to);

            g_core->post_message_to_gui("messages/del_up_to", 0, cl_coll.get());

            _on_complete->callback(error);
        };
}

void im::modify_chat(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_name)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::modify_chat>(make_wim_params(), _aimid, _m_chat_name);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
        if (_error == 0)
        {
            g_core->insert_event(stats::stats_event_names::groupchat_rename);
        }
    };

    //if (contact_list_->exist(_aimid))
    post_wim_packet(packet)->on_result_ = on_result;
}


std::shared_ptr<async_task_handlers> im::send_pending_message_async(int64_t _seq, const archive::not_sent_message_sptr& _message)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto handler = std::make_shared<async_task_handlers>();

    const auto &pending_message = _message->get_message();

    send_message_async(
        _seq,
        _message->get_aimid(),
        pending_message->get_text(),
        pending_message->get_internal_id(),
        pending_message->get_type(),
        _message->get_quotes())->on_result = [wr_this, handler, _message](int32_t _error, const send_message& _packet)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        const auto auto_handler = std::make_shared<tools::auto_scope>([handler, _error] {handler->on_result_(_error);});

        if (_error == 0)
        {
            uint64_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - ptr_this->auth_params_->time_offset_;
            auto delay = time - _message->get_message()->get_time();

            if (delay > core::stats::msg_pending_delay_s)
            {
                core::stats::event_props_type props;
                props.emplace_back("delay", std::to_string(delay));
                g_core->insert_event(core::stats::stats_event_names::message_pending, props);
            }

            if (_packet.get_hist_msg_id() > 0)
            {
                coll_helper coll(g_core->create_collection(), true);

                auto messages_block = std::make_shared<archive::history_block>();

                auto msg = _message->get_message();

                msg->set_msgid(_packet.get_hist_msg_id());
                msg->set_prev_msgid(_packet.get_before_hist_msg_id());

                messages_block->push_back(_message->get_message());

                coll.set_value_as_string("contact", _message->get_aimid());
                coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
                serialize_messages_4_gui("messages", messages_block, coll.get(), ptr_this->auth_params_->time_offset_);

                g_core->post_message_to_gui("messages/received/message_status", 0, coll.get());

                ptr_this->get_archive()->update_history(_message->get_aimid(), messages_block)->on_result = [wr_this, _message, auto_handler](
                    std::shared_ptr<archive::headers_list> _inserted_messages,
                    const archive::dlg_state&,
                    const archive::dlg_state_changes& _changes_on_update)
                {
                    std::shared_ptr<wim::im> ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->get_archive()->remove_message_from_not_sent(_message->get_aimid(), _message->get_message());
                };
            }
            else
            {
                auto message = archive::not_sent_message::make(_message, _packet.get_wim_msg_id(), time);

                ptr_this->get_archive()->update_if_exist_not_sent_message(_message->get_aimid(), message);
            }
        }
        else if (_packet.get_http_code() == 200)
        {
            switch (_packet.get_status_code())
            {
            case wim_protocol_error::INVALID_REQUEST:
            case wim_protocol_error::PARAMETER_ERROR:
            case wim_protocol_error::INVALID_TARGET:
            case wim_protocol_error::TARGET_DOESNT_EXIST:
            case wim_protocol_error::TARGET_NOT_AVAILABLE:
            case wim_protocol_error::MESSAGE_TOO_BIG:
            case wim_protocol_error::CAPCHA:
            case wim_protocol_error::MISSING_REQUIRED_PARAMETER:
                ptr_this->get_archive()->remove_message_from_not_sent(_message->get_aimid(), _message->get_message());
                break;
            case wim_protocol_error::MORE_AUTHN_REQUIRED:
            case wim_protocol_error::AUTHN_REQUIRED:
            case wim_protocol_error::SESSION_NOT_FOUND:    //Migration in process
            case wim_protocol_error::REQUEST_TIMEOUT:
                break;
            default:
                break;
            }
        }
    };

    return handler;
}

void im::post_not_sent_message_to_gui(int64_t _seq, const archive::not_sent_message_sptr& _message)
{
    coll_helper cl_coll(g_core->create_collection(), true);
    cl_coll.set_value_as_string("contact", _message->get_aimid());
    ifptr<iarray> messages_array(cl_coll->create_array());
    messages_array->reserve(1);
    coll_helper coll_message(cl_coll->create_collection(), true);
    _message->serialize(coll_message, auth_params_->time_offset_);
    ifptr<ivalue> val(cl_coll->create_value());
    val->set_as_collection(coll_message.get());
    messages_array->push_back(val.get());
    cl_coll.set_value_as_array("messages", messages_array.get());
    cl_coll.set<std::string>("my_aimid", auth_params_->aimid_);
    g_core->post_message_to_gui("archive/messages/pending", _seq, cl_coll.get());
}

void im::add_opened_dialog(const std::string& _contact)
{
    opened_dialogs_.insert(std::make_pair(_contact, archive::opened_dialog()));
}

void im::remove_opened_dialog(const std::string& _contact)
{
    opened_dialogs_.erase(_contact);
}

bool im::has_opened_dialogs(const std::string& _contact) const
{
    return (opened_dialogs_.find(_contact) != opened_dialogs_.end());
}

int64_t im::get_first_message(const std::string& _contact) const
{
    auto dialog = opened_dialogs_.find(_contact);
    if (dialog != opened_dialogs_.end())
        return dialog->second.get_first_msg_id();

    return -1;
}

void im::set_first_message(const std::string& _contact, int64_t _message)
{
    auto dialog = opened_dialogs_.find(_contact);
    if (dialog != opened_dialogs_.end())
    {
        if (dialog->second.get_first_msg_id() == -1 || dialog->second.get_first_msg_id() > _message)
        {
            dialog->second.set_first_msg_id(_message);
        }
    }
    else
    {
        opened_dialogs_[_contact].set_first_msg_id(_message);
    }
}

void im::set_last_read(const std::string& _contact, int64_t _message)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact, _message](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_local_dlg_state.get_fake())
        {
            return;
        }

        archive::dlg_state new_dlg_state = _local_dlg_state;

        if (new_dlg_state.get_yours_last_read() < _message || new_dlg_state.get_unread_count() != 0)
        {
            new_dlg_state.set_yours_last_read(_message);
            new_dlg_state.set_unread_count(0);

            ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result =
                [wr_this, _contact]
            (const archive::dlg_state &_local_dlg_state, const archive::dlg_state_changes&)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                set_dlg_state_params params;
                params.aimid_ = _contact;
                params.last_read_ = _local_dlg_state.get_yours_last_read();
                ptr_this->set_dlg_state(params)->on_result_ = [wr_this, _contact] (int32_t error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->post_dlg_state_to_gui(_contact);
                };
            };
        }
    };
}

void im::hide_dlg_state(const std::string& _contact)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        archive::dlg_state new_dlg_state = _local_dlg_state;

        new_dlg_state.set_visible(false);

        ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result = [](const archive::dlg_state&, const archive::dlg_state_changes&){};
    };
}

void im::delete_archive_messages(const int64_t _seq, const std::string &_contact_aimid, const std::vector<int64_t> &_ids, const bool _for_all)
{
    assert(_seq > 0);
    assert(!_ids.empty());
    assert(!_contact_aimid.empty());

    for (const auto id : _ids)
    {
        __INFO(
            "delete_history",
            "requested to delete a history message\n"
            "    message-id=<%1%>\n"
            "    contact=<%2%>\n"
            "    for_all=<%3%>",
            id % _contact_aimid % logutils::yn(_for_all)
            );

        auto packet = std::make_shared<core::wim::del_message>(make_wim_params(), id, _contact_aimid, _for_all);

        std::weak_ptr<wim::im> wr_this(shared_from_this());

        post_robusto_packet(packet)->on_result_ =
            [id, wr_this, _for_all](int32_t _error)
            {
                if (_error == 0)
                {
                    g_core->insert_event(_for_all ? core::stats::stats_event_names::message_delete_all
                        : core::stats::stats_event_names::message_delete_my);
                }
            };
    }
}

void im::delete_archive_messages_from(const int64_t _seq, const std::string &_contact_aimid, const int64_t _from_id)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(_from_id >= 0);

    __INFO(
        "delete_history",
        "requested to delete history messages\n"
        "    from-id=<%1%>\n"
        "    contact=<%2%>",
        _from_id % _contact_aimid
        );

    auto packet = std::make_shared<core::wim::del_history>(make_wim_params(), _from_id, _contact_aimid);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_robusto_packet(packet)->on_result_ =
        [_from_id, wr_this](int32_t _error)
    {

    };
}

std::string im::_get_protocol_uid() {
    return auth_params_->aimid_;
}

void im::post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) {
    auto packet = std::make_shared<core::wim::wim_webrtc>(make_wim_params(), msg);
    std::weak_ptr<wim::im> __this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, __this, msg](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = __this.lock();
        if (!!ptr_this) {
            auto response = packet->getRawData();
            bool success  = _error == 0 && !!response && response->available();
            ptr_this->on_voip_proto_ack(msg, success);
        } else {
            assert(false);
        }
    };
}

std::shared_ptr<wim::wim_packet> im::prepare_voip_msg(const std::string& data) {
    return std::make_shared<core::wim::wim_allocate>(make_wim_params(), data);
}

std::shared_ptr<wim::wim_packet> im::prepare_voip_pac(const voip_manager::VoipProtoMsg& data) {
    return std::make_shared<core::wim::wim_webrtc>(make_wim_params(), data);
}

void im::post_voip_alloc_to_server(const std::string& data) {
    auto packet = std::make_shared<core::wim::wim_allocate>(make_wim_params(), data);
    std::weak_ptr<wim::im> __this(shared_from_this());
    post_wim_packet(packet)->on_result_ = [packet, __this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = __this.lock();
        if (!!ptr_this) {
            auto response = packet->getRawData();
            if (!!response && response->available()) {
                uint32_t size = response->available();

                auto empty_callback = std::make_shared<auto_callback>([](int32_t){});
                ptr_this->on_voip_proto_msg(true, (const char*)response->read(size), size, std::move(empty_callback));
            }
        } else {
            assert(false);
        }
    };
}


void im::hide_chat_async(const std::string& _contact,  const int64_t _last_msg_id, std::function<void(int32_t)> _on_result)
{
    std::weak_ptr<core::wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::hide_chat>(make_wim_params(), _contact, _last_msg_id);

    post_wim_packet(packet)->on_result_ = [_contact, packet, wr_this, _on_result](int32_t _error)
    {
        if (_error == 0)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->active_dialogs_->remove(_contact);
            ptr_this->unfavorite(_contact);
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_string("contact", _contact);
            g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
        }

        _on_result(_error);
    };
}


void im::hide_chat(const std::string& _contact)
{
    std::weak_ptr<core::wim::im> wr_this(shared_from_this());

    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->hide_chat_async(_contact, _state.get_last_msgid(),[](int32_t _error)
        {

        });
    };
}

void im::mute_chats(std::shared_ptr<std::list<std::string>> _chats)
{
    if (!_chats->size())
    {
        tools::system::delete_file(get_exported_muted_chats_filename());

        return;
    }

    const std::string chat = *(_chats->begin());

    auto packet = std::make_shared<core::wim::mute_buddy>(make_wim_params(), chat, true);

    std::weak_ptr<core::wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, wr_this, _chats](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error != wim_protocol_internal_error::wpie_network_error)
        {
            _chats->pop_front();
        }

        ptr_this->mute_chats(_chats);
    };
}

std::shared_ptr<async_task_handlers> im::mute_chat_async(const std::string& _contact, bool _mute)
{
    auto packet = std::make_shared<core::wim::mute_buddy>(make_wim_params(), _contact, _mute);

    return post_wim_packet(packet);
}

void im::mute_chat(const std::string& _contact, bool _mute)
{
    mute_chat_async(_contact, _mute);
}

void im::upload_file_sharing_internal(const archive::not_sent_message_sptr& _not_sent)
{
    assert(_not_sent);

    __TRACE(
        "fs",
        "initiating internal file sharing upload\n"
        "	iid=<%1%>\n"
        "	contact=<%2%>\n"
        "	local_path=<%3%>",
        _not_sent->get_internal_id() %
        _not_sent->get_aimid() %
        _not_sent->get_file_sharing_local_path()
        );

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    std::weak_ptr<im> wr_this = shared_from_this();

    auto handler = get_loader().upload_file_sharing(
        _not_sent->get_internal_id(),
        core::tools::from_utf8(_not_sent->get_file_sharing_local_path()),
        make_wim_params()
        );

    const auto uploading_id = _not_sent->get_internal_id();

    handler->on_result = [wr_this, _not_sent, time, uploading_id](int32_t _error, const web_file_info& _info)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        __TRACE(
            "fs",
            "internal file sharing upload completed\n"
            "	iid=<%1%>\n"
            "	contact=<%2%>\n"
            "	filename=<%3%>",
            _not_sent->get_internal_id() %
            _not_sent->get_aimid() %
            _not_sent->get_file_sharing_local_path()
            );

        if (_error == (int32_t)loader_errors::network_error)
        {
            return;
        }

        ptr_this->get_archive()->remove_message_from_not_sent(_not_sent->get_aimid(), _not_sent->get_message());

        const auto success = (_error == (int32_t)loader_errors::success);
        if (success)
        {
            ptr_this->send_message_to_contact(
                0,
                _not_sent->get_aimid(),
                _info.get_file_url(),
                message_type::file_sharing,
                _not_sent->get_internal_id(),
                _not_sent->get_quotes()
                );
        }

        coll_helper cl_coll(g_core->create_collection(), true);
        _info.serialize(cl_coll.get());
        cl_coll.set<int32_t>("error", _error);

        file_sharing_content_type content_type = file_sharing_content_type::undefined;
        tools::get_content_type_from_uri(_info.get_file_url(), Out content_type);
        cl_coll.set<int32_t>("content_type", static_cast<int32_t>(content_type));

        cl_coll.set<std::string>("local_path", _not_sent->get_file_sharing_local_path());
        cl_coll.set<std::string>("uploading_id", uploading_id);
        g_core->post_message_to_gui("files/upload/result", 0, cl_coll.get());
    };

    handler->on_progress =
        [uploading_id]
    (const web_file_info& _info)
    {
        coll_helper cl_coll(g_core->create_collection(), true);
        _info.serialize(cl_coll.get());
        cl_coll.set<std::string>("uploading_id", uploading_id);
        g_core->post_message_to_gui("files/upload/progress", 0, cl_coll.get());
    };
}


void im::resume_all_file_sharing_uploading()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_pending_file_sharing()->on_result =
        [wr_this](const std::list<archive::not_sent_message_sptr>& _messages)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            for (const auto& _message : _messages)
            {
                ptr_this->resume_file_sharing_uploading(_message);
            }
        };

    get_loader().resume_file_sharing_tasks();
}


void im::resume_file_sharing_uploading(const archive::not_sent_message_sptr &_not_sent)
{
    assert(_not_sent);

    if (get_loader().has_file_sharing_task(_not_sent->get_internal_id()))
    {
        return;
    }

    const auto &msg = _not_sent->get_message();

    assert(msg->is_file_sharing());
    assert(msg->get_flags().flags_.outgoing_);

    const auto &fs_data = msg->get_file_sharing_data();

    const std::wstring local_path = tools::from_utf8(fs_data->get_local_path());

    const auto file_still_exists = core::tools::system::is_exist(local_path);
    if (!file_still_exists)
    {
        get_archive()->remove_message_from_not_sent(_not_sent->get_aimid(), msg);

        return;
    }

    post_not_sent_message_to_gui(0, _not_sent);

    upload_file_sharing_internal(_not_sent);

    return;
}

void im::upload_file_sharing(int64_t _seq, const std::string& _contact, const std::string& _file_name, std::shared_ptr<core::tools::binary_stream> _data, const std::string& _extension)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    std::string file_name = _file_name;
    if (_data->available() > 0)
    {
        auto path = tools::system::create_temp_file_path();
        path += tools::from_utf8(_extension);
        if (_data->save_2_file(path))
            file_name = tools::from_utf16(path);
    }

    __INFO(
        "fs",
        "initiating file sharing upload\n"
        "	seq=<%1%>\n"
        "	contact=<%2%>\n"
        "	filename=<%3%>",
        _seq % _contact % file_name);

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    auto not_sent = archive::not_sent_message::make_outgoing_file_sharing(_contact, time, file_name);

    auto handler = get_archive()->insert_not_sent_message(_contact, not_sent);
    handler->on_result_ =
        [wr_this, not_sent, _seq, file_name, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        assert(_error == 0);

        ptr_this->post_not_sent_message_to_gui(_seq, not_sent);

        ptr_this->upload_file_sharing_internal(not_sent);
    };
}

void im::get_file_sharing_preview_size(const int64_t _seq, const std::string& _file_url)
{
    coll_helper cl_coll(g_core->create_collection(), true);

    cl_coll.set<std::string>("file_url", _file_url);

    std::string file_id;

    if (!tools::parse_new_file_sharing_uri(_file_url, Out file_id))
    {
        cl_coll.set<int32_t>("error", 1);
        g_core->post_message_to_gui("files/error", _seq, cl_coll.get());
        return;
    }

    assert(!file_id.empty());

    auto type = file_sharing_content_type::undefined;
    tools::get_content_type_from_file_sharing_id(file_id, Out type);

    if (is_snap_file_sharing_content_type(type))
    {
        request_snap_metainfo(_seq, _file_url, file_id);
        return;
    }

    const auto mini_preview_uri = tools::format_file_sharing_preview_uri(file_id, tools::file_sharing_preview_size::small);
    assert(!mini_preview_uri.empty());

    const auto full_preview_uri = tools::format_file_sharing_preview_uri(file_id, tools::file_sharing_preview_size::normal);
    assert(!full_preview_uri.empty());

    cl_coll.set<std::string>("mini_preview_uri", mini_preview_uri);
    cl_coll.set<std::string>("full_preview_uri", full_preview_uri);

    g_core->post_message_to_gui("files/get_preview_size/result", _seq, cl_coll.get());
}

void im::download_file_sharing_metainfo(const int64_t _seq, const std::string& _file_url)
{
    get_async_loader().download_file_sharing_metainfo(_file_url, make_wim_params(), file_sharing_meta_handler_t(
        [_seq, _file_url](loader_errors _error, const file_sharing_meta_data_t& _data)
        {
            coll_helper cl_coll(g_core->create_collection(), true);

            if (_error != loader_errors::success)
            {
                cl_coll.set<std::string>("file_url", _file_url);
                cl_coll.set<int32_t>("error", static_cast<int32_t>(_error));
                g_core->post_message_to_gui("files/error", _seq, cl_coll.get());
                return;
            }

            auto meta = _data.additional_data_;

            cl_coll.set_value_as_string("file_name_short", meta->file_name_short_);
            cl_coll.set_value_as_string("file_dlink", meta->file_download_url_);
            cl_coll.set_value_as_int64("file_size", meta->file_size_);

            g_core->post_message_to_gui("files/metainfo/result", _seq, cl_coll.get());
        }));
}

void im::download_file_sharing(const int64_t _seq, const std::string& _contact, const std::string& _file_url, const bool _force_request_metainfo, const std::string& _filename, const std::string& _download_dir, bool _raise_priority)
{
    get_async_loader().set_download_dir(_download_dir.empty() ? get_im_downloads_path(_download_dir) : tools::from_utf8(_download_dir));

    if (_force_request_metainfo)
    {
        tools::system::delete_file(get_path_in_cache(get_content_cache_path(), _file_url, path_type::link_meta));
    }

    auto progress_callback = file_info_handler_t::progress_callback_t([_seq, _file_url](int64_t _total, int64_t _transferred, int32_t _completion_percent)
    {
        coll_helper coll(g_core->create_collection(), true);

        coll.set<std::string>("file_url", _file_url);
        coll.set<int64_t>("file_size", _total);
        coll.set<int64_t>("bytes_transfer", _transferred);

        g_core->post_message_to_gui("files/download/progress", _seq, coll.get());
    });

    auto completion_callback = file_info_handler_t::completion_callback_t(
        [_seq, _file_url](loader_errors _error, const file_info_data_t& _data)
        {
            coll_helper coll(g_core->create_collection(), true);

            if (_error != loader_errors::success)
            {
                coll.set<std::string>("file_url", _file_url);
                coll.set<int32_t>("error", static_cast<int32_t>(_error));
                g_core->post_message_to_gui("files/error", _seq, coll.get());
                return;
            }

            coll.set<std::string>("file_url", _data.additional_data_->url_);
            coll.set<std::string>("file_name", tools::from_utf16(_data.additional_data_->local_path_));

            g_core->post_message_to_gui("files/download/result", _seq, coll.get());
        });

    get_async_loader().download_file_sharing(_raise_priority ? highest_priority : default_priority, _contact, _file_url, _filename, make_wim_params(), file_info_handler_t(completion_callback, progress_callback));
}

void im::download_image(
    int64_t _seq,
    const std::string& _contact_aimid,
    const std::string& _image_url,
    const std::string& _forced_path,
    const bool _download_preview,
    const int32_t _preview_width,
    const int32_t _preview_height,
    const bool _raise_priority)
{
    assert(_seq > 0);
    assert(!_image_url.empty());
    assert(_preview_width >= 0);
    assert(_preview_width < 1000);
    assert(_preview_height >= 0);
    assert(_preview_height < 1000);

    __INFO("async_loader",
        "im::download_image\n"
        "seq      = <%1%>\n"
        "url      = <%2%>\n"
        "preview  = <%3%>\n", _seq % _image_url % logutils::yn(_download_preview));

    auto metainfo_handler = link_meta_handler_t([_seq, _image_url](loader_errors _error, const link_meta_data_t& _data)
    {
        __INFO("async_loader",
            "metainfo_handler\n"
            "seq      = <%1%>\n"
            "url      = <%2%>\n"
            "result   = <%3%>\n", _seq % _image_url % static_cast<int>(_error));

        if (_error != loader_errors::success)
            return;

        auto meta = _data.additional_data_;

        const auto width = std::get<0>(meta->get_preview_size());
        const auto height = std::get<1>(meta->get_preview_size());
        const auto& uri = meta->get_download_uri();
        const auto size = meta->get_file_size();

        coll_helper cl_coll(g_core->create_collection(), true);

        cl_coll.set<int32_t>("preview_width", width);
        cl_coll.set<int32_t>("preview_height", height);
        cl_coll.set<std::string>("download_uri", uri);
        cl_coll.set<int64_t>("file_size", size);

        g_core->post_message_to_gui("image/download/result/meta", _seq, cl_coll.get());
    });

    auto progress_callback = file_info_handler_t::progress_callback_t([_seq](int64_t _total, int64_t _transferred, int32_t _completion_percent)
    {
        coll_helper coll(g_core->create_collection(), true);

        coll.set<int64_t>("bytes_total", _total);
        coll.set<int64_t>("bytes_transferred", _transferred);
        coll.set<int32_t>("pct_transferred", _completion_percent);

        g_core->post_message_to_gui("image/download/progress", _seq, coll.get());
    });

    auto completion_callback = file_info_handler_t::completion_callback_t([_seq, _image_url](loader_errors _error, const file_info_data_t& _data)
    {
        __INFO("async_loader",
            "image_handler\n"
            "seq      = <%1%>\n"
            "url      = <%2%>\n"
            "result   = <%3%>\n", _seq % _image_url % static_cast<int>(_error));

        coll_helper cl_coll(g_core->create_collection(), true);

        cl_coll.set<int32_t>("error", static_cast<int32_t>(_error));

        auto image = _data.get_content_as_core_istream(cl_coll);
        cl_coll.set_value_as_stream("data", image.get());

        auto file_info = _data.additional_data_;

        const auto& url = file_info->url_;
        cl_coll.set<std::string>("url", url);

        const auto& local_path = file_info->local_path_;
        cl_coll.set<std::string>("local", tools::from_utf16(local_path));

        g_core->post_message_to_gui("image/download/result", _seq, cl_coll.get());
    });

    if (_download_preview)
    {
        get_async_loader().download_image_preview(_raise_priority ? high_priority : default_priority, _image_url, make_wim_params(), metainfo_handler, file_info_handler_t(completion_callback));
    }
    else
    {
        get_async_loader().download_image(_raise_priority ? high_priority : default_priority, _image_url, _forced_path, make_wim_params(), file_info_handler_t(completion_callback, progress_callback));
    }
}

void im::download_link_preview(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const std::string& _url,
    const int32_t _preview_width,
    const int32_t _preview_height)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(!_url.empty());
    assert(_preview_width >= 0);
    assert(_preview_width < 1000);
    assert(_preview_height >= 0);
    assert(_preview_height < 1000);

    std::weak_ptr<im> wr_this = shared_from_this();

    get_async_loader().download_image_metainfo(_url, make_wim_params(), link_meta_handler_t(
        [_seq, _contact_aimid, _preview_width, _preview_height, wr_this](loader_errors _error, const link_meta_data_t _data)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
            {
                return;
            }

            auto meta = _data.additional_data_;

            const bool success = meta && (_error == loader_errors::success);

            coll_helper cl_coll(g_core->create_collection(), true);

            cl_coll.set<bool>("success", success);

            if (success)
            {
                const auto preview_size = meta->get_preview_size();

                cl_coll.set<std::string>("title", meta->get_title());
                cl_coll.set<std::string>("annotation", meta->get_annotation());
                cl_coll.set<std::string>("site_name", meta->get_site_name());
                cl_coll.set<std::string>("content_type", meta->get_content_type());
                cl_coll.set<std::string>("download_uri", meta->get_download_uri());
                cl_coll.set<int32_t>("preview_width", std::get<0>(preview_size));
                cl_coll.set<int32_t>("preview_height", std::get<1>(preview_size));

                g_core->execute_core_context([_seq, _contact_aimid, _preview_width, _preview_height, meta, wr_this]()
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_link_preview_image(_seq, _contact_aimid, *meta, _preview_width, _preview_height);
                });

                g_core->execute_core_context([_seq, _contact_aimid, meta, wr_this]()
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_link_preview_favicon(_seq, _contact_aimid, *meta);
                });
            }

            g_core->post_message_to_gui("link_metainfo/download/result/meta", _seq, cl_coll.get());
        }));
}

void im::cancel_loader_task(const std::string& _url)
{
    get_async_loader().cancel(_url);
}

void im::abort_file_sharing_download(const std::string& _url)
{
    get_async_loader().cancel_file_sharing(_url);
}

void im::raise_download_priority(const int64_t _task_id)
{
    assert(_task_id > 0);

//TODO loader    get_loader().raise_task_priority(_task_id);
}

void im::contact_switched(const std::string &_contact_aimid)
{
    get_async_loader().contact_switched(_contact_aimid);
}

void im::abort_file_sharing_upload(
    const int64_t _seq,
    const std::string &_contact,
    const std::string &_process_seq)
{
    assert(_seq > 0);
    assert(!_process_seq.empty());
    assert(!_contact.empty());

    std::weak_ptr<im> wr_this = shared_from_this();

    auto history_message = std::make_shared<archive::history_message>();
    history_message->set_internal_id(_process_seq);

    get_archive()->remove_message_from_not_sent(_contact, history_message)->on_result_ =
        [wr_this, _process_seq](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_loader().abort_file_sharing_process(_process_seq);
    };
}

void im::download_link_preview_image(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const preview_proxy::link_meta &_meta,
    const int32_t _preview_width,
    const int32_t _preview_height)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());
    assert(_preview_width >= 0);
    assert(_preview_width < 2000);
    assert(_preview_height >= 0);
    assert(_preview_height < 2000);

    if (!_meta.has_preview_uri())
    {
        return;
    }

    const auto image_uri = _meta.get_preview_uri(_preview_width, _preview_height);

    get_async_loader().download_image(default_priority, image_uri, make_wim_params(),
        file_info_handler_t([_seq](loader_errors _error, const file_info_data_t& _data)
        {
            coll_helper cl_coll(g_core->create_collection(), true);

            const auto success = (_error == loader_errors::success);

            cl_coll.set<bool>("success", success);

            if (success)
            {
                auto image = _data.get_content_as_core_istream(cl_coll);
                cl_coll.set_value_as_stream("data", image.get());
            }

            g_core->post_message_to_gui("link_metainfo/download/result/image", _seq, cl_coll.get());
        }));
}

void im::download_link_preview_favicon(
    const int64_t _seq,
    const std::string& _contact_aimid,
    const preview_proxy::link_meta &_meta)
{
    assert(_seq > 0);
    assert(!_contact_aimid.empty());

    const auto &favicon_uri = _meta.get_favicon_uri();

    if (favicon_uri.empty())
    {
        return;
    }

    const auto favicon_callback = file_info_handler_t([_seq, favicon_uri](loader_errors _error, const file_info_data_t& _data)
    {
        __INFO("async_loader",
            "favicon_callback\n"
            "seq      = <%1%>\n"
            "url      = <%2%>\n"
            "result   = <%3%>\n", _seq % favicon_uri % static_cast<int>(_error));

        coll_helper cl_coll(g_core->create_collection(), true);

        const auto success = (_error == loader_errors::success);
        cl_coll.set<bool>("success", success);

        if (success)
        {
            ifptr<istream> data = _data.get_content_as_core_istream(cl_coll);
            cl_coll.set_value_as_stream("data", data.get());
        }

        g_core->post_message_to_gui("link_metainfo/download/result/favicon", _seq, cl_coll.get());
    });

    get_async_loader().download_image(default_priority, favicon_uri, make_wim_params(), favicon_callback);
}

void im::request_snap_metainfo(const int64_t _seq, const std::string &_uri, const std::string& _ttl_id)
{
    assert(_seq > 0);
    assert(!_ttl_id.empty());

    get_async_loader().download_snap_metainfo(_ttl_id, make_wim_params(), snap_meta_handler_t(
        [_seq, _uri](loader_errors _error, const snap_meta_data_t& _data)
        {
            coll_helper cl_coll(g_core->create_collection(), true);

            cl_coll.set<std::string>("file_url", _uri);

            if (_error != loader_errors::success)
            {
                cl_coll.set<int32_t>("error", static_cast<int32_t>(_error));
                g_core->post_message_to_gui("files/error", _seq, cl_coll.get());
                return;
            }

            auto meta = _data.additional_data_;
            if (meta)
            {
                cl_coll.set<std::string>("mini_preview_uri", meta->get_mini_preview_uri());
                cl_coll.set<std::string>("full_preview_uri", meta->get_full_preview_uri());
            }
            else
            {
                cl_coll.set<std::string>("mini_preview_uri", "");
                cl_coll.set<std::string>("full_preview_uri", "");
            }

            g_core->post_message_to_gui("files/get_preview_size/result", _seq, cl_coll.get());
        }));
}

void im::set_played(const std::string& url, bool played)
{
    get_loader().set_played(
        url,
        get_content_cache_path(),
        played,
        make_wim_params());
}

void im::speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::speech_to_text>(make_wim_params(), _url, _locale);

    post_wim_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        coll.set_value_as_int("comeback", packet->get_comeback());
        coll.set_value_as_string("text", packet->get_text());
        g_core->post_message_to_gui("files/speech_to_text/result", _seq, coll.get());
    };
}

wim::loader& im::get_loader()
{
    if (!files_loader_)
    {
        const auto previews_folder = get_content_cache_path();

        files_loader_ = std::make_shared<wim::loader>(previews_folder);
    }

    return *files_loader_;
}

wim::async_loader& im::get_async_loader()
{
    if (!async_loader_)
    {
        const auto content_cache_dir = get_content_cache_path();

        async_loader_ = std::make_shared<wim::async_loader>(content_cache_dir);
    }

    return *async_loader_;
}

void im::search_contacts(int64_t _seq, const std::string& keyword, const std::string& phonenumber, const std::string& tag)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    auto packet = std::make_shared<core::wim::search_contacts>(make_wim_params(), keyword, phonenumber, tag);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        coll_helper coll(g_core->create_collection(), true);
        if (_error == 0)
            packet->response_.serialize(coll);
        else
            coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("contacts/search/result", _seq, coll.get());
    };
}

void im::get_profile(int64_t _seq, const std::string& _contact)
{
    auto aimid = (_contact.empty() ? auth_params_->aimid_ : _contact);
    if (aimid.empty())
        return;

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::get_profile>(make_wim_params(), aimid);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _contact](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);

        if (_error == 0)
        {
            const profile::profiles_list& results = packet->get_result();

            if (!results.size())
            {
                _error = wim_protocol_internal_error::wpie_error_profile_not_found;
            }
            else
            {
                coll_helper coll_profile(coll->create_collection(), true);

                (*(results.begin()))->serialize(coll_profile);

                coll.set_value_as_collection("profile", coll_profile.get());
            }
        }

        coll.set_value_as_string("aimid", _contact);
        coll.set_value_as_int("error", _error);


        g_core->post_message_to_gui("contacts/profile/get/result", _seq, coll.get());
    };
}

void im::add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message)
{
    archive::history_message message;
    message.set_text(_auth_message);
    message.set_internal_id(core::tools::system::generate_internal_id());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;
    message.set_time(time);

    archive::dlg_state state;
    state.set_last_message(message);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    get_archive()->set_dlg_state(_aimid, state)->on_result = [wr_this, _aimid, _group, _auth_message, _seq](const archive::dlg_state&, const archive::dlg_state_changes&)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->post_dlg_state_to_gui(_aimid);

        auto packet = std::make_shared<core::wim::add_buddy>(ptr_this->make_wim_params(), _aimid, _group, _auth_message);

        ptr_this->post_wim_packet(packet)->on_result_ = [packet, _seq, _aimid](int32_t _error)
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_int("error", _error);
            coll.set_value_as_string("contact", _aimid);

            g_core->post_message_to_gui("contacts/add/result", _seq, coll.get());
        };
    };
}

void im::remove_contact(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    get_archive()->get_dlg_state(_aimid)->on_result = [wr_this, _aimid, _seq](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        int64_t last_msg_id = _state.get_last_msgid();

        ptr_this->active_dialogs_->remove(_aimid);
        ptr_this->unfavorite(_aimid);

        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set_value_as_string("contact", _aimid);
        g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());

        ptr_this->get_archive()->clear_dlg_state(_aimid)->on_result_ = [wr_this, _seq, _aimid, last_msg_id](int32_t _err)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->hide_chat_async(_aimid, last_msg_id, [wr_this, _seq, _aimid](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
                {
                    coll_helper coll(g_core->create_collection(), true);
                    coll.set_value_as_int("error", _error);
                    coll.set_value_as_string("contact", _aimid);

                    g_core->post_message_to_gui("contacts/remove/result", _seq, coll.get());

                    if (_aimid.find("@chat.agent") != _aimid.npos)
                    {
                        g_core->insert_event(core::stats::stats_event_names::groupchat_leave);
                    }
                };

                auto packet = std::make_shared<core::wim::remove_buddy>(ptr_this->make_wim_params(), _aimid);

                if (ptr_this->contact_list_->exist(_aimid))
                {
                    ptr_this->post_wim_packet(packet)->on_result_ = on_result;
                }
                else
                {
                    on_result(0);
                }
            });
        };
    };
}


void im::rename_contact(int64_t _seq, const std::string& _aimid, const std::string& _friendly)
{
    auto packet = std::make_shared<core::wim::set_buddy_attribute>(make_wim_params(), _aimid, _friendly);

    post_wim_packet(packet)->on_result_ = [_aimid, _friendly, _seq](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);

        coll.set_value_as_int("error", _error);
        coll.set_value_as_string("contact", _aimid);
        coll.set_value_as_string("friendly", _friendly);

        g_core->post_message_to_gui("contacts/rename/result", _seq, coll.get());
    };
}


void im::post_unignored_contact_to_gui(const std::string& _aimid)
{
    if (contact_list_->exist(_aimid))
    {
        coll_helper coll(g_core->create_collection(), true);

        coll.set_value_as_string("type", "created");

        contact_list_->serialize_contact(_aimid, coll.get());

        g_core->post_message_to_gui("contacts/ignore/remove", 0, coll.get());
    }
}


void im::ignore_contact(int64_t _seq, const std::string& _aimid, bool _ignore)
{
    const auto oper = _ignore ? set_permit_deny::operation::ignore : set_permit_deny::operation::ignore_remove;

    auto packet = std::make_shared<core::wim::set_permit_deny>(make_wim_params(), _aimid, oper);

    std::weak_ptr<im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [_ignore, wr_this, _aimid, _seq](int32_t _error)
    {
        if (_error == 0)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_ignore)
            {
                ptr_this->contact_list_->add_to_ignorelist(_aimid);

                ptr_this->post_ignorelist_to_gui(0);
            }
            else
            {
                ptr_this->contact_list_->remove_from_ignorelist(_aimid);

                ptr_this->post_ignorelist_to_gui(_seq);

                ptr_this->post_unignored_contact_to_gui(_aimid);
            }
        }
    };
}



void im::get_ignore_list(int64_t _seq)
{
    post_ignorelist_to_gui(_seq);
}

void im::favorite(const std::string& _contact)
{
    core::wim::favorite fvrt(_contact, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - auth_params_->time_offset_);
    favorites_->update(fvrt);

    post_dlg_state_to_gui(_contact, true, true, true);

    g_core->insert_event(core::stats::stats_event_names::favorites_set);
}

void im::unfavorite(const std::string& _contact)
{
    favorites_->remove(_contact);

    if (active_dialogs_->contains(_contact))
        post_dlg_state_to_gui(_contact, false, true, true);

    g_core->insert_event(core::stats::stats_event_names::favorites_unset);
}

void im::on_event_permit(fetch_event_permit* _event, std::shared_ptr<auto_callback> _on_complete)
{
    contact_list_->update_ignorelist(_event->ignore_list());
    need_update_search_cache();

    post_ignorelist_to_gui(0);

    _on_complete->callback(0);
}

void im::on_event_imstate(fetch_event_imstate* _event, std::shared_ptr<auto_callback> _on_complete)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    const auto& imstates = _event->get_states();

    for (const auto& _state : imstates)
    {
        if (_state.get_state() == imstate_sent_state::failed)
        {
            get_archive()->failed_pending_message(_state.get_request_id())->on_result_ = [](int32_t _error)
            {

            };
        }
        else
        {
            get_archive()->update_pending_messages_by_imstate(
                _state.get_request_id(),
                _state.get_hist_msg_id(),
                _state.get_before_hist_msg_id())->on_result = [wr_this, _state](archive::not_sent_message_sptr _message)
            {

                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_message)
                {
                    coll_helper coll(g_core->create_collection(), true);

                    auto messages_block = std::make_shared<archive::history_block>();
                    messages_block->push_back(_message->get_message());

                    coll.set_value_as_string("contact", _message->get_aimid());
                    coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
                    serialize_messages_4_gui("messages", messages_block, coll.get(), ptr_this->auth_params_->time_offset_);

                    g_core->post_message_to_gui("messages/received/message_status", 0, coll.get());

                    ptr_this->get_archive()->update_history(_message->get_aimid(), messages_block);
                }
            };
        }
    }

    get_archive()->sync_with_history()->on_result_ = [_on_complete](int32_t _error)
    {
        _on_complete->callback(0);
    };
}

void im::on_event_notification(fetch_event_notification* _event, std::shared_ptr<auto_callback> _on_complete)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto notify_callback = [wr_this](core::coll_helper collection, mailbox_change::type type)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        switch (type)
        {
        case core::wim::mailbox_change::status:
            g_core->post_message_to_gui("mailboxes/status", 0, collection.get());
            break;
        case core::wim::mailbox_change::new_mail:
            g_core->post_message_to_gui("mailboxes/new", 0, collection.get());
            break;

        case core::wim::mailbox_change::mail_read:
        default:
            break;
        }
    };

    mailbox_storage_->process(_event->changes_, notify_callback);
    _on_complete->callback(0);
}

void im::on_event_snaps(fetch_event_snaps* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto aimId = _event->aim_id();
    bool needUpdateSnaps = false;

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto get_user_snaps = [wr_this, aimId]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto snaps_packet = std::make_shared<core::wim::get_user_snaps>(ptr_this->make_wim_params(), aimId);
        ptr_this->post_robusto_packet(snaps_packet)->on_result_ = [wr_this, snaps_packet, aimId](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            coll_helper coll(g_core->create_collection(), true);
            ptr_this->snaps_storage_->update_user_snaps(aimId, snaps_packet->result_, coll.get());
            g_core->post_message_to_gui("snaps/user_snaps", 0, coll.get());
        };
    };

    if (snaps_storage_->is_user_exist(aimId))
    {
        coll_helper coll(g_core->create_collection(), true);
        snaps_storage_->update_snap_state(aimId, _event->state(), needUpdateSnaps, coll.get());

        if (needUpdateSnaps)
        {
            get_user_snaps();
        }

        if (!coll->empty())
            g_core->post_message_to_gui("snaps/user_snaps_state", 0, coll.get());
    }
    else
    {
        get_user_snaps();
    }
    _on_complete->callback(0);
}

void im::spam_contact(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    // load from local storage
    get_archive()->get_messages(_aimid, -1, 10, -1 /* _to_older */)->on_result = [_seq, wr_this, _aimid](std::shared_ptr<archive::history_block> _messages)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        std::string message_text;
        time_t message_time = 0;

        for (auto iter_message = _messages->rbegin(); iter_message != _messages->rend(); ++iter_message)
        {
            if (!(*iter_message)->get_flags().flags_.outgoing_)
            {
                message_text = (*iter_message)->get_text();
                message_time = (*iter_message)->get_time();
                break;
            }
        }

        auto packet = std::make_shared<core::wim::spam_report>(
            ptr_this->make_wim_params(),
            message_text,
            _aimid,
            ptr_this->auth_params_->aimid_,
            message_time);

        ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, _aimid, _seq](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto packet = std::make_shared<core::wim::set_permit_deny>(ptr_this->make_wim_params(), _aimid, set_permit_deny::operation::ignore);
            ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, _aimid, _seq](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->remove_contact(_seq, _aimid);
            };
        };
    };
}

void im::send_statistic_if_needed()
{
    if (imstat_->need_send())
    {
        auto packet = imstat_->get_statistic(make_wim_params());
        if (packet)
            post_wim_packet(packet);
    }
}

std::string im::get_login()
{
    return auth_params_->aimid_;
}

void post_theme_2_gui(int64_t _seq, int32_t _theme_id, tools::binary_stream& _data)
{
    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_int("theme_id", _theme_id);

    ifptr<istream> theme_image_data(coll->create_stream(), true);
    const auto data_size = _data.available();
    if (data_size)
    {
        theme_image_data->write((const uint8_t*)_data.read(data_size), data_size);
        coll.set_value_as_stream("image", theme_image_data.get());
    }
    else
    {
        coll.set_value_as_int("failed", 1);
    }
    g_core->post_message_to_gui("themes/theme/get/result", _seq, coll.get());
}

void im::get_theme(int64_t _seq, int32_t _theme_id)
{
    get_themes()->get_theme_image(_seq, _theme_id)->on_result_ =
        [_seq, _theme_id, this](tools::binary_stream& _theme_data)
    {
        if (_theme_data.available() <= 0)
        {
            this->download_themes(0);
            this->download_themes_meta(0);
        }
        post_theme_2_gui(_seq, _theme_id, _theme_data);
    };
}

themes::theme* im::get_theme_from_cache(int32_t _theme_id)
{
    return get_themes()->get_theme(_theme_id);
}

void im::get_themes_meta(int64_t _seq, const ThemesScale _themes_scale)
{
    get_themes()->set_themes_scale(_themes_scale);
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_themes()->load_meta_from_local()->on_result_ = [wr_this, _seq](const themes::load_result& _res)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto etag = _res.get_result() ? g_core->load_themes_etag() : "";
        auto packet = std::make_shared<get_themes_index>(ptr_this->make_wim_params(), etag);
        ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, packet, _seq, etag](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0)
            {
                auto response = packet->get_response();
                auto header_etag = packet->get_header_etag();

                ptr_this->get_themes()->parse(response, false)->on_result_ = [response, wr_this, _seq, header_etag, etag](const themes::parse_result& _res)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    if (_res.get_result() && header_etag != etag)
                    {
                        g_core->save_themes_etag(header_etag);
                        ptr_this->get_themes()->clear_all();

                        response->reset_out();
                        ptr_this->get_themes()->save(response)->on_result_ = [wr_this, _seq](bool _res)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            ptr_this->load_themes_meta(_seq);
                        };
                    }
                    else
                    {
                        coll_helper _coll(g_core->create_collection(), true);
                        ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
                        };
                    }
                };
            }
            else
            {
                coll_helper _coll(g_core->create_collection(), true);
                ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
                };
            }
        };
    };
}

void im::download_themes_meta(int64_t _seq)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_themes()->get_next_meta_task()->on_result_ = [wr_this, _seq](bool _res, const themes::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            coll_helper _coll(g_core->create_collection(), true);

            ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
            };

            ptr_this->download_themes(_seq);

            return;
        }

        ptr_this->get_async_loader().download_file(default_priority, _task.get_source_url(), _task.get_dest_file(), ptr_this->make_wim_params(), file_info_handler_t(
            [wr_this, _seq, _task](loader_errors _error, const file_info_data_t& _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->get_themes()->on_metadata_loaded(_task)->on_result_ = [wr_this, _seq](bool)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->download_themes_meta(_seq);
                };
            }));
    };
}

void im::load_themes_meta(int64_t _seq)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_themes()->make_download_tasks()->on_result_ = [wr_this, _seq](bool)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_themes_meta(_seq);
    };
}

void im::download_themes(int64_t _seq)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_themes()->get_next_theme_task()->on_result_ = [wr_this, _seq](bool _res, const themes::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
            return;

        auto theme_id = _task.get_theme_id();

        ptr_this->get_async_loader().download_file(default_priority, _task.get_source_url(), _task.get_dest_file(), ptr_this->make_wim_params(), file_info_handler_t(
            [wr_this, _seq, _task, theme_id](loader_errors _error, const file_info_data_t& _data)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->get_themes()->on_theme_loaded(_task)->on_result_ = [wr_this, _seq, _error, theme_id]
                (bool _res, const std::list<int64_t>& _requests)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    if (_res && !_requests.empty() && _error == loader_errors::success)
                    {
                        ptr_this->get_themes()->get_theme_image(_seq, theme_id)->on_result_ = [_requests, theme_id, _seq](tools::binary_stream& _data)
                        {
                            post_theme_2_gui(_seq, theme_id, _data);
                        };
                    }

                    ptr_this->download_themes(_seq);
                };
            }));
    };
}
void im::load_flags(const int64_t _seq)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::get_flags>(make_wim_params());

    std::function<void(int32_t)> on_result = [_seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("flags", packet->flags());
        g_core->post_message_to_gui("recv_flags", _seq, coll.get());
    };

    post_wim_packet(packet)->on_result_ = on_result;
}


void im::update_profile(int64_t _seq, const std::vector<std::pair<std::string, std::string>>& _field)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::update_profile>(make_wim_params(), _field);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("update_profile/result", _seq, coll.get());
    };
}

void im::join_live_chat(int64_t _seq, const std::string& _stamp, const int _age)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::join_chat_alpha>(make_wim_params(), _stamp, _age);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("livechat/join/result", _seq, coll.get());
    };
}

std::shared_ptr<async_task_handlers> im::send_timezone()
{
    auto handler = std::make_shared<async_task_handlers>();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::set_timezone>(make_wim_params());

    post_wim_packet(packet)->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(_error);
    };

    return handler;
}

void im::prefetch_last_dialog_messages(const std::string &_dlg_aimid, const char* const _reason)
{
    assert(!_dlg_aimid.empty());
    assert(_reason);

    __TRACE(
        "prefetch",
        "requesting last dialog messages to prefetch\n"
        "    contact=<%1%>\n"
        "    count=<%2%>\n"
        "    reason=<%3%>",
        _dlg_aimid % PREFETCH_COUNT % _reason);

    get_archive()->get_messages(_dlg_aimid, -1, PREFETCH_COUNT, -1 /* _to_older */);
}

namespace
{
    bool should_sign_url(const std::string &_url)
    {
        return false;
    }
}

void im::set_avatar(const int64_t _seq, tools::binary_stream image, const std::string& _aimId, const bool _chat)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::set_avatar>(make_wim_params(), image, _aimId, _chat);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _aimId](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error != 0)
        {
            if (_aimId.empty())
            {
                g_core->insert_event(core::stats::stats_event_names::introduce_avatar_fail);
            }
        }

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        coll.set_value_as_int64("seq", _seq);
        coll.set_value_as_string("id", packet->get_id());
        g_core->post_message_to_gui("set_avatar/result", _seq, coll.get());
    };
}

void im::load_hosts_config()
{
    if (!g_core->get_hosts_config().need_update())
    {
        return;
    }

    const std::string config_url = "https://s3.amazonaws.com/icqlive/config.txt";

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::get_hosts_config>(make_wim_params(), config_url);

    g_core->get_hosts_config().update_last_request_time();

    post_wim_packet(packet)->on_result_ = [wr_this, packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            g_core->get_hosts_config().update_hosts(packet->get_hosts(), false);
        }
    };

}

void im::need_update_search_cache()
{
    if (contact_list_->get_need_update_search_cache())
    {
        g_core->post_message_to_gui("search_need_update", 0, nullptr);
    }
}

bool im::wait_function(int32_t _timeout)
{
    auto ptr_stop = stop_objects_weak_.lock();
    if (!ptr_stop)
        return false;

    std::unique_lock<std::mutex> lock(ptr_stop->stop_mutex_);
    if (ptr_stop->condition_poll_.wait_for(lock, std::chrono::milliseconds(_timeout)) != std::cv_status::timeout)
        return false;

    return true;
}

bool im::has_valid_login() const
{
    return auth_params_->is_valid();
}
