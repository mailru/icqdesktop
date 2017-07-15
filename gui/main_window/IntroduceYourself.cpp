#include "stdafx.h"

#include "IntroduceYourself.h"
#include "../controls/CommonStyle.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/LineEditEx.h"
#include "../controls/TextEmojiWidget.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"


namespace Ui
{
    IntroduceYourself::IntroduceYourself(const QString& _aimid, const QString& _display_name, QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout *horizontal_layout = Utils::emptyHLayout(this);

        auto main_widget_ = new QWidget(this);
        main_widget_->setStyleSheet("background-color: #ffffff; border: 0;");
        horizontal_layout->addWidget(main_widget_);

        {
            auto global_layout = Utils::emptyVLayout(main_widget_);

            auto upper_widget = new QWidget(this);
            upper_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            global_layout->addWidget(upper_widget);

            auto middle_widget = new QWidget(this);
            middle_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            global_layout->addWidget(middle_widget);

            auto bottom_widget = new QWidget(this);
            bottom_widget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            global_layout->addWidget(bottom_widget);

            auto middle_layout = Utils::emptyVLayout(middle_widget);
            middle_layout->setAlignment(Qt::AlignHCenter);
            {
                auto pl = Utils::emptyHLayout(middle_widget);
                pl->setAlignment(Qt::AlignCenter);
                {
                    auto w = new ContactAvatarWidget(this, _aimid, _display_name, Utils::scale_value(180), true);
                    w->SetMode(ContactAvatarWidget::Mode::MyProfile);

                    w->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
                    pl->addWidget(w);
                    QObject::connect(w, &ContactAvatarWidget::afterAvatarChanged, this, &IntroduceYourself::avatarChanged, Qt::QueuedConnection);
                }
                middle_layout->addLayout(pl);
            }

            {

                name_edit_ = new LineEditEx(middle_widget);
                name_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("placeholders", "Your name"));
                name_edit_->setAlignment(Qt::AlignCenter);
                name_edit_->setMinimumWidth(Utils::scale_value(320));
                name_edit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                name_edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
                name_edit_->setFont(Fonts::appFontScaled(18));
                Utils::ApplyStyle(name_edit_, Ui::CommonStyle::getLineEditStyle());
                middle_layout->addWidget(name_edit_);
            }

            {
                auto horizontalSpacer = new QSpacerItem(0, Utils::scale_value(56), QSizePolicy::Preferred, QSizePolicy::Minimum);
                middle_layout->addItem(horizontalSpacer);

                auto pl = Utils::emptyHLayout(middle_widget);
                pl->setAlignment(Qt::AlignCenter);

                next_button_ = new QPushButton(middle_widget);
                Utils::ApplyStyle(next_button_, Ui::CommonStyle::getDisabledButtonStyle());
                setButtonActive(false);
                next_button_->setFlat(true);
                next_button_->setText(QT_TRANSLATE_NOOP("placeholders", "Continue"));
                next_button_->setCursor(QCursor(Qt::PointingHandCursor));
                next_button_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                connect(next_button_, SIGNAL(clicked()), this, SLOT(accept()), Qt::QueuedConnection);

                name_edit_->setAlignment(Qt::AlignCenter);
                pl->addWidget(next_button_);
                middle_layout->addLayout(pl);
            }

            {
                error_label_ = new QLabel(middle_widget);
                error_label_->setAlignment(Qt::AlignCenter);

                QString error_style = "font-size: 15dip;\
                                      background-color: transparent;\
                                      color: #d0021b;";
                Utils::ApplyStyle(error_label_, error_style);
                UpdateError(false);
                middle_layout->addWidget(error_label_);
            }

            {
                auto bottom_layout = Utils::emptyVLayout(bottom_widget);
                bottom_layout->setAlignment(Qt::AlignHCenter);

                auto horizontalSpacer = new QSpacerItem(0, Utils::scale_value(32), QSizePolicy::Preferred, QSizePolicy::Minimum);
                bottom_layout->addItem(horizontalSpacer);
            }
        }

          connect(name_edit_, &QLineEdit::textChanged, this, &IntroduceYourself::TextChanged);
          connect(next_button_, &QPushButton::clicked, this, &IntroduceYourself::UpdateProfile);
          connect(name_edit_, &LineEditEx::enter, this, &IntroduceYourself::UpdateProfile);
          connect(GetDispatcher(), &Ui::core_dispatcher::updateProfile, this, &IntroduceYourself::RecvResponse);
    }

    void IntroduceYourself::init(QWidget* /*parent*/)
    {

    }

    IntroduceYourself::~IntroduceYourself()
    {

    }

    void IntroduceYourself::UpdateError(bool _is_error)
    {
        if (_is_error)
        {
            error_label_->setText(QT_TRANSLATE_NOOP("placeholders", "Your nickname cannot be longer than 20 symbols"));
        }
        else
        {
            error_label_->setText("");
        }
    }

    void IntroduceYourself::TextChanged()
    {
        int max_nickname_length = 20;

        bool any_non_space = false;

        for (auto symb : name_edit_->text())
        {
            if (!symb.isSpace())
            {
                any_non_space = true;
                break;
            }
        }

        setButtonActive(name_edit_->text().length() <= max_nickname_length
            && !name_edit_->text().isEmpty()
            && any_non_space);
        UpdateError(name_edit_->text().length() > max_nickname_length);
    }

    void IntroduceYourself::setButtonActive(bool _is_active)
    {
        next_button_->setEnabled(_is_active);
        Utils::ApplyStyle(next_button_, _is_active ? Ui::CommonStyle::getGreenButtonStyle() : Ui::CommonStyle::getDisabledButtonStyle());
    }

    void IntroduceYourself::UpdateProfile()
    {
        setButtonActive(false);
        name_edit_->setEnabled(false);
        std::vector<std::pair<std::string, QString>> fields;
        fields.push_back(std::make_pair(std::string("friendlyName"), name_edit_->text()));
        Utils::UpdateProfile(fields);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_name_set);
    }

    void IntroduceYourself::RecvResponse(int _error)
    {
        if (_error == 0)
        {
             emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        else
        {
            name_edit_->setEnabled(true);
            setButtonActive(true);
            error_label_->setText(QT_TRANSLATE_NOOP("placeholders", "Error occured, try again later"));
        }
    }

    void IntroduceYourself::avatarChanged()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::introduce_avatar_changed);
    }
}
