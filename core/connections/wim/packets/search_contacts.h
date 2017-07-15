#pragma once

#include "../robusto_packet.h"
#include "../search_contacts_response.h"

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
        class search_contacts: public robusto_packet
        {
            struct params
            {
                std::string tag_;
                std::string keyword_;
                std::string phonenumber_;
            }
            params_;
            
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t on_response_error_code() override;
            
        public:
            search_contacts_response response_;
            
            search_contacts(const wim_packet_params& _packet_params, const std::string& keyword, const std::string& phonenumber, const std::string& tag);
            virtual ~search_contacts();
        };
        
    }
    
}
