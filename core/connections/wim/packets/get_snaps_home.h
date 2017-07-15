#pragma once

#include "../robusto_packet.h"
#include "../snaps_storage.h"

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
        class get_snaps_home: public robusto_packet
        {

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

        public:
            std::shared_ptr<snaps_storage> result_;

            get_snaps_home(const wim_packet_params& _params);

            virtual ~get_snaps_home();
        };

    }

}