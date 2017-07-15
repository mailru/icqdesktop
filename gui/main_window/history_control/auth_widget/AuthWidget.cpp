#include "stdafx.h"
#include "AuthWidget.h"

#include "../../contact_list/contact_profile.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../core_dispatcher.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/utils.h"

namespace
{
    const int AVATAR_SIZE = 180;
    const int AVATAR_UPPER_SPACE = 40;
    const int NAME_FONTSIZE = 32;
    const int INFO_FONTSIZE = 16;
    const int BUTTON_FONTSIZE = 18;

    const QString AUTH_WIDGET_STYLE =
        "background-color: #e5ffffff;"
        "border-width: 0dip;"
        "border-style: solid;"
        "border-radius: 8dip;";

    const QString BUTTON_ADD_STYLE = QString(
        "QPushButton { background-color: transparent; border: none; color: %1; }"
        "QPushButton:hover { color: %2; }"
        "QPushButton:pressed { color: %3; } ")
        .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColor()))
        .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorHovered()))
        .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getLinkColorPressed()));

    const QString RED_BUTTON_STYLE = QString(
        "QPushButton { background-color: transparent; border: none; color: %1; }"
        "QPushButton:hover { color: %2; }")
        .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getRedLinkColor()))
        .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getRedLinkColorHovered()));
}

namespace Ui
{

    ContactInfoWidget::ContactInfoWidget(QWidget* _parent, const QString& _aimid)
        :	QWidget(_parent),
        aimid_(_aimid),
        ref_(new bool(false)),
        name_(new TextEmojiWidget(this, Fonts::appFontScaled(NAME_FONTSIZE, Fonts::FontWeight::Light), CommonStyle::getTextCommonColor(), Utils::scale_value(38))),
        info_(new TextEmojiWidget(this, Fonts::appFontScaled(INFO_FONTSIZE, Fonts::FontWeight::Light), CommonStyle::getTextCommonColor(), Utils::scale_value(27) - name_->descent()))
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

        QVBoxLayout* rootLayout = Utils::emptyVLayout();
        rootLayout->setContentsMargins(0, 0, 0, Utils::scale_value(22));
        rootLayout->setAlignment(Qt::AlignTop);

        rootLayout->addSpacing(Utils::scale_value(AVATAR_SIZE - AVATAR_UPPER_SPACE));

        name_->setText(_aimid, TextEmojiAlign::allign_center);
        name_->setStyleSheet("background-color: transparent");
        info_->setText("", TextEmojiAlign::allign_center);
        info_->setStyleSheet("background-color: transparent;");
        rootLayout->addWidget(name_);
        rootLayout->addWidget(info_);

        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->addSpacing(Utils::scale_value(40));

        QPushButton* buttonAdd = new QPushButton(this);
        buttonAdd->setFont(Fonts::appFontScaled(BUTTON_FONTSIZE));
        Utils::ApplyStyle(buttonAdd, BUTTON_ADD_STYLE);
        buttonAdd->setText(QT_TRANSLATE_NOOP("auth_widget","Add"));
        buttonAdd->setCursor(Qt::PointingHandCursor);
        connect(buttonAdd, &QPushButton::clicked, [this]() { emit addContact(); });

        QPushButton* buttonSpam = new QPushButton(this);
        buttonSpam->setFont(Fonts::appFontScaled(BUTTON_FONTSIZE));
        Utils::ApplyStyle(buttonSpam, RED_BUTTON_STYLE);
        buttonSpam->setText(QT_TRANSLATE_NOOP("auth_widget", "Block"));
        buttonSpam->setCursor(Qt::PointingHandCursor);
        connect(buttonSpam, &QPushButton::clicked, [this]() { emit spamContact(); });

        QPushButton* buttonDelete = new QPushButton(this);
        buttonDelete->setFont(Fonts::appFontScaled(BUTTON_FONTSIZE));
        Utils::ApplyStyle(buttonDelete, RED_BUTTON_STYLE);
        buttonDelete->setText(QT_TRANSLATE_NOOP("auth_widget", "Delete"));
        buttonDelete->setCursor(Qt::PointingHandCursor);
        connect(buttonDelete, &QPushButton::clicked, [this]() { emit deleteContact(); });

        buttonsLayout->addWidget(buttonAdd);
        buttonsLayout->addWidget(buttonSpam);
        buttonsLayout->addWidget(buttonDelete);

        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        buttonsLayout->setSpacing(Utils::scale_value(40));

        buttonsLayout->addSpacing(Utils::scale_value(40));

        rootLayout->addSpacing(Utils::scale_value(12));
        rootLayout->addLayout(buttonsLayout);

        std::weak_ptr<bool> wr_ref = ref_;

        Logic::getContactListModel()->getContactProfile(aimid_, [this, wr_ref](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            auto ref = wr_ref.lock();
            if (!ref)
                return;

            if (_profile)
                initFromProfile(_profile);
        });

        setLayout(rootLayout);
    }

    void ContactInfoWidget::initFromProfile(Logic::profile_ptr _profile)
    {
        name_->setText(_profile->get_contact_name(), TextEmojiAlign::allign_center);

        QString info;
        QTextStream ss_info(&info);

        QString from;

        if (!_profile->get_home_address().get_city().isEmpty())
            from = _profile->get_home_address().get_city();
        else if (!_profile->get_home_address().get_state().isEmpty())
            from = _profile->get_home_address().get_state();
        else if (!_profile->get_home_address().get_country().isEmpty())
            from = _profile->get_home_address().get_country();

        if (!from.isEmpty())
            ss_info << from;

        if (_profile->get_birthdate() != 0)
        {
            QDateTime birthdate = QDateTime::fromMSecsSinceEpoch((qint64) _profile->get_birthdate() * 1000, Qt::LocalTime);

            int age = Utils::calcAge(birthdate);

            if (age > 0)
            {
                if (!from.isEmpty())
                    ss_info << " - ";

                ss_info << age << " " << Utils::GetTranslator()->getNumberString(
                    age,
                    QT_TRANSLATE_NOOP3("auth_widget", "year", "1"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "2"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "5"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "21"));
            }
        }

        info_->setText(info, TextEmojiAlign::allign_center);
    }

    void ContactInfoWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }




    AuthWidget::AuthWidget(QWidget* _parent, const QString& _aimid)
        :	QWidget(_parent),
        rootLayout_(new QVBoxLayout()),
        avatar_(new ContactAvatarWidget(this, _aimid, Logic::getContactListModel()->getDisplayName(_aimid), Utils::scale_value(AVATAR_SIZE), true)),
        aimid_(_aimid),
        infoWidget_(new ContactInfoWidget(this, _aimid))
    {
        avatar_->setCursor(Qt::PointingHandCursor);

        Utils::ApplyStyle(infoWidget_, AUTH_WIDGET_STYLE);

        rootLayout_->setSpacing(0);
        rootLayout_->setContentsMargins(0, Utils::scale_value(AVATAR_UPPER_SPACE), 0, 0);

        QHBoxLayout* infoLayout = Utils::emptyHLayout();
        infoLayout->setAlignment(Qt::AlignHCenter);

        auto h_spacer_l = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        auto h_spacer_r = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        infoLayout->addSpacerItem(h_spacer_l);
        infoLayout->addWidget(infoWidget_);
        infoLayout->addSpacerItem(h_spacer_r);

        rootLayout_->addLayout(infoLayout);

        setLayout(rootLayout_);

        avatar_->raise();

        connect(avatar_, &ContactAvatarWidget::clickedInternal, this, &AuthWidget::avatarClicked, Qt::QueuedConnection);
        connect(infoWidget_, &ContactInfoWidget::addContact, [this](){ emit addContact(aimid_); });
        connect(infoWidget_, &ContactInfoWidget::spamContact, [this](){ emit spamContact(aimid_); });
        connect(infoWidget_, &ContactInfoWidget::deleteContact, [this](){ emit deleteContact(aimid_); });
    }

    AuthWidget::~AuthWidget()
    {
    }

    void AuthWidget::placeAvatar()
    {
        QRect rc = geometry();

        int x = (rc.width() - Utils::scale_value(AVATAR_SIZE)) / 2;
        int y = 0;

        avatar_->move(x, y);
    }

    void AuthWidget::avatarClicked()
    {
        emit Utils::InterConnector::instance().profileSettingsShow(aimid_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_auth_widget);
    }

    void AuthWidget::resizeEvent(QResizeEvent * _e)
    {
        placeAvatar();

        QWidget::resizeEvent(_e);
    }


}