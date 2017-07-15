#pragma once

#include "../robusto_packet.h"
#include "../snap_info.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class get_user_snaps: public robusto_packet
        {

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::string aimId_;

        public:
            user_snaps_info result_;

            get_user_snaps(const wim_packet_params& _params, const std::string& _aimId);

            virtual ~get_user_snaps();
        };

    }

}