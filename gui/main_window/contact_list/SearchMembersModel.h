#pragma once

#include "ContactItem.h"
#include "ContactList.h"
#include "ChatMembersModel.h"
#include "AbstractSearchModel.h"
#include "../../types/contact.h"

namespace Logic
{
    class SearchMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void results();

    public Q_SLOTS:
        void searchPatternChanged(QString) override;
        void searchResult(QStringList);
        
    private Q_SLOTS:
        void avatarLoaded(QString _aimId);

    public:
        SearchMembersModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        Qt::ItemFlags flags(const QModelIndex& _index) const override;
        void setSelectEnabled(bool _value) override;
        void setFocus() override;
        const QStringList& getPattern() const;
        void emitChanged(int _first, int _last) override;
        void setChatMembersModel(ChatMembersModel* _membersModel);
        virtual bool isServiceItem(int i) const override;

        virtual QString getCurrentPattern() const;
        
    private:
        mutable std::vector<Data::ChatMemberInfo> match_;
        QStringList searchPatterns_;
        QString lastSearchPattern_;
        bool searchRequested_;
        bool selectEnabled_;
        ChatMembersModel* chatMembersModel_;
    };
    
    SearchMembersModel* getSearchMemberModel();
}
