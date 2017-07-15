#pragma once

#include "stdafx.h"

class QPixmap;

namespace core
{
	class coll_helper;
}

namespace Data
{
	enum ContactType
	{
		MIN = 0,

		BASE = MIN,
		GROUP,
        EMPTY_IGNORE_LIST,
        SEARCH_IN_ALL_CHATS,

		MAX 
	};

	class Buddy
	{
	public:
		Buddy()
			: GroupId_(-1)
			, Is_chat_(false)
			, Muted_(false)
			, HasLastSeen_(false)
			, Unreads_(0)
			, NotAuth_(false)
			, IsChecked_(false)
            , IsLiveChat_(false)
            , IsOfficial_(false)
		{
		}

		bool operator==(const Buddy& other) const
		{
			return AimId_ == other.AimId_;
		}

		QString		AimId_;
		QString		Friendly_;
		QString		AbContactName_;
		QString		State_;
		QString		UserType_;
		QString		StatusMsg_;
		QString		OtherNumber_;
		QDateTime	LastSeen_;
		int			GroupId_;
		bool		Is_chat_;
		bool		HasLastSeen_;
		int			Unreads_;
		bool		NotAuth_;
		bool		Muted_;
		bool		IsChecked_;
        bool        IsLiveChat_;
        bool        IsOfficial_;
        QString		iconId_;
        QString		bigIconId_;
        QString		largeIconId_;
	};

	class Contact : public Buddy
	{
	public:
		Contact()
		{
		}

		virtual QString GetDisplayName() const
		{
			return AbContactName_.isEmpty() ? (Friendly_.isEmpty() ? AimId_ : Friendly_) : AbContactName_;
		}

		void ApplyBuddy(Buddy* buddy)
		{
			int groupId = GroupId_;
			bool isChat = Is_chat_;
			QString ab = AbContactName_;
			*((Buddy*)this) = *buddy;
			if (buddy->GroupId_ == -1)
				GroupId_ = groupId;
			if (AbContactName_.isEmpty() && !ab.isEmpty())
				AbContactName_ = ab;
			Is_chat_ = isChat;
		}

		virtual ContactType GetType() const
		{
			return BASE;
		}

		virtual QString GetState() const
		{
			return State_;
		}
        
        virtual QDateTime GetLastSeen() const
        {
            return LastSeen_;
        }
	};

	class GroupBuddy
	{
	public:
		GroupBuddy()
			: Added_(false)
			, Removed_(false)
		{
		}

		bool operator==(const GroupBuddy& other) const
		{
			return Id_ == other.Id_;
		}

		int Id_;
		QString Name_;

		//used by diff from server
		bool		Added_;
		bool		Removed_;
	};

	class Group : public GroupBuddy, public Contact
	{
	public:
		virtual ContactType GetType() const override
		{
			return GROUP;
		}

		void ApplyBuddy(GroupBuddy* buddy)
		{
			*((GroupBuddy*)this) = *buddy;
			GroupId_ = buddy->Id_;
			AimId_ = QString::number(GroupId_);
		}

		virtual QString GetDisplayName() const override
		{
			return Name_;
		}
	};

	typedef QMap<Contact*, GroupBuddy*> ContactList;

	void UnserializeContactList(core::coll_helper* helper, ContactList& cl, QString& type);

	QPixmap* UnserializeAvatar(core::coll_helper* helper);

	Buddy* UnserializePresence(core::coll_helper* helper);

	QString UnserializeActiveDialogHide(core::coll_helper* helper);
    
    QStringList UnserializeFavorites(core::coll_helper* helper);
}

Q_DECLARE_METATYPE(Data::Buddy*);
Q_DECLARE_METATYPE(Data::Contact*);