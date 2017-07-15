#include "stdafx.h"
#include "copy_files.h"

#include "../../utils/7z/7z.h"
#include "../tools.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../legacy/const.h"

namespace installer
{
    namespace logic
    {
        const QString icq_exe_short_filename = "icq.exe";

        installer::error copy_self(const QString& _install_path)
        {
            wchar_t buffer[1025];
            if (!::GetModuleFileName(0, buffer, 1024) || !::CopyFile(buffer, (const wchar_t*)(_install_path + "/" + get_installer_exe_short()).utf16(), FALSE))
                return installer::error(errorcode::copy_installer, QString("copy installer: ") + get_installer_exe());

            return installer::error();
        }

        installer::error copy_files_to_folder(const QString _install_path)
        {
            HINSTANCE instance = (HINSTANCE) ::GetModuleHandle(0);
            if (!instance)
                return installer::error(errorcode::invalid_installer_pack, "get module instance failed");

            HRSRC hrsrc = ::FindResourceW(instance, L"FILES.7z", L"FILES");
            if (!hrsrc)
                return installer::error(errorcode::invalid_installer_pack, "FILES.7Z not found");

            HGLOBAL hgres = ::LoadResource(instance, hrsrc);
            if (!hgres)
                return installer::error(errorcode::invalid_installer_pack, "LoadResource FILES.7Z failed");

            const char* data = (const char*)::LockResource(hgres);
            if (!data)
                return installer::error(errorcode::invalid_installer_pack, "LockResource FILES.7Z failed");

            unsigned int sz = ::SizeofResource(instance, hrsrc);
            if (!sz)
                return installer::error(errorcode::invalid_installer_pack, "SizeofResource FILES.7Z failed");

            zip_pack zip_file;

            if (!zip_file.open(data, sz))
                return installer::error(errorcode::open_files_archive, QString("open archive: files.7z"));

            for (auto iter = zip_file.get_files().begin(); iter != zip_file.get_files().end(); ++iter)
            {
                QString file_name_short = iter->first;
                if (file_name_short == icq_exe_short_filename)
                    file_name_short = get_icq_exe_short();

                QString file_name = _install_path + "/" + file_name_short;

                QString folder_path = QFileInfo(file_name).absoluteDir().absolutePath();

                QDir dir;
                if (!dir.mkpath(folder_path))
                    return installer::error(errorcode::create_product_folder, QString("create folder: ") + folder_path);

                QFile file(file_name);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    return installer::error(errorcode::open_file_for_write, QString("open file: ") + file_name);

                if (!(iter->second->file_size_ == file.write((const char*) iter->second->buffer_, iter->second->file_size_)))
                    return installer::error(errorcode::write_file, QString("write file: ") + file_name);

                file.close();
            }

            return copy_self(_install_path);
        }


        installer::error copy_files()
        {
            return copy_files_to_folder(get_install_folder());
        }

        installer::error copy_files_to_updates()
        {
            return copy_files_to_folder(get_updates_folder() + "/" + VER_FILEVERSION_STR);
        }

        installer::error delete_files()
        {
            QDir dir_product(get_product_folder());

            dir_product.removeRecursively();

            return installer::error();
        }

        installer::error delete_folder_as_temp(const QString& _folder)
        {
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return installer::error(errorcode::get_temp_path, QString("get temp folder"));

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, L"icq", 0, temp_file_name))
                return installer::error(errorcode::get_temp_file_name, QString("get temp folder"));

            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, QString("get this module name"));

            if (!::CopyFile(this_module_name, temp_file_name, FALSE))
                return installer::error(errorcode::copy_installer, QString("copy installer to temp folder"));

            ::MoveFileEx(temp_file_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);

            QString command = QString::fromUtf16((const ushort*) temp_file_name) + " -uninstalltmp " + _folder;
            if (!QProcess::startDetached(command))
                return installer::error(errorcode::start_installer_from_temp, QString("start installer from temp folder"));

            return installer::error();
        }

        installer::error delete_self_from_8x()
        {
            if (build::is_icq())
            {
                wchar_t this_module_name[1024];
                if (!::GetModuleFileName(0, this_module_name, 1024))
                    return installer::error(errorcode::get_module_name, QString("get this module name"));

                QDir dir_module(QString::fromUtf16((const ushort*) this_module_name));

                dir_module.cdUp();

                QString qfolder = dir_module.path();


                return delete_folder_as_temp(qfolder);
            }
            else
            {
                return installer::error();
            }
        }

        installer::error delete_self_and_product_folder()
        {
            return delete_folder_as_temp(get_product_folder());
        }

        installer::error delete_updates()
        {
            wchar_t temp_path[1024];
            if (!::GetTempPath(1024, temp_path))
                return installer::error(errorcode::get_temp_path, QString("get temp folder"));

            wchar_t temp_file_name[1024];
            if (!::GetTempFileName(temp_path, L"icq", 0, temp_file_name))
                return installer::error(errorcode::get_temp_file_name, QString("get temp folder"));

            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, QString("get this module name"));

            if (!::CopyFile(this_module_name, temp_file_name, FALSE))
                return installer::error(errorcode::copy_installer, QString("copy installer to temp folder"));

            ::MoveFileEx(temp_file_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);

            QString command = QString::fromUtf16((const ushort*) temp_file_name) + " " + delete_updates_command;
            if (!QProcess::startDetached(command))
                return installer::error(errorcode::start_installer_from_temp, QString("start installer from temp folder"));

            return installer::error();
        }

        installer::error copy_files_from_updates()
        {
            wchar_t this_module_name[1024];
            if (!::GetModuleFileName(0, this_module_name, 1024))
                return installer::error(errorcode::get_module_name, QString("get this module name"));

            QFileInfo module_info(QString::fromUtf16((const ushort*) this_module_name));

            QDir update_dir = module_info.absoluteDir();

            update_dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
            QFileInfoList files_list = update_dir.entryInfoList();

            for (int i = 0; i < files_list.size(); ++i)
            {
                QFileInfo file_info = files_list.at(i);

                QString file_path = file_info.absoluteFilePath();

                if (file_path != module_info.absoluteFilePath())
                {
                    file_path = QDir::toNativeSeparators(file_path);

                    QString new_file_name = QDir::toNativeSeparators(get_install_folder() + "/" + file_info.fileName());

                    for (int i = 0; i < 10; ++i)
                    {
                        if (::MoveFileEx((const wchar_t*) file_path.utf16(), (const wchar_t*) new_file_name.utf16(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
                            break;

                        ::Sleep(500);
                    }
                }
            }

            return installer::error();
        }

        installer::error copy_self_from_updates()
        {
            return copy_self(get_install_folder());
        }

        installer::error copy_self_to_icqim_8x()
        {
#ifdef _WIN32
            CRegKey icq_key;
            if (ERROR_SUCCESS != icq_key.Open(HKEY_CURRENT_USER, legacy::cs_mra_key, KEY_READ) != ERROR_SUCCESS)
                return installer::error(errorcode::open_registry_key);

            wchar_t install_path[1025];
            DWORD needed = 1024;
            if (ERROR_SUCCESS != icq_key.QueryStringValue(L"InstallPath", install_path, &needed))
                return installer::error(errorcode::get_registry_value);

            return copy_self(QString::fromUtf16((const ushort*) install_path));
#else
            return installer::error();
#endif //_WIN32
        }



    }
}

