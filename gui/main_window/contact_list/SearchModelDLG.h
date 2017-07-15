#pragma once

#include "ContactItem.h"
#include "../../types/contact.h"
#include "../../types/message.h"
#include "AbstractSearchModel.h"

namespace Logic
{
	class SearchModelDLG : public AbstractSearchModel
	{
		Q_OBJECT
	Q_SIGNALS:
		void results();

	public Q_SLOTS:
		void searchPatternChanged(QString) override;
        void searchedMessage(Data::DlgState);
        void searchedContacts(QList<Data::DlgState>, qint64);
        void searchEnded();
        void disableSearchInDialog();
        void repeatSearch();

    private Q_SLOTS:
        void avatarLoaded(QString);
        void contactRemoved(QString contact);
        void sortDialogs();
        void emptySearchResults(qint64);

        void recvHideNoSearchResults();

	public:
		explicit SearchModelDLG(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		void setFocus() override;
		void emitChanged(int first, int last) override;
        virtual bool isServiceItem(int i) const override;

        void setSearchInDialog(QString _aimid);
        QString getDialogAimid() const;

        bool isSearchInDialog() const;
        int get_abs_index(int _ind) const;
        void clear_match();

        void setSearchInHistory(bool value) { SearchInHistory_ = value; }
        void setContactsOnly(bool value) { ContactsOnly_ = value; }

        QString getCurrentPattern() const;
        int count() const;

        int contactsCount() const
        {
            return ContactsCount_;
        }

	private:
        bool less(const ContactItem& first, const ContactItem& second);
        int getAllMessagesIndex() const;
        int correctIndex(int i) const;

		std::vector<Data::DlgState> Match_;
        std::map<int64_t, int, std::greater<int64_t>> TopKeys_;
        std::unordered_set<int64_t> Msgs_;
		std::vector<QStringList> SearchPatterns_;
		bool SearchRequested_;
        qint64 LastRequestId_;
        qint16 ContactsCount_;
        bool MessageResults_;
        QTimer* Timer_;

        bool isSearchInDialog_;
        QString aimid_;
        QString lastSearchPattern_;
        bool EmptySearch_;
        bool SearchInHistory_;
        bool ContactsOnly_;
	};

    SearchModelDLG* getSearchModelDLG();
	SearchModelDLG* getCustomSearchModelDLG(bool searchInHistory, bool contactsObly);
}