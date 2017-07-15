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

namespace Ui
{
    class HistoryControlPage;
}

namespace Logic
{
    struct ItemLessThan
    {
        inline bool operator() (const Logic::ContactItem& first, const Logic::ContactItem& second)
        {
            if (first.Get()->GroupId_ == second.Get()->GroupId_)
            {
                if (first.is_group() && second.is_group())
                    return false;

                if (first.is_group())
                    return true;

                if (second.is_group())
                    return false;

                return first.Get()->GetDisplayName().toUpper() < second.Get()->GetDisplayName().toUpper();
            }

            return first.Get()->GroupId_ < second.Get()->GroupId_;
        }
    };

    class contact_profile;
    typedef std::shared_ptr<contact_profile> profile_ptr;

    class ContactListModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:

        void currentDlgStateChanged() const;
        void selectedContactChanged(QString) const;
        void contactChanged(QString) const;
        void select(QString, qint64, qint64 quote_id = -1) const;
        void profile_loaded(profile_ptr _profile) const;
        void contact_added(QString _contact, bool _result);
        void contact_removed(QString _contact);
        void leave_dialog(QString _contact);
        void results();
        void needSwitchToRecents();
        void needJoinLiveChat(QString _stamp);
        void liveChatJoined(QString);
        void liveChatRemoved(QString);
        void switchTab(QString);
		void ignore_contact(QString);
        void youRoleChanged(QString);

    private Q_SLOTS:

        void contactList(std::shared_ptr<Data::ContactList>, QString);
        void avatarLoaded(QString);
        void presense(Data::Buddy*);
        void groupClicked(int);
        void scrolled(int);
        void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
        void contactRemoved(QString);


    public Q_SLOTS:
        void refresh();
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);

    public:
        explicit ContactListModel(QObject* _parent);

        QVariant data(const QModelIndex& _index, int _role) const;
        Qt::ItemFlags flags(const QModelIndex& _index) const;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const;

        ContactItem* getContactItem(const QString& _aimId);
        void setCurrentCallbackHappened(Ui::HistoryControlPage* _page);

        int getAbsIndexByVisibleIndex(int _cur, int* _visibleCount, int _iterLimit) const;

        void setFocus();
        void setCurrent(QString _aimId, qint64 id, bool _select = false, bool _switchTab = false, std::function<void(Ui::HistoryControlPage*)> _getPageCallback = nullptr, qint64 quote_id = -1);

        const ContactItem* getContactItem(const QString& _aimId) const;

        QString selectedContact() const;
        QString selectedContactName() const;

        void add(const QString& _aimId, const QString& _friendly);

        void setContactVisible(const QString& _aimId, bool visible);

        QString getDisplayName(const QString& _aimId) const;
        QString getInputText(const QString& _aimId) const;
        void setInputText(const QString& _aimId, const QString& _text);
        QDateTime getLastSeen(const QString& _aimId) const;
        QString getLastSeenString(const QString& _aimId) const;
        QString getState(const QString& _aimId) const;
        QString getStatusString(const QString& _aimId) const;
        bool isChat(const QString& _aimId) const;
        bool isMuted(const QString& _aimId) const;
        bool isLiveChat(const QString& _aimId) const;
        bool isOfficial(const QString& _aimId) const;
        bool isNotAuth(const QString& _aimId) const;
        QModelIndex contactIndex(const QString& _aimId);

        void addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack = [](bool) {});
        bool blockAndSpamContact(const QString& _aimId, bool _withConfirmation = true);
        void getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t)> _callBack = [](profile_ptr, int32_t) {});
        void ignoreContact(const QString& _aimId, bool _ignore);
        bool ignoreContactWithConfirm(const QString& _aimId);
        bool isYouAdmin(const QString& _aimId);
        QString getYourRole(const QString& _aimId);
        void setYourRole(const QString& _aimId, const QString& _role);
        void removeContactFromCL(const QString& _aimId);
        void renameChat(const QString& _aimId, const QString& _friendly);
        void renameContact(const QString& _aimId, const QString& _friendly);
        void static getIgnoreList();

        void removeContactsFromModel(const QVector<QString>& _vcontacts);

        void emitChanged(int _first, int _last);

        std::vector<ContactItem> GetCheckedContacts() const;
        void clearChecked();
        void setChecked(const QString& _aimId, bool _isChecked);
        void setIsWithCheckedBox(bool);
        bool getIsChecked(const QString& _aimId) const;

        bool isWithCheckedBox();
        QString contactToTryOnTheme() const;
        void joinLiveChat(const QString& _stamp, bool _silent);

        void next();
        void prev();

        bool contains(const QString& _aimdId) const;
        void updatePlaceholders();

    private:
        std::shared_ptr<bool>	ref_;
        std::function<void(Ui::HistoryControlPage*)> gotPageCallback_;
        void rebuild_index();
        int addItem(Data::Contact* _contact);
        void pushChange(int i);
        void processChanges();
        void sort();
        bool isVisibleItem(const ContactItem& _item);
        int getIndexByOrderedIndex(int _index) const;
        int getOrderIndexByAimid(const QString& _aimId) const;
        void updateSortedIndexesList(std::vector<int>& _list, std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> _less);
        std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> getLessFuncCL(const QDateTime& current) const;
        void updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index);
        int innerRemoveContact(const QString& _aimId);

        std::vector<ContactItem> contacts_;
        std::vector<int> sorted_index_cl_;
        std::vector<int> sorted_index_recents_;
        QHash<QString, int> indexes_;

        int scrollPosition_;
        mutable int minVisibleIndex_;
        mutable int maxVisibleIndex_;
        std::vector<int> updatedItems_;

        QString currentAimdId_;

        QTimer* timer_;

        std::vector<ContactItem> match_;
        bool searchRequested_;
        bool isSearch_;
        bool isWithCheckedBox_;
        bool is_index_valid_;
    };

    ContactListModel* getContactListModel();
    void ResetContactListModel();
}
