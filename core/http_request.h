#ifndef __HTTP_REQUEST_H_
#define __HTTP_REQUEST_H_

#pragma once

#include "proxy_settings.h"

#include "../external/curl/include/curl.h"

namespace core
{
    struct curl_context;

    class hosts_map;

    class ithread_callback;

    typedef std::function<void(tools::binary_stream&)> replace_log_function;

    namespace tools
    {
        class binary_stream;
    }

    struct file_binary_stream
    {
        std::wstring file_name_;
        tools::binary_stream file_stream_;
    };

    class http_request_simple
    {
    public:
        typedef std::function<void(bool _success)> completion_function;

        typedef std::function<bool()> stop_function;

        typedef std::function<void(int64_t _bytes_total, int64_t _bytes_transferred, int32_t _pct_transferred)> progress_function;

    private:
        stop_function stop_func_;
        progress_function progress_func_;
        replace_log_function replace_log_function_;
        std::map<std::string, std::string> post_parameters_;
        std::map<std::string, std::string> post_form_parameters_;
        std::multimap<std::string, std::string> post_form_files_;
        std::multimap<std::string, file_binary_stream> post_form_filedatas_;

        std::string url_;
        std::list<std::string> custom_headers_;

        long response_code_;
        std::shared_ptr<tools::stream> output_;
        std::shared_ptr<tools::binary_stream> header_;

        bool is_time_condition_;
        time_t last_modified_time_;

        char* post_data_;
        int32_t post_data_size_;
        bool copy_post_data_;

        int32_t connect_timeout_;
        int32_t timeout_;

        int64_t range_from_;
        int64_t range_to_;
        bool is_post_form_;
        bool need_log_;
        bool keep_alive_;
        priority_t priority_;

        void clear_post_data();
        std::string get_post_param() const;
        void set_post_params(curl_context* ctx);
        bool send_request(bool _post);
        void* send_request_async(bool _post, completion_function _completion_function);
        proxy_settings proxy_settings_;

        std::string user_agent_;

    public:
        http_request_simple(proxy_settings _proxy_settings, const std::string &_user_agent, stop_function _stop_func = nullptr, progress_function _progress_func = nullptr);
        virtual ~http_request_simple();

        static void init_global();
        static void shutdown_global();

        void set_output_stream(std::shared_ptr<tools::stream> _output);

        std::shared_ptr<tools::stream> get_response();
        std::shared_ptr<tools::binary_stream> get_header();
        long get_response_code();

        void set_modified_time_condition(time_t _modified_time);

        void set_url(const std::wstring& url);
        void set_url(const std::string& url);
        std::string get_post_url();

        void push_post_parameter(const std::wstring& name, const std::wstring& value);
        void push_post_parameter(const std::string& name, const std::string& value);
        void set_post_data(const char* _data, int32_t _size, bool _copy_post_data);
        void push_post_form_parameter(const std::wstring& name, const std::wstring& value);
        void push_post_form_parameter(const std::string& name, const std::string& value);
        void push_post_form_file(const std::wstring& name, const std::wstring& file_name);
        void push_post_form_file(const std::string& name, const std::string& file_name);
        void push_post_form_filedata(const std::wstring& name, const std::wstring& file_name);

        void set_need_log(bool _need);
        void set_keep_alive();
        void set_priority(priority_t _priority);
        void set_etag(const char *etag);
        void set_replace_log_function(replace_log_function _func);

        void get_post_parameters(std::map<std::string, std::string>& params);

        void set_range(int64_t _from, int64_t _to);

        void post_async(completion_function _completion_function);
        void* get_async(completion_function _completion_function);

        bool post();
        bool get();
        void set_post_form(bool _is_post_form);

        void set_custom_header_param(const std::string& _value);

        void set_connect_timeout(int32_t _timeout_ms);
        void set_timeout(int32_t _timeout_ms);

        static std::vector<std::mutex*> ssl_sync_objects;
        proxy_settings get_user_proxy() const;

        void replace_host(const hosts_map& _hosts);
    };
}


#endif// __HTTP_REQUEST_H_