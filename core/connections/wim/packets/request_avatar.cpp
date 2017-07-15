#include "stdafx.h"

#include "request_avatar.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;

request_avatar::request_avatar(const wim_packet_params& params, const std::string& _contact, const std::string& _avatar_type, time_t _write_time)
    :	wim_packet(params), avatar_type_(_avatar_type), contact_(_contact), write_time_(_write_time)
{
}


request_avatar::~request_avatar()
{
}

int32_t request_avatar::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;
    ss_url << "https://api.icq.net/expressions/get?" <<
        "t=" << contact_ <<
        "&f=native" <<
        "&r=" << core::tools::system::generate_guid() <<
        "&type=" << avatar_type_;

    if (write_time_ != 0)
        _request->set_modified_time_condition(write_time_ - params_.time_offset_);

    _request->set_need_log(params_.full_log_);
    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    _request->set_priority(highest_priority + increase_priority);

    return 0;
}

int32_t core::wim::request_avatar::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    data_ = _response;

    return 0;
}

std::shared_ptr<core::tools::binary_stream> core::wim::request_avatar::get_data()
{
    return data_;
}
