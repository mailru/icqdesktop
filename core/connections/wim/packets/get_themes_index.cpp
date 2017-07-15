#include "stdafx.h"
#include "get_themes_index.h"
#include "../../../tools/strings.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../tools/binary_stream.h"
#include "../../../core.h"
#include "../../../http_request.h"

using namespace core;
using namespace wim;

get_themes_index::get_themes_index(const wim_packet_params& _params, const std::string &etag): wim_packet(_params), etag_(etag)
{
}

get_themes_index::~get_themes_index()
{
}

bool get_themes_index::support_async_execution() const
{
    return true;
}

int32_t get_themes_index::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_etag(etag_.c_str());
    _request->set_url("https://icq.com/wallpaperlist/windows");
    return 0;
}

int32_t get_themes_index::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
    {
        return wpie_http_empty_response;
    }
    response_ = _response;
    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_themes_index::get_response()
{
    return response_;
}

const std::string get_themes_index::get_header_etag() const
{
    std::string etag = "";
    auto header = header_str();
    std::transform(header.begin(), header.end(), header.begin(), ::tolower);
    if (header.length())
    {
        const std::string etag_prefix = "etag: \"";
        const std::string etag_postfix = "\"";
        
        auto etag_pos_begin = header.find(etag_prefix);
        if (etag_pos_begin < header.length())
        {
            auto etag_pos_end = header.find(etag_postfix, etag_pos_begin + etag_prefix.length());
            if (etag_pos_end < header.length())
            {
                etag.assign(header.begin() + etag_pos_begin + etag_prefix.length(), header.begin() + etag_pos_end);
            }
        }
    }
    return etag;
}

