#ifndef __SEND_FILE_H_
#define __SEND_FILE_H_

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
        struct send_file_params
        {
            int64_t			size_already_sent_;
            int64_t			current_chunk_size_;
            int64_t			full_data_size_;
            int64_t			session_id_;

            std::string		file_name_;
            char*			data_;

            send_file_params();
        };

        class send_file : public wim_packet
        {
            std::string					host_;
            std::string					url_;

            const send_file_params&		chunk_;

            std::string					file_url_;


            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

        public:

            send_file(
                const wim_packet_params& _params, 
                const send_file_params& _chunk, 
                const std::string& _host, 
                const std::string& _url);

            virtual ~send_file();

            std::string get_file_url() const; 
        };
    }
}

#endif //__SEND_FILE_H_