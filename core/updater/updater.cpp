#include "stdafx.h"
#include "updater.h"

#include "../core.h"
#include "../async_task.h"
#include "../http_request.h"
#include "../utils.h"
#include "../../common.shared/version_info.h"
#include "../../external/openssl/openssl/md5.h"

namespace core
{
    namespace update
    {
        const int32_t check_period = 24; // one day

        struct update_params
        {
            std::string login_;
            std::function<bool()> must_stop_;
        };

        std::string get_update_server()
        {
#ifdef _WIN32
            return build::is_icq()
                ? "https://mra.mail.ru/icq10_win_update/"
                : "https://mra.mail.ru/mra10_win_update/";
#else
#ifdef __x86_64__
            return "https://mra.mail.ru/icq10_linux_update_x64/";
#else
            return "https://mra.mail.ru/icq10_linux_update/";
#endif //__x86_64__
#endif //_WIN32


            /*	if (build::is_debug())
            {
            return "http://icq-builder1.mail.msk:8888/icq_update/";
            }
            else
            {
            return "http://mra.mail.ru/icq_win_update/";
            }*/
        }

        std::string get_update_version_url(const update_params& _params)
        {
            std::string url = get_update_server() + "version.js" + "?" + "login=" + _params.login_;

            return url;

        }

        updater::updater()
            :	thread_(new core::async_executer(1)),
            stop_(false)
        {
            timer_id_ = g_core->add_timer([this]()
            {
                check_if_need();

            }, (build::is_debug() ? (1000 * 10) : (1000 * 60 * 10)));

            last_check_time_ = std::chrono::system_clock::now() - std::chrono::hours(check_period);
        }


        updater::~updater()
        {
            stop_ = true;

            g_core->stop_timer(timer_id_);
        }

        bool updater::must_stop()
        {
            return stop_;
        }

        int32_t run(const update_params& _params, const proxy_settings& _proxy);


        void updater::check_if_need()
        {
            if (build::is_debug())
                return;

            if (platform::is_apple())
                return;

            if ((std::chrono::system_clock::now() - last_check_time_) > std::chrono::hours(check_period))
            {
                update_params params;
                params.login_ = g_core->get_root_login();
                params.must_stop_ = std::bind(&updater::must_stop, this);

                auto proxy = g_core->get_proxy_settings();

                thread_->run_async_function(
                    [params, proxy]
                    {
                        return run(params, proxy);
                    });

                last_check_time_ = std::chrono::system_clock::now();
            }
        }

        int32_t do_update(const std::string& _file, const std::string& _md5, const update_params& _params, const proxy_settings& _proxy);

        int32_t run(const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(), _params.must_stop_);
            request.set_url(get_update_version_url(_params));

            if (!request.get())
                return -1;

            int32_t http_code = (int32_t)request.get_response_code();
            if (http_code != 200)
                return -1;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            assert(response);

            if (!response->available())
                return -1;

            response->write((char) 0);

            //{"info":{"version":{"major":10, "minor":0, "buildnumber":10008},"file":"icqsetup.exe","md5":"afda633be58e18a0aaeb3e88481058e9"}}

            rapidjson::Document doc;
            if (doc.ParseInsitu(response->read(response->available())).HasParseError())
                return -1;

            auto iter_info = doc.FindMember("info");
            if (iter_info == doc.MemberEnd())
                return -1;

            auto iter_version = iter_info->value.FindMember("version");
            if (iter_version == iter_info->value.MemberEnd())
                return -1;

            auto iter_major = iter_version->value.FindMember("major");
            if (iter_major == iter_version->value.MemberEnd() || !iter_major->value.IsInt())
                return -1;

            auto iter_minor = iter_version->value.FindMember("minor");
            if (iter_minor == iter_version->value.MemberEnd() || !iter_minor->value.IsInt())
                return -1;

            auto iter_build = iter_version->value.FindMember("buildnumber");
            if (iter_build == iter_version->value.MemberEnd() || !iter_build->value.IsInt())
                return -1;

            auto iter_file = iter_info->value.FindMember("file");
            if (iter_file == iter_info->value.MemberEnd() || !iter_file->value.IsString())
                return -1;

            auto iter_md5 = iter_info->value.FindMember("md5");
            if (iter_md5 == iter_info->value.MemberEnd() || !iter_md5->value.IsString())
                return -1;

            tools::version_info local_version_info = tools::version_info();
            tools::version_info server_version_info = tools::version_info(iter_major->value.GetInt(), iter_minor->value.GetInt(), iter_build->value.GetInt());

            if (local_version_info < server_version_info)
                return do_update(iter_file->value.GetString(), iter_md5->value.GetString(), _params, _proxy);

            return 0;
        }

        int32_t run_installer(const tools::binary_stream& _data);

        int32_t do_update(const std::string& _file, const std::string& _md5, const update_params& _params, const proxy_settings& _proxy)
        {
            http_request_simple request(_proxy, utils::get_user_agent(), _params.must_stop_);
            request.set_url(get_update_server() + _file);
            request.set_timeout(60 * 1000 * 15);
            request.set_need_log(false);
            if (!request.get())
                return -1;

            int32_t http_code = (uint32_t)request.get_response_code();
            if (http_code != 200)
                return -1;

            auto response = dynamic_cast<tools::binary_stream*>(request.get_response().get());
            assert(response);

            if (!response->available())
                return -1;

            int32_t size = response->available();

            MD5_CTX md5handler;
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_Init(&md5handler);
            MD5_Update(&md5handler, response->read(size), size);
            MD5_Final(md5digest,&md5handler);

            char buffer[100];
            std::string md5;

            for (int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
            {
                sprintf(buffer, "%02x", md5digest[i]);
                md5 += buffer;
            }

            if (md5 != _md5)
                return -1;

            response->reset_out();

            return run_installer(*response);
        }

        int32_t run_installer(const tools::binary_stream& _data)
        {
#ifdef _WIN32
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return -1;

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, build::is_icq() ? L"icqsetup" : L"magentsetup", 0, temp_file_name))
                return -1;

            _data.save_2_file(temp_file_name);

            PROCESS_INFORMATION pi = {0};
            STARTUPINFO si = {0};
            si.cb = sizeof(si);

            wchar_t command[1024];
            swprintf_s(command, 1023, L"\"%s\" -update", temp_file_name);

            if (!::CreateProcess(0, command, 0, 0, 0, 0, 0, 0, &si, &pi))
                return -1;

            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );
#elif defined __linux__
            std::string path;
            char buff[PATH_MAX];
            ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
            if (len != -1)
            {
                buff[len] = '\0';
                path = buff;
            }
            else
            {
                return -1;
            }

            auto pos = path.rfind("/");
            if (pos == std::string::npos)
                return - 1;

            path = path.substr(0, pos + 1);
            path += "updater";
            std::string sys_path = path;
            boost::replace_all(sys_path, " ", "\\ ");
            std::string rm = "rm -f ";
            rm += sys_path;
            system(rm.c_str());
            _data.save_2_file(core::tools::from_utf8(path));
            std::string chmod = "chmod 755 ";
            chmod += sys_path;
            system(chmod.c_str());
            system(sys_path.c_str());
#endif //_WIN32
            return  0;
        }

        void clean_prev_instalations()
        {
#ifdef _WIN32
            wchar_t exe_name[1025];
            if (::GetModuleFileName(0, exe_name, 1024))
            {
                boost::filesystem::wpath file_path(exe_name);
                boost::filesystem::wpath current_path = file_path.parent_path();
                boost::filesystem::wpath parent_path = file_path.parent_path().parent_path();

                std::wstring current_leaf = current_path.leaf().wstring();

                if (!tools::is_number(tools::from_utf16(current_leaf)))
                    return;

                boost::filesystem::directory_iterator end_iter;
                for (boost::filesystem::directory_iterator dir_iter(parent_path); dir_iter != end_iter ; ++dir_iter)
                {
                    if (!boost::filesystem::is_directory(dir_iter->status()))
                        continue;

                    if (current_path == dir_iter->path())
                        continue;

                    std::wstring leaf = dir_iter->path().leaf().wstring();

                    if (!tools::is_number(tools::from_utf16(leaf)))
                        continue;

                    int32_t current_num = std::stoi(current_leaf);
                    int32_t num = std::stoi(leaf);

                    if (current_num < num)
                        continue;

                    boost::system::error_code error;
                    boost::filesystem::remove_all(dir_iter->path(), error);
                }
            }

#endif //_WIN32
        }
    }
}
