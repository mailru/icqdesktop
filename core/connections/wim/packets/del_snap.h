#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"

CORE_WIM_NS_BEGIN

class del_snap : public robusto_packet
{
public:
    del_snap(
        const wim_packet_params& _params,
        const uint64_t _snap_id);

    virtual ~del_snap();

private:
    virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

    virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

    const uint64_t snap_id_;
};

CORE_WIM_NS_END