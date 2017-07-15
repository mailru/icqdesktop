#include "stdafx.h"

#include "hosts_config.h"
#include "../tools/strings.h"
#include "../core.h"
#include "../utils.h"

namespace core
{
    enum host_config_types
    {
        hct_request_time = 0,
        hct_active_scheme = 1,
        hct_hosts = 2,
        hct_host = 3,
        hct_host_key = 4,
        hct_host_alt = 5
    };

    hosts_map::hosts_map()
    {
        active_scheme_ = false;

        hosts_.emplace("api.icq.net", "api.ic2ster.com");
        hosts_.emplace("bos.icq.net", "bos.ic2ster.com");
        hosts_.emplace("api.login.icq.net", "apilogin.ic2ster.com");
        hosts_.emplace("icq.com", "www.ic2ster.com");
        hosts_.emplace("www.icq.com", "www.ic2ster.com");
        hosts_.emplace("files.icq.com", "files-com.ic2ster.com");
        hosts_.emplace("files.icq.net", "files-net.ic2ster.com");
        hosts_.emplace("rapi.icq.net", "rapi.ic2ster.com");
        hosts_.emplace("pymk.icq.net", "pymk.ic2ster.com");
        hosts_.emplace("files-upload.icq.com", "files-upload.ic2ster.com");
        hosts_.emplace("clientapi.icq.net", "clientapi.ic2ster.com");
        hosts_.emplace("store.icq.com", "store.ic2ster.com");
    }

    bool hosts_map::parse(tools::binary_stream& _bs)
    {
        if (!_bs.available())
        {
            return false;
        }

        rapidjson::Document doc;
        if (doc.ParseInsitu(_bs.read_available()).HasParseError())
        {
            return false;
        }

        for (rapidjson::Value::ConstMemberIterator iter = doc.MemberBegin(); iter != doc.MemberEnd(); ++iter)
        {
            if (!iter->value.IsString() || !iter->name.IsString())
                continue;

            hosts_[iter->name.GetString()] = tools::trim_right<std::string>(iter->value.GetString(), std::string(" "));
        }

        return true;
    }

    std::string pack_string(const std::string& _source)
    {
        std::stringstream ss;

        for (const auto _sym: _source)
        {
            ss << char(_sym ^ '~');
        }

        return ss.str();
    }

    void hosts_map::serialize(tools::tlvpack& _pack) const
    {
        _pack.push_child(tools::tlv(host_config_types::hct_active_scheme, active_scheme_));

        tools::tlvpack pack_hosts;

        for (const auto& _host : hosts_)
        {
            tools::tlvpack pack_host;

            pack_host.push_child(core::tools::tlv(host_config_types::hct_host_key, pack_string(_host.first)));
            pack_host.push_child(core::tools::tlv(host_config_types::hct_host_alt, pack_string(_host.second)));

            pack_hosts.push_child(core::tools::tlv(host_config_types::hct_host, pack_host));
        }

        _pack.push_child(tools::tlv(host_config_types::hct_hosts, pack_hosts));
    }

    bool hosts_map::unserialize(const tools::tlvpack& _pack)
    {
        auto tlv_scheme = _pack.get_item(host_config_types::hct_active_scheme);
        if (tlv_scheme)
        {
            active_scheme_ = tlv_scheme->get_value<bool>();
        }

        auto tlv_hosts = _pack.get_item(host_config_types::hct_hosts);
        if (tlv_hosts)
        {
            tools::tlvpack pack_hosts = tlv_hosts->get_value<tools::tlvpack>();

            for (auto tlv_host = pack_hosts.get_first(); tlv_host; tlv_host = pack_hosts.get_next())
            {
                tools::tlvpack pack_host = tlv_host->get_value<tools::tlvpack>();

                auto tlv_key = pack_host.get_item(host_config_types::hct_host_key);
                auto tlv_alt = pack_host.get_item(host_config_types::hct_host_alt);

                if (tlv_key && tlv_alt)
                {
                    hosts_[pack_string(tlv_key->get_value<std::string>())] = pack_string(tlv_alt->get_value<std::string>());
                }
            }
        }


        return true;
    }

    std::string hosts_map::get_host_alt(const std::string& _host) const
    {
        if (!active_scheme_)
        {
            return _host;
        }

        const auto iter = hosts_.find(_host);

        if (iter == hosts_.cend())
        {
            return _host;
        }

        return iter->second;
    }

    void hosts_map::change_scheme()
    {
        active_scheme_ = !active_scheme_;
    }

    bool hosts_map::get_active_scheme() const
    {
        return active_scheme_;
    }

    void hosts_map::set_active_scheme(bool _scheme)
    {
        active_scheme_ = _scheme;
    }






    hosts_config::hosts_config()
        :   last_request_time_(std::chrono::system_clock::now() - get_request_period()),
            timer_(0),
            changed_(false)
    {
    }

    hosts_config::~hosts_config()
    {
        if (timer_ > 0)
            g_core->stop_timer(timer_);

        save_if_needed();
    }

    std::wstring hosts_config::get_file_name() const
    {
        return (utils::get_product_data_path() + L"/settings/" + tools::from_utf8(hosts_config_file_name));
    }


    void hosts_config::serialize(tools::binary_stream& _bs) const
    {
        tools::tlvpack pack;

        pack.push_child(tools::tlv(host_config_types::hct_request_time, (int64_t) std::chrono::system_clock::to_time_t(last_request_time_)));

        tools::tlvpack pack_hosts;

        hosts_.serialize(pack_hosts);

        pack.push_child(tools::tlv(host_config_types::hct_hosts, pack_hosts));

        pack.serialize(_bs);
    }

    void hosts_config::load()
    {
        tools::binary_stream bs;

        tools::tlvpack pack;

        if (bs.load_from_file(get_file_name()) && pack.unserialize(bs))
        {
            auto tlv_time = pack.get_item(host_config_types::hct_request_time);

            if (tlv_time)
            {
                last_request_time_ = std::chrono::system_clock::from_time_t((time_t) tlv_time->get_value<int64_t>());
            }

            auto tlv_hosts = pack.get_item(host_config_types::hct_hosts);

            if (tlv_hosts)
            {
                hosts_.unserialize(tlv_hosts->get_value<tools::tlvpack>());
            }
        }
    }

    void hosts_config::save_if_needed()
    {
        if (!changed_)
            return;

        auto bs_data = std::make_shared<tools::binary_stream>();

        serialize(*bs_data);

        std::wstring file_name = get_file_name();

        g_core->save_async([bs_data, file_name]
        { 
            bs_data->save_2_file(file_name);

            return 0;
        });

        changed_ = false;
    }

    const hosts_map& hosts_config::get_hosts() const
    {
        return hosts_;
    }

    void hosts_config::update_hosts(const hosts_map& _hosts, bool _replace_scheme)
    {
        changed_ = true;

        bool active_scheme = hosts_.get_active_scheme();

        hosts_ = _hosts;

        if (!_replace_scheme)
        {
            hosts_.set_active_scheme(active_scheme);
        }
    }

    void hosts_config::update_last_request_time()
    {
        changed_ = true;

        last_request_time_ = std::chrono::system_clock::now();
    }

    bool hosts_config::need_update() const
    {
        return ((std::chrono::system_clock::now() - last_request_time_) > get_request_period());
    }

    void hosts_config::start_save()
    {
        std::weak_ptr<hosts_config> wr_this = shared_from_this();

        timer_ =  g_core->add_timer([wr_this]
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->save_if_needed();

        }, 1000*10);
    }

    std::string hosts_config::get_host_alt(const std::string& _host) const
    {
        return hosts_.get_host_alt(_host);
    }


    std::chrono::system_clock::duration hosts_config::get_request_period()
    {
        return (build::is_debug() ? std::chrono::minutes(1) : std::chrono::hours(24));
    }

    int32_t hosts_config::get_first_request_timeout_ms()
    {
        return (build::is_debug() ? (1000 * 10) : (1000 * 60 * 10));
    }

    void hosts_config::change_scheme()
    {
        changed_ = true;

        hosts_.change_scheme();
    }

    bool hosts_config::is_alt_scheme() const
    {
        return hosts_.get_active_scheme();
    }
}

