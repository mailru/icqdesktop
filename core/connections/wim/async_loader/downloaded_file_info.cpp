#include "stdafx.h"

#include "downloaded_file_info.h"

core::wim::downloaded_file_info::downloaded_file_info(const std::string _url, std::wstring _local_path)
    : url_(_url)
    , local_path_(_local_path)
{
}
