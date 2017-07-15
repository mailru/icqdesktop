#include "stdafx.h"
#include "account_widget.h"
#include "../../../logic/exported_data.h"


namespace installer
{
    namespace ui
    {
        bool is_number(const std::string& _value)
        {
            for (auto c : _value)
            {
                if (!isdigit(c))
                    return false;
            }

            return true;
        }

        std::string format_uin(const std::string& _uin)
        {
            if (!is_number(_uin))
                return _uin;

            std::stringstream ss_uin;

            int counter = 0;
            for (char c : _uin)
            {
                if (counter >= 3)
                {
                    ss_uin << '-';
                    counter = 0;
                }

                counter++;
                ss_uin << c;
            }


            return ss_uin.str();
        }

        QPixmap RoundImage(const QPixmap &img, int _width, int _height)
        {
            int scale = std::min(_width, _height);
            QImage imageOut(QSize(scale, scale), QImage::Format_ARGB32);
            imageOut.fill(Qt::transparent);

            QPainter painter(&imageOut);

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            QPainterPath path(QPointF(0,0));
            path.addEllipse(0, 0, scale, scale);

            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scale, scale, img);

            QPixmap pixmap = QPixmap::fromImage(imageOut);

            return pixmap;
        }

        QPixmap draw_avatar(const logic::wim_account& _account, int _width, int _height)
        {
            QPixmap avatar;
            if (!_account.avatar_.empty())
                avatar.loadFromData((const uchar*) &_account.avatar_[0], _account.avatar_.size());
            else
                avatar.load(":/images/content_avatarplaceholder.png");

            return RoundImage(avatar, _width, _height);
        }

        account_widget::account_widget(QWidget* _parent, std::shared_ptr<logic::wim_account> _account)
            :   QPushButton(_parent),
                account_(_account)
        {
            const int account_widget_height = dpi::scale(60);

            setFixedHeight(account_widget_height);
            setCheckable(true);
            setCursor(Qt::PointingHandCursor);

            const int avatar_h = dpi::scale(44);

            QHBoxLayout* root_layout = new QHBoxLayout();
            root_layout->setSpacing(0);
            root_layout->setContentsMargins(0, 0, 0, 0);

            QLabel* avatar = new QLabel(this);
            avatar->setPixmap(draw_avatar(*_account, avatar_h, avatar_h));
            avatar->setFixedHeight(avatar_h);
            avatar->setFixedWidth(avatar_h);
            root_layout->addWidget(avatar);

            root_layout->addSpacing(dpi::scale(12));

            QVBoxLayout* name_layout = new QVBoxLayout();
            {
                name_layout->addSpacing(dpi::scale(4));
                name_layout->setAlignment(Qt::AlignTop);

                QLabel* name_label = new QLabel(this);
                name_label->setText(_account->nick_.c_str());
                name_label->setObjectName("label_account_nick");

                name_layout->addWidget(name_label);
                name_layout->addSpacing(8);

                QLabel* uin_label = new QLabel(this);
                uin_label->setText(format_uin(_account->login_).c_str());
                uin_label->setObjectName("label_account_uin");

                name_layout->addWidget(uin_label);
            }
            root_layout->addLayout(name_layout);

            setLayout(root_layout);

            if (account_->selected_)
                setChecked(true);

            connect(this, &QPushButton::toggled, [this](bool _checked)
            {
                account_->selected_ = _checked;
            });
        }

        account_widget::~account_widget()
        {
        }

        void account_widget::paintEvent(QPaintEvent* _e)
        {
            QStyleOption opt;
            opt.init(this);
            QPainter p(this);
            style()->drawPrimitive (QStyle::PE_Widget, &opt, &p, this);

            return QPushButton::paintEvent(_e);
        }
    }
}