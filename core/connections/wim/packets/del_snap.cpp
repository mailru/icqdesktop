#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "del_snap.h"

CORE_WIM_NS_BEGIN

del_snap::del_snap(
    const wim_packet_params& _params,
    const uint64_t _snap_id)
    : robusto_packet(_params)
    , snap_id_(_snap_id)
{
}

del_snap::~del_snap()
{
}

int32_t del_snap::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);

    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", "delSnaps", a);

    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_ids(rapidjson::Type::kArrayType);
    node_ids.PushBack(snap_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("snapIds", node_ids, a);

    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), std::string());

    return 0;
}

int32_t del_snap::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

CORE_WIM_NS_END