#include "stdafx.h"
#include "tools.h"

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#endif //_WIN32

#include "../../../gui/constants.h"

namespace installer
{
    namespace logic
    {
        const QString product_display_name = (build::is_icq() ? "ICQ" : "Mail.Ru Agent");
        const QString product_menu_folder = (build::is_icq() ? "ICQ" : "Mail.Ru");
        const QString product_exe_name = (build::is_icq() ? "icq.exe" : "magent.exe");
        const QString company_name = (build::is_icq() ? "ICQ" : "Mail.Ru");

        QString install_folder;
        QString product_folder;
        QString updates_folder;
        install_config config;

        QString get_appdata_folder()
        {
            QString folder;
#ifdef _WIN32
            wchar_t buffer[1024];

            if (::SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, buffer) == S_OK)
                folder = QString::fromUtf16(reinterpret_cast<const unsigned short*>(buffer));
#endif //_WIN32

            folder = folder.replace('\\', '/');

            return folder;			
        }

        QString get_product_folder()
        {
            if (product_folder.isEmpty())
            {
                product_folder = get_appdata_folder() + "/" + (build::is_icq() ? product_path_icq_a : product_path_agent_a);
            }

            return product_folder;
        }

        QString get_updates_folder()
        {
            if (updates_folder.isEmpty())
            {
                updates_folder = get_product_folder() + "/" + updates_folder_short;
            }

            return updates_folder;
        }

        QString get_install_folder()
        {
            if (install_folder.isEmpty())
            {
                install_folder = get_product_folder() + "/bin";
            }

            return install_folder;
        }

        QString get_icq_exe()
        {
            return (get_install_folder() + "/" + product_exe_name);
        }

        QString get_icq_exe_short()
        {
            return (product_exe_name);
        }

        QString get_installer_exe_short()
        {
            return (build::is_icq() ? installer_exe_name_icq : installer_exe_name_agent);
        }

        QString get_installer_exe()
        {
            return (get_install_folder() + "/" + get_installer_exe_short());
        }

        QString get_exported_account_folder()
        {
            return (get_product_folder() + "/0001/key");
        }

        QString get_exported_settings_folder()
        {
            return (get_product_folder() + "/settings");
        }

        QString get_product_name()
        {
            return (build::is_icq() ? product_name_icq : product_name_agent);
        }

        QString get_product_display_name()
        {
            return product_display_name;
        }

        QString get_product_menu_folder()
        {
            return product_menu_folder;
        }

        QString get_company_name()
        {
            return company_name;
        }

        QString get_installed_product_path()
        {
            QString path;

#ifdef _WIN32

            CRegKey key;
            if (key.Open(HKEY_CURRENT_USER, (LPCTSTR)(QString("Software\\") + get_product_name()).utf16(), KEY_READ) == ERROR_SUCCESS)
            {
                wchar_t registry_path[1025];
                DWORD needed = 1024;
                if (key.QueryStringValue(L"path", registry_path, &needed) == ERROR_SUCCESS)
                {
                    path = QString::fromUtf16((const ushort*) registry_path);
                }
            }

#endif //_WIN32
            return path;
        }

        const install_config& get_install_config()
        {
            return config;
        }

        void set_install_config(const install_config& _config)
        {
            config = _config;
        }

        translate::translator_base* get_translator()
        {
            static translate::translator_base translator;

            return &translator;
        }

        const wchar_t* get_crossprocess_mutex_name()
        {
            return (build::is_icq() ? crossprocess_mutex_name_icq : crossprocess_mutex_name_agent);
        }

        const char* get_crossprocess_pipe_name()
        {
            return (build::is_icq() ? crossprocess_pipe_name_icq : crossprocess_pipe_name_agent);
        }
    }
}
