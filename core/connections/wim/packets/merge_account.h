#ifndef __WIM_MERGE_ACCOUNT_H_
#define __WIM_MERGE_ACCOUNT_H_

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
        class merge_account : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t on_empty_data() override;

            wim_packet_params from_params_;

        public:
            
            merge_account(
                const wim_packet_params& _from_params, 
                const wim_packet_params& _to_params);

            virtual ~merge_account();
        };

    }

}


#endif// __WIM_MERGE_ACCOUNT_H_
