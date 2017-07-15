#pragma once

#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/contact_list/SearchMembersModel.h"

namespace Logic
{
    class SearchMembersModel;
}

namespace Ui
{
    class ListViewWithTrScrollBar;
	class ConferenceSearchMember;

    class SelectionContactsForConference : public SelectContactsWidget
    {
        Q_OBJECT

    public:

        SelectionContactsForConference(
            Logic::ChatMembersModel* _chatMembersModel,
			Logic::ChatMembersModel* _otherContactsModel,			
            const QString& _labelText,
            QWidget* _parent,
			ConferenceSearchMember& usersSearchModel,
            bool _handleKeyPressEvents = true);

		void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);

		// Set maximum restriction for selected item count. Used for video conference.
		virtual void setMaximumSelectedCount(int number) override;

    public Q_SLOTS:
        void updateSize();

    private Q_SLOTS:

		void updateMemberList();
		void itemClicked(const QModelIndex& _current);
		void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);
		void updateMaxSelectedCount();
		void updateConferenceListSize();
		void clickConferenceMembers();

    private:
        QWidget* membersList_;
        QWidget* mainWidget_;
		QLabel*  memberLabel_;
		QPushButton*  memberArrowLabel_;
        ListViewWithTrScrollBar* conferenceContacts_;  
		Logic::ChatMembersModel* conferenceMembersModel_;
		int videoConferenceMaxUsers_;
    };

	// Copy of contact list without video conference memebers.
	class ContactsForVideoConference : public Logic::ChatMembersModel
	{
		Q_OBJECT

	public:

		ContactsForVideoConference(QObject* parent, const Logic::ChatMembersModel& videoConferenceModel);

	protected Q_SLOTS:

		void updateMemberList();
		void allMembersDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);		

	protected:

		const Logic::ChatMembersModel& videoConferenceModel_;
		Data::ChatInfo allMembers_;
		QMetaObject::Connection connection_;
	};


	// Draw only avatars of users for contacts list.
	// Used in horizontal mode in add to video conference dialog.
	class ContactListItemHorizontalDelegate : public Logic::AbstractItemDelegateWithRegim
	{
	public:
		ContactListItemHorizontalDelegate(QObject* parent);

		virtual ~ContactListItemHorizontalDelegate();

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

		virtual void setFixedWidth(int width) override;

		void setLeftMargin(int margin);

		void setRightMargin(int margin);

		virtual void setRegim(int _regim) override;

	private:
		::ContactList::ViewParams viewParams_;
	};



	class ConferenceSearchMember : public ::Logic::SearchMembersModel
	{
		Q_OBJECT

	public:
		ConferenceSearchMember();

		int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
		QVariant data(const QModelIndex& _index, int _role) const override;

		void setWidget(QWidget* widget);

	public Q_SLOTS:
		void searchPatternChanged(QString) override;

	private:

		QWidget* firstElement_;
		bool bSearchMode_;
	};

}
