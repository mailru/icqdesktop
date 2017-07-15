#include "stdafx.h"

#include "strings.h"

#include "file_sharing.h"

CORE_TOOLS_NS_BEGIN

using namespace boost::xpressive;

namespace
{
    #define NET_URI_PREFIX "^http(s?)://files\\.icq\\.net/get"

    #define COM_URI_PREFIX "^http(s?)://icq\\.com/files"

    const auto new_id_regex_a = sregex::compile(NET_URI_PREFIX "/(?P<id>\\w{33,})$");

    const auto new_id_regex_b = sregex::compile(COM_URI_PREFIX "/(?P<id>\\w{33,})$");

    const auto NEW_ID_LENGTH_MIN = 33;
}

std::string format_file_sharing_preview_uri(const std::string &_id, const file_sharing_preview_size _size)
{
    assert(!_id.empty());
    assert(_size > file_sharing_preview_size::min);
    assert(_size < file_sharing_preview_size::max);

    if (_id.length() < NEW_ID_LENGTH_MIN)
    {
        return std::string();
    }

    std::string result;
    result.reserve(128);

    result.append("https://files.icq.com/preview/max/");

    switch(_size)
    {
        case file_sharing_preview_size::small:
            result.append("192/");
            break;

        case file_sharing_preview_size::normal:
            result.append("600/");
            break;

        default:
            assert(!"unknown preview size");
            return std::string();
    }

    result.append(_id);

    return result;
}

bool get_content_type_from_uri(const std::string& _uri, Out core::file_sharing_content_type& _type)
{
    std::string id;

    if (!tools::parse_new_file_sharing_uri(_uri, Out id))
        return false;

    return get_content_type_from_file_sharing_id(id, Out _type);
}

bool get_content_type_from_file_sharing_id(const std::string& _file_id, Out core::file_sharing_content_type& _type)
{
    assert(!_file_id.empty());

    Out _type = core::file_sharing_content_type::undefined;

    const auto id0 = _file_id[0];

    const auto is_snap_image = (id0 == '1');
    if (is_snap_image)
    {
        Out _type = core::file_sharing_content_type::snap_image;
        return true;
    }

    const auto is_snap_gif = (id0 == '5');
    if (is_snap_gif)
    {
        Out _type = core::file_sharing_content_type::snap_gif;
        return true;
    }

    const auto is_snap_video = (id0 == '9');
    if (is_snap_video)
    {
        Out _type = core::file_sharing_content_type::snap_video;
        return true;
    }

    const auto is_gif = (id0 == '4');
    if (is_gif)
    {
        Out _type = core::file_sharing_content_type::gif;
        return true;
    }

    const auto is_image = ((id0 >= '0') && (id0 <= '7'));
    if (is_image)
    {
        Out _type = core::file_sharing_content_type::image;
        return true;
    }

    const auto is_video = (
        ((id0 >= '8') && (id0 <= '9')) ||
        ((id0 >= 'A') && (id0 <= 'F')));
    if (is_video)
    {
        Out _type = core::file_sharing_content_type::video;
        return true;
    }

    const auto is_ptt = (
        ((id0 >= 'I') && (id0 <= 'J')));
    if (is_ptt)
    {
        Out _type = core::file_sharing_content_type::ptt;
        return true;
    }

    return false;
}

bool parse_new_file_sharing_uri(const std::string &_uri, Out std::string &_fileId)
{
    assert(!_uri.empty());

    smatch m;
    if (!regex_match(_uri, m, new_id_regex_a) &&
        !regex_match(_uri, m, new_id_regex_b))
    {
        Out _fileId.clear();

        return false;
    }

    Out _fileId = m["id"].str();
    assert(_fileId.length() >= NEW_ID_LENGTH_MIN);

    return true;
}

std::string get_file_id(const std::string& _uri)
{
    auto id = core::tools::trim_right<std::string>(_uri, "/");

    const auto pos = id.rfind("/");
    if (std::string::npos != pos)
    {
        return id.substr(pos + 1);
    }

    return std::string();
}

CORE_TOOLS_NS_END