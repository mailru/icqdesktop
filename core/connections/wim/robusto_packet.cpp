#include "stdafx.h"
#include "robusto_packet.h"

#include <sstream>

#include "../../http_request.h"

using namespace core;
using namespace wim;


robusto_packet::robusto_packet(const wim_packet_params& params)
    :	wim_packet(params)
{

}

robusto_packet::~robusto_packet()
{

}

void robusto_packet::set_robusto_params(const robusto_packet_params& _params)
{
    robusto_params_ = _params;
}

int32_t robusto_packet::parse_results(const rapidjson::Value& _node_results)
{
    return 0;
}


const std::string robusto_packet::get_req_id() const
{
    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss;
    ss << robusto_params_.robusto_req_id_ << "-" << (uint32_t) ts;

    return ss.str();
}


int32_t robusto_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    try
    {
#if defined(DEBUG)
        // because ParseInsitu spoils str somehow
        const std::string json_str_dbg(response_str());
#endif

        rapidjson::Document doc;
        if (doc.Parse(response_str().c_str()).HasParseError())
            return wpie_error_parse_response;

        auto iter_status = doc.FindMember("status");
        if (iter_status == doc.MemberEnd())
            return wpie_error_parse_response;

        auto iter_code = iter_status->value.FindMember("code");
        if (iter_code == iter_status->value.MemberEnd())
            return wpie_error_parse_response;

        status_code_ = iter_code->value.GetUint();

        if (status_code_ == 20000)
        {
            auto iter_result = doc.FindMember("results");
            if (iter_result != doc.MemberEnd())
                return parse_results(iter_result->value);
        }
        else
        {
            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}


int32_t robusto_packet::on_response_error_code()
{
    if ((40200 <= status_code_ && status_code_ < 40400) || status_code_ == 40000)
    {
        return wpie_error_robusto_token_invalid;
    }

    return wpie_error_message_unknown;
}

int32_t robusto_packet::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}
