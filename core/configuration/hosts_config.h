#ifndef __HOSTS_CONFIG_H_
#define __HOSTS_CONFIG_H_

#pragma once

namespace core
{
    class hosts_map
    {
        bool active_scheme_;

        std::map<std::string, std::string>  hosts_;

    public:

        hosts_map();

        bool parse(tools::binary_stream& _bs);
        void serialize(tools::tlvpack& _pack) const;
        bool unserialize(const tools::tlvpack& _pack);

        bool get_active_scheme() const;
        void set_active_scheme(bool _scheme);

        std::string get_host_alt(const std::string& _host) const;
        void change_scheme();
    };

    class hosts_config : public std::enable_shared_from_this<hosts_config>
    {
        hosts_map hosts_;

        std::chrono::system_clock::time_point last_request_time_;

        uint32_t timer_;

        bool changed_;

        void serialize(tools::binary_stream& _bs) const;

        std::wstring get_file_name() const;

    public:

        hosts_config();
        virtual ~hosts_config();

        void load();
        void start_save();
        void save_if_needed();

        const hosts_map& get_hosts() const;

        void update_hosts(const hosts_map& _hosts, bool _replace_scheme = true);

        std::string get_host_alt(const std::string& _host) const;

        bool need_update() const;

        void update_last_request_time();

        void change_scheme();

        bool is_alt_scheme() const;

        static std::chrono::system_clock::duration get_request_period();

        static int32_t get_first_request_timeout_ms();
    };
}

#endif //__HOSTS_CONFIG_H_