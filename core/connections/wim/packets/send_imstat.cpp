#include "stdafx.h"
#include "send_imstat.h"

#include "../../../http_request.h"
#include "../../../core.h"
#include "../../../utils.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;

const std::string im_stat_url = "https://imstat.mail.ru/imstat";


send_imstat::send_imstat(const wim_packet_params& params, const std::string& _data)
    :	wim_packet(params),
    data_(_data)
{
}


send_imstat::~send_imstat()
{
}


bool send_imstat::support_async_execution() const
{
    return true;
}


int32_t send_imstat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(im_stat_url);
    _request->push_post_parameter("login", get_params().aimid_);
    _request->push_post_parameter("data", data_);
    _request->push_post_parameter("r", core::tools::system::generate_guid());

    return 0;
}


int32_t send_imstat::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

void send_imstat::execute_request_async(std::shared_ptr<core::http_request_simple> _request, handler_t _handler)
{
    std::weak_ptr<wim_packet> wr_this(shared_from_this());

    _request->post_async([_request, _handler, wr_this](bool _success)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto ptr = static_cast<send_imstat*>(ptr_this.get());

        ptr->http_code_ = (uint32_t)_request->get_response_code();

        if (!_handler)
            return;

        if (ptr->http_code_ != 200)
        {
            _handler(wpie_http_error);
        }
        else
        {
            _handler(0);
        }
    });
}

int32_t send_imstat::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    return 0;
}