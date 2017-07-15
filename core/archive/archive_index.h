#ifndef __CONTACT_ARCHIVE_INDEX_H_
#define __CONTACT_ARCHIVE_INDEX_H_

#pragma once

#include "history_message.h"
#include "dlg_state.h"
#include "message_flags.h"
#include "errors.h"

namespace core
{
    namespace archive
    {

        class storage;

        typedef std::vector<std::shared_ptr<history_message>> history_block;
        typedef std::shared_ptr<history_block> history_block_sptr;

        typedef std::list<message_header> headers_list;
        typedef std::shared_ptr<headers_list> headers_list_sptr;

        typedef std::map<int64_t, message_header> headers_map;

        class archive_hole
        {
            int64_t from_;
            int64_t to_;
            int64_t depth_;

        public:

            archive_hole() : from_(-1), to_(-1), depth_(0) {}

            int64_t get_from() const { return from_; }
            void set_from(int64_t _value) { from_ = _value; }
            bool has_from() const { return (from_ > 0); }

            int64_t get_to() const { return to_; }
            void set_to(int64_t _value) { to_ = _value; }
            bool has_to() const { return (to_ > 0); }

            void set_depth(int64_t _depth) { depth_ = _depth; }
            int64_t get_depth() const { return depth_; }
        };


        //////////////////////////////////////////////////////////////////////////
        // archive_index class
        //////////////////////////////////////////////////////////////////////////
        class archive_index
        {
            archive::error last_error_;
            headers_map headers_index_;
            std::unique_ptr<storage> storage_;

            void serialize_block(const headers_list& _headers, core::tools::binary_stream& _data) const;
            bool unserialize_block(core::tools::binary_stream& _data);
            void insert_block(const archive::headers_list& _headers);
            void insert_header(const archive::message_header& header);

        public:

            bool get_header(int64_t _msgid, message_header& _header) const;

            bool has_header(const int64_t _msgid) const;

            bool get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const;
            int64_t validate_hole_request(const archive_hole& _hole, const int32_t _count);

            bool save_all();
            bool save_block(const archive::headers_list& _block);

            void optimize();
            bool need_optimize();

            bool load_from_local();

            void serialize(headers_list& _list) const;
            bool serialize_from(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _list) const;
            bool update(const archive::history_block& _data, /*out*/ headers_list& _headers);

            void delete_up_to(const int64_t _to);

            int64_t get_last_msgid();

            archive::error get_last_error() const { return last_error_; }

            archive_index(const std::wstring& _file_name);
            virtual ~archive_index();
        };

    }
}


#endif //__CONTACT_ARCHIVE_INDEX_H_