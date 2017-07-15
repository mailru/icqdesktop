#include "stdafx.h"
#include "get_user_snaps_patch.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


get_user_snaps_patch::get_user_snaps_patch(const wim_packet_params& _params, const std::string& _aimId, int64_t _patchVersion)
    : robusto_packet(_params)
    , aimId_(_aimId)
    , patchVersion_(_patchVersion)
{
}


get_user_snaps_patch::~get_user_snaps_patch()
{

}

int32_t get_user_snaps_patch::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getUserSnapsPatch", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimId_, a);
    node_params.AddMember("patchVersion", patchVersion_, a);
    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    return 0;
}

int32_t get_user_snaps_patch::parse_results(const rapidjson::Value& _node_results)
{
    result_.unserialize(_node_results);

    return 0;
}