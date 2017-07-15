#include "stdafx.h"
#include "get_stickers_index.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

get_stickers_index::get_stickers_index(const wim_packet_params& _params, const std::string& _md5)
    :	wim_packet(_params), md5_(_md5)
{
}


get_stickers_index::~get_stickers_index()
{
}

bool get_stickers_index::support_async_execution() const
{
    return true;
}

int32_t get_stickers_index::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    std::stringstream ss_ts;
    ss_ts << (int64_t) ts;

    //  #ifdef _DEBUG
    //  	const std::string host = "https://store.icq.com/openstore/contentlist_invalid_url_for_test";
    //  #else
    const std::string host = "https://store.icq.com/openstore/contentlist";
    /*#endif _DEBUG*/

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = ss_ts.str();
    params["r"] = core::tools::system::generate_guid();

    if (!md5_.empty())
    {
        params["md5"] = md5_;
    }

    params["client"] = "icq";

    params["lang"] = g_core->get_locale();
    
    const auto sha256 = escape_symbols(get_url_sign(host, params, params_, false));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url;
    ss_url << host << "?" << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    _request->set_priority(high_priority);

    return 0;
}

int32_t get_stickers_index::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_stickers_index::get_response()
{
    return response_;
}
