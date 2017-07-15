#ifndef __WIM_SEARCH_CONTACTS_H_
#define __WIM_SEARCH_CONTACTS_H_

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
    namespace profile
    {
        class info;
        typedef std::vector<std::shared_ptr<info>> profiles_list;
    }

    namespace wim
    {
        class get_profile : public wim_packet
        {
            std::string             	aimId_;
            profile::profiles_list		search_result_;


            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;


        public:

            get_profile(
                const wim_packet_params& _params,
                const std::string& _aimId);

            virtual ~get_profile();

            const profile::profiles_list& get_result() const;
        };

    }

}


#endif// __WIM_SEARCH_CONTACTS_H_