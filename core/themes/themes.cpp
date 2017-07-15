#include "stdafx.h"
#include "themes.h"
#include "../async_task.h"
#include "../tools/binary_stream.h"
#include "../tools/strings.h"
#include <stdio.h>

namespace core
{
    namespace themes
    {
        /* face */
        face::face(const std::wstring& _themes_path)
        :	thread_(new async_executer()),
            cache_(new cache(_themes_path))
        {
        }
        
        const std::wstring themes_meta_file_name_ = L"meta";
        
        std::shared_ptr<result_handler<const load_result&>> face::load_meta_from_local()
        {
            auto handler = std::make_shared<result_handler<const load_result&>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache]()->int32_t
            {
                core::tools::binary_stream bs;
                if (!bs.load_from_file(themes_cache->get_meta_file_name()))
                    return -1;
                
                bs.write(0);
                
                if (!themes_cache->parse(bs, true))
                    return -1;

                return 0;
                
            })->on_result_ = [handler](int32_t _error)
            {
                handler->on_result_(load_result((_error == 0)));
            };
            return handler;
        }
        
        std::shared_ptr<result_handler<bool>> face::save(std::shared_ptr<core::tools::binary_stream> _data)
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache, _data]()->int32_t
            {
                return (_data->save_2_file(themes_cache->get_meta_file_name()) ? 0 : -1);
            }
            )->on_result_ = [handler](int32_t _error)
            {
                handler->on_result_(_error == 0);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<const parse_result&>> face::parse(std::shared_ptr<core::tools::binary_stream> _data, bool _insitu)
        {
            auto handler = std::make_shared<result_handler<const parse_result&>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache, _data, _insitu]()->int32_t
                                        {
                                            return (themes_cache->parse(*_data, _insitu) ? 0 : -1);
                                            
                                        })->on_result_ = [handler](int32_t _error)
                                        {
                                            handler->on_result_(parse_result(_error == 0));
                                        };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<bool>> face::make_download_tasks()
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache]()->int32_t
                                        {
                                            themes_cache->make_download_tasks();
                                            return 0;
                                            
                                        })->on_result_ = [handler](int32_t _error)
            {
                handler->on_result_(true);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_meta_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto stickers_cache = cache_;
            auto task = std::make_shared<download_task>("", L"");
            
            thread_->run_async_function([stickers_cache, task]()->int32_t
                                        {
                                            if (!stickers_cache->get_next_meta_task(*task))
                                                return -1;
                                            
                                            return 0;
                                            
                                        })->on_result_ = [handler, task](int32_t _error)
            {
                handler->on_result_(_error == 0, *task);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<bool>> face::on_metadata_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache, _task]()->int32_t
                                        {
                                            return (themes_cache->meta_loaded(_task) ? 0 : -1);
                                            
                                        })->on_result_ = [handler](int32_t _error)
            {
                handler->on_result_(_error == 0);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<bool, const std::list<int64_t>&>> face::on_theme_loaded(const download_task& _task)
        {
            auto handler = std::make_shared<result_handler<bool, const std::list<int64_t>&>>();
            auto themes_cache = cache_;
            auto requests = std::make_shared<std::list<int64_t>>();
            
            thread_->run_async_function([themes_cache, _task, requests]()->int32_t
                                        {
                                            return (themes_cache->theme_loaded(_task, *requests) ? 0 : -1);
                                            
                                        })->on_result_ = [handler, requests](int32_t _error)
            {
                handler->on_result_((_error == 0), *requests);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<tools::binary_stream&>> face::get_theme_image(int64_t _seq, int32_t _theme_id)
        {
            auto handler = std::make_shared<result_handler<tools::binary_stream&>>();
            auto themes_cache = cache_;
            auto theme_data = std::make_shared<tools::binary_stream>();
            
            thread_->run_async_function([themes_cache, theme_data, _theme_id, _seq]
                                        {
                                            themes_cache->get_theme_image(_seq, _theme_id, *theme_data);
                                            
                                            return 0;
                                            
                                        })->on_result_ = [handler, theme_data](int32_t _error)
            {
                handler->on_result_(*theme_data);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<bool, const download_task&>> face::get_next_theme_task()
        {
            auto handler = std::make_shared<result_handler<bool, const download_task&>>();
            auto themes_cache = cache_;
            auto task = std::make_shared<download_task>("", L"");
            
            thread_->run_async_function([themes_cache, task]()->int32_t
                                        {
                                            if (!themes_cache->get_next_theme_task(*task))
                                                return -1;
                                            
                                            return 0;
                                            
                                        })->on_result_ = [handler, task](int32_t _error)
            {
                handler->on_result_(_error == 0, *task);
            };
            
            return handler;
        }
        
        std::shared_ptr<result_handler<coll_helper>> face::serialize_meta(coll_helper _coll)
        {
            auto handler = std::make_shared<result_handler<coll_helper>>();
            auto themes_cache = cache_;
            
            thread_->run_async_function([themes_cache, _coll]()->int32_t
                                        {
                                            themes_cache->serialize_meta_sync(_coll);
                                            return 0;
                                            
                                        })->on_result_ = [handler, _coll](int32_t _error)
            {
                handler->on_result_(_coll);
            };
            
            return handler;
        }
        
        theme* face::get_theme(int32_t _theme_id)
        {
            return cache_->get_theme(_theme_id);
        }
        
        void face::clear_all()
        {
            cache_->clear_all();
        }

        ThemesScale face::get_themes_scale()
        {
            return cache_->get_themes_scale();
        }

        void face::set_themes_scale(ThemesScale _themes_scale)
        {
            cache_->set_themes_scale(_themes_scale);
        }
        
        /* theme */
        theme::theme(std::wstring folder_path) 
            : tile_(false)
            , position_(0)
            , folder_path_(folder_path)
            , theme_id_(0)
        {
        }

        void theme::bubble::unserialize(const rapidjson::Value& _node)
        {
            auto iter_bgc1 = _node.FindMember("BackgroundColor1");
            if (iter_bgc1 != _node.MemberEnd() && iter_bgc1->value.IsString())
                bg1_color_ = iter_bgc1->value.GetString();
            
            auto iter_bgc2 = _node.FindMember("BackgroundColor2");
            if (iter_bgc2 != _node.MemberEnd() && iter_bgc2->value.IsString())
                bg2_color_ = iter_bgc2->value.GetString();
            
            auto iter_time_color = _node.FindMember("TimeColor");
            if (iter_time_color != _node.MemberEnd() && iter_time_color->value.IsString())
                time_color_ = iter_time_color->value.GetString();
        }
        
        void theme::date::unserialize(const rapidjson::Value &_node)
        {
            auto iter_bg_color = _node.FindMember("BackgroundColor");
            if (iter_bg_color != _node.MemberEnd() && iter_bg_color->value.IsString())
                bg_color_ = iter_bg_color->value.GetString();
            
            auto iter_text_color = _node.FindMember("TextColor");
            if (iter_text_color != _node.MemberEnd() && iter_text_color->value.IsString())
                text_color_ = iter_text_color->value.GetString();
        }
        
        void theme::preview_stickers::unserialize(const rapidjson::Value &_node)
        {
            auto iter_time_color = _node.FindMember("TimeColor");
            if (iter_time_color != _node.MemberEnd() && iter_time_color->value.IsString())
                time_color_ = iter_time_color->value.GetString();
        }
        
        void theme::chat_event::unserialize(const rapidjson::Value &_node)
        {
            auto iter_bg_color = _node.FindMember("BackgroundColor");
            if (iter_bg_color != _node.MemberEnd() && iter_bg_color->value.IsString())
                bg_color_ = iter_bg_color->value.GetString();
            
            
            auto iter_text_color = _node.FindMember("TextColor");
            if (iter_text_color != _node.MemberEnd() && iter_text_color->value.IsString())
                text_color_ = iter_text_color->value.GetString();
        }
        
        void theme::contact_name::unserialize(const rapidjson::Value &_node)
        {
            auto iter_text_color = _node.FindMember("TextColor");
            if (iter_text_color != _node.MemberEnd() && iter_text_color->value.IsString())
                text_color_ = iter_text_color->value.GetString();
        }
                
        void theme::new_messages_plate::unserialize(const rapidjson::Value &_node)
        {
            auto iter_bg_color = _node.FindMember("BackgroundColor");
            if (iter_bg_color != _node.MemberEnd() && iter_bg_color->value.IsString())
                bg_color_ = iter_bg_color->value.GetString();
            
            auto iter_text_color = _node.FindMember("TextColor");
            if (iter_text_color != _node.MemberEnd() && iter_text_color->value.IsString())
                text_color_ = iter_text_color->value.GetString();
        }
                
        void theme::typing::unserialize(const rapidjson::Value &_node)
        {
            auto iter_text_color = _node.FindMember("TextColor");
            if (iter_text_color != _node.MemberEnd() && iter_text_color->value.IsString())
                text_color_ = iter_text_color->value.GetString();
            
            auto iter_light_gif = _node.FindMember("LightGif");
            if (iter_light_gif != _node.MemberEnd() && iter_light_gif->value.IsString())
                light_gif_ = std::stoi(iter_light_gif->value.GetString(), 0);
        }
        
        bool theme::unserialize(const rapidjson::Value& _node, const ThemesScale _themes_scale)
        {
            auto iter_id = _node.FindMember("ID");
            if (iter_id != _node.MemberEnd() && iter_id->value.IsString())
                theme_id_ = std::stoi(iter_id->value.GetString(), 0);
            
            if (theme_id_ <= 0)
            {
                return false;
            }
            
            auto iter_image = _node.FindMember("Image");
            if (iter_image != _node.MemberEnd() && iter_image->value.IsObject())
            {
                auto image_field_name = "";
                switch (_themes_scale) {
                    case ThemesScaleRetina:
                        image_field_name = "Image_200";
                        break;
                    case ThemesScale100:
                        image_field_name = "Image_100";
                        break;
                    case ThemesScale125:
                        image_field_name = "Image_125";
                        break;
                    case ThemesScale150:
                        image_field_name = "Image_150";
                        break;
                    case ThemesScale200:
                        image_field_name = "Image_200";
                        break;
                    default:
                        image_field_name = "Image_100";
                        assert(false);
                        break;
                }
                
                auto iter_image_name = iter_image->value.FindMember(image_field_name);
                if (iter_image_name != iter_image->value.MemberEnd() && iter_image_name->value.IsString())
                {
                    image_name_ = iter_image_name->value.GetString();
                }
            }
            
            auto iter_preview = _node.FindMember("Preview");
            if (iter_preview != _node.MemberEnd() && iter_preview->value.IsObject())
            {
                auto thumb_field_name = "";
                switch (_themes_scale) {
                    case ThemesScaleRetina:
                        thumb_field_name = "Thumb_200";
                        break;
                    case ThemesScale100:
                        thumb_field_name = "Thumb_100";
                        break;
                    case ThemesScale125:
                        thumb_field_name = "Thumb_125";
                        break;
                    case ThemesScale150:
                        thumb_field_name = "Thumb_150";
                        break;
                    case ThemesScale200:
                        thumb_field_name = "Thumb_200";
                        break;
                    default:
                        thumb_field_name = "Thumb_100";
                        break;
                }
                
                auto iter_thumb = iter_preview->value.FindMember(thumb_field_name);
                if (iter_thumb != iter_preview->value.MemberEnd() && iter_thumb->value.IsString())
                {
                    thumb_name_ = iter_thumb->value.GetString();
                }
            }
            
            auto iter_tile = _node.FindMember("Tile");
            if (iter_tile != _node.MemberEnd() && iter_tile->value.IsString())
                tile_ = std::stoi(iter_tile->value.GetString()) == 1;
            
            auto iter_typing_color = _node.FindMember("TypingColor");
            if (iter_typing_color != _node.MemberEnd() && iter_typing_color->value.IsString())
                typing_color_ = iter_typing_color->value.GetString();
            
            auto iter_tint_color = _node.FindMember("TintColor");
            if (iter_tint_color != _node.MemberEnd() && iter_tint_color->value.IsString())
                tint_color_ = iter_tint_color->value.GetString();
            
            auto iter_incoming_bubble = _node.FindMember("IncomingBubble");
            if (iter_incoming_bubble != _node.MemberEnd() && iter_incoming_bubble->value.IsObject())
            {
                incoming_bubble_.unserialize(iter_incoming_bubble->value);
            }
            
            auto iter_outgoing_bubble = _node.FindMember("OutgoingBubble");
            if (iter_outgoing_bubble != _node.MemberEnd() && iter_outgoing_bubble->value.IsObject())
            {
                outgoing_bubble_.unserialize(iter_outgoing_bubble->value);
            }
            
            auto iter_date = _node.FindMember("Date");
            if (iter_date != _node.MemberEnd() && iter_date->value.IsObject())
            {
                date_.unserialize(iter_date->value);
            }
            
            auto iter_preview_stickers = _node.FindMember("PreviewsStickers");
            if (iter_preview_stickers != _node.MemberEnd() && iter_preview_stickers->value.IsObject())
            {
                preview_stickers_.unserialize(iter_preview_stickers->value);
            }
            
            auto iter_chat_event = _node.FindMember("ChatEvent");
            if (iter_chat_event != _node.MemberEnd() && iter_chat_event->value.IsObject())
            {
                chat_event_.unserialize(iter_chat_event->value);
            }
            
            auto iter_contact_name = _node.FindMember("ContactName");
            if (iter_contact_name != _node.MemberEnd() && iter_contact_name->value.IsObject())
            {
                contact_name_.unserialize(iter_contact_name->value);
            }
                        
            auto iter_new_messages_plate = _node.FindMember("NewMessagesPlate");
            if (iter_new_messages_plate != _node.MemberEnd() && iter_new_messages_plate->value.IsObject())
            {
                new_messages_plate_.unserialize(iter_new_messages_plate->value);
            }
            
            auto iter_typing = _node.FindMember("Typing");
            if (iter_typing != _node.MemberEnd() && iter_typing->value.IsObject())
            {
                typing_.unserialize(iter_typing->value);
            }
            
            return true;
        }
        
        std::wstring theme::get_theme_folder() const
        {
            std::string folder = std::to_string(theme_id_);
            return core::tools::from_utf8(folder);
        }
        
        std::wstring theme::get_theme_folder(int32_t _theme_id)
        {
            std::string folder = std::to_string(_theme_id);
            return core::tools::from_utf8(folder);
        }
        
        std::wstring theme::get_image_path() const
        {
            return folder_path_ + L"/" + get_theme_folder() + L"/image.jpg";
        }
        
        std::wstring theme::get_thumb_path() const
        {
            return folder_path_ + L"/" + get_theme_folder() + L"/thumb.jpg";
        }
        
        /* download_task */
        download_task::download_task(const std::string& _source_url, const std::wstring& _dest_file, int32_t _theme_id = -1)
            : source_url_(_source_url)
            , dest_file_(_dest_file)
            , set_id_(0)
            , theme_id_(_theme_id)
        {
        }
        
        const std::string& download_task::get_source_url() const
        {
            return source_url_;
        }
        
        const std::wstring& download_task::get_dest_file() const
        {
            return dest_file_;
        }
        
        void download_task::push_request(int64_t _seq)
        {
            requests_.push_back(_seq);
        }
        
        std::list<int64_t> download_task::get_requests() const
        {
            return requests_;
        }
        
        
        
        
        
        /* cache */
        cache::cache(const std::wstring& _themes_path)
        :	themes_path_(_themes_path), base_url_(""), themes_scale_(ThemesScaleNoValue)
        {
        }
        
        bool cache::parse(core::tools::binary_stream& _data, bool _insitu)
        {
            themes_list_.clear();
            
            rapidjson::Document doc;
            const auto& parse_result = (_insitu ? doc.ParseInsitu(_data.read(_data.available())) : doc.Parse(_data.read(_data.available())));
            if (parse_result.HasParseError())
                return false;
            
            auto iter_themes = doc.FindMember("wallpapers");
            if (iter_themes == doc.MemberEnd() || !iter_themes->value.IsArray())
                return false;

            int32_t theme_position = 0;
            for (auto iter_set = iter_themes->value.Begin(); iter_set != iter_themes->value.End(); iter_set++)
            {
                auto new_theme = std::make_shared<themes::theme>(themes_path_);
                
                new_theme->set_position(theme_position++);
                if (!new_theme->unserialize(*iter_set, themes_scale_))
                {
                    puts("error reading theme");
                    continue;
                }
                
                themes_list_.push_back(new_theme);
            }
            
            auto iter_base_url = doc.FindMember("base_url");
            if (iter_base_url != doc.MemberEnd() && iter_base_url->value.IsString())
                base_url_ = iter_base_url->value.GetString();
            
            return true;
        }
        
        std::wstring cache::get_theme_thumb_path(const int32_t _theme_id) const
        {
            return themes_path_ + L"/" + theme::get_theme_folder(_theme_id) + L"/thumb.jpg";
        }
       
        std::string cache::get_theme_thumb_url(const theme& _theme) const
        {
            return base_url_ + _theme.get_thumb_normal_name();
        }
        
        std::wstring cache::get_theme_image_path(const int32_t _theme_id) const
        {
            return themes_path_ + L"/" + theme::get_theme_folder(_theme_id) + L"/image.jpg";
        }
        
        std::string cache::get_theme_image_url(const theme& _theme) const
        {
            return base_url_ + _theme.get_image_normal_name();
        }
        
        std::wstring cache::get_meta_file_name() const
        {
            return themes_path_ + L"/" + themes_meta_file_name_;
        }

        bool cache::get_next_meta_task(download_task& _task)
        {
            if (meta_tasks_.empty())
                return false;
            
            _task = meta_tasks_.front();
            
            return true;
        }

        void cache::make_download_tasks()
        {
            std::string base_url = base_url_;
            for (auto iter = themes_list_.cbegin(); iter != themes_list_.cend(); ++iter)
            {
                std::string thumb_url = get_theme_thumb_url(*(*iter));
                std::wstring thumb_file_name = (*iter)->get_thumb_path();
                meta_tasks_.push_back(download_task(thumb_url, thumb_file_name));
                
                std::string image_url = get_theme_image_url(*(*iter));
                std::wstring image_file_name = (*iter)->get_image_path();
                themes_tasks_.push_back(download_task(image_url, image_file_name));
            }
        }
        
        void cache::make_download_task(theme& _theme)
        {
            auto theme_id = _theme.get_theme_id();
            std::string base_url = base_url_;
            std::string thumb_url = get_theme_thumb_url(_theme);
            std::wstring thumb_file_name = _theme.get_thumb_path();
            auto meta_task = download_task(thumb_url, thumb_file_name, theme_id);
            meta_task.push_request(0);
            meta_tasks_.push_back(meta_task);
            
            std::string image_url = get_theme_image_url(_theme);
            std::wstring image_file_name = _theme.get_image_path();
            auto theme_task = download_task(image_url, image_file_name, theme_id);
            theme_task.push_request(0);
            themes_tasks_.push_back(theme_task);
        }
        
        bool cache::meta_loaded(const download_task& _task)
        {
            for (auto iter = meta_tasks_.begin(); iter != meta_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    meta_tasks_.erase(iter);
                    return true;
                }
            }
            return false;
        }
        
        bool cache::get_next_theme_task(download_task& _task)
        {
            if (themes_tasks_.empty())
                return false;
            
            _task = themes_tasks_.front();
            
            return true;
        }
        
        bool cache::theme_loaded(const download_task& _task, std::list<int64_t>& _requests)
        {
            for (auto iter = themes_tasks_.begin(); iter != themes_tasks_.end(); ++iter)
            {
                if (_task.get_source_url() == iter->get_source_url())
                {
                    _requests = iter->get_requests();
                    
                    themes_tasks_.erase(iter);
                    
                    return true;
                }
            }
            return false;
        }
        
        void cache::serialize_meta_sync(coll_helper _coll)
        {
            if (themes_list_.empty())
                return;
            
            ifptr<iarray> sets_array(_coll->create_array(), true);
            
            for (auto iter_theme = themes_list_.begin(); iter_theme != themes_list_.end(); ++iter_theme)
            {
                coll_helper coll_theme(_coll->create_collection(), true);
                ifptr<ivalue> val_set(_coll->create_value(), true);
                val_set->set_as_collection(coll_theme.get());
                sets_array->push_back(val_set.get());
                coll_theme.set_value_as_int("id", (*iter_theme)->get_theme_id());
                coll_theme.set_value_as_int("position", (*iter_theme)->get_position());
                coll_theme.set_value_as_bool("tile", (*iter_theme)->is_tile());
                coll_theme.set_value_as_string("tint_color", (*iter_theme)->get_tint_color());
                coll_theme.set_value_as_string("typing_color", (*iter_theme)->get_typing_color());
                
                std::wstring file_name = (*iter_theme)->get_thumb_path();
                core::tools::binary_stream bs_thumb;
                if (bs_thumb.load_from_file(file_name))
                {
                    ifptr<istream> thumb(_coll->create_stream(), true);
                    uint32_t file_size = bs_thumb.available();
                    thumb->write((uint8_t*)bs_thumb.read(file_size), file_size);
                    coll_theme.set_value_as_stream("thumb", thumb.get());
                }
                
                coll_helper date_set(_coll->create_collection(), true);
                date_set.set_value_as_string("bg_color", (*iter_theme)->date_.bg_color_);
                date_set.set_value_as_string("text_color", (*iter_theme)->date_.text_color_);
                coll_theme.set_value_as_collection("date", date_set.get());
                
                coll_helper incoming_bubble_set(_coll->create_collection(), true);
                incoming_bubble_set.set_value_as_string("bg1_color", (*iter_theme)->incoming_bubble_.bg1_color_);
                incoming_bubble_set.set_value_as_string("bg2_color", (*iter_theme)->incoming_bubble_.bg2_color_);
                incoming_bubble_set.set_value_as_string("time_color", (*iter_theme)->incoming_bubble_.time_color_);
                coll_theme.set_value_as_collection("incoming_bubble", incoming_bubble_set.get());
                
                coll_helper outgoing_bubble_set(_coll->create_collection(), true);
                outgoing_bubble_set.set_value_as_string("bg1_color", (*iter_theme)->outgoing_bubble_.bg1_color_);
                outgoing_bubble_set.set_value_as_string("bg2_color", (*iter_theme)->outgoing_bubble_.bg2_color_);
                outgoing_bubble_set.set_value_as_string("time_color", (*iter_theme)->outgoing_bubble_.time_color_);
                coll_theme.set_value_as_collection("outgoing_bubble", outgoing_bubble_set.get());
                
                coll_helper preview_stickers_set(_coll->create_collection(), true);
                preview_stickers_set.set_value_as_string("time_color", (*iter_theme)->preview_stickers_.time_color_);
                coll_theme.set_value_as_collection("preview_stickers", preview_stickers_set.get());
                
                coll_helper chat_event_set(_coll->create_collection(), true);
                chat_event_set.set_value_as_string("bg_color", (*iter_theme)->chat_event_.bg_color_);
                chat_event_set.set_value_as_string("text_color", (*iter_theme)->chat_event_.text_color_);
                coll_theme.set_value_as_collection("chat_event", chat_event_set.get());
                
                coll_helper contact_name_set(_coll->create_collection(), true);
                contact_name_set.set_value_as_string("text_color", (*iter_theme)->contact_name_.text_color_);
                coll_theme.set_value_as_collection("contact_name", contact_name_set.get());
                
                coll_helper new_messages_plate_set(_coll->create_collection(), true);
                new_messages_plate_set.set_value_as_string("bg_color", (*iter_theme)->new_messages_plate_.bg_color_);
                new_messages_plate_set.set_value_as_string("text_color", (*iter_theme)->new_messages_plate_.text_color_);
                coll_theme.set_value_as_collection("new_messages_plate", new_messages_plate_set.get());
                
                coll_helper typing_set(_coll->create_collection(), true);
                typing_set.set_value_as_int("light_gif", (*iter_theme)->typing_.light_gif_);
                typing_set.set_value_as_string("text_color", (*iter_theme)->typing_.text_color_);
                coll_theme.set_value_as_collection("typing", typing_set.get());
            }
            
            _coll.set_value_as_array("themes", sets_array.get());
        }
        
        void cache::get_theme_image(int64_t _seq, int32_t _theme_id, tools::binary_stream& _data)
        {
            for (auto iter = themes_tasks_.begin(); iter != themes_tasks_.end(); ++iter)
            {
                if (iter->get_theme_id() == _theme_id)
                {
                    download_task task = *iter;
                    themes_tasks_.erase(iter);
                    task.push_request(_seq);
                    themes_tasks_.push_front(task);
                    
                    return;
                }
            }
            
            if(!_data.load_from_file(get_theme_image_path(_theme_id)))
            {
                for (auto iter = themes_list_.begin(); iter != themes_list_.end(); ++iter)
                {
                    if ((*iter)->get_theme_id() == _theme_id)
                    {
                        make_download_task(*(*iter));
                        break;
                    }
                }
            }
        }
        
        theme* cache::get_theme(int32_t _theme_id)
        {
            for (auto theme : themes_list_)
            {
                if ((*theme).get_theme_id() == _theme_id)
                {
                    return &(*theme);
                }
            }
            return NULL;
        }
        
        void cache::clear_all()
        {
            boost::system::error_code e;
            boost::filesystem::remove_all(themes_path_, e);
        }
    }
}