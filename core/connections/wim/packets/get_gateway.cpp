#include "stdafx.h"
#include "get_gateway.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../utils.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;


get_gateway::get_gateway(const wim_packet_params& _params, const std::string& _file_name, int64_t _file_size)
    :	wim_packet(_params), file_name_(_file_name), file_size_(_file_size)
{
}


get_gateway::~get_gateway()
{
}


int32_t get_gateway::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    std::string host = "https://files.icq.com/files/init";

    std::stringstream ss_file_size;
    ss_file_size << file_size_;

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = tools::from_int64(ts);
    params["size"] = ss_file_size.str();
    params["filename"] = escape_symbols(file_name_);
    params["client"] = escape_symbols(utils::get_app_name());
    params["r"] = core::tools::system::generate_guid();

    const auto sha256 = escape_symbols(get_url_sign(host, params, params_, false));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url;
    ss_url << host << "?" << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t get_gateway::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(_response->available())).HasParseError())
        return wpie_error_parse_response;

    auto iter_status = doc.FindMember("status");
    if (iter_status == doc.MemberEnd() || !iter_status->value.IsInt())
        return wpie_http_parse_response;

    status_code_ = iter_status->value.GetInt();
    if (status_code_ == 200)
    {
        auto iter_data = doc.FindMember("data");
        if (iter_data == doc.MemberEnd() || !iter_data->value.IsObject())
            return wpie_http_parse_response;

        auto iter_host = iter_data->value.FindMember("host");
        auto iter_url = iter_data->value.FindMember("url");

        if (iter_host == iter_data->value.MemberEnd() || iter_url == iter_data->value.MemberEnd() || 
            !iter_host->value.IsString() || !iter_url->value.IsString())
            return wpie_http_parse_response;

        host_ = iter_host->value.GetString();
        url_ = iter_url->value.GetString();
    }

    return 0;
}

int32_t get_gateway::on_http_client_error()
{
    if (http_code_ == 413)
        return wpie_error_too_large_file;

    return wpie_client_http_error;
}


std::string get_gateway::get_host() const
{
    return host_;
}

std::string get_gateway::get_url() const
{
    return url_;
}