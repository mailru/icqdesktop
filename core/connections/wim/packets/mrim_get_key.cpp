#include "stdafx.h"
#include "mrim_get_key.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

mrim_get_key::mrim_get_key(
    const wim_packet_params& _params,
    const std::string& _email)
    :	wim_packet(_params),
    email_(_email)
{
}

mrim_get_key::~mrim_get_key()
{
}

int32_t mrim_get_key::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << c_wim_host << "mrim/getKey" <<
        "?aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&f=json" <<
        "&email="<< escape_symbols(email_);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t mrim_get_key::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(size)).HasParseError())
        return wpie_error_parse_response;

    auto iter_response = doc.FindMember("response");
    if (iter_response == doc.MemberEnd())
        return wpie_http_parse_response;

    auto iter_status = iter_response->value.FindMember("statusCode");
    if (iter_status == iter_response->value.MemberEnd() || !iter_status->value.IsUint())
        return wpie_http_parse_response;

    status_code_ = iter_status->value.GetUint();
    if (status_code_ == 20000)
    {
        auto iter_data = iter_response->value.FindMember("data");
        if (iter_data == iter_response->value.MemberEnd() || !iter_data->value.IsObject())
            return wpie_http_parse_response;

        auto iter_key = iter_data->value.FindMember("mrimkey");
        if (iter_key == iter_data->value.MemberEnd() || !iter_key->value.IsString())
            return wpie_http_parse_response;

        mrim_key_ = iter_key->value.GetString();
    }

    return 0;
}
