#include "stdafx.h"

#include "../utils/utils.h"

#include "ImageViewerImpl.h"
#include "../main_window/mplayer/VideoPlayer.h"
#include "../gui_settings.h"

Previewer::AbstractViewer::~AbstractViewer()
{
}

bool Previewer::AbstractViewer::canScroll() const
{
    return canScroll_;
}

QRect Previewer::AbstractViewer::rect() const
{
    return viewport_;
}

void Previewer::AbstractViewer::scale(double _newScaleFactor)
{
    const auto oldSize = imageRect_.size() * scaleFactor_;
    const auto newSize = imageRect_.size() * _newScaleFactor;

    if (newSize.width() > viewportRect_.width()
        || newSize.height() > viewportRect_.height())
    {
        viewport_ = getViewportRect(_newScaleFactor);

        const auto x = (newSize.width() > viewportRect_.width())
            ? (newSize.width() - viewportRect_.width()) / _newScaleFactor / 2
            : 0;

        const auto y = (newSize.height() > viewportRect_.height())
            ? (newSize.height() - viewportRect_.height()) / _newScaleFactor / 2
            : 0;

        fragment_ = QRect(x + offset_.x(), y + offset_.y(), 
            viewport_.width() / _newScaleFactor, viewport_.height() / _newScaleFactor);

        fixBounds(imageRect_.size(), fragment_);

        canScroll_ = true;
    }
    else
    {
        offset_ = QPoint();

        viewport_ = getViewportRect(_newScaleFactor);

        fragment_ = imageRect_;

        canScroll_ = false;
    }

    scaleFactor_ = _newScaleFactor;

    doResize(fragment_, viewport_);

    repaint();
}

void Previewer::AbstractViewer::repaint()
{
    static_cast<QWidget*>(parent())->repaint();
}

QRect Previewer::AbstractViewer::getViewportRect(double _scaleFactor) const
{
    const auto imageFragment =
        QRect(0, 0, imageRect_.width() * _scaleFactor, imageRect_.height() * _scaleFactor);

    QRect viewport = imageFragment.intersected(viewportRect_);

    const auto dx = viewportRect_.width() / 2 - imageFragment.width() / 2;
    const auto dy = viewportRect_.height() / 2 - imageFragment.height() / 2;
    const auto topLeft = QPoint(dx > 0 ? dx : 0, dy > 0 ? dy : 0);

    viewport.translate(topLeft);

    return viewport;
}

void Previewer::AbstractViewer::move(const QPoint& _offset)
{
    const auto scaledOffset = _offset / scaleFactor_;

    offset_ += scaledOffset;

    fragment_.translate(scaledOffset);

    fixBounds(imageRect_.size(), fragment_);

    repaint();
}

void Previewer::AbstractViewer::paint(QPainter& _painter)
{
    _painter.setRenderHints(QPainter::SmoothPixmapTransform);
    doPaint(_painter, fragment_, viewport_);
}

double Previewer::AbstractViewer::getPreferredScaleFactor() const
{
    return prefferedScaleFactor_;
}

double Previewer::AbstractViewer::getScaleFactor() const
{
    return scaleFactor_;
}

Previewer::AbstractViewer::AbstractViewer(const QSize& _viewportSize, QWidget* _parent)
    : QObject(_parent)
    , canScroll_(false)
    , viewportRect_(QPoint(), _viewportSize)
    , scaleFactor_(0.0)
{
}

void Previewer::AbstractViewer::init(const QRect& _imageRect)
{
    imageRect_ = _imageRect;
    prefferedScaleFactor_ = getPreferredScaleFactor(imageRect_.size());
    scaleFactor_ = prefferedScaleFactor_;
    offset_ = QPoint();
    scale(scaleFactor_);
}

void Previewer::AbstractViewer::doResize(const QRect& _source, const QRect& _target)
{
}

bool Previewer::AbstractViewer::closeFullscreen()
{
    return false;
}

bool Previewer::AbstractViewer::isZoomSupport() const
{
    return true;
}

double Previewer::AbstractViewer::getPreferredScaleFactor(const QSize& _imageSize) const
{
    const auto size = Utils::scale_value(_imageSize);

    bool needToDownscale = (viewportRect_.width() < size.width() || viewportRect_.height() < size.height());
    if (!needToDownscale)
        return 1.0 * Utils::getScaleCoefficient();

    const auto kx = size.width() / static_cast<double>(viewportRect_.width());
    const auto ky = size.height() / static_cast<double>(viewportRect_.height());

    return 1.0 / std::max(kx, ky) * Utils::getScaleCoefficient();
}

void Previewer::AbstractViewer::fixBounds(const QSize& _bounds, QRect& _child)
{
    if (_child.x() < 0)
        _child.moveLeft(0);
    else if (_child.x() + _child.width() >= _bounds.width())
        _child.moveRight(_bounds.width());

    if (_child.y() < 0)
        _child.moveTop(0);
    else if (_child.y() + _child.height() >= _bounds.height())
        _child.moveBottom(_bounds.height());
}

std::unique_ptr<Previewer::AbstractViewer> Previewer::GifViewer::create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent)
{
    auto viewer = std::unique_ptr<AbstractViewer>(new GifViewer(_fileName, _viewportSize, _parent));
    auto gifViewer = static_cast<GifViewer*>(viewer.get());
    gifViewer->init(gifViewer->gif_->frameRect());
    return viewer;
}

Previewer::GifViewer::GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent)
    : AbstractViewer(_viewportSize, _parent)
{
    gif_.reset(new QMovie(_fileName));
    connect(gif_.get(), &QMovie::frameChanged, this, &GifViewer::repaint);
    gif_->start();
}

void Previewer::GifViewer::doPaint(QPainter& _painter, const QRect& _source, const QRect& _target)
{
    auto pixmap = gif_->currentPixmap();
    Utils::check_pixel_ratio(pixmap);
    _painter.drawPixmap(_target, pixmap, _source);
}





std::unique_ptr<Previewer::AbstractViewer> Previewer::JpegPngViewer::create(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent)
{
    auto viewer = std::unique_ptr<AbstractViewer>(new JpegPngViewer(_image, _viewportSize, _parent));
    auto jpegViewer = static_cast<JpegPngViewer*>(viewer.get());
    jpegViewer->init(jpegViewer->originalImage_.rect());
    return viewer;
}

Previewer::JpegPngViewer::JpegPngViewer(const QPixmap& _image, const QSize& _viewportSize, QWidget* _parent)
    : AbstractViewer(_viewportSize, _parent)
    , originalImage_(_image)
{
    Utils::check_pixel_ratio(originalImage_);
}

void Previewer::JpegPngViewer::doPaint(QPainter& _painter, const QRect& _source, const QRect& _target)
{
    _painter.drawPixmap(_target, originalImage_, _source);
}





std::unique_ptr<Previewer::AbstractViewer> Previewer::FFMpegViewer::create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent, QPixmap _preview)
{
    auto viewer = std::unique_ptr<AbstractViewer>(new FFMpegViewer(_fileName, _viewportSize, _parent, _preview));

    return viewer;
}

Previewer::FFMpegViewer::FFMpegViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent, QPixmap _preview)
    :   AbstractViewer(_viewportSize, _parent),
        parent_(_parent),
        fullscreen_(false),
        preview_(_preview)
{
    ffplayer_.reset(new Ui::DialogPlayer(_parent, (
        Ui::DialogPlayer::Flags::enable_control_panel | 
        Ui::DialogPlayer::Flags::use_opengl |
        Ui::DialogPlayer::Flags::as_window)));

    connect(ffplayer_.get(), &Ui::DialogPlayer::loaded, this, [this]()
    {
        const auto videoSize = ffplayer_->getVideoSize();

        init(QRect(0, 0, videoSize.width(), videoSize.height()));

        ffplayer_->showAs();

        int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);
        ffplayer_->setVolume(volume);
        ffplayer_->setMute(false);
        ffplayer_->start(true);

    }, Qt::QueuedConnection);

    connect(ffplayer_.get(), &Ui::DialogPlayer::fullScreenClicked, this, [this]()
    {
        if (fullscreen_)
        {
            ffplayer_->showAsNormal();
        }
        else
        {
            ffplayer_->showAsFullscreen();
        }

        fullscreen_ = !fullscreen_;

    }, Qt::QueuedConnection);

    connect(ffplayer_.get(), &Ui::DialogPlayer::mouseWheelEvent, this, [this](const int _delta)
    {
        emit mouseWheelEvent(_delta);

    }, Qt::QueuedConnection);

    ffplayer_->openMedia(_fileName);
}

Previewer::FFMpegViewer::~FFMpegViewer()
{
}

QSize Previewer::FFMpegViewer::calculateInitialSize() const
{
    return ffplayer_->getVideoSize();
}

void Previewer::FFMpegViewer::doPaint(QPainter& _painter, const QRect& _source, const QRect& _target)
{
    //_painter.drawPixmap(_target, preview_, _source);
}

void Previewer::FFMpegViewer::doResize(const QRect& _source, const QRect& _target)
{
    const QPoint ptLeftTop = parent_->mapToGlobal(_target.topLeft());

    ffplayer_->setGeometry(QRect(ptLeftTop.x(), ptLeftTop.y(), _target.width(), _target.height()));
}

bool Previewer::FFMpegViewer::closeFullscreen()
{
    if (!fullscreen_)
        return false;

    ffplayer_->showAsNormal();

    fullscreen_ = false;

    return true;
}

bool Previewer::FFMpegViewer::isZoomSupport() const
{
    return false;
}
