#pragma once

#include "../wim_packet.h"

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
        class mute_buddy : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            bool mute_;

        public:

            mute_buddy(
                const wim_packet_params& _params,
                const std::string& _aimid,
                bool mute);

            virtual ~mute_buddy();
        };

    }

}