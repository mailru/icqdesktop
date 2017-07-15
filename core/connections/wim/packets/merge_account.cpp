#include "stdafx.h"
#include "merge_account.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

using namespace core;
using namespace wim;

#define WIM_API_ATTACH_UIN_HOST "https://www.icq.com/smsreg/mergeAccount.php"
#define WIM_API_REPLACE_ACCOUNT_HOST "https://www.icq.com/mergeAccount"

merge_account::merge_account(
    const wim_packet_params& _from_params, const wim_packet_params& _to_params)
    :   wim_packet(_to_params)
    ,   from_params_(_from_params)
{
}

merge_account::~merge_account()
{
}

int32_t merge_account::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (from_params_.a_token_.empty())
        return wpie_invalid_login;

    auto from_uin_signed_url = std::string();
    {
        core::http_request_simple request_from(_request->get_user_proxy(), utils::get_user_agent());
        const std::string host = WIM_API_REPLACE_ACCOUNT_HOST;
        request_from.set_url(host);
        request_from.push_post_parameter("a", escape_symbols(from_params_.a_token_));
        request_from.push_post_parameter("k", escape_symbols(from_params_.dev_id_));

        time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - from_params_.time_offset_;
        request_from.push_post_parameter("ts", tools::from_int64(ts));

        std::map<std::string, std::string> post_params;
        request_from.get_post_parameters(post_params);

        request_from.push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, post_params, from_params_, true)));
        from_uin_signed_url = request_from.get_post_url();
    }

    {
        const std::string host = WIM_API_ATTACH_UIN_HOST;
        _request->set_url(host);

        _request->push_post_parameter("a", escape_symbols(params_.a_token_));
        _request->push_post_parameter("k", escape_symbols(params_.dev_id_));
        _request->push_post_parameter("f", "json");
        _request->push_post_parameter("r", core::tools::system::generate_guid());

        time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - from_params_.time_offset_;
        _request->push_post_parameter("ts", tools::from_int64(ts));

        _request->push_post_parameter("fromUinSignedUrl", escape_symbols(from_uin_signed_url));

        std::map<std::string, std::string> post_params;
        _request->get_post_parameters(post_params);

        _request->push_post_parameter("sig_sha256", escape_symbols(get_url_sign(host, post_params, params_, true)));
    }
    return 0;
}

int32_t merge_account::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t merge_account::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t) _request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t merge_account::on_empty_data()
{
    return 0;
}
