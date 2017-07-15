#include "stdafx.h"
#include "worker.h"

#include "tools.h"

#include "tasks/copy_files.h"
#include "tasks/start_process.h"
#include "tasks/terminate_process.h"
#include "tasks/write_registry.h"
#include "tasks/create_links.h"
#include "tasks/update_from_8x_version.h"
#include "tasks/unistall_8x.h"
#include "exported_data.h"




namespace installer
{
    namespace logic
    {
        worker current_worker;

        worker* get_worker()
        {
            return &current_worker;
        }

        worker::worker()
            :   progress_(0),
                delete_8x_files_on_final_(false),
                delete_8x_self_on_final_(false)
        {
        }

        worker::~worker()
        {
        }

        task_callback dummy_callback = [](const installer::error&){};

        void worker::reset_watcher()
        {
            current_task_watcher_.reset(new QFutureWatcher<installer::error>());
        }

        void worker::final_install(bool _is_from_8x)
        {
            run_async_function<installer::error>(std::bind(store_exported_settings, _is_from_8x), [this, _is_from_8x](const installer::error& /*_err*/)
            {
                std::function<installer::error()> store_exported_account_without_args = std::bind(store_exported_accounts, _is_from_8x);

                run_async_function<installer::error>(store_exported_account_without_args, [this](const installer::error& /*_err*/)
                {
                    run_async_function<installer::error>(start_process, [this](const installer::error& /*_err*/)
                    {

                        if (is_delete_8x_files_on_final())
                        {
                            delete_8x_registry_and_files();
                        }

                        if (is_delete_8x_self_on_final())
                        {
                            run_async_function<installer::error>(delete_self_from_8x, [this](const installer::error& /*_err*/)
                            {

                                emit finish();
                            }, 100);
                        }
                        else
                        {
                            emit finish();
                        }

                    }, 100);

                }, 97);
            }, 93);
        }

        void worker::install()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(export_from_8x, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }
                run_async_function<installer::error>(delete_8x_links, [this](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }
                    
                    run_async_function<installer::error>(terminate_process, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function<installer::error>(copy_files, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            if (get_install_config().is_appx())
                            {
                                run_async_function<installer::error>(create_links, [this](const installer::error& _err)
                                {
                                    if (!_err.is_ok())
                                    {
                                        emit error(_err);
                                        return;
                                    }

                                    emit finish();

                                }, 100);
                            }
                            else
                            {
                                run_async_function<installer::error>(write_registry, [this](const installer::error& _err)
                                {
                                    if (!_err.is_ok())
                                    {
                                        emit error(_err);
                                        return;
                                    }

                                    run_async_function<installer::error>(create_links, [this](const installer::error& _err)
                                    {
                                        if (!_err.is_ok())
                                        {
                                            emit error(_err);
                                            return;
                                        }

                                        set_delete_8x_files_on_final(true);
                                        set_delete_8x_self_on_final(false);

                                        if (select_account_if_need())
                                        {
                                            emit select_account();

                                            return;
                                        }

                                        final_install(!get_exported_data().get_accounts().empty() /* from 8x */);
                                    }, 85);
                                }, 60);
                            }
                        }, 40);
                    }, 30);
                }, 20);
            }, 10 );
        }

        bool worker::select_account_if_need()
        {
            const auto& accounts = logic::get_exported_data().get_accounts();

            int icq_accounts_count = 0, agent_accounts_count = 0;

            std::shared_ptr<logic::wim_account> icq_account;
            std::shared_ptr<logic::wim_account> agent_account;

            for (auto account : accounts)
            {
                if (account->type_ == logic::wim_account::account_type::atIcq)
                {
                    icq_account = account;
                    ++icq_accounts_count;
                }
                else if (account->type_ == logic::wim_account::account_type::atAgent)
                {
                    agent_account = account;
                    ++agent_accounts_count;
                }
            }

            if (icq_accounts_count == 1)
            {
                get_exported_data().add_exported_account(icq_account);
            }

            if (agent_accounts_count == 1)
            {
                get_exported_data().add_exported_account(agent_account);
            }

            if (icq_accounts_count <= 1 && agent_accounts_count <= 1)
            {
                return false;
            }

            return true;
        }

        void worker::uninstall()
        {
            progress_ = 10;
            emit progress(progress_);

            run_async_function<installer::error>(terminate_process, [this](const installer::error&)
            {
                run_async_function<installer::error>(clear_registry, [this](const installer::error&)
                {
                    run_async_function<installer::error>(delete_links, [this](const installer::error&)
                    {
                        run_async_function<installer::error>(delete_files, [this](const installer::error&)
                        {
                            run_async_function<installer::error>(delete_self_and_product_folder, [this](const installer::error&)
                            {
                                emit finish();
                            }, 100);
                        }, 80);
                    }, 60);
                }, 40);
            }, 20);
        }

        void worker::uninstalltmp()
        {
            QString folder_for_delete = get_product_folder();

            if (!get_install_config().get_folder_for_delete().isEmpty())
            {
                folder_for_delete = get_install_config().get_folder_for_delete();
            }

            QDir delete_dir(folder_for_delete);

            for (int i = 0; i < 50; i++)
            {
                if (delete_dir.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::clear_updates()
        {
            QDir dir_updates(get_updates_folder());

            for (int i = 0; i < 50; i++)
            {
                if (dir_updates.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::update()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(copy_files_to_updates, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(write_update_version, [this](const installer::error&)
                {
                    emit finish();
                }, 100);

            }, 95);
        }

        void worker::update_final(CHandle& _mutex)
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(copy_files_from_updates, [this, &_mutex](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(write_to_uninstall_key, [this, &_mutex](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    _mutex.Close();

                    run_async_function<installer::error>(start_process, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function<installer::error>(copy_self_from_updates, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function<installer::error>(delete_updates, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                emit finish();

                            }, 100);
                        }, 90);
                    }, 80);
                }, 75);
            }, 70);
        }

        void worker::update_from_8x_step_1()
        {
            run_async_function<installer::error>(copy_files, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);

                    return;
                }

                run_async_function<installer::error>(set_8x_update_downloaded, [this](const installer::error& /*_err*/)
                {
                    copy_self_to_bin_8x();

                    emit finish();
                }, 100);

            }, 50);
        }

        void worker::update_from_8x_step_2()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(export_from_8x, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(delete_8x_links, [this](const installer::error& /*_err*/)
                {
                    run_async_function<installer::error>(terminate_process, [this](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        run_async_function<installer::error>(write_registry, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function<installer::error>(create_links, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                set_delete_8x_files_on_final(true);
                                set_delete_8x_self_on_final(true);

                                if (select_account_if_need())
                                {
                                    emit select_account();

                                    return;
                                }

                                final_install(true /* from 8x */);
                            }, 85);
                        }, 60);
                    }, 30);
                }, 15);

            }, 10 );
        }

        void worker::copy_self_to_bin_8x()
        {
            copy_self_to_icqim_8x();
        }
    }
}