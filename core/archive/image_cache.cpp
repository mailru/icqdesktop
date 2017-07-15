#include "stdafx.h"

#include "../../common.shared/url_parser/url_parser.h"

#include "../tools/file_sharing.h"
#include "../tools/system.h"

#include "archive_index.h"
#include "contact_archive.h"
#include "history_message.h"
#include "messages_data.h"
#include "storage.h"

#include "image_cache.h"

namespace
{
    const std::wstring tmp_to_add_extension = L".a.tmp";
    const std::wstring tmp_to_delete_extension = L".d.tmp";

    const int32_t max_block_size        = 1000;

    const int32_t fetch_size            = 30;

    enum tlv_fields : uint32_t
    {
        tlv_img_pack                = 1,

        tlv_msg_id                  = 2,
        tlv_msg_url                 = 3,
        tlv_msg_is_filesharing      = 4
    };
}

core::archive::image_data::image_data()
    : msgid_(0)
    , is_filesharing_(false)
{
}

core::archive::image_data::image_data(int64_t _msgid)
    : msgid_(_msgid)
    , is_filesharing_(false)
{
}

core::archive::image_data::image_data(int64_t _msgid, std::string _url, bool _is_filesharing)
    : msgid_(_msgid)
    , url_(_url)
    , is_filesharing_(_is_filesharing)
{
}

int64_t core::archive::image_data::get_msgid() const
{
    return msgid_;
}

void core::archive::image_data::set_msgid(int64_t _value)
{
    msgid_ = _value;
}

std::string core::archive::image_data::get_url() const
{
    return url_;
}

void core::archive::image_data::set_url(const std::string& _value)
{
    url_ = _value;
}

bool core::archive::image_data::get_is_filesharing() const
{
    return is_filesharing_;
}

void core::archive::image_data::set_is_filesharing(bool _value)
{
    is_filesharing_ = _value;
}

void core::archive::image_data::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack msg_pack;

    msg_pack.push_child(core::tools::tlv(tlv_fields::tlv_msg_id, (int64_t) msgid_));
    msg_pack.push_child(core::tools::tlv(tlv_fields::tlv_msg_url, (std::string) url_));
    msg_pack.push_child(core::tools::tlv(tlv_fields::tlv_msg_is_filesharing, (bool) is_filesharing_));

    msg_pack.serialize(_data);
}

bool core::archive::image_data::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack msg_pack;

    if (!msg_pack.unserialize(_data))
        return false;

    for (auto tlv_field = msg_pack.get_first(); tlv_field; tlv_field = msg_pack.get_next())
    {
        switch (static_cast<tlv_fields>(tlv_field->get_type()))
        {
        case tlv_fields::tlv_msg_id:
            msgid_ = tlv_field->get_value<int64_t>();
            break;
        case tlv_fields::tlv_msg_url:
            url_ = tlv_field->get_value<std::string>();
            break;
        case tlv_fields::tlv_msg_is_filesharing:
            is_filesharing_ = tlv_field->get_value<bool>();
            break;
        default:
            assert(!"invalid field");
        }
    }

    return true;
}

core::archive::image_cache::image_cache(const std::wstring& _file_name)
    : storage_(new storage(_file_name))
    , tmp_to_add_storage_(new storage(_file_name + tmp_to_add_extension))
    , tmp_to_delete_storage_(new storage(_file_name + tmp_to_delete_extension))
    , building_in_progress_(false)
    , tree_is_consistent_(false)
{
}

core::archive::image_cache::~image_cache()
{
}

bool core::archive::image_cache::load_from_local(const contact_archive& _archive)
{
    const bool success = read_file(*storage_, image_by_msgid_);
    if (success)
    {
        tree_is_consistent_ = update_file_from_tmp_files();
        return tree_is_consistent_;
    }

    // file doesn't exists or corruped so rebuild anyway
    return build(_archive);
}

void core::archive::image_cache::get_images(int64_t _from, int64_t _count, image_list& _images) const
{
    _images.clear();

    images_map_t::const_iterator it;

    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (_from == -1)
        {
            while (building_in_progress_ && image_by_msgid_.empty())
            {
                data_ready_.wait(lock);
            }

            it = image_by_msgid_.end();
        }
        else
        {
            it = image_by_msgid_.lower_bound(_from);
            while (building_in_progress_ && it == image_by_msgid_.begin())
            {
                data_ready_.wait(lock);
                it = image_by_msgid_.lower_bound(_from);
            }

            if (it == image_by_msgid_.end())
            {
                assert(!"invalid index number");
                return;
            }
        }
    }

    for (auto begin = image_by_msgid_.begin(); it != begin; )
    {
        --it;

        if (_count != -1 && _images.size() >= _count)
            return;

        _images.emplace_front(it->second);
    }
}

bool core::archive::image_cache::update(const history_block& _block)
{
    if (!tree_is_consistent_)
    {
        image_vector_t to_add;
        image_vector_t to_delete;

        for (auto it : _block)
        {
            const history_message& message = *it;
            if (message.is_patch())
                to_delete.emplace_back(image_data(message.get_msgid()));
            else
                extract_images(message, to_add);
        }

        const bool add_result = append_to_file(*tmp_to_add_storage_, to_add);
        const bool delete_result = append_to_file(*tmp_to_delete_storage_, to_delete);

        return add_result && delete_result;
    }

    const auto end = image_by_msgid_.end();

    bool need_to_save_all = false;

    image_vector_t to_add;

    for (auto it : _block)
    {
        const history_message& message = *it;
        const auto msgid = message.get_msgid();

        if (message.is_patch())
        {
            auto to_delete = image_by_msgid_.find(msgid);
            if (to_delete != end)
            {
                need_to_save_all = true;
                image_by_msgid_.erase(to_delete);
            }
        }
        else
        {
            extract_images(message, to_add);
        }
    }

    if (need_to_save_all)
    {
        return save_all();
    }
    else if (!to_add.empty())
    {
        archive::storage_mode mode;
        mode.flags_.write_ = true;
        mode.flags_.append_ = true;
        if (!storage_->open(mode))
            return false;

        core::tools::auto_scope lb([this]{ storage_->close(); });

        add_images_to_tree(to_add);

        return serialize_block(*storage_, to_add);
    }

    return true;
}

bool core::archive::image_cache::synchronize(const archive_index& _index)
{
    erase_deleted_from_tree(_index);
    return save_all();
}

bool core::archive::image_cache::build(const contact_archive& _archive)
{
    cancel_build_.clear();

    tree_is_consistent_ = false;
    building_in_progress_ = true;

    image_by_msgid_.clear();

    history_block messages;

    int64_t from = -1;
    while (true)
    {
        if (cancel_build_.test_and_set())
        {
            return false;
        }

        cancel_build_.clear();

        _archive.get_messages(from, fetch_size, -1, messages, contact_archive::get_message_policy::skip_patches_and_deleted);
        if (messages.empty())
        {
            building_in_progress_ = false;
            tree_is_consistent_ = true;

            update_tree_from_tmp_files();

            data_ready_.notify_all();

            if (save_all())
            {
                delete_tmp_files();
                return true;
            }

            return false;
        }

        from = (*messages.begin())->get_msgid();

        const auto images = extract_images(messages);
        if (!images.empty())
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                add_images_to_tree(images);
            }
            data_ready_.notify_all();
        }
    }
}

void core::archive::image_cache::cancel_build()
{
    cancel_build_.test_and_set();
}

bool core::archive::image_cache::serialize_block(storage& _storage, const image_vector_t& _images) const
{
    assert(!_images.empty());

    core::tools::tlvpack block;

    for (auto& img : _images)
    {
        core::tools::binary_stream img_data;
        img.serialize(img_data);
        block.push_child(core::tools::tlv(tlv_fields::tlv_img_pack, img_data));
    }

    core::tools::binary_stream stream;
    block.serialize(stream);

    int64_t offset = 0;
    return _storage.write_data_block(stream, offset);
}

bool core::archive::image_cache::unserialize_block(core::tools::binary_stream& _stream, images_map_t& _images) const
{
    while (_stream.available())
    {
        core::tools::tlvpack block;
        if (!block.unserialize(_stream))
            return false;

        for (auto item = block.get_first(); item; item = block.get_next())
        {
            image_data img;
            auto stream = item->get_value<core::tools::binary_stream>();
            if (!img.unserialize(stream))
                return false;

            _images.emplace(img.get_msgid(), img);
        }
    }

    return true;
}

bool core::archive::image_cache::save_all() const
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;

    if (!storage_->open(mode))
        return false;

    core::tools::auto_scope lb([this]{ storage_->close(); });

    image_vector_t block;
    for (auto& image : image_by_msgid_)
    {
        block.emplace_back(image.second);
        if (block.size() >= max_block_size)
        {
            if (!serialize_block(*storage_, block))
                return false;
            block.clear();
        }
    }

    if (!block.empty())
    {
        return serialize_block(*storage_, block);
    }

    return true;
}

void core::archive::image_cache::extract_images(const history_message& _message, image_vector_t& _images) const
{
    const auto& quote_list = _message.get_quotes();
    if (quote_list.empty())
    {
        extract_images(_message.get_msgid(), _message.get_text(), _images);
    }
    else
    {
        for (const auto& quote : quote_list)
            extract_images(_message.get_msgid(), quote.get_text(), _images);
    }
}

void core::archive::image_cache::extract_images(int64_t _msgid, const std::string& _text, image_vector_t& _images) const
{
    auto urls = common::tools::url_parser::parse_urls(_text);

    for (auto& url_info : urls)
    {
        if (url_info.is_filesharing())
        {
            auto content_type = core::file_sharing_content_type::undefined;

            if (!tools::get_content_type_from_uri(url_info.url_, content_type))
            {
                continue;
            }

            if ((content_type == file_sharing_content_type::image) ||
                (content_type == file_sharing_content_type::gif) ||
                (content_type == file_sharing_content_type::snap_image) ||
                (content_type == file_sharing_content_type::snap_gif ||
                (content_type == file_sharing_content_type::snap_video) ||
                (content_type == file_sharing_content_type::video)))
            {
                _images.emplace_back(_msgid, url_info.url_, url_info.is_filesharing());
            }
        }

        if (url_info.is_image())
        {
            _images.emplace_back(_msgid, url_info.url_, url_info.is_filesharing());
        }
    }
}

core::archive::image_cache::image_vector_t core::archive::image_cache::extract_images(const history_block& _block) const
{
    image_vector_t images;

    for (auto it : _block)
    {
        const history_message& message = *it;
        if (!message.is_patch())
            extract_images(message, images);
    }

    return images;
}

void core::archive::image_cache::add_images_to_tree(const image_vector_t& _images)
{
    for (auto& img : _images)
        image_by_msgid_.emplace(img.get_msgid(), img);
}

void core::archive::image_cache::add_images_to_tree(const images_map_t& _images)
{
    for (auto& img : _images)
        image_by_msgid_.emplace(img.first, img.second);
}

void core::archive::image_cache::erase_deleted_from_tree(const archive_index& _index)
{
    std::vector<int64_t> msg_ids;

    headers_list headers;
    int64_t from = -1;
    while (true)
    {
        _index.serialize_from(from, fetch_size, -1, headers);
        if (headers.empty())
            break;

        from = headers.begin()->get_id();

        for (auto& msg : headers)
            msg_ids.push_back(msg.get_id());

        headers.clear();
    }

    std::vector<int64_t> to_delete;

    int64_t prev = 0;
    for (auto id : image_by_msgid_)
    {
        if (id.first == prev)
            continue;

        prev = id.first;

        if (!std::binary_search(msg_ids.begin(), msg_ids.end(), id.first)) // messages always sorted by design
            to_delete.push_back(id.first);
    }

    for (auto id : to_delete)
        image_by_msgid_.erase(id);
}

bool core::archive::image_cache::read_file(storage& _storage, images_map_t& _images) const
{
    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!_storage.open(mode))
        return false;

    core::tools::auto_scope lb([&_storage]{ _storage.close(); });

    core::tools::binary_stream stream;
    while (_storage.read_data_block(-1, stream))
    {
        if (!unserialize_block(stream, _images))
            return false;

        stream.reset();
    }

    return _storage.get_last_error() == archive::error::end_of_file;
}

bool core::archive::image_cache::append_to_file(storage& _storage, const image_vector_t& _images) const
{
    if (_images.empty())
        return true;

    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.append_ = true;

    if (!_storage.open(mode))
        return false;

    core::tools::auto_scope lb([&_storage]{ _storage.close(); });

    return serialize_block(_storage, _images);
}

bool core::archive::image_cache::update_tree_from_tmp_files()
{
    images_map_t to_add;
    if (!read_file(*tmp_to_add_storage_, to_add))
        return false;

    add_images_to_tree(to_add);

    bool need_to_save_all = false;
    return process_deleted(need_to_save_all);
}

bool core::archive::image_cache::process_deleted(bool& _need_to_save_all)
{
    images_map_t to_delete;
    if (!read_file(*tmp_to_delete_storage_, to_delete))
        return false;

    const auto end = image_by_msgid_.end();
    for (auto& it : to_delete)
    {
        auto to_delete = image_by_msgid_.find(it.second.get_msgid());
        if (to_delete != end)
        {
            _need_to_save_all = true;
            image_by_msgid_.erase(to_delete);
        }
    }

    return true;
}

bool core::archive::image_cache::update_file_from_tmp_files()
{
    const bool to_add_exists = tools::system::is_exist(tmp_to_add_storage_->get_file_name());
    if (to_add_exists)
    {
        images_map_t to_add;
        if (!read_file(*tmp_to_add_storage_, to_add))
            return false;

        image_vector_t tmp;
        for (auto& it : to_add)
            tmp.emplace_back(it.second);

        add_images_to_tree(tmp);

        if (!append_to_file(*storage_, tmp))
            return false;
    }

    const bool to_delete_exists = tools::system::is_exist(tmp_to_delete_storage_->get_file_name());
    if (to_delete_exists)
    {
        bool need_to_save_all = false;
        if (!process_deleted(need_to_save_all))
            return false;

        if (need_to_save_all && !save_all())
            return false;
    }

    delete_tmp_files();

    return true;
}

void core::archive::image_cache::delete_tmp_files()
{
    tools::system::delete_file(tmp_to_add_storage_->get_file_name());
    tools::system::delete_file(tmp_to_delete_storage_->get_file_name());
}
