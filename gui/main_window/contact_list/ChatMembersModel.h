#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "ContactItem.h"

namespace core
{
    struct icollection;
}

namespace Data
{
    class ChatMemberInfo;
}

namespace Logic
{
    const int InitMembersLimit = 20;
    const int MaxMembersLimit = 1000;

    class ChatMembersModel : public CustomAbstractListModel
    {
        Q_OBJECT
        
    Q_SIGNALS:
        void results();

    public Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);

    private Q_SLOTS:
        void avatarLoaded(QString);
        void chatBlocked(QList<Data::ChatMemberInfo>);
        void chatPending(QList<Data::ChatMemberInfo>);

    public:
        ChatMembersModel(QObject* _parent);
        ChatMembersModel(std::shared_ptr<Data::ChatInfo>& _info, QObject* _parent);

        ~ChatMembersModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const;
        QVariant data(const QModelIndex& _index, int _role) const;
        Qt::ItemFlags flags(const QModelIndex& _index) const;
        void setSelectEnabled(bool _value);

        const Data::ChatMemberInfo* getMemberItem(const QString& _aimId) const;
        int getMembersCount() const;
        void loadAllMembers();
        void loadAllMembers(const QString& _aimId, int _count);
        void initForSingle(const QString& _aimId);
        void adminsOnly();
        void loadBlocked();
        void loadPending();
        bool isFullListLoaded_;
        bool isShortView_;

        bool isContactInChat(const QString& _aimId) const;
        QString getChatAimId() const;
        void updateInfo(std::shared_ptr<Data::ChatInfo> &_info, bool _isAllChatInfo);
        static qint64  loadAllMembers(QString _aimId, int _limit, QObject* _recv);
        static bool receiveMembers(qint64 _sendSeq, qint64 _seq, QObject* _recv);
        unsigned getVisibleRowsCount() const;

        static int get_limit(int limit);

        bool isAdmin() const;
        bool isModer() const;
        void clear();

        std::vector<Data::ChatMemberInfo> getMembers() { return members_; }

    private:
        mutable std::vector<Data::ChatMemberInfo>   members_;
        int                                         membersCount_;
        std::shared_ptr<Data::ChatInfo>             info_;
        QString                                     AimId_;
        qint64                                      chatInfoSequence_;
        QString                                     YourRole_;
        bool                                        selectEnabled_;
        bool                                        single_;

        friend class SearchMembersModel;
    };

    ChatMembersModel* getChatMembersModel();
    void setChatMembersModel(ChatMembersModel*);

    void updateIgnoredModel(const QVector<QString>& _ignoredList);
    ChatMembersModel* getIgnoreModel();
}