#ifndef __GET_GATEWAY_H_
#define __GET_GATEWAY_H_

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
        class get_gateway : public wim_packet
        {
            std::string		file_name_;
            int64_t			file_size_;
            std::string		host_;
            std::string		url_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t on_http_client_error() override;

        public:

            get_gateway(const wim_packet_params& _params, const std::string& _file_name, int64_t _file_size);
            virtual ~get_gateway();

            std::string get_host() const;
            std::string get_url() const;
        };
    }
}

#endif //__GET_GATEWAY_H_