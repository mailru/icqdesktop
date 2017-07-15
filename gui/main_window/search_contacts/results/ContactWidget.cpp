#include "stdafx.h"
#include "ContactWidget.h"

#include "../../contact_list/contact_profile.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/utils.h"

const int32_t widget_height	= 100;
const int32_t widget_width	= 260;


namespace Ui
{
	QString getContactInfo(std::shared_ptr<Logic::contact_profile> _profile, const std::map<QString, QString>& _countries)
	{
		QString info;
		QTextStream result(&info);

		QString gender;
		if (_profile->get_gender() == "male")
			gender = QT_TRANSLATE_NOOP("search_widget", "M");
		else if (_profile->get_gender() == "female")
			gender = QT_TRANSLATE_NOOP("search_widget", "F");

		if (!gender.isEmpty())
			result << gender;

        if (_profile->get_birthdate() != 0)
        {
//            QDateTime birthdate = QDateTime::fromMSecsSinceEpoch((qint64) _profile->get_birthdate() * 1000, Qt::LocalTime);
            QDateTime birthdate = QDateTime::fromMSecsSinceEpoch((qint64)_profile->get_birthdate(), Qt::LocalTime);
            
            int age = Utils::calcAge(birthdate);

            if (age > 0)
            {
                if (!info.isEmpty())
                    result << ", ";

                result << age;

                result << " " << Utils::GetTranslator()->getNumberString(
                    age,
                    QT_TRANSLATE_NOOP3("search_widget", "year", "1"),
                    QT_TRANSLATE_NOOP3("search_widget", "years", "2"),
                    QT_TRANSLATE_NOOP3("search_widget", "years", "5"),
                    QT_TRANSLATE_NOOP3("search_widget", "years", "21"));
            }
        }

		auto getAddress = [_profile, &_countries]()->QString
		{
			if (!_profile->get_home_address().get_country().isEmpty())
			{
				auto iter_country = _countries.find(_profile->get_home_address().get_country().toLower());
				if (_countries.end() != iter_country)
					return iter_country->second;

				return _profile->get_home_address().get_country();
			}

			if (!_profile->get_home_address().get_state().isEmpty())
				return _profile->get_home_address().get_state();

			if (!_profile->get_home_address().get_city().isEmpty())
				return _profile->get_home_address().get_city();

			return "";
		};



		QString loc = getAddress();
		if (!loc.isEmpty())
		{
			if (!info.isEmpty())
				result << ", ";

			result << loc;
		}


		return info;
	}

	ContactWidget::ContactWidget(QWidget* _parent, std::shared_ptr<Logic::contact_profile> _profile, const std::map<QString, QString>& _countries)
        :QWidget(_parent),
        profile_(_profile),
        name_(new TextEmojiWidget(this, Fonts::appFontScaled(18), CommonStyle::getTextCommonColor())),
        info_(new TextEmojiWidget(this, Fonts::appFontScaled(14), QColor("#767676"))),
        addButton_(new QPushButton(this)),
        msgButton_(new QPushButton(this)),
        callButton_(new QPushButton(this))
    {
        Utils::grabTouchWidget(this);

        setFixedSize(Utils::scale_value(widget_width), Utils::scale_value(widget_height));

		QHBoxLayout* rootLayout = Utils::emptyHLayout();
        rootLayout->setAlignment(Qt::AlignLeft);

		const QString displayName = _profile->get_contact_name();

		auto avatarWidget = new ContactAvatarWidget(
			this,
			_profile->get_aimid(),
			displayName,
			Utils::scale_value(widget_height),
            false);

        avatarWidget->setCursor(QCursor(Qt::PointingHandCursor));

		rootLayout->addWidget(avatarWidget);

		Utils::grabTouchWidget(avatarWidget);

		QVBoxLayout* infoLayout = Utils::emptyVLayout();
		infoLayout->setContentsMargins(Utils::scale_value(12), 0, 0, 0);
		infoLayout->setAlignment(Qt::AlignTop);

		name_->setObjectName("contact_name");
        name_->setFading(true);
		name_->setText(_profile->get_contact_name());
		infoLayout->addWidget(name_);
		Utils::grabTouchWidget(name_);

		info_->setObjectName("contact_info");
		info_->setText(getContactInfo(_profile, _countries));
		infoLayout->addWidget(info_);
		Utils::grabTouchWidget(info_);

		QHBoxLayout* buttonsLayout = new QHBoxLayout();
		buttonsLayout->setSpacing(Utils::scale_value(12));
		buttonsLayout->setAlignment(Qt::AlignLeft);

		Logic::ContactItem* contactItem = Logic::getContactListModel()->getContactItem(profile_->get_aimid());

		addButton_->setObjectName("add_contact_button");
		addButton_->setVisible(!contactItem);
		addButton_->setCursor(QCursor(Qt::PointingHandCursor));
		buttonsLayout->addWidget(addButton_);
		Utils::grabTouchWidget(addButton_);

		msgButton_->setVisible(!!contactItem);
		msgButton_->setObjectName("msg_contact_button");
		msgButton_->setCursor(QCursor(Qt::PointingHandCursor));
		buttonsLayout->addWidget(msgButton_);
		Utils::grabTouchWidget(msgButton_);

		callButton_->setVisible(!!contactItem);
		callButton_->setObjectName("call_contact_button");
		callButton_->setCursor(QCursor(Qt::PointingHandCursor));
		buttonsLayout->addWidget(callButton_);
		Utils::grabTouchWidget(callButton_);

		infoLayout->addLayout(buttonsLayout);

		rootLayout->addLayout(infoLayout);

		setLayout(rootLayout);

		connect(addButton_, &QPushButton::clicked, [this]()
		{
			emit addContact(profile_->get_aimid());
		});

		connect(msgButton_, &QPushButton::clicked, [this]()
		{
			emit msgContact(profile_->get_aimid());
		});

		connect(callButton_, &QPushButton::clicked, [this]()
		{
			emit callContact(profile_->get_aimid());
		});

        connect(avatarWidget, &ContactAvatarWidget::clickedInternal, [this]()
        {
            emit contactInfo(profile_->get_aimid());
        });
	}


	ContactWidget::~ContactWidget()
	{
	}

	void ContactWidget::onAddResult(bool _res)
	{
		if (_res)
		{
			addButton_->setVisible(false);
			msgButton_->setVisible(true);
			callButton_->setVisible(true);
		}
	}
}

