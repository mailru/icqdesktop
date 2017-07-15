#include "stdafx.h"
#include "send_message_typing.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

send_message_typing::send_message_typing(const wim_packet_params& _params, const std::string& _contact, const core::typing_status& _status)
    :
    wim_packet(_params),
    contact_(_contact),
    status_(_status)
{
}

send_message_typing::~send_message_typing()
{
}

bool send_message_typing::support_async_execution() const
{
    return true;
}

int32_t send_message_typing::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string status = "none";
    switch (status_)
    {
        case core::typing_status::typing:
            status = "typing";
            break;
        case core::typing_status::typed:
            status = "typed";
            break;
        default:
            break;
    }
    
    std::stringstream ss_url;
    ss_url << c_wim_host << "im/setTyping" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) << 
        "&t=" << escape_symbols(contact_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&typingStatus=" << status;
    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    return 0;
}

int32_t send_message_typing::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
