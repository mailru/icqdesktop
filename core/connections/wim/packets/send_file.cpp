#include "stdafx.h"
#include "send_file.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;


send_file_params::send_file_params()
    :	size_already_sent_(0),
    current_chunk_size_(0),
    full_data_size_(0),
    data_(0),
    session_id_(0)
{

}




send_file::send_file(
    const wim_packet_params& _params,
    const send_file_params& _chunk,
    const std::string& _host,
    const std::string& _url)
    :	wim_packet(_params),
    chunk_(_chunk),
    host_(_host),
    url_(_url)
{
}


send_file::~send_file()
{
}


int32_t send_file::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_timeout(15000);

    _request->set_need_log(params_.full_log_);
    _request->set_keep_alive();

    std::stringstream ss_id;
    ss_id << "Session-ID: " << chunk_.session_id_;
    _request->set_custom_header_param(ss_id.str());

    std::stringstream ss_disp;
    ss_disp << "Content-Disposition: attachment; filename=\"" << chunk_.file_name_ << "\"";
    _request->set_custom_header_param(ss_disp.str());

    _request->set_custom_header_param("Content-Type: application/octet-stream");

    std::stringstream ss_range;
    ss_range << "bytes " << chunk_.size_already_sent_ << "-" << (chunk_.size_already_sent_ + chunk_.current_chunk_size_ - 1) << "/" <<  chunk_.full_data_size_;

    std::stringstream ss_content_range;
    ss_content_range << "Content-Range: " << ss_range.str();
    _request->set_custom_header_param(ss_content_range.str());

    _request->set_post_data(chunk_.data_, (uint32_t)chunk_.current_chunk_size_, false);

    std::stringstream ss_url;
    ss_url << "https://" << host_ << url_;

    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = tools::from_int64(ts);
    params["r"] = core::tools::system::generate_guid();

    const auto sha256 = escape_symbols(get_url_sign(ss_url.str(), params, params_, true));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << "?" << format_get_params(params);

    _request->set_url(ss_url_signed.str());

    return 0;
}

int32_t send_file::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(size)).HasParseError())
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

        auto iter_static_url = iter_data->value.FindMember("static_url");
        if (iter_static_url == iter_data->value.MemberEnd() || !iter_static_url->value.IsString())
            return wpie_http_parse_response;

        file_url_ = iter_static_url->value.GetString();
    }

    return 0;
}

int32_t send_file::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200 && http_code_ != 206 && http_code_ != 201)
        return wpie_http_error;

    return 0;
}


std::string send_file::get_file_url() const
{
    return file_url_;
}