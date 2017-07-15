#pragma once

namespace installer
{
    namespace logic
    {
        struct wim_account;
    }

    namespace ui
    {
        class account_widget : public QPushButton
        {
            Q_OBJECT

            private Q_SLOTS:

        Q_SIGNALS:

        private:

            std::shared_ptr<logic::wim_account> account_;

            virtual void paintEvent(QPaintEvent* _e) override;

        public:

            account_widget(QWidget* _parent, std::shared_ptr<logic::wim_account> _account);
            virtual ~account_widget();
        };
    }
}

