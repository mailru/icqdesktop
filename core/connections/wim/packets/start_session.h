#ifndef __STARTSESSION_H_
#define __STARTSESSION_H_

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
        class start_session : public wim_packet
        {
            int32_t init_request_full_start_session(std::shared_ptr<core::http_request_simple> _request);
            int32_t init_request_short_start_session(std::shared_ptr<core::http_request_simple> _request);

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual void parse_response_data_on_error(const rapidjson::Value& _data) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t on_response_error_code() override;

            std::string uniq_device_id_;
            std::string locale_;
            bool is_ping_;

            bool need_relogin_;
            bool correct_ts_;

            std::string fetch_url_;
            std::string aimsid_;
            std::string aimid_;

            time_t timeout_;
            std::function<bool(int32_t)> wait_function_;

            int64_t ts_;

        public:

            bool need_relogin() const { return need_relogin_; }
            bool need_correct_ts() const { return correct_ts_; }

            const std::string get_fetch_url() const { return fetch_url_; }
            const std::string get_aimsid() const { return aimsid_; }
            const time_t get_ts() const { return ts_; }
            const std::string get_aimid() const { return aimid_; }

            start_session(const wim_packet_params& params, bool _is_ping, const std::string& _uniq_device_id, const std::string& _locale, time_t _timeout, std::function<bool(int32_t)> _wait_function);
            virtual ~start_session();
        };
    }
}

#endif //__STARTSESSION_H_