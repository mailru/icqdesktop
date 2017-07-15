#pragma once
#include "../../types/snap.h"

namespace Logic
{
    struct SnapItem
    {
        SnapItem()
            : Views_(0)
            , HaveNewSnap_(false)
            , IsOfficial_(false)
        {
        }

        QString AimId_;
        QString Friendly_;
        QString TimeLeft_;
        QPixmap Snap_;
        int Views_;
        bool HaveNewSnap_;
        bool IsOfficial_;
    };

    struct PlayItem
    {
        PlayItem()
            : User_(false)
            , Id_(-1)
        {
        }

        qint64 Id_;
        QString Url_;
        QString AimId_;
        QString OriginalAimId_;
        QString LocalPath_;

        QPixmap Preview_;
        bool User_;
        bool First_;
    };

    struct PreviewItem
    {
        PreviewItem()
            : Id_(-1)
        {
        }

        bool operator==(const PreviewItem& other)
        {
            return AimId_ == other.AimId_;
        }

        QString AimId_;
        qint64 Id_;
    };



	class SnapStorage : public QStandardItemModel
    {
        Q_OBJECT
Q_SIGNALS:
        void playSnap(QString, QString, QString, QString, qint64, bool);
        void playUserSnap(QString, QString, QString, QString, qint64, bool);
        void indexChanged();
        void tvStarted(QList<Logic::PreviewItem>, bool);
        void snapProgress(QString, int);
        void previewChanged(QString);
        void removed(QString);
        void snapRemoved(QString, qint64, QString);

    public:
        SnapStorage();
        ~SnapStorage();

        QVariant data(const QModelIndex& _index, int _role) const;
        Qt::ItemFlags flags(const QModelIndex& _index) const;

        void startTv(int row, int col);
        void startUserSnaps(const QString& _aimId = QString());
        QPixmap getFirstUserPreview(const QString& _aimId = QString());

        QPixmap getSnapPreviewFull(qint64 _id);
        QString getSnapUrl(qint64 _id);

        int getSnapsCount(const QString& _aimId) const;
        int loadingSnapsCount(const QString& _aimId) const;

        QString getFriednly(const QString& _aimId) const;
        bool isOfficial(const QString& _aimId) const;
        int getViews(qint64 id) const;
        int32_t getTimestamp(qint64 id) const;

        PlayItem getLoadingSnap() const;
        bool haveLoading() const;

        int getFriendsSnapsCount() const;
        int getFeaturedSnapsCount() const;

        int getFriendsRow() const { return 0; }
        int getFeaturedRow() const { return 1; }

        void readSnap(const QString& aimId, qint64 id);
        void clearPlaylist();

        void clearStorage();

    public Q_SLOTS:
        void refresh();

    private Q_SLOTS:
        void userSnaps(Logic::UserSnapsInfo, bool);
        void userSnapsStorage(QList<Logic::UserSnapsInfo>, bool);
        void snapPreviewInfoDownloaded(qint64 _snapId, QString _preview, QString _ttl_id, bool _found);
        void imageDownloaded(qint64 _seq, QString _rawUri, QPixmap _image, QString _localPath);
        void fileDownloaded(qint64, QString, QString);
        void fileDownloading(qint64, QString, qint64, qint64);
        void fileSharingError(qint64, QString, qint32);
        void userSnapsState(Logic::SnapState);
        void expired();

    private:
        bool updateSnaps(Logic::UserSnapsInfo _info);
        QPixmap preparePreview(const QPixmap& _preview);
        QPixmap prepareFull(const QPixmap& _preview);
        void rebuildIndex(bool fromCache);
        SnapItem getData(int row, int col) const;
        void calcUserLastSnaps(const QString& _aimid);
        void updateSnapImage(qint64 _snapId, QPixmap image);
        void updateSnapLocalPath(const QString& _url, const QString& _local);
        void downloadSnap(const QString& _aimId, const QString& _url);
        void processPlaylist();

        QMap<QString, Logic::UserSnapsInfo> Snaps_;
        QList<int64_t> Seq_;
        QList<PlayItem> PlayList_;

        //models data
        std::vector<UserSnapsInfo> FeaturedIndex_;
        std::vector<UserSnapsInfo> Index_;
        QString Selected_;

        QStringList DownloadingUrls_;
        QTimer* expiredTimer_;
        qint64 expiredId_;
    };

    SnapStorage* GetSnapStorage();
}

Q_DECLARE_METATYPE(Logic::SnapItem);
Q_DECLARE_METATYPE(Logic::PreviewItem);