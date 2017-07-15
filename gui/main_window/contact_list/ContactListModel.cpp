#include "stdafx.h"
#include "ContactListModel.h"

#include "contact_profile.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../MainWindow.h"
#include "../history_control/HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace
{
    struct ItemLessThan
    {
        inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
        {
            if (_first.Get()->GroupId_ == _second.Get()->GroupId_)
            {
                if (_first.is_group() && _second.is_group())
                    return false;

                if (_first.is_group())
                    return true;

                if (_second.is_group())
                    return false;

                return _first.Get()->GetDisplayName().toUpper() < _second.Get()->GetDisplayName().toUpper();
            }

            return _first.Get()->GroupId_ < _second.Get()->GroupId_;
        }
    };

    bool IsActiveContact(const Logic::ContactItem& _contact, const QDateTime& _current)
    {
        return (_contact.is_online() || _contact.recently(_current) || _contact.Get()->Is_chat_) && !_contact.is_group();
    }

    struct ItemLessThanNoGroups
    {
        ItemLessThanNoGroups(const QDateTime& _current)
            : current_(_current)
        {

        }


        inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
        {
            if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                return _first.Get()->IsChecked_;

            const auto firstIsActive = IsActiveContact(_first, current_);
            const auto secondIsActive = IsActiveContact(_second, current_);
            if (firstIsActive != secondIsActive)
                return firstIsActive;

            return _first.Get()->GetDisplayName().toUpper() < _second.Get()->GetDisplayName().toUpper();
        }

        QDateTime current_;
    };

    struct ItemLessThanSelectMembers
    {
        ItemLessThanSelectMembers(const QDateTime& _current)
            : current_(_current)
        {

        }

        inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
        {
            if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                return _first.Get()->IsChecked_;

            const auto firstIsActive = IsActiveContact(_first, current_);
            const auto secondIsActive = IsActiveContact(_second, current_);
            if (firstIsActive != secondIsActive)
                return firstIsActive;

            const auto& firstName = _first.Get()->GetDisplayName().toUpper();
            const auto& secondName = _second.Get()->GetDisplayName().toUpper();

            if (firstName[0].isLetter() != secondName[0].isLetter())
                return firstName[0].isLetter();

            std::function<bool(QChar)> is_latin = [](QChar _c){return ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z'));};

            if (is_latin(firstName[0]) != is_latin(secondName[0]))
                return !is_latin(firstName[0]);

            return firstName < secondName;
        }

        QDateTime current_;
    };

    const int REFRESH_TIMER = 5000;
}

namespace Logic
{
    std::unique_ptr<ContactListModel> g_contact_list_model;

    ContactListModel::ContactListModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , scrollPosition_(0)
        , minVisibleIndex_(0)
        , maxVisibleIndex_(0)
        , ref_(new bool(false))
        , timer_(new QTimer(this))
        , searchRequested_(false)
        , isWithCheckedBox_(false)
        , gotPageCallback_(nullptr)
        , is_index_valid_(true)
    {
        // !!!temporary, on dlg_state engine must add contact to contactlist and send changes to gui
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &ContactListModel::dlgStates, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), SIGNAL(contactList(std::shared_ptr<Data::ContactList>, QString)), this, SLOT(contactList(std::shared_ptr<Data::ContactList>, QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(presense(Data::Buddy*)), this, SLOT(presense(Data::Buddy*)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(contactRemoved(QString)), this, SLOT(contactRemoved(QString)), Qt::QueuedConnection);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::QueuedConnection);

        timer_->setSingleShot(true);
        timer_->setInterval(REFRESH_TIMER);
        connect(timer_, &QTimer::timeout, this, &Logic::ContactListModel::refresh, Qt::QueuedConnection);
    }

    int ContactListModel::rowCount(const QModelIndex &) const
    {
        int visibleCount = -1;
        getAbsIndexByVisibleIndex((int)contacts_.size(), &visibleCount, (int)contacts_.size());
        return visibleCount + 1;
    }

    int ContactListModel::getAbsIndexByVisibleIndex(int _cur, int* _visibleCount, int _iterLimit) const
    {
        bool groups_enabled = Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false);
        int iter = 0;
        *_visibleCount = -1;
        for (;*_visibleCount < _cur && (unsigned)iter < contacts_.size() && iter < _iterLimit; ++iter)
        {
            if (contacts_[getIndexByOrderedIndex(iter)].is_visible() && (groups_enabled || !contacts_[getIndexByOrderedIndex(iter)].is_group()))
            {
                ++*_visibleCount;
            }
        }
        return iter;
    }

    QModelIndex ContactListModel::contactIndex(const QString& _aimId)
    {
        int visibleCount = 0;
        auto contact_index = getOrderIndexByAimid(_aimId);
        getAbsIndexByVisibleIndex(contact_index, &visibleCount, contact_index);

        return index(visibleCount + 1);
    }

    QVariant ContactListModel::data(const QModelIndex& _i, int _r) const
    {
        int cur = _i.row();

        if (!_i.isValid() || (unsigned)cur >= contacts_.size())
        {
            return QVariant();
        }

        if (_r == Qt::DisplayRole)
        {
            if (maxVisibleIndex_ == 0 && minVisibleIndex_ == 0)
                maxVisibleIndex_ = minVisibleIndex_ = cur;

            if (cur < minVisibleIndex_)
                minVisibleIndex_ = cur;
            if (cur > maxVisibleIndex_)
                maxVisibleIndex_ = cur;
        }

        int visibleCount = -1;
        auto iter = getAbsIndexByVisibleIndex(cur, &visibleCount, (int)contacts_.size());
        auto cont = contacts_[getIndexByOrderedIndex(iter - 1)].Get();

        if (Testing::isAccessibleRole(_r))
            return cont->AimId_;

        return QVariant::fromValue(cont);
    }

    QString ContactListModel::contactToTryOnTheme() const
    {
        if (!currentAimdId_.isEmpty())
            return currentAimdId_;

        QString recentsFirstContact = Logic::getRecentsModel()->firstContact();
        if (!recentsFirstContact.isEmpty())
            return recentsFirstContact;

        if (!contacts_.empty())
            return contacts_[getIndexByOrderedIndex(0)].get_aimid();

        return QString();
    }

    Qt::ItemFlags ContactListModel::flags(const QModelIndex& _i) const
    {
        if (!_i.isValid())
            return Qt::ItemIsEnabled;

        unsigned flags = QAbstractItemModel::flags(_i) | Qt::ItemIsEnabled;

        int visibleCount = -1;
        auto iter = getAbsIndexByVisibleIndex(_i.row(), &visibleCount, (int)contacts_.size());
        auto cont = contacts_[getIndexByOrderedIndex(iter - 1)].Get();

        if (_i.row() == 0 || cont->GetType() == Data::GROUP)
            flags |= ~Qt::ItemIsSelectable;
        flags |= ~Qt::ItemIsEditable;

        return (Qt::ItemFlags)flags;
    }

    bool ContactListModel::isVisibleItem(const ContactItem& _item)
    {
        if (!isWithCheckedBox_)
        {
            return true;
        }

        if (_item.is_chat())
        {
            return false;
        }

        return (Ui::MyInfo()->aimId() != _item.get_aimid());
    }

    void ContactListModel::setFocus()
    {
    }

    int ContactListModel::addItem(Data::Contact* _contact)
    {
        auto item = getContactItem(_contact->AimId_);
        if (item != nullptr)
        {
            item->Get()->ApplyBuddy(_contact);
            setContactVisible(_contact->AimId_, true);
            Logic::GetAvatarStorage()->UpdateDefaultAvatarIfNeed(_contact->AimId_);
        }
        else
        {
            contacts_.emplace_back(_contact);
            sorted_index_cl_.emplace_back((int)contacts_.size() - 1);
            sorted_index_recents_.emplace_back((int)contacts_.size() - 1);
            indexes_.insert(_contact->AimId_, (int)contacts_.size() - 1);
            emit dataChanged(index((int)contacts_.size() - 1), index((int)contacts_.size() - 1));
        }

        updatePlaceholders();
        return (int)contacts_.size();
    }

    void ContactListModel::rebuild_index()
    {
        indexes_.clear();
        unsigned i = 0;

        for (const auto &order_index : sorted_index_cl_)
        {
            indexes_.insert(contacts_[order_index].Get()->AimId_, i);
            ++i;
        }
        is_index_valid_ = true;
    }

    void ContactListModel::contactList(std::shared_ptr<Data::ContactList> _cl, QString _type)
    {
        const int size = (int)contacts_.size();
        beginInsertRows(QModelIndex(), size, _cl->size() + size);

        for (auto iter : _cl->uniqueKeys())
        {
            if (_type != "deleted")
            {
                if (iter->UserType_ != "sms")
                {
                    addItem(iter);
                    if (iter->IsLiveChat_)
                        emit liveChatJoined(iter->AimId_);
                    emit contactChanged(iter->AimId_);
                }
            }
            else
            {
                innerRemoveContact(iter->AimId_);
            }
        }

        std::list<int> groupIds;
        for (auto iter : *_cl)
        {
            if (std::find(groupIds.begin(), groupIds.end(), iter->Id_) != groupIds.end())
                continue;

            if (_type != "deleted")
            {
                groupIds.push_back(iter->Id_);
                Data::Group* group = new Data::Group();
                group->ApplyBuddy(iter);
                addItem(group);
            }
            else if (iter->Removed_)
            {
                int id = iter->Id_;
                contacts_.erase(std::remove_if(contacts_.begin(), contacts_.end(), [id](const ContactItem& item) { return item.Get()->GroupId_ == id; }), contacts_.end());
                is_index_valid_ = false;
            }
        }

        sort();
        endInsertRows();

        rebuild_index();
        updatePlaceholders();
    }

    void ContactListModel::updatePlaceholders()
    {
        int visibleCount = -1;
        getAbsIndexByVisibleIndex((int)contacts_.size(), &visibleCount, (int)contacts_.size());
        if ((visibleCount + 1) == 0)
        {
            emit Utils::InterConnector::instance().showNoContactsYet();
            emit Utils::InterConnector::instance().showNoRecentsYet();
        }
        else if (visibleCount == 0)
            emit Utils::InterConnector::instance().hideNoContactsYet();
    }

    void ContactListModel::dlgStates(std::shared_ptr<QList<Data::DlgState>> _states)
    {
        for (auto _dlgState : *_states)
        {
            if (!getContactItem(_dlgState.AimId_))
            {
                std::unique_ptr<Data::Contact> addContact(new Data::Contact());
                addContact->AimId_ = _dlgState.AimId_;
                addContact->State_ = "";
                addContact->NotAuth_ = true;
                addContact->Friendly_ = _dlgState.Friendly_;
                addContact->Is_chat_ = _dlgState.Chat_;
                addItem(addContact.release());
            }
        }
    }

    int ContactListModel::innerRemoveContact(const QString& _aimId)
    {
        size_t idx = 0;
        for ( ; idx < contacts_.size(); ++idx)
        {
            if (contacts_[idx].Get()->AimId_ == _aimId)
                break;
        }

        auto cont = contacts_.begin() + idx;
        if (cont == contacts_.end())
            return contacts_.size();

        if (cont->is_live_chat())
            emit liveChatRemoved(_aimId);

        contacts_.erase(cont);
        is_index_valid_ = false;
        emit contact_removed(_aimId);
        return idx;
    }

    void ContactListModel::updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index)
    {
        _list.erase(std::remove(_list.begin(), _list.end(), _index), _list.end());
        for (auto& idx : _list)
        {
            if (idx > _index)
            {
                idx -= 1;
            }
        }
    }

    void ContactListModel::contactRemoved(QString _contact)
    {
        auto removedIdx = innerRemoveContact(_contact);
        updateIndexesListAfterRemoveContact(sorted_index_cl_, removedIdx);
        updateIndexesListAfterRemoveContact(sorted_index_recents_, removedIdx);
        rebuild_index();
        updatePlaceholders();
    }

    void ContactListModel::refresh()
    {
        sort();
        rebuild_index();
        emit dataChanged(index(minVisibleIndex_), index(maxVisibleIndex_));
    }

    void ContactListModel::avatarLoaded(QString _aimId)
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx != -1)
        {
            emit dataChanged(index(idx), index(idx));
        }
    }

    void ContactListModel::presense(Data::Buddy* _presence)
    {
        auto idx = getOrderIndexByAimid(_presence->AimId_);
        if (idx != -1)
        {
            auto contact = contacts_[getIndexByOrderedIndex(idx)].Get();
            contact->ApplyBuddy(_presence);
            pushChange(idx);
            emit contactChanged(_presence->AimId_);
            if (!timer_->isActive())
                timer_->start();
        }
    }

    void ContactListModel::groupClicked(int _groupId)
    {
        for (size_t contactIndex = 0; contactIndex < contacts_.size(); ++contactIndex)
        {
            auto &contact = contacts_[getIndexByOrderedIndex(contactIndex)];
            if (contact.Get()->GroupId_ == _groupId && !contact.is_group())
            {
                contact.set_visible(!contact.is_visible());
                emit dataChanged(index(contactIndex), index(contactIndex));
            }
        }
    }

    void ContactListModel::scrolled(int _value)
    {
        minVisibleIndex_ = 0;
        maxVisibleIndex_ = 0;
        scrollPosition_ = _value;
    }

    void ContactListModel::pushChange(int _i)
    {
        updatedItems_.push_back(_i);
        int scrollPos = scrollPosition_;
        QTimer::singleShot(2000, [this, scrollPos](){ if (scrollPosition_ == scrollPos) processChanges(); });
    }

    void ContactListModel::processChanges()
    {
        if (updatedItems_.empty())
            return;

        for (auto iter : updatedItems_)
        {
            if (iter >= minVisibleIndex_ && iter <= maxVisibleIndex_)
                emit dataChanged(index(iter), index(iter));
        }

        updatedItems_.clear();
    }

    void ContactListModel::setCurrent(QString _aimId, qint64 id, bool _sel, bool _needSwitchTab, std::function<void(Ui::HistoryControlPage*)> _gotPageCallback, qint64 quote_id)
    {
        if (!currentAimdId_.isEmpty())
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", currentAimdId_);
            collection.set_value_as_int("status", (int32_t)core::typing_status::typed);
            Ui::GetDispatcher()->post_message_to_core("message/typing", collection.get());
        }

        if (_gotPageCallback)
        {
            auto page = Utils::InterConnector::instance().getMainWindow()->getHistoryPage(_aimId);
            if (page)
            {
                _gotPageCallback(page);
            }
            else
            {
                gotPageCallback_ = _gotPageCallback;
            }
        }

        if (platform::is_apple() && _aimId.isEmpty() && !currentAimdId_.isEmpty())
            emit leave_dialog(currentAimdId_);

        currentAimdId_ = _aimId;
        if (_sel && !currentAimdId_.isEmpty())
            emit select(currentAimdId_, id, quote_id);
       // else
         //   emit selectedContactChanged(currentAimdId_);

        if (_needSwitchTab)
            emit switchTab(_aimId);

        auto recentState = Logic::getRecentsModel()->getDlgState(_aimId, false);
        auto unknownState = Logic::getUnknownsModel()->getDlgState(_aimId, false);
        if (recentState.AimId_ != _aimId && unknownState.AimId_ == _aimId)
            emit Utils::InterConnector::instance().unknownsGoSeeThem();
        else if (!_aimId.isEmpty())
            emit Utils::InterConnector::instance().unknownsGoBack();
    }

    void ContactListModel::setCurrentCallbackHappened(Ui::HistoryControlPage* _page)
    {
        if (gotPageCallback_)
        {
            gotPageCallback_(_page);
            gotPageCallback_ = nullptr;
        }
    }

    void ContactListModel::sort()
    {
        if (!contacts_.size())
            return;
        updateSortedIndexesList(sorted_index_cl_, getLessFuncCL(QDateTime::currentDateTime()));
    }

    std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> ContactListModel::getLessFuncCL(const QDateTime& current) const
    {
        std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> less = ItemLessThanNoGroups(current);

        // if (GetIsWithCheckedBox())
        // 	less = ItemLessThanSelectMembers();
        // else
        if (Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false))
            less = ItemLessThan();

        return less;
    }

    void ContactListModel::updateSortedIndexesList(std::vector<int>& _list, std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> _less)
    {
        _list.resize(contacts_.size());
        std::iota(_list.begin(), _list.end(), 0);

        std::sort(_list.begin(), _list.end(), [_less, this] (int a, int b)
            {
                return _less(contacts_[a], contacts_[b]);
            });
    }

    bool ContactListModel::contains(const QString& _aimdId) const
    {
        return indexes_.find(_aimdId) != indexes_.end();
    }

    QString ContactListModel::selectedContact() const
    {
        return currentAimdId_;
    }

    QString ContactListModel::selectedContactName() const
    {
        auto contact = getContactItem(currentAimdId_);
        return contact == nullptr ? QString() : contact->Get()->GetDisplayName();
    }

    void ContactListModel::add(const QString& _aimId, const QString& _friendly)
    {
        std::unique_ptr<Data::Contact> addContact(new Data::Contact());
        addContact->AimId_ = _aimId;
        addContact->State_ = "";
        addContact->NotAuth_ = true;
        addContact->Friendly_ = _friendly;
        addContact->Is_chat_ = _aimId.indexOf("@chat.agent") != -1;
        addItem(addContact.release());
    }

    void ContactListModel::setContactVisible(const QString& _aimId, bool visible)
    {
        auto item = getContactItem(_aimId);
        if (item)
            item->set_visible(visible);

        emit dataChanged(index(0), index(indexes_.size()));
    }

    ContactItem* ContactListModel::getContactItem(const QString& _aimId)
    {
        return const_cast<ContactItem*>(static_cast<const ContactListModel*>(this)->getContactItem(_aimId));
    }

    const ContactItem* ContactListModel::getContactItem(const QString& _aimId) const
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx == -1)
            return nullptr;

        if (idx >= (int) contacts_.size())
            return nullptr;

        return &contacts_[getIndexByOrderedIndex(idx)];
	}

	QString ContactListModel::getDisplayName(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return _aimId;
		}

		return ci->Get()->GetDisplayName();
	}

	bool ContactListModel::isChat(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return false;
		}

		return ci->is_chat();
	}

	bool ContactListModel::isMuted(const QString& _aimId) const
	{
		auto ci = getContactItem(_aimId);
		if (!ci)
		{
			return false;
		}

		return ci->is_muted();
	}

    bool ContactListModel::isLiveChat(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_live_chat();
    }

    bool ContactListModel::isOfficial(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_official();
    }

    bool ContactListModel::isNotAuth(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return false;
        }

        return ci->is_not_auth();
    }

    QString ContactListModel::getState(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return "";
        }
		return ci->Get()->GetState();
	}

    QString ContactListModel::getStatusString(const QString& _aimId) const
    {
        QString statusString;
        auto contact = getContactItem(_aimId);
        if (!contact) return "";
        if (isOfficial(_aimId))
        {
            statusString = QT_TRANSLATE_NOOP("chat_page", "Official account");
        }
        else if (isNotAuth(_aimId))
        {
            statusString = QT_TRANSLATE_NOOP("chat_page", "Not authorized");
        }
        else
        {
            QString state;
            QDateTime lastSeen = contact->Get()->LastSeen_;
            if (lastSeen.isValid())
            {
                state = getLastSeenString(_aimId);
            }
            else
            {
                state = contact->is_phone() ? contact->Get()->AimId_ :
                    (contact->Get()->StatusMsg_.isEmpty() ? contact->Get()->State_ : contact->Get()->StatusMsg_);
            }
            statusString = state;
        }
        return statusString;
    }

    QDateTime ContactListModel::getLastSeen(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return QDateTime::currentDateTime();
        }

        return ci->Get()->GetLastSeen();
    }

    QString ContactListModel::getLastSeenString(const QString& _aimId) const
    {
        QString slastSeen = QT_TRANSLATE_NOOP("contact_list", "Seen ");
        QString slastSeenSuffix = "";
        const auto lastSeen = getLastSeen(_aimId);

        if (!lastSeen.isValid()) return "";

        const auto current = QDateTime::currentDateTime();
        const auto days = lastSeen.daysTo(current);
        if (days == 0)
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "today");
        else if (days == 1)
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", "yesterday");
        else
            slastSeenSuffix += Utils::GetTranslator()->formatDate(lastSeen.date(), lastSeen.date().year() == current.date().year());
        if (lastSeen.date().year() == current.date().year())
        {
            slastSeenSuffix += QT_TRANSLATE_NOOP("contact_list", " at ");
            slastSeenSuffix += lastSeen.time().toString(Qt::SystemLocaleShortDate);
        }
        if (!slastSeenSuffix.length())
            slastSeen = "";
        else
            slastSeen += slastSeenSuffix;
        return slastSeen;
    }

    ContactListModel* getContactListModel()
    {
        if (!g_contact_list_model)
            g_contact_list_model.reset(new ContactListModel(0));

        return g_contact_list_model.get();
    }

    void ResetContactListModel()
    {
        if (g_contact_list_model)
            g_contact_list_model.reset();
    }

    QString ContactListModel::getInputText(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return "";
        }

        return ci->get_input_text();
    }

    void ContactListModel::setInputText(const QString& _aimId, const QString& _text)
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
        {
            return;
        }

        ci->set_input_text(_text);
    }

    void ContactListModel::getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t error)> _callBack)
    {
        profile_ptr profile;

        if (!_aimId.isEmpty())
        {
            auto ci = getContactItem(_aimId);

            if (ci)
            {
                profile = ci->getContactProfile();
            }
        }

        if (profile)
        {
            emit profile_loaded(profile);

            _callBack(profile, 0);
        }

        if (!profile)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

            collection.set_value_as_qstring("aimid", _aimId);

            Ui::GetDispatcher()->post_message_to_core("contacts/profile/get", collection.get(), this, [this, _callBack](core::icollection* _coll)
            {
                Ui::gui_coll_helper coll(_coll, false);

                QString aimid = coll.get_value_as_string("aimid");
                int32_t err = coll.get_value_as_int("error");

                if (err == 0)
                {
                    Ui::gui_coll_helper coll_profile(coll.get_value_as_collection("profile"), false);

                    profile_ptr profile(new contact_profile());

                    if (profile->unserialize(coll_profile))
                    {
                        if (!aimid.isEmpty())
                        {
                            ContactItem* ci = getContactItem(aimid);
                            if (ci)
                                ci->set_contact_profile(profile);
                        }

                        emit profile_loaded(profile);

                        _callBack(profile, 0);
                    }
                }
                else
                {
                    _callBack(nullptr, 0);
                }
            });
        }
    }

    void ContactListModel::addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("group", "General");
        collection.set_value_as_qstring("message", QT_TRANSLATE_NOOP("contact_list","Hello. Please add me to your contact list"));

        Ui::GetDispatcher()->post_message_to_core("contacts/add", collection.get(), this, [this, _aimId, _callBack](core::icollection* _coll)
        {
            Logic::ContactItem* item = getContactItem(_aimId);
            if (item && item->is_not_auth())
            {
                item->reset_not_auth();
                emit contactChanged(_aimId);
            }

            Ui::gui_coll_helper coll(_coll, false);

            QString contact = coll.get_value_as_string("contact");
            int32_t err = coll.get_value_as_int("error");

            _callBack(err == 0);
            emit contact_added(contact, (err == 0));
            emit needSwitchToRecents();
        });
    }

    void ContactListModel::renameContact(const QString& _aimId, const QString& _friendly)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("friendly", _friendly);

        Ui::GetDispatcher()->post_message_to_core("contacts/rename", collection.get());
    }

    void ContactListModel::renameChat(const QString& _aimId, const QString& _friendly)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_qstring("m_chat_name", _friendly);
        Ui::GetDispatcher()->post_message_to_core("modify_chat", collection.get());
    }

    void ContactListModel::removeContactFromCL(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("contacts/remove", collection.get());
    }

    void ContactListModel::removeContactsFromModel(const QVector<QString>& _vcontacts)
    {
        for (const auto _aimid : _vcontacts)
        {
            if (getContactItem(_aimid))
            {
                contactRemoved(_aimid);
            }
        }
    }

    bool ContactListModel::blockAndSpamContact(const QString& _aimId, bool _withConfirmation /*= true*/)
    {
        bool confirm = false;
        if (_withConfirmation)
        {
            confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure this contact is spam?"),
                getDisplayName(_aimId),
                NULL);
        }

        if (confirm || !_withConfirmation)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", _aimId);
            Ui::GetDispatcher()->post_message_to_core("contacts/block", collection.get());
        }
        return confirm;
    }

    void ContactListModel::ignoreContact(const QString& _aimId, bool _ignore)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("ignore", _ignore);
        Ui::GetDispatcher()->post_message_to_core("contacts/ignore", collection.get());

		if (_ignore)
		{
			emit ignore_contact(_aimId);
            Ui::GetDispatcher()->getVoipController().setDecline(_aimId.toStdString().c_str(), true);
		}
    }

    bool ContactListModel::ignoreContactWithConfirm(const QString& _aimId)
    {
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to move contact to ignore list?"),
            getDisplayName(_aimId),
            NULL);

        if (confirm)
        {
            ignoreContact(_aimId, true);
        }
        return confirm;
    }

    bool ContactListModel::isYouAdmin(const QString& _aimId)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            const auto role = cont->get_chat_role();
            return role == "admin" || role == "moder";
        }

        return false;
    }

    QString ContactListModel::getYourRole(const QString& _aimId)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            return cont->get_chat_role();
        }

        return QString();
    }

    void ContactListModel::setYourRole(const QString& _aimId, const QString& _role)
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            emit youRoleChanged(_aimId);
            return cont->set_chat_role(_role);
        }
    }

    void ContactListModel::getIgnoreList()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        Ui::GetDispatcher()->post_message_to_core("contacts/get_ignore", collection.get());
    }

    void ContactListModel::emitChanged(int _first, int _last)
    {
        emit dataChanged(index(_first), index(_last));
    }

    std::vector<ContactItem> ContactListModel::GetCheckedContacts() const
    {
        std::vector<ContactItem> result;
        for (const auto& item : contacts_)
        {
            if (item.is_checked())
            {
                result.push_back(item);
            }
        }
        return result;
    }

    void ContactListModel::clearChecked()
    {
        for (auto item : contacts_)
        {
            item.set_checked(false);
        }
    }

    void ContactListModel::setChecked(const QString& _aimid, bool _isChecked)
    {
        getContactItem(_aimid)->set_checked(_isChecked);
    }

    bool ContactListModel::getIsChecked(const QString& _aimId) const
    {
        return getContactItem(_aimId)->is_checked();
    }

    bool ContactListModel::isWithCheckedBox()
    {
        return isWithCheckedBox_;
    }

    void ContactListModel::setIsWithCheckedBox(bool _isWithCheckedBox)
    {
        isWithCheckedBox_ = _isWithCheckedBox;
    }

    void ContactListModel::chatInfo(qint64, std::shared_ptr<Data::ChatInfo> info)
    {
        auto idx = getOrderIndexByAimid(info->AimId_);
        if (idx == -1)
            return;

        if (idx >= (int) contacts_.size())
            return;

        QString role = info->YourRole_;
        if (info->YouPending_)
            role = "pending";
        else if (!info->YouMember_ || role.isEmpty())
            role = "notamember";

        if (contacts_[getIndexByOrderedIndex(idx)].get_chat_role() != role)
            emit youRoleChanged(info->AimId_);

        contacts_[getIndexByOrderedIndex(idx)].set_chat_role(role);
    }


    void ContactListModel::joinLiveChat(const QString& _stamp, bool _silent)
    {
        if (_silent)
        {
            getContactProfile(Ui::MyInfo()->aimId(), [_stamp](profile_ptr profile, int32_t)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("stamp", _stamp);
                if (profile)
                {
                    int age = Utils::calcAge(QDateTime::fromMSecsSinceEpoch((qint64)profile->get_birthdate() * 1000, Qt::LocalTime));
                    collection.set_value_as_int("age", age);
                }
                Ui::GetDispatcher()->post_message_to_core("livechat/join", collection.get());
            });
        }
        else
        {
            emit needJoinLiveChat(_stamp);
        }
    }

    void ContactListModel::next()
    {
        int current = 0;
        if (!currentAimdId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimdId_);
            if (idx != -1)
                current = idx;
        }

        ++current;

        for (auto iter = indexes_.begin(); iter != indexes_.end(); ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true, false);
                break;
            }
        }
    }

    void ContactListModel::prev()
    {
        int current = 0;
        if (!currentAimdId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimdId_);
            if (idx != -1)
                current = idx;
        }

        --current;

        for (auto iter = indexes_.begin(); iter != indexes_.end(); ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true, false);
                break;
            }
        }
    }

    int ContactListModel::getIndexByOrderedIndex(int _index) const
    {
        return sorted_index_cl_[_index];
    }

    int ContactListModel::getOrderIndexByAimid(const QString& _aimId) const
    {
        assert(is_index_valid_);

        auto item = indexes_.find(_aimId);
        return item != indexes_.end() ? item.value() : -1;
    }
}
