#pragma once

#include "../../../namespaces.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

typedef std::unique_ptr<class link_meta> link_meta_uptr;

typedef boost::optional<int32_t> int32_t_opt;

typedef std::tuple<int32_t, int32_t> preview_size;

class link_meta final
{
public:
    link_meta(
        const std::string &_title,
        const std::string &_annotation,
        const std::string &_preview_uri,
        const std::string &_download_uri,
        const std::string &_favicon_uri,
        const std::string &_site_name,
        const std::string &_content_type,
        const preview_size &_preview_size,
        const int64_t _file_size);

    ~link_meta();

    std::string get_preview_uri(
        const int32_t_opt _width = int32_t_opt(),
        const int32_t_opt _height = int32_t_opt()) const;

    const std::string& get_annotation() const;

    const std::string& get_content_type() const;

    const std::string& get_download_uri() const;

    const std::string& get_favicon_uri() const;

    int64_t get_file_size() const;

    preview_size get_preview_size() const;

    const std::string& get_site_name() const;

    const std::string& get_title() const;

    bool has_favicon_uri() const;

    bool has_preview_uri() const;

private:
    std::string preview_uri_;

    std::string download_uri_;

    std::string title_;

    std::string annotation_;

    std::string favicon_uri_;

    std::string site_name_;

    std::string content_type_;

    preview_size preview_size_;

    int64_t file_size_;

};

enum class favicon_size
{
    undefined,

    min,

    small,
    med,

    max,
};

str_2_str_map format_get_preview_params(
    const std::string &_uri_to_preview,
    const int32_t_opt _width = int32_t_opt(),
    const int32_t_opt _height = int32_t_opt(),
    const bool _crop = false,
    const favicon_size _favicon_size = favicon_size::small);

str_2_str_map format_get_url_content_params(const std::string &_uri);

link_meta_uptr parse_json(InOut char *_json, const std::string &_uri);

namespace uri
{
    const std::string& get_preview();

    const std::string& get_url_content();
}

CORE_WIM_PREVIEW_PROXY_NS_END