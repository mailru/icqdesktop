#ifndef __WIM_SET_TIMEZONE_H_
#define __WIM_SET_TIMEZONE_H_

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
        class set_timezone : public wim_packet
        {
            bool support_async_execution() const override;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

        public:

            set_timezone(const wim_packet_params& _params);
            virtual ~set_timezone();
        };

    }

}


#endif// __WIM_SET_TIMEZONE_H_