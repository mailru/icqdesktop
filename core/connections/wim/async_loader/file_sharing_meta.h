#pragma once

namespace core
{
    namespace wim
    {
        struct file_sharing_meta;

        typedef std::unique_ptr<file_sharing_meta> file_sharing_meta_uptr;

        struct file_sharing_meta
        {
            explicit file_sharing_meta(const std::string& _url);

            std::string     file_url_;
            std::wstring    file_name_;
            int64_t         file_size_;
            std::string     file_name_short_;
            std::string     file_download_url_; // dlink
            std::string     mime_;
            std::string     file_mini_preview_url_;
            std::string     file_full_preview_url_;

            static file_sharing_meta_uptr parse_json(InOut char* _json, const std::string& _uri);
        };
    }
}
