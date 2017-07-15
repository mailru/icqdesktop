#include "stdafx.h"
#include "create_links.h"
#include "../tools.h"
#include "../../utils/links/shell_link.h"

#include <comutil.h>

const wchar_t* str_app_name_shortcut_en = (build::is_icq() ? L"ICQ.lnk" : L"Mail.Ru Agent.lnk");
const wchar_t* str_app_name_en = (build::is_icq() ? L"ICQ" : L"Mail.Ru Agent");
const wchar_t* str_app_user_model_id = (build::is_icq() ? L"ICQ.Client" : L"MailRu.Agent.Client");

namespace installer
{
    namespace logic
    {
        installer::error create_links()
        {
            ::CoInitializeEx(0, COINIT_MULTITHREADED);

            installer::error err;

            CAtlString icq_exe = (const wchar_t*) get_icq_exe().replace('/', '\\').utf16();
            CAtlString installer_exe = (const wchar_t*) get_installer_exe().replace('/', '\\').utf16();
            CAtlString icq_exe_short = (const wchar_t*) get_icq_exe_short().utf16();

            CAtlString program_dir, start, desktop, quick_launch;

            // get spaecial folders
            {
                if (!::SHGetSpecialFolderPath(0, program_dir.GetBuffer(4096), CSIDL_PROGRAMS, 0))
                    return installer::error(errorcode::get_special_folder);
                program_dir.ReleaseBuffer();
                program_dir += CAtlString(L"\\") + (const wchar_t*) get_product_menu_folder().utf16();

                if (!::SHGetSpecialFolderPath(0, start.GetBuffer(4096), CSIDL_STARTMENU, 0))
                    return installer::error(errorcode::get_special_folder);
                start.ReleaseBuffer();

                if (!::SHGetSpecialFolderPath(0, desktop.GetBuffer(4096), CSIDL_DESKTOPDIRECTORY, 0))
                    return installer::error(errorcode::get_special_folder);
                desktop.ReleaseBuffer();

                quick_launch = links::GetQuickLaunchDir();
            }

            links::RemoveFromMFUList(icq_exe_short);

            if (::GetFileAttributes(program_dir) == (DWORD)-1 && !::CreateDirectory(program_dir, NULL))
                return installer::error(errorcode::get_special_folder);

            int iIconIndex = 0;

            CAtlString str = program_dir + L"\\" + str_app_name_shortcut_en;
            if (S_OK != links::CreateLink(icq_exe, L"", str, str_app_name_en, iIconIndex))
            {
                assert(!"link creation was failed!");
            }

            str = program_dir + L"\\Uninstall " + str_app_name_shortcut_en;
            if (S_OK != links::CreateLink(installer_exe, L"-uninstall", str, (LPCWSTR) (CAtlString(L"Uninstall ") + str_app_name_shortcut_en), 1))
            {
                assert(!"link creation was failed!");
            }

            str = start + L"\\" + str_app_name_shortcut_en;
            if (S_OK != links::CreateLink(icq_exe, L"", str, str_app_name_en, iIconIndex))
            {
                assert(!"link creation was failed!");
            }

            str = links::GetQuickLaunchDir();
            str += CAtlString(L"\\") + str_app_name_shortcut_en;
            CComPtr<IShellLink> psl;
            psl.Attach(links::CreateShellLink(icq_exe,(LPCTSTR) L"", str_app_name_en, iIconIndex));
            if (psl == NULL)
            {
                assert(!"link creation was failed!");
            }

            links::SetShellLinkTitle(psl, str_app_name_en);
            links::SetShellLinkAppID(psl, str_app_user_model_id);
            links::SaveShellLink(psl, str);

            str = desktop + L"\\" + str_app_name_shortcut_en;
            psl.Attach(links::CreateShellLink(icq_exe,(LPCTSTR) L"", str_app_name_en, iIconIndex));
            if (psl == NULL)
            {
                assert(!"link creation was failed!");
            }

            links::SetShellLinkTitle(psl, str_app_name_en);
            links::SetShellLinkAppID(psl, str_app_user_model_id);
            links::SaveShellLink(psl, str);

            links::PinToTaskbar(str);
            links::PinToStartmenu(str);

            ::CoUninitialize();

            return err;
        }

        installer::error delete_links()
        {
            ::CoInitializeEx(0, COINIT_MULTITHREADED);

            installer::error err;

            CAtlString icq_exe = (const wchar_t*) get_icq_exe().replace('/', '\\').utf16();
            CAtlString installer_exe = (const wchar_t*) get_installer_exe().replace('/', '\\').utf16();
            CAtlString icq_exe_short = (const wchar_t*) get_icq_exe_short().utf16();

            CAtlString program_dir, start, desktop, quick_launch;

            // get special folders
            {
                if (!::SHGetSpecialFolderPath(0, program_dir.GetBuffer(4096), CSIDL_PROGRAMS, 0))
                    return installer::error(errorcode::get_special_folder);
                program_dir.ReleaseBuffer();
                program_dir += CAtlString(L"\\") + (const wchar_t*) get_product_menu_folder().utf16();

                if (!::SHGetSpecialFolderPath(0, start.GetBuffer(4096), CSIDL_STARTMENU, 0))
                    return installer::error(errorcode::get_special_folder);
                start.ReleaseBuffer();

                if (!::SHGetSpecialFolderPath(0, desktop.GetBuffer(4096), CSIDL_DESKTOPDIRECTORY, 0))
                    return installer::error(errorcode::get_special_folder);
                desktop.ReleaseBuffer();

                quick_launch = links::GetQuickLaunchDir();
            }

            links::RemoveFromMFUList(icq_exe_short);

            CAtlString str = program_dir + L"\\" + str_app_name_shortcut_en;
            links::RemoveLink(str);

            str = program_dir + L"\\Uninstall " + str_app_name_shortcut_en;
            links::RemoveLink(str);

            str = start + _T("\\") + str_app_name_shortcut_en;
            links::RemoveLink(str);

            str = links::GetQuickLaunchDir();
            str += CAtlString(L"\\") + str_app_name_shortcut_en;
            links::RemoveLink(str);

            str = desktop;
            str += CAtlString(L"\\") + str_app_name_shortcut_en;
            links::RemoveLink(str);

            ::CoUninitialize();

            return err;
        }
    }
}