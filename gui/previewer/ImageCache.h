#pragma once

#include "../types/images.h"

namespace Previewer
{
    class ImageIterator;

    class ImageCache
        : public QObject
    {
        Q_OBJECT
    public:
        explicit ImageCache(const QString& _aimId);

        void repair();

    signals:
        void imagesGetted(bool cacheLoaded);

    private slots:
        void onGetImagesResult(Data::ImageListPtr _result);

    private:
        void loadImages(int64_t _from);

    private:
        friend class ImageIterator;

        QString         aimId_;
        Data::ImageList images_;

        mutable std::mutex mutex_;
    };
}
