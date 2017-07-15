#include "stdafx.h"
#include "set_timezone.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

set_timezone::set_timezone(const wim_packet_params& _params)
    : wim_packet(_params)
{
}

set_timezone::~set_timezone()
{
}

bool set_timezone::support_async_execution() const
{
    return true;
}

int32_t set_timezone::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    time_t server_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    time_t time_offset = boost::posix_time::to_time_t(boost::posix_time::second_clock::local_time()) - server_time;

    std::stringstream ss_url;

    ss_url << c_wim_host << "timezone/set" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&TimeZoneOffset=" << time_offset;

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t set_timezone::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
