#pragma once

#include "../../../namespaces.h"

CORE_NS_BEGIN

struct icollection;

CORE_NS_END

CORE_WIM_SNAPS_NS_BEGIN

class snap_metainfo
{
public:
    snap_metainfo(
        const uint64_t _id,
        const std::string &_mini_preview_uri,
        const std::string &_full_preview_uri,
        const std::string &_iphone_preview_uri,
        const int64_t _expire_utc,
        const std::string _author_uin,
        const std::string _author_name,
        const std::string& _ttl_id);

    ~snap_metainfo();

    const std::string& get_author_name() const;

    const std::string& get_author_uin() const;

    uint64_t get_id() const;

    const std::string& get_full_preview_uri() const;

    const std::string& get_mini_preview_uri() const;

    const std::string& get_iphone_preview_uri() const;

    int64_t get_expire_utc() const;

    void serialize(icollection* _collection) const;

private:
    const uint64_t id_;

    const std::string full_preview_uri_;

    const std::string mini_preview_uri_;

    const std::string iphone_preview_uri_;

    const int64_t expire_utc_;

    const std::string author_uin_;

    const std::string author_name_;

    const std::string ttl_id_;
};

typedef std::unique_ptr<snap_metainfo> snap_metainfo_uptr;

str_2_str_map format_get_metainfo_params(const std::string &_ttl_id);

snap_metainfo_uptr parse_json(InOut char *_json, const std::string& _ttl_id);

namespace uri
{
    const std::string& get_metainfo();
}

CORE_WIM_SNAPS_NS_END