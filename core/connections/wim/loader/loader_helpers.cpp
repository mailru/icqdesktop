#include "stdafx.h"

#include "../../../core.h"
#include "../../../tools/md5.h"

#include "../wim_packet.h"

#include "preview_proxy.h"
#include "snap_metainfo.h"

#include "loader_helpers.h"

CORE_WIM_NS_BEGIN

namespace
{
    std::string get_uri_ext(const std::string& _uri);
}

std::string get_filename_in_cache(const std::string& _uri)
{
    assert(!_uri.empty());

    return core::tools::md5(
        _uri.c_str(),
        (int32_t)_uri.length());
}

std::wstring get_path_in_cache(const std::wstring& _cache_dir, const std::string& _uri, const path_type _path_type)
{
    assert(!_cache_dir.empty());
    assert(!_uri.empty());
    assert(_path_type > path_type::min);
    assert(_path_type < path_type::max);

    boost::filesystem::wpath path(_cache_dir);

    auto filename = get_filename_in_cache(_uri);

    filename += get_path_suffix(_path_type);

    const auto append_extension = (
        (_path_type == path_type::file) ||
        (_path_type == path_type::link_preview));
    if (append_extension)
    {
        filename += get_uri_ext(_uri);
    }

    const auto append_js_extension = (
        (_path_type == path_type::link_meta) ||
        (_path_type == path_type::snap_meta));
    if (append_js_extension)
    {
        assert(!append_extension);
        filename += ".js";
    }

    path /= filename;

    return path.wstring();
}

const std::string& get_path_suffix(const path_type _type)
{
    assert(_type > path_type::min);
    assert(_type < path_type::max);

    static const std::string LINK_PREVIEW_SUFFIX = "lp";
    static const std::string LINK_META_SUFFIX = "lm";
    static const std::string SNAP_META_SUFFIX = "sm";

    switch(_type)
    {
    case path_type::link_preview:
        return LINK_PREVIEW_SUFFIX;

    case path_type::link_meta:
        return LINK_META_SUFFIX;

    case path_type::snap_meta:
        return SNAP_META_SUFFIX;

    case path_type::file:
        // no suffix for the gentlemen please
        break;

    default:
        assert(!"unexpected path type");
        break;
    }

    static const std::string no_suffix;
    return no_suffix;
}

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring &_path, const std::string &_uri)
{
    assert(!_path.empty());
    assert(!_uri.empty());

    tools::binary_stream json_file;
    if (!json_file.load_from_file(_path))
    {
        return nullptr;
    }

    const auto file_size = json_file.available();
    if (file_size == 0)
    {
        return nullptr;
    }

    const auto json_str = (char*)json_file.read(file_size);

    std::vector<char> json;
    json.reserve(file_size + 1);

    json.assign(json_str, json_str + file_size);
    json.push_back('\0');

    return preview_proxy::parse_json(json.data(), _uri);
}

snaps::snap_metainfo_uptr load_snap_meta_from_file(const std::wstring &_path, const std::string &_ttl_id)
{
    assert(!_path.empty());
    assert(!_ttl_id.empty());

    tools::binary_stream json_file;
    if (!json_file.load_from_file(_path))
    {
        return nullptr;
    }

    const auto file_size = json_file.available();
    if (file_size == 0)
    {
        return nullptr;
    }

    const auto json_str = (char*)json_file.read(file_size);

    std::vector<char> json;
    json.reserve(file_size + 1);

    json.assign(json_str, json_str + file_size);
    json.push_back('\0');

    return snaps::parse_json(json.data(), _ttl_id);
}

std::string sign_loader_uri(const std::string &_host, const wim_packet_params &_params, const str_2_str_map &_extra)
{
    assert(!_host.empty());

    const time_t ts = (std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - _params.time_offset_);

    str_2_str_map p;
    p["a"] = _params.a_token_;
    p["k"] = _params.dev_id_;
    p["ts"] = tools::from_int64(ts);
    p["client"] = "icq";

    p.insert(_extra.begin(), _extra.end());

    const auto sha256 = wim_packet::escape_symbols(wim_packet::get_url_sign(_host, p, _params, false));
    p["sig_sha256"] = sha256;

    std::stringstream ss_url;
    ss_url << _host << "?" << wim_packet::format_get_params(p);

    return ss_url.str();
}

namespace
{
    std::string get_uri_ext(const std::string& _uri)
    {
        auto ext_end_pos = _uri.length();

        const auto last_anchor_pos = _uri.find_last_of('#');
        if (last_anchor_pos != std::string::npos)
        {
            ext_end_pos = last_anchor_pos;
        }

        const auto last_question_pos = _uri.find_last_of('?');
        if (last_question_pos != std::string::npos)
        {
            ext_end_pos = std::min(ext_end_pos, last_question_pos);
        }

        const auto last_slash_pos = _uri.find_last_of('/');
        const auto last_dot_pos = _uri.find_last_of('.');
        const auto is_ext_found = ((last_dot_pos != std::string::npos) && (last_dot_pos > last_slash_pos));
        if (!is_ext_found)
        {
            return std::string();
        }

        const auto ext_begin_pos = last_dot_pos;

        const auto ext_length = (ext_end_pos - ext_begin_pos);

        const auto is_valid_ext = (ext_length <= 5);
        if (!is_valid_ext)
        {
            return std::string();
        }

        return _uri.substr(ext_begin_pos, ext_length);
    }
}

CORE_WIM_NS_END