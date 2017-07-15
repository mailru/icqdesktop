#pragma once

#include "../../corelib/collection_helper.h"
#include "../../common.shared/themes_constants.h"

namespace core
{
    class async_executer;
    
    namespace tools
    {
        class binary_stream;
    }
    
    namespace themes
    {
        class theme
        {
            std::string image_name_;
            std::string thumb_name_;
            bool tile_;
            int32_t position_;
            std::string tint_color_;
            
            std::string typing_color_;
            std::wstring folder_path_;
            int32_t theme_id_;
        public:
            theme(std::wstring folder_path);
            bool is_tile() const { return tile_; }
            std::string get_typing_color() { return typing_color_; }
            std::string get_image_normal_name() const { return image_name_; }
            std::string get_thumb_normal_name() const { return thumb_name_; }
            std::wstring get_theme_folder() const;
            static std::wstring get_theme_folder(int32_t _theme_id);
            std::string get_tint_color() const { return tint_color_; }
            int32_t get_theme_id() const { return theme_id_; }
            int32_t get_position() { return position_; }
            void set_position(int32_t _position) { position_ = _position; }
            bool unserialize(const rapidjson::Value& _node, const ThemesScale _themes_scale);
            std::wstring get_image_path() const;
            std::wstring get_thumb_path() const;
            
        public:

            struct bubble {
                std::string bg1_color_;
                std::string bg2_color_;
                std::string time_color_;
                std::string info_color_;
                void unserialize(const rapidjson::Value& _node);
            };
            bubble incoming_bubble_;
            bubble outgoing_bubble_;
            
            struct date {
                std::string bg_color_;
                std::string text_color_;
                void unserialize(const rapidjson::Value& _node);
            } date_;
            
            struct preview_stickers {
                std::string time_color_;
                void unserialize(const rapidjson::Value& _node);
            } preview_stickers_;
            
            struct chat_event {
                std::string bg_color_;
                std::string text_color_;
                void unserialize(const rapidjson::Value& _node);
            } chat_event_;
            
            struct contact_name {
                std::string text_color_;
                void unserialize(const rapidjson::Value& _node);
            } contact_name_;
            
            struct new_messages_plate {
                std::string bg_color_;
                std::string text_color_;
                void unserialize(const rapidjson::Value& _node);
            } new_messages_plate_;
            
            struct typing {
                typing()
                    : light_gif_(0)
                {
                }

                std::string text_color_;
                int32_t light_gif_;
                void unserialize(const rapidjson::Value& _node);
            } typing_;
        };
        
        class download_task
        {
            std::string source_url_;
            std::wstring dest_file_;
            
            int32_t set_id_;
            int32_t theme_id_;
            
            std::list<int64_t> requests_;
        public:
            
            download_task(
                          const std::string& _source_url,
                          const std::wstring& _dest_file,
                          int32_t _theme_id
//                          , sticker_size _size
                          );
            
            const std::string& get_source_url() const;
            const std::wstring& get_dest_file() const;
//            int32_t get_set_id() const;
//            int32_t get_sticker_id() const;
//            sticker_size get_size() const;
            std::list<int64_t> get_requests() const;
            int32_t get_theme_id() const { return theme_id_; }
            
            void push_request(int64_t _seq);
        };
        
        typedef std::list<std::shared_ptr<themes::theme>>	themes_list;
        typedef std::list<download_task> download_tasks;
        
        class cache
        {
            std::wstring themes_path_;
            themes_list themes_list_;
            std::string base_url_;
            download_tasks meta_tasks_;
            download_tasks themes_tasks_;
            ThemesScale themes_scale_;
        public:
            cache(const std::wstring& _stickers_path);
            std::wstring get_meta_file_name() const;
            bool parse(core::tools::binary_stream& _data, bool _insitu);
            void make_download_tasks();
            void make_download_task(theme& _theme);
            bool get_next_meta_task(download_task& _task);
            std::string get_theme_thumb_url(const theme& _theme) const;
            std::string get_theme_image_url(const theme& _theme) const;
            std::wstring get_theme_image_path(const int32_t _theme_id) const;
            std::wstring get_theme_thumb_path(const int32_t _theme_id) const;
            bool meta_loaded(const download_task& _task);
            bool get_next_theme_task(download_task& _task);
            bool theme_loaded(const download_task& _task, std::list<int64_t>& _requests);
            void serialize_meta_sync(coll_helper _coll);
            void get_theme_image(int64_t _seq, int32_t _theme_id, tools::binary_stream& _data);
            theme* get_theme(int32_t _theme_id);
            void clear_all();
            inline ThemesScale get_themes_scale() { return themes_scale_; }
            void set_themes_scale(ThemesScale _themes_scale) { themes_scale_ = _themes_scale; }
        };
        
        class parse_result
        {
            bool	result_;
            std::string base_url_;
        public:
            parse_result(bool _result) : result_(_result) {}
            bool get_result() const { return result_; }
        };
        
        class load_result
        {
            bool				result_;
        public:
            load_result(bool _result): result_(_result) {}
            inline bool get_result() const { return result_; }
        };

        template<class T0, class T1 = void>
        class result_handler
        {
        public:
            std::function<void(T0, T1)>	on_result_;
            result_handler()
            {
                on_result_ = [](T0, T1){};
            }
        };

        template<class T0>
        class result_handler<T0, void>
        {
        public:
            std::function<void(T0)>	on_result_;
            result_handler()
            {
                on_result_ = [](T0){};
            }
        };
        
        class face
        {
            std::shared_ptr<cache> cache_;
            std::shared_ptr<async_executer> thread_;
        public:
            face(const std::wstring& _themes_path);
            std::shared_ptr<result_handler<const load_result&>> load_meta_from_local();
            std::shared_ptr<result_handler<const parse_result&>> parse(std::shared_ptr<core::tools::binary_stream> _data, bool _insitu);
            std::shared_ptr<result_handler<bool>> save(std::shared_ptr<core::tools::binary_stream> _data);
            std::shared_ptr<result_handler<bool>> make_download_tasks();
            std::shared_ptr<result_handler<bool, const download_task&>> get_next_meta_task();
            std::shared_ptr<result_handler<bool>> on_metadata_loaded(const download_task& _task);
            std::shared_ptr<result_handler<bool, const std::list<int64_t>&>> on_theme_loaded(const download_task& _task);
            std::shared_ptr<result_handler<tools::binary_stream&>> get_theme_image(int64_t _seq, int32_t _theme_id);
            std::shared_ptr<result_handler<bool, const download_task&>> get_next_theme_task();
            std::shared_ptr<result_handler<coll_helper>> serialize_meta(coll_helper _coll);
            theme* get_theme(int32_t _theme_id);
            void clear_all();
            ThemesScale get_themes_scale();
            void set_themes_scale(ThemesScale _themes_scale);
        };
    }
}