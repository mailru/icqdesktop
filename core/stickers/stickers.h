#pragma once

#include "../../corelib/collection_helper.h"
enum class loader_errors;

namespace core
{
    class async_executer;
    struct icollection;
    enum class sticker_size;

    namespace tools
    {
        class binary_stream;
    }

    namespace stickers
    {
        class sticker_params
        {
            sticker_size size_;
            int32_t width_;
            int32_t height_;

        public:

            sticker_params(sticker_size _size, int32_t _width, int32_t _height);

            sticker_size get_size() const;

        };

        typedef std::map<sticker_size, sticker_params> size_map;

        class sticker
        {
            int32_t		id_;
            size_map	sizes_;

        public:

            sticker();

            int32_t get_id() const;

            const size_map& get_sizes() const;

            bool unserialize(const rapidjson::Value& _node);

        };


        enum set_icon_size
        {
            invalid = 0,

            _20	= 20,
            _32 = 32,
            _48 = 48,
            _64 = 64
        };

        class set_icon
        {
            set_icon_size	size_;
            std::string		url_;

        public:
            set_icon();
            set_icon(set_icon_size _size, std::string _url);

            set_icon_size get_size() const;
            std::string get_url() const;
        };


        typedef std::map<stickers::set_icon_size, set_icon>		icons_map;
        typedef std::list<std::shared_ptr<sticker>>				stickers_list;

        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class set
        {
            int32_t			id_;
            std::string		name_;
            bool			show_;

            icons_map		icons_;
            stickers_list	stickers_;

        public:

            set();

            int32_t get_id() const;
            void set_id(int32_t _id);

            const std::string& get_name() const;
            void set_name(const std::string& _name);

            void put_icon(const set_icon& _icon);
            set_icon get_icon(stickers::set_icon_size _size);

            bool is_show() const;
            void set_show(bool _show);

            const icons_map& get_icons() const;
            const stickers_list& get_stickers() const;

            bool unserialize(const rapidjson::Value& _node);
        };

        typedef std::list<std::shared_ptr<stickers::set>>	sets_list;

        template<class t0_, class t1_ = void, class t2_ = void>
        class result_handler
        {
        public:
            std::function<void(t0_, t1_, t2_)>	on_result_;

            result_handler()
            {
                on_result_ = [](t0_, t1_, t2_){};
            }
        };

        template<class t0_>
        class result_handler<t0_, void, void>
        {
        public:
            std::function<void(t0_)>	on_result_;

            result_handler()
            {
                on_result_ = [](t0_){};
            }
        };

        template<class t0_, class t1_>
        class result_handler<t0_, t1_, void>
        {
        public:
            std::function<void(t0_, t1_)>	on_result_;
            result_handler()
            {
                on_result_ = [](t0_, t1_){};
            }
        };

        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class parse_result
        {
            bool	result_;
            bool	up_to_date_;

        public:

            parse_result(bool _result, bool _up_to_date) :	result_(_result), up_to_date_(_up_to_date) {}

            bool is_success() const { return result_; }
            bool is_up_to_date() const { return up_to_date_; }
        };

        class load_result
        {
            bool				result_;
            const std::string	md5_;

        public:

            load_result(bool _result, const std::string _md5) :	result_(_result), md5_(_md5) {}

            bool get_result() const { return result_; }
            std::string get_md5() const { return md5_; }
        };


        //////////////////////////////////////////////////////////////////////////
        //
        //////////////////////////////////////////////////////////////////////////
        class download_task
        {
            std::string source_url_;
            std::wstring dest_file_;

            int32_t set_id_;
            int32_t sticker_id_;
            sticker_size size_;

        public:

            download_task(
                const std::string& _source_url,
                const std::wstring& _dest_file,
                int32_t _set_id,
                int32_t _sticker_id,
                sticker_size _size);

            const std::string& get_source_url() const;
            const std::wstring& get_dest_file() const;
            int32_t get_set_id() const;
            int32_t get_sticker_id() const;
            sticker_size get_size() const;
        };

        typedef std::list<download_task> download_tasks;
        typedef std::list<int64_t> requests_list;

        //////////////////////////////////////////////////////////////////////////
        // class cache
        //////////////////////////////////////////////////////////////////////////
        class cache
        {
            std::string md5_;
            std::wstring stickers_path_;
            sets_list sets_;

            download_tasks meta_tasks_;
            download_tasks stickers_tasks_;

            typedef std::map<int32_t, requests_list> stickers_ids_list;

            typedef std::map<int32_t, stickers_ids_list> stickers_sets_ids_list;

            stickers_sets_ids_list gui_requests_;

            requests_list get_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id) const;
            void clear_sticker_gui_requests(int32_t _set_id, int32_t _sticker_id);
            bool has_gui_request(int32_t _set_id, int32_t _sticker_id);

        public:

            void make_download_tasks();

            cache(const std::wstring& _stickers_path);
            virtual ~cache();

            std::wstring get_meta_file_name() const;
            bool parse(core::tools::binary_stream& _data, bool _insitu, bool& _up_todate);
            bool is_meta_icons_exist();

            void serialize_meta_sync(coll_helper _coll, const std::string& _size);

            std::wstring get_set_icon_path(const set& _set, const set_icon& _icon) const;
            std::wstring get_sticker_path(const set& _set, const sticker& _sticker, sticker_size _size) const;
            std::wstring get_sticker_path(int32_t _set_id, int32_t _sticker_id, sticker_size _size) const;

            bool get_next_meta_task(download_task& _task);
            bool get_next_sticker_task(download_task& _task);
            void get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, const sticker_size _size, tools::binary_stream& _data);
            std::string get_md5() const;
            bool sticker_loaded(const download_task& _task, /*out*/ requests_list&);
            bool meta_loaded(const download_task& _task);
        };

        struct gui_request_params
        {
            std::string size_;
            int64_t seq_;
            std::string md5_;

            gui_request_params(const std::string& _size, int64_t _seq, const std::string& _md5)
                :   size_(_size), seq_(_seq), md5_(_md5) {}

            gui_request_params()
                :   seq_(-1) {}
        };

        enum failed_step
        {
            ok = 0,
            download_metafile = 1,
            download_metadata = 2,
            download_stickers = 3
        };

        class face
        {
            std::shared_ptr<cache> cache_;
            std::shared_ptr<async_executer> thread_;

            bool meta_requested_;

            bool up_to_date_;

            bool download_in_progress_;

            loader_errors error_;
            failed_step failed_step_;

            gui_request_params gui_request_params_;

        public:

            face(const std::wstring& _stickers_path);

            std::shared_ptr<result_handler<const parse_result&>> parse(
                std::shared_ptr<core::tools::binary_stream> _data,
                bool _insitu);
            std::shared_ptr<result_handler<const load_result&>> load_meta_from_local();
            std::shared_ptr<result_handler<bool>> save(std::shared_ptr<core::tools::binary_stream> _data);
            std::shared_ptr<result_handler<bool>> make_download_tasks();
            std::shared_ptr<result_handler<coll_helper>> serialize_meta(coll_helper _coll, const std::string& _size);
            std::shared_ptr<result_handler<tools::binary_stream&>> get_sticker(
                int64_t _seq,
                int32_t _set_id,
                int32_t _sticker_id,
                const core::sticker_size _size);
            std::shared_ptr<result_handler<bool, const download_task&>> get_next_meta_task();
            std::shared_ptr<result_handler<bool, const download_task&>> get_next_sticker_task();
            std::shared_ptr<result_handler<bool, const requests_list&>> on_sticker_loaded(const download_task& _task);
            std::shared_ptr<result_handler<bool>> on_metadata_loaded(const download_task& _task);
            std::shared_ptr<result_handler<const std::string&>> get_md5();

            void set_meta_requested();
            bool is_meta_requested();

            void set_failed_step(failed_step _step);
            failed_step get_failed_step();

            void set_last_error(loader_errors _error);
            loader_errors get_last_error() const;

            void set_gui_request_params(const gui_request_params& _params);
            const gui_request_params& get_gui_request_params();

            void set_up_to_date(bool _up_to_date);
            bool is_up_to_date() const;

            bool is_download_in_progress();
            void set_download_in_progress(bool _in_progress);
        };
    }
}