#ifndef __CONTACT_ARCHIVE_H_
#define __CONTACT_ARCHIVE_H_

#pragma once

namespace core
{
    namespace archive
    {
        class history_message;
        class archive_index;
        class messages_data;
        class message_header;
        class dlg_state;
        struct dlg_state_changes;
        class archive_hole;
        class archive_state;
        class image_cache;
        class image_data;

        typedef std::list<image_data> image_list;
        typedef std::vector<std::shared_ptr<history_message>> history_block;
        typedef std::shared_ptr<history_block>                      history_block_sptr;
        typedef std::list<int64_t>									msgids_list;
        typedef std::list<message_header>							headers_list;

        class contact_archive
        {
            const std::wstring					path_;

            std::unique_ptr<archive_index>		index_;
            std::unique_ptr<messages_data>		data_;
            std::unique_ptr<archive_state>		state_;
            std::unique_ptr<image_cache>		images_;

            bool								local_loaded_;

            bool update_dlg_state_last_message();

            mutable std::mutex                  mutex_;
            std::thread                         image_cache_thread_;

        public:
            void get_images(int64_t _from, int64_t _count, image_list& _images) const;
            bool repair_images() const;

            enum class get_message_policy
            {
                get_all,
                skip_patches_and_deleted
            };
            void get_messages(int64_t _from, int64_t _count_early, int64_t _count_later, history_block& _messages, get_message_policy policy) const;
            void get_messages_index(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _headers) const;
            bool get_messages_buddies(std::shared_ptr<archive::msgids_list> _ids, std::shared_ptr<history_block> _messages) const;
            static bool get_history_file(const std::wstring& _file_name, core::tools::binary_stream& _data
                , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode);

            bool get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth);
            int64_t validate_hole_request(const archive_hole& _hole, const int32_t _count);

            const dlg_state& get_dlg_state() const;
            void set_dlg_state(const dlg_state& _state, Out dlg_state_changes& _changes);
            void clear_dlg_state();

            void insert_history_block(
                history_block_sptr _data,
                Out headers_list& _inserted_messages,
                Out dlg_state& _updated_state,
                Out dlg_state_changes& _state_changes);

            int32_t load_from_local();

            bool need_optimize();
            void optimize();

            void delete_messages_up_to(const int64_t _up_to);

            contact_archive(const std::wstring& _archive_path, const std::string& _contact_id);
            virtual ~contact_archive();

        };

        std::wstring db_filename();
        std::wstring index_filename();
        std::wstring dlg_state_filename();
        std::wstring image_cache_filename();
        std::wstring cache_filename();
    }
}

#endif //__CONTACT_ARCHIVE_H_