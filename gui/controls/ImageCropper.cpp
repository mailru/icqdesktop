#include "stdafx.h"

#include "ImageCropper.h"
#include "../utils/utils.h"

namespace Ui
{
    namespace {
        static QSize WIDGET_MINIMUM_SIZE;
        static const auto MIN_SIZE = 200;
    }

    ImageCropper::ImageCropper(QWidget* parent, const QSize &minimumSize)
        : QWidget(parent)
        , pimpl(new ImageCropperPrivate)
    {
        WIDGET_MINIMUM_SIZE = (minimumSize.isEmpty() ? QSize(Utils::scale_value(600), Utils::scale_value(420)) : minimumSize);
        width_ = WIDGET_MINIMUM_SIZE.width();
        height_ = WIDGET_MINIMUM_SIZE.height();

        setMouseTracking(true);
    }

    ImageCropper::~ImageCropper()
    {
    }

    void ImageCropper::setImage(const QPixmap& _image)
    {
        pimpl->imageForCropping = _image;
        pimpl->scaledImage = pimpl->imageForCropping.scaled(QSize(width_, height_), Qt::KeepAspectRatio, Qt::SmoothTransformation);
         
        if (height_ != pimpl->scaledImage.height())
        {
            height_ = pimpl->scaledImage.height();
        }

        if (width_ != pimpl->scaledImage.width())
        {
            width_ = pimpl->scaledImage.width();
        }
        this->setMinimumSize(width_, height_);
        this->setFixedSize(width_, height_);

        // NOTE : not need scale min_size here, before it will be scaled with image
        minScaledSize_ = Utils::scale_value(MIN_SIZE);
        update();
    }

    void ImageCropper::setBackgroundColor(const QColor& _backgroundColor)
    {
        pimpl->backgroundColor = _backgroundColor;
        update();
    }

    void ImageCropper::setCroppingRectBorderColor(const QColor& _borderColor)
    {
        pimpl->croppingRectBorderColor = _borderColor;
        update();
    }

    void ImageCropper::updareCroppingRect(bool _need_update)
    {
        if (pimpl->isProportionFixed) 
        {
            auto croppintRectSideRelation = 1.0 * pimpl->croppingRect.width() / pimpl->croppingRect.height();
            auto proportionSideRelation = 1.0 * pimpl->proportion.width() / pimpl->proportion.height();

            if (croppintRectSideRelation != proportionSideRelation)
            {
                if (pimpl->croppingRect.width() < pimpl->croppingRect.height())
                {
                    pimpl->croppingRect.setHeight(pimpl->croppingRect.width() * pimpl->deltas.height());
                } else {
                    pimpl->croppingRect.setWidth(pimpl->croppingRect.height() * pimpl->deltas.width());
                }
                if (_need_update)
                {
                    update();
                }
            }
        }
    }

    void ImageCropper::setProportion(const QSizeF& _proportion)
    {
        if (pimpl->proportion != _proportion)
        {
            pimpl->proportion = _proportion;
            auto heightDelta = 1.0 * _proportion.height() / _proportion.width();
            auto widthDelta = 1.0 * _proportion.width() / _proportion.height();
            pimpl->deltas.setHeight(heightDelta);
            pimpl->deltas.setWidth(widthDelta);
        }

        updareCroppingRect(true /* need_update */);
    }

    void ImageCropper::setProportionFixed(const bool _isFixed)
    {
        if (pimpl->isProportionFixed != _isFixed)
        {
            pimpl->isProportionFixed = _isFixed;
            setProportion(pimpl->proportion);
        }
    }

    void ImageCropper::setCroppingRect(QRectF _croppingRect)
    {
        pimpl->croppingRect = _croppingRect;
    }

    const QPixmap ImageCropper::cropImage() const
    {
        auto scaledImageSize = pimpl->scaledImage;
        
        float leftDelta = 0;
        float topDelta = 0;
        if (this->size().height() == scaledImageSize.height()) {
            leftDelta = (this->width() - scaledImageSize.width()) / 2;
        } else {
            topDelta = (this->height() - scaledImageSize.height()) / 2;
        }

        auto xScale = 1.0 * pimpl->imageForCropping.width()  / scaledImageSize.width();
        auto yScale = 1.0 * pimpl->imageForCropping.height() / scaledImageSize.height();

        QRectF realSizeRect(
            QPointF(pimpl->croppingRect.left() - leftDelta, pimpl->croppingRect.top() - topDelta),
            pimpl->croppingRect.size());

        realSizeRect.setLeft((pimpl->croppingRect.left() - leftDelta) * xScale);
        realSizeRect.setTop ((pimpl->croppingRect.top() - topDelta) * yScale);

        realSizeRect.setWidth(pimpl->croppingRect.width() * xScale);
        realSizeRect.setHeight(pimpl->croppingRect.height() * yScale);

        return pimpl->imageForCropping.copy(realSizeRect.toRect());
    }
    
    const QRectF ImageCropper::getCroppingRect() const
    {
        return pimpl->croppingRect; 
    }

    const QPixmap ImageCropper::getImage() const
    {
        return pimpl->imageForCropping.copy();
    }

    void ImageCropper::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent( _event );

        QPainter widgetPainter(this);
        {
            widgetPainter.fillRect( this->rect(), pimpl->backgroundColor );
            if (this->size().height() == pimpl->scaledImage.height()) {
                widgetPainter.drawPixmap( ( this->width() - pimpl->scaledImage.width() ) / 2, 0, pimpl->scaledImage );
            } else {
                widgetPainter.drawPixmap( 0, ( this->height() - pimpl->scaledImage.height() ) / 2, pimpl->scaledImage );
            }
        }

        {
            // if it's first showing, center crop area
            if (pimpl->croppingRect.isNull()) {
                
                auto width = std::max<int>(this->minScaledSize_, std::min(WIDGET_MINIMUM_SIZE.width(), this->width()) / 2);
                auto height = std::max<int>(this->minScaledSize_, std::min(WIDGET_MINIMUM_SIZE.height(), this->height()) / 2);
                pimpl->croppingRect.setSize(QSize(width, height));
                
                updareCroppingRect(false);

                float x = (this->width() - pimpl->croppingRect.width()) / 2;
                float y = (this->height() - pimpl->croppingRect.height()) / 2;
                pimpl->croppingRect.moveTo(x, y);
            }

            // shadow area
            QPainterPath p;
            p.addRect(pimpl->croppingRect);
            p.addRect(this->rect());
            QColor foggingColor("#000000");
            foggingColor.setAlphaF(0.6);
            widgetPainter.setBrush(QBrush(foggingColor));
            widgetPainter.setPen(Qt::transparent);
            widgetPainter.drawPath(p);

            // border and corners
            widgetPainter.setPen(pimpl->croppingRectBorderColor);

            {
                widgetPainter.setBrush(QBrush(Qt::transparent));
                widgetPainter.drawRect(pimpl->croppingRect);
            }

            {
                widgetPainter.setBrush(QBrush(pimpl->croppingRectBorderColor));

                auto leftXCoord   = pimpl->croppingRect.left() - CORNER_RECT_SIZE/2;
                auto rightXCoord  = pimpl->croppingRect.right() - CORNER_RECT_SIZE/2;

                auto topYCoord    = pimpl->croppingRect.top() - CORNER_RECT_SIZE/2;
                auto bottomYCoord = pimpl->croppingRect.bottom() - CORNER_RECT_SIZE/2;

                const QSize pointSize(Utils::scale_value(CORNER_RECT_SIZE), Utils::scale_value(CORNER_RECT_SIZE));
                QVector<QRect> points;
                points
                    // left side
                    << QRect( QPoint(leftXCoord, topYCoord), pointSize )
                    << QRect( QPoint(leftXCoord, bottomYCoord), pointSize )
                    // right size
                    << QRect( QPoint(rightXCoord, topYCoord), pointSize )
                    << QRect( QPoint(rightXCoord, bottomYCoord), pointSize );

                widgetPainter.drawRects( points );
            }
        }
        widgetPainter.end();
    }

    void ImageCropper::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton) {
            pimpl->isMousePressed = true;
            pimpl->startResizeCropMousePos = _event->pos();
            pimpl->lastStaticCroppingRect = pimpl->croppingRect;
        }
        updateCursorIcon(_event->pos());
    }

    void ImageCropper::mouseMoveEvent(QMouseEvent* _event)
    {
        QPointF mousePos = _event->pos(); // relative this
        if (!pimpl->isMousePressed) {
            pimpl->cursorPosition = cursorPosition(pimpl->croppingRect, mousePos);
            updateCursorIcon(mousePos);
        } else if (pimpl->cursorPosition != CursorPositionUndefined) {
            QPointF mouseDelta;

            QPointF limitedMousePos = mousePos;

            mouseDelta.setX(limitedMousePos.x() - pimpl->startResizeCropMousePos.x() );
            mouseDelta.setY(limitedMousePos.y() - pimpl->startResizeCropMousePos.y() );

            if (pimpl->cursorPosition != CursorPositionMiddle) {
                auto newGeometry = calculateGeometry(pimpl->lastStaticCroppingRect, pimpl->cursorPosition, mouseDelta);
                // inside out rect
                if (!newGeometry.isNull()) {
                    pimpl->croppingRect = newGeometry;
                }
            } else { // move crop area
                auto new_top_left_crop_corner = pimpl->lastStaticCroppingRect.topLeft() + mouseDelta;

                new_top_left_crop_corner.setX(std::max<int>(0, new_top_left_crop_corner.x()));
                new_top_left_crop_corner.setY(std::max<int>(0, new_top_left_crop_corner.y()));

                new_top_left_crop_corner.setX(std::min<int>(new_top_left_crop_corner.x(), this->size().width() - pimpl->croppingRect.width()));
                new_top_left_crop_corner.setY(std::min<int>(new_top_left_crop_corner.y(), this->size().height() - pimpl->croppingRect.height()));

                pimpl->croppingRect.moveTo(new_top_left_crop_corner);
            }
            update();
        }
    }

    void ImageCropper::mouseReleaseEvent(QMouseEvent* _event)
    {
        pimpl->isMousePressed = false;
        updateCursorIcon(_event->pos());
    }

    namespace {
        static bool isPointNearSide (const int _sideCoordinate, const int _pointCoordinate)
        {
            return std::abs(_sideCoordinate - _pointCoordinate) < INDENT;
        }
    }

    CursorPosition ImageCropper::cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition)
    {
        auto cursorPosition = CursorPositionUndefined;

        auto top_left = _mousePosition - QPointF(INDENT, INDENT);
        auto buttom_right = _mousePosition + QPointF(INDENT, INDENT);
        auto mouse_rect = QRectF(top_left, buttom_right);

        if (_cropRect.intersects(mouse_rect)) {
            // two directions
            if (isPointNearSide(_cropRect.top(), _mousePosition.y())
                && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
                    cursorPosition = CursorPositionTopLeft;
            } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y()) 
                && isPointNearSide(_cropRect.left(), _mousePosition.x())) {
                    cursorPosition = CursorPositionBottomLeft;
            } else if (isPointNearSide(_cropRect.top(), _mousePosition.y()) 
                && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
                    cursorPosition = CursorPositionTopRight;
            } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y()) 
                && isPointNearSide(_cropRect.right(), _mousePosition.x())) {
                    cursorPosition = CursorPositionBottomRight;
            // one direction
            } else if (isPointNearSide(_cropRect.left(), _mousePosition.x())) {
                cursorPosition = CursorPositionLeft;
            } else if (isPointNearSide(_cropRect.right(), _mousePosition.x())) {
                cursorPosition = CursorPositionRight;
            } else if (isPointNearSide(_cropRect.top(), _mousePosition.y())) {
                cursorPosition = CursorPositionTop;
            } else if (isPointNearSide(_cropRect.bottom(), _mousePosition.y())) {
                cursorPosition = CursorPositionBottom;
            // no direction
            } else {
                cursorPosition = CursorPositionMiddle;
            }
        }
        return cursorPosition;
    }

    void ImageCropper::updateCursorIcon(const QPointF& _mousePosition)
    {
        QCursor cursorIcon;
        
        switch (cursorPosition(pimpl->croppingRect, _mousePosition))
        {
        case CursorPositionTopRight:
        case CursorPositionBottomLeft:
            cursorIcon = QCursor(Qt::SizeBDiagCursor);
            break;
        case CursorPositionTopLeft:
        case CursorPositionBottomRight:
            cursorIcon = QCursor(Qt::SizeFDiagCursor);
            break;
        case CursorPositionTop:
        case CursorPositionBottom:
            cursorIcon = QCursor(Qt::SizeVerCursor);
            break;
        case CursorPositionLeft:
        case CursorPositionRight:
            cursorIcon = QCursor(Qt::SizeHorCursor);
            break;
        case CursorPositionMiddle:
            cursorIcon = pimpl->isMousePressed ? QCursor(Qt::ClosedHandCursor) : QCursor(Qt::OpenHandCursor);
            break;
        case CursorPositionUndefined:
        default:
            cursorIcon = QCursor(Qt::ArrowCursor);
            break;
        }
        this->setCursor(cursorIcon);
    }

    const QRectF ImageCropper::calculateGeometry(
        const QRectF& _sourceGeometry,
        const CursorPosition _cursorPosition,
        const QPointF& _mouseDelta
        )
    {
        QRectF resultGeometry;
        if (pimpl->isProportionFixed)
        {
            resultGeometry = calculateGeometryWithFixedProportions(_sourceGeometry,
                _cursorPosition, _mouseDelta, pimpl->deltas);
        } else {
            resultGeometry = calculateGeometryWithCustomProportions(_sourceGeometry,
                _cursorPosition, _mouseDelta);
        }

        // inside out rectangle
        if (resultGeometry.left() >= resultGeometry.right() || resultGeometry.top() >= resultGeometry.bottom()) 
        {
            resultGeometry = QRect();
        }

        return resultGeometry;
    }

    const QRectF ImageCropper::calculateGeometryWithCustomProportions(
        const QRectF& _sourceGeometry,
        const CursorPosition _cursorPosition,
        const QPointF& _mouseDelta
        )
    {
        auto resultGeometry = _sourceGeometry;

        switch ( _cursorPosition )
        {
        case CursorPositionTopLeft:
            resultGeometry.setLeft( _sourceGeometry.left() + _mouseDelta.x() );
            resultGeometry.setTop ( _sourceGeometry.top()  + _mouseDelta.y() );
            break;
        case CursorPositionTopRight:
            resultGeometry.setTop  ( _sourceGeometry.top()   + _mouseDelta.y() );
            resultGeometry.setRight( _sourceGeometry.right() + _mouseDelta.x() );
            break;
        case CursorPositionBottomLeft:
            resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
            resultGeometry.setLeft  ( _sourceGeometry.left()   + _mouseDelta.x() );
            break;
        case CursorPositionBottomRight:
            resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
            resultGeometry.setRight ( _sourceGeometry.right()  + _mouseDelta.x() );
            break;
        case CursorPositionTop:
            resultGeometry.setTop( _sourceGeometry.top() + _mouseDelta.y() );
            break;
        case CursorPositionBottom:
            resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
            break;
        case CursorPositionLeft:
            resultGeometry.setLeft( _sourceGeometry.left() + _mouseDelta.x() );
            break;
        case CursorPositionRight:
            resultGeometry.setRight( _sourceGeometry.right() + _mouseDelta.x() );
            break;
        default:
            break;
        }

        return resultGeometry;
    }

    const QRectF ImageCropper::calculateGeometryWithFixedProportions(
        const QRectF& _sourceGeometry,
        const CursorPosition _cursorPosition,
        const QPointF& _mouseDelta,
        const QSizeF& _deltas
        )
    {
        auto resultGeometry = _sourceGeometry;

        QPointF mouseDelta = _mouseDelta;

        switch (_cursorPosition)
        {
        case CursorPositionLeft:
            if (_sourceGeometry.top() + mouseDelta.x() * _deltas.height() < 0)
               mouseDelta.setX(0 - _sourceGeometry.top() / _deltas.height());

            if (_sourceGeometry.left() + mouseDelta.x() < 0)
               mouseDelta.setX(0 - _sourceGeometry.left());

            if (_sourceGeometry.right() - _sourceGeometry.left() - mouseDelta.x() < this->minScaledSize_)
                mouseDelta.setX(- this->minScaledSize_ + _sourceGeometry.right() - _sourceGeometry.left());

            resultGeometry.setTop(_sourceGeometry.top() + mouseDelta.x() * _deltas.height());
            resultGeometry.setLeft(_sourceGeometry.left() + mouseDelta.x());
            break;
        case CursorPositionRight:
            if (_sourceGeometry.top() - mouseDelta.x() * _deltas.height() < 0)
                mouseDelta.setX(_sourceGeometry.top() /  _deltas.height());

            if (_sourceGeometry.right() + mouseDelta.x() > this->width())
                mouseDelta.setX(this->width() - _sourceGeometry.right());

            if (_sourceGeometry.right() - _sourceGeometry.left() + mouseDelta.x() < this->minScaledSize_)
                mouseDelta.setX(this->minScaledSize_ - _sourceGeometry.right() + _sourceGeometry.left());

            resultGeometry.setTop(_sourceGeometry.top() - mouseDelta.x() * _deltas.height());
            resultGeometry.setRight(_sourceGeometry.right() + mouseDelta.x());
            break;
        case CursorPositionTop:
             if (_sourceGeometry.top() + mouseDelta.y() < 0)
               mouseDelta.setY(0 - _sourceGeometry.top());

            if (_sourceGeometry.right() - mouseDelta.y() * _deltas.width() > this->width())
               mouseDelta.setY((_sourceGeometry.right() - this->width()) / _deltas.width());

            if (-_sourceGeometry.top() - mouseDelta.y() + _sourceGeometry.bottom() < this->minScaledSize_)
                mouseDelta.setY(-this->minScaledSize_ + _sourceGeometry.top() + _sourceGeometry.bottom());

            resultGeometry.setTop(_sourceGeometry.top() + mouseDelta.y());
            resultGeometry.setRight(_sourceGeometry.right() - mouseDelta.y() * _deltas.width());
            break;
        case CursorPositionBottom:
             if (_sourceGeometry.bottom() + mouseDelta.y() > this->height())
               mouseDelta.setY(-_sourceGeometry.bottom() + this->height());

            if (_sourceGeometry.right() + mouseDelta.y() * _deltas.width() > this->width())
               mouseDelta.setY((this->width() - _sourceGeometry.right()) / _deltas.width());

            if (-_sourceGeometry.top() + mouseDelta.y() + _sourceGeometry.bottom() < this->minScaledSize_)
                mouseDelta.setY(this->minScaledSize_ + _sourceGeometry.top() - _sourceGeometry.bottom());

            resultGeometry.setBottom(_sourceGeometry.bottom() + mouseDelta.y());
            resultGeometry.setRight(_sourceGeometry.right() + mouseDelta.y() * _deltas.width());
            break;
        case CursorPositionTopLeft:
            if ((mouseDelta.x() * _deltas.height()) < (mouseDelta.y())) {

                if (_sourceGeometry.top() + mouseDelta.x() * _deltas.height() < 0)
                    mouseDelta.setX(0 - _sourceGeometry.top()/  _deltas.height());

                if (_sourceGeometry.left() + mouseDelta.x() < 0)
                    mouseDelta.setX(0 - _sourceGeometry.left());

                if (_sourceGeometry.right() - _sourceGeometry.left() - mouseDelta.x() < this->minScaledSize_)
                    mouseDelta.setX(- this->minScaledSize_ + _sourceGeometry.right() - _sourceGeometry.left());

                resultGeometry.setTop(_sourceGeometry.top() + mouseDelta.x() * _deltas.height());
                resultGeometry.setLeft(_sourceGeometry.left() + mouseDelta.x());
            } else {
                 if (_sourceGeometry.top() + mouseDelta.y() < 0)
                    mouseDelta.setY(0 - _sourceGeometry.top());

                if (_sourceGeometry.left() + mouseDelta.y() * _deltas.width() < 0)
                    mouseDelta.setY(0 - _sourceGeometry.left() / _deltas.width());

                if (_sourceGeometry.right() - _sourceGeometry.left() - mouseDelta.y() * _deltas.width() < this->minScaledSize_)
                    mouseDelta.setY((- this->minScaledSize_ + _sourceGeometry.right() - _sourceGeometry.left()) / _deltas.width());

                resultGeometry.setTop(_sourceGeometry.top() + mouseDelta.y());
                resultGeometry.setLeft(_sourceGeometry.left() + mouseDelta.y() * _deltas.width());
            }
            break;
        case CursorPositionTopRight:
            if ((mouseDelta.x() * _deltas.height() * -1) < (mouseDelta.y())) {
                if (_sourceGeometry.top() - mouseDelta.x() * _deltas.height() < 0)
                   mouseDelta.setX(_sourceGeometry.top() /  _deltas.height());

                if (_sourceGeometry.right() + mouseDelta.x() > this->width())
                   mouseDelta.setX(this->width() - _sourceGeometry.right());

                if (_sourceGeometry.right() - _sourceGeometry.left() + mouseDelta.x() < this->minScaledSize_)
                    mouseDelta.setX(this->minScaledSize_ - _sourceGeometry.right() + _sourceGeometry.left());

                resultGeometry.setTop(_sourceGeometry.top() - mouseDelta.x() * _deltas.height());
                resultGeometry.setRight(_sourceGeometry.right() + mouseDelta.x() );
            } else {
                if (_sourceGeometry.top() + mouseDelta.y() < 0)
                    mouseDelta.setY(0 - _sourceGeometry.top());

                if (_sourceGeometry.right() - mouseDelta.y() * _deltas.width() > this->width())
                    mouseDelta.setY((0 - this->width() + _sourceGeometry.right()) / _deltas.width());

                if (_sourceGeometry.right() - _sourceGeometry.left() - mouseDelta.y() * _deltas.width() < this->minScaledSize_)
                    mouseDelta.setY((-this->minScaledSize_ + _sourceGeometry.right() - _sourceGeometry.left()) / _deltas.width());

                resultGeometry.setTop(_sourceGeometry.top() + mouseDelta.y());
                resultGeometry.setRight(_sourceGeometry.right() - mouseDelta.y() * _deltas.width());
            }
            break;
        case CursorPositionBottomLeft:
            if ((mouseDelta.x() * _deltas.height()) < (mouseDelta.y() * -1)) {

                if (_sourceGeometry.bottom() - mouseDelta.x() * _deltas.height() > this->height())
                    mouseDelta.setX((-this->height() + _sourceGeometry.bottom()) / _deltas.height());

                if (_sourceGeometry.left() + mouseDelta.x() < 0)
                    mouseDelta.setX(-_sourceGeometry.left());

                if (_sourceGeometry.right() - _sourceGeometry.left() - mouseDelta.x() < this->minScaledSize_)
                    mouseDelta.setX(-this->minScaledSize_ + _sourceGeometry.right() - _sourceGeometry.left());

                resultGeometry.setBottom(_sourceGeometry.bottom() - mouseDelta.x() * _deltas.height());
                resultGeometry.setLeft(_sourceGeometry.left() + mouseDelta.x());
            } else {

                if (_sourceGeometry.bottom() + mouseDelta.y() > this->height())
                    mouseDelta.setY(this->height() - _sourceGeometry.bottom());

                if (_sourceGeometry.left() - mouseDelta.y() * _deltas.width() < 0)
                    mouseDelta.setY(_sourceGeometry.left() / _deltas.width());

                if (_sourceGeometry.right() - _sourceGeometry.left() + mouseDelta.y() * _deltas.width() < this->minScaledSize_)
                    mouseDelta.setY((this->minScaledSize_ - _sourceGeometry.right() + _sourceGeometry.left()) / _deltas.width());

                resultGeometry.setBottom(_sourceGeometry.bottom() + mouseDelta.y());
                resultGeometry.setLeft(_sourceGeometry.left() - mouseDelta.y() * _deltas.width());
            }
            break;
        case CursorPositionBottomRight:
            if ((mouseDelta.x() * _deltas.height()) > (mouseDelta.y())) {
                if (_sourceGeometry.bottom() + mouseDelta.x() * _deltas.height() > this->height())
                    mouseDelta.setX((this->height() - _sourceGeometry.bottom() ) / _deltas.height());

                if (_sourceGeometry.right() + mouseDelta.x() > this->width())
                    mouseDelta.setX(this->width() - _sourceGeometry.right());

                if (_sourceGeometry.right() - _sourceGeometry.left() + mouseDelta.x() < this->minScaledSize_)
                    mouseDelta.setX(this->minScaledSize_ - _sourceGeometry.right() + _sourceGeometry.left());

                resultGeometry.setBottom(_sourceGeometry.bottom() + mouseDelta.x() * _deltas.height());
                resultGeometry.setRight(_sourceGeometry.right() + mouseDelta.x());
            } else {

                if (_sourceGeometry.bottom() + mouseDelta.y() > this->height())
                    mouseDelta.setY(-_sourceGeometry.bottom() + this->height());

                if (_sourceGeometry.right() + mouseDelta.y() * _deltas.width() > this->width())
                    mouseDelta.setY((this->width() - _sourceGeometry.right()) / _deltas.width());

                if (_sourceGeometry.right() - _sourceGeometry.left() + mouseDelta.y() * _deltas.width() < this->minScaledSize_)
                    mouseDelta.setY((this->minScaledSize_ - _sourceGeometry.right() + _sourceGeometry.left()) / _deltas.width());

                resultGeometry.setBottom(_sourceGeometry.bottom() + mouseDelta.y());
                resultGeometry.setRight(_sourceGeometry.right() + mouseDelta.y() * _deltas.width());
            }
            break;
        default:
            break;
        }

        return resultGeometry;
    }
}
