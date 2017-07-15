#pragma once

#include "../../types/message.h"

namespace core
{
    enum class message_type;
}

namespace HistoryControl
{
    typedef std::shared_ptr<class FileSharingInfo> FileSharingInfoSptr;
    typedef std::shared_ptr<class StickerInfo> StickerInfoSptr;
    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

}

namespace Ui
{
    class MessageItem;
    class ServiceMessageItem;
    class HistoryControlPageItem;
    enum class MessagesBuddiesOpt;
}

namespace Logic
{
    enum class control_type
    {
        ct_date         = 0,
        ct_message      = 1,
        ct_new_messages = 2
    };

    enum class preview_type
    {
        min,

        none,
        site,

        max
    };

    enum class model_regim
    {
        min,

        jump_to_msg,
        jump_to_bottom,
        first_load,
        normal_load,

        max = 0
    };

    class MessageKey
    {
    public:
        static const MessageKey MAX;
        static const MessageKey MIN;

        MessageKey();

        MessageKey(
            const qint64 _id,
            const control_type _control_type);


        MessageKey(
            const qint64 _id,
            const qint64 _prev,
            const QString& _internalId,
            const int _pendingId,
            const qint32 _time,
            const core::message_type _type,
            const bool _outgoing,
            const preview_type _previewType,
            const control_type _control_type);

        bool isEmpty() const
        {
            return id_ == -1 && prev_ == -1 && internalId_.isEmpty();
        }

        void setEmpty()
        {
            id_ = -1;
            prev_ = -1;
            internalId_.clear();
        }

        bool isPending() const
        {
            return id_ == -1 && !internalId_.isEmpty();
        }

        bool operator==(const MessageKey& _other) const
        {
            const auto idsEqual = (hasId() && id_ == _other.id_);

            const auto internalIdsEqual = (!internalId_.isEmpty() && (internalId_ == _other.internalId_));

            if (idsEqual || internalIdsEqual)
            {
                return controlType_ == _other.controlType_;
            }

            return false;
        }

        bool operator!=(const MessageKey& _other) const
        {
            return !operator==(_other);
        }

        bool operator<(const MessageKey& _other) const;

        bool hasId() const;

        bool checkInvariant() const;

        bool isChatEvent() const;

        bool isOutgoing() const;

        bool isFileSharing() const;

        bool isVoipEvent() const;

        bool isSticker() const;

        bool isDate() const;

        void setId(const int64_t _id);

        qint64 getId() const;

        void setOutgoing(const bool _isOutgoing);

        QString toLogStringShort() const;

        void setControlType(const control_type _controlType);
        control_type getControlType() const;

        core::message_type getType() const;
        void setType(core::message_type _type);

        preview_type getPreviewType() const;

        qint64 getPrev() const;

        QString getInternalId() const;

    private:

        qint64 id_;

        qint64 prev_;

        QString internalId_;

        core::message_type type_;

        control_type controlType_;

        qint32 time_;

        int pendingId_;

        preview_type previewType_;

        bool outgoing_;

        bool compare(const MessageKey& _rhs) const;

    };

    typedef std::set<MessageKey> MessageKeySet;

    typedef std::vector<MessageKey> MessageKeyVector;

    typedef std::map<MessageKey, class Message> MessagesMap;

    typedef MessagesMap::iterator MessagesMapIter;

    class Message
    {

    public:

        Message(const QString& _aimId)
            : deleted_(false)
            , aimId_(_aimId)
        {
        }

        bool isBase() const;

        bool isChatEvent() const;

        bool isDate() const;

        bool isDeleted() const;

        bool isFileSharing() const;

        bool isOutgoing() const;

        bool isPending() const;

        bool isStandalone() const;

        bool isSticker() const;

        bool isVoipEvent() const;

        bool isPreview() const;

        const HistoryControl::ChatEventInfoSptr& getChatEvent() const;

        const QString& getChatSender() const;

        const HistoryControl::FileSharingInfoSptr& getFileSharing() const;

        const HistoryControl::StickerInfoSptr& getSticker() const;

        const HistoryControl::VoipEventInfoSptr& getVoipEvent() const;

        void setChatEvent(const HistoryControl::ChatEventInfoSptr& _info);

        void setChatSender(const QString& _chatSender);

        void setDeleted(const bool _deleted);

        void setFileSharing(const HistoryControl::FileSharingInfoSptr& info);

        void setSticker(const HistoryControl::StickerInfoSptr& _info);

        void setVoipEvent(const HistoryControl::VoipEventInfoSptr& _info);

        void removeMessageKey(MessageKey _key) const;

        void applyModification(const Data::MessageBuddy& _modification);

        void setBuddy(std::shared_ptr<Data::MessageBuddy> _buddy);
        std::shared_ptr<Data::MessageBuddy> getBuddy() const;

        const QString& getAimId() const;

        void setKey(const MessageKey& _key);
        const MessageKey& getKey() const;

        const QDate& getDate();
        void setDate(const QDate& _date);

        void setChatFriendly(const QString& _friendly);
        const QString& getChatFriendly();

    private:

        HistoryControl::FileSharingInfoSptr fileSharing_;

        HistoryControl::StickerInfoSptr sticker_;

        HistoryControl::ChatEventInfoSptr chatEvent_;

        HistoryControl::VoipEventInfoSptr voipEvent_;

        QString chatSender_;

        bool deleted_;

        std::shared_ptr<Data::MessageBuddy> buddy_;

        QString aimId_;

        MessageKey key_;

        QDate date_;

        QString chatFriendly_;

        void EraseEventData();
    };

    typedef std::map<QDate, Message> DatesMap;

    class ContactDialog
    {
        std::unique_ptr<MessagesMap> messages_;
        std::unique_ptr<MessagesMap> pendingMessages_;
        std::unique_ptr<DatesMap> dateItems_;
        std::unique_ptr<MessageKey> newKey_;

        std::unique_ptr<qint64> lastRequestedMessage_;

        MessageKey lastKey_;
        MessageKey firstKey_;
        bool recvLastMessage_;

    public:

        ContactDialog() 
            : recvLastMessage_(false)
        {}

        void setLastRequestedMessage(const qint64 _message);
        qint64 getLastRequestedMessage() const;
        bool isLastRequestedMessageEmpty() const;

        MessagesMap& getMessages();
        MessagesMap& getPendingMessages();
        DatesMap& getDatesMap();

        const MessageKey& getFirstKey() const;
        const MessageKey& getLastKey() const;
        void setLastKey(const MessageKey& _key);
        void deleteLastKey();
        void setFirstKey(const MessageKey& _key);

        bool hasItemsInBetween(const MessageKey& _l, const MessageKey& _r) const;

        std::shared_ptr<Data::MessageBuddy> addDateItem(const QString& _aimId, const MessageKey& _key, const QDate& _date);
        bool hasDate(const QDate& date) const;
        void removeDateItem(const QDate& date);
        void removeDateItems();

        Logic::MessageKey findFirstKeyAfter(const Logic::MessageKey& _key) const;
        int64_t getLastMessageId() const;
        bool hasPending() const;

        void setNewKey(const MessageKey& _key);
        void resetNewKey();
        const MessageKey* getNewKey() const;

        void setRecvLastMessage(bool _value);
        bool getRecvLastMessage() const;
    };

    class MessagesModel : public QObject
    {
        Q_OBJECT

    public:

        enum UpdateMode
        {
            BASE = 0,
            NEW_PLATE,
            HOLE,
            PENDING,
            REQUESTED,
            MODIFIED
        };

    Q_SIGNALS:

        void ready(QString, bool, int64_t _mess_id);
        void updated(QList<Logic::MessageKey>, QString, unsigned);
        void deleted(QList<Logic::MessageKey>, QString);
        void readByClient(QString _aimid, qint64 _id);
        void messageIdFetched(QString, Logic::MessageKey);
        void pttPlayed(qint64);
        void canFetchMore(QString);
        void indentChanged(Logic::MessageKey, bool);
        void hasAvatarChanged(Logic::MessageKey, bool _hasAvatar);
        void chatEvent(QString aimId);
        void quote(int64_t quote_id);

    private Q_SLOTS:

        void messageBuddies(std::shared_ptr<Data::MessageBuddies> _buddies, QString _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid);

        void messagesModified(QString, std::shared_ptr<Data::MessageBuddies>);
        void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
        void fileSharingUploadingResult(QString seq, bool success, QString localPath, QString uri, int contentType, bool isFileTooBig);

    public Q_SLOTS:
        void messagesDeletedUpTo(QString, int64_t);
        void setRecvLastMsg(QString _aimId, bool _value);
        void contactChanged(QString, qint64 _messageId, qint64 last_read_msg);

        void messagesDeleted(QString, QList<int64_t>);

    public:

        explicit MessagesModel(QObject* _parent = 0);

        int getUnreadCount() const;
        void setItemWidth(int _width);

        QMap<MessageKey, Ui::HistoryControlPageItem*> tail(const QString& _aimId, QWidget* _parent, bool _is_search, int64_t _mess_id, bool _is_jump_to_bottom = false);
        QMap<MessageKey, Ui::HistoryControlPageItem*> more(const QString& _aimId, QWidget* _parent, bool _isMoveToBottomIfNeed);

        Ui::HistoryControlPageItem* getById(const QString& _aimId, const MessageKey& _key, QWidget* _parent);

        int32_t preloadCount() const;
        int32_t moreCount() const;

        void setLastKey(const MessageKey& _key, const QString& _aimId);
        void removeDialog(const QString& _aimId);

        Ui::ServiceMessageItem* createNew(const QString& _aimId, const MessageKey& _index, QWidget* _parent) const;
        void updateNew(const QString& _aimId, const qint64 _newId, const bool _hide = false);
        void hideNew(const QString& _aimId);

        QString formatRecentsText(const Data::MessageBuddy &buddy) const;
        int64_t getLastMessageId(const QString &aimId) const;
        qint64 normalizeNewMessagesId(const QString& aimid, qint64 id);

        Logic::MessageKey findFirstKeyAfter(const QString &aimId, const Logic::MessageKey &key) const;

        bool hasPending(const QString& _aimId) const;

        void eraseHistory(const QString& _aimid);

        void emitQuote(int64_t quote_id);

    private:

        Data::MessageBuddy item(const Message& _index);
        void requestMessages(const QString& _aimId, qint64 _messageId, bool _toOlder, bool _needPrefetch, bool _is_jump_to_bottom);
        int generatedDomUid();

        Ui::HistoryControlPageItem* makePageItem(const Data::MessageBuddy& _msg, QWidget* _parent) const;
        Ui::HistoryControlPageItem* fillItemById(const QString& _aimId, const MessageKey& _key, QWidget* _parent);

        void setFirstMessage(const QString& _aimId, qint64 _id);

        MessagesMapIter previousMessage(MessagesMap& _map, MessagesMapIter _iter) const;
        MessagesMapIter nextMessage(MessagesMap& _map, MessagesMapIter _iter) const;

        void processPendingMessage(InOut Data::MessageBuddies& _msgs, const QString& _aimId, const Ui::MessagesBuddiesOpt _state);

        void emitUpdated(const QList<Logic::MessageKey>& _list, const QString& _aimId, unsigned _mode);
        void emitDeleted(const QList<Logic::MessageKey>& _list, const QString& _aimId);

        // widgets factory
        void createFileSharingWidget(Ui::MessageItem& _messageItem, const Data::MessageBuddy& _messageBuddy) const;

        MessageKey applyMessageModification(const QString& _aimId, Data::MessageBuddy& _modification);

        void removeDateItemIfOutdated(const QString& _aimId, const Data::MessageBuddy& _msg);
        bool hasItemsInBetween(const QString& _aimId, const Message& _l, const Message& _r) const;
        void updateDateItems(const QString& _aimId);
        void updateMessagesMarginsAndAvatars(const QString& _aimId);

        MessagesMapIter findIndexRecord(MessagesMap& _indexRecords, const Logic::MessageKey& _key) const;

        ContactDialog& getContactDialog(const QString& _aimid);
        const ContactDialog* getContactDialogConst(const QString& _aimid) const;

        void messageBuddiesUnloadUnusedMessages(std::shared_ptr<Data::MessageBuddies> _buddies, const QString& _aimId, bool& _hole);
        void messageBuddiesInsertMessages(
            std::shared_ptr<Data::MessageBuddies> _buddies,
            const QString& _aimId,
            const qint64 _modelFirst,
            const Ui::MessagesBuddiesOpt _option,
            const qint64 _seq,
            const bool hole,
            int64_t _mess_id,
            int64_t _last_mess_id,
            model_regim _regim);

        void updateLastSeen(const QString& _aimid);

    private:

        QHash<QString, std::shared_ptr<ContactDialog>> dialogs_;

        QStringList requestedContact_;
        QStringList failedUploads_;
        QStringList subscribed_;
        QList<qint64> sequences_;

        QHash<qint64, int64_t> seqAndToOlder_;
        QHash<qint64, int64_t> seqAndJumpBottom_;
        int32_t itemWidth_;
    };

    MessagesModel* GetMessagesModel();
    void ResetMessagesModel();
}