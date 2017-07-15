#include "stdafx.h"

#include "../../common.shared/url_parser/url_parser.h"

#include "../../corelib/collection_helper.h"

#include "../log/log.h"

#include "image_cache.h"
#include "history_message.h"
#include "contact_archive.h"
#include "archive_index.h"
#include "not_sent_messages.h"
#include "messages_data.h"

#include "local_history.h"

using namespace core;
using namespace archive;

local_history::local_history(const std::wstring& _archive_path)
    :	archive_path_(_archive_path)
{
}


local_history::~local_history()
{
}

std::shared_ptr<contact_archive> local_history::get_contact_archive(const std::string& _contact)
{
    // load contact archive, insert to map

    auto iter_arch = archives_.find(_contact);
    if (iter_arch != archives_.end())
        return iter_arch->second;

    std::wstring contact_folder = core::tools::from_utf8(_contact);
    std::replace(contact_folder.begin(), contact_folder.end(), L'|', L'_');
    auto contact_arch = std::make_shared<contact_archive>(archive_path_ + L"/" + contact_folder, _contact);

    archives_.insert(std::make_pair(_contact, contact_arch));

    return contact_arch;
}

void local_history::update_history(
    const std::string& _contact,
    archive::history_block_sptr _data,
    Out headers_list& _inserted_messages,
    Out dlg_state& _state,
    Out dlg_state_changes& _state_changes)
{
    get_contact_archive(_contact)->insert_history_block(_data, Out _inserted_messages, Out _state, Out _state_changes);
}

void local_history::get_images(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ image_list& _images)
{
    get_contact_archive(_contact)->load_from_local();
    get_contact_archive(_contact)->get_images(_from, _count, _images);
}

bool local_history::repair_images(const std::string& _contact)
{
    return get_contact_archive(_contact)->repair_images();
}

void local_history::get_messages_index(const std::string& _contact, int64_t _from, int64_t _count, /*out*/ headers_list& _headers)
{
    get_contact_archive(_contact)->load_from_local();
    get_contact_archive(_contact)->get_messages_index(_from, _count, -1, _headers);
}

bool local_history::get_messages(const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, /*out*/ std::shared_ptr<history_block> _messages)
{
    headers_list headers;

    auto archive = get_contact_archive(_contact);
    if (!archive)
        return false;

    archive->load_from_local();
    archive->get_messages_index(_from, _count_early, _count_later, headers);

    auto ids_list = std::make_shared<archive::msgids_list>();

    for (auto header : headers)
        ids_list->push_back(header.get_id());

    archive->get_messages_buddies(ids_list, _messages);

    return true;
}

bool local_history::get_history_file(const std::string& _contact, /*out*/ core::tools::binary_stream& _history_archive
    , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode)
{
    std::wstring contact_folder = core::tools::from_utf8(_contact);
    std::replace(contact_folder.begin(), contact_folder.end(), L'|', L'_');
    std::wstring file_name = archive_path_ + L"/" + contact_folder + L"/" + db_filename();

    contact_archive::get_history_file(file_name, _history_archive, _offset, _remaining_size, _cur_index, _mode);
    return true;
}

void local_history::get_messages_buddies(
    const std::string& _contact,
    std::shared_ptr<archive::msgids_list> _ids,
    /*out*/ std::shared_ptr<history_block> _messages)
{
    get_contact_archive(_contact)->get_messages_buddies(_ids, _messages);
}

void local_history::get_dlg_state(const std::string& _contact, dlg_state& _state)
{
        _state = get_contact_archive(_contact)->get_dlg_state();
}

void local_history::get_dlg_states(const std::vector<std::string>& _contacts, std::vector<dlg_state>& _states)
{
    for (auto iter : _contacts)
    {
        auto state = get_contact_archive(iter)->get_dlg_state();
        _states.push_back(state);
    }
}

void local_history::set_dlg_state(const std::string& _contact, const dlg_state& _state, Out dlg_state& _result, Out dlg_state_changes& _changes)
{
    get_contact_archive(_contact)->set_dlg_state(_state, Out _changes);

    Out _result = get_contact_archive(_contact)->get_dlg_state();
}


bool local_history::clear_dlg_state(const std::string& _contact)
{
    get_contact_archive(_contact)->clear_dlg_state();

    return true;
}



std::shared_ptr<archive_hole> local_history::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto hole = std::make_shared<archive_hole>();

    if (get_contact_archive(_contact)->get_next_hole(_from, *hole, _depth))
        return hole;

    return nullptr;
}


int64_t local_history::validate_hole_request(
    const std::string& _contact, 
    const archive_hole& _hole_request, 
    const int32_t _count)
{
    return get_contact_archive(_contact)->validate_hole_request(_hole_request, _count);
}

void local_history::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    const std::string c_headers = "headers";
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (auto iter_header = _headers->begin(); iter_header != _headers->end(); iter_header++)
    {
        std::unique_ptr<hheader> hh(new hheader());
        hh->id_ = iter_header->get_id();
        hh->prev_id_ = iter_header->get_prev_msgid();
        hh->time_ = iter_header->get_time();
        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders(c_headers, val_headers.get());
}


void local_history::serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll)
{
    const std::string c_headers = "headers";
    ifptr<ihheaders_list> val_headers(_coll->create_hheaders_list(), false);

    for (auto iter = _data->begin(); iter != _data->end(); iter++)
    {
        std::unique_ptr<hheader> hh(new hheader());
        hh->id_ = (*iter)->get_msgid();
        hh->prev_id_ = (*iter)->get_prev_msgid();
        hh->time_ = (*iter)->get_time();

        val_headers->push_back(hh.release());
    }

    _coll.set_value_as_hheaders(c_headers, val_headers.get());
}

not_sent_messages& local_history::get_pending_messages()
{
    if (!not_sent_messages_)
    {
        not_sent_messages_.reset(new not_sent_messages(archive_path_ + L"/" + L"pending.db"));
        not_sent_messages_->load_if_need();
    }

    return *not_sent_messages_;
}

int32_t local_history::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    get_pending_messages().insert(_contact, _msg);
    return 0;
}


int32_t local_history::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    get_pending_messages().update_if_exist(_contact, _msg);
    return 0;
}


not_sent_message_sptr local_history::get_first_message_to_send()
{
    return get_pending_messages().get_first_ready_to_send();
}

not_sent_message_sptr local_history::get_not_sent_message_by_iid(const std::string& _iid)
{
    return get_pending_messages().get_by_internal_id(_iid);
}

void local_history::get_pending_file_sharing(std::list<not_sent_message_sptr>& _messages)
{
    return get_pending_messages().get_pending_file_sharing_messages(_messages);
}

not_sent_message_sptr local_history::update_pending_with_imstate(const std::string& _message_internal_id,
                                                                 const int64_t& _hist_msg_id,
                                                                 const int64_t& _before_hist_msg_id)
{
    return get_pending_messages().update_with_imstate(_message_internal_id, _hist_msg_id, _before_hist_msg_id);
}


void local_history::failed_pending_message(const std::string& _message_internal_id)
{
    return get_pending_messages().failed_pending_message(_message_internal_id);
}

void local_history::delete_messages_up_to(const std::string& _contact, const int64_t _id)
{
    assert(!_contact.empty());
    assert(_id > -1);

    __INFO(
        "delete_history",
        "deleting history\n"
        "    contact=<%1%>\n"
        "    up-to=<%2%>",
        _contact % _id
    );

    return get_contact_archive(_contact)->delete_messages_up_to(_id);
}

void local_history::find_previewable_links(
    const archive::history_block_sptr &_block,
    Out common::tools::url_vector_t &_uris)
{
    for (const auto &message : *_block)
    {
        assert(message);

        auto message_uris = common::tools::url_parser::parse_urls(message->get_text());

        _uris.insert(
            _uris.end(),
            std::make_move_iterator(message_uris.begin()),
            std::make_move_iterator(message_uris.end()));
    }
}

int32_t local_history::remove_messages_from_not_sent(const std::string& _contact, archive::history_block_sptr _data)
{
    get_pending_messages().remove(_contact, _data);
    return 0;
}

void local_history::mark_message_duplicated(const std::string _message_internal_id)
{
    get_pending_messages().mark_duplicated(_message_internal_id);
}

void local_history::update_message_post_time(
    const std::string& _message_internal_id,
    const std::chrono::system_clock::time_point& _time_point)
{
    get_pending_messages().update_message_post_time(_message_internal_id, _time_point);
}

bool local_history::has_not_sent_messages(const std::string& _contact)
{
    return get_pending_messages().exist(_contact);
}

void local_history::get_not_sent_messages(const std::string& _contact, /*out*/ std::shared_ptr<history_block> _messages)
{
    get_pending_messages().get_messages(_contact, *_messages);
}

void local_history::optimize_contact_archive(const std::string& _contact)
{
    get_contact_archive(_contact)->optimize();
}

face::face(const std::wstring& _archive_path)
    : thread_(new core::async_executer())
    , history_cache_(new local_history(_archive_path))
{
}

std::shared_ptr<update_history_handler> face::update_history(const std::string& _contact, std::shared_ptr<archive::history_block> _data)
{
    assert(!_contact.empty());

    __LOG(core::log::info("archive", boost::format("update_history, contact=%1%") % _contact);)

    auto handler = std::make_shared<update_history_handler>();

    auto history_cache = history_cache_;
    auto ids = std::make_shared<headers_list>();
    auto state = std::make_shared<dlg_state>();
    auto state_changes = std::make_shared<dlg_state_changes>();

    thread_->run_async_function(
        [history_cache, _data, _contact, ids, state, state_changes]
        {
            history_cache->update_history(_contact, _data, Out *ids, Out *state, Out *state_changes);
            return 0;
        }
    )->on_result_ =
        [handler, ids, state, state_changes](int32_t _error)
        {
            if (handler->on_result)
            {
                handler->on_result(
                    ids,
                    *state,
                    *state_changes
                );
            }
        };

    return handler;
}

std::shared_ptr<request_images_handler> face::get_images(const std::string& _contact, int64_t _from, int64_t _count)
{
    assert(!_contact.empty());

    __LOG(core::log::info("archive", boost::format("get_images, contact=%1%") % _contact);)

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_images_handler>();
    auto images = std::make_shared<image_list>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([history_cache, _contact, _from, _count, images]() -> int32_t
    {
        history_cache->get_images(_contact, _from, _count, *images);
        return 0;

    })->on_result_ = [wr_this, handler, images](int32_t /*_error*/)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(images);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::repair_images(const std::string& _contact)
{
    assert(!_contact.empty());

    __LOG(core::log::info("archive", boost::format("repair_images, contact=%1%") % _contact);)

    auto history_cache = history_cache_;
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([history_cache, _contact]() -> int32_t
    {
        return history_cache->repair_images(_contact) ? 0 : 1;

    })->on_result_ = [wr_this, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<request_headers_handler> face::get_messages_index(const std::string& _contact, int64_t _from, int64_t _count)
{
    __LOG(core::log::info("archive", boost::format("get_history, contact=%1%") % _contact);)

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_headers_handler>();
    auto headers = std::make_shared<headers_list>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([history_cache, _contact, headers, _from, _count]()->int32_t
    {
        history_cache->get_messages_index(_contact, _from, _count, *headers);
        return 0;

    })->on_result_ = [wr_this, history_cache, handler, headers, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->thread_->run_async_function([history_cache, _contact]()->int32_t
        {
            history_cache->optimize_contact_archive(_contact);
            return 0;

        });

        if (handler->on_result)
            handler->on_result(headers);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids)
{
    auto handler = std::make_shared<request_buddies_handler>();
    auto history_cache = history_cache_;
    auto out_messages = std::make_shared<history_block>();

    thread_->run_async_function([history_cache, _contact, _ids, out_messages]()->int32_t
    {
        history_cache->get_messages_buddies(_contact, _ids, out_messages);
        return 0;

    })->on_result_ = [handler, out_messages](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_messages(const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later)
{
    assert(!_contact.empty());

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();
    std::weak_ptr<face> wr_this = shared_from_this();

    thread_->run_async_function([_contact, out_messages, _from, _count_early, _count_later, history_cache]()->int32_t
    {
        return (history_cache->get_messages(_contact, _from, _count_early, _count_later, out_messages) ? 0 : -1);

    })->on_result_ = [wr_this, handler, out_messages, _contact, history_cache](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->thread_->run_async_function([history_cache, _contact]()->int32_t
        {
            history_cache->optimize_contact_archive(_contact);
            return 0;
        });

        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}

std::shared_ptr<request_history_file_handler> face::get_history_block(std::shared_ptr<contact_and_offsets> _contacts
                                                                     , std::shared_ptr<contact_and_msgs> _archive, std::shared_ptr<tools::binary_stream> _data)
{
    assert(!_contacts->empty());

    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_history_file_handler>();
    std::weak_ptr<face> wr_this = shared_from_this();

    std::shared_ptr<contact_and_offsets> remaining(new contact_and_offsets());

    thread_->run_async_function([_contacts, _archive, history_cache, remaining, _data]()->int32_t
    {
        auto remain_size = std::make_shared<int64_t>(1024 * 1024 * 10);
        auto index = 0u;

        _archive->clear();
        auto has_result = false;
        int64_t cur_index = 0;

        while (*remain_size > 0 && index < _contacts->size())
        {
            auto _contact = (*_contacts)[index].first.first;
            auto regim = (*_contacts)[index].first.second;
            auto _offset = (*_contacts)[index].second;
            ++index;

            auto prev_cur_index = cur_index;
            auto cur_result = history_cache->get_history_file(_contact, *_data, _offset, remain_size, cur_index, regim);
            has_result |= cur_result;

            if (!cur_result)
            {
                *_offset = -2;
            }

            //if (cur_result && cur_index != prev_cur_index)
            {
                _archive->push_back(std::make_pair(_contact, prev_cur_index));
            }
        }
        _archive->push_back(std::make_pair("", cur_index));

        auto last_index = index;
        for (; index < _contacts->size(); ++index)
        {
            remaining->push_back((*_contacts)[index]);
        }
        _contacts->resize(last_index);

        return (has_result ? 0 : -1);

    })->on_result_ = [wr_this, handler, _archive, history_cache, remaining, _data](int32_t _error)
    {
        // we calc count of finished threads
        // if (_error == -1)
        //    return;

        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(_archive, remaining, _data);
    };

    return handler;
}

std::shared_ptr<request_dlg_state_handler> face::get_dlg_state(const std::string& _contact)
{
    auto handler = std::make_shared<request_dlg_state_handler>();
    auto history_cache = history_cache_;
    auto dialog = std::make_shared<dlg_state>();

    thread_->run_async_function([history_cache, dialog, _contact]()->int32_t
    {
        history_cache->get_dlg_state(_contact, *dialog);

        return 0;

    })->on_result_ = [handler, dialog](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*dialog);
    };

    return handler;
}

std::shared_ptr<request_dlg_states_handler> face::get_dlg_states(const std::vector<std::string>& _contacts)
{
    auto handler = std::make_shared<request_dlg_states_handler>();
    auto history_cache = history_cache_;
    auto dialogs = std::make_shared<std::vector<dlg_state>>();

    thread_->run_async_function([history_cache, _contacts, dialogs]()->int32_t
    {
        history_cache->get_dlg_states(_contacts, *dialogs);

        return 0;

    })->on_result_ = [handler, dialogs](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*dialogs);
    };

    return handler;
}

std::shared_ptr<set_dlg_state_handler> face::set_dlg_state(const std::string& _contact, const dlg_state& _state)
{
    auto handler = std::make_shared<set_dlg_state_handler>();
    auto history_cache = history_cache_;
    auto result_state = std::make_shared<dlg_state>();
    auto state_changes = std::make_shared<dlg_state_changes>();

    thread_->run_async_function(
        [history_cache, _state, _contact, result_state, state_changes]
        {
            history_cache->set_dlg_state(_contact, _state, Out *result_state, Out *state_changes);

            return 0;
        })
    ->on_result_ =
        [handler, result_state, state_changes]
        (int32_t _error)
        {
            if (handler->on_result)
                handler->on_result(*result_state, *state_changes);
        };

    return handler;
}


std::shared_ptr<async_task_handlers> face::clear_dlg_state(const std::string& _contact)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([history_cache, _contact]()->int32_t
    {
        return (history_cache->clear_dlg_state(_contact) ? 0 : -1);

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(_error);
    };

    return handler;
}



std::shared_ptr<request_next_hole_handler> face::get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth)
{
    auto handler = std::make_shared<request_next_hole_handler>();
    auto history_cache = history_cache_;
    auto hole = std::make_shared<archive_hole>();

    thread_->run_async_function([history_cache, hole, _contact, _from, _depth]()->int32_t
    {
        auto new_hole = history_cache->get_next_hole(_contact, _from, _depth);

        if (new_hole)
        {
            *hole = *new_hole;
            return 0;
        }

        return -1;

    })->on_result_ = [handler, hole](int32_t _error)
    {
        handler->on_result( (_error == 0) ? hole : nullptr);
    };

    return handler;
}

std::shared_ptr<validate_hole_request_handler> face::validate_hole_request(const std::string& _contact, const archive_hole& _hole_request, const int32_t _count)
{
    auto handler = std::make_shared<validate_hole_request_handler>();
    auto history_cache = history_cache_;
    auto from_result = std::make_shared<int64_t>();

    thread_->run_async_function([history_cache, _contact, _hole_request, from_result, _count]()->int32_t
    {
        *from_result = history_cache->validate_hole_request(_contact, _hole_request, _count);

        return 0;

    })->on_result_ = [handler, from_result](int32_t _error)
    {
        handler->on_result(*from_result);
    };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_pending_message()
{
    auto handler = std::make_shared<not_sent_messages_handler>();
    auto history_cache = history_cache_;
    auto message = std::make_shared<not_sent_message_sptr>();

    auto task = thread_->run_async_function(
        [history_cache, message]
    {
        *message = history_cache->get_first_message_to_send();
        return (*message) ? 0 : -1;
    }
    );

    task->on_result_ = [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::delete_messages_up_to(const std::string& _contact, const int64_t _id)
{
    assert(!_contact.empty());
    assert(_id > -1);

    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function(
        [history_cache, _contact, _id]
        {
            history_cache->delete_messages_up_to(_contact, _id);
            return 0;
        }
    )->on_result_ =
        [handler](int32_t _error)
        {
            handler->on_result_(_error);
        };

    return handler;
}

std::shared_ptr<find_previewable_links_handler> face::find_previewable_links(const archive::history_block_sptr &_block)
{
    assert(_block);

    auto handler = std::make_shared<find_previewable_links_handler>();
    auto history_cache = history_cache_;
    auto uris = std::make_shared<common::tools::url_vector_t>();

    thread_->run_async_function(
        [_block, history_cache, uris]
        {
            history_cache->find_previewable_links(_block, Out *uris);

            return 0;
        })
    ->on_result_ =
        [handler, uris]
        (int32_t _error)
        {
            if (_error != 0)
            {
                return;
            }

            assert(handler->on_result_);
            handler->on_result_(*uris);
        };

    return handler;
}

std::shared_ptr<not_sent_messages_handler> face::get_not_sent_message_by_iid(const std::string& _iid)
{
    assert(!_iid.empty());

    auto handler = std::make_shared<not_sent_messages_handler>();
    auto history_cache = history_cache_;
    auto message = std::make_shared<not_sent_message_sptr>();

    auto task = thread_->run_async_function(
        [history_cache, message, _iid]
    {
        *message = history_cache->get_not_sent_message_by_iid(_iid);
        return (*message) ? 0 : -1;
    }
    );

    task->on_result_ =
        [handler, message](int32_t _error)
    {
        if (handler->on_result)
        {
            const auto succeed = (_error == 0);
            handler->on_result(succeed ? *message : nullptr);
        }
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _msg, history_cache]()->int32_t
    {
        return history_cache->insert_not_sent_message(_contact, _msg);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _msg, history_cache]()->int32_t
    {
        return history_cache->update_if_exist_not_sent_message(_contact, _msg);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::remove_messages_from_not_sent(const std::string& _contact, archive::history_block_sptr _data)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([_contact, _data, history_cache]()->int32_t
    {
        return history_cache->remove_messages_from_not_sent(_contact, _data);

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::remove_message_from_not_sent(const std::string& _contact, const history_message_sptr _data)
{
    auto block = std::make_shared<history_block>();
    block->push_back(std::const_pointer_cast<history_message>(_data));

    return remove_messages_from_not_sent(_contact, block);
}

std::shared_ptr<async_task_handlers> face::mark_message_duplicated(const std::string& _message_internal_id)
{
    auto handler = std::make_shared<async_task_handlers>();
    auto history_cache = history_cache_;

    thread_->run_async_function([history_cache, _message_internal_id]()->int32_t
    {
        history_cache->mark_message_duplicated(_message_internal_id);

        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

void face::serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll)
{
    local_history::serialize(_headers, _coll);
}

void face::serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll)
{
    local_history::serialize_headers(_data, _coll);
}

std::shared_ptr<has_not_sent_handler> face::has_not_sent_messages(const std::string& _contact)
{
    auto handler = std::make_shared<has_not_sent_handler>();
    auto history_cache = history_cache_;
    auto res = std::make_shared<bool>(false);

    thread_->run_async_function([_contact, history_cache, res]()->int32_t
    {
        *res = history_cache->has_not_sent_messages(_contact);

        return 0;

    })->on_result_ = [handler, res](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(*res);
    };

    return handler;
}

std::shared_ptr<request_buddies_handler> face::get_not_sent_messages(const std::string& _contact)
{
    auto history_cache = history_cache_;
    auto handler = std::make_shared<request_buddies_handler>();
    auto out_messages = std::make_shared<history_block>();

    thread_->run_async_function([_contact, out_messages, history_cache]()->int32_t
    {
        history_cache->get_not_sent_messages(_contact, out_messages);

        return 0;

    })->on_result_ = [handler, out_messages](int32_t _error)
    {
        if (handler->on_result)
            handler->on_result(out_messages);
    };

    return handler;
}


std::shared_ptr<pending_messages_handler> face::get_pending_file_sharing()
{
    auto history_cache = history_cache_;
    auto handler = std::make_shared<pending_messages_handler>();
    auto messages_list = std::make_shared<std::list<not_sent_message_sptr>>();

    thread_->run_async_function([messages_list, history_cache]()->int32_t
    {
        history_cache->get_pending_file_sharing(*messages_list);

        return 0;

    })->on_result_ = [handler, messages_list](int32_t _error)
    {
        handler->on_result(*messages_list);
    };

    return handler;
}


std::shared_ptr<not_sent_messages_handler> face::update_pending_messages_by_imstate(const std::string& _message_internal_id,
                                                                                    const int64_t& _hist_msg_id,
                                                                                    const int64_t& _before_hist_msg_id)
{
    auto handler = std::make_shared<not_sent_messages_handler>();

    auto history_cache = history_cache_;

    auto message = std::make_shared<not_sent_message_sptr>();

    auto task = thread_->run_async_function([history_cache, message, _message_internal_id, _hist_msg_id, _before_hist_msg_id]
    {
        *message = history_cache->update_pending_with_imstate(_message_internal_id, _hist_msg_id, _before_hist_msg_id);

        return (*message) ? 0 : -1;
    });

    task->on_result_ = [handler, message](int32_t _error)
    {
        const auto succeed = (_error == 0);

        handler->on_result(succeed ? *message : nullptr);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> face::update_message_post_time(
    const std::string& _message_internal_id,
    const std::chrono::system_clock::time_point& _time_point)
{
    auto handler = std::make_shared<async_task_handlers>();

    auto history_cache = history_cache_;

    auto task = thread_->run_async_function([history_cache, _message_internal_id, _time_point]
    {
        history_cache->update_message_post_time(_message_internal_id, _time_point);

        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(0);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::failed_pending_message(const std::string& _message_internal_id)
{
    auto handler = std::make_shared<async_task_handlers>();

    auto history_cache = history_cache_;

    auto task = thread_->run_async_function([history_cache, _message_internal_id]
    {
        history_cache->failed_pending_message(_message_internal_id);

        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(0);
    };

    return handler;
}


std::shared_ptr<async_task_handlers> face::sync_with_history()
{
    auto handler = std::make_shared<async_task_handlers>();

    thread_->run_async_function([]()->int32_t
    {
        return 0;

    })->on_result_ = [handler](int32_t _error)
    {
        handler->on_result_(_error);
    };

    return handler;
}