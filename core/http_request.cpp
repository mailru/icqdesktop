#include "stdafx.h"
#include "http_request.h"
#include "curl_context.h"
#include "curl_handler.h"
#include "../external/openssl/openssl/crypto.h"
#include "tools/system.h"
#include "tools/url.h"
#include "log/log.h"
#include "utils.h"
#include "async_task.h"
#include "core.h"
#include "proxy_settings.h"
#include "network_log.h"
#include "../corelib/enumerations.h"
#include "configuration/hosts_config.h"

using namespace core;

const milliseconds_t default_http_connect_timeout = 15000; // 15 sec
const milliseconds_t default_http_execute_timeout = 8000;

http_request_simple::http_request_simple(proxy_settings _proxy_settings, const std::string &_user_agent, stop_function _stop_func, progress_function _progress_func)
    : stop_func_(_stop_func),
    output_(new tools::binary_stream()),
    header_(new tools::binary_stream()),
    is_time_condition_(false),
    last_modified_time_(0),
    post_data_(0),
    post_data_size_(0),
    copy_post_data_(false),
    timeout_(default_http_execute_timeout),
    connect_timeout_(default_http_connect_timeout),
    range_from_(-1),
    range_to_(-1),
    is_post_form_(false),
    need_log_(true),
    keep_alive_(false),
    priority_(100),
    proxy_settings_(_proxy_settings),
    user_agent_(_user_agent),
    replace_log_function_([](tools::binary_stream&){}),
    progress_func_(_progress_func)
{
    assert(!user_agent_.empty());
}

http_request_simple::~http_request_simple()
{
    clear_post_data();
}

void http_request_simple::set_output_stream(std::shared_ptr<tools::stream> _output)
{
    output_ = _output;
}

void http_request_simple::set_need_log(bool _need)
{
    need_log_ = _need;
}

void http_request_simple::push_post_parameter(const std::wstring& name, const std::wstring& value)
{
    assert(!name.empty());

    post_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_parameter(const std::string& name, const std::string& value)
{
    post_parameters_[name] = value;
}

void http_request_simple::push_post_form_parameter(const std::wstring& name, const std::wstring& value)
{
    post_form_parameters_[tools::from_utf16(name)] = tools::from_utf16(value);
}

void http_request_simple::push_post_form_parameter(const std::string& name, const std::string& value)
{
    assert(!name.empty());
    post_form_parameters_[name] = value;
}

void http_request_simple::push_post_form_file(const std::wstring& name, const std::wstring& file_name)
{
    push_post_form_file(tools::from_utf16(name), tools::from_utf16(file_name));
}

void http_request_simple::push_post_form_file(const std::string& name, const std::string& file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    post_form_files_.insert(std::make_pair(name, file_name));
}

void http_request_simple::push_post_form_filedata(const std::wstring& name, const std::wstring& file_name)
{
    assert(!name.empty());
    assert(!file_name.empty());
    file_binary_stream file_data;
    file_data.file_name_ = file_name.substr(file_name.find_last_of(L"\\/") + 1);
    file_data.file_stream_.load_from_file(file_name);
    if (file_data.file_stream_.available())
        post_form_filedatas_.insert(std::make_pair(tools::from_utf16(name), file_data));
}

void http_request_simple::set_url(const std::wstring& url)
{
    set_url(tools::from_utf16(url));
}

void http_request_simple::set_url(const std::string& url)
{
    url_ = tools::encode_url(url);
}

void http_request_simple::set_modified_time_condition(time_t _modified_time)
{
    is_time_condition_ = true;
    last_modified_time_ = _modified_time;
}

void http_request_simple::set_connect_timeout(int32_t _timeout_ms)
{
    connect_timeout_ = _timeout_ms;
}

void http_request_simple::set_timeout(int32_t _timeout_ms)
{
    timeout_ = _timeout_ms;
}

std::string http_request_simple::get_post_param() const
{
    std::string result = "";
    if (!post_parameters_.empty())
    {
        std::stringstream ss_post_params;

        for (auto iter = post_parameters_.begin(); iter != post_parameters_.end(); ++iter)
        {
            if (iter != post_parameters_.begin())
                ss_post_params << '&';

            ss_post_params << iter->first;

            if (post_parameters_.size() > 1 || !iter->second.empty())
            {
                ss_post_params << '=' << iter->second;
            }
        }

        result = ss_post_params.str();
    }
    return result;
}

void http_request_simple::set_post_params(curl_context* _ctx)
{
    auto post_params = get_post_param();

    if (!post_params.empty())
        set_post_data(post_params.c_str(), (int32_t)post_params.length(), true);

    for (auto iter = post_form_parameters_.begin(); iter != post_form_parameters_.end(); ++iter)
    {
        if (!iter->second.empty())
            _ctx->set_form_field(iter->first.c_str(), iter->second.c_str());
    }

    for (auto iter = post_form_files_.begin(); iter != post_form_files_.end(); ++iter)
    {
        _ctx->set_form_file(iter->first.c_str(), iter->second.c_str());
    }

    for (auto iter = post_form_filedatas_.begin(); iter != post_form_filedatas_.end(); ++iter)
    {
        _ctx->set_form_filedata(iter->first.c_str(), tools::from_utf16(iter->second.file_name_).c_str(), iter->second.file_stream_);
    }

    if (post_data_ && post_data_size_)
        _ctx->set_post_fields(post_data_, post_data_size_);

    if (is_post_form_)
        _ctx->set_http_post();
    else
        _ctx->set_post();
}

bool http_request_simple::send_request(bool _post)
{
    curl_context ctx(output_, stop_func_, progress_func_, keep_alive_);

    const auto& proxy_settings = g_core->get_proxy_settings();

    if (!ctx.init(connect_timeout_, timeout_, proxy_settings, user_agent_))
    {
        assert(false);
        return false;
    }

    if (_post)
        set_post_params(&ctx);

    if (is_time_condition_)
        ctx.set_modified_time(last_modified_time_);

    if (range_from_ >= 0 && range_to_ > 0)
        ctx.set_range(range_from_, range_to_);

    ctx.set_need_log(need_log_);

    ctx.set_custom_header_params(custom_headers_);
    ctx.set_url(url_.c_str());

    ctx.set_replace_log_function(replace_log_function_);

    ctx.set_priority(priority_);

    if (!ctx.execute_request())
        return false;

    response_code_ = ctx.get_response_code();
    header_ = ctx.get_header();

    return true;
}

void* http_request_simple::send_request_async(bool _post, completion_function _completion_function)
{
    const auto& proxy_settings = g_core->get_proxy_settings();

    auto ctx = std::make_shared<curl_context>(output_, stop_func_, progress_func_, keep_alive_);
    if (!ctx->init(connect_timeout_, timeout_, proxy_settings, user_agent_))
    {
        if (_completion_function)
            _completion_function(false);
        return 0;
    }

    if (_post)
        set_post_params(ctx.get());

    if (is_time_condition_)
        ctx->set_modified_time(last_modified_time_);

    if (range_from_ >= 0 && range_to_ > 0)
        ctx->set_range(range_from_, range_to_);

    ctx->set_need_log(need_log_);

    ctx->set_custom_header_params(custom_headers_);
    ctx->set_url(url_.c_str());

    ctx->set_replace_log_function(replace_log_function_);

    ctx->execute_request_async([this, ctx, _completion_function](bool _success)
    {
        if (_success)
        {
            response_code_ = ctx->get_response_code();
            header_ = ctx->get_header();
        }

        if (_completion_function)
            _completion_function(_success);
    });

    return ctx->curl_;
}

void http_request_simple::post_async(completion_function _completion_function)
{
    send_request_async(true, _completion_function);
}

void* http_request_simple::get_async(completion_function _completion_function)
{
    return send_request_async(false, _completion_function);
}

bool http_request_simple::post()
{
    return send_request(true);
}

bool http_request_simple::get()
{
    return send_request(false);
}

void http_request_simple::set_range(int64_t _from, int64_t _to)
{
    range_from_ = _from;
    range_to_ = _to;
}

std::shared_ptr<tools::stream> http_request_simple::get_response()
{
    return output_;
}

std::shared_ptr<tools::binary_stream> http_request_simple::get_header()
{
    return header_;
}

long http_request_simple::get_response_code()
{
    return response_code_;
}

void http_request_simple::get_post_parameters(std::map<std::string, std::string>& _params)
{
    _params = post_parameters_;
}

void http_request_simple::set_custom_header_param(const std::string& _value)
{
    custom_headers_.push_back(_value);
}

void http_request_simple::clear_post_data()
{
    if (copy_post_data_ && post_data_)
        free(post_data_);

    copy_post_data_ = false;
    post_data_ = 0;
    post_data_size_ = 0;
}

void http_request_simple::set_post_data(const char* _data, int32_t _size, bool _copy_post_data)
{
    assert(_data);
    assert(_size);


    clear_post_data();

    copy_post_data_ = _copy_post_data;
    post_data_size_ = _size;

    if (_copy_post_data)
    {
        post_data_ = (char*) malloc(_size);
        memcpy(post_data_, _data, _size);
    }
    else
    {
        post_data_ = (char*) _data;
    }
}


std::vector<std::mutex*> http_request_simple::ssl_sync_objects;

static unsigned long id_function(void)
{
    return tools::system::get_current_thread_id();
}


static void locking_function( int32_t mode, int32_t n, const char *file, int32_t line )
{
    if ( mode & CRYPTO_LOCK )
        http_request_simple::ssl_sync_objects[n]->lock();
    else
        http_request_simple::ssl_sync_objects[n]->unlock();
}


void core::http_request_simple::init_global()
{
    auto lock_count = CRYPTO_num_locks();
    http_request_simple::ssl_sync_objects.resize(lock_count);
    for (auto i = 0;  i < lock_count;  i++)
        http_request_simple::ssl_sync_objects[i] = new std::mutex();

    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
}

void core::http_request_simple::shutdown_global()
{
    if (!http_request_simple::ssl_sync_objects.size())
        return;

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    for (auto i = 0;  i < CRYPTO_num_locks(  );  i++)
        delete(http_request_simple::ssl_sync_objects[i]);

    http_request_simple::ssl_sync_objects.clear();
}

void core::http_request_simple::set_post_form(bool _is_post_form)
{
    is_post_form_ = _is_post_form;
}

void core::http_request_simple::set_keep_alive()
{
    if (keep_alive_)
        return;

    keep_alive_ = true;

    custom_headers_.push_back("Connection: keep-alive");
}

void core::http_request_simple::set_priority(priority_t _priority)
{
    priority_ = _priority;
}

void core::http_request_simple::set_etag(const char *etag)
{
    if (etag && strlen(etag))
    {
        std::stringstream ss;
        ss << "If-None-Match: \"" << etag << "\"";
        custom_headers_.push_back(ss.str().c_str());
    }
}

void core::http_request_simple::set_replace_log_function(replace_log_function _func)
{
    replace_log_function_ = _func;
}

std::string core::http_request_simple::get_post_url()
{
    return url_ + "?" + get_post_param();
}

proxy_settings core::http_request_simple::get_user_proxy() const
{
    return proxy_settings_;
}

void core::http_request_simple::replace_host(const hosts_map& _hosts)
{
    std::string::const_iterator it_prot = url_.begin();

    size_t pos_start = 0;

    auto pos_prot = url_.find("://");
    if (pos_prot != std::string::npos)
    {
        pos_start = pos_prot + 3;
    }

    if (pos_prot >= url_.length())
        return;

    std::stringstream ss_host;

    size_t pos_end = pos_start;

    while (url_[pos_end] != '/' && url_[pos_end] != '?' && url_[pos_end] != ':' && pos_end < url_.length())
    {
        ss_host << url_[pos_end];
        ++pos_end;
    }

    const auto host = ss_host.str();

    if (host.empty())
        return;

    url_.replace(pos_start, pos_end - pos_start, _hosts.get_host_alt(host));
}
