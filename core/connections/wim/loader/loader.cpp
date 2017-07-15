#include "stdafx.h"

#include "../../../async_task.h"
#include "../wim_packet.h"
#include "upload_task.h"
#include "download_task.h"
#include "loader_handlers.h"
#include "loader_helpers.h"
#include "preview_proxy.h"
#include "snap_metainfo.h"
#include "web_file_info.h"
#include "../../../http_request.h"
#include "../../../tools/md5.h"
#include "../../../tools/system.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../utils.h"
#include "../../../log/log.h"
#include "../../../disk_cache/disk_cache.h"
#include "../../../profiling/profiler.h"
#include "../../../../common.shared/loader_errors.h"


#include "loader.h"

using namespace core;
using namespace wim;

namespace
{
    bool is_suspendable_error(const loader_errors _error);
}

loader::loader(const std::wstring &_cache_dir)
    : cache_(disk_cache::disk_cache::make(_cache_dir))
    , file_sharing_threads_(new async_executer(1))
{
    initialize_tasks_runners();
}

loader::~loader()
{
}

void loader::add_file_sharing_task(std::shared_ptr<fs_loader_task> _task)
{
    assert(_task);

    const auto &task_id = _task->get_id();
    assert(!task_id.empty());

    auto iter_task = find_task_by_id(file_sharing_tasks_, task_id);
    if (iter_task != file_sharing_tasks_.end())
    {
        assert(!"task with the same id already exist");
        return;
    }

    file_sharing_tasks_.push_back(_task);
}

void loader::remove_file_sharing_task(const std::string &_id)
{
    assert(!_id.empty());

    auto iter_task = find_task_by_id(file_sharing_tasks_, _id);
    if (iter_task == file_sharing_tasks_.end())
    {
        return;
    }

    file_sharing_tasks_.erase(iter_task);
}

loader::const_fs_tasks_iter loader::find_task_by_id(const fs_tasks_queue &_tasks, const std::string &_id)
{
    auto task_iter = std::find_if(
        _tasks.cbegin(),
        _tasks.cend(),
        [&_id]
        (const fs_task_sptr &_item)
        {
            assert(_item);
            return (_item->get_id() == _id);
        });

    return task_iter;
}

void loader::on_file_sharing_task_result(std::shared_ptr<fs_loader_task> _task, int32_t _error)
{
    _task->on_result(_error);

    remove_file_sharing_task(_task->get_id());
}

void loader::on_file_sharing_task_progress(std::shared_ptr<fs_loader_task> _task)
{
    _task->on_progress();
}

bool loader::has_file_sharing_task(const std::string &_id)
{
    assert(!_id.empty());

    return (find_task_by_id(file_sharing_tasks_, _id) != file_sharing_tasks_.cend());
}

void loader::resume_file_sharing_tasks()
{
    for (const auto &task : file_sharing_tasks_)
    {
        if (task->get_last_error() != 0)
        {
            task->set_last_error(0);
            task->resume(*this);
        }
    }
}

void loader::set_played(const std::string& _file_url, const std::wstring& _previews_folder, bool _played, const wim_packet_params& _params)
{
    auto task = std::make_shared<download_task>(
        "0",
        _params,
        _file_url,
        std::wstring(),
        std::wstring(),
        _previews_folder);

    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task, _played]
        {
            const auto task = wr_task.lock();
            if (!task)
                return 0;

            task->set_played(_played);

            return 0;
        }
    )->on_result_ =
        [wr_this, wr_task](int32_t _error)
        {
            const auto task = wr_task.lock();
            if (!task)
                return;

            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->remove_file_sharing_task(task->get_id());
        };
}

void loader::send_task_ranges_async(std::weak_ptr<upload_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    file_sharing_threads_->run_async_function(
        [_wr_task]
        {
            auto task = _wr_task.lock();
            if (!task)
                return -1;

            return (int32_t)task->send_next_range();
        }
    )->on_result_ =
        [wr_this, _wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = _wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                task->set_last_error(_error);

                if (_error != (int32_t)loader_errors::network_error)
                    ptr_this->on_file_sharing_task_result(task, _error);

                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            if (!task->is_end())
            {
                ptr_this->send_task_ranges_async(task);
            }
            else
            {
                ptr_this->on_file_sharing_task_result(task, 0);
                g_core->insert_event(core::stats::stats_event_names::filesharing_sent_success);
            }
        };
}


void loader::load_file_sharing_task_ranges_async(std::weak_ptr<download_task> _wr_task)
{
    std::weak_ptr<loader> wr_this = shared_from_this();

    file_sharing_threads_->run_async_function(
        [_wr_task]
        {
            auto task = _wr_task.lock();
            if (!task)
                return -1;

            auto err = task->load_next_range();

            if (err == loader_errors::success)
            {
                if (task->is_end())
                    err = task->on_finish();
            }

            return (int32_t)err;

        }
    )->on_result_ =
        [wr_this, _wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = _wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                task->set_last_error(_error);

                if (_error != (int32_t)loader_errors::network_error)
                {
                    ptr_this->on_file_sharing_task_result(task, _error);
                }

                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            if (!task->is_end())
            {
                ptr_this->load_file_sharing_task_ranges_async(task);
            }
            else
            {
                ptr_this->on_file_sharing_task_result(task, 0);
            }
        };
}

std::shared_ptr<upload_progress_handler> loader::upload_file_sharing(
    const std::string &_guid,
    const std::wstring& _file_name,
    const wim_packet_params& _params)
{
    assert(!_guid.empty());
    assert(!_file_name.empty());

    auto task = std::make_shared<upload_task>(_guid, _params, _file_name);
    task->set_handler(std::make_shared<upload_progress_handler>());
    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<upload_task> wr_task = task;

    file_sharing_threads_->run_async_function(
        [wr_task]
        {
            auto task = wr_task.lock();
            if (!task)
                return -1;

            return (int32_t)task->open_file();

        }
    )->on_result_ =
        [wr_this, wr_task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            if (_error != 0)
            {
                ptr_this->on_file_sharing_task_result(task, _error);
                return;
            }

            ptr_this->on_file_sharing_task_progress(task);

            ptr_this->file_sharing_threads_->run_async_function(
                [wr_task]
                {
                    auto task = wr_task.lock();
                    if (!task)
                        return (int32_t)loader_errors::undefined;

                    return (int32_t)task->get_gate();
                }
            )->on_result_ =
                [wr_this, wr_task](int32_t _error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    auto task = wr_task.lock();
                    if (!task)
                        return;

                    if (_error != 0)
                    {
                        ptr_this->on_file_sharing_task_result(task, _error);
                        return;
                    }

                    ptr_this->on_file_sharing_task_progress(task);

                    ptr_this->send_task_ranges_async(task);
                };
        };

    return task->get_handler();
}


void cleanup_cache(const std::wstring& _cache_folder)
{
    const int32_t max_files_to_delete = 100;
    const auto delete_files_older = std::chrono::hours(24);
    const auto cleanup_period = std::chrono::minutes(10);

    static std::chrono::system_clock::time_point last_cleanup_time = std::chrono::system_clock::now();

    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();

    if (current_time - last_cleanup_time < cleanup_period)
        return;

    tools::binary_stream bs;
    bs.write<std::string>("Start cleanup files cache\r\n");

    last_cleanup_time = current_time;

    typedef std::list<boost::filesystem::wpath> results_list;

    results_list files_to_delete;

    boost::filesystem::wpath cache_path(_cache_folder);

    boost::filesystem::directory_iterator end_iter;

    boost::system::error_code error;


    if (boost::filesystem::exists(cache_path, error) && boost::filesystem::is_directory(cache_path, error))
    {
        for (boost::filesystem::directory_iterator dir_iter(cache_path) ; ((dir_iter != end_iter) && (files_to_delete.size() < max_files_to_delete)); ++dir_iter)
        {
            if (boost::filesystem::is_regular_file(dir_iter->status()))
            {
                auto write_time = std::chrono::system_clock::from_time_t(boost::filesystem::last_write_time(dir_iter->path(), error));
                if ((current_time - write_time) > delete_files_older)
                    files_to_delete.push_back(*dir_iter);
            }
        }
    }

    for (auto _file_path : files_to_delete)
    {
        boost::filesystem::remove(_file_path, error);

        bs.write<std::string>("Delete file: ");
        bs.write<std::string>(_file_path.string());
        bs.write<std::string>("\r\n");

    }

    bs.write<std::string>("Finish cleanup\r\n");
    g_core->get_network_log().write_data(bs);
}


std::shared_ptr<download_progress_handler> loader::download_file_sharing(
    const int64_t _seq,
    const std::string& _file_url,
    const file_sharing_function _function,
    const std::wstring& _files_folder,
    const std::wstring& _previews_folder,
    const std::wstring& _filename,
    const bool _force_request_metainfo,
    const wim_packet_params& _params)
{
    assert(_function > file_sharing_function::min);
    assert(_function < file_sharing_function::max);
    assert(_function != file_sharing_function::download_preview_metainfo);

    auto task = std::make_shared<download_task>(
        std::to_string(_seq),
        _params,
        _file_url,
        _files_folder,
        _previews_folder,
        _filename
    );
    task->set_handler(std::make_shared<download_progress_handler>());
    add_file_sharing_task(task);

    std::weak_ptr<loader> wr_this = shared_from_this();
    std::weak_ptr<download_task> wr_task = task;

    file_sharing_threads_->run_async_function([wr_task, _function, _force_request_metainfo]
    {
        const auto task = wr_task.lock();
        if (!task)
        {
            return 0;
        }

        // return -1 (failure) if there are no metainfo file or there are no downloaded fs link in the cache

        if (!task->load_metainfo_from_local_cache())
        {
            return -1;
        }

        const auto check_file_existence = (
            (_function == file_sharing_function::download_file) ||
            (_function == file_sharing_function::check_local_copy_exists));

        if (check_file_existence && !task->is_downloaded_file_exists())
        {
            return -1;
        }

        if (_force_request_metainfo)
        {
            return -1;
        }

        return 0;

    })->on_result_ = [wr_this, wr_task, _function, _force_request_metainfo, _previews_folder](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto task = wr_task.lock();
        if (!task)
            return;

        const auto is_local_copy_test_mode = (_function == file_sharing_function::check_local_copy_exists);
        const auto is_file_and_metainfo_ready = (_error == 0);

        if (is_file_and_metainfo_ready || is_local_copy_test_mode)
        {
            task->copy_if_needed();

            ptr_this->on_file_sharing_task_result(task, _error);

            ptr_this->file_sharing_threads_->run_async_function([wr_task, _previews_folder]
            {
                cleanup_cache(_previews_folder);

                return 0;
            });

            return;
        }

        ptr_this->file_sharing_threads_->run_async_function([wr_task]
        {
            auto task = wr_task.lock();
            if (!task)
            {
                return (int32_t)loader_errors::orphaned;
            }

            loader_errors error = task->download_metainfo();

            if (error == loader_errors::metainfo_not_found)
            {
                task->delete_metainfo_file();
            }

            return (int32_t) error;

        })->on_result_ = [wr_this, wr_task, _function, _force_request_metainfo, _previews_folder](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto task = wr_task.lock();
            if (!task)
                return;

            const auto is_download_file_metainfo_mode = (_function == file_sharing_function::download_file_metainfo);

            const auto success = (_error == 0);

            if (success && (is_download_file_metainfo_mode || _force_request_metainfo))
            {
                task->serialize_metainfo();
            }

            if (!success || is_download_file_metainfo_mode)
            {
                ptr_this->on_file_sharing_task_result(task, _error);
                return;
            }

            if (_force_request_metainfo)
            {
                if (task->is_downloaded_file_exists())
                {
                    ptr_this->on_file_sharing_task_result(task, 0);
                    return;
                }
            }

            g_core->insert_event(stats::stats_event_names::filesharing_download_success);

            ptr_this->on_file_sharing_task_progress(task);

            ptr_this->file_sharing_threads_->run_async_function([wr_task]
            {
                auto task = wr_task.lock();
                if (!task)
                    return -1;

                return (int32_t)task->open_temporary_file();

            })->on_result_ = [wr_this, wr_task, _previews_folder](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                auto task = wr_task.lock();
                if (!task)
                    return;

                if (_error != 0)
                {
                    ptr_this->on_file_sharing_task_result(task, _error);
                    return;
                }

                ptr_this->load_file_sharing_task_ranges_async(task);

                ptr_this->file_sharing_threads_->run_async_function([wr_task, _previews_folder]
                {
                    cleanup_cache(_previews_folder);

                    return 0;
                });
            };
        };
    };

    return task->get_handler();
}


std::shared_ptr<get_file_direct_uri_handler> loader::get_file_direct_uri(
    const int64_t _seq, 
    const std::string& _file_url, 
    const std::wstring& _cache_dir, 
    const wim_packet_params& _params)
{
    auto handler = std::make_shared<get_file_direct_uri_handler>();

    auto url = std::make_shared<std::string>();

    file_sharing_threads_->run_async_function([url, _params, _file_url, _cache_dir]()->int32_t
    {
        loader_errors error = loader_errors::success;

        download_task task(tools::system::generate_guid(), _params, _file_url, L"", _cache_dir, L"");
        error = task.download_metainfo();

        if (error == loader_errors::success)
        {
            *url = task.get_file_direct_url();
        }

        return (int32_t)error;

    })->on_result_ = [handler, url](int32_t _error)
    {
        handler->on_result(_error, *url);
    };

    return handler;
}

void loader::add_task(loader_task_sptr _task)
{
}

void loader::initialize_tasks_runners()
{
}

void loader::run_next_task(const tasks_runner_slot _slot)
{
}

void loader::resume_task(
    const int64_t _id,
    const wim_packet_params &_wim_params)
{
}

void loader::abort_file_sharing_process(const std::string &_process_id)
{
    assert(!_process_id.empty());

    remove_file_sharing_task(_process_id);
}

namespace
{
    bool is_suspendable_error(const loader_errors _error)
    {
        return (_error == loader_errors::network_error) ||
               (_error == loader_errors::suspend);
    }
}
