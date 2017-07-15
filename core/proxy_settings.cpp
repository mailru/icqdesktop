#include "stdafx.h"

#include "../external/curl/include/curl.h"
#include "../corelib/collection_helper.h"

#include "core_settings.h"

#include "proxy_settings.h"

namespace
{
    enum pos_by_proxe_type
    {
        user_settings = 0,
        auto_settings,
#ifdef _WIN32
        system_settings
#endif
    };
}

namespace core
{
    enum proxy_settings_values
    {
        proxy_settings_proxy_type = 1,
        proxy_settings_proxy_server = 2,
        proxy_settings_proxy_port = 3,
        proxy_settings_proxy_login = 4,
        proxy_settings_proxy_password = 5,
        proxy_settings_proxy_need_auth = 6,
    };

    const int32_t proxy_settings::default_proxy_port = -1;

    proxy_settings_manager::proxy_settings_manager(core_settings& _settings_storage)
        : settings_storage_(_settings_storage)
        , current_settings_(user_settings)
    {
        settings_[user_settings] = settings_storage_.get_user_proxy_settings(); // at first we apply the user settings
        settings_[auto_settings] = proxy_settings::auto_proxy();                // then we apply default (auto) settings
#ifdef _WIN32
        settings_[system_settings] = load_from_registry();                      // and only on Windows we apply system settings at the end
#endif

        if (settings_[user_settings].proxy_type_ == static_cast<int32_t>(core::proxy_types::auto_proxy))
        {
            current_settings_ = auto_settings;
        }
    }

    proxy_settings_manager::~proxy_settings_manager()
    {
    }

    proxy_settings proxy_settings_manager::get_current_settings() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return settings_[current_settings_];
    }

    void proxy_settings_manager::apply(const proxy_settings& _settings)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (_settings.proxy_type_ == static_cast<int32_t>(core::proxy_types::auto_proxy))
            {
                current_settings_ = auto_settings;
            }
            else
            {
                settings_[0] = _settings;
                current_settings_ = user_settings;
            }
        }

        settings_storage_.set_user_proxy_settings(_settings);
    }

    bool proxy_settings_manager::try_to_apply_alternative_settings()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (current_settings_ == user_settings) // we won't change the settings if they are user
            return false;

        if (current_settings_ < settings_.size() - 1) // try the next settings if they available
        {
            ++current_settings_;
            return true;
        }

        return false;
    }

#ifdef _WIN32
    proxy_settings proxy_settings_manager::load_from_registry()
    {
        proxy_settings settings;

        settings.use_proxy_ = false;
        CRegKey key;
        if (key.Open(HKEY_CURRENT_USER, CAtlString(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")) == ERROR_SUCCESS)
        {
            DWORD proxy_used = 0;
            if (key.QueryDWORDValue(L"ProxyEnable", proxy_used) == ERROR_SUCCESS)
            {
                if (proxy_used == 0)
                    return settings;

                wchar_t buffer[MAX_PATH];
                unsigned long len = MAX_PATH;
                if (key.QueryStringValue(L"ProxyServer", buffer, &len) == ERROR_SUCCESS)
                {
                    settings.proxy_server_ = std::wstring(buffer);
                    if (settings.proxy_server_.empty())
                        return settings;

                    settings.use_proxy_ = true;
                    settings.proxy_type_ = CURLPROXY_HTTP;

                    if (key.QueryStringValue(L"ProxyUser", buffer, &len) == ERROR_SUCCESS)
                    {
                        settings.login_ = std::wstring(buffer);
                        if (settings.login_.empty())
                            return settings;

                        settings.need_auth_ = true;

                        if (key.QueryStringValue(L"ProxyPass", buffer, &len) == ERROR_SUCCESS)
                        {
                            settings.password_ = std::wstring(buffer);
                        }
                    }
                }
            }
        }

        return settings;
    }
#endif //_WIN32

    bool proxy_settings::unserialize(tools::binary_stream& _stream)
    {
        core::tools::tlvpack tlv_pack;
        if (!tlv_pack.unserialize(_stream))
            return false;

        auto root_tlv = tlv_pack.get_item(0);
        if (!root_tlv)
            return false;

        core::tools::tlvpack tlv_pack_childs;
        if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
            return false;

        auto tlv_proxy_server = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_server);
        auto tlv_proxy_port = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_port);
        auto tlv_login = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_login);
        auto tlv_password = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_password);
        auto tlv_proxy_type = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_type);
        auto tlv_need_auth = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_need_auth);

        if (!tlv_proxy_server 
            || !tlv_proxy_port
            || !tlv_login 
            || !tlv_password
            || !tlv_proxy_type
            || !tlv_need_auth)
            return false;

        proxy_server_ = tools::from_utf8(tlv_proxy_server->get_value<std::string>(""));
        proxy_port_ = tlv_proxy_port->get_value<int32_t>(proxy_settings::default_proxy_port);
        login_ = tools::from_utf8(tlv_login->get_value<std::string>(""));
        password_ = tools::from_utf8(tlv_password->get_value<std::string>(""));
        proxy_type_ = tlv_proxy_type->get_value<int32_t>((int32_t)core::proxy_types::auto_proxy);

        need_auth_ = tlv_need_auth->get_value<bool>(false);
        use_proxy_ = proxy_type_ != (int32_t)core::proxy_types::auto_proxy;

        return true;
    }

    void proxy_settings::serialize(tools::binary_stream& _stream) const
    {
        core::tools::tlvpack pack;
        core::tools::binary_stream temp_stream;
        
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_type, proxy_type_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_server, tools::from_utf16(proxy_server_)));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_port, proxy_port_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_login, tools::from_utf16(login_)));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_password, tools::from_utf16(password_)));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_need_auth, need_auth_));

        pack.serialize(temp_stream);

        core::tools::tlvpack rootpack;
        rootpack.push_child(core::tools::tlv(0, temp_stream));

        rootpack.serialize(_stream);
    }

    void proxy_settings::serialize(core::coll_helper _collection) const
    {
        _collection.set_value_as_int("settings_proxy_type", proxy_type_);
        _collection.set_value_as_string("settings_proxy_server", core::tools::from_utf16(proxy_server_));
        _collection.set_value_as_int("settings_proxy_port", proxy_port_);
        _collection.set_value_as_string("settings_proxy_username", core::tools::from_utf16(login_));
        _collection.set_value_as_string("settings_proxy_password", core::tools::from_utf16(password_));
        _collection.set_value_as_bool("settings_proxy_need_auth", need_auth_);
    }
}