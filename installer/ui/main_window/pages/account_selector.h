#pragma once

namespace installer
{
    namespace ui
    {
        class account_widget;

        class accounts_page_agent : public QWidget
        {
            Q_OBJECT

        private Q_SLOTS:

            void nextClicked(bool _checked);
            void skipClicked(bool _checked);
            void prevClicked(bool _checked);

        Q_SIGNALS:

            void account_selected();

        private:

            int active_step_;
            int icq_accounts_count_;
            int agent_accounts_count_;
            int selected_icq_accounts_count_;
            int selected_agent_accounts_count_;

            QVBoxLayout* root_layout_;
            QHBoxLayout* buttonLayout_;

            QWidget* agentPageIcon_;
            QLabel* agentPageText_;
            QWidget* icqPageIcon_;
            QLabel* icqPageText_;

            QPushButton* button_prev_;
            QPushButton* button_next_;

            QPushButton* buttonSkip_;

            QWidget* accounts_area_widget_;
            QScrollArea* accounts_area_;
            QVBoxLayout* accounts_layout_;

            std::vector<account_widget*> account_widgets_;

        public:

            void update_tabs();
            void update_buttons();
            void init_with_step(const int _step);
            void store_accounts_and_exit();

            accounts_page_agent(QWidget* _parent);
            virtual ~accounts_page_agent();
        };

    }
}


