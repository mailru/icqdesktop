#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

namespace Ui
{
    class MainWindow;
}

namespace Logic
{
	class RecentsModel : public CustomAbstractListModel
	{
		Q_OBJECT

	Q_SIGNALS:
		void orderChanged();
		void updated();
        void readStateChanged(QString);
        void selectContact(QString);
        void dlgStatesHandled(std::shared_ptr<QList<Data::DlgState>>);
        void favoriteChanged(QString);

    public Q_SLOTS:
        void refresh();
        
	private Q_SLOTS:
		void activeDialogHide(QString);
		void contactChanged(QString);
		void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
		void sortDialogs();
        void contactRemoved(QString);

	public:
		explicit RecentsModel(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;

		Data::DlgState getDlgState(const QString& aimId = QString(), bool fronDialog = false);
        void unknownToRecents(Data::DlgState);

        void toggleFavoritesVisible();
        
        void unknownAppearance();

		void sendLastRead(const QString& aimId = QString());
		void markAllRead();
		void hideChat(const QString& aimId);
		void muteChat(const QString& aimId, bool mute);

        bool isFavorite(const QString& aimid) const;
        bool isServiceItem(const QModelIndex& i) const;
        bool isFavoritesGroupButton(const QModelIndex& i) const;
        bool isFavoritesVisible() const;
        bool isUnknownsButton(const QModelIndex& i) const;
        void setFavoritesHeadVisible(bool _isVisible);

        bool isServiceAimId(const QString& _aimId);

		QModelIndex contactIndex(const QString& aimId);

		int totalUnreads() const;
        int recentsUnreads() const;
        int favoritesUnreads() const;
        
        QString firstContact();
        QString nextUnreadAimId();
        QString nextAimId(QString aimId);
        QString prevAimId(QString aimId);

        bool lessRecents(const QString& _aimid1, const QString& _aimid2);

        std::vector<QString> getSortedRecentsContacts() const;

        void setSnapsVisible(bool _visible);
        bool isSnapsVisible() const { return SnapsVisible_; }

	private:
        int correctIndex(int i) const;
        int visibleContactsInFavorites() const;
        
        int getUnknownHeaderIndex() const;
        int getSizeOfUnknownBlock() const;

        int getFavoritesHeaderIndex() const;
        int getRecentsHeaderIndex() const;
        int getVisibleServiceItemInFavorites() const;

		std::vector<Data::DlgState> Dialogs_;
		QHash<QString, int> Indexes_;
		QTimer* Timer_;
        quint16 FavoritesCount_;
        bool FavoritesVisible_;
        bool FavoritesHeadVisible_;
        bool SnapsVisible_;
	};

	RecentsModel* getRecentsModel();
    void ResetRecentsModel();
}