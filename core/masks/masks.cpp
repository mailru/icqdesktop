#include "stdafx.h"

#include "../async_task.h"
#include "../core.h"
#include "../../corelib/collection_helper.h"

#include "../log/log.h"

#include "../tools/strings.h"
#include "../tools/system.h"

#include "../connections/wim/async_loader/async_loader.h"
#include "../connections/wim/wim_im.h"
#include "../connections/wim/wim_packet.h"
#include "../connections/wim/loader/loader.h"

#include "masks.h"

core::masks::masks(std::weak_ptr<wim::im> _im, const std::wstring& _root_path, unsigned _version)
    : im_(_im)
    , root_(_root_path)
    , version_(_version)
    , state_(state::not_loaded)
{
    init();
}

void core::masks::init()
{
    if (!tools::system::is_exist(root_))
    {
        __INFO("masks", "creating of the root directory: %1%", root_);
        if (!tools::system::create_directory(root_))
        {
            __WARN("masks", "can't create the root directory: %1%", root_);
            return;
        }
    }
    else
    {
        for(auto& entry : boost::make_iterator_range(
            boost::filesystem::directory_iterator(root_), boost::filesystem::directory_iterator()))
        {
            const auto n = tools::to_uint64(entry.path().filename().wstring());
            __INFO("masks", "removing the old version: %1%", entry.path());
            if (n != version_)
            {
                boost::system::error_code ec;
                boost::filesystem::remove_all(entry, ec);
            }
        }
    }

    const auto dir = get_working_dir();
    __INFO("masks", "creating of the working directory: %1%", dir);
    tools::system::create_directory_if_not_exists(dir);
}

boost::filesystem::path core::masks::get_working_dir() const
{
    return root_ / std::to_string(version_);
}

boost::filesystem::path core::masks::get_json_path() const
{
    return get_working_dir() / ".info";
}

boost::filesystem::path core::masks::get_mask_dir(const std::string& _name) const
{
    return get_working_dir() / _name;
}

boost::filesystem::path core::masks::get_mask_version_path(const std::string& _name) const
{
    return get_mask_dir(_name) / ".version";
}

boost::filesystem::path core::masks::get_mask_preview_path(const std::string& _name, const mask_info& _info) const
{
    const auto file_name = boost::filesystem::path(_info.preview_).filename();
    return get_mask_dir(_name) / file_name;
}

boost::filesystem::path core::masks::get_mask_archive_path(const std::string& _name, const mask_info& _info) const
{
    const auto file_name = boost::filesystem::path(_info.archive_).filename();
    return get_mask_dir(_name) / file_name;
}

boost::filesystem::path core::masks::get_mask_content_dir(const std::string& _name) const
{
    return get_mask_dir(_name) / "content";
}

boost::filesystem::path core::masks::get_mask_json_path(const std::string& _name) const
{
    return get_mask_content_dir(_name) / "mask.json";
}

boost::filesystem::path core::masks::get_model_dir() const
{
    return get_working_dir() / ".model";
}

boost::filesystem::path core::masks::get_model_version_path() const
{
    return get_model_dir() / ".version";
}

boost::filesystem::path core::masks::get_model_sentry_path() const
{
    return get_model_dir() / ".done";
}

boost::filesystem::path core::masks::get_model_archive_path() const
{
    return get_working_dir() / ".model.zip";
}

bool core::masks::save_etag(const boost::filesystem::path& _path, const std::string& _etag)
{
    __INFO("masks", "saving etag: %1% : %2%", _path % _etag);

    auto file = std::ofstream(_path.string());
    if (!file)
        return false;

    file << _etag;
    return true;
}

void core::masks::post_message_to_gui(int64_t _seq, const std::string& _message) const
{
    coll_helper coll(g_core->create_collection(), true);
    g_core->post_message_to_gui(_message.c_str(), _seq, coll.get());
}

void core::masks::post_message_to_gui(int64_t _seq, const std::string& _message, const boost::filesystem::path& _local_path) const
{
    coll_helper coll(g_core->create_collection(), true);
    coll.set_value_as_string("local_path", tools::from_utf16(_local_path.wstring()));
    g_core->post_message_to_gui(_message.c_str(), _seq, coll.get());
}

void core::masks::get_mask_id_list(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    state_ = state::loading;

    if (im->has_created_call())
    {
        post_message_to_gui(_seq, "masks/update/retry");
        return;
    }

    __INFO("masks", "getting the masks list %1%", _seq);

    const std::string json_url = "https://www.icq.com/masks/list/v" + std::to_string(version_);
    const auto json_path = get_json_path();

    __INFO("masks", "downloading: %1%", json_url);

    im->get_async_loader().download_file(low_priority, json_url, json_path.wstring(), im->make_wim_params(), wim::file_info_handler_t(
        [this, im, _seq, json_path](loader_errors _error, const wim::file_info_data_t& _data)
        {
            if (_error != loader_errors::success)
            {
                __INFO("masks", "downloading error: %1%", static_cast<int>(_error));
                state_ = state::not_loaded;
                return;
            }

            __INFO("masks", "the downloaded file: %1%", json_path);

            if (im->has_created_call())
            {
                post_message_to_gui(_seq, "masks/update/retry");
                return;
            }

            auto source = _data.get_content();
            source->write('\0');

            rapidjson::Document json;
            if (json.Parse(source->get_data()).HasParseError())
            {
                __WARN("masks", "can't parse the downloaded file %1%", json_path);
                return;
            }

            mask_by_name_.clear();
            mask_list_.clear();

            base_url_ = json["base_url"].GetString();

            const auto& masks = json["masks"];
            for (auto i = 0u, size = masks.Size(); i != size; ++i)
            {
                const auto& elem = masks[i];

                const auto name = elem["name"].GetString();
                const auto etag = elem["etag"].GetString();

                mask_by_name_[name] = mask_list_.size();
                mask_list_.emplace_back(
                    name, elem["archive"].GetString(), elem["image"].GetString(), etag);

                const auto mask_dir = get_mask_dir(name);
                if (!tools::system::is_exist(mask_dir))
                {
                    __INFO("masks", "creating of the mask directory: %1%", mask_dir);

                    if (tools::system::create_directory(mask_dir))
                    {
                        save_etag(get_mask_version_path(name), etag);
                    }

                    __WARN("masks", "can't create the mask directory: %1%", mask_dir);
                }
                else
                {
                    __INFO("masks", "checking the mask version: %1%", mask_dir);

                    std::string prev_etag;
                    if (tools::system::read_file(get_mask_version_path(name), prev_etag))
                    {
                        if (prev_etag != etag)
                        {
                            __INFO("masks", "deleting the old mask: %1%", mask_dir);
                            tools::system::clean_directory(mask_dir);
                            save_etag(get_mask_version_path(name), etag);
                        }
                    }
                }
            }

            coll_helper coll(g_core->create_collection(), true);
            core::ifptr<core::iarray> arr(coll->create_array());

            for (const auto& mask : mask_list_)
            {
                core::ifptr<core::ivalue> val(coll->create_value());
                const auto& name = mask.name_;
                val->set_as_string(name.c_str(), name.length());
                arr->push_back(val.get());
            }

            __INFO("masks", "sending the masks list %1%", _seq);

            coll.set_value_as_array("mask_id_list", arr.get());
            g_core->post_message_to_gui("masks/get_id_list/result", _seq, coll.get());

            const auto& model = json["model"];
            const auto etag = model["etag"].GetString();
            const auto model_dir = get_model_dir();

            __INFO("masks", "preparing to download the mask model %1%", model_dir);

            if (!tools::system::is_exist(model_dir))
            {
                if (tools::system::create_directory(model_dir))
                {
                    save_etag(get_model_version_path(), etag);
                }
            }
            else
            {
                const auto model_versio_path = get_model_version_path();

                __INFO("masks", "check the model version: %1%", model_versio_path);

                std::string prev_etag;
                if (tools::system::read_file(model_versio_path, prev_etag))
                {
                    if (prev_etag != etag)
                    {
                        __INFO("masks", "deleting the old model: %1%", model_dir);
                        tools::system::clean_directory(model_dir);
                        save_etag(get_model_version_path(), etag);
                    }
                }
            }

            model_url_ = base_url_ + model["archive"].GetString();
        }));
}

void core::masks::get_mask_preview(int64_t _seq, const std::string& mask_id)
{
    auto im = im_.lock();
    if (!im)
        return;

    __INFO("masks", "getting the mask preview: %1%", mask_id);

    const auto mask_pos = mask_by_name_.find(mask_id);
    if (mask_pos == mask_by_name_.end())
    {
        assert(!"call get_mask_id_list first!");
        return;
    }

    const auto& mask = mask_list_[mask_pos->second];
    const auto preview_path = get_mask_preview_path(mask_id, mask);
    if (tools::system::is_exist(preview_path))
    {
        __INFO("masks", "the preview is already loaded: %1%", preview_path);
        post_message_to_gui(_seq, "masks/preview/result", preview_path);
    }
    else
    {
        const std::string preview_url = base_url_ + mask.preview_;

        __INFO("masks", "downloading the preview: %1%", preview_url);

        im->get_async_loader().download_file(low_priority, preview_url, preview_path.wstring(), im->make_wim_params(), wim::file_info_handler_t(
            [this, _seq, preview_path](loader_errors _error, const wim::file_info_data_t& _data)
            {
                if (_error != loader_errors::success)
                {
                    __WARN("masks", "preview downloading error: %1%", static_cast<int>(_error));
                    return;
                }

                __INFO("masks", "preview downloading complete %1%", preview_path);

                post_message_to_gui(_seq, "masks/preview/result", preview_path);
            }));
    }
}

void core::masks::get_mask_model(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    if (!tools::system::is_exist(get_model_sentry_path()))
    {
        __INFO("masks", "downloading the model: %1%", model_url_);

        im->get_async_loader().download_file(low_priority, model_url_, get_model_archive_path().wstring(), im->make_wim_params(), wim::file_info_handler_t(
            [this, _seq](loader_errors _error, const wim::file_info_data_t& _data)
            {
                if (_error != loader_errors::success)
                {
                    __WARN("masks", "model downloading error: %1%", static_cast<int>(_error));
                    return;
                }

                const auto model_archive_path = get_model_archive_path();
                const auto model_dir = get_model_dir();
                if (!tools::system::unzip(model_archive_path, model_dir))
                {
                    __WARN("masks", "unzip error: %1% -> %2%", model_archive_path % model_dir);
                    return;
                }

                tools::system::delete_file(get_model_archive_path().wstring());

                std::ofstream sentry_file(get_model_sentry_path().string());

                on_model_loading(_seq);
            }));
    }
    else
    {
        on_model_loading(_seq);
    }
}

void core::masks::get_mask(int64_t _seq, const std::string& mask_id)
{
    auto im = im_.lock();
    if (!im)
        return;

    __INFO("masks", "getting mask: %1%", mask_id);

    const auto mask_pos = mask_by_name_.find(mask_id);
    if (mask_pos == mask_by_name_.end())
    {
        assert(!"call get_mask_id_list first!");
        return;
    }

    const auto json_path = get_mask_json_path(mask_id);
    if (tools::system::is_exist(json_path))
    {
        __INFO("masks", "sending mask %1%", mask_id);
        post_message_to_gui(_seq, "masks/get/result", json_path);
    }
    else
    {
        const auto& mask = mask_list_[mask_pos->second];
        const std::string archive_url = base_url_ + mask.archive_;
        const auto archive_path = get_mask_archive_path(mask_id, mask);

        __INFO("masks", "downloading the mask %1%", mask_id);

        im->get_async_loader().download_file(low_priority, archive_url, archive_path.wstring(), im->make_wim_params(), wim::file_info_handler_t(
            [this, _seq, mask_id, archive_path, json_path](loader_errors _error, const wim::file_info_data_t& _data)
            {
                if (_error != loader_errors::success)
                {
                    __WARN("masks", "mask downloading error: %1%", static_cast<int>(_error));
                    return;
                }

                const auto content_dir = get_mask_content_dir(mask_id);
                if (!tools::system::unzip(archive_path, content_dir))
                {
                    __WARN("masks", "unzip error: %1% -> %2%", archive_path % content_dir);
                    return;
                }

                tools::system::delete_file(archive_path.wstring());

                __INFO("masks", "sending the mask %1%", mask_id);

                post_message_to_gui(_seq, "masks/get/result", json_path);

            },
            [_seq](int64_t /*_bytes_total*/, int64_t /*_bytes_transferred*/, int32_t _percent)
            {
                coll_helper coll(g_core->create_collection(), true);
                coll.set_value_as_uint("percent", _percent);
                g_core->post_message_to_gui("masks/progress", _seq, coll.get());
            }));
    }
}

void core::masks::on_model_loading(int64_t _seq)
{
    auto im = im_.lock();
    if (!im)
        return;

    const auto path = tools::from_utf16(get_model_dir().wstring());
    im->voip_set_model_path(path);

    coll_helper coll(g_core->create_collection(), true);
    g_core->post_message_to_gui("masks/model/result", _seq, coll.get());

    state_ = state::loaded;
}

void core::masks::on_connection_restored()
{
#ifndef __linux__
    if (state_ == state::not_loaded)
        post_message_to_gui(0, "masks/update/retry");
#endif //__linux__
}

core::masks::mask_info::mask_info()
    : downloaded_(false)
{
}

core::masks::mask_info::mask_info(
    const std::string& _name, const std::string& _archive, const std::string& _preview, const std::string& _etag)
    : name_(_name)
    , downloaded_(false)
    , archive_(_archive)
    , preview_(_preview)
    , etag_(_etag)
{
}
