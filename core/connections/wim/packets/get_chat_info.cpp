#include "stdafx.h"
#include "get_chat_info.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


get_chat_info::get_chat_info(const wim_packet_params& _params, const get_chat_info_params& _chat_params)
    :   robusto_packet(_params),
        params_(_chat_params)
{
}


get_chat_info::~get_chat_info()
{

}

int32_t get_chat_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getChatInfo", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (!params_.aimid_.empty())
    {
        node_params.AddMember("sn", params_.aimid_, a);
    }
    else if (!params_.stamp_.empty())
    {
        node_params.AddMember("stamp", params_.stamp_, a);
    }
    else
    {
        assert(false);
    }

    if (params_.members_limit_ != 0)
        node_params.AddMember("memberLimit", params_.members_limit_, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    return 0;
}

int32_t get_chat_info::parse_results(const rapidjson::Value& _node_results)
{
    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;

    return 0;
}

int32_t get_chat_info::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}