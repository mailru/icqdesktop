#include "stdafx.h"
#include "load_file.h"

#include "../../../http_request.h"
#include "../../../core.h"

#include "../loader/web_file_info.h"

using namespace core;
using namespace wim;

const int32_t max_block_size		= 1024*1024;

load_file::load_file(const wim_packet_params& _params, const web_file_info& _info)
    :	wim_packet(_params), 
    info_(new web_file_info(_info))
{
}


load_file::~load_file()
{
}


int32_t load_file::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_need_log(params_.full_log_);
    _request->set_url(info_->get_file_dlink());
    _request->set_keep_alive();

    int64_t tail = info_->get_file_size() - info_->get_bytes_transfer();
    int64_t read_size = max_block_size;

    if (tail <= 0)
    {
        assert(false);
        return wpie_error_internal_logic;
    }

    if (tail <= max_block_size)
        read_size = tail;

    _request->set_range(info_->get_bytes_transfer(), info_->get_bytes_transfer() + read_size);

    return 0;
}

int32_t load_file::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->get())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200 && http_code_ != 206 && http_code_ != 201)
        return wpie_http_error;

    return 0;
}


int32_t load_file::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    response_ = _response;

    info_->set_bytes_transfer(info_->get_bytes_transfer()+ _response->available());

    return 0;
}

const web_file_info& load_file::get_info() const
{
    return *info_;
}

const core::tools::binary_stream& load_file::get_response() const
{
    return *response_;
}