#include "stdafx.h"
#include "client_login.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../../common.shared/version_info.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../tools/md5.h"


#define WIM_API_LOGIN_HOST		"https://api.login.icq.net/auth/clientLogin"
#define WIM_APP_TOKENTYPE		"longterm"

using namespace core;
using namespace wim;

client_login::client_login(
    const wim_packet_params& params,
    const std::string& login,
    const std::string& password)
    :
wim_packet(params),
    login_(login),
    password_(password),
    expired_in_(0),
    host_time_(0),
    time_offset_(0)
{
    set_can_change_hosts_scheme(true);
}


client_login::~client_login()
{
}


void client_login::set_product_guid_8x(const std::string& _guid)
{
    product_guid_8x_ = _guid;
}

int32_t client_login::parse_response_data(const rapidjson::Value& _data)
{
    try
    {
        auto iter_session_secret = _data.FindMember("sessionSecret");
        if (iter_session_secret == _data.MemberEnd() || !iter_session_secret->value.IsString())
            return wpie_http_parse_response;

        session_secret_ = iter_session_secret->value.GetString();

        std::vector<uint8_t> data(session_secret_.size());
        std::vector<uint8_t> password(password_.size());
        memcpy(&data[0], session_secret_.c_str(), session_secret_.size());
        memcpy(&password[0], password_.c_str(), password_.size());
        session_key_ = core::tools::base64::hmac_base64(data, password);

        auto iter_host_time = _data.FindMember("hostTime");
        if (iter_host_time == _data.MemberEnd() || !iter_host_time->value.IsUint())
            return wpie_http_parse_response;

        host_time_ = iter_host_time->value.GetUint();

        auto iter_token = _data.FindMember("token");
        if (iter_token == _data.MemberEnd() || !iter_token->value.IsObject())
            return wpie_http_parse_response;

        auto iter_expired_in = iter_token->value.FindMember("expiresIn");
        auto iter_a = iter_token->value.FindMember("a");
        if (iter_expired_in == iter_token->value.MemberEnd() || iter_a == iter_token->value.MemberEnd() || 
            !iter_expired_in->value.IsUint() || !iter_a->value.IsString())
            return wpie_http_parse_response;

        expired_in_ = iter_expired_in->value.GetUint();
        a_token_ = iter_a->value.GetString();

        time_offset_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - host_time_;
    }
    catch (const std::exception&)
    {
        return wpie_http_parse_response;
    }

    return 0;
}


int32_t client_login::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (login_.empty() || password_.empty())
        return wpie_invalid_login;

    _request->set_url(WIM_API_LOGIN_HOST);
    _request->push_post_parameter("f", "json");
    _request->push_post_parameter("devId", params_.dev_id_);
    _request->push_post_parameter("s", escape_symbols(login_));

    std::string password = escape_symbols(password_);
    _request->push_post_parameter("pwd", password);

    if (!product_guid_8x_.empty())
    {
        std::string guid = escape_symbols(product_guid_8x_);
        _request->push_post_parameter("installationToken", guid);
    }

    _request->push_post_parameter("clientName", escape_symbols(utils::get_app_name()));
    _request->push_post_parameter("clientVersion", core::tools::version_info().get_version());
    _request->push_post_parameter("tokenType", WIM_APP_TOKENTYPE);
    _request->push_post_parameter("r", core::tools::system::generate_guid());

    _request->set_keep_alive();
    
    _request->set_replace_log_function([password](tools::binary_stream& _bs)
    {
        uint32_t sz = _bs.available();
        char* logdata = _bs.get_data();

        if (!logdata || !sz)
            return;

        std::string marker("&pwd=");

        std::string search_text = marker + password;

        char* cursor = std::search(logdata, logdata + sz, search_text.c_str(), search_text.c_str() + search_text.length());

        if (cursor >= logdata + sz)
        {
           // assert(false);
            return;
        }

        cursor += marker.length();
        if (cursor >= logdata + sz)
        {
            assert(false);
            return;
        }

        for (uint32_t i  = 0; i < password.length() && cursor < logdata + sz; ++i)
        {
            *cursor++ = '*';
        }
    });

    return 0;
}

int32_t client_login::on_response_error_code()
{
    switch (status_code_)
    {
        case 330:
        {
            if (status_detail_code_ == 3012)
            {
                return wpie_wrong_login_2x_factor;
            }

            return wpie_wrong_login;
        }
        case 408:
        {
            return wpie_request_timeout;
        }
        default:
        {
            return wpie_login_unknown_error;
        }

    }
}


int32_t client_login::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}
