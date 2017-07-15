#include "stdafx.h"
#include "ChatMembersModel.h"

#include "ContactListModel.h"
#include "SearchMembersModel.h"
#include "../../core_dispatcher.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

namespace Logic
{
    ChatMembersModel::ChatMembersModel(QObject * /*parent*/)
        : isShortView_(true)
        , selectEnabled_(true)
        , single_(false)
    {
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
    }

    ChatMembersModel::ChatMembersModel(std::shared_ptr<Data::ChatInfo>& _info, QObject *_parent)
        : CustomAbstractListModel(_parent)
        , isShortView_(true)
        , selectEnabled_(true)
        , single_(false)
    {
        updateInfo(_info, false);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
    }

    ChatMembersModel::~ChatMembersModel()
    {
    }

    unsigned int ChatMembersModel::getVisibleRowsCount() const
    {
        int currentCount = (int)members_.size();

        if (isShortView_)
            currentCount = std::min(Logic::InitMembersLimit, currentCount);
        return currentCount;
    }

    int ChatMembersModel::get_limit(int limit)
    {
        if (limit > InitMembersLimit)
            return limit;

        return InitMembersLimit;
    }

    int ChatMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)getVisibleRowsCount();
    }

    QVariant ChatMembersModel::data(const QModelIndex & _ind, int _role) const
    {
        int currentCount = getVisibleRowsCount();

        if (!_ind.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || _ind.row() >= currentCount)
            return QVariant();
        Data::ChatMemberInfo* ptr = &(members_[_ind.row()]);

        if (Testing::isAccessibleRole(_role))
            return ptr->AimId_;

        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags ChatMembersModel::flags(const QModelIndex &) const
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled;
        if (selectEnabled_)
            return flags |= Qt::ItemIsSelectable;
        
        return flags;
    }

    void ChatMembersModel::setSelectEnabled(bool _value)
    {
        selectEnabled_ = _value;
    }

    const Data::ChatMemberInfo* ChatMembersModel::getMemberItem(const QString& _aimId) const
    {
        for (auto& item : members_)
        {
            if (item.AimId_ == _aimId)
                return &item;
        }

        return nullptr;
    }

    int ChatMembersModel::getMembersCount() const
    {
        return membersCount_;
    }

    void ChatMembersModel::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (single_)
            return;

        if (receiveMembers(chatInfoSequence_, _seq, this))
        {
            updateInfo(_info, true);
            isFullListLoaded_ = true;
            emit dataChanged(index(0), index((int)members_.size()));
            emit results();
        }
    }

    void ChatMembersModel::avatarLoaded(QString _aimId)
    {
        int i = 0;
        for (auto iter : members_)
        {
            if (iter.AimId_ == _aimId)
            {
                emit dataChanged(index(i), index(i));
                break;
            }
            ++i;
        }
    }

    void ChatMembersModel::chatBlocked(QList<Data::ChatMemberInfo> _blocked)
    {
        members_.clear();
        for (auto iter : _blocked)
            members_.push_back(iter);

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::chatPending(QList<Data::ChatMemberInfo> _pending)
    {
        members_.clear();
        for (auto iter : _pending)
            members_.push_back(iter);

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::loadAllMembers(const QString& _aimId, int _count)
    {
        single_ = false;
        chatInfoSequence_ = loadAllMembers(_aimId, _count, this);
    }

    void ChatMembersModel::adminsOnly()
    {
        for (auto iter = members_.begin(); iter != members_.end();)
        {
            if (iter->Role_ != "admin" && iter->Role_ != "moder")
                iter = members_.erase(iter);
            else
                ++iter;
        }

        emit dataChanged(index(0), index(members_.size()));
    }

    void ChatMembersModel::loadBlocked()
    {
        members_.clear();
        emit dataChanged(index(0), index(0));
        connect(Ui::GetDispatcher(), SIGNAL(chatBlocked(QList<Data::ChatMemberInfo>)), this, SLOT(chatBlocked(QList<Data::ChatMemberInfo>)), Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        Ui::GetDispatcher()->post_message_to_core("chats/blocked/get", collection.get());
    }

    void ChatMembersModel::loadPending()
    {
        members_.clear();
        emit dataChanged(index(0), index(0));
        connect(Ui::GetDispatcher(), SIGNAL(chatPending(QList<Data::ChatMemberInfo>)), this, SLOT(chatPending(QList<Data::ChatMemberInfo>)), Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        Ui::GetDispatcher()->post_message_to_core("chats/pending/get", collection.get());
    }

    void ChatMembersModel::initForSingle(const QString& _aimId)
    {
        single_ = true;
        members_.clear();
        Data::ChatMemberInfo info;
        info.AimId_ = _aimId;
        info.NickName_ = Logic::getContactListModel()->getDisplayName(_aimId);
        members_.push_back(info);
        info.AimId_ = Ui::MyInfo()->aimId();
        info.NickName_ = Ui::MyInfo()->friendlyName();
        members_.push_back(info);
        emit dataChanged(index(0), index(1));
    }

    void ChatMembersModel::loadAllMembers()
    {
        single_ = false;
        chatInfoSequence_ = loadAllMembers(AimId_, membersCount_, this);
    }

    bool ChatMembersModel::receiveMembers(qint64 _sendSeq, qint64 _seq, QObject* _recv)
    {
        if (_seq != _sendSeq)
            return false;
        disconnect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), _recv, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)));
        disconnect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), _recv, SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)));
        return true;
    }

    qint64 ChatMembersModel::loadAllMembers(QString _aimId, int _limit, QObject* _recv)
    {
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), _recv, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), _recv,
            SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)), Qt::UniqueConnection);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_int("limit", get_limit(_limit));
        return Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
    }

    void ChatMembersModel::updateInfo(std::shared_ptr<Data::ChatInfo>& _info, bool _isAllChatInfo)
    {
        AimId_ = _info->AimId_;
        YourRole_ = _info->YourRole_;
        membersCount_ = _info->MembersCount_;
        members_.clear();
        for (auto item : _info->Members_)
        {
            members_.emplace_back(item);
        }

        emit dataChanged(index(0), index(members_.size()));

        isFullListLoaded_ = membersCount_ <= InitMembersLimit;

        if (_isAllChatInfo)
            isFullListLoaded_ = _isAllChatInfo;
    }

    static ChatMembersModel* ChatMembersModel_ = NULL;

    ChatMembersModel* getChatMembersModel()
    {
        return ChatMembersModel_;
    }

    void setChatMembersModel(ChatMembersModel* _model)
    {
        ChatMembersModel_ = _model;
        getSearchMemberModel()->setChatMembersModel(ChatMembersModel_);
    }

    bool ChatMembersModel::isContactInChat(const QString& _aimId) const
    {
        // TODO : use hash-table here
        for (auto& member : members_)
            if (member.AimId_ == _aimId)
                return true;
        return false;
    }

    QString ChatMembersModel::getChatAimId() const
    {
        return AimId_;
    }

    bool ChatMembersModel::isAdmin() const
    {
        return (YourRole_ == "admin");
    }

    bool ChatMembersModel::isModer() const
    {
        return (YourRole_ == "moder");
    }

    void ChatMembersModel::clear()
    {
        const int size = members_.size();
        members_.clear();
        emit dataChanged(index(0), index(size));
    }

    void updateIgnoredModel(const QVector<QString>& _ignoredList)
    {
        auto il = _ignoredList;

        auto info = std::make_shared<Data::ChatInfo>();
        info->Members_.reserve(il.size());
        info->MembersCount_ = il.size();
        for (auto aimId : il)
        {
            Data::ChatMemberInfo member;
            member.AimId_ = aimId;
            info->Members_.append(member);
        }

        Logic::getContactListModel()->removeContactsFromModel(_ignoredList);

        getIgnoreModel()->updateInfo(info, true);
        emit getIgnoreModel()->results();
    }

    ChatMembersModel* getIgnoreModel()
    {
        auto chatInfo = std::make_shared<Data::ChatInfo>();
        static std::unique_ptr<ChatMembersModel> ignoreModel(new ChatMembersModel(chatInfo, NULL));
        return ignoreModel.get();
    }
}