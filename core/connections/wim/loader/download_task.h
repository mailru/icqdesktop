#ifndef __WIM_DOWNLOAD_TASK_H_
#define __WIM_DOWNLOAD_TASK_H_

#pragma once

#include "fs_loader_task.h"

enum class loader_errors;

namespace core
{
    namespace wim
    {
        struct wim_packet_params;
        struct download_progress_handler;
        class web_file_info;

        class download_task : public fs_loader_task, public std::enable_shared_from_this<download_task>
        {
            std::unique_ptr<web_file_info> info_;
            std::ofstream file_stream_;

            std::wstring files_folder_;
            std::wstring previews_folder_;
            std::wstring file_name_temp_;
            std::wstring filename_;

            std::shared_ptr<download_progress_handler> handler_;

            std::shared_ptr<web_file_info> make_info();

            std::wstring get_info_file_name() const;

            virtual void resume(loader& _loader) override;

        public:

            // core thread functions

            download_task(
                const std::string &_id,
                const wim_packet_params& _params,
                const std::string& _file_url,
                const std::wstring& _files_folder,
                const std::wstring& _previews_folder,
                const std::wstring& _filename);

            virtual ~download_task();

            static bool get_file_id(const std::string& _file_url, std::string& _file_id);

            void set_handler(std::shared_ptr<download_progress_handler> _handler);
            std::shared_ptr<download_progress_handler> get_handler();

            virtual void on_result(int32_t _error) override;
            virtual void on_progress() override;

            bool is_end();

            bool load_metainfo_from_local_cache();
            bool is_downloaded_file_exists();

            void set_played(bool played);
            
            std::string get_file_direct_url() const;

            // download thread functions
            loader_errors download_metainfo();
            loader_errors open_temporary_file();
            loader_errors load_next_range();
            loader_errors get_preview_2k();
            loader_errors get_preview();
            loader_errors on_finish();
            int32_t copy_if_needed();
            bool serialize_metainfo();
            void delete_metainfo_file();

        };

    }
}

#endif //__WIM_DOWNLOAD_TASK_H_