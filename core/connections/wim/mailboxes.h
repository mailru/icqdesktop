#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../archive/storage.h"

namespace core
{
    namespace wim
    {
        struct mailbox_change
        {
            enum type
            {
                status = 0,
                new_mail = 1,
                mail_read = 2,
            } type_;
            std::string email_;

            mailbox_change(type _type)
                : type_(_type)
            {
            }
        };

        struct mailbox_status_change : public mailbox_change
        {
            mailbox_status_change()
                : mailbox_change(mailbox_change::status)
                , unreads_(0)
            {
            }

            unsigned unreads_;
        };

        struct mailbox_read_change : public mailbox_change
        {
            mailbox_read_change()
                : mailbox_change(mailbox_change::mail_read)
            {
            }

            std::string uidl_;
        };

        struct mailbox_new_mail_change : public mailbox_change
        {
            mailbox_new_mail_change()
                : mailbox_change(mailbox_change::new_mail)
                , unreads_(0)
            {
            }

            unsigned unreads_;
            std::string uidl_;
            std::string from_;
            std::string subject_;
        };

        typedef std::vector<std::shared_ptr<mailbox_change>> mailbox_changes;

        class mailbox
        {
            std::string email_;
            unsigned unreadCount_;

        public:
            mailbox();
            mailbox(const std::string& _email);

            void setUnreads(unsigned _unreads);
            unsigned get_unreads() const;
            std::string get_mailbox() const;

            void unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _collection);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
        };

        class mailbox_storage
        {
            bool is_changed_;
            std::map<std::string, mailbox> mailboxes_;

        public:
            mailbox_storage();

            void process(const mailbox_changes& _changes, std::function<void(core::coll_helper, mailbox_change::type)> _notify_callback);

            static void unserialize(const rapidjson::Value& _node_event_data, mailbox_changes& _changes);

            void serialize(core::coll_helper _collection);

            bool is_changed() const;

            int32_t save(const std::wstring& _filename);
            int32_t load(const std::wstring& _filename);
        };
    }
}