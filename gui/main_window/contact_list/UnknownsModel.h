#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

namespace Utils
{
    class SignalsDisconnector;
}

namespace Ui
{
    class MainWindow;
}

namespace Logic
{
    class UnknownsModel : public CustomAbstractListModel
    {
        Q_OBJECT

            Q_SIGNALS:
        void orderChanged();
        void updated();
        void readStateChanged(QString);
        void selectContact(QString);
        void dlgStatesHandled(std::shared_ptr<QList<Data::DlgState>>);
        
    public Q_SLOTS:
        void refresh();
        
    private Q_SLOTS:
        void activeDialogHide(QString);
        void contactChanged(QString);
        void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
        void sortDialogs();

    public:
        explicit UnknownsModel(QObject* _parent = nullptr);
        ~UnknownsModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const;
        QVariant data(const QModelIndex& _index, int _role) const;
        Qt::ItemFlags flags(const QModelIndex& _index) const;

        int itemsCount() const;

        void add(const QString& _aimId);
        
        Data::DlgState getDlgState(const QString& _aimId = QString(), bool _fromDialog = false);

        void sendLastRead(const QString& _aimId = QString());
        void markAllRead();
        void hideChat(const QString& _aimId);

        QModelIndex contactIndex(const QString& _aimId);

        bool isServiceItem(const QModelIndex& _i) const;

        int unreads(size_t _i) const;
        int totalUnreads() const;
        
        QString firstContact();
        QString nextUnreadAimId();
        QString nextAimId(QString _aimId);
        QString prevAimId(QString _aimId);
        void setDeleteAllVisible(bool _isVisible);

    private:
        int correctIndex(int _i) const;

        std::vector<Data::DlgState> dialogs_;
        QHash<QString, int> indexes_;
        QTimer* timer_;
        bool isDeleteAllVisible_;
    };

    UnknownsModel* getUnknownsModel();
    void ResetUnknownsModel();
}

