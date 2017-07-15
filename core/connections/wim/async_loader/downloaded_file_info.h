#pragma once

namespace core
{
    namespace wim
    {
        struct downloaded_file_info
        {
            explicit downloaded_file_info(const std::string _url, std::wstring _local_path = std::wstring());

            std::string url_;
            std::wstring local_path_;
        };
    }
}
