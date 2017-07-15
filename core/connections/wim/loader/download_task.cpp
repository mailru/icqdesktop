#include "stdafx.h"

#include "../../../common.shared/loader_errors.h"
#include "loader_handlers.h"
#include "web_file_info.h"
#include "../../../tools/md5.h"
#include "../../../tools/system.h"

#include "../packets/get_file_meta_info.h"
#include "../packets/load_file.h"
#include "loader.h"

#include "download_task.h"

using namespace core;
using namespace wim;

download_task::download_task(
    const std::string &_id,
    const wim_packet_params& _params,
    const std::string& _file_url,
    const std::wstring& _files_folder,
    const std::wstring& _previews_folder,
    const std::wstring& _filename)
    : fs_loader_task(_id, _params)
    , files_folder_(_files_folder)
    , previews_folder_(_previews_folder)
    , filename_(_filename)
{
    info_.reset(new web_file_info());
    info_->set_file_url(_file_url);
}

download_task::~download_task()
{
    if (file_stream_.is_open())
        file_stream_.close();
}

void download_task::set_handler(std::shared_ptr<download_progress_handler> _handler)
{
    handler_ = _handler;
}

std::shared_ptr<download_progress_handler> download_task::get_handler()
{
    return handler_;
}

std::shared_ptr<web_file_info> download_task::make_info()
{
    return std::make_shared<web_file_info>(*info_);
}

std::wstring download_task::get_info_file_name() const
{
    std::string url = info_->get_file_url();

    return previews_folder_ + L"/" + core::tools::from_utf8(core::tools::md5(url.c_str(), (uint32_t)url.size()));
}

loader_errors download_task::on_finish()
{
    if (file_stream_.is_open())
        file_stream_.close();

    if (!core::tools::system::move_file(file_name_temp_, info_->get_file_name()))
        return loader_errors::move_file;

    if (!serialize_metainfo())
    {
        return loader_errors::store_info;
    }

    return loader_errors::success;
}

int32_t download_task::copy_if_needed()
{
    if (info_->get_file_name().empty())
        return 0;

    std::wstring default_filename = core::tools::system::get_file_name(info_->get_file_name());
    if (!core::tools::system::compare_dirs(core::tools::system::get_file_directory(info_->get_file_name()), files_folder_) || (!filename_.empty() && filename_ != default_filename))
    {
        std::wstring filename = filename_.empty() ? default_filename : filename_;
        const auto is_complete = boost::filesystem::path(filename_).is_complete(); // check if filename is complete
        core::tools::system::copy_file(info_->get_file_name(), is_complete ? filename : files_folder_ + L"/" + filename);
    }

    return 0;
}

bool download_task::serialize_metainfo()
{
    assert(info_);

    core::tools::binary_stream bs;
    info_->serialize(bs);

    if (!bs.save_2_file(get_info_file_name()))
    {
        return false;
    }

    return true;
}

void download_task::delete_metainfo_file()
{
    tools::system::delete_file(get_info_file_name());
}

bool download_task::load_metainfo_from_local_cache()
{
    core::tools::binary_stream bs;
    if (!bs.load_from_file(get_info_file_name()))
    {
        return false;
    }

    web_file_info info;
    if (!info.unserialize(bs))
    {
        return false;
    }

    if (core::tools::system::is_exist(info.get_file_name()))
    {
        info.set_bytes_transfer(info.get_file_size());
    }

    *info_ = info;

    return true;
}

bool download_task::is_downloaded_file_exists()
{
    const auto &local_path = info_->get_file_name();

    if (local_path.empty())
    {
        return false;
    }

    return core::tools::system::is_exist(local_path);
}

void download_task::set_played(bool played)
{
    std::wstring info_file = get_info_file_name();

    core::tools::binary_stream bs;
    if (!bs.load_from_file(info_file))
    {
        return;
    }

    web_file_info info;
    if (!info.unserialize(bs))
    {
        return;
    }

    info.set_played(played);
    info.serialize(bs);
    bs.save_2_file(info_file);
}

void download_task::on_result(int32_t _error)
{
    // core thread
    if (!get_handler()->on_result)
        return;

    get_handler()->on_result(_error, *make_info());
}

void download_task::on_progress()
{
    // core thread
    if (get_handler()->on_progress)
        get_handler()->on_progress(*make_info());
}

bool download_task::get_file_id(const std::string& _file_url, std::string& _file_id)
{
    _file_id = core::tools::trim_right<std::string>(_file_url, "/");

    size_t endpos = _file_id.rfind("/");

    if (std::string::npos != endpos)
    {
        _file_id = _file_id.substr(++endpos);
        if (_file_id.empty())
            return false;

        return true;
    }

    return false;
}

loader_errors download_task::download_metainfo()
{
    std::string file_id;
    if (!get_file_id(info_->get_file_url(), file_id))
    {
        return loader_errors::invalid_url;
    }

    info_->set_file_id(file_id);

    get_file_meta_info packet(get_wim_params(), *info_);
    auto err = packet.execute();
    if (err != 0)
    {
        if (err == wpie_error_metainfo_not_found)
            return loader_errors::metainfo_not_found;

        return loader_errors::get_metainfo;
    }

    *info_ = packet.get_info();

    return loader_errors::success;
}

loader_errors download_task::open_temporary_file()
{
    if (!core::tools::system::is_exist(files_folder_))
    {
        if (!core::tools::system::create_directory(files_folder_))
            return loader_errors::create_directory;
    }

    std::wstring file_name;
    
    if (!filename_.empty() && boost::filesystem::path(filename_).is_complete()) // check if filename is complete
    {
        file_name = filename_;
    }
    else
    {
        file_name = files_folder_ + L"/" + (filename_.empty() ? core::tools::from_utf8(info_->get_file_name_short()) : filename_);
        if (core::tools::system::is_exist(file_name))
        {
            boost::filesystem::wpath path_for_file(file_name);
            std::wstring ext = path_for_file.extension().wstring();
            std::wstring file_name_without_ext = file_name.substr(0, file_name.length() - ext.length());

            for (auto i = 0; i < 1000; i++)
            {
                std::wstringstream ss_file_name;

                ss_file_name << file_name_without_ext << L"-" << i << ext;

                if (!core::tools::system::is_exist(ss_file_name.str()))
                {
                    file_name = ss_file_name.str();
                    break;
                }
            }
        }
    }

    file_name_temp_ = file_name + L".tmp";

    info_->set_file_name(file_name);

#ifdef _WIN32
    file_stream_.open(file_name_temp_, std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);
#else
    file_stream_.open(tools::from_utf16(file_name_temp_), std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);
#endif

    if (!file_stream_.is_open())
        return loader_errors::create_file;

    return loader_errors::success;
}

bool download_task::is_end()
{
    assert(info_->get_bytes_transfer() <= info_->get_file_size());

    return (info_->get_bytes_transfer() == info_->get_file_size());
}

loader_errors download_task::load_next_range()
{
    load_file packet(get_wim_params(), *info_);

    uint32_t res = packet.execute();

    if (res != 0)
    {
        if (res == wim_protocol_internal_error::wpie_network_error)
            return loader_errors::network_error;

        return loader_errors::load_range;
    }

    *info_ = packet.get_info();
    uint32_t size = packet.get_response().available();
    file_stream_.write(
        (const char*) packet.get_response().read(size), size);

    return loader_errors::success;
}

void download_task::resume(loader& _loader)
{
    _loader.load_file_sharing_task_ranges_async(shared_from_this());
}

std::string download_task::get_file_direct_url() const
{
    if (!info_)
    {
        return std::string();
    }

    return info_->get_file_dlink();
}
