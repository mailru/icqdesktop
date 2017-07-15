#include "stdafx.h"
#include "SearchModelDLG.h"
#include "ContactListModel.h"
#include "ContactItem.h"
#include "RecentsModel.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../../common.shared/common_defs.h"

namespace
{
	static const unsigned SORT_TIMEOUT = (build::is_debug() ? 300 : 300);
}

namespace Logic
{
	SearchModelDLG::SearchModelDLG(QObject *parent)
		: AbstractSearchModel(parent)
		, SearchRequested_(false)
        , LastRequestId_(-1)
        , ContactsCount_(0)
        , Timer_(new QTimer(this))
        , MessageResults_(false)
        , isSearchInDialog_(false)
        , EmptySearch_(false)
        , SearchInHistory_(true)
        , ContactsOnly_(false)
	{
		connect(Ui::GetDispatcher(), SIGNAL(searchedMessage(Data::DlgState)), this, SLOT(searchedMessage(Data::DlgState)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(searchedContacts(QList<Data::DlgState>, qint64)), this, SLOT(searchedContacts(QList<Data::DlgState>, qint64)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(emptySearchResults(qint64)), this, SLOT(emptySearchResults(qint64)), Qt::QueuedConnection);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, &SearchModelDLG::disableSearchInDialog, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoSearchResults()), this, SLOT(recvHideNoSearchResults()), Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), SIGNAL(repeatSearch()), this, SLOT(repeatSearch()), Qt::QueuedConnection);

        Timer_->setSingleShot(true);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(sortDialogs()), Qt::QueuedConnection);
	}

    int SearchModelDLG::count() const
    {
        auto c = (int)Match_.size() + (ContactsCount_ == 0 ? 0 : 1) + (!isSearchInDialog_ && MessageResults_ ? 1 : 0);
        if (!SearchInHistory_ && c > 0)
            --c;

        return c;
    }

	int SearchModelDLG::rowCount(const QModelIndex &) const
	{
		return count();
	}
	
	QVariant SearchModelDLG::data(const QModelIndex & ind, int role) const
	{
		if (!ind.isValid() || (role != Qt::DisplayRole && !Testing::isAccessibleRole(role)))// || (unsigned)ind.row() >= Match_.size())
			return QVariant();

        int cur = ind.row();
		if (cur >= (int)rowCount(ind))
			return QVariant();

        if (SearchInHistory_ && cur == (ContactsCount_ == 0 ? -1 : 0))
        {
            Data::DlgState st;
            st.AimId_ = "contacts";
            st.SetText(QT_TRANSLATE_NOOP("contact_list", "CONTACTS"));
            return QVariant::fromValue(st);
        }
        else if (cur == getAllMessagesIndex())
        {
            Data::DlgState st;
            st.AimId_ = "all messages";
            st.SetText(!isSearchInDialog_ ? QT_TRANSLATE_NOOP("contact_list", "ALL MESSAGES") : QT_TRANSLATE_NOOP("contact_list", "MESSAGES"));
            return QVariant::fromValue(st);
        }

		return QVariant::fromValue(Match_[correctIndex(ind.row())]);
	}

    int SearchModelDLG::correctIndex(int i) const
    {
        if (isServiceItem(i))
            return i;

        auto add = SearchInHistory_ ? (ContactsCount_ == 0 ? 0 : 1) : 0;
        auto all_message_index = getAllMessagesIndex();

        if (all_message_index != -1 && i > all_message_index)
            return i - add - 1;
        else
            return i - add;
    }

    int SearchModelDLG::get_abs_index(int _ind) const
    {
        if (!SearchInHistory_)
            return _ind;
        
        if (ContactsCount_ > _ind)
            return _ind + 1;
        
        return (_ind + 1) + (ContactsCount_ ? ContactsCount_ + 1 : 0);
    }

	Qt::ItemFlags SearchModelDLG::flags(const QModelIndex &) const
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}

	void SearchModelDLG::setFocus()
	{
		clear_match();
	}

    void SearchModelDLG::clear_match()
    {
        Match_.clear();
        TopKeys_.clear();
        ContactsCount_ = 0;
        MessageResults_ = false;
        LastRequestId_ = -1;
        //lastSearchPattern_.clear();
        emit dataChanged(index(0), index(count()));
    }

    QString SearchModelDLG::getCurrentPattern() const
    {
        return lastSearchPattern_;
    }

    int SearchModelDLG::getAllMessagesIndex() const
    {
        if (isSearchInDialog_)
            return -1;
        auto add = ContactsCount_ == 0 ? 0 : ContactsCount_ + 1;

        return MessageResults_ ? add : -1;
    }

    bool SearchModelDLG::isServiceItem(int i) const
    {
        if (!SearchInHistory_)
            return false;

        return i == (ContactsCount_ == 0 ? -1 : 0) 
            || i == getAllMessagesIndex();
    }

	void SearchModelDLG::searchPatternChanged(QString p)
	{
        auto time = 200;

        if (EmptySearch_ 
            && Match_.empty() 
            && !lastSearchPattern_.isEmpty() 
            && p.startsWith(lastSearchPattern_) 
            && p.length() >= lastSearchPattern_.length())
        {
            lastSearchPattern_ = p;
            return;
        }

        unsigned patternsCount = 0;
        lastSearchPattern_ = p;
		SearchPatterns_ = Utils::GetPossibleStrings(p, patternsCount);
		if (p.isEmpty() && SearchInHistory_)
		{
            QTimer::singleShot(time, [this, patternsCount]()
            {
                emit Utils::InterConnector::instance().hideSearchSpinner();
            });

            clear_match();

            if (isSearchInDialog_)
            {
                emit dataChanged(index(0), index(count()));
            }
            return;
        }

        clear_match();

        emit dataChanged(index(0), index(count()));

		if (!SearchRequested_)
		{
            QTimer::singleShot(time, [this, patternsCount]()
            {
                SearchRequested_ = false;
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                if (!SearchPatterns_.empty())
                {
                    core::ifptr<core::iarray> symbolsArray(collection->create_array());
                    symbolsArray->reserve(SearchPatterns_.size());
                    for (auto symbol_iter = SearchPatterns_.begin(); symbol_iter != SearchPatterns_.end(); ++symbol_iter)
                    {
                        core::coll_helper symbol_coll(collection->create_collection(), true);
                        core::ifptr<core::iarray> patternForSymbolArray(symbol_coll->create_array());
                        patternForSymbolArray->reserve(symbol_iter->size());

                        for (auto iter = symbol_iter->begin(); iter != symbol_iter->end(); ++iter)
                        {
                            core::coll_helper coll(symbol_coll->create_collection(), true);
                            coll.set_value_as_string("symbol_pattern", iter->toUtf8().data(), iter->toUtf8().size());
                            core::ifptr<core::ivalue> val(collection->create_value());
                            val->set_as_collection(coll.get());
                            patternForSymbolArray->push_back(val.get());
                        }
                        symbol_coll.set_value_as_array("symbols_patterns", patternForSymbolArray.get());
                        core::ifptr<core::ivalue> symbols_val(symbol_coll->create_value());
                        symbols_val->set_as_collection(symbol_coll.get());
                        symbolsArray->push_back(symbols_val.get());
                    }
                    collection.set_value_as_array("symbols_array", symbolsArray.get());
                    collection.set_value_as_uint("fixed_patterns_count", patternsCount);
                }
                else
                {
                    collection.set_value_as_uint("fixed_patterns_count", 1);
                }
                
                if (isSearchInDialog_)
                {
                    assert(aimid_.size() > 0);
                    collection.set_value_as_qstring("aimid", aimid_);
                }
                
                collection.set_value_as_string("init_pattern", lastSearchPattern_.toUtf8().data(), lastSearchPattern_.toUtf8().size());
                collection.set_value_as_bool("search_in_history", SearchInHistory_);
                LastRequestId_ = Ui::GetDispatcher()->post_message_to_core("history_search", collection.get());
                emit Utils::InterConnector::instance().hideNoSearchResults();
                emit Utils::InterConnector::instance().showSearchSpinner();
                emit Utils::InterConnector::instance().resetSearchResults();
            });
            SearchRequested_ = true;
		}
	}

    void SearchModelDLG::searchEnded()
    {
        LastRequestId_ = Ui::GetDispatcher()->post_message_to_core("history_search_ended", nullptr);
        lastSearchPattern_.clear();
    }

    bool SearchModelDLG::less(const ContactItem& first, const ContactItem& second)
    {
        if (first.Get() && second.Get())
            return first.Get()->GetDisplayName().compare(second.Get()->GetDisplayName(), Qt::CaseInsensitive) < 0;
        else 
            return first.get_aimid().compare(second.get_aimid()) < 0; 
    }

    void SearchModelDLG::searchedMessage(Data::DlgState dlgState)
    {
        if (LastRequestId_ != dlgState.RequestId_)
            return;

        beginInsertRows(QModelIndex(), count(), count() + 1);

        if (!Timer_->isActive())
            Timer_->start(SORT_TIMEOUT);

		{
            if (Match_.size() < ::common::get_limit_search_results())
            {
                Match_.push_back(dlgState);
                TopKeys_.insert(std::make_pair(dlgState.SearchedMsgId_, Match_.size() - 1));
            }
            else
            {
                auto greater = TopKeys_.upper_bound(dlgState.SearchedMsgId_);

                if (greater == TopKeys_.end())
                {
                    endInsertRows();
                    return;
                }

                auto index = TopKeys_.rbegin()->second;
                TopKeys_.erase(TopKeys_.rbegin()->first);
                TopKeys_.insert(std::make_pair(dlgState.SearchedMsgId_, index));
                Match_[index] = dlgState;
            }

            MessageResults_ = true;

            emit dataChanged(index(0), index(count()));
            emit results();
            endInsertRows();
		}
    }

    void SearchModelDLG::searchedContacts(QList<Data::DlgState> contacts, qint64 reqId)
    {
        if (LastRequestId_ != reqId)
            return;

        if (isSearchInDialog_)
            return;

        for (auto c : contacts)
        {
            if (ContactsOnly_ && c.Chat_)
                continue;

            Match_.push_back(c);
            ++ContactsCount_;
        }

        sortDialogs();
        emit dataChanged(index(0), index(count()));
        if (!contacts.isEmpty())
        {
            emit Utils::InterConnector::instance().hideNoSearchResults();
            emit results();
        }
    }

    void SearchModelDLG::avatarLoaded(QString aimid)
    {
        int i = 0;
        for (auto iter : Match_)
        {
            if (iter.AimId_ == aimid)
            {
                emit dataChanged(index(i), index(i));
                break;
            }
            ++i;
        }
    }

    void SearchModelDLG::contactRemoved(QString contact)
    {
        Match_.erase(std::remove_if(Match_.begin(), Match_.end(), [contact](const Data::DlgState& item) { return item.AimId_ == contact; }), Match_.end());
        --ContactsCount_;
        emitChanged(0, count());

        if (count() == 0)
            emit Utils::InterConnector::instance().showNoSearchResults();
    }

	void SearchModelDLG::emitChanged(int first, int last)
	{
		emit dataChanged(index(first), index(last));
	}

    void SearchModelDLG::sortDialogs()
	{
        blockSignals(true);
		std::sort(Match_.begin(), Match_.end(), [this](Data::DlgState first, Data::DlgState second)
        {
            if (first.IsContact_ != second.IsContact_)
            {
                return (int)first.IsContact_ > (int)second.IsContact_;
            }

            if (first.IsContact_)
            {
                auto first_name = Logic::getContactListModel()->getDisplayName(first.AimId_);
                auto second_name = Logic::getContactListModel()->getDisplayName(second.AimId_);

                auto less = [](const Data::DlgState& f, const QString& fn, const Data::DlgState& s, const QString& sn)
                {
                    if (f.FavoriteTime_ != -1 && s.FavoriteTime_ != -1)
                        return f.FavoriteTime_ < s.FavoriteTime_;

                    if (f.FavoriteTime_ != -1)
                        return true;

                    if (s.FavoriteTime_ != -1)
                        return false;
                    
                    if (f.SearchPriority_ == s.SearchPriority_)
                    {
                        if (f.Time_ != -1 && s.Time_ != -1)
                            return f.Time_ > s.Time_;

                        if (f.Time_ == -1)
                            return false;

                        if (s.Time_ == -1)
                            return true;

                        return fn.compare(sn, Qt::CaseInsensitive) < 0;
                    }

                    return f.SearchPriority_ < s.SearchPriority_;
                };

                return less(first, first_name, second, second_name);
            }

            return first.Time_ > second.Time_;

        });

        TopKeys_.clear();
        int ind = 0;
        for (auto item : Match_)
        {
            if (!item.IsContact_)
            {
                TopKeys_.emplace_hint(TopKeys_.end(), item.SearchedMsgId_, ind);
            }
            ++ind;
        }

        blockSignals(false);

		emit dataChanged(index(0), index(rowCount()));

	}

    void SearchModelDLG::emptySearchResults(qint64 _seq)
    {
        if (LastRequestId_ == _seq)
        {
            EmptySearch_ = true;
            emit Utils::InterConnector::instance().hideSearchSpinner();
            emit Utils::InterConnector::instance().showNoSearchResults();
        }
    }

    void SearchModelDLG::disableSearchInDialog()
    {
        isSearchInDialog_ = false;
        aimid_.clear();
        EmptySearch_ = false;
    }

    void SearchModelDLG::setSearchInDialog(QString _aimid)
    {
        clear_match();
        isSearchInDialog_ = true;
        aimid_ = _aimid;

        searchEnded();
        searchPatternChanged(lastSearchPattern_);
    }

    QString SearchModelDLG::getDialogAimid() const
    {
        return aimid_;
    }

    bool SearchModelDLG::isSearchInDialog() const
    {
        return isSearchInDialog_;
    }
    
    void SearchModelDLG::recvHideNoSearchResults()
    {
        EmptySearch_ = false;
    }

    void SearchModelDLG::repeatSearch()
    {
        EmptySearch_ = false;
        searchPatternChanged(lastSearchPattern_);
    }

    SearchModelDLG* getSearchModelDLG()
    {	
        static std::unique_ptr<SearchModelDLG> model(new SearchModelDLG(0));
        return model.get();
    }

    SearchModelDLG* getCustomSearchModelDLG(bool searchInHistory, bool contactsOnly)
    {	
        static std::unique_ptr<SearchModelDLG> model(new SearchModelDLG(0));
        model->setSearchInHistory(searchInHistory);
        model->setContactsOnly(contactsOnly);
        return model.get();
    }
}