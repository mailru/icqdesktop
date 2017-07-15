#include "stdafx.h"
#include "utils.h"

#include "tools/system.h"
#include "../common.shared/version_info.h"
#include "../common.shared/common_defs.h"

namespace core
{
    namespace utils
    {
        std::wstring get_product_data_path()
        {
            const wchar_t* product_path = build::is_icq() ? product_path_icq_w : (platform::is_apple() ? product_path_agent_mac_w : product_path_agent_w);

#ifdef __linux__
            return (core::tools::system::get_user_profile() + L"/.config/" + product_path);
#elif _WIN32
            return ::common::get_user_profile() + L"/" + product_path;
#else
            return (core::tools::system::get_user_profile() + L"/" + product_path);
#endif //__linux__
        }

        std::string get_product_name()
        {
            return "icq.desktop";
        }

        std::string get_app_name()
        {
            if (platform::is_windows())
            {
                return build::is_icq() ? "Mail.ru Windows ICQ" : "Mail.ru Windows Agent";
            }
            else if (platform::is_apple())
            {
                return build::is_icq() ? "Mail.ru Mac OS X ICQ" : "Mail.ru Mac OS X Agent";
            }
            else if (platform::is_linux())
            {
                return build::is_icq() ? "Mail.ru Linux ICQ" : "Mail.ru Linux Agent";
            }
            else
            {
                assert(false);
                return build::is_icq() ? "Mail.ru ICQ" : "Mail.ru Agent";
            }
        }

        std::string get_user_agent(const string_opt &_uin)
        {
            std::string user_agent;
            user_agent.reserve(256);

            if (_uin)
            {
                const auto &uin = *_uin;
                assert(!uin.empty());

                user_agent += uin;
                user_agent += " ";
            }

            user_agent += get_app_name();
            user_agent += " (version ";
            user_agent += tools::version_info().get_version();
            user_agent += ")";

            user_agent.shrink_to_fit();

            return user_agent;
        }

        std::string get_platform_string()
        {
            if (platform::is_windows())
            {
                return "Win";
            }
            else if (platform::is_apple())
            {
                return "Apple";
            }
            else
            {
                assert(false);
                return "Unknown";
            }
        }

        std::wstring get_report_path()
        {
            return core::utils::get_product_data_path() + L"/reports";
        }

        std::wstring get_report_log_path()
        {
            return get_report_path() + L"/crash.log";
        }

        std::wstring get_report_mini_dump_path()
        {
            return get_report_path() + L"/crashdump.dmp";
        }

        const boost::filesystem::wpath get_logs_path()
        {
            boost::filesystem::wpath logs_dir(get_product_data_path());

            logs_dir.append(L"logs");

            return logs_dir;
        }

        bool is_writable(const boost::filesystem::path &p)
        {
            std::ofstream of(p.string());
            return of.good();
        }
    }
}
