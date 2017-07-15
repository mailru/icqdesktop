#include "stdafx.h"

#include "../utils/utils.h"

#include "ImageViewerImpl.h"

#include "ImageViewerWidget.h"

namespace
{
    const int maxZoomSteps = 5;
}

Previewer::ImageViewerWidget::ImageViewerWidget(QWidget* _parent)
    : QLabel(_parent)
    , zoomStep_(0)
    , parent_(_parent)
{
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

Previewer::ImageViewerWidget::~ImageViewerWidget()
{
}

void Previewer::ImageViewerWidget::showImage(const QPixmap& _preview, const QString& _fileName)
{
    assert(!_fileName.isEmpty());

    auto reader = std::unique_ptr<QImageReader>(new QImageReader(_fileName));

    QString type = reader->format();

    if (type.isEmpty()) // extension does not match the content
    {
        reader.reset(new QImageReader(_fileName));
        reader->setDecideFormatFromContent(true);
        type = reader->format();
    }

    clear();

    zoomStep_ = 0;

    setCursor(Qt::ArrowCursor);

    if (type == "jpeg" || type == "png" || type == "bmp")
    {
        if (_preview.isNull())
        {
            QPixmap pixmap;
            Utils::loadPixmap(_fileName, pixmap);
            viewer_ = JpegPngViewer::create(pixmap, size(), this);
        }
        else
            viewer_ = JpegPngViewer::create(_preview, size(), this);
    }
    else
    {
        viewer_ = FFMpegViewer::create(_fileName, size(), this, _preview);
    }

    repaint();
}

bool Previewer::ImageViewerWidget::isZoomSupport() const
{
    assert(viewer_);
    if (!viewer_)
        return false;

    return viewer_->isZoomSupport();
}

void Previewer::ImageViewerWidget::reset()
{
    viewer_.reset();
    clear();
}

void Previewer::ImageViewerWidget::connectExternalWheelEvent(std::function<void(const int)> _func)
{
    assert(viewer_);
    if (!viewer_)
        return;

    connect(viewer_.get(), &AbstractViewer::mouseWheelEvent, this, [_func](const int _delta)
    {
        _func(_delta);
    });
}

bool Previewer::ImageViewerWidget::closeFullscreen()
{
    if (!viewer_)
        return false;

    return viewer_->closeFullscreen();
}

void Previewer::ImageViewerWidget::mousePressEvent(QMouseEvent* _event)
{
    if (!viewer_ || _event->button() != Qt::LeftButton)
    {
        _event->ignore();
        return;
    }

    mousePos_ = _event->pos();
    const auto rect = viewer_->rect();
    if (rect.contains(mousePos_))
    {
        if (viewer_->canScroll())
            setCursor(Qt::ClosedHandCursor);
        else
            emit imageClicked();
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!viewer_ || _event->button() != Qt::LeftButton || !viewer_->canScroll())
    {
        _event->ignore();
        return;
    }

    const auto pos = _event->pos();
    const auto rect = viewer_->rect();
    if (rect.contains(pos))
    {
        setCursor(Qt::OpenHandCursor);
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::mouseMoveEvent(QMouseEvent* _event)
{
    if (!viewer_)
    {
        _event->ignore();
        return;
    }

    const auto pos = _event->pos();
    const auto rect = viewer_->rect();

    if (rect.contains(pos))
    {
        if (viewer_->canScroll())
        {
            setCursor(_event->buttons() & Qt::LeftButton ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
            if (_event->buttons() & Qt::LeftButton)
            {
                viewer_->move(mousePos_ - pos);
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
        _event->accept();
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        _event->ignore();
    }

    mousePos_ = pos;
}

void Previewer::ImageViewerWidget::wheelEvent(QWheelEvent* _event)
{
    if (viewer_ && _event->modifiers().testFlag(Qt::ControlModifier))
    {
        if (_event->delta() > 0)
            zoomIn();
        else
            zoomOut();

        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::paintEvent(QPaintEvent* _event)
{
    if (!viewer_)
        return;

    QPainter painter(this);
    viewer_->paint(painter);
}

void Previewer::ImageViewerWidget::zoomIn()
{
    if (!canZoomIn())
        return;

    ++zoomStep_;

    viewer_->scale(getZoomInValue(zoomStep_));
}

void Previewer::ImageViewerWidget::zoomOut()
{
    if (!canZoomOut())
        return;

    --zoomStep_;

    viewer_->scale(getZoomOutValue(zoomStep_));
}

bool Previewer::ImageViewerWidget::canZoomIn() const
{
    return zoomStep_ < maxZoomSteps;
}

bool Previewer::ImageViewerWidget::canZoomOut() const
{
    return zoomStep_ > 0 || -zoomStep_ < maxZoomSteps;
}

double Previewer::ImageViewerWidget::getScaleStep() const
{
    return 1.5;
}

double Previewer::ImageViewerWidget::getZoomInValue(int _zoomStep) const
{
    if (_zoomStep < 0)
        return getZoomOutValue(_zoomStep);

    auto scaleValue = viewer_->getPreferredScaleFactor();
    for (int i = 0; i != _zoomStep; ++i)
        scaleValue *= getScaleStep();
    return scaleValue;
}

double Previewer::ImageViewerWidget::getZoomOutValue(int _zoomStep) const
{
    if (_zoomStep > 0)
        return getZoomInValue(_zoomStep);

    auto scaleValue = viewer_->getPreferredScaleFactor();
    for (int i = 0; i != -_zoomStep; ++i)
        scaleValue /= getScaleStep();
    return scaleValue;
}
