#include "stdafx.h"

#include "archive_index.h"
#include "storage.h"
#include "options.h"

using namespace core;
using namespace archive;

namespace
{
    void skip_patches_forward(const headers_map &_headers, headers_map::const_reverse_iterator &_iter);
    void skip_patches_forward(headers_map &_headers, headers_map::reverse_iterator &_iter);
}

//////////////////////////////////////////////////////////////////////////
// archive_index class
//////////////////////////////////////////////////////////////////////////

const int32_t max_index_size			= 25000;
const int32_t index_size_need_optimize	= 30000;

enum archive_index_types : uint32_t
{
    header		= 1,
    msgid		= 2,
    msgflags	= 3,
};

archive_index::archive_index(const std::wstring& _file_name)
    :	storage_(new storage(_file_name)),
    last_error_(archive::error::ok)
{
}


archive_index::~archive_index()
{
}

void archive_index::serialize(headers_list& _list) const
{
    for (auto iter_header = headers_index_.begin(); iter_header != headers_index_.end(); iter_header++)
        _list.emplace_back(iter_header->second);
}

bool archive_index::serialize_from(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _list) const
{
    bool _to_older = _count_early > 0;
    int _count = 30;

    if (headers_index_.empty())
        return true;

    headers_map::const_iterator iter_header;

    if (_from == -1)
    {
        iter_header = headers_index_.end();
    }
    else
    {
        iter_header = headers_index_.lower_bound(_from);
        if (iter_header == headers_index_.end())
        {
            assert(!"invalid index number");
            return false;
        }
    }

    if (_to_older)
    {
        while (iter_header != headers_index_.begin())
        {
            --iter_header;

            if (_count != -1 && _list.size() >= _count)
                return true;

            _list.emplace_front(iter_header->second);
        };
    }
    else
    {
        while (iter_header != headers_index_.end())
        {
            _list.emplace_back(iter_header->second);
            ++iter_header;

            if (_list.size() >= _count)
                break;
        };
    }

    return true;

    //if (headers_index_.empty())
    //    return true;

    //headers_map::const_iterator iter_header;

    //if (_from == -1)
    //{
    //    iter_header = headers_index_.end();
    //}
    //else
    //{
    //    iter_header = headers_index_.lower_bound(_from);
    //    if (iter_header == headers_index_.end())
    //    {
    //        assert(!"invalid index number");
    //        return false;
    //    }
    //}
    //
    //if (_count_early > 0)
    //{
    //    auto it_before = iter_header;
    //    while (it_before != headers_index_.begin())
    //    {
    //        --it_before;
    //        
    //        if (_count_early-- == 0)
    //            break;

    //        _list.emplace_front(it_before->second);
    //    }
    //}

    //if (_count_later > 0)
    //{
    //    auto it_after = iter_header;
    //    while (it_after != headers_index_.end())
    //    {
    //        _list.emplace_back(it_after->second);

    //        ++it_after;

    //        if (--_count_later == 0)
    //            break;
    //    };
    //}

    //std::stringstream str;
    //str << "SERIAL" << _from << "\n";
    //for (auto val : _list)
    //{
    //    str << "  id:" << val.get_id() << "\n";
    //}
    //OutputDebugStringA(str.str().data());

    //return true;
}

void archive_index::insert_block(const archive::headers_list& _inserted_headers)
{
    for (const auto &header : _inserted_headers)
    {
        insert_header(header);
    }
}

void archive_index::insert_header(const archive::message_header& header)
{
    const auto msg_id = header.get_id();
    assert(msg_id > 0);

    const auto existing_iter = headers_index_.find(msg_id);

    if (existing_iter == headers_index_.end())
    {
        headers_index_.emplace_hint(
            headers_index_.end(),
            std::make_pair(msg_id, header)
            );

        return;
    }

    auto &existing_header = existing_iter->second;

    existing_header.merge_with(header);
}

bool archive_index::get_header(int64_t _msgid, message_header& _header) const
{
    auto iter_header = headers_index_.find(_msgid);
    if (iter_header == headers_index_.end())
        return false;

    _header = iter_header->second;

    return true;
}

bool archive_index::has_header(const int64_t _msgid) const
{
    auto iter_header = headers_index_.find(_msgid);
    return (iter_header != headers_index_.end());
}

bool archive_index::update(const archive::history_block& _data, /*out*/ headers_list& _headers)
{
    _headers.clear();

    for (auto iter_hm = _data.begin(); iter_hm != _data.end(); iter_hm++)
    {
        _headers.emplace_back(
            (*iter_hm)->get_flags(),
            (*iter_hm)->get_time(),
            (*iter_hm)->get_msgid(),
            (*iter_hm)->get_prev_msgid(),
            (*iter_hm)->get_data_offset(),
            (*iter_hm)->get_data_size()
            );
    }

    insert_block(_headers);

    return save_block(_headers);
}

void archive_index::serialize_block(const headers_list& _headers, core::tools::binary_stream& _data) const
{
    core::tools::tlvpack tlv_headers;

    core::tools::binary_stream header_data;

    for (auto iter_header = _headers.begin(); iter_header != _headers.end(); iter_header++)
    {
        header_data.reset();
        iter_header->serialize(header_data);

        tlv_headers.push_child(core::tools::tlv(archive_index_types::header, header_data));
    }

    tlv_headers.serialize(_data);
}

bool archive_index::unserialize_block(core::tools::binary_stream& _data)
{
    uint32_t tlv_type = 0;
    uint32_t tlv_length = 0;
    archive::message_header msg_header;

    while (uint32_t available = _data.available())
    {
        if (available < sizeof(uint32_t)*2)
            return false;

        tlv_type = _data.read<uint32_t>();
        tlv_length = _data.read<uint32_t>();

        if (available < tlv_length)
            return false;

        if (!msg_header.unserialize(_data))
            return false;

        insert_header(msg_header);
    }

    return true;
}

bool archive_index::save_block(const archive::headers_list& _block)
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return false;

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream block_data;
    serialize_block(_block, block_data);

    int64_t offset = 0;
    return storage_->write_data_block(block_data, offset);
}

bool archive_index::save_all()
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage_->open(mode))
        return false;

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    if (headers_index_.empty())
        return true;

    std::list<message_header> headers;

    auto iter_last = headers_index_.end();
    iter_last--;

    for (auto iter_header = headers_index_.begin(); iter_header != headers_index_.end(); iter_header++)
    {
        headers.emplace_back(iter_header->second);

        if (headers.size() >= history_block_size || iter_header == iter_last)
        {
            core::tools::binary_stream block_data;
            serialize_block(headers, block_data);

            int64_t offset = 0;
            if (!storage_->write_data_block(block_data, offset))
                return false;

            headers.clear();
        }
    }

    return true;
}

bool archive_index::load_from_local()
{
    last_error_ = archive::error::ok;

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage_->open(mode))
    {
        last_error_ = storage_->get_last_error();
        return false;
    }

    auto p_storage = storage_.get();
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream data_stream;
    while (storage_->read_data_block(-1, data_stream))
    {
        if (!unserialize_block(data_stream))
            return false;

        data_stream.reset();
    }

    if (storage_->get_last_error() != archive::error::end_of_file)
    {
        last_error_ = storage_->get_last_error();
        return false;
    }

    return true;
}

void archive_index::delete_up_to(const int64_t _to)
{
    assert(_to > -1);

    auto iter = headers_index_.lower_bound(_to);

    const auto delete_all = (iter == headers_index_.end());
    if (delete_all)
    {
        headers_index_.clear();

        save_all();

        return;
    }

    auto &header = iter->second;

    const auto is_del_up_to_found = (header.get_id() == _to);

    if (is_del_up_to_found)
    {
        auto iter_after_deleted = iter;
        ++iter_after_deleted;

        headers_index_.erase(headers_index_.begin(), iter_after_deleted);

        if (iter_after_deleted != headers_index_.end())
        {
            auto &header_after_deleted = iter_after_deleted->second;
            if (header_after_deleted.get_prev_msgid() == _to)
            {
                header_after_deleted.set_prev_msgid(-1);
            }
        }

        save_all();
    }
    else
    {
        if (headers_index_.begin() != iter)
        {
            headers_index_.erase(headers_index_.begin(), iter);

            save_all();
        }
    }

}

int64_t archive_index::get_last_msgid()
{
    if (headers_index_.empty())
        return -1;

    return headers_index_.rbegin()->first;
}

bool archive_index::get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const
{
    if (headers_index_.empty())
        return true;

    int64_t current_depth = 0;

    // 1. position search cursor

    auto iter_cursor = headers_index_.crbegin();

    const auto is_from_specified = (_from != -1);
    if (is_from_specified)
    {
        const auto last_header_key = iter_cursor->first;

        const auto is_hole_at_the_end = (last_header_key < _from);
        if (is_hole_at_the_end)
        {
            // if "from" from dlg_state (still not in index obviously)

            assert(iter_cursor->second.get_id() == last_header_key);

            _hole.set_from(-1);
            _hole.set_to(last_header_key);

            return true;
        }

        auto from_iter = headers_index_.find(_from);
        if (from_iter == headers_index_.cend())
        {
            assert(!"index not found");
            return false;
        }

        iter_cursor = headers_map::const_reverse_iterator(++from_iter);
    }

    skip_patches_forward(headers_index_, iter_cursor);

    const auto only_patches_in_index = (iter_cursor == headers_index_.crend());
    if (only_patches_in_index)
        return true;

    // 2. search for holes

    while (iter_cursor != headers_index_.crend())
    {
        current_depth++;

        const auto &current_header = iter_cursor->second;
        assert(!current_header.is_patch());

        auto iter_next = iter_cursor;
        ++iter_next;
        skip_patches_forward(headers_index_, iter_next);

        const auto reached_last_header = (iter_next == headers_index_.crend());
        if (reached_last_header)
        {
            if (current_header.get_prev_msgid() != -1)
            {
                _hole.set_from(current_header.get_id());
                return true;
            }

            return false;
        }

        const auto &prev_header = iter_next->second;
        assert(!prev_header.is_patch());

        if (current_header.get_prev_msgid() != prev_header.get_id())
        {
            _hole.set_from(current_header.get_id());
            _hole.set_to(prev_header.get_id());
            _hole.set_depth(current_depth);

            return true;
        }

        if (_depth != -1 && current_depth >= _depth)
            return false;

        ++iter_cursor;
        skip_patches_forward(headers_index_, iter_cursor);
    }

    return false;
}

int64_t archive_index::validate_hole_request(const archive_hole& _hole, const int32_t _count)
{
    int64_t ret_from = _hole.get_from();

    do 
    {
        if (_hole.get_from() <= 0 || _hole.get_to() <= 0 || abs(_count) <= 1)
            break;

        auto from_iter = headers_index_.find(_hole.get_from());
        if (from_iter == headers_index_.cend())
            break;

        auto iter_cursor = headers_map::reverse_iterator(++from_iter);
        if (iter_cursor->second.is_patch())
            break;

        auto iter_prev = iter_cursor;
        ++iter_prev;

        skip_patches_forward(headers_index_, iter_prev);

        if (iter_prev == headers_index_.rend())
            break;

        if (iter_prev->first != iter_cursor->second.get_prev_msgid())
        {
            ret_from = iter_prev->first;
        }
    }
    while (false);

    return ret_from;
}

bool archive_index::need_optimize()
{
    return (headers_index_.size() > index_size_need_optimize);
}

void archive_index::optimize()
{
    if (!need_optimize())
        return;

    if (headers_index_.size() > index_size_need_optimize)
    {
        auto iter = headers_index_.end();
        for (int32_t i = 0; i < max_index_size && iter != headers_index_.begin(); i++)
        {
            --iter;
        }

        headers_index_.erase(headers_index_.begin(), iter);

        assert(!headers_index_.empty());
        if (!headers_index_.empty())
            headers_index_.begin()->second.set_prev_msgid(-1);

        save_all();
    }
}

namespace
{
    void skip_patches_forward(const headers_map& _headers, headers_map::const_reverse_iterator &_iter)
    {
        for (; _iter != _headers.crend(); ++_iter)
        {
            const auto &header = _iter->second;

            if (!header.is_patch())
            {
                break;
            }
        }
    }

    void skip_patches_forward(headers_map& _headers, headers_map::reverse_iterator &_iter)
    {
        for (; _iter != _headers.crend(); ++_iter)
        {
            const auto &header = _iter->second;

            if (!header.is_patch())
            {
                break;
            }
        }
    }
}