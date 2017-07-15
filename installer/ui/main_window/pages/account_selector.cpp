#include "stdafx.h"
#include "account_selector.h"
#include "account_widget.h"
#include "../../../logic/exported_data.h"


namespace installer
{
    namespace ui
    {

        accounts_page_agent::accounts_page_agent(QWidget* _parent)
            :   QWidget(_parent),
                agentPageIcon_(0),
                agentPageText_(0),
                icqPageIcon_(0),
                icqPageText_(0),
                accounts_area_widget_(0),
                accounts_area_(0),
                accounts_layout_(0),
                active_step_(-1),
                icq_accounts_count_(0),
                agent_accounts_count_(0),
                selected_icq_accounts_count_(0),
                selected_agent_accounts_count_(0),
                buttonLayout_(0),
                buttonSkip_(0)
        {
            for (auto account : logic::get_exported_data().get_accounts())
            {
                if (account->type_ == logic::wim_account::account_type::atIcq)
                    ++icq_accounts_count_;
                else if (account->type_ == logic::wim_account::account_type::atAgent)
                    ++agent_accounts_count_;
            }

            root_layout_ = new QVBoxLayout();
            {
                root_layout_->setSpacing(0);
                root_layout_->setContentsMargins(dpi::scale(72), dpi::scale(32), dpi::scale(72), dpi::scale(16));
                root_layout_->setAlignment(Qt::AlignTop);

                QLabel* label_choose = new QLabel(this);
                label_choose->setObjectName("choose_account_label");
                label_choose->setText(QT_TR_NOOP("Account settings"));
                label_choose->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                root_layout_->addWidget(label_choose);

                root_layout_->addSpacing(dpi::scale(16));

                QLabel* label_choose_comment = new QLabel(this);
                label_choose_comment->setObjectName("choose_account_comment_label");
                label_choose_comment->setText(QT_TR_NOOP("Now Mail.Ru Agent supports only one account. You can merge it with ICQ one."));
                label_choose_comment->setWordWrap(true);
                label_choose_comment->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                root_layout_->addWidget(label_choose_comment);
                
                root_layout_->addSpacing(dpi::scale(12));

                QHBoxLayout* pagesLayout = new QHBoxLayout();

                pagesLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                agentPageIcon_ = new QWidget(this);
                agentPageIcon_->setObjectName("agent_page_icon");
                agentPageIcon_->setProperty("state", "normal");
                agentPageIcon_->setFixedSize(dpi::scale(12), dpi::scale(12));
                pagesLayout->addWidget(agentPageIcon_);

                pagesLayout->addSpacing(dpi::scale(4));

                agentPageText_ = new QLabel(this);
                agentPageText_->setObjectName("agent_page_text");
                agentPageText_->setText(icq_accounts_count_ > 1 ? QT_TR_NOOP("Mail.Ru Agent") : QT_TR_NOOP("Choose Mail.Ru Agent account"));
                agentPageText_->setProperty("state", "normal");
                pagesLayout->addWidget(agentPageText_);

                pagesLayout->addSpacing(dpi::scale(20));

                icqPageIcon_ = new QWidget(this);
                icqPageIcon_->setObjectName("icq_page_icon");
                icqPageIcon_->setProperty("state", "normal");
                icqPageIcon_->setFixedSize(dpi::scale(12), dpi::scale(12));
                pagesLayout->addWidget(icqPageIcon_);

                pagesLayout->addSpacing(dpi::scale(4));

                icqPageText_ = new QLabel(this);
                icqPageText_->setObjectName("icq_page_text");
                icqPageText_->setText(agent_accounts_count_ > 1 ? QT_TR_NOOP("ICQ") : QT_TR_NOOP("Choose ICQ account"));
                icqPageText_->setProperty("state", "normal");
                pagesLayout->addWidget(icqPageText_);

                pagesLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                root_layout_->addLayout(pagesLayout);

                root_layout_->addSpacing(dpi::scale(12));

                accounts_area_ = new QScrollArea(this);
                accounts_area_widget_ = new QWidget(this);
                accounts_area_widget_->setObjectName("accounts_area_widget");
                accounts_area_->setWidget(accounts_area_widget_);
                accounts_area_->setWidgetResizable(true);
                accounts_area_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                accounts_layout_ = new QVBoxLayout();
                accounts_layout_->setSpacing(0);
                accounts_layout_->setContentsMargins(0, 0, 0, 0);
                accounts_layout_->setAlignment(Qt::AlignTop);

                accounts_area_widget_->setLayout(accounts_layout_);

                root_layout_->addWidget(accounts_area_);

                root_layout_->addSpacing(dpi::scale(16));

                buttonLayout_ = new QHBoxLayout();
                buttonLayout_->setSpacing(dpi::scale(20));
                buttonLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                button_prev_ = new QPushButton(this);
                button_prev_->setContentsMargins(40, 0, 40, 0);
                button_prev_->setFixedHeight(dpi::scale(40));
                button_prev_->setFixedWidth(dpi::scale(120));
                button_prev_->setObjectName("frame_button");
                button_prev_->setText(QT_TR_NOOP("Back"));
                button_prev_->setCursor(QCursor(Qt::PointingHandCursor));
                buttonLayout_->addWidget(button_prev_);
                connect(button_prev_, &QPushButton::clicked, this, &accounts_page_agent::prevClicked);

                button_next_ = new QPushButton(this);
                button_next_->setContentsMargins(40, 0, 40, 0);
                button_next_->setFixedHeight(dpi::scale(40));
                button_next_->setFixedWidth(dpi::scale(120));
                button_next_->setObjectName("custom_button");
                button_next_->setText(QT_TR_NOOP("Next"));
                button_next_->setCursor(QCursor(Qt::PointingHandCursor));
                buttonLayout_->addWidget(button_next_);
                connect(button_next_, &QPushButton::clicked, this, &accounts_page_agent::nextClicked);

                buttonLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                root_layout_->addLayout(buttonLayout_);

                buttonSkip_ = new QPushButton(this);
                buttonSkip_->setText(QT_TR_NOOP("Skip step"));
                buttonSkip_->setCursor(QCursor(Qt::PointingHandCursor));
                buttonSkip_->setObjectName("skip_button");
                buttonSkip_->setVisible(false);
                root_layout_->addWidget(buttonSkip_);
                connect(buttonSkip_, &QPushButton::clicked, this, &accounts_page_agent::skipClicked);

                init_with_step(agent_accounts_count_ > 1 ? 1 : 2);
            }
            setLayout(root_layout_);
        }


        accounts_page_agent::~accounts_page_agent()
        {
        }


        void accounts_page_agent::init_with_step(const int _step)
        {
            active_step_ = _step;

            const auto& accounts = logic::get_exported_data().get_accounts();
            
            logic::wim_account::account_type active_account_type = 
                (_step == 1 ? logic::wim_account::account_type::atAgent : logic::wim_account::account_type::atIcq);

            icq_accounts_count_ = 0;
            agent_accounts_count_ = 0;
            selected_icq_accounts_count_ = 0;
            selected_agent_accounts_count_ = 0;

            for (auto _widget : account_widgets_)
            {
                accounts_layout_->removeWidget(_widget);
                delete _widget;
            }

            account_widgets_.clear();

            for (auto account : accounts)
            {
                if (account->type_ == active_account_type)
                {
                    account_widget* widget = new account_widget(accounts_area_widget_, account);

                    accounts_layout_->addWidget(widget);

                    account_widgets_.push_back(widget);

                    connect(widget, &QPushButton::toggled, [this, widget](bool _checked)
                    {
                        if (_checked)
                        {
                            for (auto _widget : account_widgets_)
                            {
                                if (_widget != widget && _widget->isChecked())
                                {
                                    _widget->setChecked(false);
                                }
                            }
                        }

                        selected_agent_accounts_count_ = 0;
                        selected_icq_accounts_count_ = 0;

                        for (auto _account : logic::get_exported_data().get_accounts())
                        {
                            if (_account->selected_)
                            {
                                if (_account->type_ == logic::wim_account::account_type::atAgent)
                                    selected_agent_accounts_count_ = 1;
                                else if (_account->type_ == logic::wim_account::account_type::atIcq)
                                    selected_icq_accounts_count_ = 1;
                            }
                        }

                        update_tabs();
                        update_buttons();
                    });
                }

                if (account->type_ == logic::wim_account::account_type::atIcq)
                {
                    ++icq_accounts_count_;
                    if (account->selected_)
                        ++selected_icq_accounts_count_;
                }
                else if (account->type_ == logic::wim_account::account_type::atAgent)
                {
                    ++agent_accounts_count_;
                    if (account->selected_)
                        ++selected_agent_accounts_count_;
                }
            }

            buttonSkip_->setVisible(build::is_agent() && active_step_ == 2);

            update_tabs();
            update_buttons();
        }

        void accounts_page_agent::update_tabs()
        {
            if (agent_accounts_count_ <= 1)
            {
                agentPageIcon_->setVisible(false);
                agentPageText_->setVisible(false);
                icqPageIcon_->setVisible(false);
            }

            if (icq_accounts_count_ <= 1)
            {
                icqPageIcon_->setVisible(false);
                icqPageText_->setVisible(false);
                agentPageIcon_->setVisible(false);
            }

            QString state_agent, state_icq;

            switch (active_step_)
            {
                case 1:
                {
                    if (icq_accounts_count_ > 1)
                    {
                        state_agent = (selected_agent_accounts_count_ ? "normalchecked" : "normal");
                        state_icq = (selected_icq_accounts_count_ ? "disabledchecked" : "disabled");
                    }
                    else
                    {
                        state_agent = "gray";
                    }

                    break;
                }
                case 2:
                {
                    if (agent_accounts_count_ > 1)
                    {
                        state_agent = (selected_agent_accounts_count_ ? "disabledchecked" : "disabled");
                        state_icq = (selected_icq_accounts_count_ ? "normalchecked" : "normal");
                    }
                    else
                    {
                        state_icq = "gray";
                    }

                    break;
                }
                default:
                {
                    assert(false);
                    break;
                }
            }

            agentPageIcon_->setProperty("state", state_agent);
            agentPageText_->setProperty("state", state_agent);
            icqPageIcon_->setProperty("state", state_icq);
            icqPageText_->setProperty("state", state_icq);

            agentPageIcon_->setStyle(QApplication::style());
            agentPageText_->setStyle(QApplication::style());
            icqPageIcon_->setStyle(QApplication::style());
            icqPageText_->setStyle(QApplication::style());
        }

        void accounts_page_agent::update_buttons()
        {
            switch (active_step_)
            {
                case 1:
                {
                    button_prev_->setVisible(false);
                    button_next_->setVisible(true);
                    button_next_->setDisabled(selected_agent_accounts_count_ <= 0);
                    break;
                }
                case 2:
                {
                    button_prev_->setVisible(agent_accounts_count_ > 1);
                    button_next_->setVisible(true);
                    break;
                }
                default:
                    break;
            }
        }

        void accounts_page_agent::nextClicked(bool /*_checked*/)
        {
            switch (active_step_)
            {
                case 1:
                {
                    if (icq_accounts_count_ <= 1)
                    {
                        store_accounts_and_exit();
                    }
                    else
                    {
                        init_with_step(2);
                    }

                    break;
                }
                case 2:
                {
                    store_accounts_and_exit();

                    break;
                }
            }
        }

        void accounts_page_agent::skipClicked(bool /*_checked*/)
        {
            for (auto account : logic::get_exported_data().get_accounts())
            {
                if (account->selected_)
                {
                    if (build::is_icq())
                    {
                        if (account->type_ != logic::wim_account::account_type::atAgent)
                        {
                            logic::get_exported_data().add_exported_account(account);
                        }
                    }
                    else
                    {
                        if (account->type_ != logic::wim_account::account_type::atIcq)
                        {
                            logic::get_exported_data().add_exported_account(account);
                        }
                    }
                }
            }

            emit account_selected();
        }

        void accounts_page_agent::store_accounts_and_exit()
        {
            for (auto account : logic::get_exported_data().get_accounts())
            {
                if (account->selected_)
                {
                    logic::get_exported_data().add_exported_account(account);
                }
            }

            emit account_selected();
        }

        void accounts_page_agent::prevClicked(bool /*_checked*/)
        {
            init_with_step(1);
        }
    }
}
