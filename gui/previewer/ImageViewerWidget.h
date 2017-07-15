#pragma once

namespace Previewer
{
    class AbstractViewer;

    class ImageViewerWidget final
        : public QLabel
    {
        Q_OBJECT
    public:
        explicit ImageViewerWidget(QWidget* _parent);
        ~ImageViewerWidget();

        void showImage(const QPixmap& _preview, const QString& _fileName);
        bool isZoomSupport() const;

        void zoomIn();
        void zoomOut();

        bool canZoomIn() const;
        bool canZoomOut() const;

        void reset();

        void connectExternalWheelEvent(std::function<void(const int)> _func);
        bool closeFullscreen();

    signals:
        void imageClicked();

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;

        void paintEvent(QPaintEvent* _event);

    private:
        double getScaleStep() const;

        double getZoomInValue(int _zoomStep) const;
        double getZoomOutValue(int _zoomStep) const;

    private:
        QPoint mousePos_;

        int zoomStep_;

        QWidget* parent_;

    private:
        std::unique_ptr<AbstractViewer> viewer_;
    };
}
