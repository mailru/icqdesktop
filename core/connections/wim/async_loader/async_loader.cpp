#include "stdafx.h"

#include "../loader/loader_helpers.h"

#include "../wim_packet.h"

#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../network_log.h"
#include "../../../tools/file_sharing.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

#include "async_loader.h"

core::wim::async_loader::async_loader(const std::wstring& _content_cache_dir)
    : content_cache_dir_(_content_cache_dir)
{
}

void core::wim::async_loader::set_download_dir(const std::wstring& _download_dir)
{
    download_dir_ = _download_dir;
}

void core::wim::async_loader::download(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, default_handler_t _handler)
{
    const auto start = std::chrono::steady_clock().now();

    __INFO("async_loader",
        "download\n"
        "url      = <%1%>\n"
        "handler  = <%2%>\n", _url % _handler.to_string());

    auto user_proxy = g_core->get_proxy_settings();

    const auto hash = std::hash<std::string>()(_url);

    auto wim_stop_handler = _wim_params.stop_handler_;

    auto stop = [wim_stop_handler, hash, this]()
    {
        if (wim_stop_handler && wim_stop_handler())
            return true;

        std::lock_guard<std::mutex> lock(to_cancel_mutex_);
        auto it = to_cancel_.find(hash);
        if (it != to_cancel_.end())
        {
            to_cancel_.erase(it);
            return true;
        }

        return false;
    };

    auto request = std::make_shared<http_request_simple>(
        user_proxy, utils::get_user_agent(), stop, _handler.progress_callback_);

    request->set_url(_url);
    request->set_need_log(_wim_params.full_log_);
    request->set_keep_alive();
    request->replace_host(_wim_params.hosts_);
    request->set_priority(_priority);

    request->get_async([start, _url, hash, request, _handler, this](bool _success)
    {
        const auto finish = std::chrono::steady_clock().now();

        {
            std::lock_guard<std::mutex> lock(to_cancel_mutex_);
            auto it = to_cancel_.find(hash);
            if (it != to_cancel_.end())
            {
                to_cancel_.erase(it);
            }
        }

        std::stringstream log;
        log << "async request\n";
        log << "url: " << _url << '\n';
        log << "completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish -start).count() << " ms\n";
        if (_success)
            log << request->get_response_code();
        else
            log << "error";
        log << '\n';

        g_core->get_network_log().write_string(log.str());

        __INFO("async_loader",
            "download\n"
            "url      = <%1%>\n"
            "handler  = <%2%>\n"
            "success  = <%3%>\n"
            "response = <%4%>\n", _url % _handler.to_string() % logutils::yn(_success) % request->get_response_code());

        if (_success)
        {
            if (request->get_response_code() == 200)
            {
                if (_handler.completion_callback_)
                {
                    default_data_t data(request->get_response_code(), request->get_header(), std::static_pointer_cast<tools::binary_stream>(request->get_response()));
                    _handler.completion_callback_(loader_errors::success, data);
                }
            }
            else
            {
                fire_callback(loader_errors::http_error, default_data_t(), _handler.completion_callback_);
            }
        }
        else
        {
            fire_callback(loader_errors::network_error, default_data_t(), _handler.completion_callback_);
        }
    });
}

void core::wim::async_loader::cancel(const std::string& _url)
{
    std::lock_guard<std::mutex> lock(to_cancel_mutex_);
    const auto hash = std::hash<std::string>()(_url);
    to_cancel_.insert(hash);
}

void core::wim::async_loader::download_file(priority_t _priority, const std::string& _url, const std::wstring& _file_name, const wim_packet_params& _wim_params, async_handler<downloaded_file_info> _handler)
{
    download_file(_priority, _url, tools::from_utf16(_file_name), _wim_params, _handler);
}

void core::wim::async_loader::download_file(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, async_handler<downloaded_file_info> _handler)
{
    __INFO("async_loader",
        "download_file\n"
        "url      = <%1%>\n"
        "file     = <%2%>\n"
        "handler  = <%3%>\n", _url % _file_name % _handler.to_string());

    auto local_handler = default_handler_t([_url, _file_name, _handler](loader_errors _error, const default_data_t& _data)
    {
        __INFO("async_loader",
            "download_file\n"
            "url      = <%1%>\n"
            "file     = <%2%>\n"
            "handler  = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", _url % _file_name % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        const auto file_name_utf16 = tools::from_utf8(_file_name);

        file_info_data_t data(_data, std::make_shared<downloaded_file_info>(_url, file_name_utf16));

        if (_error != loader_errors::success)
        {
            fire_callback(_error, data, _handler.completion_callback_);
            return;
        }

        if (!_data.content_->save_2_file(file_name_utf16))
        {
            fire_callback(loader_errors::save_2_file, data, _handler.completion_callback_);
            return;
        }

        data.content_->reset_out();

        fire_callback(loader_errors::success, data, _handler.completion_callback_);

    }, _handler.progress_callback_);

    const auto file_name_utf16 = core::tools::from_utf8(_file_name);

    if (core::tools::system::is_exist(_file_name))
    {
        auto file = core::tools::system::open_file_for_read(_file_name, std::ios::binary);
        if (file.good())
        {
            file_info_data_t data;

            data.content_ = std::make_shared<core::tools::binary_stream>();
            data.content_->write_stream(file);
            data.additional_data_ = std::make_shared<core::wim::downloaded_file_info>(_url, file_name_utf16);

            fire_callback(loader_errors::success, data, _handler.completion_callback_);

            return;
        }
        else
        {
            core::tools::system::delete_file(file_name_utf16);
        }
    }

    download(_priority, _url, _wim_params, local_handler);
}

void core::wim::async_loader::download_image_metainfo(const std::string& _url, const wim_packet_params& _wim_params, link_meta_handler_t _handler)
{
    const auto ext_params = preview_proxy::format_get_preview_params(_url);
    const auto signed_url = sign_loader_uri(preview_proxy::uri::get_preview(), _wim_params, ext_params);
    download_metainfo(_url, signed_url, preview_proxy::parse_json, _wim_params, _handler);
}

void core::wim::async_loader::download_snap_metainfo(const std::string& _ttl_id, const wim_packet_params& _wim_params, snap_meta_handler_t _handler)
{
    const auto ext_params = snaps::format_get_metainfo_params(_ttl_id);
    const auto signed_url = sign_loader_uri(snaps::uri::get_metainfo(), _wim_params, ext_params);
    download_metainfo(_ttl_id, signed_url, snaps::parse_json, _wim_params, _handler);
}

void core::wim::async_loader::download_file_sharing_metainfo(const std::string& _url, const wim_packet_params& _wim_params, file_sharing_meta_handler_t _handler)
{
    std::string id = tools::get_file_id(_url);
    assert(!id.empty());

    std::stringstream ss_url;

    ss_url << "https://files.icq.com/getinfo?file_id=" << id;
    ss_url << "&r=" <<  core::tools::system::generate_guid();

    download_metainfo(_url, ss_url.str(), &file_sharing_meta::parse_json, _wim_params, _handler);
}

void core::wim::async_loader::download_image_preview(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params,
    link_meta_handler_t _metainfo_handler, file_info_handler_t _preview_handler)
{
    __INFO("async_loader",
        "download_image_preview\n"
        "url      = <%1%>\n"
        "mhandler = <%2%>\n"
        "phandler = <%3%>\n", _url % _metainfo_handler.to_string() % _preview_handler.to_string());

    auto local_handler = link_meta_handler_t([_priority, _url, _wim_params, _metainfo_handler, _preview_handler, this](loader_errors _error, const link_meta_data_t& _data)
    {
        __INFO("async_loader",
            "download_metainfo\n"
            "url      = <%1%>\n"
            "mhandler = <%2%>\n"
            "phandler = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", _url % _metainfo_handler.to_string() % _preview_handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        file_info_data_t file_data(_data, std::make_shared<downloaded_file_info>(_url));

        if (_error == loader_errors::network_error)
        {
            suspended_tasks_.push([_priority, _url, _metainfo_handler, _preview_handler, this](const wim_packet_params& wim_params)
            {
                download_image_preview(_priority, _url, wim_params, _metainfo_handler, _preview_handler);
            });
            return;
        }

        if (_error != loader_errors::success)
        {
            fire_callback(_error, _data, _metainfo_handler.completion_callback_);

            fire_callback(_error, file_data, _preview_handler.completion_callback_);
            return;
        }

        fire_callback(_error, _data, _metainfo_handler.completion_callback_);

        const auto meta = *_data.additional_data_;

        if (!meta.has_preview_uri())
        {
            fire_callback(loader_errors::no_link_preview, file_data, _preview_handler.completion_callback_);
            return;
        }

        const auto preview_url = meta.get_preview_uri(0, 0);
        const auto file_path = get_path_in_cache(content_cache_dir_, preview_url, path_type::link_preview);

        download_file(_priority, preview_url, file_path, _wim_params, _preview_handler);

    }, _metainfo_handler.progress_callback_);

    download_image_metainfo(_url, _wim_params, local_handler);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const wim_packet_params& _wim_params, file_info_handler_t _handler)
{
    download_image(_priority, _url, std::string(), _wim_params, _handler);
}

void core::wim::async_loader::download_image(priority_t _priority, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, file_info_handler_t _handler)
{
    const auto path = _file_name.empty()
        ? tools::from_utf16(get_path_in_cache(content_cache_dir_, _url, path_type::file))
        : _file_name;

    __INFO("async_loader",
        "download_image\n"
        "url      = <%1%>\n"
        "file     = <%2%>\n"
        "handler  = <%3%>\n", _url % path % _handler.to_string());

    auto local_handler = file_info_handler_t([_priority, _url, _file_name, path, _wim_params, _handler, this](loader_errors _error, const file_info_data_t& _data)
    {
        __INFO("async_loader",
            "download_image\n"
            "url      = <%1%>\n"
            "file     = <%2%>\n"
            "handler  = <%3%>\n"
            "result   = <%4%>\n"
            "response = <%5%>\n", _url % path % _handler.to_string() % static_cast<int>(_error) % _data.response_code_);

        if (_error == loader_errors::network_error)
        {
            suspended_tasks_.push([_priority, _url, _file_name, _handler, this](const wim_packet_params& wim_params)
            {
                download_image(_priority, _url, _file_name, wim_params, _handler);
            });
            return;
        }

        fire_callback(_error, _data, _handler.completion_callback_);

    }, _handler.progress_callback_);

    const auto is_dropbox = boost::ends_with(_url, "?dl=0")
        && _url.find("dropbox.com/") != std::string::npos;
    if (is_dropbox)
    {
        // replace ?dl=0 with ?dl=1
        auto url = _url;
        url[url.size() - 1] = '1';
        download_file(_priority, url, path, _wim_params, local_handler);
        return;
    }

    download_file(_priority, _url, path, _wim_params, local_handler);
}

void core::wim::async_loader::download_file_sharing(
    priority_t _priority, const std::string& _contact, const std::string& _url, const std::string& _file_name, const wim_packet_params& _wim_params, file_info_handler_t _handler)
{
    {
        std::lock_guard<std::mutex> lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        if (it != in_progress_.end())
        {
            update_file_chunks(*it->second, _priority, _handler);
            return;
        }
    }

    tools::system::delete_file(get_path_in_cache(content_cache_dir_, _url, path_type::link_meta));

    std::weak_ptr<async_loader> wr_this(shared_from_this());

    download_file_sharing_metainfo(_url, _wim_params, file_sharing_meta_handler_t(
        [_priority, _contact, _url, _file_name, _wim_params, _handler, wr_this, this](loader_errors _error, const file_sharing_meta_data_t& _data)
        {
            std::shared_ptr<async_loader> ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == loader_errors::network_error)
            {
                suspended_tasks_.push([_priority, _contact, _url, _file_name, _handler, this](const wim_packet_params& wim_params)
                {
                    download_file_sharing(_priority, _contact, _url, _file_name, wim_params, _handler);
                });
                return;
            }

            auto meta = _data.additional_data_;
            if (!meta)
            {
                fire_callback(loader_errors::network_error, file_info_data_t(), _handler.completion_callback_);
                return;
            }

            ptr_this->cleanup_cache();

            std::wstring file_path;

            if (!_file_name.empty())
            {
                file_path = tools::from_utf8(_file_name);
            }
            else
            {
                file_sharing_content_type content_type = file_sharing_content_type::undefined;
                tools::get_content_type_from_uri(_url, Out content_type);

                switch (content_type)
                {
                    case file_sharing_content_type::image:
                    case file_sharing_content_type::gif:
                    case file_sharing_content_type::video:
                    case file_sharing_content_type::snap_image:
                    case file_sharing_content_type::snap_gif:
                    case file_sharing_content_type::snap_video:
                    case file_sharing_content_type::ptt:
                        file_path = get_path_in_cache(ptr_this->content_cache_dir_, _url, path_type::file) + tools::from_utf8(boost::filesystem::extension(meta->file_name_short_));
                        break;
                    default:
                        {
                            auto tmp_path = boost::filesystem::wpath(ptr_this->download_dir_) / tools::from_utf8(meta->file_name_short_);
                            file_path = tmp_path.wstring();
                            int n = 0;
                            while (tools::system::is_exist(file_path))
                            {
                                const auto stem = tmp_path.stem().wstring();
                                const auto extension = tmp_path.extension().wstring();
                                std::wstringstream new_name;
                                new_name << "/" << stem << " (" << ++n << ")" << extension;
                                file_path = tmp_path.parent_path().wstring() + new_name.str();
                            }
                        }
                        break;
                }
            }

            file_info_data_t data(std::make_shared<core::wim::downloaded_file_info>(_url, file_path));

            if (tools::system::is_exist(file_path))
            {
                if (tools::system::get_file_size(file_path) == meta->file_size_)
                {
                    fire_callback(loader_errors::success, data, _handler.completion_callback_);
                    return;
                }
                else
                {
                    tools::system::delete_file(file_path);
                }
            }
            else
            {
                const auto dir = boost::filesystem::path(file_path).parent_path();
                tools::system::create_directory_if_not_exists(dir);
            }

            auto file_chunks = std::make_shared<downloadable_file_chunks>(_priority, _contact, meta->file_download_url_, file_path, meta->file_size_);
            file_chunks->downloaded_ = tools::system::get_file_size(file_chunks->tmp_file_name_);

            if (file_chunks->total_size_ == file_chunks->downloaded_)
            {
                if (!tools::system::move_file(file_chunks->tmp_file_name_, file_chunks->file_name_))
                {
                    fire_callback(loader_errors::move_file, data, _handler.completion_callback_);
                    return;
                }

                fire_callback(loader_errors::success, data, _handler.completion_callback_);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(ptr_this->in_progress_mutex_);
                auto it = ptr_this->in_progress_.find(_url);
                if (it != ptr_this->in_progress_.end())
                {
                    update_file_chunks(*it->second, _priority, _handler);
                    return;
                }
                file_chunks->handlers_.push_back(_handler);
                ptr_this->in_progress_[_url] = file_chunks;
            }

            ptr_this->download_file_sharing_impl(_url, _wim_params, file_chunks);
        }));
}

void core::wim::async_loader::cancel_file_sharing(const std::string& _url)
{
    std::lock_guard<std::mutex> lock(in_progress_mutex_);
    auto it = in_progress_.find(_url);
    if (it != in_progress_.end())
        it->second->cancel_ = true;
}

void core::wim::async_loader::resume_suspended_tasks(const wim_packet_params& _wim_params)
{
    while (!suspended_tasks_.empty())
    {
        auto task = suspended_tasks_.front();
        suspended_tasks_.pop();
        task(_wim_params);
    }
}

void core::wim::async_loader::contact_switched(const std::string& _contact)
{
    std::lock_guard<std::mutex> lock(in_progress_mutex_);

    const auto hash = std::hash<std::string>()(_contact);

    for (auto& it : in_progress_)
    {
        const auto& contacts = it.second->contacts_;

        if (std::find(std::begin(contacts), std::end(contacts), hash) != std::end(contacts))
        {
            it.second->priority_ = it.second->priority_on_start_;
        }
        else if (it.second->priority_ > highest_priority)
        {
            it.second->priority_ = low_priority;
        }
    }
}

void core::wim::async_loader::download_file_sharing_impl(std::string _url, wim_packet_params _wim_params, downloadable_file_chunks_ptr _file_chunks)
{
    if (_file_chunks->cancel_)
    {
        tools::system::delete_file(_file_chunks->tmp_file_name_);
        fire_chunks_callback(loader_errors::cancelled, _url);
        return;
    }

    auto progress = [_file_chunks, this](int64_t /*_total*/, int64_t _transferred, int32_t /*_in_percentages*/)
    {
        downloadable_file_chunks::handler_list_t handler_list;

        {
            std::lock_guard<std::mutex> lock(in_progress_mutex_);
            handler_list = _file_chunks->handlers_;
        }

        const auto downloaded = _file_chunks->downloaded_ + _transferred;
        for (auto& handler : handler_list)
        {
            if (handler.progress_callback_)
            {
                g_core->execute_core_context([_file_chunks, handler, downloaded]()
                    {
                        handler.progress_callback_(_file_chunks->total_size_, downloaded, downloaded / (_file_chunks->total_size_ / 100.0));
                    });
            }
        }
    };

    auto stop = [_file_chunks]()
    {
        return _file_chunks->cancel_;
    };

    auto user_proxy = g_core->get_proxy_settings();

    auto request = std::make_shared<http_request_simple>(user_proxy, utils::get_user_agent(), stop, progress);

    request->set_url(_file_chunks->url_);
    request->set_need_log(_wim_params.full_log_);
    request->set_keep_alive();
    request->replace_host(_wim_params.hosts_);
    request->set_priority(_file_chunks->priority_);

    static const int64_t max_chunk_size = 512 * 1024;
    const auto bytes_left = _file_chunks->total_size_ - _file_chunks->downloaded_;
    const auto chunk_size = _file_chunks->priority_ <= highest_priority
        ? bytes_left
        : bytes_left < max_chunk_size ? bytes_left : max_chunk_size;

    request->set_range(_file_chunks->downloaded_, _file_chunks->downloaded_ + chunk_size);

    const auto flags = _file_chunks->downloaded_ > 0
        ? std::ios::binary | std::ios::app
        : std::ios::binary | std::ios::trunc;

    auto tmp_file = tools::system::open_file_for_write(_file_chunks->tmp_file_name_, flags);

    if (!tmp_file.good())
    {
        fire_chunks_callback(loader_errors::save_2_file, _url);
        return;
    }

    request->set_output_stream(std::make_shared<tools::file_output_stream>(std::move(tmp_file)));

    std::weak_ptr<async_loader> wr_this(shared_from_this());

    request->get_async([_url, _wim_params, _file_chunks, request, wr_this, this](bool _success) mutable
    {
        std::shared_ptr<async_loader> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        const auto code = request->get_response_code();
        if (_success && (code == 206 || code == 200 || code == 201))
        {
            const auto size = request->get_response()->all_size();
            _file_chunks->downloaded_ += size;

            request->get_response()->close();

            if (_file_chunks->total_size_ == _file_chunks->downloaded_)
            {
                if (!tools::system::move_file(_file_chunks->tmp_file_name_, _file_chunks->file_name_))
                {
                    ptr_this->fire_chunks_callback(loader_errors::move_file, _url);
                    return;
                }

                ptr_this->fire_chunks_callback(loader_errors::success, _url);
                return;
            }

            ptr_this->download_file_sharing_impl(_url, _wim_params, _file_chunks);
        }
        else
        {
            if (_file_chunks->cancel_)
            {
                request->get_response()->close();
                tools::system::delete_file(_file_chunks->tmp_file_name_);
                fire_chunks_callback(loader_errors::cancelled, _url);
                return;
            }

            suspended_tasks_.push([_url, _file_chunks, this](const wim_packet_params& wim_params)
            {
                download_file_sharing_impl(_url, wim_params, _file_chunks);
            });
        }
    });
}

void core::wim::async_loader::update_file_chunks(downloadable_file_chunks& _file_chunks, priority_t _new_priority, file_info_handler_t _additional_handlers)
{
    _file_chunks.handlers_.push_back(_additional_handlers);
    if (_file_chunks.priority_ > _new_priority)
    {
        _file_chunks.priority_on_start_ = _new_priority;
        _file_chunks.priority_ = _new_priority;
    }
}

void core::wim::async_loader::fire_chunks_callback(loader_errors _error, const std::string& _url)
{
    std::wstring file_name;

    downloadable_file_chunks_ptr file_chunks;

    {
        std::lock_guard<std::mutex> lock(in_progress_mutex_);
        auto it = in_progress_.find(_url);
        assert(it != in_progress_.end());
        if (it != in_progress_.end())
        {
            file_chunks = it->second;
            in_progress_.erase(it);
        }
    }

    if (file_chunks)
    {
        auto data = file_info_data_t(std::make_shared<downloaded_file_info>(_url, file_chunks->file_name_));

        for (auto& handler : file_chunks->handlers_)
        {
            if (handler.completion_callback_)
            {
                g_core->execute_core_context([=]()
                {
                    handler.completion_callback_(_error, data);
                });
            }
        }
    }
}

void core::wim::async_loader::cleanup_cache()
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

    boost::filesystem::directory_iterator end_iter;

    boost::system::error_code error;


    if (boost::filesystem::exists(content_cache_dir_, error) && boost::filesystem::is_directory(content_cache_dir_, error))
    {
        for (boost::filesystem::directory_iterator dir_iter(content_cache_dir_) ; ((dir_iter != end_iter) && (files_to_delete.size() < max_files_to_delete)); ++dir_iter)
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
