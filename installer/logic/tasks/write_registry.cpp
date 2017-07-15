#include "stdafx.h"
#include "write_registry.h"
#include "../tools.h"
#include "../../../common.shared/version_info_constants.h"
#include <regex>
#include "../exported_data.h"

namespace installer
{
    namespace logic
    {
        installer::error write_to_uninstall_key()
        {
            CAtlString uninstall_command = (LPCWSTR)(QString("\"") + get_installer_exe().replace('/', '\\') + "\"" + " -uninstall").utf16();
            CAtlString display_name = (LPCWSTR)(get_product_display_name() + " (" + QT_TRANSLATE_NOOP("write_to_uninstall", "version") + " " + VERSION_INFO_STR + ")").utf16();
            CAtlString exe = (LPCWSTR) get_icq_exe().replace('/', '\\').utf16();
            CAtlString install_path = (LPCWSTR) get_install_folder().replace('/', '\\').utf16();

            CRegKey key_current_version;
            if (ERROR_SUCCESS != key_current_version.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software\\Microsoft\\Windows\\CurrentVersion");

            CRegKey key_uninstall;
            if (ERROR_SUCCESS != key_uninstall.Create(key_current_version, L"Uninstall"))
                return installer::error(errorcode::create_registry_key, QString("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));

            CRegKey key_icq;
            if (ERROR_SUCCESS != key_icq.Create(key_uninstall, (LPCWSTR) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, QString("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + get_product_name());

            QString version = VERSION_INFO_STR;
            QString major_version, minor_version;
            QStringList version_list = version.split('.');
            if (version_list.size() > 2)
            {
                major_version = version_list[0];
                minor_version = version_list[1];
            }


            if (
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayName", display_name) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayIcon", CAtlString(L"\"") + exe + L"\"") ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"DisplayVersion", (LPCWSTR) version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"VersionMajor", (LPCWSTR) major_version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"VersionMinor", (LPCWSTR) minor_version.utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"Publisher", (LPCWSTR) get_company_name().utf16()) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"InstallLocation", install_path) ||
                ERROR_SUCCESS != key_icq.SetStringValue(L"UninstallString", uninstall_command)
                )
            {
                return installer::error(errorcode::set_registry_value, QString("set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + get_product_name() + "\\...");
            }

            return installer::error();
        }

        CAtlString get_default_referer()
        {
            return (const wchar_t*) get_product_name().utf16();
        }

        CAtlString get_referer()
        {
            CAtlString referer = get_default_referer();

            wchar_t buffer[1025] = {0};

            if (::GetModuleBaseName(::GetCurrentProcess(), 0, buffer, 1024))
            {
                CAtlString module_name = buffer;

                std::wcmatch mr; 
                std::wregex rx(L"[^a-zA-Z0-9_-]");
                if (std::regex_search((const wchar_t*) module_name, mr, rx))
                {
                    CAtlString omit = mr[0].first;
                    module_name = module_name.Mid(0, module_name.Find(omit));
                }

                CAtlString referer_prefix = L"_rfr";
                int rfr_pos = module_name.Find(referer_prefix);
                if (rfr_pos != -1)
                {
                    referer = module_name.Mid(rfr_pos + referer_prefix.GetLength());
                }
            }

            return referer;
        }

        void write_referer(CRegKey& _key_product)
        {
            CAtlString referer = get_referer();

            _key_product.SetStringValue(L"referer", referer);

            ULONG len = 1024;
            wchar_t buffer[1025];
            if (_key_product.QueryStringValue(L"referer_first", buffer, &len) != ERROR_SUCCESS)
                _key_product.SetStringValue(L"referer_first", referer);
        }

        installer::error write_registry()
        {
            installer::error err;

            QString exe_path = get_icq_exe();
            exe_path = exe_path.replace('/', '\\');
            CRegKey key_software;

            auto error = write_registry_autorun();
            if (!error.is_ok())
                return error;

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software");

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, QString("create registry key: ") + get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"path", (const wchar_t*) exe_path.utf16()))
                return installer::error(errorcode::set_registry_value, QString("set registry value: Software\\") + get_product_name() + "path");

            write_referer(key_product);

            HKEY key_cmd, key_icq = 0, key_icon = 0;
            DWORD disp = 0;

            CAtlString sub_path = (build::is_icq() ? L"Software\\Classes\\icq" : L"Software\\Classes\\magent");
            CAtlString command_string = CAtlString("\"") + (const wchar_t*) exe_path.utf16() + L"\" -urlcommand \"%1\"";
            CAtlString url_link_string = (build::is_icq() ? L"URL:ICQ Link" : L"URL:Agent Link");

            if (ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_CURRENT_USER, (LPCWSTR) (sub_path + L"\\shell\\open\\command"), 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_cmd, &disp)) 
            {
                ::RegSetValueEx(key_cmd, NULL, 0, REG_SZ, (LPBYTE) command_string.GetBuffer(), sizeof(TCHAR)*(command_string.GetLength() + 1));
                
                if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, (LPCWSTR) sub_path, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &key_icq))
                {
                    ::RegSetValueEx(key_icq, NULL, 0, REG_SZ, (LPBYTE) url_link_string.GetBuffer(), sizeof(TCHAR)*(url_link_string.GetLength() + 1));
                    ::RegSetValueEx(key_icq, L"URL Protocol", 0, REG_SZ, (LPBYTE) L"", sizeof(TCHAR));


                    if (ERROR_SUCCESS == ::RegCreateKeyEx(key_icq, L"DefaultIcon", 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_icon, &disp))
                    {
                        CAtlString icon_path = (const wchar_t*) exe_path.utf16();

                        ::RegSetValueEx(key_icon, NULL, 0, REG_SZ, (LPBYTE) icon_path.GetBuffer(), sizeof(TCHAR)*(icon_path.GetLength() + 1));

                        ::RegCloseKey(key_icon);
                    }

                    ::RegCloseKey(key_icq);
                }

                ::RegCloseKey(key_cmd);
            }

            err = write_to_uninstall_key();
            if (!err.is_ok())
                return err;

            return err;
        }

        installer::error write_registry_autorun()
        {
            QString exe_path = get_icq_exe();
            exe_path = exe_path.replace('/', '\\');

            CRegKey key_software_run;
            if (ERROR_SUCCESS != key_software_run.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE | KEY_QUERY_VALUE))
                return installer::error(errorcode::open_registry_key, "open registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Run");

            bool haveAutorun = false;
            {
                ULONG len = 1024;
                wchar_t buffer[1025];
                if (key_software_run.QueryStringValue((const wchar_t*) get_product_name().utf16(), buffer, &len) == ERROR_SUCCESS)
                    haveAutorun = true;
            }

            if (!get_exported_data().get_settings() || get_exported_data().get_settings()->auto_run_ || haveAutorun)
            {
                if (ERROR_SUCCESS != key_software_run.SetStringValue((const wchar_t*) get_product_name().utf16(), (const wchar_t*)(QString("\"") + exe_path + "\"" + " /startup").utf16()))
                    return installer::error(errorcode::set_registry_value, QString("set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Run ") + get_product_name());
            }

            return installer::error();
        }

        installer::error clear_registry()
        {
            CAtlString key_product = CAtlString(L"Software\\") + (const wchar_t*) get_product_name().utf16();

            ::SHDeleteKey(HKEY_CURRENT_USER, key_product);
            ::SHDeleteKey(HKEY_CURRENT_USER, (LPCWSTR)(CAtlString(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + (const wchar_t*) get_product_name().utf16()));
            ::SHDeleteValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", (const wchar_t*) get_product_name().utf16());
            ::SHDeleteKey(HKEY_CURRENT_USER, CAtlString(L"Software\\Classes\\") + (build::is_icq() ? L"icq" : L"magent"));

            return installer::error();
        }

        installer::error write_update_version()
        {
            CRegKey key_software;

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, "open registry key: Software");

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, QString("create registry key: ") + get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"update_version", (const wchar_t*) QString(VERSION_INFO_STR).utf16()))
                return installer::error(errorcode::set_registry_value, "set registry value: Software\\icq.desktop update_version");

            return installer::error();
        }
    }
}