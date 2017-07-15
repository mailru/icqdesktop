#include "stdafx.h"
#include "get_profile.h"

#include "../../../http_request.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../contact_profile.h"

using namespace core;
using namespace wim;

get_profile::get_profile(
    const wim_packet_params& _params,
    const std::string& _aimId)
    : wim_packet(_params),
    aimId_(_aimId)
{
}


get_profile::~get_profile()
{

}

int32_t get_profile::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << c_wim_host << "presence/get" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&mdir=1" <<
        "&t=" << escape_symbols(aimId_);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

const profile::profiles_list& get_profile::get_result() const
{
    return search_result_;
}

int32_t get_profile::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_users = _data.FindMember("users");
    if (iter_users == _data.MemberEnd() || !iter_users->value.IsArray())
        return wpie_http_parse_response;

    for (auto iter = iter_users->value.Begin(); iter != iter_users->value.End(); ++iter)
    {
        auto iter_aimid = iter->FindMember("aimId");
        if (iter_aimid == iter->MemberEnd() || !iter_aimid->value.IsString())
            return wpie_http_parse_response;

        auto contact_profile = std::make_shared<profile::info>(iter_aimid->value.GetString());
        if (!contact_profile->unserialize(*iter))
        {
            assert(false);
            return wpie_http_parse_response;
        }

        search_result_.push_back(contact_profile);
    }

    return 0;
}
