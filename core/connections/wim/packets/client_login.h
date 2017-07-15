#ifndef __WIM_CLIENT_LOGIN_H_
#define __WIM_CLIENT_LOGIN_H_

#pragma once

#include "../wim_packet.h"

#define WIM_APP_VER "5000"

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
        class client_login : public wim_packet
        {
            std::string login_;
            std::string password_;
            
            std::string session_secret_;
            std::string session_key_;
            std::string a_token_;
            uint32_t expired_in_;
            uint32_t host_time_;
            int64_t time_offset_;

            std::string product_guid_8x_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

        public:

            const std::string get_session_secret() const { return session_secret_; }
            const std::string get_session_key() const { return session_key_; }
            const std::string get_a_token() const { return a_token_; }
            const uint32_t get_expired_in() const { return expired_in_; }
            const uint32_t get_host_time() const { return host_time_; }
            const int64_t	get_time_offset() const { return time_offset_; }

            void set_product_guid_8x(const std::string& _guid);

        public:

            client_login(
                const wim_packet_params& params,
                const std::string& login,
                const std::string& password);

            virtual ~client_login();
        };
    }
}


#endif //__WIM_CLIENT_LOGIN_H_