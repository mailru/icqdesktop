#pragma once

namespace Data
{
    struct Image;
}

namespace Ui
{
    class CustomButton;
}

namespace Previewer
{
    class DownloadWidget;
    class ImageCache;
    class ImageIterator;
    class ImageLoader;
    class ImageViewerWidget;

    class GalleryWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit GalleryWidget(QWidget* _parent);
        ~GalleryWidget();

        void openGallery(const QString& _aimId, const Data::Image& _image, const QString& _localPath);

    public slots:
        void closeGallery();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;

    private slots:
        void onNumberFound();
        void onIteratorUpdated();

        void onPrevClicked();
        void onNextClicked();

        void onImageClicked();

        void onSaveClicked();

        void onImageLoaded();
        void onImageLoadingError();

        void onCacheLoaded();

        void onZoomIn();
        void onZoomOut();

        void onWheelEvent(const int _delta);
        void onEscapeClick();

    private:
        void moveToScreen();

        QFrame* createZoomFrame();
        QFrame* createButtonFrame();
        QFrame* createCloseFrame();

        QString getDefaultText() const;
        QString getInfoText(int _n, int _total) const;

        void updateButtonFrame();

        void showImage(const QPixmap& _preview, const QString& _fileName);
        void showProgress();
        void showError();

        void setFirstImage(const Data::Image& _imageInfo, const QString& _localPath);

        void downloadRelativeImage(int _n);

        ImageLoader& currentLoader() const;

        void connectLoader();
        void disconnectLoader();

        void connectContainerEvents();

        bool hasPrev() const;
        void prev();

        bool hasNext() const;
        void next();

        void startLoading();

        void tryShowImage(ImageLoader& _loader);

    private:
        QString aimId_;

        std::deque<std::unique_ptr<ImageLoader>> loaders_;

        std::unique_ptr<ImageCache> imageCache_;
        std::unique_ptr<ImageIterator> imageIterator_;

        QStackedLayout* imageOrDownload_;

        DownloadWidget* download_;
        ImageViewerWidget* imageViewer_;

        Ui::CustomButton* zoomOut_;
        Ui::CustomButton* zoomIn_;

        Ui::CustomButton* prev_;
        Ui::CustomButton* next_;
        Ui::CustomButton* openInBrowser_;
        Ui::CustomButton* save_;
        QLabel* info_;

        bool navigationKeyPressed_;

        bool initComplete_;
        bool cacheLoaded_;

        QTimer* scrollTimer_;
        QTimer* delayTimer_;

        std::shared_ptr<bool> ref_;
    };
}
