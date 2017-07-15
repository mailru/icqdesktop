#include "stdafx.h"

#include "file_sharing_meta.h"

core::wim::file_sharing_meta::file_sharing_meta(const std::string& _url)
    : file_url_(_url)
{
}

core::wim::file_sharing_meta_uptr core::wim::file_sharing_meta::parse_json(InOut char* _json, const std::string& _uri)
{
    rapidjson::Document doc;

    if (doc.ParseInsitu(_json).HasParseError())
        return file_sharing_meta_uptr();

    auto iter_status = doc.FindMember("status");
    if (iter_status == doc.MemberEnd() || !iter_status->value.IsInt())
        return file_sharing_meta_uptr();

    const auto status_code = iter_status->value.GetInt();

    if (status_code != 200)
        return file_sharing_meta_uptr();

    auto iter_flist = doc.FindMember("file_list");
    if (iter_flist == doc.MemberEnd() || !iter_flist->value.IsArray())
        return file_sharing_meta_uptr();

    if (iter_flist->value.Empty())
        return file_sharing_meta_uptr();

    auto meta = file_sharing_meta_uptr(new file_sharing_meta(_uri));

    auto iter_flist0 = iter_flist->value.Begin();

    auto iter_dlink = iter_flist0->FindMember("dlink");
    if (iter_dlink != iter_flist0->MemberEnd())
        meta->file_download_url_ = iter_dlink->value.GetString();

    auto iter_mime = iter_flist0->FindMember("mime");
    if (iter_mime != iter_flist0->MemberEnd())
        meta->mime_ = iter_mime->value.GetString();

    auto iter_file_size = iter_flist0->FindMember("filesize");
    if (iter_file_size != iter_flist0->MemberEnd())
        meta->file_size_ = strtol(iter_file_size->value.GetString(), 0, 0);

    auto iter_file_name = iter_flist0->FindMember("filename");
    if (iter_file_name != iter_flist0->MemberEnd())
        meta->file_name_short_ = iter_file_name->value.GetString();

    auto iter_iphone = iter_flist0->FindMember("iphone");
    if (iter_iphone != iter_flist0->MemberEnd())
        meta->file_mini_preview_url_ = iter_iphone->value.GetString();

    auto iter_file_preview800 = iter_flist0->FindMember("static800");
    if (iter_file_preview800 != iter_flist0->MemberEnd())
    {
        meta->file_full_preview_url_ = iter_file_preview800->value.GetString();
    }
    else
    {
        auto iter_file_preview600 = iter_flist0->FindMember("static600");
        if (iter_file_preview600 != iter_flist0->MemberEnd())
            meta->file_full_preview_url_ = iter_file_preview600->value.GetString();
        else
        {
            auto iter_file_preview194 = iter_flist0->FindMember("static194");
            if (iter_file_preview194 != iter_flist0->MemberEnd())
                meta->file_full_preview_url_ = iter_file_preview194->value.GetString();
        }
    }

    return meta;
}
