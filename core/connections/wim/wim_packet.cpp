#include "stdafx.h"

#include "wim_packet.h"

#include "../../http_request.h"
#include "../../tools/hmac_sha_base64.h"
#include "../../log/log.h"
#include "../../utils.h"

using namespace core;
using namespace wim;

wim_packet::wim_packet(const wim_packet_params& params)
    :   params_(params),
    http_code_(uint32_t(-1)),
    status_code_(uint32_t(-1)),
    status_detail_code_(uint32_t(-1)),
    repeat_count_(uint32_t(0)),
    can_change_hosts_scheme_(false),
    hosts_scheme_changed_(false)
{

}

wim_packet::~wim_packet()
{

}

bool wim_packet::support_async_execution() const
{
    return false;
}

int32_t wim_packet::execute()
{
    auto request = std::make_shared<core::http_request_simple>(params_.proxy_, utils::get_user_agent(), params_.stop_handler_);

    ++repeat_count_;

    int32_t err = init_request(request);
    if (err != 0)
        return err;

    request->replace_host(params_.hosts_);

    err = execute_request(request);
    if (err != 0)
        return err;

    auto response = std::static_pointer_cast<tools::binary_stream>(request->get_response());
    assert(response);
    err = parse_response(response);
    return err;
}

void wim_packet::execute_async(handler_t _handler)
{
    assert(support_async_execution());

    auto request = std::make_shared<core::http_request_simple>(params_.proxy_, utils::get_user_agent(), params_.stop_handler_);

    int32_t err = init_request(request);
    if (err != 0)
    {
        _handler(err);
        return;
    }

    request->replace_host(params_.hosts_);

    std::weak_ptr<wim_packet> wr_this(shared_from_this());

    execute_request_async(request, [wr_this, request, _handler](int32_t _err)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_err != 0)
        {
            _handler(_err);
            return;
        }

        auto response = std::static_pointer_cast<tools::binary_stream>(request->get_response());
        assert(response);
        const auto err = ptr_this->parse_response(response);
        _handler(err);
    });
}

bool wim_packet::is_stopped() const
{
    return params_.stop_handler_();
}

int32_t wim_packet::init_request(std::shared_ptr<core::http_request_simple> request)
{
    return 0;
}

int32_t wim_packet::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->get())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (request->get_header()->available())
    {
        auto header = request->get_header();
        uint32_t size = header->available();
        auto buf = (const char *)header->read(size);

        if (buf && size)
        {
            header_str_.assign(buf, size);
        }

        header->reset_out();
    }
    
    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();

        return wpie_http_error;
    }

    return 0;
}

void wim_packet::execute_request_async(std::shared_ptr<core::http_request_simple> _request, handler_t _handler)
{
    std::weak_ptr<wim_packet> wr_this(shared_from_this());

    _request->get_async([_request, _handler, wr_this](bool _success)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_success)
        {
            if (_handler)
                _handler(wpie_network_error);
            return;
        }

        ptr_this->http_code_ = (uint32_t) _request->get_response_code();

        if (_request->get_header()->available())
        {
            auto header = _request->get_header();
            uint32_t size = header->available();
            auto buf = (const char *)header->read(size);

            if (buf && size)
            {
                ptr_this->header_str_.assign(buf, size);
            }

            header->reset_out();
        }

        if (!_handler)
            return;

        if (ptr_this->http_code_ == 200)
        {
            _handler(0);
        }
        else if (ptr_this->http_code_ > 400 && ptr_this->http_code_ < 500)
        {
            _handler(ptr_this->on_http_client_error());
        }
        else
        {
            _handler(wpie_http_error);
        }
    });
}

void wim_packet::load_response_str(const char* buf, unsigned size)
{
    assert(buf && size);
    if (buf && size)
    {
        response_str_.assign(buf, size);
    }
    else
    {
        response_str_.clear();
    }
}

const std::string& wim_packet::response_str() const
{
    return response_str_;
}

const std::string& wim_packet::header_str() const
{
    return header_str_;
}

int32_t wim_packet::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    if (!response->available())
        return wpie_http_empty_response;

    int32_t err = 0;

    response->write((char) 0);

    uint32_t size = response->available();

    load_response_str((const char*) response->read(size), size);

    response->reset_out();

    try
    {
        const auto json_str = response->read(response->available());

#ifdef DEBUG__OUTPUT_NET_PACKETS
        puts(json_str);
#endif // DEBUG__OUTPUT_NET_PACKETS

#ifdef DEBUG
        const std::string json_str_dbg(json_str);
#endif

        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        auto iter_response = doc.FindMember("response");
        if (iter_response == doc.MemberEnd())
            return wpie_http_parse_response;

        auto iter_status = iter_response->value.FindMember("statusCode");
        if (iter_status == iter_response->value.MemberEnd() || !iter_status->value.IsUint())
            return wpie_http_parse_response;

        status_code_ = iter_status->value.GetUint();

        auto iter_status_text = iter_response->value.FindMember("statusText");
        if (iter_status_text != iter_response->value.MemberEnd() && iter_status_text->value.IsString())
            status_text_ = iter_status_text->value.GetString();

        auto iter_status_detail = iter_response->value.FindMember("statusDetailCode");
        if (iter_status_detail != iter_response->value.MemberEnd() && iter_status_detail->value.IsUint())
            status_detail_code_ = (uint32_t) iter_status_detail->value.GetUint();

        if (status_code_ == 200)
        {
            auto iter_data = iter_response->value.FindMember("data");
            if (iter_data == iter_response->value.MemberEnd())
            {
                err = on_empty_data();
                if (err != 0)
                    return err;
            }
            else
            {
                return parse_response_data(iter_data->value);
            }
        }
        else
        {
            auto iter_data = iter_response->value.FindMember("data");
            if (iter_data != iter_response->value.MemberEnd())
            {
                parse_response_data_on_error(iter_data->value);
            }

            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}

int32_t wim_packet::on_response_error_code()
{
    switch (status_code_)
    {
    case INVALID_REQUEST:
        return wpie_error_invalid_request;
    case AUTHN_REQUIRED:
    case MISSING_REQUIRED_PARAMETER:
        return wpie_error_need_relogin;
    case TARGET_RATE_LIMIT_REACHED:
        return wpie_error_rate_limit;
    case SEND_IM_RATE_LIMIT:
        return wpie_error_too_fast_sending;
    default:
        return wpie_error_message_unknown;
    }
}

int32_t wim_packet::on_http_client_error()
{
    return wpie_client_http_error;
}


int32_t wim_packet::on_empty_data()
{
    return wpie_http_parse_response;
}

std::string wim_packet::escape_symbols(const std::string& _data)
{
    std::stringstream ss_out;

    std::array<char, 100> buffer;

    for (uint32_t i = 0; i < _data.size(); i++)
    {
        auto sym = _data[i];

        if (core::tools::is_latin(sym) || core::tools::is_digit(sym) || strchr("-._~", sym))
        {
            ss_out << sym;
        }
        else
        {
#ifdef _WIN32
            sprintf_s(buffer.data(), buffer.size(), "%%%.2X", (unsigned char) sym);
#else
            sprintf(buffer.data(), "%%%.2X", (unsigned char) sym);
#endif
            ss_out << buffer.data();
        }
    }

    return ss_out.str();
}

std::string wim_packet::escape_symbols_data(const char* _data, uint32_t _len)
{
    std::stringstream ss_out;

    char buffer[100];

    const char* p = _data;

    for (uint32_t i = 0; i < _len; i++)
    {
        char sym = *p;

        if (core::tools::is_latin(sym) || core::tools::is_digit(sym))
        {
            ss_out << sym;
        }
        else
        {
            sprintf(buffer, "%%%.2X", (unsigned char) sym);
            ss_out << buffer;
        }

        ++p;
    }

    return ss_out.str();
}

std::string wim_packet::create_query_from_map(const str_2_str_map& _params)
{
    std::stringstream ss_query;

    for (auto iter = _params.begin(); iter != _params.end(); iter++)
    {
        if (iter != _params.begin())
        {
            ss_query << "&";
        }

        ss_query << iter->first << "=" << iter->second;
    }

    return ss_query.str();
}

std::string wim_packet::detect_digest(const std::string& hashed_data, const std::string& session_key)
{
    if (hashed_data.empty() || session_key.empty())
    {
        assert(false);
        return "";
    }

    std::vector<uint8_t> hash_data_vector(hashed_data.size());
    memcpy(&hash_data_vector[0], hashed_data.c_str(), hashed_data.size());

    std::vector<uint8_t> session_key_vector(session_key.size());
    memcpy(&session_key_vector[0], session_key.c_str(), session_key.size());

    return core::tools::base64::hmac_base64(hash_data_vector, session_key_vector);
}


std::string wim_packet::get_url_sign(const std::string& _host, const str_2_str_map& _params, const wim_packet_params& _wim_params,  bool _post_method, bool make_escape_symbols/* = true*/)
{
    std::string http_method = _post_method ? "POST" : "GET";
    std::string query_string = create_query_from_map(_params);

    std::string hash_data = http_method + "&" + (make_escape_symbols ? escape_symbols(_host) : _host) + "&" + (make_escape_symbols ? escape_symbols(query_string) : query_string);

    return detect_digest(hash_data, _wim_params.session_key_);
}

std::string wim_packet::format_get_params(const std::map<std::string, std::string>& _params)
{
    std::stringstream ss_out;

    auto first = true;

    for (const auto& it : _params)
    {
        if (!first)
        {
            ss_out << "&";
        }

        first = false;

        ss_out << it.first.c_str() << "=" << it.second.c_str();
    }

    return ss_out.str();
}

int32_t wim_packet::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

void wim_packet::parse_response_data_on_error(const rapidjson::Value& _data)
{
}

const std::string core::wim::wim_packet::get_rand_float() const
{
    std::srand((uint32_t) time(0));
    std::stringstream ss_rand;
    ss_rand << "0." << (int32_t) std::rand();

    return ss_rand.str();
}


const wim_packet_params& core::wim::wim_packet::get_params() const
{
    return params_;
}

uint32_t core::wim::wim_packet::get_repeat_count() const
{
    return repeat_count_;
}

void core::wim::wim_packet::set_repeat_count(const uint32_t _count)
{
    repeat_count_ = _count;
}

void core::wim::wim_packet::replace_log_messages(tools::binary_stream& _bs)
{
    uint32_t sz = _bs.available();
    char* logdata = _bs.get_data();
    char* end = logdata + sz;

    if (!logdata || !sz)
        return;

    std::vector<std::string> vmarkers;
    vmarkers.push_back("\"text\":");
    vmarkers.push_back("\"message\":");
    
    auto replace_marker = [logdata, end](const std::string& _marker)
    {
        char* cursor = logdata;

        while (cursor < end)
        {
            cursor = std::search(cursor, end, _marker.c_str(), _marker.c_str() + _marker.length());
            cursor += _marker.length();
            if (cursor >= end)
                return;

            while (cursor < end && *cursor++ != '\"') {}


            while (cursor < end)
            {
                if (*cursor == '\"')
                    break;

                if (*cursor == '\\')
                {
                    *cursor = '*';

                    ++cursor;
                    if (cursor >= end)
                        return;
                }

                *cursor = '*';

                ++cursor;
            }
        }
    };
    

    for (const auto& _marker : vmarkers)
    {
        replace_marker(_marker);
    }
}

bool wim_packet::can_change_hosts_scheme() const
{
    return can_change_hosts_scheme_;
}

void wim_packet::set_can_change_hosts_scheme(const bool _can)
{
    can_change_hosts_scheme_ = _can;
}

void wim_packet::change_hosts_scheme()
{
    hosts_scheme_changed_ = !hosts_scheme_changed_;

    params_.hosts_.change_scheme();
}

bool wim_packet::is_hosts_scheme_changed() const
{
    return hosts_scheme_changed_;
}

const hosts_map& wim_packet::get_hosts_scheme() const
{
    return params_.hosts_;
}
