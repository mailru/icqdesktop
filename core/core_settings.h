#ifndef __CORE_SETTINGS_H_
#define __CORE_SETTINGS_H_

#pragma once

#include "tools/settings.h"
#include "proxy_settings.h"

namespace core
{
    enum core_settings_values
    {
        min = 0,

        csv_device_id = 1,
        core_settings_proxy = 2,
        core_settings_locale = 3,
		voip_mute_fix_flag   = 4,

        themes_settings_etag = 10,

        core_settings_locale_was_changed = 20,
        core_need_show_promo = 21,
        
        max
    };

	class core_settings : public core::tools::settings
	{
		std::wstring file_name_;
        std::wstring file_name_exported_;

        bool load_exported();

	public:
		core_settings(const boost::filesystem::wpath& _path, const boost::filesystem::wpath& _file_name_exported);
		virtual ~core_settings();
        
		bool save();
		bool load();

        bool end_init_default();

        void set_user_proxy_settings(const proxy_settings& _user_proxy_settings);

        void set_locale_was_changed(bool _was_changed);
        bool get_locale_was_changed() const;
        
        void set_locale(const std::string& _locale);
        std::string get_locale() const;
        
        proxy_settings get_user_proxy_settings();
		
        bool get_voip_mute_fix_flag() const;
		void set_voip_mute_fix_flag(bool bValue);

        void set_need_show_promo(bool _need_show_promo);
        bool get_need_show_promo() const;

        bool unserialize(const rapidjson::Value& _node);
	};

}

#endif //__CORE_SETTINGS_H_