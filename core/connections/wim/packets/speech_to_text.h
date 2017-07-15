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
        class speech_to_text : public wim_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            std::string Url_;
            std::string Locale_;
            std::string Text_;
            int32_t Comeback_;

        public:

            speech_to_text(
                const wim_packet_params& _params,
                const std::string& _url,
                const std::string& _locale);

            virtual ~speech_to_text();

            std::string get_text() const;
            int32_t get_comeback() const;
        };
    }
}

#endif //__GET_FILE_META_INFO_H_