#include "stdafx.h"
#include "MessagesModel.h"

#include "ChatEventItem.h"
#include "DeletedMessageItem.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "ServiceMessageItem.h"
#include "ContentWidgets/FileSharingWidget.h"

#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../history_control/ChatEventInfo.h"
#include "../history_control/FileSharingInfo.h"
#include "../history_control/VoipEventInfo.h"
#include "../history_control/VoipEventItem.h"
#include "../history_control/complex_message/ComplexMessageItem.h"
#include "../history_control/complex_message/ComplexMessageItemBuilder.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/log/log.h"
#include "../../cache/emoji/Emoji.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace
{
    QString NormalizeAimId(const QString& _aimId)
    {
        int pos = _aimId.indexOf("@uin.icq");
        return pos == -1 ? _aimId : _aimId.left(pos);
    }

    QString GetChatFriendly(const QString& _aimId, const QString& _chatFriendly)
    {
        QString normalized = NormalizeAimId(_aimId);
        QString clFriendly = Logic::getContactListModel()->getDisplayName(normalized);
        if (clFriendly == normalized)
        {
            return _chatFriendly.isEmpty() ? _aimId : _chatFriendly;
        }

        return clFriendly;
    }
}

namespace Logic
{
    const MessageKey MessageKey::MAX(INT64_MAX, INT64_MAX - 1, QString(), -1, 0, core::message_type::base, false, preview_type::none, control_type::ct_message);
    const MessageKey MessageKey::MIN(2, 1, QString(), -1, 0, core::message_type::base, false, preview_type::none, control_type::ct_message);

    MessageKey::MessageKey()
        : id_(-1)
        , prev_(-1)
        , type_(core::message_type::base)
        , outgoing_(false)
        , pendingId_(-1)
        , time_(-1)
        , previewType_(preview_type::none)
        , controlType_(control_type::ct_message)
    {
    }

    MessageKey::MessageKey(
        const qint64 _id,
        const control_type _control_type)
            : id_(_id)
            , controlType_(_control_type)
            , prev_(-1)
            , type_(core::message_type::base)
            , outgoing_(false)
            , pendingId_(-1)
            , time_(-1)
            , previewType_(preview_type::none)

    {
    }

    MessageKey::MessageKey(
        const qint64 _id,
        const qint64 _prev,
        const QString& _internalId,
        const int _pendingId,
        const qint32 _time,
        const core::message_type _type,
        const bool _outgoing,
        const preview_type _previewType,
        const control_type _control_type)
        : id_(_id)
        , internalId_(_internalId)
        , type_(_type)
        , prev_(_prev)
        , outgoing_(_outgoing)
        , pendingId_(_pendingId)
        , time_(_time)
        , previewType_(_previewType)
        , controlType_(_control_type)
    {
        assert(type_ > core::message_type::min);
        assert(type_ < core::message_type::max);
        assert(previewType_ > preview_type::min);
        assert(previewType_ < preview_type::max);
    }

    bool MessageKey::operator<(const MessageKey& _other) const
    {
        return compare(_other);
    }

    bool MessageKey::hasId() const
    {
        return (id_ != -1);
    }

    bool MessageKey::checkInvariant() const
    {
        if (outgoing_)
        {
            if (!hasId() && internalId_.isEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!internalId_.isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    bool MessageKey::isChatEvent() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::chat_event);
    }

    bool MessageKey::isOutgoing() const
    {
        return outgoing_;
    }

    void MessageKey::setId(const int64_t _id)
    {
        assert(_id >= -1);

        id_ = _id;
    }

    qint64 MessageKey::getId() const
    {
        return id_;
    }

    void MessageKey::setOutgoing(const bool _isOutgoing)
    {
        outgoing_ = _isOutgoing;

        if (!hasId() && outgoing_)
        {
            assert(!internalId_.isEmpty());
        }
    }

    void MessageKey::setControlType(const control_type _controlType)
    {
        controlType_ = _controlType;
    }

    QString MessageKey::toLogStringShort() const
    {
        QString logStr;
        logStr.reserve(512);

        QTextStream fmt(&logStr);

        fmt << "id=" << id_ << ";"
            "prev=" << prev_;

        return logStr;
    }

    bool MessageKey::isFileSharing() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::file_sharing);
    }

    bool MessageKey::isVoipEvent() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::voip_event);
    }

    bool MessageKey::isDate() const
    {
        return (controlType_ == control_type::ct_date);
    }

    bool MessageKey::isSticker() const
    {
        assert(type_ >= core::message_type::min);
        assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::sticker);
    }

    control_type MessageKey::getControlType() const
    {
        return controlType_;
    }

    core::message_type MessageKey::getType() const
    {
        return type_;
    }

    void MessageKey::setType(core::message_type _type)
    {
        type_ = _type;
    }

    preview_type MessageKey::getPreviewType() const
    {
        return previewType_;
    }

    qint64 MessageKey::getPrev() const
    {
        return prev_;
    }

    QString MessageKey::getInternalId() const
    {
        return internalId_;
    }

    bool MessageKey::compare(const MessageKey& _rhs) const
    {
        if (!internalId_.isEmpty() && internalId_ == _rhs.internalId_)
        {
            return controlType_ < _rhs.controlType_;
        }

        if (pendingId_ != -1 && _rhs.pendingId_ == -1)
        {
            if (controlType_ != _rhs.controlType_)
                return controlType_ < _rhs.controlType_;

            if (controlType_ == _rhs.controlType_)
                return false;

            return time_ == _rhs.time_ ? false : time_ < _rhs.time_;
        }

        if (pendingId_ == -1 && _rhs.pendingId_ != -1)
        {
            if (controlType_ != _rhs.controlType_)
                return controlType_ < _rhs.controlType_;

            if (controlType_ == _rhs.controlType_)
                return true;

            return time_ == _rhs.time_ ? true : time_ < _rhs.time_;
        }

        if ((id_ != -1) && (_rhs.id_ != -1))
        {
            if (id_ != _rhs.id_)
            {
                return (id_ < _rhs.id_);
            }
        }

        if (pendingId_ != -1 && _rhs.pendingId_ != -1)
        {
            if (pendingId_ != _rhs.pendingId_)
            {
                return (pendingId_ < _rhs.pendingId_);
            }
            else if (internalId_ != _rhs.internalId_)
            {
                return time_ < _rhs.time_;
            }
        }

        return (controlType_ < _rhs.controlType_);
    }

    bool Message::isBase() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);

        return (key_.getType() == core::message_type::base);
    }

    bool Message::isChatEvent() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);

        assert(!buddy_ || key_.getType() == getBuddy()->GetType());
        return (key_.getType() == core::message_type::chat_event);
    }

    bool Message::isDate() const
    {
        return (key_.isDate());
    }

    bool Message::isDeleted() const
    {
        return deleted_;
    }

    void Message::setDeleted(const bool _deleted)
    {
        deleted_ = _deleted;

        if (buddy_)
        {
            buddy_->SetDeleted(_deleted);
        }
    }

    bool Message::isFileSharing() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::file_sharing) || fileSharing_);
        assert(!buddy_ || key_.getType() == buddy_->GetType());

        return (key_.getType() == core::message_type::file_sharing);
    }

    bool Message::isOutgoing() const
    {
        assert(!buddy_ || key_.isOutgoing() == getBuddy()->IsOutgoing());
        return key_.isOutgoing();
    }

    bool Message::isPending() const
    {
        return (!key_.hasId() && !key_.getInternalId().isEmpty());
    }

    bool Message::isStandalone() const
    {
        return true;
        //return (IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || IsPreview() || IsDeleted() || IsPending());
    }

    bool Message::isSticker() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::sticker) || sticker_);

        return (key_.getType() == core::message_type::sticker);
    }

    bool Message::isVoipEvent() const
    {
        assert(key_.getType() > core::message_type::min);
        assert(key_.getType() < core::message_type::max);
        assert((key_.getType() != core::message_type::voip_event) || voipEvent_);

        return (key_.getType() == core::message_type::voip_event);
    }

    bool Message::isPreview() const
    {
        return (key_.getPreviewType() != preview_type::none);
    }

    const HistoryControl::ChatEventInfoSptr& Message::getChatEvent() const
    {
        assert(!chatEvent_ || isChatEvent());

        return chatEvent_;
    }

    const QString& Message::getChatSender() const
    {
        return chatSender_;
    }

    const HistoryControl::FileSharingInfoSptr& Message::getFileSharing() const
    {
        assert(!fileSharing_ || isFileSharing());

        return fileSharing_;
    }


    const HistoryControl::StickerInfoSptr& Message::getSticker() const
    {
        assert(!sticker_ || isSticker());

        return sticker_;
    }

    const HistoryControl::VoipEventInfoSptr& Message::getVoipEvent() const
    {
        assert(!voipEvent_ || isVoipEvent());

        return voipEvent_;
    }

    void Message::setChatEvent(const HistoryControl::ChatEventInfoSptr& _info)
    {
        assert(!chatEvent_);
        assert(!_info || (key_.getType() == core::message_type::chat_event));

        chatEvent_ = _info;
    }

    void Message::setChatSender(const QString& _chatSender)
    {
        chatSender_ = _chatSender;
    }

    void Message::setFileSharing(const HistoryControl::FileSharingInfoSptr& _info)
    {
        assert(!fileSharing_);
        assert(!_info || (key_.getType() == core::message_type::file_sharing));

        fileSharing_ = _info;
    }

    void Message::setSticker(const HistoryControl::StickerInfoSptr& _info)
    {
        assert(!sticker_);
        assert(!_info || (key_.getType() == core::message_type::sticker));

        sticker_ = _info;
    }

    void Message::setVoipEvent(const HistoryControl::VoipEventInfoSptr& _info)
    {
        assert(!voipEvent_);
        assert(!_info || (key_.getType() == core::message_type::voip_event));

        voipEvent_ = _info;
    }

    void Message::applyModification(const Data::MessageBuddy& _modification)
    {
        assert(key_.getId() == _modification.Id_);

        EraseEventData();

        if (_modification.IsBase())
        {
            key_.setType(core::message_type::base);

            return;
        }

        if (_modification.IsChatEvent())
        {
            key_.setType(core::message_type::base);

            auto key = getKey();
            key.setType(core::message_type::chat_event);
            setKey(key);
            setChatEvent(_modification.GetChatEvent());
            getBuddy()->SetType(core::message_type::chat_event);

            return;
        }

        buddy_->ApplyModification(_modification);
    }

    void Message::EraseEventData()
    {
        fileSharing_.reset();
        sticker_.reset();
        chatEvent_.reset();
        voipEvent_.reset();
    }

    void Message::setBuddy(std::shared_ptr<Data::MessageBuddy> _buddy)
    {
        buddy_ = _buddy;
    }

    std::shared_ptr<Data::MessageBuddy> Message::getBuddy() const
    {
        return buddy_;
    }

    const QString& Message::getAimId() const
    {
        return aimId_;
    }

    void Message::setKey(const MessageKey& _key)
    {
        key_ = _key;
    }

    const MessageKey& Message::getKey() const
    {
        return key_;
    }

    const QDate& Message::getDate()
    {
        return date_;
    }

    void Message::setDate(const QDate& _date)
    {
        date_ = _date;
    }

    void Message::setChatFriendly(const QString& _friendly)
    {
        chatFriendly_ = _friendly;
    }

    const QString& Message::getChatFriendly()
    {
        return chatFriendly_;
    }


    //////////////////////////////////////////////////////////////////////////
    // ContactDialog
    //////////////////////////////////////////////////////////////////////////

    void ContactDialog::setLastRequestedMessage(const qint64 _message)
    {
        if (!lastRequestedMessage_)
        {
            lastRequestedMessage_.reset(new qint64(_message));

            return;
        }

        *lastRequestedMessage_ = _message;
    }

    qint64 ContactDialog::getLastRequestedMessage() const
    {
        if (!lastRequestedMessage_)
        {
            return -1;
        }

        return *lastRequestedMessage_;
    }

    bool ContactDialog::isLastRequestedMessageEmpty() const
    {
        return !lastRequestedMessage_;
    }



    MessagesMap& ContactDialog::getMessages()
    {
        if (!messages_)
            messages_.reset(new MessagesMap());

        return *messages_;
    }

    MessagesMap& ContactDialog::getPendingMessages()
    {
        if (!pendingMessages_)
            pendingMessages_.reset(new MessagesMap());

        return *pendingMessages_;
    }

    DatesMap& ContactDialog::getDatesMap()
    {
        if (!dateItems_)
            dateItems_.reset(new DatesMap());

        return *dateItems_;
    }

    const MessageKey& ContactDialog::getLastKey() const
    {
        return lastKey_;
    }

    const MessageKey& ContactDialog::getFirstKey() const
    {
        return firstKey_;
    }

    void ContactDialog::setLastKey(const MessageKey& _key)
    {
        lastKey_ = _key;
    }

    void ContactDialog::deleteLastKey()
    {
        lastKey_.setEmpty();
    }

    void ContactDialog::setFirstKey(const MessageKey& _key)
    {
        firstKey_ = _key;
    }

    bool ContactDialog::hasItemsInBetween(const MessageKey& _l, const MessageKey& _r) const
    {
        if (!messages_)
            return false;

        const auto iterL = messages_->upper_bound(_l);
        const auto iterR = messages_->lower_bound(_r);

        for (auto iter = iterL; iter != iterR; ++iter)
        {
            if (!iter->second.isDeleted())
            {
                return true;
            }
        }

        return false;
    }

    std::shared_ptr<Data::MessageBuddy> ContactDialog::addDateItem(const QString& _aimId, const MessageKey& _key, const QDate& _date)
    {
        assert(_date.isValid());
        assert(_key.hasId());
        assert(_key.getControlType() == control_type::ct_date);

        auto message = std::make_shared<Data::MessageBuddy>();
        message->Id_ = _key.getId();
        message->Prev_ = _key.getPrev();
        message->AimId_ = _aimId;
        message->SetTime(0);
        message->SetDate(_date);
        message->SetType(core::message_type::undefined);

        Message dateMessage(_aimId);
        dateMessage.setKey(_key);
        dateMessage.setDate(_date);
        getDatesMap().emplace(_date, dateMessage);

        __INFO(
            "gui_dates",
            "added gui date item\n"
            "    contact=<" << _aimId << ">\n"
            "    id=<" << _key.getId() << ">\n"
            "    prev=<" << _key.getPrev() << ">"
            );

        return message;
    }

    bool ContactDialog::hasDate(const QDate& _date) const
    {
        if (!dateItems_)
            return false;

        return (dateItems_->count(_date) > 0);
    }

    void ContactDialog::removeDateItem(const QDate& _date)
    {
        getDatesMap().erase(_date);
    }

    void ContactDialog::removeDateItems()
    {
        dateItems_.reset();
    }

    Logic::MessageKey ContactDialog::findFirstKeyAfter(const Logic::MessageKey& _key) const
    {
        assert(!_key.isEmpty());

        if (!messages_)
        {
            return Logic::MessageKey();
        }

        auto messageIter = messages_->upper_bound(_key);
        if ((messageIter == messages_->begin()) ||
            (messageIter == messages_->end()))
        {
            return Logic::MessageKey();
        }

        return messageIter->first;
    }

    int64_t ContactDialog::getLastMessageId() const
    {
        if (!messages_)
        {
            return -1;
        }

        if (!messages_->empty())
        {
            return messages_->crbegin()->first.getId();
        }

        return -1;
    }

    bool ContactDialog::hasPending() const
    {
        if (!pendingMessages_)
        {
            return false;
        }

        return !pendingMessages_->empty();
    }

    void ContactDialog::setNewKey(const MessageKey& _key)
    {
        if (!newKey_)
        {
            newKey_.reset(new MessageKey(_key));
        }

        *newKey_ = _key;
    }

    const MessageKey* ContactDialog::getNewKey() const
    {
        if (!newKey_)
        {
            return nullptr;
        }

        return newKey_.get();
    }

    void ContactDialog::resetNewKey()
    {
        newKey_.reset();
    }

    void ContactDialog::setRecvLastMessage(bool _value)
    {
        recvLastMessage_ = _value;
    }

    bool ContactDialog::getRecvLastMessage() const
    {
        return recvLastMessage_;
    }

    //////////////////////////////////////////////////////////////////////////
    // MessagesModel
    //////////////////////////////////////////////////////////////////////////
    std::unique_ptr<MessagesModel> g_messages_model;


    MessagesModel::MessagesModel(QObject *parent)
        : QObject(parent)
        , itemWidth_(0)
    {
        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messageBuddies,
            this,
            &MessagesModel::messageBuddies,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeleted,
            this,
            &MessagesModel::messagesDeleted,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesDeletedUpTo,
            this,
            &MessagesModel::messagesDeletedUpTo,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::messagesModified,
            this,
            &MessagesModel::messagesModified,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::dlgStates,
            this,
            &MessagesModel::dlgStates,
            Qt::QueuedConnection);

        connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::fileSharingUploadingResult,
            this,
            &MessagesModel::fileSharingUploadingResult,
            Qt::DirectConnection);
    }

    void MessagesModel::dlgStates(std::shared_ptr<QList<Data::DlgState>> _states)
    {
        for (auto _state : *_states)
        {
            emit readByClient(_state.AimId_, _state.TheirsLastRead_);
        }
    }

    void MessagesModel::fileSharingUploadingResult(
        QString _seq, bool /*success*/, QString /*localPath*/, QString /*uri*/, int /*contentType*/, bool _isFileTooBig)
    {
        if (_isFileTooBig)
            failedUploads_ << _seq;
    }

    void traceBuddies(std::shared_ptr<Data::MessageBuddies> _buddies, qint64 _seq)
    {
        for (const auto &buddy : *_buddies)
        {
            if (!buddy->IsFileSharing())
            {
                continue;
            }

            __TRACE(
                "fs",
                "incoming file sharing message in model\n" <<
                "    seq=<" << _seq << ">\n" <<
                buddy->GetFileSharing()->ToLogString());
        }
    }

    void MessagesModel::messageBuddiesUnloadUnusedMessages(std::shared_ptr<Data::MessageBuddies> _buddies, const QString& _aimId, bool& _hole)
    {
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        auto idFirst = _buddies->first()->Id_;

        QList<MessageKey> deletedValues;

        auto iter = dialogMessages.begin();
        while(iter != dialogMessages.end())
        {
            if (iter->first.isPending())
            {
                ++iter;
                continue;
            }

            if (iter->first.getId() < idFirst)
            {
                deletedValues << iter->first;

                if (iter->first.isDate())
                {
                    dialog.removeDateItem(iter->second.getDate());
                }

                if (iter->first.getId() <= dialog.getLastRequestedMessage())
                {
                    dialog.setLastRequestedMessage(-1);
                }

                iter = dialogMessages.erase(iter);

                continue;
            }

            if (iter->first.isDate() && (iter->second.getDate() == _buddies->first()->GetDate()))
            {
                deletedValues << iter->first;

                dialog.removeDateItem(iter->second.getDate());

                iter = dialogMessages.erase(iter);

                continue;
            }

            ++iter;
        }

        MessageKey key = ( dialogMessages.empty() ? _buddies->first()->ToKey() : dialogMessages.begin()->first);

        if (dialog.getLastKey() < key)
            dialog.setLastKey(key);

        sequences_.clear();
        dialog.setLastRequestedMessage(-1);

        _hole = !deletedValues.isEmpty();

        if (_hole)
        {
            emitDeleted(deletedValues, _aimId);
        }
    }

    void MessagesModel::messageBuddiesInsertMessages(
        std::shared_ptr<Data::MessageBuddies> _buddies,
        const QString& _aimId,
        const qint64 _modelFirst,
        const Ui::MessagesBuddiesOpt _option,
        const qint64 _seq,
        const bool _hole,
        int64_t _mess_id,
        int64_t _last_mess_id,
        model_regim _regim)

    {
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        if (_regim == model_regim::jump_to_bottom)
        {
            messagesDeletedUpTo(_aimId, -1);
            dialog.setLastRequestedMessage(-1);
        }

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        QList<MessageKey> updatedValues;

        std::list<MessagesMapIter> insertedMessages;

        for (auto msg : *_buddies)
        {
            const auto key = msg->ToKey();

            const auto messageAlreadyInserted = (std::find_if(dialogMessages.begin(), dialogMessages.end(), [&key](const std::pair<MessageKey, Message>& _msg)->bool
            {
                return (_msg.first == key);

            }) != dialogMessages.end());

            if (messageAlreadyInserted)
            {
                continue;
            }


            Message newMessage(_aimId);
            newMessage.setKey(key);
            newMessage.setChatSender(msg->GetChatSender());
            newMessage.setChatFriendly(msg->ChatFriendly_);
            newMessage.setFileSharing(msg->GetFileSharing());
            newMessage.setSticker(msg->GetSticker());
            newMessage.setChatEvent(msg->GetChatEvent());
            newMessage.setVoipEvent(msg->GetVoipEvent());
            newMessage.setDeleted(msg->IsDeleted());
            newMessage.setBuddy(msg);

            auto insertPos = dialogMessages.emplace(newMessage.getKey(), newMessage);
            if (insertPos.second)
            {
                insertedMessages.push_back(insertPos.first);
            }
        }

        //qDebug("insertedMessages %i", insertedMessages.size());

        for (auto iter : insertedMessages)
        {
            if (!updatedValues.contains(iter->first))
                updatedValues << iter->first;

            const auto &msgBuddy = iter->second.getBuddy();

            auto prevMsg = previousMessage(dialogMessages, iter);
            const auto havePrevMsg = (prevMsg != dialogMessages.end());

            auto nextMsg = nextMessage(dialogMessages, iter);
            const auto haveNextMsg = (nextMsg != dialogMessages.end());

            bool hasAvatar = iter->first.isVoipEvent() ? (!msgBuddy->IsOutgoingVoip()) : (!iter->first.isOutgoing());

            if (hasAvatar && havePrevMsg)
            {
                hasAvatar = msgBuddy->hasAvatarWith(*prevMsg->second.getBuddy(), isMultichat);
            }
            msgBuddy->SetHasAvatar(hasAvatar);

            if (haveNextMsg)
            {
                hasAvatar = !nextMsg->first.isOutgoing() && nextMsg->second.getBuddy()->hasAvatarWith(*msgBuddy, isMultichat);
                if (nextMsg->second.getBuddy()->HasAvatar() != hasAvatar)
                {
                    nextMsg->second.getBuddy()->SetHasAvatar(hasAvatar);
                    emit hasAvatarChanged(nextMsg->first, hasAvatar);
                }
            }


            removeDateItemIfOutdated(_aimId, *msgBuddy);

            const auto dateItemAlreadyExists = dialog.hasDate(msgBuddy->GetDate());
            const auto scheduleForDatesProcessing = (!dateItemAlreadyExists && !msgBuddy->IsDeleted());
            if (scheduleForDatesProcessing)
            {
                auto dateKey = msgBuddy->ToKey();
                dateKey.setType(core::message_type::undefined);
                dateKey.setControlType(control_type::ct_date);

                Message newMessage(_aimId);
                newMessage.setDate(msgBuddy->GetDate());
                newMessage.setKey(dateKey);
                newMessage.setDeleted(msgBuddy->IsDeleted());

                const auto isDateDiffer = (havePrevMsg && (msgBuddy->GetDate() != prevMsg->second.getBuddy()->GetDate()));
                const auto insertDateTablet = (!havePrevMsg || isDateDiffer);

                if (insertDateTablet)
                {
                    newMessage.setBuddy(dialog.addDateItem(_aimId, newMessage.getKey(), newMessage.getDate()));
                    auto insertPos = dialogMessages.emplace(newMessage.getKey(), newMessage);
                    hasAvatar = iter->first.isVoipEvent() ? (!msgBuddy->IsOutgoingVoip()) : (!iter->first.isOutgoing());
                    msgBuddy->SetHasAvatar(hasAvatar);
                    updatedValues << newMessage.getKey();
                }
            }
        }

        updateMessagesMarginsAndAvatars(_aimId);

        if (_regim == model_regim::first_load && sequences_.contains(_seq))
            emit ready(_aimId, false /* search */, -1 /* _mess_id */);

        if (_regim == model_regim::jump_to_msg && sequences_.contains(_seq))
            emit ready(_aimId, true /* search */, _mess_id);

        if (_option != Ui::MessagesBuddiesOpt::Requested || _regim == model_regim::jump_to_bottom)
        {
            //qDebug("updatedValues:%i", updatedValues.size());
            emitUpdated(updatedValues, _aimId, _hole ? HOLE : BASE);
        }
        else
        {
            if (subscribed_.contains(_aimId))
            {
                subscribed_.removeAll(_aimId);
                emit canFetchMore(_aimId);
            }
        }
    }

    void MessagesModel::messageBuddies(std::shared_ptr<Data::MessageBuddies> _buddies, QString _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid)
    {
        //qDebug("messageBuddies %i", _buddies->size());
        //for (auto& val : *_buddies)
        //{
        //    qDebug("  id:%lld", val->ToKey().getId());
        //}

        assert(_option > Ui::MessagesBuddiesOpt::Min);
        assert(_option < Ui::MessagesBuddiesOpt::Max);
        assert(_buddies);

        const auto isDlgState = (_option == Ui::MessagesBuddiesOpt::DlgState || _option == Ui::MessagesBuddiesOpt::Init || _option == Ui::MessagesBuddiesOpt::MessageStatus);
        const auto isPending = (_option == Ui::MessagesBuddiesOpt::Pending);

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        if (build::is_debug())
        {
            traceBuddies(_buddies, _seq);
        }

        auto regim = model_regim::normal_load;
        qint64 modelFirst = -1;
        if (dialogMessages.empty())
        {
            regim = model_regim::first_load;
        }
        else
        {
            modelFirst = dialogMessages.begin()->first.getPrev();
        }
        int64_t mess_id = -1;

        if (dialogMessages.empty() 
            && seqAndToOlder_.contains(_seq) 
            && seqAndToOlder_[_seq] != -1)
        {
            regim = model_regim::jump_to_msg;
            mess_id = seqAndToOlder_[_seq];
        }

        if (seqAndJumpBottom_.contains(_seq))
        {
            regim = model_regim::jump_to_bottom;
        }

        bool is_bottom_upload = false;
        if (seqAndToOlder_.contains(_seq) && seqAndToOlder_[_seq] != -1)
        {
            is_bottom_upload = true;
        }

        if (seqAndToOlder_.contains(_seq))
        {
            seqAndToOlder_.remove(_seq);
        }

        if (seqAndJumpBottom_.contains(_seq))
        {
            seqAndJumpBottom_.remove(_seq);
        }

        if (_havePending || isDlgState || isPending)
        {
            processPendingMessage(*_buddies, _aimId, _option);
        }

        if (!_buddies->isEmpty() && _buddies->last()->Id_ == _last_msgid)
        {
            getContactDialog(_aimId).setRecvLastMessage(true);
        }

        if (_buddies->isEmpty())
        {
            if (sequences_.contains(_seq))
            {
                getContactDialog(_aimId).setLastRequestedMessage(-1);
            }

            updateMessagesMarginsAndAvatars(_aimId);

            return;
        }

        if ((!sequences_.contains(_seq) && _buddies->last()->Id_ < modelFirst) || !requestedContact_.contains(_aimId))
        {
            updateMessagesMarginsAndAvatars(_aimId);

            return;
        }

        bool hole = false;
        hole |= is_bottom_upload;

        if (_option == Ui::MessagesBuddiesOpt::FromServer)
        {
            messageBuddiesUnloadUnusedMessages(_buddies, _aimId, hole);
        }

        messageBuddiesInsertMessages(_buddies, _aimId, modelFirst, _option, _seq, hole, mess_id, _last_msgid, regim);

        const auto isFromServer = (_option == Ui::MessagesBuddiesOpt::FromServer);
        const auto isRequested = (_option == Ui::MessagesBuddiesOpt::Requested);
        const auto isInit = (_option == Ui::MessagesBuddiesOpt::Init);

        if (isFromServer || isRequested || isInit)
        {
            if ((int32_t)dialogMessages.size() >= preloadCount())
            {
                setFirstMessage(
                    _aimId,
                    dialogMessages.begin()->first.getId()
                    );
            }
        }

        if (isInit || regim == model_regim::jump_to_msg)
        {
            updateLastSeen(_aimId);
        }
    }

    void MessagesModel::updateLastSeen(const QString& _aimid)
    {
        auto dlgState = Logic::getRecentsModel()->getDlgState(_aimid);
        if (dlgState.AimId_ != _aimid)
            dlgState = Logic::getUnknownsModel()->getDlgState(_aimid);

        emit readByClient(dlgState.AimId_, dlgState.TheirsLastRead_);
    }

    void MessagesModel::messagesDeleted(QString _aimId, QList<int64_t> _deletedIds)
    {
        assert(!_aimId.isEmpty());
        assert(!_deletedIds.isEmpty());

        auto &dialogMessages = getContactDialog(_aimId).getMessages();

        QList<Logic::MessageKey> messageKeys;

        for (const auto id : _deletedIds)
        {
            assert(id > 0);

            MessageKey newKey;
            newKey.setId(id);


            // find the deleted index record

            auto msgsRecIter = dialogMessages.find(newKey);
            if (msgsRecIter == dialogMessages.end())
            {
                continue;
            }

            msgsRecIter->second.setDeleted(true);

            auto key = msgsRecIter->first;

            messageKeys.push_back(key);
        }

        if (!messageKeys.empty())
        {
            emitDeleted(messageKeys, _aimId);
        }

        updateMessagesMarginsAndAvatars(_aimId);

        updateDateItems(_aimId);
    }

    void MessagesModel::messagesDeletedUpTo(QString _aimId, int64_t _id)
    {
        assert(!_aimId.isEmpty());

        auto &dialog = getContactDialog(_aimId);
        auto &dialogMessages = dialog.getMessages();

        QList<Logic::MessageKey> messageKeys;

        auto iter = dialogMessages.cbegin();
        for (; iter != dialogMessages.cend(); ++iter)
        {
            const auto currentId = iter->first.getId();
            if (currentId > _id && _id != -1)
            {
                break;
            }

            const Logic::MessageKey key(currentId, -1, QString(), -1, 0, core::message_type::base, false, Logic::preview_type::none, Logic::control_type::ct_message);

            messageKeys.push_back(key);
        }

        emitDeleted(messageKeys, _aimId);

        dialogMessages.erase(dialogMessages.begin(), iter);

        updateDateItems(_aimId);

        if (dialogMessages.empty())
        {
            dialog.deleteLastKey();
        }
        else
        {
            dialog.setLastKey(dialogMessages.begin()->first);
        }
    }

    void MessagesModel::setRecvLastMsg(QString _aimId, bool _value)
    {
        getContactDialog(_aimId).setRecvLastMessage(_value);
    }

    void MessagesModel::messagesModified(QString _aimId, std::shared_ptr<Data::MessageBuddies> _modifications)
    {
        assert(!_aimId.isEmpty());
        assert(_modifications);
        assert(!_modifications->empty());

        QList<Logic::MessageKey> messageKeys;

        for (const auto &modification : *_modifications)
        {
            if (!modification->HasText() &&
                !modification->IsChatEvent())
            {
                assert(!"unexpected modification");
                continue;
            }

            const auto key = applyMessageModification(_aimId, *modification);
            if (!key.hasId())
            {
                continue;
            }

            messageKeys.push_back(key);

            __INFO(
                "delete_history",
                "sending update request to the history page\n"
                "    message-id=<" << key.getId() << ">");
        }

        updateMessagesMarginsAndAvatars(_aimId);

        emitUpdated(messageKeys, _aimId, MODIFIED);
    }

    void MessagesModel::processPendingMessage(InOut Data::MessageBuddies& _msgs, const QString& _aimId, const Ui::MessagesBuddiesOpt _state)
    {
        assert(_state > Ui::MessagesBuddiesOpt::Min);
        assert(_state < Ui::MessagesBuddiesOpt::Max);

        if (_msgs.empty())
        {
            return;
        }

        QList<MessageKey> updatedValues;

        auto& dialog = getContactDialog(_aimId);
        auto& pendingMessages = dialog.getPendingMessages();
        auto& dialogMessages = dialog.getMessages();

        for (Data::MessageBuddies::iterator iter = _msgs.begin(); iter != _msgs.end();)
        {
            auto msg = *iter;

            MessageKey key = msg->ToKey();
            if (msg->IsPending())
            {
                msg->SetHasAvatar(!msg->IsOutgoing());

                Message newMessage(_aimId);
                newMessage.setKey(key);
                newMessage.setChatSender(msg->GetChatSender());
                newMessage.setChatFriendly(msg->ChatFriendly_);
                newMessage.setFileSharing(msg->GetFileSharing());
                newMessage.setSticker(msg->GetSticker());
                newMessage.setChatEvent(msg->GetChatEvent());
                newMessage.setVoipEvent(msg->GetVoipEvent());
                newMessage.setDeleted(msg->IsDeleted());
                newMessage.setBuddy(msg);

                iter = _msgs.erase(iter);

                pendingMessages.emplace(newMessage.getKey(), newMessage);
                dialogMessages.emplace(newMessage.getKey(), newMessage);

                updatedValues << newMessage.getKey();
            }
            else if (key.isOutgoing())
            {
                auto exist_pending = std::find_if(pendingMessages.begin(), pendingMessages.end(), [key](const std::pair<MessageKey, Message>& _pair)->bool
                {
                    return (key == _pair.first);
                });

                if (exist_pending != pendingMessages.end())
                {
                    assert(key.hasId());
                    assert(!key.getInternalId().isEmpty());
                    emit messageIdFetched(_aimId, key);

                    auto exist_message = std::find_if(dialogMessages.begin(), dialogMessages.end(), [key](const std::pair<MessageKey, Message>& _pair)->bool
                    {
                        return (key == _pair.first);
                    });

                    if (exist_message != dialogMessages.end())
                    {
                        dialogMessages.erase(exist_message);
                    }

                    pendingMessages.erase(exist_pending);
                }

                ++iter;
            }
            else
            {
                ++iter;
            }
        }


        if (_state != Ui::MessagesBuddiesOpt::Requested)
            emitUpdated(updatedValues, _aimId, BASE);
    }

    Data::MessageBuddy MessagesModel::item(const Message& _message)
    {
        Data::MessageBuddy result;

        result.Id_ = _message.getKey().getId();
        result.Prev_ = _message.getKey().getPrev();
        result.InternalId_ = _message.getKey().getInternalId();

        const auto &dialogMessages = getContactDialog(_message.getAimId()).getMessages();

        auto iterMsg = dialogMessages.find(_message.getKey());

        if (iterMsg != dialogMessages.end())
        {
            result.SetType(_message.getKey().getType());
            result.AimId_ = _message.getAimId();
            result.SetOutgoing(iterMsg->second.getBuddy()->IsOutgoing());
            result.Chat_ = iterMsg->second.getBuddy()->Chat_;
            result.SetChatSender(iterMsg->second.getBuddy()->GetChatSender());
            result.ChatFriendly_ = iterMsg->second.getBuddy()->ChatFriendly_;
            result.SetHasAvatar(iterMsg->second.getBuddy()->HasAvatar());
            result.SetIndentBefore(iterMsg->second.getBuddy()->GetIndentBefore());

            result.FillFrom(*iterMsg->second.getBuddy());

            result.SetTime(iterMsg->second.getBuddy()->GetTime());

            result.SetFileSharing(_message.getFileSharing());
            result.SetSticker(_message.getSticker());
            result.SetChatEvent(_message.getChatEvent());
            result.SetVoipEvent(_message.getVoipEvent());
            result.Quotes_ = _message.getBuddy()->Quotes_;

            if (_message.getKey().getId() == -1)
            {
                result.Id_ = -1;
            }
        }
        else
        {
            result.Id_ = -1;
        }

        return result;
    }

    void MessagesModel::removeDateItemIfOutdated(const QString& _aimId, const Data::MessageBuddy& _msg)
    {
        assert(_msg.HasId());

        const auto msgKey = _msg.ToKey();

        const auto &date = _msg.GetDate();

        auto& dialog = getContactDialog(_aimId);

        auto& dateItems = dialog.getDatesMap();

        auto dateItemIter = dateItems.find(date);
        if (dateItemIter == dateItems.end())
        {
            return;
        }

        auto &indexRecord = dateItemIter->second;

        const auto isOutdated = (msgKey < indexRecord.getKey());
        if (!isOutdated)
        {
            return;
        }

        dialog.getMessages().erase(indexRecord.getKey());

        QList<Logic::MessageKey> toRemove;
        toRemove << indexRecord.getKey();

        emitDeleted(toRemove, _aimId);

        dateItems.erase(dateItemIter);
    }



    void MessagesModel::emitUpdated(const QList<Logic::MessageKey>& _list, const QString& _aimId, unsigned _mode)
    {
        bool containsChatEvent = false;
        for (auto iter : _list)
        {
            if (iter.isChatEvent())
            {
                containsChatEvent = true;
                break;
            }
        }

        if (containsChatEvent && Logic::getContactListModel()->isChat(_aimId))
            emit chatEvent(_aimId);

        if (_list.size() <= moreCount())
        {
            emit updated(_list, _aimId, _mode);
            return;
        }

        int i = 0;
        QList<Logic::MessageKey> updatedList;
        for (auto iter : _list)
        {
            if (++i < moreCount())
            {
                updatedList.push_back(iter);
                continue;
            }

            updatedList.push_back(iter);

            i = 0;
            emit updated(updatedList, _aimId, _mode);
            updatedList.clear();
        }

        if (!updatedList.isEmpty())
            emit updated(updatedList, _aimId, _mode);
    }

    void MessagesModel::emitDeleted(const QList<Logic::MessageKey>& _list, const QString& _aimId)
    {
        emit deleted(_list, _aimId);
    }

    void MessagesModel::createFileSharingWidget(Ui::MessageItem& _messageItem, const Data::MessageBuddy& _messageBuddy) const
    {
        auto parent = &_messageItem;

        std::unique_ptr<HistoryControl::MessageContentWidget> item;

        const auto previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);


        item.reset(
            new HistoryControl::FileSharingWidget(
                parent,
                _messageBuddy.IsOutgoing(),
                _messageBuddy.AimId_,
                _messageBuddy.GetFileSharing(),
                previewsEnabled));

        item->setFixedWidth(itemWidth_);

        connect(
            item.get(),
            &HistoryControl::MessageContentWidget::removeMe,
            [_messageBuddy]
            {
                QList<MessageKey> keys;
                keys << _messageBuddy.ToKey();
                emit GetMessagesModel()->deleted(keys, _messageBuddy.AimId_);
            });

        _messageItem.setContentWidget(item.release());
    }

    MessageKey MessagesModel::applyMessageModification(const QString& _aimId, Data::MessageBuddy& _modification)
    {
        assert(_modification.AimId_ == _aimId);

        auto &dialogMessages = getContactDialog(_aimId).getMessages();

        auto key = _modification.ToKey();
        assert(key.hasId());

        // find index

        const auto existingIndexIter = findIndexRecord(dialogMessages, key);
        const auto isIndexMissing = (existingIndexIter == dialogMessages.end());
        if (isIndexMissing)
        {
            __INFO(
                "delete_history",
                "modification patch skipped\n"
                "    reason=<no-index>\n"
                "    message-id=<" << key.getId() << ">");

            return MessageKey();
        }

        // apply modification to the index record

        auto existingIndex = existingIndexIter->second;
        assert(!existingIndex.isDeleted());

        existingIndex.applyModification(_modification);
        existingIndex.getBuddy()->ApplyModification(_modification);
        key = existingIndex.getKey();

        auto insertionIter = existingIndexIter;
        ++insertionIter;

        dialogMessages.erase(existingIndexIter);

        dialogMessages.emplace_hint(insertionIter, std::make_pair(key, existingIndex));

        return key;
    }

    bool MessagesModel::hasItemsInBetween(const QString& _aimId, const Message& _l, const Message& _r) const
    {
        assert(!_aimId.isEmpty());

        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
            return false;

        return dialog->hasItemsInBetween(_l.getKey(), _r.getKey());
    }

    void MessagesModel::updateDateItems(const QString& _aimId)
    {
        assert(!_aimId.isEmpty());

        auto &dateRecords = getContactDialog(_aimId).getDatesMap();

        QList<MessageKey> toRemove;
        toRemove.reserve(dateRecords.size());

        for(
            auto dateRecordsIter = dateRecords.cbegin();
            dateRecordsIter != dateRecords.cend();
        )
        {
            const auto &dateIndexRecord = dateRecordsIter->second;

            Message nextDateIndexRecord(_aimId);

            auto nextDateRecordsIter = dateRecordsIter;
            ++nextDateRecordsIter;

            const auto isLastDateRecord = (nextDateRecordsIter == dateRecords.cend());
            if (isLastDateRecord)
            {
                nextDateIndexRecord.setKey(MessageKey::MAX);
            }
            else
            {
                nextDateIndexRecord = nextDateRecordsIter->second;
            }

            const auto hasItemsBetweenDates = hasItemsInBetween(_aimId, dateIndexRecord, nextDateIndexRecord);
            if (!hasItemsBetweenDates)
            {
                toRemove << dateIndexRecord.getKey();
                dateRecords.erase(dateRecordsIter);
            }

            dateRecordsIter = nextDateRecordsIter;
        }

        emitDeleted(toRemove, _aimId);
    }

    void MessagesModel::updateMessagesMarginsAndAvatars(const QString& _aimId)
    {
        assert(!_aimId.isEmpty());

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        const auto &dialogMessages = getContactDialog(_aimId).getMessages();

        auto messagesIter = dialogMessages.crbegin();

        for(;;)
        {
            for (;;)
            {
                const auto isFirstElementReached = (messagesIter == dialogMessages.crend());
                if (isFirstElementReached)
                {
                    return;
                }

                if (!messagesIter->second.getBuddy()->IsDeleted())
                {
                    break;
                }

                ++messagesIter;
            }

            auto &message = *messagesIter->second.getBuddy();

            const auto &messageKey = messagesIter->first;

            for (;;)
            {
                ++messagesIter;

                const auto isFirstElementReached = (messagesIter == dialogMessages.crend());
                if (isFirstElementReached)
                {
                    if (message.GetIndentBefore())
                    {
                        message.SetIndentBefore(false);
                        emit indentChanged(messageKey, false);
                    }

                    const auto hasAvatar = (message.IsVoipEvent() ? !message.IsOutgoingVoip() : !message.IsOutgoing());
                    if (hasAvatar != message.HasAvatar())
                    {
                        message.SetHasAvatar(hasAvatar);
                        emit hasAvatarChanged(messageKey, hasAvatar);
                    }

                    return;
                }

                if (!messagesIter->second.getBuddy()->IsDeleted())
                {
                    break;
                }
            }

            const auto &prevMessage = *messagesIter->second.getBuddy();

            const auto oldMessageIndent = message.GetIndentBefore();
            const auto newMessageIndent = message.GetIndentWith(prevMessage);

            if (newMessageIndent != oldMessageIndent)
            {
                message.SetIndentBefore(newMessageIndent);
                emit indentChanged(messageKey, newMessageIndent);
            }

            auto hasAvatar = (message.IsVoipEvent() ? !message.IsOutgoingVoip() : !message.IsOutgoing());
            if (hasAvatar)
            {
                hasAvatar = message.hasAvatarWith(prevMessage, isMultichat);
            }

            if (hasAvatar != message.HasAvatar())
            {
                message.SetHasAvatar(hasAvatar);
                emit hasAvatarChanged(messageKey, hasAvatar);
            }
        }
    }

    MessagesMapIter MessagesModel::findIndexRecord(MessagesMap& _indexRecords, const Logic::MessageKey& _key) const
    {
        assert(!_key.isEmpty());

        return std::find_if(_indexRecords.begin(), _indexRecords.end(), [&_key](const std::pair<MessageKey, Message>& _pair)
        {
            return (_pair.first == _key);
        });
    }

    Logic::MessageKey MessagesModel::findFirstKeyAfter(const QString& _aimId, const Logic::MessageKey& _key) const
    {
        assert(!_aimId.isEmpty());
        assert(!_key.isEmpty());

        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
        {
            return Logic::MessageKey();
        }

        return dialog->findFirstKeyAfter(_key);
    }

    void MessagesModel::requestMessages(const QString& _aimId, qint64 _messageId, bool _toOlder, bool _needPrefetch, bool _is_jump_to_bottom)
    {
        //qDebug("request message : %lld %i %i", _messageId, _toOlder, _needPrefetch);
        assert(!_aimId.isEmpty());

        if (!requestedContact_.contains(_aimId))
            requestedContact_.append(_aimId);

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        qint64 lastId = dialogMessages.empty() ? -1 : dialogMessages.begin()->second.getBuddy()->Id_;
        qint64 lastPrev = dialogMessages.empty() ? -1 : dialogMessages.begin()->second.getBuddy()->Prev_;

        if (_is_jump_to_bottom)
        {
            lastId = -1;
            _toOlder = true;
        }
        else
        {
            if (!_toOlder)
            {
                lastId = dialogMessages.empty() ? -1 : dialogMessages.rbegin()->second.getBuddy()->Id_;
            } 
            else if (!dialog.isLastRequestedMessageEmpty() && dialog.getLastRequestedMessage() == lastId)
            {
                return;
            }
        }

        if (lastId != -1 && lastPrev == -1 && _toOlder)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_int64("from", _messageId == -1 ? lastId : _messageId);
        collection.set_value_as_int64("count_early", _toOlder ? Data::PRELOAD_MESSAGES_COUNT : 0);
        collection.set_value_as_int64("count_later", _toOlder ? 0 : Data::PRELOAD_MESSAGES_COUNT);
        collection.set_value_as_bool("need_prefetch", _needPrefetch);
		collection.set_value_as_int64("last_read_msg", -1);
        auto seq = Ui::GetDispatcher()->post_message_to_core("archive/messages/get", collection.get());
        sequences_ << seq;

        if (_is_jump_to_bottom)
        {
            seqAndJumpBottom_.insert(seq, _messageId);
        }
        else
        {
            seqAndToOlder_.insert(seq, _messageId);
        }

        dialog.setLastRequestedMessage(lastId);
    }

    Ui::HistoryControlPageItem* MessagesModel::makePageItem(const Data::MessageBuddy& _msg, QWidget* _parent) const
    {
        if (_msg.IsEmpty())
            return 0;

        const auto isServiceMessage = (!_msg.IsBase() && !_msg.IsFileSharing() && !_msg.IsSticker() && !_msg.IsChatEvent() && !_msg.IsVoipEvent());
        if (isServiceMessage)
        {
            std::unique_ptr<Ui::ServiceMessageItem> serviceMessageItem(new Ui::ServiceMessageItem(_parent));

            serviceMessageItem->setDate(_msg.GetDate());
            serviceMessageItem->setWidth(itemWidth_);
            serviceMessageItem->setContact(_msg.AimId_);
            serviceMessageItem->updateStyle();
            serviceMessageItem->setDeleted(_msg.IsDeleted());

            return serviceMessageItem.release();
        }

        if (_msg.IsChatEvent())
        {
            std::unique_ptr<Ui::ChatEventItem> item(new Ui::ChatEventItem(_parent, _msg.GetChatEvent(), _msg.Id_));
            item->setContact(_msg.AimId_);
            item->setHasAvatar(_msg.HasAvatar());
            item->setFixedWidth(itemWidth_);
            item->setDeleted(_msg.IsDeleted());
            return item.release();
        }

        if (_msg.IsVoipEvent())
        {
            const auto &voipEvent = _msg.GetVoipEvent();

            std::unique_ptr<Ui::VoipEventItem> item(new Ui::VoipEventItem(_parent, voipEvent));
            item->setTopMargin(_msg.GetIndentBefore());
            item->setFixedWidth(itemWidth_);
            item->setHasAvatar(_msg.HasAvatar());
            item->setId(_msg.Id_);
            item->setDeleted(_msg.IsDeleted());
            return item.release();
        }

        if (_msg.IsDeleted())
        {
            std::unique_ptr<Ui::DeletedMessageItem> deletedItem(new Ui::DeletedMessageItem(_parent));
            deletedItem->setDeleted(true);
            return deletedItem.release();
        }

        const auto sender =
            (_msg.Chat_ && _msg.HasChatSender()) ?
                NormalizeAimId(_msg.GetChatSender()) :
                _msg.AimId_;


        const bool previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
        const bool isSitePreview = ((previewsEnabled && (_msg.GetPreviewableLinkType() == preview_type::site)));
        const bool is_not_auth = (!_msg.Chat_ && Logic::getContactListModel()->isNotAuth(_msg.AimId_));

        if (isSitePreview || !_msg.Quotes_.isEmpty() || _msg.IsSticker() || _msg.ContainsPttAudio())
        {
            QString senderFriendly;

            if (_msg.IsOutgoing())
            {
                senderFriendly = Ui::MyInfo()->friendlyName();
            }
            else if (_msg.Chat_)
            {
                senderFriendly = GetChatFriendly(_msg.GetChatSender(), _msg.ChatFriendly_);
            }
            else
            {
                senderFriendly = Logic::getContactListModel()->getDisplayName(_msg.AimId_);
            }

            std::unique_ptr<Ui::ComplexMessage::ComplexMessageItem> item(
                Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(
                    _parent,
                    _msg.Id_,
                    _msg.GetDate(),
                    _msg.Prev_,
                    _msg.GetText(),
                    _msg.AimId_,
                    sender,
                    senderFriendly,
                    _msg.Quotes_,
                    _msg.GetSticker(),
                    _msg.IsOutgoing(),
                    is_not_auth));

            item->setContact(_msg.AimId_);
            item->setTime(_msg.GetTime());
            item->setHasAvatar(_msg.HasAvatar());
            item->setTopMargin(_msg.GetIndentBefore());
            item->setDeliveredToServer(_msg.IsDeliveredToServer(), true);

            if (_msg.Chat_ && !senderFriendly.isEmpty())
            {
                item->setMchatSender(senderFriendly);
            }

            item->setFixedWidth(itemWidth_);

            return item.release();
        }

        std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem(_parent));
        messageItem->setContact(_msg.AimId_);
        messageItem->setId(_msg.Id_, _msg.AimId_);
        messageItem->setSender(sender);
        messageItem->setHasAvatar(_msg.HasAvatar());
        if (_msg.HasAvatar())
        {
            messageItem->loadAvatar(Utils::scale_bitmap(Ui::MessageStyle::getAvatarSize()));
        }

        messageItem->setTopMargin(_msg.GetIndentBefore());
        messageItem->setOutgoing(_msg.IsOutgoing(), _msg.IsDeliveredToServer(), _msg.Chat_, true);
        messageItem->setMchatSender(GetChatFriendly(_msg.GetChatSender(), _msg.ChatFriendly_));
        messageItem->setMchatSenderAimId(_msg.HasChatSender() ? _msg.GetChatSender(): _msg.AimId_);
        messageItem->setTime(_msg.GetTime());
        messageItem->setDate(_msg.GetDate());
        messageItem->setDeleted(_msg.IsDeleted());
        messageItem->setNotAuth(is_not_auth);

        if (_msg.IsFileSharing())
        {
            createFileSharingWidget(*messageItem, _msg);
        }
        else
        {
            messageItem->setMessage(_msg.GetText());
        }

        return messageItem.release();
    }


    Ui::HistoryControlPageItem* MessagesModel::fillItemById(const QString& _aimId, const MessageKey& _key,  QWidget* _parent)
    {
        assert(!_aimId.isEmpty());

        if (_key.getControlType() == control_type::ct_new_messages)
        {
            return createNew(_aimId, _key, _parent);
        }

        if (failedUploads_.contains(_key.getInternalId()))
        {
            failedUploads_.removeAll(_key.getInternalId());
            return nullptr;
        }

        auto& dialog = getContactDialog(_aimId);

        auto &pendingMessages = dialog.getPendingMessages();

        auto current_pending = pendingMessages.crbegin();

        while (current_pending != pendingMessages.crend())
        {
            if (current_pending->first == _key)
            {
                break;
            }

            ++current_pending;
        }


        if (current_pending != pendingMessages.crend())
        {
            // merge message widgets
            auto result = item(current_pending->second);

            return makePageItem(result, _parent);
        }

        auto &dialogMessages = dialog.getMessages();

        auto haveIncoming = false;

        auto currentMessage = dialogMessages.crbegin();

        while (currentMessage != dialogMessages.crend())
        {
            haveIncoming |= !currentMessage->first.isOutgoing();

            if (currentMessage->first == _key)
            {
                break;
            }

            ++currentMessage;
        }

        if (currentMessage == dialogMessages.crend())
        {
            return nullptr;
        }

        // merge message widgets
        auto result = item(currentMessage->second);

        return makePageItem(result, _parent);
    }

    void MessagesModel::setFirstMessage(const QString& aimId, qint64 msgId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        collection.set_value_as_int64("message", msgId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/set_first_message", collection.get());
    }

    MessagesMapIter MessagesModel::previousMessage(MessagesMap& _map, MessagesMapIter _iter) const
    {
        MessagesMapIter result = _map.end();

        while (_iter != _map.begin())
        {
            --_iter;

            if (_iter->first.isDate() || _iter->second.isDeleted())
            {
                continue;
            }

            result = _iter;

            break;
        }

        return result;
    }

    MessagesMapIter MessagesModel::nextMessage(MessagesMap& _map, MessagesMapIter _iter) const
    {
        auto iter = _iter;

        while (++iter != _map.end())
        {
            if (iter->first.isDate() || iter->second.isDeleted())
            {
                continue;
            }

            break;
        }

        return iter;
    }

    void MessagesModel::contactChanged(QString contact, qint64 _messageId, qint64 quote_id)
    {
        if (contact.isEmpty())
        {
            return;
        }

        Ui::GetDispatcher()->contactSwitched(contact);

        const ContactDialog* dialog = getContactDialogConst(contact);

        if (dialog && !dialog->isLastRequestedMessageEmpty() && _messageId == -1)
        {
            return;
        }

        requestMessages(contact, _messageId, true /* to_older */, _messageId == -1, false /* _is_jump_to_bottom */);
        if (_messageId != -1)
            requestMessages(contact, _messageId, false /* to_older */, _messageId == -1, false /* _is_jump_to_bottom */);
    }

    void MessagesModel::setItemWidth(int width)
    {
        itemWidth_ = width;
    }

    QMap<MessageKey, Ui::HistoryControlPageItem*> MessagesModel::tail(const QString& aimId, QWidget* parent, bool _is_search, int64_t _mess_id, bool _is_jump_to_bottom)
    {
        QMap<MessageKey, Ui::HistoryControlPageItem*> result;

        auto& dialog = getContactDialog(aimId);
        auto& dialogMessages = dialog.getMessages();
        auto& pendingMessages = dialog.getPendingMessages();

        if (!_is_jump_to_bottom && dialogMessages.empty() && pendingMessages.empty())
            return result;

        int i = 0;
        MessageKey key;

        MessagesMap::reverse_iterator iterPending = pendingMessages.rbegin();
        
        auto margin = 0;
        if (_mess_id != -1)
        {
            auto index_of_msg = 0;
            auto iter_of_msg = dialogMessages.begin();
            while (iter_of_msg != dialogMessages.end() && iter_of_msg->first.getId() != _mess_id)
            {
                ++index_of_msg;
                ++iter_of_msg;
            }

            assert(iter_of_msg != dialogMessages.end());

            margin = std::min<int>(std::max<int>(dialogMessages.size() - index_of_msg - 3, 0), moreCount() / 2);
        }

        while (iterPending != pendingMessages.rend())
        {
            assert(!iterPending->second.isDeleted());

            key = iterPending->first;

            Data::MessageBuddy buddy = item(iterPending->second);

            result.insert(key, makePageItem(buddy, parent));

            if (++i == moreCount())
            {
                break;
            }

            ++iterPending;
        }

        if (i < moreCount())
        {
            bool haveImcoming = false;
            auto iterIndex = dialogMessages.rbegin();
            
            auto endIter = dialogMessages.rend();
            std::advance(iterIndex, _is_search ? margin : 0);
            while (iterIndex != endIter)
            {
                haveImcoming |= !iterIndex->first.isOutgoing();
                key = iterIndex->first;

                auto buddy = item(iterIndex->second);

                result.insert(key, makePageItem(buddy, parent));

                if (++i == moreCount())
                {
                    break;
                }

                ++iterIndex;
            }
        }

        dialog.setLastKey(key);
        dialog.setFirstKey(key);

        if (dialog.getLastKey().isEmpty() 
            || dialog.getLastKey().isPending()
            || ( dialog.getLastKey().getPrev() != -1 && dialogMessages.begin()->first.getId() == dialog.getLastKey().getId() )
            || _is_jump_to_bottom
           )
            requestMessages(aimId, -1 /* _messageId */, true /* to_older */, true /* _needPrefetch */, _is_jump_to_bottom);

        if (result.isEmpty())
            subscribed_ << aimId;

        //qDebug("tail : %lld %i %lld", _mess_id, result.size(), key.getId());
        return result;
    }

    QMap<MessageKey, Ui::HistoryControlPageItem*> MessagesModel::more(const QString& aimId, QWidget* parent, bool _isMoveToBottomIfNeed)
    {
        auto& dialog = getContactDialog(aimId);
        auto& dialogMessages = dialog.getMessages();
        auto& pendingMessages = dialog.getPendingMessages();

        if (dialog.getLastKey().isEmpty())
            return tail(aimId, parent, false /* _is_search */, -1 /* _mess_id */);

        QMap<MessageKey, Ui::HistoryControlPageItem*> result;
        if (dialogMessages.empty())
            return result;

        auto i = 0;
        MessageKey key = dialog.getLastKey();
        MessageKey firstKey = dialog.getFirstKey();

        MessagesMap::const_reverse_iterator iterPending = pendingMessages.rbegin();

        while (iterPending != pendingMessages.rend())
        {
            assert(!iterPending->second.isDeleted());

            if (!(iterPending->first < key))
            {
                ++iterPending;
                continue;
            }

            key = iterPending->first;
            Data::MessageBuddy buddy = item(iterPending->second);

            result.insert(key, makePageItem(buddy, parent));

            if (++i == moreCount())
            {
                break;
            }

            ++iterPending;
        }

        if (i < moreCount())
        {
            if (_isMoveToBottomIfNeed)
            {
                auto iterIndex = dialogMessages.rbegin();
                auto iterEnd = dialogMessages.rend();
            
                while (iterIndex != iterEnd)
                {
                    if (!(iterIndex->first < key))
                    {
                        ++iterIndex;
                        continue;
                    }

                    Data::MessageBuddy buddy = item(iterIndex->second);
                    result.insert(iterIndex->first, makePageItem(buddy, parent));

                    if (iterIndex->first < key)
                    {
                        key = iterIndex->first;
                    }

                    if (++i == moreCount())
                    {
                        break;
                    }

                    ++iterIndex;
                }
            }
            else
            {
                auto iterIndex = dialogMessages.begin();
                auto iterEnd = dialogMessages.end();
            
                while (iterIndex != iterEnd)
                {
                    if (!(firstKey < iterIndex->first))
                    {
                        ++iterIndex;
                        continue;
                    }

                    Data::MessageBuddy buddy = item(iterIndex->second);
                    result.insert(iterIndex->first, makePageItem(buddy, parent));

                    if (!(iterIndex->first < firstKey))
                    {
                        firstKey = iterIndex->first;
                    }

                    if (++i == moreCount())
                    {
                        break;
                    }

                    ++iterIndex;
                }
            }
        }

        dialog.setLastKey(key);
        dialog.setFirstKey(firstKey);

        if (
            dialog.getLastKey().isEmpty() ||
            dialog.getLastKey().isPending() ||
            (
                (dialog.getLastKey().getPrev() != -1) &&
                dialogMessages.begin()->first.getId() == dialog.getLastKey().getId()
            )
        )
        {
            if (_isMoveToBottomIfNeed)
                requestMessages(aimId, -1 /* _messageId */, true /* _toOlder */, true /* _needPrefetch */, false /* _is_jump_to_bottom */);
        }

        if (!dialog.getRecvLastMessage())
        {
            if (!_isMoveToBottomIfNeed)
                requestMessages(aimId, -1 /* _messageId */, false /* _toOlder */, true /* _needPrefetch */, false /* _is_jump_to_bottom */);
        }

        if (!result.isEmpty())
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_preload);
        }
        else
        {
            subscribed_ << aimId;
        }

        //qDebug("more : %i %lld %lld", result.size(), key.getId(), firstKey.getId());
        return result;
    }

    Ui::HistoryControlPageItem* MessagesModel::getById(const QString& aimId, const MessageKey& key, QWidget* parent)
    {
        return fillItemById(aimId, key, parent);
    }

    int32_t MessagesModel::preloadCount() const
    {
        return Data::PRELOAD_MESSAGES_COUNT;
    }

    int32_t MessagesModel::moreCount() const
    {
        return Data::MORE_MESSAGES_COUNT;
    }

    void MessagesModel::setLastKey(const MessageKey& key, const QString& aimId)
    {
        assert(!key.isEmpty());
        assert(!aimId.isEmpty());

        __TRACE(
            "history_control",
            "setting a last key for the dialog\n"
            "	contact=<" << aimId << ">"
            "	key=<" << key.toLogStringShort() << ">");

        auto& dialog = getContactDialog(aimId);
        auto& dialogMessages = dialog.getMessages();

        dialog.setLastKey(key);

        auto iter = dialogMessages.begin();
        while(iter != dialogMessages.end())
        {
            if (iter->first < key)
            {
                if (iter->second.isDate())
                {
                    dialog.removeDateItem(iter->second.getDate());
                }

                if (iter->first.getId() <= dialog.getLastRequestedMessage())
                    dialog.setLastRequestedMessage(-1);

                iter = dialogMessages.erase(iter);
            }
            else
            {
                break;
            }
        }

        if ((int32_t)dialogMessages.size() >= preloadCount())
        {
            setFirstMessage(aimId, dialogMessages.begin()->first.getId());
        }
    }

    void MessagesModel::removeDialog(const QString& _aimId)
    {
        dialogs_.remove(_aimId);

        requestedContact_.removeAll(_aimId);
    }

    Ui::ServiceMessageItem* MessagesModel::createNew(const QString& _aimId, const MessageKey& /*_key*/, QWidget* _parent) const
    {
        assert(_parent);

        std::unique_ptr<Ui::ServiceMessageItem> newPlate(new Ui::ServiceMessageItem(_parent));

        newPlate->setWidth(itemWidth_);
        newPlate->setNew();
        newPlate->setContact(_aimId);
        newPlate->updateStyle();

        return newPlate.release();
    }

    void MessagesModel::hideNew(const QString& _aimId)
    {
        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        if (!dialog.getNewKey())
            return;

        const auto isMultichat = Logic::getContactListModel()->isChat(_aimId);

        const Logic::MessageKey& key = *dialog.getNewKey();

        QList<Logic::MessageKey> keyslist;
        keyslist.push_back(key);
        MessagesModel::emitDeleted(keyslist, _aimId);

        auto iter_message_after_new = dialogMessages.rend();

        for (auto iterMessage = dialogMessages.rbegin(); iterMessage != dialogMessages.rend(); ++iterMessage)
        {
            if (iterMessage->first.getId() <= key.getId())
                break;

            if (!iterMessage->first.isOutgoing() && !iterMessage->second.isDeleted() && iterMessage->first.getControlType() == control_type::ct_message)
            {
                iter_message_after_new = iterMessage;
                continue;
            }

            iter_message_after_new = dialogMessages.rend();
        }

        if (iter_message_after_new != dialogMessages.rend())
        {
            auto iter_prev_message = iter_message_after_new;
            ++iter_prev_message;

            bool hasAvatar = false;
            if (iter_prev_message == dialogMessages.rend() || iter_message_after_new->second.getBuddy()->hasAvatarWith(*iter_prev_message->second.getBuddy(), isMultichat))
                hasAvatar = true;

            if (!hasAvatar)
                emit hasAvatarChanged(iter_message_after_new->first, false);
        }

        dialog.resetNewKey();
    }

    void MessagesModel::updateNew(const QString& _aimId, const qint64 _newId, const bool _hide)
    {
        hideNew(_aimId);

        if (_hide || _newId <= 0)
            return;

        auto& dialog = getContactDialog(_aimId);
        auto& dialogMessages = dialog.getMessages();

        bool isShow = false;

        auto iter_prev_message = dialogMessages.rend();

        for (auto iterMessage = dialogMessages.rbegin(); iterMessage != dialogMessages.rend(); ++iterMessage)
        {
            if (iterMessage->first.getId() <= _newId)
                break;

            if (!iterMessage->first.isOutgoing() && !iterMessage->second.isDeleted() && iterMessage->first.getControlType() == control_type::ct_message)
            {
                isShow = true;
                iter_prev_message = iterMessage;

                continue;
            }

            iter_prev_message = dialogMessages.rend();
        }

        if (!isShow)
            return;

        dialog.setNewKey(MessageKey(_newId, control_type::ct_new_messages));

        QList<MessageKey> updatedValues;
        updatedValues << *dialog.getNewKey();

        emitUpdated(updatedValues, _aimId, NEW_PLATE);

        if (iter_prev_message != dialogMessages.rend())
        {
            emit hasAvatarChanged(iter_prev_message->first, true);
        }
    }

    QString MessagesModel::formatRecentsText(const Data::MessageBuddy &buddy) const
    {
        if (buddy.IsServiceMessage())
        {
            return QString();
        }

        if (buddy.IsChatEvent())
        {
            std::unique_ptr<Ui::ChatEventItem> item(new Ui::ChatEventItem(buddy.GetChatEvent(), buddy.Id_));
            item->setContact(buddy.AimId_);
            return item->formatRecentsText();
        }

        if (buddy.IsVoipEvent())
        {
            std::unique_ptr<Ui::VoipEventItem> item(new Ui::VoipEventItem(buddy.GetVoipEvent()));
            return item->formatRecentsText();
        }

        if (buddy.ContainsPreviewableSiteLink() || !buddy.Quotes_.isEmpty() || buddy.IsSticker())
        {
            std::unique_ptr<Ui::ComplexMessage::ComplexMessageItem> item(
                Ui::ComplexMessage::ComplexMessageItemBuilder::makeComplexItem(
                    nullptr,
                    0,
                    QDate::currentDate(),
                    0,
                    buddy.GetText(),
                    buddy.AimId_,
                    buddy.AimId_,
                    buddy.AimId_,
                    buddy.Quotes_,
                    buddy.GetSticker(),
                    false,
                    false));

            return item->formatRecentsText();
        }

        std::unique_ptr<Ui::MessageItem> messageItem(new Ui::MessageItem());
        messageItem->setContact(buddy.AimId_);
        if (buddy.IsFileSharing())
        {
            if (buddy.ContainsPttAudio())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Voice message");
            }

            if (buddy.ContainsImage())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Photo");
            }

            if (buddy.ContainsVideo())
            {
                return QT_TRANSLATE_NOOP("contact_list", "Video");
            }

            auto item = new HistoryControl::FileSharingWidget(buddy.GetFileSharing(), buddy.AimId_);
            messageItem->setContentWidget(item);
        }
        else
        {
            messageItem->setMessage(buddy.GetText());
        }

        return messageItem->formatRecentsText();
    }

    int64_t MessagesModel::getLastMessageId(const QString &aimId) const
    {
        assert(!aimId.isEmpty());

        auto dialog = getContactDialogConst(aimId);
        if (!dialog)
            return -1;

        return dialog->getLastMessageId();
    }

    qint64 MessagesModel::normalizeNewMessagesId(const QString& _aimid, qint64 _id)
    {
        if (_id == -1)
            return _id;

        auto& dialog = getContactDialog(_aimid);
        auto& dialogMessages = dialog.getMessages();

        qint64 newId = -1;

        MessagesMap::reverse_iterator iter = dialogMessages.rbegin();

        while (iter != dialogMessages.rend() && iter->first.getId() >= _id)
        {
            if (iter->first.isOutgoing())
            {
                newId = iter->first.getId();
                break;
            }

            newId = iter->first.getId();
            ++iter;
        }

        return newId == -1 ? _id : newId;
    }

    bool MessagesModel::hasPending(const QString& _aimId) const
    {
        const ContactDialog* dialog = getContactDialogConst(_aimId);
        if (!dialog)
        {
            return false;
        }

        return dialog->hasPending();
    }

    void MessagesModel::eraseHistory(const QString& _aimid)
    {
        const auto lastMessageId = getLastMessageId(_aimid);

        if (lastMessageId > 0)
        {
            Ui::GetDispatcher()->deleteMessagesFrom(_aimid, lastMessageId);

            Utils::InterConnector::instance().setSidebarVisible(false);
        }

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::history_delete);
    }

    void MessagesModel::emitQuote(int64_t quote_id)
    {
        emit quote(quote_id);
    }

    ContactDialog& MessagesModel::getContactDialog(const QString& _aimid)
    {
        auto iter = dialogs_.find(_aimid);
        if (iter == dialogs_.end())
        {
            iter = dialogs_.insert(_aimid, std::make_shared<ContactDialog>());
        }

        return *iter.value();
    }

    const ContactDialog* MessagesModel::getContactDialogConst(const QString& _aimid) const
    {
        const auto iter = dialogs_.find(_aimid);
        if (iter == dialogs_.end())
            return nullptr;

        return iter.value().get();
    }

    MessagesModel* GetMessagesModel()
    {
        if (!g_messages_model)
            g_messages_model.reset(new MessagesModel(0));

        return g_messages_model.get();
    }



    void ResetMessagesModel()
    {
        if (g_messages_model)
            g_messages_model.reset();
    }
}