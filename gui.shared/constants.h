#pragma once

#if defined (_WIN32)
#define CORELIBRARY "corelib.dll"
#elif defined (__APPLE__)
#define CORELIBRARY "libcorelib.dylib"
#elif defined (__linux__)
#define CORELIBRARY "libcorelib.so"
#endif

const QString product_name_icq = "icq.desktop";
const QString product_name_agent = "agent.desktop";

const QString updates_folder_short = "updates";
const QString installer_exe_name_agent = "magentsetup.exe";
const QString installer_exe_name_icq = "icqsetup.exe";
const QString update_final_command = "-update_final";
const QString delete_updates_command = "-delete_updates";
const QString autoupdate_from_8x = "-autoupdate";
const QString nolaunch_from_8x = "-nolaunch";
const QString send_dump_arg = "-send_dump";

#define crossprocess_mutex_name_icq L"{DB17D844-6D3A-4039-968B-0B1D10B8AA81}"
#define crossprocess_mutex_name_agent L"{F54EF375-7D0C-4235-A692-EDE9712B450E}"

#define crossprocess_pipe_name_icq "{4FA385C7-06F7-4C20-A23E-6B0AB2C02FD3}"
#define crossprocess_pipe_name_agent "{B3A5CAD7-9CDE-466F-9CD9-D8DF08B20460}"

#define crossprocess_pipe_name_postfix ".client"
#define crossprocess_message_get_process_id "get_process_id"
#define crossprocess_message_get_hwnd_activate "get_hwnd_and_activate"
#define crossprocess_message_shutdown_process "shutdown_process"
#define crossprocess_message_execute_url_command "execute_url_command"

#define application_user_model_id_icq L"ICQ.Client"
#define application_user_model_id_agent L"MailRu.Agent.Client"

#define url_command_join_livechat "chat"
#define url_command_open_profile "people"
#define url_command_app "app"
#define url_command_livechats_home "livechats"
#define url_command_show_public_livechats "show_public_livechats"


const std::wstring updater_singlton_mutex_name_icq = L"{D7364340-9348-4397-9F56-73CE62AAAEA8}";
const std::wstring updater_singlton_mutex_name_agent = L"{E9B7BB24-D200-401E-B797-E9A85796D506}";


namespace Ui
{
    enum KeyToSendMessage
    {
        Enter = 0x0,
        Shift_Enter = 0x01000020,
        Ctrl_Enter = 0x01000021
    };
}
