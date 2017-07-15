#include "stdafx.h"
#include "statistics.h"

#include "core.h"
#include "tools/binary_stream.h"
#include "tools/strings.h"
#include "tools/system.h"
#include "tools/tlv.h"
#include "../external/curl/include/curl.h"
#include "http_request.h"
#include "tools/hmac_sha_base64.h"
#include "async_task.h"
#include "utils.h"
#include "../corelib/enumerations.h"
#include "../common.shared/keys.h"

using namespace core::stats;
using namespace core;

enum statistics_info_types
{
    //0,2 and 3 reserved
    event_name = 1,
    event_props = 4,
    event_prop_name = 5,
    event_prop_value = 6,
    last_sent_time = 7,
    event_time = 8,
    event_id = 9,
};

long long statistics::stats_event::session_event_id_ = 0;
std::shared_ptr<statistics::stop_objects> statistics::stop_objects_;

statistics::statistics(const std::wstring& _file_name)
    : file_name_(_file_name)
    , changed_(false)
    , stats_thread_(new async_executer())
    , last_sent_time_(std::chrono::system_clock::now())
{
    stop_objects_.reset(new stop_objects());
}

void statistics::init()
{
    load();
    start_save();
    delayed_start_send();
}

statistics::~statistics()
{
    stop_objects_->is_stop_ = true;
    if (save_timer_ > 0 && g_core)
        g_core->stop_timer(save_timer_);

    if (send_timer_ > 0 && g_core)
        g_core->stop_timer(send_timer_);

    save_if_needed();

    stats_thread_.reset();
}

void statistics::start_save()
{
    std::weak_ptr<statistics> wr_this = shared_from_this();

    save_timer_ =  g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->save_if_needed();
    }, save_to_file_interval_ms);
}

void statistics::delayed_start_send()
{
    std::weak_ptr<statistics> wr_this = shared_from_this();
    start_send_timer_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->start_send();
        g_core->stop_timer(ptr_this->start_send_timer_);

    }, delay_send_on_start_ms);
}

void statistics::start_send()
{
    auto current_time = std::chrono::system_clock::now();
    if (current_time - last_sent_time_ >= std::chrono::milliseconds(send_interval_ms))
        send_async();

    std::weak_ptr<statistics> wr_this = shared_from_this();

    send_timer_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->send_async();
    }, send_interval_ms);
}

bool statistics::load()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(file_name_))
        return false;

    return unserialize(bstream);
}

void statistics::serialize(tools::binary_stream& _bs) const
{
    tools::tlvpack pack;
    int32_t counter = 0;

    // push stats info
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(statistics_info_types::last_sent_time, (int64_t)std::chrono::system_clock::to_time_t(last_sent_time_)));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    for (auto stat_event = events_.begin(); stat_event != events_.end(); ++stat_event)
    {
        tools::tlvpack value_tlv;
        // TODO : push id, time, ..
        value_tlv.push_child(tools::tlv(statistics_info_types::event_name, stat_event->get_name()));
        value_tlv.push_child(tools::tlv(statistics_info_types::event_time, (int64_t)std::chrono::system_clock::to_time_t(stat_event->get_time())));
        value_tlv.push_child(tools::tlv(statistics_info_types::event_id, (int64_t)stat_event->get_id()));

        tools::tlvpack props_pack;
        int32_t prop_counter = 0;
        auto props = stat_event->get_props();

        for (auto prop : props)
        {
            tools::tlvpack value_tlv_prop;
            value_tlv_prop.push_child(tools::tlv(statistics_info_types::event_prop_name, prop.first));
            value_tlv_prop.push_child(tools::tlv(statistics_info_types::event_prop_value, prop.second));

            tools::binary_stream bs_value;
            value_tlv_prop.serialize(bs_value);
            props_pack.push_child(tools::tlv(++prop_counter, bs_value));
        }

        value_tlv.push_child(tools::tlv(statistics_info_types::event_props, props_pack));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    pack.serialize(_bs);
}

bool unserialize_props(tools::tlvpack& prop_pack, event_props_type* props)
{
    assert(!!props);

    for (auto tlv_prop_val = prop_pack.get_first(); tlv_prop_val; tlv_prop_val = prop_pack.get_next())
    {
        tools::binary_stream val_data = tlv_prop_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
        {
            assert(false);
            return false;
        }

        auto tlv_event_name = pack_val.get_item(statistics_info_types::event_prop_name);
        auto tlv_event_value = pack_val.get_item(statistics_info_types::event_prop_value);

        if (!tlv_event_name || !tlv_event_value)
        {
            assert(false);
            return false;
        }

        props->emplace_back(std::make_pair(tlv_event_name->get_value<std::string>(), tlv_event_value->get_value<std::string>()));
    }
    return true;
}

bool statistics::unserialize(tools::binary_stream& _bs)
{
    if (!_bs.available())
    {
        assert(false);
        return false;
    }

    tools::tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    int32_t counter = 0;
    for (auto tlv_val = pack.get_first(); tlv_val; tlv_val = pack.get_next())
    {
        tools::binary_stream val_data = tlv_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
            return false;

        if (counter++ == 0)
        {
            auto tlv_last_sent_time = pack_val.get_item(statistics_info_types::last_sent_time);

            if (!tlv_last_sent_time)
            {
                assert(false);
                return false;
            }

            time_t last_time = tlv_last_sent_time->get_value<int64_t>();
            last_sent_time_ = std::chrono::system_clock::from_time_t(last_time);
        }
        else
        {
            auto curr_event_name = pack_val.get_item(statistics_info_types::event_name);

            if (!curr_event_name)
            {
                assert(false);
                return false;
            }
            stats_event_names name = curr_event_name->get_value<stats_event_names>();

            auto tlv_event_time = pack_val.get_item(statistics_info_types::event_time);
            auto tlv_event_id = pack_val.get_item(statistics_info_types::event_id);
            if (!tlv_event_time || !tlv_event_id)
            {
                assert(false);
                return false;
            }

            event_props_type props;
            const auto tlv_prop_pack = pack_val.get_item(statistics_info_types::event_props);
            assert(tlv_prop_pack);
            if (tlv_prop_pack)
            {
                auto prop_pack = tlv_prop_pack->get_value<tools::tlvpack>();
                if (!unserialize_props(prop_pack, &props))
                {
                    assert(false);
                    return false;
                }
            }

            auto read_event_time = std::chrono::system_clock::from_time_t(tlv_event_time->get_value<int64_t>());
            auto read_event_id = tlv_event_id->get_value<int64_t>();
            insert_event(name, props, read_event_time, read_event_id);
        }
    }

    return true;
}

void statistics::save_if_needed()
{
    if (changed_)
    {
        changed_ = false;

        auto bs_data = std::make_shared<tools::binary_stream>();
        serialize(*bs_data);
        std::wstring file_name = file_name_;

        g_core->save_async([bs_data, file_name]
        {
            bs_data->save_2_file(file_name);
            return 0;
        });
    }
}

void statistics::clear()
{
    last_sent_time_ = std::chrono::system_clock::now();

    auto last_service_event_ptr = events_.end();
    while (last_service_event_ptr != events_.begin())
    {
        --last_service_event_ptr;
        if (last_service_event_ptr->get_name() == stats_event_names::service_session_start
            || last_service_event_ptr->get_name() == stats_event_names::absolete_service_session_start)
            break;
    }

    auto last_service_event = *last_service_event_ptr;
    events_.clear();
    events_.push_back(last_service_event);

    changed_ = true;

    // reset_session_event_id();
    // TODO : mb need save map with counts here?
}

void statistics::send_async()
{
    if (events_.empty())
        return;

    std::vector<std::string> post_data_vector = get_post_data();
    std::weak_ptr<statistics> wr_this = shared_from_this();

    if (post_data_vector.empty())
        return;

    auto user_proxy = g_core->get_proxy_settings();

    auto file_name = file_name_;
    stats_thread_->run_async_function([post_data_vector, user_proxy, file_name]
    {
        for (auto& post_data : post_data_vector)
            statistics::send(user_proxy, post_data, file_name);
        return 0;

    })->on_result_ = [wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        ptr_this->clear();
        ptr_this->save_if_needed();
    };
}

std::string statistics::events_to_json(events_ci begin, events_ci end, time_t _start_time) const
{
    std::stringstream data_stream;
    std::map<stats_event_names, int32_t> events_and_count;

    for (auto stat_event  = begin; stat_event != end; ++stat_event)
    {
        if (stat_event != begin)
            data_stream << ",";
        data_stream << stat_event->to_string(_start_time);
        ++events_and_count[stat_event->get_name()];
    }

    data_stream << "],\"bm\":false,\"bn\":{";

    for (auto stat_event  = events_and_count.begin(); stat_event != events_and_count.end(); ++stat_event)
    {
        if (stat_event != events_and_count.begin())
            data_stream << ",";
        data_stream << "\"" << stat_event->first << "\":" << stat_event->second;
    }
    return data_stream.str();
}

std::vector<std::string> statistics::get_post_data() const
{
    std::vector<std::string> result;

    events_ci begin = events_.begin();

    while (begin != events_.end())
    {
        events_ci end = std::next(begin);
        while (end != events_.end() 
            && end->get_name() != stats_event_names::service_session_start
            && end->get_name() != stats_event_names::absolete_service_session_start)
            ++end;

        assert(begin->get_name() == stats_event_names::service_session_start 
            || begin->get_name() == stats_event_names::absolete_service_session_start);

        const long long time_now = std::chrono::system_clock::to_time_t(begin->get_time()) * 1000; // milliseconds

        std::string user_key = "";
        auto props = begin->get_props();

        if (props.size() > 0 && props.begin()->first == "hashed_user_key")
        {
            user_key = props.begin()->second;
        }
        else
        {
            assert(false);
        }

        ++begin;

        if (begin == end)
            continue;

        auto time = time_now;
        auto time1 = time + 4;
        auto time3 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000;
        auto delta = time3 - time;

        auto version = core::utils::get_user_agent();
        auto flurryKey = build::is_icq() ? flurry_key : agent_flurry_key;
        std::stringstream data_stream;

        data_stream << "{\"a\":{\"af\":" << time3
            <<",\"aa\":1,\"ab\":10,\"ac\":9,\"ae\":\""<< version
            << "\",\"ad\":\"" << flurryKey
            << "\",\"ag\":" << time
            << ",\"ah\":" << time1
            << ",\"ak\":1,"
            << "\"cg\":\"" << user_key
            << "\"},\"b\":[{\"bd\":\"\",\"be\":\"\",\"bk\":-1,\"bl\":0,\"bj\":\"ru\",\"bo\":[";

        data_stream << events_to_json(begin, end, time);

        data_stream << "}"
            <<",\"bv\":[],\"bt\":false,\"bu\":{},\"by\":[],\"cd\":0,"
            << "\"ba\":" << time1
            << ",\"bb\":" << delta
            << ",\"bc\":-1,\"ch\":\"Etc/GMT-3\"}]}";
        result.push_back(data_stream.str());

        if (end != events_.end())
        {
            begin = end;
        }
        else
            break;
    }

    return result;
}

bool statistics::send(const proxy_settings& _user_proxy, const std::string& post_data, const std::wstring& _file_name)
{
    const std::weak_ptr<stop_objects> wr_stop(stop_objects_);

    const auto stop_handler =
        [wr_stop]
        {
            auto ptr_stop = wr_stop.lock();
            if (!ptr_stop)
                return true;

            return ptr_stop->is_stop_.load();
        };

    core::http_request_simple post_request(_user_proxy, utils::get_user_agent(), stop_handler);
    post_request.set_connect_timeout(1000);
    post_request.set_timeout(1000);
    post_request.set_keep_alive();

#ifdef DEBUG
    {
        auto path = _file_name + L".txt";
        boost::system::error_code e;
        boost::filesystem::ofstream fOut;
        
        fOut.open(path.c_str(), std::ios::out);
        fOut << post_data;
        fOut.close();
    }
#endif // DEBUG

    auto result_url = flurry_url
        + "?d=" + core::tools::base64::encode64(post_data)
        + "&c=" + core::tools::adler32(post_data);
    post_request.set_url(result_url);
    return post_request.get();
}

void statistics::insert_event(stats_event_names _event_name, const event_props_type& _props,
                              std::chrono::system_clock::time_point _event_time, int32_t _event_id)
{
    events_.emplace_back(_event_name, _event_time, _event_id, _props);
    changed_ = true;
}

void statistics::insert_event(stats_event_names _event_name, const event_props_type& _props)
{
    if (_event_name == stats_event_names::start_session || _event_name == stats_event_names::absolete_start_session)
    {
        stats_event::reset_session_event_id();
        insert_event(core::stats::stats_event_names::service_session_start, _props);
    }
    insert_event(_event_name, _props, std::chrono::system_clock::now(), -1);
}

void statistics::insert_event(stats_event_names _event_name)
{
    event_props_type props;
    insert_event(_event_name, props);
}

statistics::stats_event::stats_event(stats_event_names _name,
                                     std::chrono::system_clock::time_point _event_time, int32_t _event_id, const event_props_type& _props)
    : name_(_name)
    , event_time_(_event_time)
    , props_(_props)
{
    if (_event_id == -1)
        event_id_ = session_event_id_++; // started from 1
    else
        event_id_ = _event_id;
}

const std::string statistics::stats_event::to_string(time_t _start_time) const
{
    std::stringstream result;

    // TODO : use actual params here
    auto br = 0;

    std::stringstream params_in_json;
    for (auto prop = props_.begin(); prop != props_.end(); ++prop)
    {
        if (prop != props_.begin())
            params_in_json << ",";
        params_in_json << "\"" << prop->first << "\":\"" << prop->second << "\"";
    }

    result << "{\"ce\":" << event_id_
        << ",\"bp\":\"" << name_
        << "\",\"bq\":" << std::chrono::system_clock::to_time_t(event_time_) * 1000 - _start_time// milliseconds
        << ",\"bs\":{" << params_in_json.str() <<"},"
        << "\"br\":" << br << "}";
    return result.str();
}

stats_event_names statistics::stats_event::get_name() const
{
    return name_;
}

event_props_type statistics::stats_event::get_props() const
{
    return props_;
}

void statistics::stats_event::reset_session_event_id()
{
    session_event_id_ = 0;
}

std::chrono::system_clock::time_point statistics::stats_event::get_time() const
{
    return event_time_;
}

int32_t statistics::stats_event::get_id() const
{
    return event_id_;
}