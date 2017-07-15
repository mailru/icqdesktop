#ifndef __LOAD_FILE_H_
#define __LOAD_FILE_H_

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

        class load_file : public wim_packet
        {
            std::unique_ptr<web_file_info>				info_;
            std::shared_ptr<core::tools::binary_stream>	response_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;							
        public:

            load_file(const wim_packet_params& _params, const web_file_info& _info);
            virtual ~load_file();

            const web_file_info& get_info() const;
            const core::tools::binary_stream& get_response() const;
        };
    }
}

#endif //__LOAD_FILE_H_