#pragma once

#include "../../../namespaces.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

class link_meta;

typedef std::unique_ptr<link_meta> link_meta_uptr;

CORE_WIM_PREVIEW_PROXY_NS_END

CORE_WIM_SNAPS_NS_BEGIN

class snap_metainfo;

typedef std::unique_ptr<snap_metainfo> snap_metainfo_uptr;

CORE_WIM_SNAPS_NS_END

CORE_WIM_NS_BEGIN

enum class path_type
{
    min,

    link_preview,
    link_meta,
    file,
    snap_meta,

    max
};

struct wim_packet_params;

std::string get_filename_in_cache(const std::string& _uri);

std::wstring get_path_in_cache(const std::wstring& _cache_dir, const std::string& _uri, const path_type _path_type);

const std::string& get_path_suffix(const path_type _type);

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring &_path, const std::string &_url);

snaps::snap_metainfo_uptr load_snap_meta_from_file(const std::wstring &_path, const std::string &_ttl_id);

std::string sign_loader_uri(const std::string &_host, const wim_packet_params &_params, const str_2_str_map &_extra = str_2_str_map());

CORE_WIM_NS_END