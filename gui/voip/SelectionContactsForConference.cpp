#include "stdafx.h"
#include "SelectionContactsForConference.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/SearchWidget.h"
#include "../main_window/contact_list/ContactListItemDelegate.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/GroupChatOperations.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../core_dispatcher.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/SearchMembersModel.h"
#include "../main_window/sidebar/SidebarUtils.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/contact_list/ContactListItemRenderer.h"
#include "../controls/GeneralDialog.h"
#include "../my_info.h"
#include "../utils/InterConnector.h"

#include "CommonUI.h"

namespace Ui
{
    SelectionContactsForConference::SelectionContactsForConference(
        Logic::ChatMembersModel* _conferenceMembersModel,
		Logic::ChatMembersModel* _otherContactsModel,
        const QString& _labelText,
        QWidget* _parent,
		ConferenceSearchMember& usersSearchModel,
        bool _handleKeyPressEvents) : SelectContactsWidget(_otherContactsModel,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            _labelText,
            "",
            "",
            _parent,
            _handleKeyPressEvents,
			&usersSearchModel),
		conferenceMembersModel_(_conferenceMembersModel),
		videoConferenceMaxUsers_(-1)
    {
		setStyleSheet(Utils::LoadStyle(":/voip/video_conference_dialog.qss"));

		mainWidget_ = new QWidget(this);
		auto mainWidgetLayout = Utils::emptyVLayout(mainWidget_);
		mainWidget_->setLayout(mainWidgetLayout);
		mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
		mainWidget_->setContentsMargins(0, 0, 0, 0);

		auto memberWidget = new PureClickedWidget(this);
		memberWidget->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
		auto memberWidgetLayout = Utils::emptyHLayout(memberWidget);		
		memberWidget->setLayout(memberWidgetLayout);
		memberWidget->setCursor(Qt::PointingHandCursor);
		memberWidget->setStyleSheet(Utils::LoadStyle(":/voip/video_conference_dialog.qss"));

		memberLabel_ = new QLabel(QT_TRANSLATE_NOOP("voip_pages", "MEMBERS"), memberWidget);
		memberLabel_->setContentsMargins(0, 0, 0, 0);
		Utils::ApplyStyle(memberLabel_, "color: #579e1c; font-size: 12dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;");
		memberWidgetLayout->addWidget(memberLabel_);
		memberWidgetLayout->addSpacing(Utils::scale_value(6));

		memberArrowLabel_ = new QPushButton(memberWidget);
        memberArrowLabel_->setStyleSheet("border: 0;");
		memberArrowLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		memberArrowLabel_->setStyle(QApplication::style());
		memberArrowLabel_->setContentsMargins(0, 0, 0, 0);

		if (platform::is_apple())
		{
			memberWidgetLayout->addWidget(memberArrowLabel_, 0, Qt::AlignTop);
		}
		else
		{
			memberWidgetLayout->addWidget(memberArrowLabel_);
		}

		memberWidgetLayout->addStretch();

        auto clDelegate  = new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::VIDEO_CONFERENCE, conferenceMembersModel_);
        conferenceContacts_ = CreateFocusableViewAndSetTrScrollBar(this);
        conferenceContacts_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        conferenceContacts_->setFrameShape(QFrame::NoFrame);
        conferenceContacts_->setSpacing(0);
        conferenceContacts_->setModelColumn(0);
        conferenceContacts_->setUniformItemSizes(false);
        conferenceContacts_->setBatchSize(50);
        conferenceContacts_->setStyleSheet("background: transparent;");
        conferenceContacts_->setCursor(Qt::PointingHandCursor);
        conferenceContacts_->setMouseTracking(true);
        conferenceContacts_->setAcceptDrops(true);
        conferenceContacts_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        conferenceContacts_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		conferenceContacts_->setAutoScroll(false);
        conferenceContacts_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        conferenceContacts_->setAttribute(Qt::WA_MacShowFocusRect, false);
        conferenceContacts_->setModel(conferenceMembersModel_);
        conferenceContacts_->setItemDelegate(clDelegate);
		conferenceContacts_->setContentsMargins(0, 0, 0, 0);

        auto othersLabel = new QLabel(QT_TRANSLATE_NOOP("voip_pages", "OTHERS"), mainWidget_);
		othersLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
		othersLabel->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
		//Qt ignory our Margins if zoom is 200%. This line fix this problem.
		othersLabel->setMinimumHeight(2 * Utils::scale_value(12) + Utils::scale_value(4));
        Utils::ApplyStyle(othersLabel, "color: #579e1c; font-size: 12dip; font-family: %FONT_FAMILY_MEDIUM%; font-weight: %FONT_WEIGHT_MEDIUM%;");

        auto index = globalLayout_->indexOf(searchWidget_);

		mainWidgetLayout->addWidget(memberWidget);
		mainWidgetLayout->addWidget(conferenceContacts_);
		mainWidgetLayout->addWidget(othersLabel);

		contactList_->setIndexWidget(0, mainWidget_);
		usersSearchModel.setWidget(mainWidget_);

		globalLayout_->insertWidget(index + 1, contactList_, 1);

		mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("voip_pages", "Cancel"), QT_TRANSLATE_NOOP("voip_pages", "Add"), Utils::scale_value(16), Utils::scale_value(12));

		connect(conferenceContacts_, &FocusableListView::clicked, this, &SelectionContactsForConference::itemClicked, Qt::QueuedConnection);

		// Update other contacts list.
		connect(_otherContactsModel, &Logic::ChatMembersModel::dataChanged, this, [=]() {
			searchModel_->searchPatternChanged(searchModel_->getCurrentPattern());
		});

		connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged,
			this, &SelectionContactsForConference::onVoipCallNameChanged, Qt::DirectConnection);

		connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed,
			this, &SelectionContactsForConference::onVoipCallDestroyed, Qt::DirectConnection);

		connect(memberWidget, &PureClickedWidget::clicked, this, &SelectionContactsForConference::clickConferenceMembers);
        
		connect(memberArrowLabel_, &QPushButton::clicked, this, &SelectionContactsForConference::clickConferenceMembers);

		clickConferenceMembers();
		updateMemberList();
    }

    void SelectionContactsForConference::itemClicked(const QModelIndex& _current)
    {
        if (!(QApplication::mouseButtons() & Qt::RightButton /*|| tapAndHoldModifier()*/))
        {
            QString aimid;
            if (Logic::is_video_conference_regim(regim_))
            {
                auto cont = _current.data().value<Data::ChatMemberInfo*>();
                if (cont)
                {
                    aimid = cont->AimId_;
                }
            }

            if (!aimid.isEmpty())
            {
                deleteMemberDialog(conferenceMembersModel_, aimid, regim_, this);
            }
        }
        
        return;
    }

	void SelectionContactsForConference::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
	{
		updateMemberList();
	}

	void SelectionContactsForConference::updateMemberList()
	{
		std::shared_ptr<Data::ChatInfo> chatInfo = std::make_shared<Data::ChatInfo>();

		for (const voip_manager::Contact& contact : Ui::GetDispatcher()->getVoipController().currentCallContacts())
		{
			Data::ChatMemberInfo memberInfo;
			memberInfo.AimId_ = QString(contact.contact.c_str());
			memberInfo.NickName_ = Logic::getContactListModel()->getDisplayName(memberInfo.AimId_);
			chatInfo->Members_.push_back(memberInfo);
		}

		chatInfo->MembersCount_ = Ui::GetDispatcher()->getVoipController().currentCallContacts().size();

		conferenceMembersModel_->updateInfo(chatInfo, false);

		updateConferenceListSize();

		updateMaxSelectedCount();
		UpdateMembers();
		conferenceContacts_->update();
	}

	void SelectionContactsForConference::updateConferenceListSize()
	{
		if (conferenceContacts_->flow() == QListView::TopToBottom)
		{
			conferenceContacts_->setFixedHeight(::ContactList::GetContactListParams().itemHeight() * conferenceMembersModel_->getMembers().size());
		}
		else
		{
			// All in one row.
			conferenceContacts_->setFixedHeight(::ContactList::GetContactListParams().itemHeight());
		}

		// Force update first element in list view.
		conferenceMembersModel_->dataChanged(conferenceMembersModel_->index(0, 0), conferenceMembersModel_->index(0, 0));
	}

	void SelectionContactsForConference::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
	{
		if (!_contactEx.incoming && _contactEx.connection_count <= 1)
		{
			reject();
		}
	}

	// Set maximum restriction for selected item count. Used for video conference.
	void SelectionContactsForConference::setMaximumSelectedCount(int number)
	{	
		videoConferenceMaxUsers_ = number;
		updateMaxSelectedCount();
	}

	void SelectionContactsForConference::updateMaxSelectedCount()
	{
		// -1 is your own contact.
		if (videoConferenceMaxUsers_ >= 0)
		{
			memberLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", "MEMBERS") + " (" + QString::number(conferenceMembersModel_->getMembersCount()) + "/" + QString::number(videoConferenceMaxUsers_ - 1) + ")");
			SelectContactsWidget::setMaximumSelectedCount(qMax(videoConferenceMaxUsers_ - conferenceMembersModel_->getMembersCount() - 1, 0));
		}
	}

	void SelectionContactsForConference::clickConferenceMembers()
	{		
		if (conferenceContacts_->flow() == QListView::TopToBottom)
		{
			conferenceContacts_->setFlow(QListView::LeftToRight);
			conferenceContacts_->setItemDelegate(new ContactListItemHorizontalDelegate(this));
			Utils::ApplyPropertyParameter(memberArrowLabel_, "Full_view", false);
		}
		else
		{
			conferenceContacts_->setFlow(QListView::TopToBottom);
			auto deligate = new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::VIDEO_CONFERENCE, conferenceMembersModel_);
			deligate->setFixedWidth(conferenceContacts_->width());
			conferenceContacts_->setItemDelegate(deligate);
			Utils::ApplyPropertyParameter(memberArrowLabel_, "Full_view", true);
		}

		updateConferenceListSize();
	}

    void SelectionContactsForConference::updateSize()
    {
        if (parentWidget())
        {
            mainDialog_->updateSize();
            UpdateView();
        }
    }


	ContactsForVideoConference::ContactsForVideoConference(QObject* parent, const Logic::ChatMembersModel& videoConferenceModel) :
		Logic::ChatMembersModel(parent), videoConferenceModel_(videoConferenceModel)
	{
		connect(&videoConferenceModel_, &Logic::ChatMembersModel::dataChanged, this, &ContactsForVideoConference::updateMemberList);
		updateMemberList();
	}

	void ContactsForVideoConference::updateMemberList()
	{
		if (allMembers_.Members_.size() == 0)
		{
			// Fetch all members first.
			auto allMembersModel = Logic::getCurrentSearchModel(Logic::MembersWidgetRegim::SELECT_MEMBERS);
			connection_ = connect(allMembersModel, &QAbstractItemModel::dataChanged, this, &ContactsForVideoConference::allMembersDataChanged);
			allMembersModel->searchPatternChanged("");
		}
		else
		{
			std::shared_ptr<Data::ChatInfo> chatInfo = std::make_shared<Data::ChatInfo>();
			int nCount = 0;
			for (int i = 0; i < allMembers_.Members_.size(); i++)
			{
				Data::ChatMemberInfo memberInfo = allMembers_.Members_[i];
				
				if (!videoConferenceModel_.isContactInChat(memberInfo.AimId_))
				{
					chatInfo->Members_.push_back(memberInfo);
					nCount++;
				}
			}

			chatInfo->MembersCount_ = nCount;

			updateInfo(chatInfo, true);		
		}
	}

	void ContactsForVideoConference::allMembersDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
	{
		auto allMembersModel = Logic::getCurrentSearchModel(Logic::MembersWidgetRegim::SELECT_MEMBERS);

		allMembers_.Members_.clear();

		int nCount = 0;
		for (int i = 0; i < allMembersModel->rowCount(); i++)
		{
			QModelIndex modelIndex = allMembersModel->index(i);
			Data::DlgState dataContact = qvariant_cast<Data::DlgState>(allMembersModel->data(modelIndex, Qt::DisplayRole));

			Data::ChatMemberInfo memberInfo;
			memberInfo.AimId_ = dataContact.AimId_;
			memberInfo.NickName_ = dataContact.Friendly_;
			allMembers_.Members_.push_back(memberInfo);
			nCount++;
		}

		allMembers_.MembersCount_ = nCount;

		if (allMembers_.MembersCount_ > 0)
		{
			updateMemberList();
			disconnect(connection_);
		}		
	}

	ContactListItemHorizontalDelegate::ContactListItemHorizontalDelegate(QObject* parent)
		: AbstractItemDelegateWithRegim(parent)
	{
	}

	ContactListItemHorizontalDelegate::~ContactListItemHorizontalDelegate()
	{
	}

	void ContactListItemHorizontalDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		const auto searchMemberModel = qobject_cast<const Logic::SearchMembersModel*>(index.model());
		const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(index.model());

		bool isGroup = false;
		QString displayName, status, state;
		QString aimId;
		bool hasLastSeen = false, isChecked = false, isChatMember = false, isOfficial = false;
		QDateTime lastSeen;

		Data::Contact* contact_in_cl = NULL;
		QString role;

		if (membersModel || searchMemberModel)
		{
			auto cont = index.data(Qt::DisplayRole).value<Data::ChatMemberInfo*>();
			displayName = cont->getFriendly();

			aimId = cont->AimId_;
			if (aimId != Ui::MyInfo()->aimId())
			{
				auto contact_item = Logic::getContactListModel()->getContactItem(aimId);
				contact_in_cl = contact_item == NULL ? NULL : contact_item->Get();
			}
		}
		else
		{
			contact_in_cl = index.data(Qt::DisplayRole).value<Data::Contact*>();
		}

		if (contact_in_cl)
		{
			isGroup = (contact_in_cl->GetType() == Data::GROUP);
			displayName = contact_in_cl->GetDisplayName();
			aimId = contact_in_cl->AimId_;
			status = contact_in_cl->StatusMsg_;
			state = contact_in_cl->State_;
			hasLastSeen = contact_in_cl->HasLastSeen_;
			lastSeen = contact_in_cl->LastSeen_;
			isChecked = contact_in_cl->IsChecked_;
			isOfficial = contact_in_cl->IsOfficial_;
		}

		//const bool isHovered = ((option.state & QStyle::State_MouseOver) && !isGroup) && !StateBlocked_ && !isSelected && hasMouseOver;

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		painter->translate(option.rect.topLeft());

		{
			const auto isMultichat = Logic::getContactListModel()->isChat(aimId);
			const auto isFilled = !isMultichat;
			auto isDefault = false;

			const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimId, displayName, Utils::scale_bitmap(::ContactList::GetContactListParams().avatarSize())
				, isMultichat ? QString() : state, isFilled, isDefault, false /* _regenerate */, ::ContactList::GetContactListParams().isCL());
			const ::ContactList::VisualDataBase visData(aimId, *avatar, state, status, false, false, displayName, hasLastSeen, lastSeen
				, isChecked, isChatMember, isOfficial, false /* draw last read */, QPixmap() /* last seen avatar*/, role, 0 /* unread count */, "" /* search_term */);

			::ContactList::ViewParams viewParams(viewParams_.regim_, viewParams_.fixedWidth_, viewParams_.leftMargin_, viewParams_.rightMargin_);
			//::ContactList::RenderContactItem(*painter, visData, viewParams);

			auto contactList = ::ContactList::GetContactListParams();
			::ContactList::RenderAvatar(*painter, Utils::scale_value(16), visData.Avatar_, contactList);
		}

		painter->restore();
	}

	QSize ContactListItemHorizontalDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
	{
		return QSize(::ContactList::GetContactListParams().avatarSize() + Utils::scale_value(16), ::ContactList::GetContactListParams().avatarSize());
	}

	void ContactListItemHorizontalDelegate::setFixedWidth(int width)
	{
		viewParams_.fixedWidth_ = width;
	}

	void ContactListItemHorizontalDelegate::setLeftMargin(int margin)
	{
		viewParams_.leftMargin_ = margin;
	}

	void ContactListItemHorizontalDelegate::setRightMargin(int margin)
	{
		viewParams_.rightMargin_ = margin;
	}

	void ContactListItemHorizontalDelegate::setRegim(int _regim)
	{
		viewParams_.regim_ = _regim;
	}


	ConferenceSearchMember::ConferenceSearchMember() : Logic::SearchMembersModel(0), firstElement_(nullptr), bSearchMode_(false) {}

	int ConferenceSearchMember::rowCount(const QModelIndex& _parent) const
	{
		return Logic::SearchMembersModel::rowCount(_parent) + 1;
	}

	QVariant ConferenceSearchMember::data(const QModelIndex& _index, int _role) const
	{
		if (_index.row() == 0)
		{
			if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
				return QVariant();

			if (firstElement_)
			{
				firstElement_->setVisible(!bSearchMode_);
			}
			return QVariant::fromValue(firstElement_);
		}
		QModelIndex id = index(_index.row() - 1);
		return Logic::SearchMembersModel::data(id, _role);
	}
	
	void ConferenceSearchMember::setWidget(QWidget* widget)
	{
		firstElement_ = widget;
	}

	void ConferenceSearchMember::searchPatternChanged(QString search)
	{
		bSearchMode_ = !search.isEmpty();

		::Logic::SearchMembersModel::searchPatternChanged(search);
	}
}

