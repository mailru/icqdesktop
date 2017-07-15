#pragma once

#include "../../types/contact.h"

namespace Logic
{
	class contact_profile;
	typedef std::shared_ptr<contact_profile> profile_ptr;

	class ContactItem
	{
	public:

		explicit ContactItem(Data::Contact* _contact);

		bool operator== (const ContactItem& _other) const;

		Data::Contact* Get() const;

		bool is_visible() const;
		void set_visible(bool);
		bool is_group() const;
		bool is_online() const;
		bool is_phone() const;
		bool recently() const;
        bool recently(const QDateTime& _current) const;
		bool is_chat() const;
		bool is_muted() const;
        bool is_live_chat() const;
        bool is_official() const;
		bool is_not_auth() const;
		void reset_not_auth();

		bool is_checked() const;
		void set_checked(bool _isChecked);

        void set_chat_role(const QString& role);
        QString get_chat_role() const;
		
		void set_input_text(const QString& _inputText);
		QString get_input_text() const;

		void set_contact_profile(profile_ptr _profile);
		profile_ptr getContactProfile() const;
		QString get_aimid() const;

	private:

		std::shared_ptr<Data::Contact>				contact_;
		std::shared_ptr<Logic::contact_profile>		profile_;
		
		
		bool			visible_;
		QString			input_text_;
        QString         chat_role_;
	};

	static_assert(std::is_move_assignable<ContactItem>::value, "ContactItem must be move assignable");
	static_assert(std::is_move_constructible<ContactItem>::value, "ContactItem must be move constructible");
}