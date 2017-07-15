#include "stdafx.h"
#include "contact.h"

#include "../../corelib/collection_helper.h"


namespace Data
{
	void UnserializeContactList(core::coll_helper* helper, ContactList& cl, QString& type)
	{
		type = QString();
		if (helper->is_value_exist("type"))
			type = helper->get_value_as_string("type");

		core::iarray* groups = helper->get_value_as_array("groups");
		for (int igroups = 0; igroups < groups->size(); ++igroups)
		{
			core::coll_helper group_coll(groups->get_at(igroups)->get_as_collection(), false);
			GroupBuddy* group = new GroupBuddy();
			group->Id_ = group_coll.get_value_as_int("group_id");
			group->Name_ = group_coll.get_value_as_string("group_name");
			group->Added_ = group_coll.get_value_as_bool("added");
			group->Removed_ = group_coll.get_value_as_bool("removed");
			core::iarray* contacts = group_coll.get_value_as_array("contacts");
			for (int icontacts = 0; icontacts < contacts->size(); ++icontacts)
			{
				core::coll_helper value(contacts->get_at(icontacts)->get_as_collection(), false);
                qlonglong lastSeen = value.get_value_as_int("lastseen");
				Contact* contact = new Contact();
				contact->AimId_ = value.get_value_as_string("aimId");
				contact->Friendly_ = value.get_value_as_string("friendly");
				contact->AbContactName_ = value.get_value_as_string("abContactName");
                QString state = value.get_value_as_string("state");
                contact->State_ = (lastSeen <= 0 || state == "mobile") ? state : "offline";
                if (contact->State_ == "mobile" && lastSeen == 0)
                    contact->State_ = "online";
				contact->UserType_ = value.get_value_as_string("userType");
				contact->StatusMsg_ = value.get_value_as_string("statusMsg");
				contact->OtherNumber_ = value.get_value_as_string("otherNumber");
				contact->HasLastSeen_ = lastSeen != -1;
				contact->LastSeen_ = lastSeen > 0 ? QDateTime::fromTime_t((uint)lastSeen) : QDateTime();
				contact->Is_chat_ = value.get_value_as_bool("is_chat");
				contact->GroupId_ = group->Id_;
				contact->Muted_ = value.get_value_as_bool("mute");
                contact->IsLiveChat_ = value.get_value_as_bool("livechat");
                contact->IsOfficial_ = value.get_value_as_bool("official");
                contact->iconId_ = value.get_value_as_string("iconId");
                contact->bigIconId_ = value.get_value_as_string("bigIconId");
                contact->largeIconId_ = value.get_value_as_string("largeIconId");
                
				cl.insert(contact, group);
			}
		}
	}

	QPixmap* UnserializeAvatar(core::coll_helper* helper)
	{
		if (helper->get_value_as_bool("result"))
		{
			QPixmap* result = new QPixmap();
			core::istream* stream = helper->get_value_as_stream("avatar");
			if (stream)
			{
				uint32_t size = stream->size();
				result->loadFromData(stream->read(size), size);
				stream->reset();
			}
			return result;
		}

		return 0;
	}

	Buddy* UnserializePresence(core::coll_helper* helper)
	{
		Buddy* result = new Buddy();
		
		result->AimId_ = helper->get_value_as_string("aimId");
		result->Friendly_ = helper->get_value_as_string("friendly");
		result->AbContactName_ = helper->get_value_as_string("abContactName");
		result->State_ = helper->get_value_as_string("state");
		result->UserType_ = helper->get_value_as_string("userType");
		result->StatusMsg_ = helper->get_value_as_string("statusMsg");
		result->OtherNumber_ = helper->get_value_as_string("otherNumber");
		qlonglong lastSeen = helper->get_value_as_int("lastseen");
		result->HasLastSeen_ = lastSeen != -1;
		result->LastSeen_ = lastSeen > 0 ? QDateTime::fromTime_t((uint)lastSeen) : QDateTime();
		result->Is_chat_ = helper->get_value_as_bool("is_chat");
		result->Muted_ = helper->get_value_as_bool("mute");
        result->IsLiveChat_ = helper->get_value_as_bool("livechat");
        result->IsOfficial_ = helper->get_value_as_bool("official");
        result->iconId_ = helper->get_value_as_string("iconId");
        result->bigIconId_ = helper->get_value_as_string("bigIconId");
        result->largeIconId_ = helper->get_value_as_string("largeIconId");
        
		return result;
	}
	
	QString UnserializeActiveDialogHide(core::coll_helper* helper)
	{
		return helper->get_value_as_string("contact");
	}
    
    QStringList UnserializeFavorites(core::coll_helper* helper)
    {
        QStringList result;
        core::iarray* contacts = helper->get_value_as_array("favorites");
        for (int i = 0; i < contacts->size(); ++i)
        {
            core::coll_helper value(contacts->get_at(i)->get_as_collection(), false);
            result.push_back(value.get_value_as_string("aimId"));
        }
        
        return result;
    }
}