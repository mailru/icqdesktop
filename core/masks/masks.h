#pragma once

namespace core
{
    namespace wim
    {
        class im;
    }

    class masks
    {
        struct mask_info;
    public:
        masks(std::weak_ptr<wim::im> _im, const std::wstring& _root_path, unsigned _version);

        void get_mask_id_list(int64_t _seq);
        void get_mask_preview(int64_t _seq, const std::string& mask_id);
        void get_mask_model(int64_t _seq);
        void get_mask(int64_t _seq, const std::string& mask_id);

        void on_connection_restored();

    private:
        void init();

        boost::filesystem::path get_working_dir() const;
        boost::filesystem::path get_json_path() const;
        boost::filesystem::path get_mask_dir(const std::string& _name) const;
        boost::filesystem::path get_mask_version_path(const std::string& _name) const;
        boost::filesystem::path get_mask_preview_path(const std::string& _name, const mask_info& _info) const;
        boost::filesystem::path get_mask_archive_path(const std::string& _name, const mask_info& _info) const;
        boost::filesystem::path get_mask_content_dir(const std::string& _name) const;
        boost::filesystem::path get_mask_json_path(const std::string& _name) const;
        boost::filesystem::path get_model_dir() const;
        boost::filesystem::path get_model_version_path() const;
        boost::filesystem::path get_model_sentry_path() const;
        boost::filesystem::path get_model_archive_path() const;

        bool save_etag(const boost::filesystem::path& _path, const std::string& _etag);

        void post_message_to_gui(int64_t _seq, const std::string& _message) const;
        void post_message_to_gui(int64_t _seq, const std::string& _message, const boost::filesystem::path& _local_path) const;

        void on_model_loading(int64_t _seq);

    private:
        const std::weak_ptr<wim::im> im_;

        const boost::filesystem::path root_;
        const unsigned version_;

        std::string base_url_;

        struct mask_info
        {
            mask_info();
            mask_info(const std::string& _name, const std::string& _archive, const std::string& _preview, const std::string& _etag);

            std::string name_;

            bool downloaded_;
            std::string archive_;
            std::string preview_;

            std::string etag_;
        };

        std::map<std::string, size_t> mask_by_name_;
        std::vector<mask_info> mask_list_;

        std::string model_url_;

        enum class state
        {
            not_loaded,
            loading,
            loaded
        };

        state state_;
    };
}
