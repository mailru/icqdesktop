#include "stdafx.h"
#include "get_snaps_home.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


get_snaps_home::get_snaps_home(const wim_packet_params& _params)
    :	robusto_packet(_params)
    ,   result_(new snaps_storage())
{
}


get_snaps_home::~get_snaps_home()
{

}

int32_t get_snaps_home::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getSnapHome", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    return 0;
}

int32_t get_snaps_home::parse_results(const rapidjson::Value& _node_results)
{
    return result_->unserialize(_node_results);
}