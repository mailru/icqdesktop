#include "stdafx.h"

#include "tools/system.h"

#include "core.h"
#include "curl_handler.h"
#include "network_log.h"

#include "curl_context.h"

static size_t write_header_function(void *contents, size_t size, size_t nmemb, void *userp);
static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded);
static int32_t trace_function(CURL* _handle, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp);

core::curl_context::curl_context(std::shared_ptr<tools::stream> _output, http_request_simple::stop_function _stop_func, http_request_simple::progress_function _progress_func, bool _keep_alive)
    :
    curl_(curl_handler::instance().get_handle()),
    output_(_output),
    header_(new core::tools::binary_stream()),
    log_data_(new core::tools::binary_stream()),
    stop_func_(_stop_func),
    progress_func_(_progress_func),
    header_chunk_(0),
    post(NULL),
    last(NULL),
    need_log_(true),
    keep_alive_(_keep_alive),
    priority_(100),
    timeout_(0),
    replace_log_function_([](tools::binary_stream&){}),
    bytes_transferred_pct_(0)
{
}

core::curl_context::~curl_context()
{
    curl_handler::instance().release_handle(curl_);

    if (header_chunk_)
        curl_slist_free_all(header_chunk_);
}

bool core::curl_context::init(milliseconds_t _connect_timeout, milliseconds_t _timeout, core::proxy_settings _proxy_settings, const std::string &_user_agent)
{
    assert(!_user_agent.empty());

    timeout_ = _timeout;

    if (!curl_)
        return false;

    if (platform::is_windows() && !platform::is_windows_vista_or_late())
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    else
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);

    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void *)this);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, (void *)this);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, write_header_function);
    curl_easy_setopt(curl_, CURLOPT_PROGRESSDATA, (void *)this);
    curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl_, CURLOPT_PROGRESSFUNCTION, progress_callback);

    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 5L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 5L);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "gzip");

    curl_easy_setopt(curl_, CURLOPT_USERAGENT, _user_agent.c_str());

    curl_easy_setopt(curl_, CURLOPT_DEBUGDATA, (void*) this);
    curl_easy_setopt(curl_, CURLOPT_DEBUGFUNCTION, trace_function);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, 0);

    if (_connect_timeout > 0)
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, _connect_timeout);

    if (_proxy_settings.use_proxy_)
    {
        curl_easy_setopt(curl_, CURLOPT_PROXY, tools::from_utf16(_proxy_settings.proxy_server_).c_str());

        if (_proxy_settings.proxy_port_ != proxy_settings::default_proxy_port)
        {
            curl_easy_setopt(curl_, CURLOPT_PROXYPORT, (long) _proxy_settings.proxy_port_);
        }
        curl_easy_setopt(curl_, CURLOPT_PROXYTYPE, (long) _proxy_settings.proxy_type_);

        if (_proxy_settings.need_auth_)
        {
            curl_easy_setopt(curl_, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl_, CURLOPT_PROXYUSERPWD, tools::from_utf16(_proxy_settings.login_ + L":" + _proxy_settings.password_).c_str());
        }
    }

    return true;
}

void core::curl_context::set_replace_log_function(replace_log_function _func)
{
    replace_log_function_ = _func;
}

bool core::curl_context::is_need_log()
{
    return need_log_;
}

void core::curl_context::set_need_log(bool _need)
{
    need_log_ = _need;
}

void core::curl_context::write_log_data(const char* _data, uint32_t _size)
{
    log_data_->write(_data, _size);
}

void core::curl_context::write_log_string(const std::string& _log_string)
{
    log_data_->write<std::string>(_log_string);
}

void core::curl_context::set_range(int64_t _from, int64_t _to)
{
    assert(_from >= 0);
    assert(_to > 0);
    assert(_from < _to);

    std::stringstream ss_range;
    ss_range << _from << "-" << _to;

    curl_easy_setopt(curl_, CURLOPT_RANGE, ss_range.str().c_str());
}

void core::curl_context::set_url(const char* sz_url)
{
    curl_easy_setopt(curl_, CURLOPT_URL, sz_url);

    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);

    if (!keep_alive_)
    {
        curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, "");
    }
}

void core::curl_context::set_post()
{
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
}

void core::curl_context::set_http_post()
{
    curl_easy_setopt(curl_, CURLOPT_HTTPPOST, post);
}

void core::curl_context::set_post_fields(const char* sz_fields, int32_t _size)
{
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, (long) _size);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, sz_fields);
}

long core::curl_context::get_response_code()
{
    long response_code = 0;

    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);

    return response_code;
}

void core::curl_context::set_modified_time(time_t _last_modified_time)
{
    curl_easy_setopt(curl_, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
    curl_easy_setopt(curl_, CURLOPT_TIMEVALUE, _last_modified_time);
}

std::shared_ptr<core::tools::binary_stream> core::curl_context::get_header()
{
    return header_;
}

void core::curl_context::set_form_field(const char* _field_name, const char* _value)
{
    curl_formadd(&post, &last, CURLFORM_COPYNAME, _field_name,
        CURLFORM_COPYCONTENTS, _value, CURLFORM_END);
}

void core::curl_context::set_form_file(const char* _field_name, const char* _file_name)
{
    curl_formadd(&post, &last, CURLFORM_COPYNAME, _field_name, CURLFORM_FILE, _file_name,
        CURLFORM_END);
}

void core::curl_context::set_form_filedata(const char* _field_name, const char* _file_name, tools::binary_stream& _data)
{
    long size = _data.available();//it should be long
    const auto data = _data.read_available();
    _data.reset_out();
    curl_formadd(&post, &last,
                    CURLFORM_COPYNAME, _field_name,
                    CURLFORM_BUFFER, _file_name,
                    CURLFORM_BUFFERPTR, data,
                    CURLFORM_BUFFERLENGTH, size,
                    CURLFORM_CONTENTTYPE, "application/octet-stream",
                    CURLFORM_END);
}

bool core::curl_context::execute_request()
{
    const auto start = std::chrono::steady_clock().now();

    auto handler = curl_handler::instance().perform(priority_, timeout_, curl_);
    assert(handler.valid());

    handler.wait();

    const CURLcode res = handler.get();

    const auto finish = std::chrono::steady_clock().now();

    std::stringstream error;
    error << "curl_easy_perform result is ";
    error << res << '\n';
    error << "completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish -start).count() << " ms";
    error << std::endl;

    write_log_string(error.str());

    replace_log_function_(*log_data_);

    g_core->get_network_log().write_data(*log_data_);

    return (res == CURLE_OK);
}

void core::curl_context::execute_request_async(http_request_simple::completion_function _completion_function)
{
    curl_handler::instance().perform_async(priority_, timeout_, curl_, _completion_function);
}

void core::curl_context::set_custom_header_params(const std::list<std::string>& _params)
{
    for (auto parameter : _params)
        header_chunk_ = curl_slist_append(header_chunk_, parameter.c_str());

    if (header_chunk_)
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_chunk_);
}

void core::curl_context::set_priority(priority_t _priority)
{
    priority_ = _priority;
}

static size_t write_header_function(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    auto ctx = (core::curl_context *) userp;
    ctx->header_->reserve((uint32_t)realsize);
    ctx->header_->write((char *)contents, (uint32_t)realsize);

    return realsize;
}

static size_t write_memory_callback(void* _contents, size_t _size, size_t _nmemb, void* _userp)
{
    size_t realsize = _size * _nmemb;
    auto ctx = (core::curl_context*) _userp;
    ctx->output_->write((char*) _contents, (uint32_t) realsize);

    if (ctx->is_need_log())
    {
        ctx->write_log_data((const char*) _contents, (uint32_t) realsize);
    }

    return realsize;
}

static int32_t progress_callback(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double /*NowUploaded*/)
{
    auto cntx = (core::curl_context*) ptr;

    assert(cntx->bytes_transferred_pct_ >= 0);
    assert(cntx->bytes_transferred_pct_ <= 100);

    if (cntx->stop_func_ && cntx->stop_func_())
    {
        return -1;
    }

    const auto file_too_small = (TotalToDownload <= 1);
    const auto skip_progress = (file_too_small || !cntx->progress_func_);
    if (skip_progress)
    {
        return 0;
    }

    const auto bytes_transferred_pct = (int32_t)((NowDownloaded * 100) / TotalToDownload);
    assert(bytes_transferred_pct >= 0);
    assert(bytes_transferred_pct <= 100);

    const auto percentage_updated = (bytes_transferred_pct != cntx->bytes_transferred_pct_);
    if (!percentage_updated)
    {
        return 0;
    }

    cntx->bytes_transferred_pct_ = bytes_transferred_pct;

    cntx->progress_func_((int64_t)TotalToDownload, (int64_t)NowDownloaded, bytes_transferred_pct);

    return 0;
}

std::vector<std::string> get_filter_keywords()
{
    std::vector<std::string> keywords;
    keywords.push_back("schannel:");
    keywords.push_back("STATE:");

    return keywords;
}

const std::string filter1 = "schannel:";
const std::string filter2 = "STATE:";

static int32_t trace_function(CURL* _handle, curl_infotype _type, unsigned char* _data, size_t _size, void* _userp)
{
    auto ctx = (core::curl_context*) _userp;
    if (!ctx->is_need_log())
        return 0;

    std::stringstream ss;

    const char *text = "";

    (void)_userp;
    (void)_handle; /* prevent compiler warning */

    switch (_type)
    {
    case CURLINFO_TEXT:
        {
            if (filter1.length() < _size && memcmp(_data, filter1.c_str(), filter1.length()) == 0)
                return 0;

            if (filter2.length() < _size && memcmp(_data, filter2.c_str(), filter2.length()) == 0)
                return 0;
        }
        break;
    case CURLINFO_HEADER_OUT:
    //    text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
    case CURLINFO_SSL_DATA_OUT:
    //    text = "=> Send data";
        break;
    case CURLINFO_HEADER_IN:
    //    text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
    case CURLINFO_SSL_DATA_IN:
    //    text = "<= Recv data";
        return 0;
    default:
        break;
    }

    core::tools::binary_stream bs;

    if (*text != '\0')
    {
        ctx->write_log_string(text);
        ctx->write_log_string("\n");
    }

    ctx->write_log_data((const char*) _data, (uint32_t) _size);

    return 0;
}
