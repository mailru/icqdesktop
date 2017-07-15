#pragma once

#include "../external/curl/include/curl.h"

#include "http_request.h"

namespace core
{
    struct curl_context
    {
        CURL* curl_;
        std::shared_ptr<tools::stream> output_;
        std::shared_ptr<tools::binary_stream> header_;

        http_request_simple::stop_function stop_func_;

        http_request_simple::progress_function progress_func_;

        int32_t bytes_transferred_pct_;

        bool need_log_;
        std::shared_ptr<tools::binary_stream> log_data_;

        core::replace_log_function replace_log_function_;

        curl_slist* header_chunk_;
        struct curl_httppost* post;
        struct curl_httppost* last;

        bool keep_alive_;

        priority_t priority_;

        milliseconds_t timeout_;

        curl_context(std::shared_ptr<tools::stream> _output, http_request_simple::stop_function _stop_func, http_request_simple::progress_function _progress_func, bool _keep_alive);
        ~curl_context();

        bool init(milliseconds_t _connect_timeout, milliseconds_t _timeout, core::proxy_settings _proxy_settings, const std::string &_user_agent);

        bool is_need_log();
        void set_need_log(bool _need);

        void write_log_data(const char* _data, uint32_t _size);
        void write_log_string(const std::string& _log_string);

        void set_replace_log_function(replace_log_function _func);
        void set_range(int64_t _from, int64_t _to);
        void set_url(const char* sz_url);
        void set_post();
        void set_http_post();
        void set_post_fields(const char* sz_fields, int32_t _size);
        void set_modified_time(time_t _last_modified_time);
        void set_form_field(const char* _field_name, const char* _value);
        void set_form_file(const char* _field_name, const char* _file_name);
        void set_form_filedata(const char* _field_name, const char* _file_name, tools::binary_stream& _data);
        void set_custom_header_params(const std::list<std::string>& _params);
        void set_priority(priority_t _priority);

        long get_response_code();
        std::shared_ptr<tools::binary_stream> get_header();

        bool execute_request();
        void execute_request_async(http_request_simple::completion_function _completion_function);
    };
}
