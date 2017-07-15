#pragma once

namespace Ui
{
    class DialogPlayer;
}

namespace Previewer
{
    class AbstractViewer
        : public QObject
    {
        Q_OBJECT

        public:
Q_SIGNALS:
        void mouseWheelEvent(const int _delta);

    public:
        virtual ~AbstractViewer();

        bool canScroll() const;

        QRect rect() const;

        void scale(double _newScaleFactor);
        void move(const QPoint& _offset);

        void paint(QPainter& _painter);

        double getPreferredScaleFactor() const;
        double getScaleFactor() const;

        virtual bool isZoomSupport() const;
        virtual bool closeFullscreen();

    protected:
        explicit AbstractViewer(const QSize& _viewportSize, QWidget* _parent);

        void init(const QRect& _imageRect);

    protected slots:
        void repaint();

    private:
        virtual void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) = 0;
        virtual void doResize(const QRect& _source, const QRect& _target);
        
        double getPreferredScaleFactor(const QSize& _imageSize) const;

        void fixBounds(const QSize& _bounds, QRect& _child);

        QRect getViewportRect(double _scaleFactor) const;

    protected:
        bool canScroll_;

        QRect imageRect_;

    private:
        QRect viewportRect_;

        QRect viewport_;
        QRect fragment_;

        QPoint offset_;

        double prefferedScaleFactor_;
        double scaleFactor_;
    };


    class GifViewer : public AbstractViewer
    {
        Q_OBJECT
    public:
        static std::unique_ptr<AbstractViewer> create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

    private:
        GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        std::unique_ptr<QMovie> gif_;
    };


    class JpegPngViewer : public AbstractViewer
    {
    public:
        static std::unique_ptr<AbstractViewer> create(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent);

    private:
        JpegPngViewer(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QPixmap originalImage_;
    };

    class FFMpegViewer : public AbstractViewer
    {
        Q_OBJECT

    public:

        static std::unique_ptr<AbstractViewer> create(
            const QString& _fileName, 
            const QSize& _viewportSize, 
            QWidget* _parent,
            QPixmap _preview);

    private:

        QWidget* parent_;

        bool fullscreen_;
        QRect normalSize_;

        QPixmap preview_;

        FFMpegViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent, QPixmap _preview);
        virtual ~FFMpegViewer();

        QSize calculateInitialSize() const;

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;
        void doResize(const QRect& _source, const QRect& _target) override;
        bool closeFullscreen() override;
        bool isZoomSupport() const override;
    private:

        std::unique_ptr<Ui::DialogPlayer> ffplayer_;
    };
}
