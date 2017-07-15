#pragma once

#include "../../corelib/enumerations.h"

#include "../namespaces.h"

CORE_TOOLS_NS_BEGIN

enum class file_sharing_preview_size
{
    min,

    small,
    normal,

    max
};

std::string format_file_sharing_preview_uri(const std::string &_id, const file_sharing_preview_size _size);

bool get_content_type_from_uri(const std::string& _uri, Out core::file_sharing_content_type& _type);

bool get_content_type_from_file_sharing_id(const std::string& _file_id, Out core::file_sharing_content_type& _type);

bool parse_new_file_sharing_uri(const std::string &_uri, Out std::string &_fileId);

std::string get_file_id(const std::string& _uri);

CORE_TOOLS_NS_END