#include "stdafx.h"
#include "exported_data.h"
#include "../legacy/gbase.h"
#include "../legacy/makarov_blowfish.h"
#include "../legacy/login_data.h"
#include "../legacy/http_login_session.h"
#include "../legacy/contact_info.h"
#include "../legacy/const.h"
#include "../../gui.shared/constants.h"

namespace installer
{
    namespace logic
    {
        const std::wstring profile_folder = (build::is_icq() ? L"ICQ-Profile" : L"Mra");
        const wchar_t* im_name_agent = L"Agent";
        const wchar_t* im_name_icq = L"ICQ";

        exported_data::exported_data()
        {
        }


        exported_data::~exported_data()
        {
        }

        std::wstring exported_data::get_profile_folder() const
        {
            std::wstringstream product_profile_path;

            wchar_t app_data[1024] = { 0 };

            if (::SHGetSpecialFolderPath(NULL, app_data, CSIDL_APPDATA, TRUE))
            {
                product_profile_path << app_data << L"\\" << profile_folder;
            }

            return product_profile_path.str();
        }

        std::wstring exported_data::get_options_database_filename() const
        {
            return get_profile_folder() + L"\\Base\\opt.dbs";
        }

        std::wstring exported_data::get_key_file_name() const
        {
            return get_profile_folder() + L"\\Update\\ver.txt";
        }

        std::shared_ptr<wim_account> convert_from_8x(const wim_auth_parameters& _old_account, const MAKFC_CLoginData& _ld)
        {
            auto new_account = std::make_shared<wim_account>();

            new_account->login_ = _ld.GetLogin().NetStrA(CP_UTF8);
            new_account->aimid_ = _old_account.m_aimid.NetStrA(CP_UTF8);
            new_account->token_ = _old_account.m_a_token.NetStrA(CP_UTF8);
            new_account->session_key_ = _old_account.m_session_key.NetStrA(CP_UTF8);
            new_account->devid_ = _old_account.m_dev_id.NetStrA(CP_UTF8);
            new_account->exipired_in_ = _old_account.m_exipired_in;
            new_account->time_offset_ = (time_t) _old_account.m_time_offset;
            new_account->aim_sid_ = _old_account.m_aim_sid.NetStrA(CP_UTF8);
            new_account->fetch_url_ = _old_account.m_fetch_url.NetStrA(CP_UTF8);

            return new_account;
        }

        bool read_avatar_from_file(const std::wstring& _file, std::vector<char>& _data)
        {
            CHandle file(::CreateFile(_file.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
            if (file.m_h == INVALID_HANDLE_VALUE)
                return false;

            unsigned int file_size = ::GetFileSize(file.m_h, 0);
            if (!file_size)
                return false;

            _data.resize(file_size);
            DWORD size_read = 0;
            if (!::ReadFile(file.m_h, &_data[0], file_size, &size_read, 0) || size_read != file_size)
            {
                _data.clear();
                return false;
            }

            return true;
        }
        
        bool read_avatar(const MAKFC_CLoginData& _login_data, const std::wstring& _profile_folder, std::vector<char>& _data)
        {
            static std::vector<std::wstring> file_names;

            if (file_names.empty())
            {
                file_names.reserve(10);
                file_names.push_back(L"avatar_180180.jpg");
                file_names.push_back(L"avatar_120120.jpg");
                file_names.push_back(L"avatar_100100.jpg");
                file_names.push_back(L"avatar_9090.jpg");
                file_names.push_back(L"avatar_6464.jpg");
                file_names.push_back(L"avatar_5050.jpg");
                file_names.push_back(L"avatar_4545.jpg");
                file_names.push_back(L"avatar_3232.jpg");
                file_names.push_back(L"avatar_2222.jpg");
            }

            std::wstring avatars_folder = _profile_folder + L"\\Avatars\\" + (LPCWSTR) _login_data.GetProtocolUID();
            if (build::is_icq())
            {
                avatars_folder += L"###ICQ";
            }

            for (const auto& _file : file_names)
            {
                if (read_avatar_from_file(avatars_folder + L"\\" + _file, _data))
                    return true;
            }

            return false;
        }


        std::string get_nick(MRABase& _base, const MAKFC_CString& _login,  const MAKFC_CString& _database_key)
        {
            std::string nick = _login.NetStrA(CP_UTF8);

            tstring tnick;
            if (_base.ReadString((LPCWSTR) _database_key, L"", L"ICQMyNick", tnick))
            {
                nick = MAKFC_CString(tnick.c_str()).NetStrA(CP_UTF8);
            }
            else
            {
                MAKFC_CContactInfo rci;
                if (_base.RCI_Get((LPCWSTR) _database_key, (LPCWSTR) _login, rci))
                {
                    if (!rci.nickname.IsEmpty())
                    {
                        nick = rci.nickname.NetStrA(CP_UTF8);
                    }
                    else if (!rci.firstname.IsEmpty() || !rci.lastname.IsEmpty())
                    {
                        nick = std::string(rci.firstname.NetStrA(CP_UTF8)) + " " + std::string(rci.lastname.NetStrA(CP_UTF8));
                    }
                } 
            }
                        
            return nick;
        }


        void read_muted_chats(MRABase& _base, const MAKFC_CString& _database_key, OUT std::list<std::string>& _chats)
        {
            _base.ReadMutedChats((LPCWSTR) _database_key, _chats);
        }


        void merge_account(accounts_list& _accounts_list, std::shared_ptr<wim_account> _account)
        {
//             static bool agent_account_exist = false;
// 
//             if (_account->type_ == logic::wim_account::account_type::atAgent)
//             {
//                 if (agent_account_exist)
//                     return;
// 
//                 agent_account_exist = true;
//             }
// 
//             _accounts_list.push_back(_account);


            for (auto _existed_account : _accounts_list)
            {
                if (_existed_account->type_ == _account->type_ && _existed_account->login_ == _account->login_)
                {
                    return;
                }
            }

            _accounts_list.push_back(_account);
        }

        std::string exported_data::read_guid()
        {
            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, legacy::cs_mra_key, KEY_READ) != ERROR_SUCCESS)
                return std::string();

            wchar_t guid[1024];
            DWORD val = 1023;

            if (ERROR_SUCCESS == icq_key.QueryStringValue(L"GUID", guid, &val))
            {
                return MAKFC_CString(guid).NetStrA(CP_UTF8);
            }

            icq_key.Close();

            return std::string();
        }

        void exported_data::convert_auth_params(MRABase& _base, MAKFC_CLoginData& _login_data_conv, bool _is_root_login)
        {
            bool is_icq_login = ((_is_root_login && build::is_icq()) || (_login_data_conv.m_sIMName == im_name_icq));
            bool is_agent_login = ((_is_root_login && build::is_agent()) || (_login_data_conv.m_sIMName == im_name_agent));

            if (is_icq_login)
            {
                std::shared_ptr<wim_auth_parameters> auth_par = load_auth_params_from_db(_base, _login_data_conv);

                if (auth_par)
                {
                    auto converted_params = convert_from_8x(*auth_par, _login_data_conv);
                    converted_params->nick_ = get_nick(_base, _login_data_conv.GetLogin(), auth_par->m_database_key);
                    converted_params->database_key_ = auth_par->m_database_key;
                    read_avatar(_login_data_conv, get_profile_folder(), converted_params->avatar_);
                    converted_params->type_ = wim_account::account_type::atIcq;

                    read_muted_chats(_base, auth_par->m_database_key, converted_params->muted_chats_);

                    merge_account(accounts_list_, converted_params);
                }
            }
            else if (is_agent_login)
            {
                auto converted_params = std::make_shared<wim_account>();
                converted_params->login_ = _login_data_conv.GetLogin().NetStrA(CP_UTF8);
                converted_params->database_key_ = (LPCWSTR) _login_data_conv.GetLogin();
                converted_params->nick_ = get_nick(_base, _login_data_conv.GetLogin(), _login_data_conv.GetLogin());
                read_avatar(_login_data_conv, get_profile_folder(), converted_params->avatar_);

                read_muted_chats(_base,  _login_data_conv.GetLogin(), converted_params->muted_chats_);

                if (_login_data_conv.GetLoginType() != MAKFC_CLoginData::TLoginType::LT_Token)
                {
                    converted_params->password_md5_ = _login_data_conv.m_password_md5;
                }
                else
                {
                    converted_params->token_ = _login_data_conv.GetToken().NetStrA(CP_UTF8);
                }

                converted_params->type_ = wim_account::account_type::atAgent;
                converted_params->guid_ = read_guid();

                merge_account(accounts_list_, converted_params);
            }
        }

        void exported_data::read_accounts(MRABase& _base)
        {
            accounts_list_.reserve(10);
            
            CHandle key_file(::CreateFile(get_key_file_name().c_str(), GENERIC_READ, 
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
            if (key_file.m_h == INVALID_HANDLE_VALUE)
                return;

            BLOWFISHKEY key = {0};
            DWORD dwRead = 0;
            if (!::ReadFile(key_file, &key, sizeof(BLOWFISHKEY), &dwRead, NULL) || dwRead != sizeof(BLOWFISHKEY))
                return;

            LOGINRECORDS records = LOGIN_ConvertPasswords(key);

            for (const auto& login_data : records)
            {
                auto login_data_conv = login_data;
                login_data_conv.SetLogin(login_data.GetLogin().Mid(4), login_data.GetProtocolUID(), login_data.m_nLoginType);

                convert_auth_params(_base, login_data_conv, true);

                for (const MAKFC_CLoginData* login_data_child : login_data.m_imLogins)
                {
                    MAKFC_CLoginData login_data_child_conv(*login_data_child);
                    login_data_child_conv.UnPackLogin();

                    convert_auth_params(_base, login_data_child_conv, false);
                }
            }

            std::sort(accounts_list_.begin(), accounts_list_.end(), [](const std::shared_ptr<wim_account>& _a1, const std::shared_ptr<wim_account>& _a2)->bool
            {
                return (_a1->type_ < _a2->type_);
            });
        }

#define HOTKEY_ENTER		1
#define HOTKEY_DBENTER		2
#define HOTKEY_ALTS			4
#define HOTKEY_CTRLENTER	8


        void exported_data::read_settings(MRABase& _base)
        {
            auto settings = std::make_shared<settings_8x>();

            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, legacy::cs_mra_key, KEY_READ) != ERROR_SUCCESS)
                return;

            DWORD val = 0;
            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_show_cl_in_taskbar", val))
                settings->show_in_taskbar_ = !!val;

            DWORD sounds_scheme = 0;
            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"sound_scheme", sounds_scheme))
            {
                if (!sounds_scheme)
                {
                    settings->enable_sounds_ = false;
                }
            }

            DWORD enable_sounds = 0;
            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_sounds", enable_sounds))
            {
                if (!enable_sounds)
                {
                    settings->enable_sounds_ = false;
                }
            }

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_autosave_incoming_files", val))
                settings->auto_save_files_ = !!val;

            if (!accounts_list_.empty())
            {
                try
                {
                    std::wstring path;
                    if (_base.ReadString((*accounts_list_.begin())->database_key_, L"", L"SAVEPATH_DEFAULTW", path))
                        settings->path_file_save_ = MAKFC_CString(path.c_str()).Trim(L"\\").NetStrA(CP_UTF8);


                    if (_base.ReadDWORD((*accounts_list_.begin())->database_key_, L"", L"ENABLE_PREVIEW", val))
                        settings->enable_preview_ = !!val;
                }
                catch (...)
                {

                }
            }

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_sendhotkey", val))
            {
                if (val & HOTKEY_CTRLENTER)
                    settings->send_hotkey_ = Ui::KeyToSendMessage::Ctrl_Enter;
                else if (val & HOTKEY_DBENTER)
                    settings->send_hotkey_ = 0;
                else
                    settings->send_hotkey_ = 0;
            }

            wchar_t lang[1024];
            val = 1023;
            if (ERROR_SUCCESS == icq_key.QueryStringValue(L"lang", lang, &val))
            {
                settings->language_ = MAKFC_CString(lang).NetStrA(CP_UTF8);
                if (settings->language_ == "ua")
                    settings->language_ = "uk";
                else if (settings->language_ == "cz")
                    settings->language_ = "cs";

            }

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_messagetooltips", val))
                settings->notify_messages_ = !!val;

            if (ERROR_SUCCESS == icq_key.QueryDWORDValue(L"set_autorun", val))
                settings->auto_run_ = !!val;

            settings_ = settings;

            icq_key.Close();
        }

        void exported_data::read(bool _accounts, bool _settings)
        {
            if (!_settings && !_accounts)
                return;

            MRABase base;
            base.Open(get_options_database_filename());

            read_accounts(base);

            if (_settings)
                read_settings(base);

            if (!_accounts)
                accounts_list_.clear();

            base.Close();
        }
        
        const accounts_list& exported_data::get_accounts() const
        {
            return accounts_list_;
        }
        
        void exported_data::add_exported_account(std::shared_ptr<wim_account> _account)
        {
            if (build::is_icq())
            {
                if (_account->type_ == wim_account::account_type::atIcq)
                    exported_accounts_.push_front(_account);
                else
                    exported_accounts_.push_back(_account);
            }
            else if (build::is_agent())
            {
                if (_account->type_ == wim_account::account_type::atIcq)
                    exported_accounts_.push_back(_account);
                else
                    exported_accounts_.push_front(_account);
            }
            
        }


        void exported_data::store_exported_accounts(const QString& _folder_name, bool _is_from_8x)
        {
            if (exported_accounts_.empty())
                return;

            bool is_first = true;

            std::set<std::string> muted_chats;

            for (auto _exported_account : exported_accounts_)
            {
                rapidjson::Document doc(rapidjson::Type::kObjectType);

                auto& a = doc.GetAllocator();

                doc.AddMember("login", _exported_account->login_, a);
                doc.AddMember("aimid", _exported_account->aimid_, a);

                if (_exported_account->type_ == wim_account::account_type::atIcq)
                {
                    doc.AddMember("atoken", _exported_account->token_, a);
                    doc.AddMember("sessionkey", _exported_account->session_key_, a);
                    doc.AddMember("devid", _exported_account->devid_, a);
                    doc.AddMember("expiredin", _exported_account->exipired_in_, a);
                    doc.AddMember("timeoffset", _exported_account->time_offset_, a);
                    doc.AddMember("aimsid", _exported_account->aim_sid_, a);
                    doc.AddMember("fetchurl", _exported_account->fetch_url_, a);
                }
                else
                {
                    if (!_exported_account->token_.empty())
                    {
                        doc.AddMember("agenttoken", _exported_account->token_, a);
                    }

                    doc.AddMember("productguid", _exported_account->guid_, a);
                }

                if (!_exported_account->password_md5_.empty())
                {
                    QByteArray password_d5((const char *) &_exported_account->password_md5_[0], _exported_account->password_md5_.size());
                    doc.AddMember("password_md5", std::string((const char*) password_d5.toHex()), a);
                }

                if (_is_from_8x)
                    doc.AddMember(settings_need_show_promo, true, a);


                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                std::string json_string = buffer.GetString();

                const QString file_name = _folder_name + "/" + (is_first ? auth_export_file_name : auth_export_file_name_merge);

                QFile file(file_name);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return;

                if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
                {
                }

                file.close();

                is_first = false;

                if (_exported_account->muted_chats_.size())
                {
                    for (const std::string& _chat : _exported_account->muted_chats_)
                    {
                        muted_chats.insert(_chat);
                    }
                }
            }

            // store muted chats
            if (!muted_chats.empty())
            {
                rapidjson::Document doc(rapidjson::Type::kArrayType);

                auto& a = doc.GetAllocator();

                for (const std::string& _chat : muted_chats)
                {
                    rapidjson::Value val_chat;
                    val_chat.SetString(_chat, a);

                    doc.PushBack(val_chat, a);
                }

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);

                std::string json_string = buffer.GetString();

                const QString file_name = _folder_name + "/" + muted_chats_export_file_name;

                QFile file(file_name);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return;

                if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
                {
                }

                file.close();

            }
        }

        void exported_data::store_exported_ui_settings(const QString& _file_name, bool /*_is_from_8x*/)
        {
            if (!settings_)
                return;

            rapidjson::Document doc(rapidjson::Type::kObjectType);

            auto& a = doc.GetAllocator();

            doc.AddMember(settings_show_in_taskbar, settings_->show_in_taskbar_, a);
            doc.AddMember(settings_sounds_enabled, settings_->enable_sounds_, a);
            doc.AddMember(settings_download_files_automatically, settings_->auto_save_files_, a);
            if (!settings_->path_file_save_.empty())
                doc.AddMember(settings_download_directory, settings_->path_file_save_, a);
            doc.AddMember(settings_key1_to_send_message, settings_->send_hotkey_, a);
            doc.AddMember(settings_show_video_and_images, settings_->enable_preview_, a);
            doc.AddMember(settings_language, settings_->language_, a);
            doc.AddMember(settings_notify_new_messages, settings_->notify_messages_, a);
                        
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            std::string json_string = buffer.GetString();

            QFile file(_file_name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return;

            if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
            {
            }

            file.close();
        }

        void exported_data::store_exported_core_settings(const QString& _file_name, bool _is_from_8x)
        {
            if (!settings_)
                return;

            rapidjson::Document doc(rapidjson::Type::kObjectType);

            auto& a = doc.GetAllocator();

            if (_is_from_8x)
                doc.AddMember(settings_need_show_promo, true, a);
                        
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            std::string json_string = buffer.GetString();

            QFile file(_file_name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return;

            if (!(json_string.length() == file.write(json_string.c_str(), json_string.length())))
            {
            }

            file.close();
        }

        std::shared_ptr<settings_8x> exported_data::get_settings()
        {
            return settings_;
        }

        exported_data& get_exported_data()
        {
            static exported_data exp_data;

            return exp_data;
        }


        installer::error set_8x_update_downloaded()
        {
            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, legacy::cs_mra_key, KEY_SET_VALUE) != ERROR_SUCCESS)
            {
                return installer::error(errorcode::open_registry_key);
            }

            icq_key.SetDWORDValue(STR_REG_INSTALLER_DOWNLOADED, 1);

            return installer::error();
        }
    }
}