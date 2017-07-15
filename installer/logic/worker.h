#pragma once

namespace installer
{
    namespace logic
    {
        typedef std::function<void(const installer::error&)> task_callback;

        class worker : public QObject
        {
            Q_OBJECT

Q_SIGNALS:

            void progress(int);
            void error(installer::error);
            void select_account();
            void finish();

        protected:

            int progress_;
            bool delete_8x_files_on_final_;
            bool delete_8x_self_on_final_;

            void reset_watcher();
            std::unique_ptr<QFutureWatcher<installer::error>> current_task_watcher_;

            template <class t_>
            void run_async_function(std::function<t_()> _function_pointer, task_callback _on_complete, int _progress)
            {
                QFuture<installer::error> future = QtConcurrent::run(_function_pointer);
                reset_watcher();
                connect(current_task_watcher_.get(), &QFutureWatcher<installer::error>::finished, [this, _on_complete, _progress]()
                {
                    installer::error err = current_task_watcher_->result();
                    if (!err.is_ok())
                    {
                        emit error(err);
                    }
                    else
                    {
                        progress_ = _progress;
                        emit progress(progress_);

                    }

                    _on_complete(err);
                });

                current_task_watcher_->setFuture(future);
            }

            bool select_account_if_need();

        public:

            void install();
            void final_install(bool _is_from_8x);
            void update();
            void update_final(CHandle& _mutex);
            void uninstall();
            void uninstalltmp();
            void clear_updates();

            void update_from_8x_step_1();
            void update_from_8x_step_2();

            void copy_self_to_bin_8x();

            bool is_delete_8x_files_on_final() const { return delete_8x_files_on_final_; }
            void set_delete_8x_files_on_final(bool _val) { delete_8x_files_on_final_ = _val; }
            bool is_delete_8x_self_on_final() const { return delete_8x_self_on_final_; }
            void set_delete_8x_self_on_final(bool _val) { delete_8x_self_on_final_ = _val; }

            worker();
            virtual ~worker();
        };

        worker* get_worker();
    }
};
