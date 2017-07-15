#include "stdafx.h"
#include "UnknownsModel.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "../MainPage.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../gui_settings.h"

namespace
{
    static const unsigned SORT_TIMEOUT = (build::is_debug() ? 120000 : 1000);
}

namespace Logic
{
    std::unique_ptr<UnknownsModel> g_unknownsModel;

    UnknownsModel::UnknownsModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , timer_(new QTimer(this))
        , isDeleteAllVisible_(true)
    {
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(contactChanged(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(activeDialogHide(QString)), this, SLOT(activeDialogHide(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &UnknownsModel::dlgStates, Qt::QueuedConnection);
        timer_->setSingleShot(true);
        connect(timer_, SIGNAL(timeout()), this, SLOT(sortDialogs()), Qt::QueuedConnection);

        auto remover = [=](QString _aimId, bool _passToRecents)
        {
            const auto wasNotEmpty = !dialogs_.empty();
            auto iter = std::find_if(
                dialogs_.begin(), dialogs_.end(), [_aimId](const Data::DlgState& _item)
            {
                return (_item.AimId_ == _aimId);
            });
            if (iter != dialogs_.end())
            {
//                Logic::getContactListModel()->setContactVisible(_aimId, true);
                if (_passToRecents)
                {
                    //Logic::getContactListModel()->setContactVisible(_aimId, true);
                    const bool setAsCurrent = (Logic::getContactListModel()->selectedContact() == _aimId);
                    Logic::getRecentsModel()->unknownToRecents(*iter);
                    if (setAsCurrent)
                    {
                        Logic::getContactListModel()->setCurrent(_aimId, -1, true);
                    }
                }
                else
                {
                    Logic::getContactListModel()->setCurrent("", -1, true);
                }

                dialogs_.erase(iter);
                sortDialogs();
            }
            if (wasNotEmpty && dialogs_.empty())
            {
                emit updated();
                emit Utils::InterConnector::instance().unknownsGoBack();
            }
        };
        
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_added, this, [=](QString _aimId, bool /*succeeded*/)
        {
            remover(_aimId, true);
        });
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, [=](QString _aimId)
        {
            remover(_aimId, false);
        });
        connect(Logic::getContactListModel(), &Logic::ContactListModel::ignore_contact, this, [=](QString _aimId)
        {
            remover(_aimId, false);
        });
        
        connect(Logic::getContactListModel(), &ContactListModel::contactChanged, this, [=](QString _aimId)
        {
            auto contact = Logic::getContactListModel()->getContactItem(_aimId);
            if (contact && !contact->is_not_auth())
            {
                remover(_aimId, true);
            }
            contactChanged(_aimId);
        });
        
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsDeleteThemAll, this, [=]()
        {
            for (auto it: dialogs_)
            {
                Logic::getContactListModel()->removeContactFromCL(it.AimId_);
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", it.AimId_);
                Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());
            }
            dialogs_.clear();
            emit updated();
            emit Utils::InterConnector::instance().unknownsGoBack();
        });
    }

    UnknownsModel::~UnknownsModel()
    {
        //
    }
    
    int UnknownsModel::rowCount(const QModelIndex &) const
    {
        return (int)dialogs_.size() + (isDeleteAllVisible_ ? 1 : 0);
    }
    
    int UnknownsModel::itemsCount() const
    {
        return (int)dialogs_.size();
    }

    void UnknownsModel::add(const QString& _aimId)
    {
        Data::DlgState st;
        st.AimId_ = _aimId;
        dialogs_.push_back(st);

        Logic::getContactListModel()->setContactVisible(_aimId, false);

        if (!timer_->isActive())
            timer_->start(SORT_TIMEOUT);
    }

    QVariant UnknownsModel::data(const QModelIndex& _i, int _r) const
    {
        if (!_i.isValid() || (_r != Qt::DisplayRole && !Testing::isAccessibleRole(_r)))
            return QVariant();

        int cur = _i.row();
        if (cur >= (int) rowCount(_i))
            return QVariant();

        if (!cur && isDeleteAllVisible_)
        {
            Data::DlgState st;
            st.AimId_ = "delete_all";
            return QVariant::fromValue(st);
        }
        
        cur = correctIndex(cur);
        
        Data::DlgState cont = dialogs_[cur];

        if (Testing::isAccessibleRole(_r))
            return cont.AimId_;

        return QVariant::fromValue(cont);
    }

    Qt::ItemFlags UnknownsModel::flags(const QModelIndex& _i) const
    {
        if (!_i.isValid())
            return Qt::ItemIsEnabled;
        return QAbstractItemModel::flags(_i) | Qt::ItemIsEnabled;
    }

    void UnknownsModel::contactChanged(QString _aimId)
    {
        int i = indexes_[_aimId];
        if (i != -1)
        {
            emit dataChanged(index(i), index(i));
        }
    }

    void UnknownsModel::activeDialogHide(QString _aimId)
    {
        const auto wasNotEmpty = !dialogs_.empty();
        Data::DlgState state;
        state.AimId_ = _aimId;
        std::vector<Data::DlgState>::iterator iter = std::find(dialogs_.begin(), dialogs_.end(), state);
        if (iter != dialogs_.end())
        {
            QString hideContact = iter->AimId_;
            contactChanged(hideContact);
            indexes_[iter->AimId_] = -1;
            dialogs_.erase(iter);
            Logic::getContactListModel()->setCurrent("", -1, true);
            emit updated();
        }
        if (wasNotEmpty && dialogs_.empty())
        {
            emit updated();
            emit Utils::InterConnector::instance().unknownsGoBack();
            if (!Logic::getRecentsModel()->rowCount())
                emit Utils::InterConnector::instance().showNoRecentsYet();
        }
    }

    void UnknownsModel::dlgStates(std::shared_ptr<QList<Data::DlgState>> _states)
    {
        bool syncSort = false;

        for (auto _dlgState : *_states)
        {
            auto isUnknown = Logic::getContactListModel()->isNotAuth(_dlgState.AimId_);
            if (_dlgState.Chat_ || _dlgState.Official_ || !isUnknown)
                continue;

            auto iter = std::find(dialogs_.begin(), dialogs_.end(), _dlgState);
            if (iter != dialogs_.end())
            {
                auto &existingDlgState = *iter;

                if (existingDlgState.YoursLastRead_ != _dlgState.YoursLastRead_)
                    emit readStateChanged(_dlgState.AimId_);

                const auto existingText = existingDlgState.GetText();

                existingDlgState = _dlgState;

                const auto mustRecoverText = (!existingDlgState.HasText() && existingDlgState.HasLastMsgId());
                if (mustRecoverText)
                {
                    existingDlgState.SetText(existingText);
                }

                if (!syncSort)
                {
                    if (!timer_->isActive())
                        timer_->start(SORT_TIMEOUT);

                    int dist = (int)std::distance(dialogs_.begin(), iter);
                    emit dataChanged(index(dist), index(dist));
                }
            }
            else if (!_dlgState.GetText().isEmpty())
            {
                dialogs_.push_back(_dlgState);

                Logic::getContactListModel()->setContactVisible(_dlgState.AimId_, false);

                if (!timer_->isActive())
                    timer_->start(SORT_TIMEOUT);

                if (dialogs_.size() == 1)
                {
                    emit updated();
                    emit Utils::InterConnector::instance().hideNoRecentsYet();
                }
            }

            Ui::MainWindow* w = Utils::InterConnector::instance().getMainWindow();
            if (_dlgState.AimId_ == Logic::getContactListModel()->selectedContact() && w && w->isActive() && w->isMainPage())
            {
                sendLastRead(_dlgState.AimId_);
            }
        }

        if (syncSort)
        {
            sortDialogs();
        }

        emit updated();
        emit dlgStatesHandled(_states);
    }

    void UnknownsModel::sortDialogs()
    {
        std::sort(dialogs_.begin(), dialogs_.end(), [this](Data::DlgState first, Data::DlgState second)
        {
            if (first.FavoriteTime_ == -1 && second.FavoriteTime_ == -1)
                return first.Time_ > second.Time_;

            if (first.FavoriteTime_ == -1)
                return false;
            else if (second.FavoriteTime_ == -1)
                return true;

            if (first.FavoriteTime_ == second.FavoriteTime_)
                return first.AimId_ > second.AimId_;

            return first.FavoriteTime_ < second.FavoriteTime_;
        });
        indexes_.clear();
        int i = 0;
        for (auto iter : dialogs_)
        {
            indexes_[iter.AimId_] = i;
            ++i;
        }
        emit dataChanged(index(0), index(rowCount()));
        emit orderChanged();
    }
    
    void UnknownsModel::refresh()
    {
        emit dataChanged(index(0), index(rowCount()));
    }

    Data::DlgState UnknownsModel::getDlgState(const QString& _aimId, bool _fromDialog)
    {
        QString contact = _aimId.isEmpty() ? getContactListModel()->selectedContact() : _aimId;
        Data::DlgState state;
        auto iter = std::find_if(dialogs_.begin(), dialogs_.end(), [&_aimId](const Data::DlgState &item)
        {
            return (item.AimId_ == _aimId);
        });
        if (iter != dialogs_.end())
            state = *iter;

        if (_fromDialog)
            sendLastRead(_aimId);

        return state;
    }

    void UnknownsModel::sendLastRead(const QString& _aimId)
    {
        Data::DlgState state;
        state.AimId_ = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId;
        std::vector<Data::DlgState>::iterator iter = std::find(dialogs_.begin(), dialogs_.end(), state);
        if (iter != dialogs_.end() && (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_))
        {
            iter->UnreadCount_ = 0;

            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", state.AimId_);
            collection.set_value_as_int64("message", iter->LastMsgId_);
            Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

            int ind = (int)std::distance(dialogs_.begin(), iter);
            emit dataChanged(index(ind), index(ind));
            emit updated();
        }
    }

    void UnknownsModel::markAllRead()
    {
        for (std::vector<Data::DlgState>::iterator iter = dialogs_.begin(); iter != dialogs_.end(); ++iter)
        {
            if (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", iter->AimId_);
                collection.set_value_as_int64("message", iter->LastMsgId_);
                Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

                int ind = (int)std::distance(dialogs_.begin(), iter);
                emit dataChanged(index(ind), index(ind));
                emit updated();
            }
        }
    }

    void UnknownsModel::hideChat(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());
    }

    bool UnknownsModel::isServiceItem(const QModelIndex& _i) const
    {
        return _i.row() == 0 && isDeleteAllVisible_;
    }

    QModelIndex UnknownsModel::contactIndex(const QString& _aimId)
    {
        int i = isDeleteAllVisible_ ? 1 : 0;
        for (auto iter : dialogs_)
        {
            if (iter.AimId_ == _aimId)
            {
                return index(i);
            }
            ++i;
        }
        return QModelIndex();
    }

    QString UnknownsModel::firstContact()
    {
        if (!dialogs_.empty())
        {
            return dialogs_.front().AimId_;
        }
        return QString();
    }

    int UnknownsModel::unreads(size_t _index) const
    {
        auto i = correctIndex((int)_index);
        if (i < (int)dialogs_.size())
            return (int)dialogs_[i].UnreadCount_;
        return 0;
    }

    int UnknownsModel::totalUnreads() const
    {
        int result = 0;
        for (auto iter : dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_))
                result += iter.UnreadCount_;
        }
        return result;
    }
    
    QString UnknownsModel::nextUnreadAimId()
    {
        for (auto iter : dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_) &&
                iter.UnreadCount_ > 0)
            {
                return iter.AimId_;
            }
        }

        return "";
    }

    QString UnknownsModel::nextAimId(QString _aimId)
    {
        for (int i = 0; i < (int)dialogs_.size(); i++)
        {
            Data::DlgState iter = dialogs_.at(i);
            if (iter.AimId_ == _aimId &&
                i < (int)dialogs_.size() - 1)
            {
                return dialogs_.at(i + 1).AimId_;
            }
        }

        return "";
    }

    QString UnknownsModel::prevAimId(QString _aimId)
    {
        for (int i = 0; i < (int)dialogs_.size(); i++)
        {
            Data::DlgState iter = dialogs_.at(i);
            if (iter.AimId_ == _aimId &&
                i > 0)
            {
                return dialogs_.at(i - 1).AimId_;
            }
        }

        return "";
    }

    int UnknownsModel::correctIndex(int _i) const
    {
        if (isDeleteAllVisible_)
        {
            if (_i == 0)
                return _i;
        
            return _i - 1;
        }
        else
            return _i;
    }

    void UnknownsModel::setDeleteAllVisible(bool _isVisible)
    {
        isDeleteAllVisible_ = _isVisible;
    }

    UnknownsModel* getUnknownsModel()
    {
        if (!g_unknownsModel)
        {
            g_unknownsModel.reset(new UnknownsModel(0));
        }

        return g_unknownsModel.get();
    }

    void ResetUnknownsModel()
    {
        g_unknownsModel.reset();
    }
}
