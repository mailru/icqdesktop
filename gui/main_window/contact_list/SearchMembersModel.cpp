#include "stdafx.h"
#include "SearchMembersModel.h"
#include "ChatMembersModel.h"
#include "ContactItem.h"
#include "AbstractSearchModel.h"
#include "ContactListModel.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    SearchMembersModel::SearchMembersModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , searchRequested_(false)
        , chatMembersModel_(NULL)
        , selectEnabled_(true)
    {
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
    }

    int SearchMembersModel::rowCount(const QModelIndex &) const
    {
        return (int)match_.size();
    }

    QVariant SearchMembersModel::data(const QModelIndex & _ind, int _role) const
    {
        auto currentCount = match_.size();

        if (!_ind.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || (unsigned)_ind.row() >= currentCount)
            return QVariant();

        Data::ChatMemberInfo* ptr = &(match_[_ind.row()]);
        
        if (Testing::isAccessibleRole(_role))
            return match_[_ind.row()].AimId_;
        
        return QVariant::fromValue<Data::ChatMemberInfo*>(ptr);
    }

    Qt::ItemFlags SearchMembersModel::flags(const QModelIndex &) const
    {
        Qt::ItemFlags result = Qt::ItemIsEnabled;
        if (selectEnabled_)
            result |= Qt::ItemIsEnabled;

        return result;
    }

    void SearchMembersModel::setFocus()
    {
        match_.clear();
    }

    const QStringList& SearchMembersModel::getPattern() const
    {
        return searchPatterns_;
    }

    void SearchMembersModel::searchPatternChanged(QString _p)
    {
        match_.clear();

        lastSearchPattern_ = _p;

        if (chatMembersModel_ == NULL)
            return;

        if (_p.isEmpty())
        {
            for (auto item : chatMembersModel_->members_)
            {
                match_.emplace_back(item);
            }
        }
        else
        {
            _p = _p.toLower();
            unsigned fixed_patterns_count = 0;
            auto searchPatterns = Utils::GetPossibleStrings(_p, fixed_patterns_count);
            for (auto item : chatMembersModel_->members_)
            {
                bool founded = false;
                auto displayName = Logic::getContactListModel()->getDisplayName(item.AimId_);
                unsigned i = 0;
                for (; i < fixed_patterns_count; ++i)
                {
                    QString pattern;
                    if (searchPatterns.empty())
                    {
                        pattern = _p;
                    }
                    else
                    {
                        for (auto iter = searchPatterns.begin(); iter != searchPatterns.end(); ++iter)
                            pattern += iter->at(i);
                    }

                    if (item.AimId_.contains(pattern)
                        || displayName.toLower().contains(pattern)
                        || item.NickName_.toLower().contains(pattern) 
                        || item.FirstName_.toLower().contains(pattern)
                        || item.LastName_.toLower().contains(pattern))
                    {
                        match_.emplace_back(item);
                        founded = true;
                        break;
                    }
                }
                while (!searchPatterns.empty() && searchPatterns.begin()->size() > i && !founded)
                {
                    QString pattern;
                    for (auto iter = searchPatterns.begin(); iter != searchPatterns.end(); ++iter)
                    {
                        if (iter->size() > i)
                            pattern += iter->at(i);
                    }

                    pattern = pattern.toLower();

                    if (item.AimId_.contains(pattern)
                        || displayName.toLower().contains(pattern)
                        || item.NickName_.toLower().contains(pattern) 
                        || item.FirstName_.toLower().contains(pattern)
                        || item.LastName_.toLower().contains(pattern))
                    {
                        match_.emplace_back(item);
                        break;
                    }
                    ++i;
                }
            }
        }
        unsigned size = (unsigned)match_.size();
        emit dataChanged(index(0), index(size));
    }

    void SearchMembersModel::searchResult(QStringList _result)
    {
        assert(!!"Methon is not implemented. Search in members don't use corelib.");
        return;
    }

    void SearchMembersModel::emitChanged(int _first, int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    void SearchMembersModel::setChatMembersModel(ChatMembersModel* _chatMembersModel)
    {
        chatMembersModel_ = _chatMembersModel;
    }

    void SearchMembersModel::setSelectEnabled(bool _value)
    {
        selectEnabled_ = _value;
    }

    QString SearchMembersModel::getCurrentPattern() const
    {
        return lastSearchPattern_;
    }
    
    void SearchMembersModel::avatarLoaded(QString _aimId)
    {
        int i = 0;
        for (auto iter : match_)
        {
            if (iter.AimId_ == _aimId)
            {
                emit dataChanged(index(i), index(i));
                break;
            }
            ++i;
        }
    }

    bool SearchMembersModel::isServiceItem(int i) const
    {
        return false;
    }

    SearchMembersModel* getSearchMemberModel()
    {
        static std::unique_ptr<SearchMembersModel> model(new SearchMembersModel(0));
        return model.get();
    }
}