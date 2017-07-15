#pragma once

namespace installer
{
    namespace ui
    {
        enum main_window_start_page
        {
            page_start = 0,
            page_progress = 1,
            page_error = 2,
            page_accounts = 3
        };

        class start_page;
        class progress_page;
        class error_page;
        class accounts_page_agent;

        class main_window : public QMainWindow
        {
            Q_OBJECT

        private Q_SLOTS:

            void on_start_install();
            void on_finish();
            void on_error(installer::error);
            void on_close();
            void on_select_account();
            void on_account_selected();

        private:

            QStackedWidget*		pages_;

            start_page* start_page_;
            progress_page* progress_page_;
            error_page* error_page_;
            accounts_page_agent* accounts_page_;

            void start_installation();

        protected:

            virtual void paintEvent(QPaintEvent* _e) override;

        public:

            main_window(main_window_start_page _start_page = main_window_start_page::page_start);
            virtual ~main_window();
        };
    }
}

