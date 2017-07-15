#pragma once

#include "../corelib/enumerations.h"

namespace core
{
    class coll_helper;
    class core_settings;

    struct proxy_settings
    {
        static const int32_t default_proxy_port;

        bool			use_proxy_;
        std::wstring	proxy_server_;
        int32_t     	proxy_port_;
        bool			need_auth_;
        std::wstring	login_;
        std::wstring	password_;
        int32_t			proxy_type_;

        proxy_settings()
            : use_proxy_(false)
            , need_auth_(false)
            , proxy_type_((int32_t)core::proxy_types::auto_proxy)
            , proxy_port_(default_proxy_port)
        {
        }

        void serialize(tools::binary_stream& _bs) const;
        bool unserialize(tools::binary_stream& _bs);

        void serialize(core::coll_helper _collection) const;

        static proxy_settings auto_proxy()
        {
            return proxy_settings();
        }
    };

    class proxy_settings_manager
    {
        core_settings& settings_storage_;

#ifdef _WIN32
        std::array<proxy_settings, 3> settings_;
#else
        std::array<proxy_settings, 2> settings_;
#endif

        size_t current_settings_;

        mutable std::mutex mutex_;

    public:
        explicit proxy_settings_manager(core_settings& _settings_storage);
        virtual ~proxy_settings_manager();

        proxy_settings get_current_settings() const;

        void apply(const proxy_settings& _settings);

        bool try_to_apply_alternative_settings();

    private:
#ifdef _WIN32
        proxy_settings load_from_registry();
#endif
    };
}
