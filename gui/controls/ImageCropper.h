#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

namespace Ui
{
    enum CursorPosition
    {
        CursorPositionUndefined,
        CursorPositionMiddle,
        CursorPositionTop,
        CursorPositionBottom,
        CursorPositionLeft,
        CursorPositionRight,
        CursorPositionTopLeft,
        CursorPositionTopRight,
        CursorPositionBottomLeft,
        CursorPositionBottomRight
    };

    namespace
    {
        const QRect INIT_CROPPING_RECT = QRect();
        const QSizeF INIT_PROPORTION = QSizeF(1.0, 1.0);
        const int INDENT = 10;
        const int CORNER_RECT_SIZE = 4;
    }

    class ImageCropperPrivate
    {
    public:
        ImageCropperPrivate() :
            imageForCropping(QPixmap()),
            croppingRect(INIT_CROPPING_RECT),
            lastStaticCroppingRect(QRect()),
            cursorPosition(CursorPositionUndefined),
            isMousePressed(false),
            isProportionFixed(false),
            startResizeCropMousePos(QPoint()),
            proportion(INIT_PROPORTION),
            deltas(INIT_PROPORTION),
            backgroundColor(QColor("#000000")),
            croppingRectBorderColor(QColor("#ffffff"))
        {}

    public:
        QPixmap imageForCropping;
        QRectF croppingRect;
        QRectF lastStaticCroppingRect;
        CursorPosition cursorPosition;
        bool isMousePressed;
        bool isProportionFixed;
        QPointF startResizeCropMousePos;
        QSizeF proportion;
        QSizeF deltas;
        QColor backgroundColor;
        QColor croppingRectBorderColor;
        QPixmap scaledImage;
    };

    class ImageCropper : public QWidget
    {
        Q_OBJECT

    public:
        ImageCropper(QWidget *parent, const QSize &minimumSize = QSize());
        ~ImageCropper();

        public slots:
            void setImage(const QPixmap& _image);
            void setBackgroundColor(const QColor& _backgroundColor);
            void setCroppingRectBorderColor(const QColor& _borderColor);
            void setProportion(const QSizeF& _proportion);
            void setProportionFixed(const bool _isFixed);
            void setCroppingRect(QRectF _croppingRect);

    public:
        const QPixmap cropImage() const;
        const QPixmap getImage() const;
        const QRectF getCroppingRect() const;

    protected:
        virtual void paintEvent(QPaintEvent* _event);
        virtual void mousePressEvent(QMouseEvent* _event);
        virtual void mouseMoveEvent(QMouseEvent* _event);
        virtual void mouseReleaseEvent(QMouseEvent* _event);

    private:
        CursorPosition cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition);
        void updateCursorIcon(const QPointF& _mousePosition);

        const QRectF calculateGeometry(
            const QRectF& _sourceGeometry,
            const CursorPosition _cursorPosition,
            const QPointF& _mouseDelta
            );
        const QRectF calculateGeometryWithCustomProportions(
            const QRectF& _sourceGeometry,
            const CursorPosition _cursorPosition,
            const QPointF& _mouseDelta
            );
        const QRectF calculateGeometryWithFixedProportions(const QRectF &_sourceGeometry,
            const CursorPosition _cursorPosition,
            const QPointF &_mouseDelta,
            const QSizeF &_deltas
            );
        void updareCroppingRect(bool _need_update);

    private:
        std::unique_ptr<ImageCropperPrivate> pimpl;
        int width_;
        int height_;
        int minScaledSize_;
    };
}
#endif // IMAGECROPPER_H
