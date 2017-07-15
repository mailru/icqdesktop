#include "stdafx.h"

#ifdef _WIN32

#include "../../common.shared/win32/common_crash_sender.h"

#include "../../external/curl/include/curl.h"
#include "../../common.shared/keys.h"
#include "../logic/tools.h"

using namespace installer;

namespace common_crash_sender
{
    const std::string& get_hockeyapp_url()
    {
        auto app_id = build::is_agent() ? agent_installer_hockey_app_id : hockey_app_id_installer;
        static std::string hockeyapp_url = "https://rink.hockeyapp.net/api/2/apps/" + app_id + "/crashes/upload";
        return hockeyapp_url;
    }

    void post_dump_to_hockey_app_and_delete()
    {
        auto log_file_path = logic::get_product_folder() + "/reports/crash.log";
        auto dump_file_path = logic::get_product_folder() +"/reports/crashdump.dmp";


        CURL *curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_ALL);

        curl = curl_easy_init();
        if(curl)
        {
            struct curl_httppost *formpost=NULL;
            struct curl_httppost *lastptr=NULL;

            auto temp = logic::get_product_folder();

            curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "log",
                CURLFORM_FILE, log_file_path.toStdString().c_str(),
                CURLFORM_END);

            curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "attachment0",
                CURLFORM_FILE, dump_file_path.toStdString().c_str(),
                CURLFORM_END);

            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

            curl_easy_setopt(curl, CURLOPT_URL, get_hockeyapp_url().c_str());

            res = curl_easy_perform(curl);
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

            curl_easy_cleanup(curl);

            curl_formfree(formpost);
        }
        curl_global_cleanup();

        wchar_t log_path[1024];
        log_file_path.toWCharArray(log_path);
        log_path[log_file_path.length()] = '\0';

        wchar_t dump_path[1024];
        dump_file_path.toWCharArray(dump_path);
        dump_path[dump_file_path.length()] = '\0';

        wchar_t path[1024];
        (logic::get_product_folder() + "/reports/").toWCharArray(path);
        path[(logic::get_product_folder() + "/reports/").length()] = '\0';

        DeleteFile(log_path);
        DeleteFile(dump_path);
        RemoveDirectory(path);
    }

    void start_sending_process()
    {
        PROCESS_INFORMATION pi = {0};
        STARTUPINFO si = {0};
        si.cb = sizeof(si);

        wchar_t arg[100];
        send_dump_arg.toWCharArray(arg);
        arg[send_dump_arg.length()] = '\0';

        wchar_t buffer[1025];
        ::GetModuleFileName(0, buffer, 1024);

        wchar_t command[1024];
        swprintf_s(command, 1023, L"\"%s\" %s", buffer, arg);

        if (::CreateProcess(0, command, 0, 0, 0, 0, 0, 0, &si, &pi))
        {
            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );
        }
    }
}

#endif // _WIN32
