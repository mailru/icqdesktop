#ifndef __GET_FILE_META_INFO_H_
#define __GET_FILE_META_INFO_H_

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
        class web_file_info;

        class get_file_meta_info : public wim_packet
        {
            std::unique_ptr<web_file_info>	info_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

        protected:
            virtual int32_t on_http_client_error() override;

        public:

            get_file_meta_info(
                const wim_packet_params& _params,
                const web_file_info& _info);

            virtual ~get_file_meta_info();

            const web_file_info& get_info() const;
        };
    }
}

#endif //__GET_FILE_META_INFO_H_