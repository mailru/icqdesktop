#include "stdafx.h"
#include "start_page.h"
#include "../../../utils/styles.h"

namespace installer
{
    namespace ui
    {
        start_page::start_page(QWidget* _parent)
            :	QWidget(_parent)
        {
            QVBoxLayout* root_layout = new QVBoxLayout();
            {
                root_layout->setSpacing(0);
                root_layout->setContentsMargins(0, 0, 0, 0);
                root_layout->setAlignment(Qt::AlignTop);

                root_layout->addSpacing(dpi::scale(48));

                QWidget* logo_widget = new QWidget(this);
                logo_widget->setObjectName(
                    build::is_icq() ? 
                        "logo_icq" : 
                        "logo_agent");
                logo_widget->setFixedHeight(dpi::scale(80));
                root_layout->addWidget(logo_widget);

                QLabel* label_welcome = new QLabel(this);
                label_welcome->setFixedHeight(dpi::scale(48));
                label_welcome->setObjectName(
                    build::is_icq() ? 
                        "start_page_welcome_label_icq" :
                        "start_page_welcome_label_agent");

                label_welcome->setText(
                    build::is_icq() ? 
                        QT_TR_NOOP("Welcome to ICQ") : 
                        QT_TR_NOOP("Welcome to Mail.Ru Agent"));

                label_welcome->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
                root_layout->addWidget(label_welcome);

                QLabel* label_install = new QLabel(this);
                label_install->setFixedHeight(dpi::scale(32));
                label_install->setObjectName("start_page_will_install");

                if (build::is_icq())
                {
                    label_install->setText(QT_TR_NOOP("This will install ICQ on your computer"));
                }
                else
                {
                    label_install->setText(QT_TR_NOOP("This will install Mail.Ru Agent on your computer"));
                }
                

                label_install->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
                root_layout->addWidget(label_install);

                root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

                QHBoxLayout* privacy_layout = new QHBoxLayout();
                privacy_layout->setAlignment(Qt::AlignHCenter);
                {
                    QLabel* label_privacy = new QLabel(this);
                    label_privacy->setText(QT_TR_NOOP("By continuing, you accept the"));
                    label_privacy->setObjectName("start_page_privacy_label");
                    privacy_layout->addWidget(label_privacy);

                    privacy_layout->addSpacing(dpi::scale(4));

                    QLabel* label_privacy_link = new QLabel(this);
                    label_privacy_link->setObjectName("start_page_privacy_link");
                    if (build::is_icq())
                        label_privacy_link->setText(QString("<a href=\"https://www.icq.com/legal/privacypolicy/\" style=\"color: #579e1c;\">") + QT_TR_NOOP("Privacy Policy") + "</a>");
                    else
                        label_privacy_link->setText(QString("<a href=\"https://help.mail.ru/mail-help/UA\" style=\"color: #579e1c;\">") + QT_TR_NOOP("Privacy Policy") + "</a>");
                    label_privacy_link->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
                    label_privacy_link->setOpenExternalLinks(true);
                    privacy_layout->addWidget(label_privacy_link);
                }
                root_layout->addLayout(privacy_layout);
                root_layout->addSpacing(dpi::scale(24));

                QHBoxLayout* button_layout = new QHBoxLayout();
                button_layout->setAlignment(Qt::AlignHCenter);
                {
                    QPushButton* button_install = new QPushButton(this);
                    Testing::setAccessibleName(button_install, "install_button");
                    button_install->setContentsMargins(40, 0, 40, 0);
                    button_install->setFixedHeight(dpi::scale(40));
                    button_install->setObjectName("custom_button");
                    button_install->setText(QT_TR_NOOP("Install"));
                    button_install->setCursor(QCursor(Qt::PointingHandCursor));
                    button_layout->addWidget(button_install);

                    connect(button_install, SIGNAL(clicked(bool)), this, SLOT(on_install_button_click(bool)), Qt::QueuedConnection);

                    button_install->setFocus(Qt::MouseFocusReason);
                    button_install->setDefault(true);
                }
                root_layout->addLayout(button_layout);

                root_layout->addSpacing(dpi::scale(16));
            }


            setLayout(root_layout);
        }

        start_page::~start_page()
        {

        }

        void start_page::on_install_button_click(bool)
        {
            emit start_install();
        }
    }
}
