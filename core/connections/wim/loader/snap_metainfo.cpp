#include "stdafx.h"

#include "../../../../corelib/collection_helper.h"

#include "snap_metainfo.h"

CORE_WIM_SNAPS_NS_BEGIN

snap_metainfo::snap_metainfo(
    const uint64_t _id,
    const std::string &_mini_preview_uri,
    const std::string &_full_preview_uri,
    const std::string &_iphone_preview_uri,
    const int64_t _expire_utc,
    const std::string _author_uin,
    const std::string _author_name,
    const std::string& _ttl_id)
    : id_(_id)
    , mini_preview_uri_(_mini_preview_uri)
    , full_preview_uri_(_full_preview_uri)
    , iphone_preview_uri_(_iphone_preview_uri)
    , expire_utc_(_expire_utc)
    , author_uin_(_author_uin)
    , author_name_(_author_name)
    , ttl_id_(_ttl_id)
{
    assert(expire_utc_ >= 0);
    assert(!author_uin_.empty());
    assert(!author_name_.empty());
}

snap_metainfo::~snap_metainfo()
{
}

const std::string& snap_metainfo::get_author_name() const
{
    assert(!author_name_.empty());

    return author_name_;
}

const std::string& snap_metainfo::get_author_uin() const
{
    assert(!author_uin_.empty());

    return author_uin_;
}

uint64_t snap_metainfo::get_id() const
{
    assert(id_ > 0);
    return id_;
}

const std::string& snap_metainfo::get_full_preview_uri() const
{
    return full_preview_uri_;
}

const std::string& snap_metainfo::get_mini_preview_uri() const
{
    return mini_preview_uri_;
}

const std::string& snap_metainfo::get_iphone_preview_uri() const
{
    return iphone_preview_uri_;
}

int64_t snap_metainfo::get_expire_utc() const
{
    assert(expire_utc_ >= 0);
    return expire_utc_;
}

void snap_metainfo::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);

    coll.set<uint64_t>("snap_id", id_);
    coll.set<int64_t>("expire_utc", expire_utc_);
    coll.set<std::string>("author_uin", author_uin_);
    coll.set<std::string>("author_name", author_name_);
    coll.set<std::string>("mini_preview_uri", mini_preview_uri_);
    coll.set<std::string>("full_preview_uri", full_preview_uri_);
    coll.set<std::string>("iphone_preview_uri", iphone_preview_uri_);
    coll.set<std::string>("ttl_id", ttl_id_);
}

str_2_str_map format_get_metainfo_params(const std::string &_ttl_id)
{
    assert(!_ttl_id.empty());

    str_2_str_map result;

    result.emplace("ttl_id", _ttl_id);

    return result;
}

snap_metainfo_uptr parse_json(InOut char *_json, const std::string& _ttl_id)
{
    assert(_json);

    rapidjson::Document doc;
    if (doc.ParseInsitu(_json).HasParseError())
    {
        return nullptr;
    }

    uint64_t snap_id = 0;
    auto iter_snap_id = doc.FindMember("snap_id");
    if ((iter_snap_id != doc.MemberEnd()) && iter_snap_id->value.IsString())
    {
        try
        {
            snap_id = std::stoull(iter_snap_id->value.GetString());
        }
        catch (std::invalid_argument&)
        {
            assert(!"invalid snap id format");
            return nullptr;
        }
        catch (std::out_of_range&)
        {
            assert(!"invalid snap id value");
        }

        if (snap_id == 0)
        {
            assert(!"snap id cannot be zero");
            return nullptr;
        }
    }

    auto iter_author_uin = doc.FindMember("author_uin");
    if ((iter_author_uin == doc.MemberEnd()) ||
        !iter_author_uin->value.IsString())
    {
        return nullptr;
    }

    auto iter_author_name = doc.FindMember("author_name");
    if ((iter_author_name == doc.MemberEnd()) ||
        !iter_author_name->value.IsString())
    {
        return nullptr;
    }

    auto iter_expire = doc.FindMember("expire");
    if ((iter_expire == doc.MemberEnd()) ||
        !iter_expire->value.IsString())
    {
        return nullptr;
    }

    auto iter_file_list = doc.FindMember("file_list");
    if ((iter_file_list == doc.MemberEnd()) ||
        !iter_file_list->value.IsArray() ||
        iter_file_list->value.Empty())
    {
        return nullptr;
    }

    const auto author_uin = iter_author_uin->value.GetString();

    const auto author_name = iter_author_name->value.GetString();

    const auto &node_file = iter_file_list->value[0];

    std::string mini_preview_uri;
    auto iter_static = node_file.FindMember("static");
    if (iter_static != node_file.MemberEnd() && iter_static->value.IsString())
    {
        mini_preview_uri = iter_static->value.GetString();
    }

    std::string full_preview_uri;
    auto iter_static600 = node_file.FindMember("static600");
    if (iter_static600 != node_file.MemberEnd() && iter_static600->value.IsString())
    {
        full_preview_uri = iter_static600->value.GetString();
    }

    std::string iphone_preview_uri;
    auto iter_iphone = node_file.FindMember("mdpi");
    if (iter_iphone != node_file.MemberEnd() && iter_iphone->value.IsString())
    {
        iphone_preview_uri = iter_iphone->value.GetString();
    }

    int64_t expire = 0;
    try
    {
        expire = std::stoll(iter_expire->value.GetString());
    }
    catch (std::invalid_argument&)
    {
        assert(!"invalid expire format");
        return nullptr;
    }
    catch (std::out_of_range&)
    {
        assert(!"invalid expire value");
    }

    if (expire <= 0)
    {
        assert(!"snap id cannot be zero");
        return nullptr;
    }

    snap_metainfo_uptr info(new snap_metainfo(snap_id, mini_preview_uri, full_preview_uri, iphone_preview_uri, expire, author_uin, author_name, _ttl_id));

    return info;
}

namespace uri
{
    const std::string& get_metainfo()
    {
        static const std::string URI = "https://files.icq.com/getsnapinfo";
        return URI;
    }
}


CORE_WIM_SNAPS_NS_END
