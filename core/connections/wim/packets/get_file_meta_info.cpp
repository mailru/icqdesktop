#include "stdafx.h"
#include "get_file_meta_info.h"
#include "../../../http_request.h"
#include "../loader/web_file_info.h"
#include "../../../tools/system.h"


using namespace core;
using namespace wim;


get_file_meta_info::get_file_meta_info(
    const wim_packet_params& _params,
    const web_file_info& _info)
    :	wim_packet(_params),
    info_(new web_file_info(_info))
{
}


get_file_meta_info::~get_file_meta_info()
{
}


int32_t get_file_meta_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << "https://files.icq.com/getinfo?file_id=" << info_->get_file_id();
    ss_url << "&r=" <<  core::tools::system::generate_guid();

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

int32_t get_file_meta_info::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
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

    auto iter_status = doc.FindMember("status");
    if (iter_status == doc.MemberEnd() || !iter_status->value.IsInt())
        return wpie_http_parse_response;

    status_code_ = iter_status->value.GetInt();

    if (status_code_ == 200)
    {
        auto iter_flist = doc.FindMember("file_list");
        if (iter_flist == doc.MemberEnd() || !iter_flist->value.IsArray())
            return wpie_error_parse_response;

        if (iter_flist->value.Empty())
            return wpie_error_parse_response;

        auto iter_flist0 = iter_flist->value.Begin();

        auto iter_is_previewable = iter_flist0->FindMember("is_previewable");
        if (iter_is_previewable != iter_flist0->MemberEnd() && iter_is_previewable->value.IsInt())
            info_->set_is_previewable(!!iter_is_previewable->value.GetInt());

        auto iter_dlink = iter_flist0->FindMember("dlink");
        if (iter_dlink != iter_flist0->MemberEnd() && iter_dlink->value.IsString())
            info_->set_file_dlink(iter_dlink->value.GetString());

        auto iter_mime = iter_flist0->FindMember("mime");
        if (iter_mime != iter_flist0->MemberEnd() && iter_mime->value.IsString())
            info_->set_mime(iter_mime->value.GetString());

        auto iter_md5 = iter_flist0->FindMember("md5");
        if (iter_md5 != iter_flist0->MemberEnd() && iter_md5->value.IsString())
            info_->set_md5(iter_md5->value.GetString());

        auto iter_file_size = iter_flist0->FindMember("filesize");
        if (iter_file_size != iter_flist0->MemberEnd() && iter_file_size->value.IsString())
            info_->set_file_size(strtol(iter_file_size->value.GetString(), 0, 0));

        auto iter_file_name = iter_flist0->FindMember("filename");
        if (iter_file_name != iter_flist0->MemberEnd() && iter_file_name->value.IsString())
            info_->set_file_name_short(iter_file_name->value.GetString());

        auto iter_iphone = iter_flist0->FindMember("iphone");
        if (iter_iphone != iter_flist0->MemberEnd() && iter_iphone->value.IsString())
            info_->set_file_preview_2k(iter_iphone->value.GetString());

        auto iter_file_preview800 = iter_flist0->FindMember("static800");
        if (iter_file_preview800 != iter_flist0->MemberEnd() && iter_file_preview800->value.IsString())
            info_->set_file_preview(iter_file_preview800->value.GetString());
        else
        {
            auto iter_file_preview600 = iter_flist0->FindMember("static600");
            if (iter_file_preview600 != iter_flist0->MemberEnd() && iter_file_preview600->value.IsString())
                info_->set_file_preview(iter_file_preview600->value.GetString());
            else
            {
                auto iter_file_preview194 = iter_flist0->FindMember("static194");
                if (iter_file_preview194 != iter_flist0->MemberEnd() && iter_file_preview194->value.IsString())
                    info_->set_file_preview(iter_file_preview194->value.GetString());
            }
        }
    }

    return 0;
}

int32_t get_file_meta_info::on_http_client_error()
{
    if (http_code_ == 404)
        return wpie_error_metainfo_not_found;

    return wpie_client_http_error;
}

const web_file_info& get_file_meta_info::get_info() const
{
    return *info_;
}
