#include "stdafx.h"

#include "../../../core_dispatcher.h"
#include "../../../main_window/MainWindow.h"
#include "../../../main_window/contact_list/ContactListModel.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../gui_settings.h"
#include "../../../controls/TextEditEx.h"
#include "../../../theme_settings.h"
#include "../../../cache/themes/themes.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../KnownFileTypes.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlockLayout.h"
#include "Selection.h"
#include "Style.h"

#include "ImagePreviewBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

const QString SELECTION_STYLE = QString("background: transparent; selection-background-color: %1;").arg(Utils::rgbaStringFromColor(Utils::getSelectionColor()));

ImagePreviewBlock::ImagePreviewBlock(ComplexMessageItem *parent, const QString& aimId, const QString &imageUri, const QString &imageType)
    : GenericBlock(
          parent,
          imageUri,
          (MenuFlags)(MenuFlagFileCopyable | MenuFlagLinkCopyable | MenuFlagOpenInBrowser),
          true)
    , AimId_(aimId)
    , ImageUri_(imageUri)
    , ImageType_(imageType)
    , Layout_(nullptr)
    , PreviewDownloadSeq_(-1)
    , PressedOverPreview_(false)
    , FullImageDownloadSeq_(-1)
    , IsSelected_(false)
    , ActionButton_(nullptr)
    , CopyFile_(false)
    , IsGifPlaying_(false)
    , FileSize_(-1)
    , SnapId_(0)
    , SnapMetainfoRequestId_(-1)
    , MaxPreviewWidth_(0)
    , IsVisible_(false)
    , IsInPreloadDistance_(true)
    , ref_(new bool(false))
    , TextOpacity_(1.0)
    , TextFontSize_(-1)
    , Selection_(BlockSelectionType::None)
{
    assert(!ImageUri_.isEmpty());
    assert(!ImageType_.isEmpty());

    ImageUri_ = Utils::normalizeLink(ImageUri_);

    Layout_ = new ImagePreviewBlockLayout();
    setLayout(Layout_);

    QuoteAnimation_.setSemiTransparent();

    link_ = createTextEditControl(imageUri);
    link_->show();
}

ImagePreviewBlock::~ImagePreviewBlock()
{
}

TextEditEx* ImagePreviewBlock::createTextEditControl(const QString& _text)
{
    assert(!_text.isEmpty());

    blockSignals(true);
    setUpdatesEnabled(false);

    QPalette p;
    p.setColor(QPalette::Text, MessageStyle::getTextColor(TextOpacity_));

    auto textControl = new Ui::TextEditEx(
        this,
        MessageStyle::getTextFont(TextFontSize_),
        p,
        false,
        false);

    textControl->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());

    textControl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    textControl->setStyle(QApplication::style());
    textControl->setFrameStyle(QFrame::NoFrame);
    textControl->setFocusPolicy(Qt::NoFocus);
    textControl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setOpenLinks(true);
    textControl->setOpenExternalLinks(true);
    textControl->setWordWrapMode(QTextOption::WordWrap);
    textControl->setStyleSheet("background: transparent");
    textControl->setContextMenuPolicy(Qt::NoContextMenu);
    textControl->setReadOnly(true);
    textControl->setUndoRedoEnabled(false);

    setTextEditTheme(textControl);

    textControl->verticalScrollBar()->blockSignals(true);

    Logic::Text4Edit(ImageUri_, *textControl, Logic::Text2DocHtmlMode::Escape, true, true);

    textControl->document()->setDocumentMargin(0);

    textControl->verticalScrollBar()->blockSignals(false);

    setUpdatesEnabled(true);
    blockSignals(false);

    return textControl;
}

void ImagePreviewBlock::setTextEditTheme(TextEditEx *textControl)
{
    assert(textControl);

    Utils::ApplyStyle(textControl, SELECTION_STYLE);
    auto textColor = MessageStyle::getTextColor(TextOpacity_);
    textColor.setAlpha(TextOpacity_ * 255);

    QPalette palette = textControl->palette();
    palette.setColor(QPalette::Text, textColor);
    textControl->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());
    textControl->setPalette(palette);
}


void ImagePreviewBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    assert(selection > BlockSelectionType::Min);
    assert(selection < BlockSelectionType::Max);
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);
    
    if (!link_)
    {
        return;
    }

    Selection_ = selection;

    const auto selectAll = (selection == BlockSelectionType::Full);
    if (selectAll)
    {
        link_->selectWholeText();
        return;
    }

    const auto selectFromBeginning = (selection == BlockSelectionType::FromBeginning);
    if (selectFromBeginning)
    {
        link_->selectFromBeginning(to);
        return;
    }

    const auto selectTillEnd = (selection == BlockSelectionType::TillEnd);
    if (selectTillEnd)
    {
        link_->selectTillEnd(from);
        return;
    }

    link_->selectByPos(from, to);
}

void ImagePreviewBlock::clearSelection()
{
//     if (!IsSelected_)
//     {
//         return;
//     }

    link_->clearSelection();

    Selection_ = BlockSelectionType::None;

//    IsSelected_ = false;

//    update();
}

bool ImagePreviewBlock::hasActionButton() const
{
    return ActionButton_;
}

QPoint ImagePreviewBlock::getActionButtonLogicalCenter() const
{
    assert(ActionButton_);

    return ActionButton_->getLogicalCenter();
}

QSize ImagePreviewBlock::getActionButtonSize() const
{
    assert(ActionButton_);

    return ActionButton_->sizeHint();
}

IItemBlockLayout* ImagePreviewBlock::getBlockLayout() const
{
    return Layout_;
}

QSize ImagePreviewBlock::getPreviewSize() const
{
    assert (!Preview_.isNull());

    return Preview_.size();
}

QString ImagePreviewBlock::getSelectedText(bool isFullSelect) const
{
    assert(!ImageUri_.isEmpty());

    switch (Selection_)
    {
    case BlockSelectionType::Full:
        return ImageUri_;

    case BlockSelectionType::TillEnd:
        return (link_->selection()/* + TrailingSpaces_*/);
    }

    return link_->selection();
}

void ImagePreviewBlock::hideActionButton()
{
    assert(ActionButton_);

    ActionButton_->setVisible(false);
}

bool ImagePreviewBlock::hasPreview() const
{
    return !Preview_.isNull();
}

bool ImagePreviewBlock::isBubbleRequired() const
{
    return false;
}

bool ImagePreviewBlock::isSelected() const
{
    return IsSelected_;
}

void ImagePreviewBlock::onVisibilityChanged(const bool isVisible)
{
    qDebug() << "ImagePreviewBlock::onVisibilityChanged(const bool isVisible =" << isVisible << ")";

    IsVisible_ = isVisible;

    GenericBlock::onVisibilityChanged(isVisible);

    if (isVisible && (PreviewDownloadSeq_ > 0))
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewDownloadSeq_);
    }

    if (isGifPreview())
    {
        qDebug() << "onGifVisibilityChanged";

        onGifVisibilityChanged(isVisible);
    }
}

bool ImagePreviewBlock::isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);

    if (intersected.height() != 0)
        return true;

    return std::min(abs(_viewportVisibilityAbsRect.y() - _widgetAbsGeometry.y())
        , abs(_viewportVisibilityAbsRect.bottom() - _widgetAbsGeometry.bottom())) < 1000;
}

void ImagePreviewBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    if (IsInPreloadDistance_ == isInPreload)
    {
        return;
    }
    IsInPreloadDistance_ = isInPreload;

    GenericBlock::onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);

    if (isGifPreview())
    {
        onChangeLoadState(IsInPreloadDistance_);
    }
}

void ImagePreviewBlock::onChangeLoadState(const bool _isLoad)
{
    assert(isGifPreview());

    if (!videoPlayer_)
        return;

    videoPlayer_->setLoadingState(_isLoad);
}

void ImagePreviewBlock::setMaxPreviewWidth(int width)
{
    MaxPreviewWidth_ = width;
}

int ImagePreviewBlock::getMaxPreviewWidth() const
{
    return MaxPreviewWidth_;
}

void ImagePreviewBlock::showActionButton(const QRect &pos)
{
    assert(hasActionButton());

    if (!pos.isEmpty())
    {
        assert(pos.size() == ActionButton_->sizeHint());
        ActionButton_->setGeometry(pos);
    }

    const auto isActionButtonVisible = (!isGifPreview() || !isGifPlaying());
    if (isActionButtonVisible)
    {
        ActionButton_->setVisible(true);
        ActionButton_->raise();
    }
}

void ImagePreviewBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quote_color)
{
    const bool isSingleBlock = isSingle();

    const auto& imageRect = Layout_->getPreviewRect();
    const auto& blockRect = Layout_->getBlockRect();
    const auto clipRect = (isSingleBlock ? blockRect : imageRect);

    const auto updateClippingPath = (PreviewClippingPathRect_ != clipRect);
    if (updateClippingPath)
    {
        PreviewClippingPathRect_ = clipRect;
        PreviewClippingPath_= evaluateClippingPath(clipRect);

        auto relativePreviewRect = QRect(0, 0, clipRect.width(), clipRect.height());
        RelativePreviewClippingPath_= evaluateClippingPath(relativePreviewRect);
    }

    p.setClipPath(PreviewClippingPath_);

    if (isSingleBlock)
        p.fillRect(blockRect, MessageStyle::getBodyBrush(isOutgoing(), false, get_qt_theme_settings()->themeForContact(getParentComplexMessage()->getAimid())->get_id()));

    if (Preview_.isNull())
    {
        drawEmptyBubble(p, imageRect);
    }
    else
    {
        if (!videoPlayer_)
        {
            p.drawPixmap(imageRect, Preview_);
        }
        else
        {
            if (!videoPlayer_->isFullScreen())
            {
                videoPlayer_->setClippingPath(RelativePreviewClippingPath_);

                if (videoPlayer_->state() == QMovie::MovieState::Paused && videoPlayer_->isGif())
                {
                    p.drawPixmap(imageRect, videoPlayer_->getActiveImage());
                }
            }
        }

    }

    const auto isDownloading = (FullImageDownloadSeq_ != -1);
    if (isDownloading)
    {
        p.fillRect(imageRect, Style::Preview::getImageShadeBrush());
    }

    if (IsSelected_)
    {
        const QBrush brush(Utils::getSelectionColor());
        p.fillRect(imageRect, brush);
    }

    if (quote_color.isValid() && !videoPlayer_)
    {
        p.fillRect(imageRect, QBrush(quote_color));
    }

    GenericBlock::drawBlock(p, _rect, quote_color);
}

void ImagePreviewBlock::drawEmptyBubble(QPainter &p, const QRect &bubbleRect)
{
    auto bodyBrush = Style::Preview::getImagePlaceholderBrush();

    p.setBrush(bodyBrush);

    p.drawRoundedRect(
        bubbleRect,
        MessageStyle::getBorderRadius(),
        MessageStyle::getBorderRadius());
}

void ImagePreviewBlock::initialize()
{
    GenericBlock::initialize();

    connectSignals();

    requestSnapMetainfo();

    assert(PreviewDownloadSeq_ == -1);
    PreviewDownloadSeq_ = Ui::GetDispatcher()->downloadImage(
        ImageUri_,
        getChatAimid(),
        QString(),
        true,
        Style::Preview::getImageWidthMax(),
        0);
}

void ImagePreviewBlock::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();

    const auto mousePos = event->pos();

    if (isOverImage(mousePos))
    {
        setCursor(Qt::PointingHandCursor);

        return GenericBlock::mouseMoveEvent(event);
    }

    setCursor(Qt::ArrowCursor);

    return GenericBlock::mouseMoveEvent(event);
}

void ImagePreviewBlock::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return;
    }

    if (isOverImage(event->pos()))
    {
        event->accept();
        PressedOverPreview_ = true;
    }

    return GenericBlock::mousePressEvent(event);
}

void ImagePreviewBlock::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return;
    }

    const auto pressedOverPreview = PressedOverPreview_;
    PressedOverPreview_ = false;

    if (!pressedOverPreview)
    {
        return;
    }

    const auto releasedOverImage = isOverImage(event->pos());
    if (!releasedOverImage)
    {
        return;
    }

    if (isFullImageDownloading())
    {
        return;
    }

    event->accept();

    onLeftMouseClick(event->globalPos());

    /// lock clicked in quote
    clickHandled();

    return GenericBlock::mouseReleaseEvent(event);
}

void ImagePreviewBlock::onMenuCopyFile()
{
    CopyFile_ = true;

    if (isFullImageDownloading())
    {
        return;
    }

    QUrl urlParser(ImageUri_);

    auto filename = urlParser.fileName();
    if (filename.isEmpty())
    {
        static const QRegularExpression re("[{}-]");

        filename = QUuid::createUuid().toString();
        filename.remove(re);
    }

    const auto dstPath = (QDir::temp().absolutePath() + "/" + filename);
    downloadFullImage(dstPath);
}

void ImagePreviewBlock::onMenuCopyLink()
{
    assert(!ImageUri_.isEmpty());

    QApplication::clipboard()->setText(ImageUri_);
}

void ImagePreviewBlock::onMenuSaveFileAs()
{
    assert(!ImageUri_.isEmpty());

    QUrl urlParser(ImageUri_);

    std::weak_ptr<bool> wr_ref = ref_;

    Utils::saveAs(urlParser.fileName(), [this, wr_ref](const QString& file, const QString& dir_result)
    {
        auto ref = wr_ref.lock();
        if (!ref)
            return;

        QString dir = dir_result;
        const auto addTrailingSlash = (!dir.endsWith('\\') && !dir.endsWith('/'));
        if (addTrailingSlash)
        {
            dir += "/";
        }
        dir += file;
        QFile::remove(dir);

        if (isFullImageDownloading())
        {
            SaveAs_ = dir;
            return;
        }

        downloadFullImage(dir);
    });
}

void ImagePreviewBlock::onMenuOpenInBrowser()
{
    QDesktopServices::openUrl(ImageUri_);
}

void ImagePreviewBlock::onRestoreResources()
{
    if (isGifPreview() && isFullImageDownloaded())
    {
        __TRACE(
            "resman",
            "restoring gif fs block\n"
            __LOGP(local, FullImageLocalPath_)
            __LOGP(uri, ImageUri_));

        playGif(FullImageLocalPath_, false);
    }

    GenericBlock::onRestoreResources();
}

void ImagePreviewBlock::onUnloadResources()
{
    if (videoPlayer_)
    {
        __TRACE(
            "resman",
            "unloading gif image\n"
            __LOGP(local, FullImageLocalPath_)
            __LOGP(uri, ImageUri_));

        videoPlayer_.reset();

        setGifPlaying(false);
    }

    GenericBlock::onUnloadResources();
}

void ImagePreviewBlock::showEvent(QShowEvent*)
{
    qDebug() << "ImagePreviewBlock::showEvent(QShowEvent*)";

    if (isGifPreview() && isFullImageDownloaded() && ((!videoPlayer_) || (videoPlayer_ && !videoPlayer_->isPausedByUser())))
    {
        playGif(FullImageLocalPath_, false);
    }
}

bool ImagePreviewBlock::drag()
{
    return Utils::dragUrl(this, Preview_, DownloadUri_);
}

void ImagePreviewBlock::connectSignals()
{
    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloaded,
        this,
        &ImagePreviewBlock::onImageDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloadingProgress,
        this,
        &ImagePreviewBlock::onImageDownloadingProgress,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageMetaDownloaded,
        this,
        &ImagePreviewBlock::onImageMetaDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::imageDownloadError,
        this,
        &ImagePreviewBlock::onImageDownloadError,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        GetDispatcher(),
        &core_dispatcher::snapMetainfoDownloaded,
        this,
        &ImagePreviewBlock::onSnapMetainfoDownloaded,
        (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
}

void ImagePreviewBlock::downloadFullImage(const QString &destination)
{
    assert(FullImageDownloadSeq_ == -1);

    const auto &imageUri = (DownloadUri_.isEmpty() ? ImageUri_ : DownloadUri_);
    assert(!imageUri.isEmpty());

    FullImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(imageUri, getChatAimid(), destination, false, 0, 0);

    if (!shouldDisplayProgressAnimation())
    {
        return;
    }

    if (ActionButton_)
    {
        ActionButton_->startAnimation();
    }

    notifyBlockContentsChanged();
}

QPainterPath ImagePreviewBlock::evaluateClippingPath(const QRect &imageRect) const
{
    assert(!imageRect.isEmpty());

    return Utils::renderMessageBubble(imageRect, Ui::MessageStyle::getBorderRadius(), false);
}

void ImagePreviewBlock::initializeActionButton()
{
    assert(!Preview_.isNull());
    assert(!ImageType_.isEmpty());

    if (ActionButton_)
    {
        return;
    }

    const auto &resourceSet =
        [&]
        {
            if (isGifPreview())
            {
                return ActionButtonWidget::ResourceSet::Gif_;
            }

            if (isVideoPreview())
            {
                return ActionButtonWidget::ResourceSet::Play_;
            }

            return ActionButtonWidget::ResourceSet::DownloadMediaFile_;
        }();

    __TRACE(
        "gif_preview",
        "initialized action button\n"
        "    uri=<" << ImageUri_ << ">\n"
        "    image_type=<" << ImageType_ << ">\n"
        "    is_gif=<" << logutils::yn(isGifPreview()) << ">\n"
        "    is_video=<" << logutils::yn(isVideoPreview()) << ">");

    ActionButton_ = new ActionButtonWidget(resourceSet, this);

    QObject::connect(
        ActionButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            schedulePreviewerOpening(globalClickCoords);

            if (isGifPreview())
            {
                onGifLeftMouseClick();
                return;
            }

            if (isFullImageDownloading())
            {
                return;
            }

            downloadFullImage(QString());
        });

    QObject::connect(
        ActionButton_,
        &ActionButtonWidget::stopClickedSignal,
        this,
        [this]
        (QPoint globalClickCoords)
        {
            Q_UNUSED(globalClickCoords);

            const auto isFullImageDownloading = (FullImageDownloadSeq_ != -1);
            if (isFullImageDownloading)
            {
                GetDispatcher()->cancelImageDownloading(ImageUri_);
                FullImageDownloadSeq_ = -1;
            }

            ActionButton_->stopAnimation();

            notifyBlockContentsChanged();
        });
}

bool ImagePreviewBlock::isFullImageDownloaded() const
{
    return !FullImageLocalPath_.isEmpty();
}

bool ImagePreviewBlock::isFullImageDownloading() const
{
    assert(FullImageDownloadSeq_ >= -1);

    return (FullImageDownloadSeq_ > 0);
}

bool ImagePreviewBlock::isGifPlaying() const
{
    assert(isGifPreview());

    qDebug() << "isGifPlaying = " << IsGifPlaying_;

    return IsGifPlaying_;
}

void ImagePreviewBlock::setGifPlaying(const bool _playing)
{
    qDebug() << "setGifPlaying = " << _playing;

    IsGifPlaying_ = _playing;
}

bool ImagePreviewBlock::isGifPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    return ((ImageType_ == "gif") || (ImageType_ == "snap/gif"));
}

bool ImagePreviewBlock::isOverImage(const QPoint &pos) const
{
    if (Preview_.isNull())
    {
        return false;
    }

    const auto &previewRect = Layout_->getPreviewRect();

    return  previewRect.contains(pos);
}

bool ImagePreviewBlock::isVideoPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    if (ImageType_ == "snap/video")
    {
        return true;
    }

    return Utils::is_video_extension(ImageType_);
}

void ImagePreviewBlock::onFullImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!image.isNull());
    assert(!localPath.isEmpty());

    FullImageLocalPath_ = localPath;

    stopDownloadingAnimation();

    if (!QFile::exists(localPath))
    {
        assert(!"file is missing");
        return;
    }

    if (!OpenPreviewer_.isNull())
    {
        openPreviewer(image, FullImageLocalPath_);

        OpenPreviewer_ = QPoint();
    }

    if (!SaveAs_.isEmpty())
    {
        QFile::copy(localPath, SaveAs_);
        SaveAs_ = QString();
    }

    if (CopyFile_)
    {
        CopyFile_ = false;
        Utils::copyFileToClipboard(localPath);
    }

    if (isGifPreview())
    {
        playGif(localPath, false);
    }
}

void ImagePreviewBlock::onGifLeftMouseClick()
{
    assert(isGifPreview());

    if (!isFullImageDownloaded())
    {
        downloadFullImage(QString());
        return;
    }

    if (isGifPlaying())
    {
        pauseGif(true);

        return;
    }

    playGif(FullImageLocalPath_, true);
}

void ImagePreviewBlock::onGifPlaybackStatusChanged(const bool isPlaying)
{
    if (isPlaying && ActionButton_)
    {
        hideActionButton();
    }

    if (!isPlaying && ActionButton_)
    {
        showActionButton(QRect());
    }
}

void ImagePreviewBlock::onGifVisibilityChanged(const bool isVisible)
{
    assert(isGifPreview());

    if (isVisible)
    {
        if (isFullImageDownloaded() && ((!videoPlayer_) || (videoPlayer_ && !videoPlayer_->isPausedByUser())))
        {
            qDebug() << "call playGif " << FullImageLocalPath_;

            playGif(FullImageLocalPath_, false);
        }

        return;
    }

    if (isGifPlaying())
    {
        pauseGif(false);
    }
}

void ImagePreviewBlock::onLeftMouseClick(const QPoint &globalPos)
{
    schedulePreviewerOpening(globalPos);

    const auto hasSnapId = (SnapId_ > 0);
    if (hasSnapId)
    {
        GetDispatcher()->read_snap(getSenderAimid(), SnapId_, false);
    }

    if (isGifPreview())
    {
        onGifLeftMouseClick();
        return;
    }

    downloadFullImage(QString());
}

void ImagePreviewBlock::onPreviewImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!image.isNull());
    assert(Preview_.isNull());

    PreviewLocalPath_ = localPath;

    Preview_ = image;

    Utils::check_pixel_ratio(Preview_);

    initializeActionButton();

    notifyBlockContentsChanged();

    preloadFullImageIfNeeded();
}

void ImagePreviewBlock::openPreviewer(QPixmap /*image*/, const QString &localPath)
{
    assert(!localPath.isEmpty());
    assert(!isGifPreview());

    Utils::InterConnector::instance().getMainWindow()->openGallery(AimId_, Data::Image(getId(), ImageUri_, false), localPath);
}

void ImagePreviewBlock::playGif(const QString &localPath, const bool _byUser)
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (!mainWindow || !mainWindow->isActive() || Logic::getContactListModel()->selectedContact() != AimId_)
        return;

    assert(isGifPreview());
    assert(!localPath.isEmpty());

    const auto isFileExists = QFile::exists(localPath);
    //assert(isFileExists);

    if (isGifPlaying() || !isFileExists)
    {
        return;
    }

    if (videoPlayer_)
    {
        const auto &previewRect = Layout_->getPreviewRect();
        if (previewRect.isEmpty())
        {
            return;
        }

        if (!_byUser && videoPlayer_->isPausedByUser())
        {
            return;
        }


        onGifPlaybackStatusChanged(isGifPlaying());

        const auto gifSize = Utils::unscale_value(previewRect.size());

        const bool play = IsVisible_;

        setGifPlaying(play);
        
        videoPlayer_->start(play);

        if (play)
        {
            videoPlayer_->raise();
        }
        return;
    }

    load_task_ = std::unique_ptr<Utils::LoadMovieToFFMpegPlayerFromFileTask>(new Utils::LoadMovieToFFMpegPlayerFromFileTask(localPath, isGifPreview(), this));

    QObject::connect(
        load_task_.get(),
        &Utils::LoadMovieToFFMpegPlayerFromFileTask::loadedSignal,
        this,
        [this]
        (QSharedPointer<Ui::DialogPlayer> movie)
        {
            if (!IsInPreloadDistance_)
                return;

            assert(movie);

            movie->setPreview(Preview_);
            videoPlayer_ = movie;

            QObject::connect(
                videoPlayer_.data(),
                &Ui::DialogPlayer::paused,
                this,
                &ImagePreviewBlock::onPaused,
                (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

            bool mute = true;
            int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

            const auto &imageRect = Layout_->getPreviewRect();
            
            videoPlayer_->setParent(this);
            videoPlayer_->start(IsVisible_);

            videoPlayer_->updateSize(imageRect);

            videoPlayer_->setVolume(volume);
            videoPlayer_->setMute(mute);

            videoPlayer_->show();

            setGifPlaying(IsVisible_);

            update();
        }
    );

    load_task_->run();

    // playGif(localPath);
}

void ImagePreviewBlock::preloadFullImageIfNeeded()
{
    if (!isGifPreview())
    {
        return;
    }

    if (isFullImageDownloading())
    {
        return;
    }

    if (isFullImageDownloaded() && !isGifPlaying())
    {
        playGif(FullImageLocalPath_, false);
        return;
    }

    downloadFullImage(QString());
}

void ImagePreviewBlock::requestSnapMetainfo()
{
    assert(SnapMetainfoRequestId_ == -1);

    const auto fileSharingId = extractIdFromFileSharingUri(ImageUri_);
    if (fileSharingId.isEmpty())
    {
        return;
    }

    const auto fileSharingType = extractContentTypeFromFileSharingId(fileSharingId);

    if (!is_snap_file_sharing_content_type(fileSharingType))
    {
        return;
    }

    SnapMetainfoRequestId_ = GetDispatcher()->download_snap_metainfo(getChatAimid(), fileSharingId);
}

void ImagePreviewBlock::schedulePreviewerOpening(const QPoint &globalPos)
{
    if (isGifPreview())
    {
        assert(OpenPreviewer_.isNull());
        return;
    }

    OpenPreviewer_ = globalPos;
}

bool ImagePreviewBlock::shouldDisplayProgressAnimation() const
{
    return true;
}

bool ImagePreviewBlock::isSingle() const
{
    return getParentComplexMessage()->isSimple();
}

void ImagePreviewBlock::stopDownloadingAnimation()
{
    if (isGifPreview() || isVideoPreview())
    {
        assert(ActionButton_);
        ActionButton_->stopAnimation();
    }
    else
    {
        delete ActionButton_;
        ActionButton_ = nullptr;
    }
}


void ImagePreviewBlock::pauseGif(const bool _byUser)
{
    qDebug() << "ImagePreviewBlock::pauseGif";

    assert(isGifPreview());

    if (isGifPlaying())
    {
        setGifPlaying(false);

        onGifPlaybackStatusChanged(isGifPlaying());
    }

    if (!videoPlayer_)
    {
        return;
    }

    videoPlayer_->setPaused(true, _byUser);
}

void ImagePreviewBlock::onGifFrameUpdated(int /*frameNumber*/)
{
    assert(videoPlayer_);

    //const auto frame = Player_->currentPixmap();
   // Preview_ = frame;

    //update();
}

void ImagePreviewBlock::onImageDownloadError(qint64 seq, QString rawUri)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {

        getParentComplexMessage()->replaceBlockWithSourceText(this);

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        FullImageDownloadSeq_ = -1;

        update();

        stopDownloadingAnimation();

        const auto shouldOpenPreviewer = (!OpenPreviewer_.isNull() && !PreviewLocalPath_.isEmpty());
        if (shouldOpenPreviewer)
        {
            openPreviewer(Preview_, PreviewLocalPath_);

            OpenPreviewer_ = QPoint();
        }
    }
}

void ImagePreviewBlock::onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {
        assert(!image.isNull());

        onPreviewImageDownloaded(image, localPath);

        PreviewDownloadSeq_ = -1;

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        assert(!image.isNull());
        assert(!localPath.isEmpty());

        onFullImageDownloaded(image, localPath);

        FullImageDownloadSeq_ = -1;

        update();
    }
}

void ImagePreviewBlock::onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred)
{
    assert(seq > 0);
    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    if (seq != FullImageDownloadSeq_)
    {
        return;
    }

    if (!ActionButton_)
    {
        return;
    }

    const auto normalizedPercents = ((double)pctTransferred / 100.0);

    ActionButton_->setProgress(normalizedPercents);

    const auto progressText = HistoryControl::formatProgressText(bytesTotal, bytesTransferred);
    assert(!progressText.isEmpty());

    ActionButton_->setProgressText(progressText);

    notifyBlockContentsChanged();
}

void ImagePreviewBlock::onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (!isPreviewSeq)
    {
        return;
    }

    DownloadUri_ = meta.getDownloadUri();

    assert(FileSize_ == -1);
    FileSize_ = meta.getFileSize();
}

void ImagePreviewBlock::onSnapMetainfoDownloaded(int64_t _seq, bool _success, uint64_t _snap_id)
{
    assert(_seq > 0);
    assert(!_success || (_snap_id > 0));

    if (SnapMetainfoRequestId_ != _seq)
    {
        return;
    }

    if (_success)
    {
        assert(SnapId_ == 0);
        SnapId_ = _snap_id;
    }

    SnapMetainfoRequestId_ = -1;
}

void ImagePreviewBlock::onPaused()
{
    setGifPlaying(false);

    onGifPlaybackStatusChanged(isGifPlaying());

    repaint();
}

void ImagePreviewBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    connectButtonToHover(ActionButton_, hover);
    GenericBlock::connectToHover(hover);
}

void ImagePreviewBlock::resizeEvent(QResizeEvent* _event)
{
    if (videoPlayer_)
    {
        const auto &imageRect = Layout_->getPreviewRect();

        videoPlayer_->updateSize(imageRect);
    }
    GenericBlock::resizeEvent(_event);
}

void ImagePreviewBlock::setQuoteSelection()
{
    emit setQuoteAnimation();
    GenericBlock::setQuoteSelection();
}

UI_COMPLEX_MESSAGE_NS_END
