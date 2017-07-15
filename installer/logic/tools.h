#pragma once

namespace installer
{
    namespace logic
    {
        class install_config
        {
            bool uninstall_;
            bool appx_;
            bool uninstalltmp_;
            bool update_;
            bool update_final_;
            bool delete_updates_;
            bool autoupdate_from_8x_;
            bool nolaunch_from_8x_;

            QString folder_for_delete_;

        public:

            install_config()
                :
                uninstall_(false),
                appx_(false),
                uninstalltmp_(false),
                update_(false),
                update_final_(false),
                delete_updates_(false),
                autoupdate_from_8x_(false),
                nolaunch_from_8x_(false)
            {

            }

            bool is_uninstall() const { return uninstall_; }
            void set_uninstall(bool _val) { uninstall_ = _val; }
            bool is_appx() const { return appx_; }
            void set_appx(bool _val) { appx_ = _val; }
            bool is_uninstalltmp() const { return uninstalltmp_; }
            void set_uninstalltmp(bool _val) { uninstalltmp_ = _val; }
            bool is_update() const { return update_; }
            void set_update(bool _val) { update_ = _val; }
            bool is_update_final() const { return update_final_; }
            void set_update_final(bool _val) { update_final_ = _val; }
            bool is_delete_updates() const { return delete_updates_; }
            void set_delete_updates(bool _val) { delete_updates_ = _val; }
            void set_autoupdate_from_8x(bool _val) { autoupdate_from_8x_ = _val; }
            bool is_autoupdate_from_8x() const { return autoupdate_from_8x_; }
            void set_nolaunch_from_8x(bool _val) { nolaunch_from_8x_ = _val; }
            bool is_nolaunch_from_8x() const { return nolaunch_from_8x_; }
            void set_folder_for_delete(const QString& _folder) { folder_for_delete_ = _folder; }
            const QString& get_folder_for_delete() const { return folder_for_delete_; }
        };

        QString get_product_folder();
        QString get_install_folder();
        QString get_updates_folder();
        QString get_icq_exe();
        QString get_icq_exe_short();
        QString get_installer_exe();
        QString get_installer_exe_short();
        QString get_installed_product_path();
        QString get_product_name();
        QString get_product_display_name();
        QString get_product_menu_folder();
        QString get_company_name();
        QString get_exported_account_folder();
        QString get_exported_settings_folder();

        const wchar_t* get_crossprocess_mutex_name();
        const char* get_crossprocess_pipe_name();
        
        const install_config& get_install_config();
        void set_install_config(const install_config& _config);

        translate::translator_base* get_translator();
    }
}
