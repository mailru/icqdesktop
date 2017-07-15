#include "stdafx.h"
#include "mailboxes.h"
#include "../../core.h"


namespace core
{
namespace wim
{
    mailbox::mailbox()
        : unreadCount_(0)
    {
    }

    mailbox::mailbox(const std::string& _email)
        : email_(_email)
    {
    }

    void mailbox::setUnreads(unsigned _unreads)
    {
        unreadCount_ = _unreads;
    }

    unsigned mailbox::get_unreads() const
    {
        return unreadCount_;
    }

    std::string mailbox::get_mailbox() const
    {
        return email_;
    }

    void mailbox::unserialize(const rapidjson::Value& _node)
    {
        auto iter_email = _node.FindMember("email");

        if (iter_email == _node.MemberEnd() || !iter_email->value.IsString())
            return;

        email_ = iter_email->value.GetString();

        auto iter_unread = _node.FindMember("unreadCount");
        if (iter_unread == _node.MemberEnd() || !iter_unread->value.IsUint())
            return;

        unreadCount_ = iter_unread->value.GetUint();
    }

    void mailbox::serialize(core::coll_helper _collection)
    {
        _collection.set_value_as_string("email", email_);
        _collection.set_value_as_uint("unreads", unreadCount_);
    }

    void mailbox::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
    {
        _node.AddMember("email", get_mailbox(), _a);
        _node.AddMember("unreadCount", get_unreads(), _a);
    }

    mailbox_storage::mailbox_storage()
        : is_changed_(false)
    {

    }

    void mailbox_storage::process(const mailbox_changes& _changes, std::function<void(core::coll_helper, mailbox_change::type)> _notify_callback)
    {
        is_changed_ = true;
        bool init = false;

        for (auto change : _changes)
        {
            mailbox m(change->email_);
            coll_helper coll_mailbox(g_core->create_collection(), true);

            switch (change->type_)
            {
            case mailbox_change::status:
                init = mailboxes_.find(change->email_) == mailboxes_.end();
                m.setUnreads(((mailbox_status_change*)change.get())->unreads_);
                mailboxes_[change->email_] = m;

                serialize(coll_mailbox);
                coll_mailbox.set_value_as_bool("init", init);
                _notify_callback(coll_mailbox, change->type_);
                break;

            case mailbox_change::new_mail:
                m.setUnreads(((mailbox_new_mail_change*)change.get())->unreads_);
                mailboxes_[change->email_] = m;

                coll_mailbox.set_value_as_string("email", change->email_);
                coll_mailbox.set_value_as_string("from", ((mailbox_new_mail_change*)change.get())->from_);
                coll_mailbox.set_value_as_string("subj", ((mailbox_new_mail_change*)change.get())->subject_);
                coll_mailbox.set_value_as_string("uidl", ((mailbox_new_mail_change*)change.get())->uidl_);

                _notify_callback(coll_mailbox, change->type_);

                serialize(coll_mailbox);
                _notify_callback(coll_mailbox, mailbox_change::status);
                break;

            case mailbox_change::mail_read:
            default:
                break;
            }
        }
    }

    void mailbox_storage::unserialize(const rapidjson::Value& _node_event_data, mailbox_changes& _changes)
    {        
        auto event_status = _node_event_data.FindMember("mailbox.status");
        if (event_status != _node_event_data.MemberEnd() && event_status->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_status->value.GetString());

            auto change = std::make_shared<mailbox_status_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = iter_email->value.GetString();

            auto iter_unreads = doc.FindMember("unreadCount");
            if (iter_unreads != doc.MemberEnd() && iter_unreads->value.IsUint())
                change->unreads_ = iter_unreads->value.GetUint();

            _changes.push_back(change);
        }

        auto event_read = _node_event_data.FindMember("mailbox.messageReaded");
        if (event_read != _node_event_data.MemberEnd() && event_read->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_read->value.GetString());

            auto change = std::make_shared<mailbox_read_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = iter_email->value.GetString();

            auto iter_id = doc.FindMember("uidl");
            if (iter_id != doc.MemberEnd() && iter_id->value.IsString())
                change->uidl_ = iter_id->value.GetString();

            _changes.push_back(change);
        }

        auto event_new = _node_event_data.FindMember("mailbox.newMessage");
        if (event_new != _node_event_data.MemberEnd() && event_new->value.IsString())
        {
            rapidjson::Document doc;
            doc.Parse(event_new->value.GetString());

            auto change = std::make_shared<mailbox_new_mail_change>();

            auto iter_email = doc.FindMember("email");
            if (iter_email != doc.MemberEnd() && iter_email->value.IsString())
                change->email_ = iter_email->value.GetString();

            auto iter_unreads = doc.FindMember("unreadCount");
            if (iter_unreads != doc.MemberEnd() && iter_unreads->value.IsUint())
                change->unreads_ = iter_unreads->value.GetUint();

            auto iter_id = doc.FindMember("uidl");
            if (iter_id != doc.MemberEnd() && iter_id->value.IsString())
                change->uidl_ = iter_id->value.GetString();

            auto iter_from = doc.FindMember("from");
            if (iter_from != doc.MemberEnd() && iter_from->value.IsString())
                change->from_ = iter_from->value.GetString();

            auto iter_subject = doc.FindMember("subject");
            if (iter_subject != doc.MemberEnd() && iter_subject->value.IsString())
                change->subject_ = iter_subject->value.GetString();

            _changes.push_back(change);
        }
    }

    void mailbox_storage::serialize(core::coll_helper _collection)
    {
        ifptr<iarray> mailbox_array(_collection->create_array());

        if (!mailboxes_.empty())
        {
            mailbox_array->reserve(mailboxes_.size());
            for (auto mailbox : mailboxes_)
            {
                coll_helper coll_mailbox(_collection->create_collection(), true);
                mailbox.second.serialize(coll_mailbox);
                ifptr<ivalue> val(_collection->create_value());
                val->set_as_collection(coll_mailbox.get());
                mailbox_array->push_back(val.get());
            }
        }

        _collection.set_value_as_array("mailboxes", mailbox_array.get());
    }

    bool mailbox_storage::is_changed() const
    {
        return is_changed_;
    }

    int32_t mailbox_storage::save(const std::wstring& _filename)
    {
        if (!is_changed())
            return 0;

        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& allocator = doc.GetAllocator();
        rapidjson::Value node_mailboxes(rapidjson::Type::kArrayType);

        for (auto mailbox : mailboxes_)
        {
            rapidjson::Value node_mailbox(rapidjson::Type::kObjectType);
            mailbox.second.serialize(node_mailbox, allocator);
            node_mailboxes.PushBack(node_mailbox, allocator);
        }

        doc.AddMember("mailboxes", node_mailboxes, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::string json_string = buffer.GetString();

        if (!json_string.length())
        {
            return - 1;
        }

        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(_filename))
            return -1;

        is_changed_ = false;

        return 0;
    }

    int32_t mailbox_storage::load(const std::wstring& _filename)
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(_filename))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        auto iter_mailboxes = doc.FindMember("mailboxes");
        if (iter_mailboxes == doc.MemberEnd() || !iter_mailboxes->value.IsArray())
            return -1;

        for (auto iter = iter_mailboxes->value.Begin(); iter != iter_mailboxes->value.End(); iter++)
        {
            mailbox m;
            m.unserialize(*iter);
            mailboxes_[m.get_mailbox()] = m;
        }

        return 0;
    }
}
}