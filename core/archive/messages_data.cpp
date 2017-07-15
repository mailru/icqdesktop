#include "stdafx.h"
#include "messages_data.h"
#include "storage.h"
#include "archive_index.h"
#include "../tools/system.h"
#include "../../common.shared/common_defs.h"

using namespace core;
using namespace archive;

messages_data::messages_data(const std::wstring& _file_name)
    :	storage_(new storage(_file_name))
{
}


messages_data::~messages_data()
{
}


bool messages_data::get_messages(headers_list& _headers, history_block& _messages) const
{
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.read_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return false;
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    _messages.reserve(_headers.size());

    bool res = true;

    core::tools::binary_stream message_data;

    for (auto iter_header = _headers.begin(); iter_header != _headers.end(); iter_header++)
    {
        message_data.reset();

        const auto &header = *iter_header;
        assert(!header.is_patch());

        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid message data");
            res = false;
            continue;
        }

        auto msg = std::make_shared<history_message>();
        if (msg->unserialize(message_data) != 0)
        {
            assert(!"unserialize message error");
            continue;
        }

        if (msg->get_msgid() != header.get_id())
        {
            assert(!"message data invalid");
            continue;
        }

        msg->apply_header_flags(header);

        const auto modifications = get_message_modifications(header);
        msg->apply_modifications(modifications);

        _messages.push_back(msg);
    }

    return res;
}

bool is_equal(const char* _str1, const char* _str2, int _b, int _l)
{
    return std::memcmp(_str1, _str2 + _b, _l) == 0;
}

int32_t kmp_strstr(const char* _str, uint32_t _str_sz, const std::vector<int32_t>& _term
               , const std::vector<int32_t>& _prefix, const std::string& _symbs, const std::vector<int32_t>& _symb_indexes)
{
    if (!_str)
        return -1;

    auto j = 0u;

    for (auto i = 0u; i < _str_sz; )
    {
        auto len = tools::utf8_char_size(*(_str + i));

        while (j > 0 && (!is_equal(_symbs.c_str() + _symb_indexes[2 * j], _str, i, len) && !is_equal(_symbs.c_str() + _symb_indexes[2 * j + 1], _str, i, len)))
            j = _prefix[j - 1];

        if (is_equal(_symbs.c_str() + _symb_indexes[2 * j], _str, i, len) || is_equal(_symbs.c_str() + _symb_indexes[2 * j + 1], _str, i, len))
            j += 1;

        if (j == _term.size())
        {
            return i - j + 1;
        }
        i += len;
    }
    return -1;
}

bool messages_data::get_history_archive(const std::wstring& _file_name, core::tools::binary_stream& _buffer
    , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode)
{
    auto file = tools::system::open_file_for_read(_file_name, std::ios::binary | std::ios::ate);

    auto init_size = static_cast<int64_t>(file.tellg());
    std::streamsize size = std::min<int64_t>(*_remaining_size, init_size - *_offset);

    if (size <= 0 || init_size == -1)
    {
        *_offset = -1;
        return false;
    }

    if (*_mode == 0)
    {
        auto limit = 1024 * 1024 * 10;

        if (init_size > limit  && *_offset + limit < init_size)
        {
            *_offset = init_size - limit;
        }
        else
        {
            *_mode = 1;
        }
    }

    file.seekg(*_offset, std::ios::beg);

    if (!file.read(_buffer.get_data_for_write() + _cur_index, size))
    {
        return false;
    }

    *_remaining_size -= size;
    _cur_index += size;

    if (size == init_size - *_offset )
    {
        *_offset = -1;
    }

    return true;
}

void messages_data::search_in_archive(std::shared_ptr<contact_and_offsets> _contacts_and_offsets, std::shared_ptr<coded_term> _cterm
                , std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data
                , std::vector<std::shared_ptr<::core::archive::searched_msg>>& messages_ids
                , int64_t _min_id)
{
    std::set<int64_t, std::greater<int64_t>> top_ids;

    for (auto contact_i = 0u; contact_i < _archive->size() - 1; ++contact_i)
    {
        auto current_pos = (*_archive)[contact_i].second;
        auto end_pos = (*_archive)[contact_i + 1].second;
        auto _offset = (*_contacts_and_offsets)[contact_i].second;
        auto _contact = (*_archive)[contact_i].first;

        int64_t begin_of_block;

        while (storage::fast_read_data_block((*_data), current_pos, begin_of_block, end_pos))
        {
            _data->set_output(begin_of_block);
            auto mess_id = history_message::get_id_field(*_data);

            if ((mess_id != -1 && top_ids.count(mess_id) != 0) || mess_id == -1 || mess_id <= _min_id)
            {
                continue;
            }

            if (top_ids.size() > ::common::get_limit_search_results())
            {
                auto greater = top_ids.upper_bound(mess_id);

                if (greater != top_ids.end())
                {
                    auto min_id = (*top_ids.rbegin());
                    top_ids.erase(min_id);
                    top_ids.insert(mess_id);
                }
                else
                {
                    continue;
                }
            }

            uint32_t text_length = 0;

            _data->set_output(begin_of_block);

            if (history_message::is_sticker(*_data))
            {
                continue;
            }

            _data->set_output(begin_of_block);

            history_message::jump_to_text_field(*_data, text_length);

            if (!text_length)
            {
                continue;
            }

            char* pointer = nullptr;
            if (_data->available())
            {
                pointer = _data->read_available();
            }

            if (kmp_strstr(pointer, text_length, _cterm->coded_string, _cterm->prefix, _cterm->symbs, _cterm->symb_indexes) != -1)
            {
                top_ids.insert(mess_id);

                std::shared_ptr<searched_msg> search_msg(new searched_msg());
                search_msg->contact = _contact;
                search_msg->id = mess_id;
                search_msg->term = _cterm->lower_term;
                search_msg->contact = _contact;
                messages_ids.push_back(search_msg);
            }
        }

        if (current_pos == (*_archive)[contact_i].second && (*_archive)[contact_i].first != "")
        {
            *_offset = -1;
        }

        if (*_offset != -1)
            *_offset += current_pos - (*_archive)[contact_i].second;
    }
}

history_block messages_data::get_message_modifications(const message_header& _header) const
{
    if (!_header.is_modified())
    {
        return history_block();
    }

    history_block modifications;

    core::tools::binary_stream message_data;

    const auto &modification_headers = _header.get_modifications();
    for (const auto &header : modification_headers)
    {
        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid modification data");
            continue;
        }

        auto modification = std::make_shared<history_message>();
        if (modification->unserialize(message_data) != 0)
        {
            assert(!"unserialize modification error");
            continue;
        }

        if (modification->get_msgid() != header.get_id())
        {
            assert(!"modification data invalid");
            continue;
        }

        modifications.push_back(modification);
    }

    return modifications;
}

bool messages_data::update(const archive::history_block& _data)
{
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.write_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return false;
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream message_data;

    for (auto iter_hm = _data.begin(); iter_hm != _data.end(); ++iter_hm)
    {
        auto msg = *iter_hm;

        message_data.reset();
        msg->serialize(message_data);

        int64_t offset = 0;
        if (!storage_->write_data_block(message_data, offset))
            return false;

        msg->set_data_offset(offset);
        msg->set_data_size(message_data.available());
    }

    return true;
}
