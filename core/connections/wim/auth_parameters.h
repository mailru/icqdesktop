#ifndef __AUTH_PARAMETERS_H_
#define __AUTH_PARAMETERS_H_

#pragma once


namespace core
{
    namespace wim
    {
        typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> timepoint;

        struct auth_parameters
        {
            std::string aimid_;
            std::string a_token_;
            std::string session_key_;
            std::string dev_id_;
            std::string aimsid_;
            std::string locale_;
            std::string password_md5_;
            std::string product_guid_8x_;
            std::string agent_token_;

            time_t exipired_in_;
            time_t time_offset_;
                        
            std::string robusto_token_;
            int32_t robusto_client_id_;
            std::string version_;

            bool serializable_;

            std::string login_;
            std::string fetch_url_;

            auth_parameters();
            bool is_valid_agent_token() const;
            bool is_valid_token() const;
            bool is_valid_md5() const;
            bool is_valid() const;
            bool is_robusto_valid() const;

            void reset_robusto();
            void clear();

            void serialize(core::tools::binary_stream& _stream) const;
            bool unserialize(core::tools::binary_stream& _stream);

            bool unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        };

        struct fetch_parameters
        {
            std::string fetch_url_;
            timepoint next_fetch_time_;
            time_t last_successful_fetch_;

            fetch_parameters();
            bool is_valid() const;
            void serialize(core::tools::binary_stream& _stream) const;
            bool unserialize(core::tools::binary_stream& _stream);
        };

    }
}




#endif //__AUTH_PARAMETERS_H_
