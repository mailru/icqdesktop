#include "stdafx.h"

#include "../log/log.h"

#include "cache_garbage_collector.h"

namespace fs = boost::filesystem;

namespace
{
    std::atomic<bool> is_running_(false);
}

CORE_DISK_CACHE_NS_BEGIN

void cleanup_dir(const std::wstring &_path)
{
    assert(!_path.empty());

    auto expected = false;
    if (!is_running_.compare_exchange_strong(InOut expected, true))
    {
        return;
    }

    boost::system::error_code error;
    if (!fs::is_directory(_path, Out error))
    {
        assert(!"wrong path");
        return;
    }

    const fs::directory_iterator dir_end;
    for (fs::directory_iterator dir_entry(_path);
         dir_entry != dir_end;
         ++dir_entry )
    {
        //const auto file_name_path = dir_entry->path().filename().c_str();
        //const auto filename = (
        //    platform::is_windows() ?
        //        tools::from_utf16(file_name_path) :
        //        file_name_path);


    }

    is_running_ = false;
}

CORE_DISK_CACHE_NS_END