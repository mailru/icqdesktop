#include "stdafx.h"

#include "../../../cache/themes/themes.h"
#include "../../../core_dispatcher.h"
#include "../../../controls/TextEditEx.h"
#include "../../../fonts.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/profiling/auto_stop_watch.h"
#include "../../../utils/Text.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/PainterPath.h"

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"
#include "../ResizePixmapTask.h"

#include "ComplexMessageItem.h"
#include "FileSharingUtils.h"
#include "LinkPreviewBlockBlankLayout.h"
#include "Selection.h"
#include "Style.h"
#include "YoutubeLinkPreviewBlockLayout.h"

#include "LinkPreviewBlock.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int getLinesNumber(const TextEditEx &textControl);

    int getLinesNumber(const TextEditEx &textControl);
}

LinkPreviewBlock::LinkPreviewBlock(ComplexMessageItem *parent, const QString &uri, const bool _hasLinkInMessage)
    : GenericBlock(parent, uri, MenuFlagLinkCopyable, false)
    , Uri_(uri)
    , RequestId_(-1)
    , Title_(nullptr)
    , Annotation_(nullptr)
    , Layout_(nullptr)
    , Time_(-1)
    , PressedOverSiteLink_(false)
    , MetaDownloaded_(false)
    , ImageDownloaded_(false)
    , FaviconDownloaded_(false)
    , IsSelected_(false)
    , ActionButton_(nullptr)
    , PreviewSize_(0, 0)
    , PreloadingTickerValue_(0)
    , PreloadingTickerAnimation_(nullptr)
    , SnapMetainfoRequestId_(-1)
    , SnapId_(0)
    , TextFontSize_(-1)
    , TextOpacity_(1.0)
    , MaxPreviewWidth_(0)
    , hasLinkInMessage_(_hasLinkInMessage)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    assert(!Uri_.isEmpty());
    Layout_ = std::unique_ptr<ILinkPreviewBlockLayout>(
        new YoutubeLinkPreviewBlockLayout());;
    setLayout(Layout_->asQLayout());

    Uri_ = Utils::normalizeLink(Uri_);
}

LinkPreviewBlock::~LinkPreviewBlock()
{

}

void LinkPreviewBlock::clearSelection()
{
    if (!IsSelected_)
    {
        return;
    }

    IsSelected_ = false;

    update();
}

QPoint LinkPreviewBlock::getActionButtonLogicalCenter() const
{
    assert(ActionButton_);

    return ActionButton_->getLogicalCenter();
}

QSize LinkPreviewBlock::getActionButtonSize() const
{
    assert(hasActionButton());

    return ActionButton_->sizeHint();
}

IItemBlockLayout* LinkPreviewBlock::getBlockLayout() const
{
    if (!Layout_)
    {
        return nullptr;
    }

    return Layout_->asBlockLayout();
}

const QString& LinkPreviewBlock::getDescription() const
{
    return Meta_.getDescription();
}

QSize LinkPreviewBlock::getFaviconSizeUnscaled() const
{
    if (isInPreloadingState() || FavIcon_.isNull())
    {
        return QSize(0, 0);
    }

    return Style::Snippet::getFaviconSizeUnscaled();
}

QSize LinkPreviewBlock::getPreviewImageSize() const
{
    if (PreviewImage_.isNull())
    {
        assert(PreviewSize_.width() >= 0);
        assert(PreviewSize_.height() >= 0);

        return PreviewSize_;
    }

    const auto previewImageSize = PreviewImage_.size();

    return previewImageSize;
}

QString LinkPreviewBlock::getSelectedText(bool isFullSelect) const
{
    if (hasLinkInMessage_)
        return QString();

    if (IsSelected_)
    {
        return getSourceText();
    }

    return QString();
}

QString LinkPreviewBlock::getTextForCopy() const
{
    return (isHasLinkInMessage() ? QString() : getSourceText());
}

const QString& LinkPreviewBlock::getSiteName() const
{
    return SiteName_;
}

const QFont& LinkPreviewBlock::getSiteNameFont() const
{
    return SiteNameFont_;
}

int32_t LinkPreviewBlock::getTitleTextHeight() const
{
    assert(Title_);

    return Title_->getTextHeight();
}

bool LinkPreviewBlock::hasActionButton() const
{
    return ActionButton_;
}

bool LinkPreviewBlock::hasTitle() const
{
    return Title_;
}

void LinkPreviewBlock::hideActionButton()
{
    assert(hasActionButton());

    ActionButton_->setVisible(false);
}

bool LinkPreviewBlock::isInPreloadingState() const
{
    return (!Title_ && !Annotation_ && PreviewImage_.isNull());
}

bool LinkPreviewBlock::isSelected() const
{
    return IsSelected_;
}

void LinkPreviewBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);

    if (isVisible && (RequestId_ > 0))
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), RequestId_);
    }

    if (isVisible)
    {
        const auto isPaused = (
            PreloadingTickerAnimation_ &&
            (PreloadingTickerAnimation_->state() == QAbstractAnimation::Paused));
        if (isPaused)
        {
            PreloadingTickerAnimation_->setPaused(false);
        }
    }
    else
    {
        const auto isRunnning = (
            PreloadingTickerAnimation_ &&
            (PreloadingTickerAnimation_->state() == QAbstractAnimation::Running));
        if (isRunnning)
        {
            PreloadingTickerAnimation_->setPaused(true);
        }
    }
}

void LinkPreviewBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{}

void LinkPreviewBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType /*selection*/)
{
    if (isHasLinkInMessage())
        return;

    const QRect globalWidgetRect(
        mapToGlobal(rect().topLeft()),
        mapToGlobal(rect().bottomRight()));

    auto selectionArea(globalWidgetRect);
    selectionArea.setTop(from.y());
    selectionArea.setBottom(to.y());
    selectionArea = selectionArea.normalized();

    const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);

    const auto widgetHeight = std::max(globalWidgetRect.height(), 1);
    const auto overlappedHeight = selectionOverlap.height();
    const auto overlapRatePercents = ((overlappedHeight * 100) / widgetHeight);
    assert(overlapRatePercents >= 0);

    const auto isSelected = (overlapRatePercents > 45);

    if (isSelected != IsSelected_)
    {
        IsSelected_ = isSelected;

        update();
    }
}

void LinkPreviewBlock::setTitleGeometry(const QRect &geometry)
{
    assert(Title_);
    assert(!geometry.isEmpty());

    Title_->setGeometry(geometry);
}

void LinkPreviewBlock::setTitleWidth(const int32_t width)
{
    assert(Title_);
    assert(width > 0);

    Title_->setFixedWidth(width);
    Title_->document()->setTextWidth(width);
}

void LinkPreviewBlock::showActionButton(const QRect &pos)
{
    assert(hasActionButton());

    assert(pos.size() == ActionButton_->sizeHint());

    ActionButton_->setVisible(true);
    ActionButton_->setGeometry(pos);
}

void LinkPreviewBlock::setMaxPreviewWidth(int width)
{
    MaxPreviewWidth_ = width;
}

int LinkPreviewBlock::getMaxPreviewWidth() const
{
    return MaxPreviewWidth_;
}

void LinkPreviewBlock::setFontSize(int size)
{
    TextFontSize_ = size;
    if (Title_)
    {
        auto f = Title_->font();
        f.setPixelSize(size);
        Title_->setFont(f);
    }
    if (Annotation_)
    {
        auto f = Annotation_->font();
        f.setPixelSize(size);
        Annotation_->setFont(f);
    }
}

void LinkPreviewBlock::setTextOpacity(double opacity)
{
    TextOpacity_ = opacity;
    if (Title_)
    {
        QPalette p = Title_->palette();
        p.setColor(QPalette::Text, MessageStyle::getTextColor(TextOpacity_));
        Title_->setPalette(p);
    }
    if (Annotation_)
    {
        QPalette p = Annotation_->palette();
        p.setColor(QPalette::Text, MessageStyle::getTextColor(TextOpacity_));
        Annotation_->setPalette(p);
    }
}

void LinkPreviewBlock::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();

    const auto mousePos = event->pos();

    if (isOverSiteLink(mousePos))
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    return GenericBlock::mouseMoveEvent(event);
}

void LinkPreviewBlock::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        return GenericBlock::mousePressEvent(event);
    }

    if (isOverSiteLink(event->pos()))
    {
        PressedOverSiteLink_ = true;
    }

    return GenericBlock::mousePressEvent(event);
}

void LinkPreviewBlock::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    const auto isLeftButton = (event->button() == Qt::LeftButton);
    if (!isLeftButton)
    {
        GenericBlock::mouseReleaseEvent(event);
        return;
    }

    const auto releasedOverSiteLink = isOverSiteLink(event->pos());

    const auto pressedOverSiteLink = PressedOverSiteLink_;

    PressedOverSiteLink_ = false;

    if (pressedOverSiteLink && releasedOverSiteLink)
    {
        assert(!Uri_.isEmpty());

        const auto hasSnapId = (SnapId_ > 0);
        if (hasSnapId)
        {
            GetDispatcher()->read_snap(getSenderAimid(), SnapId_, false);
        }

        QString uin;
        if (Utils::extractUinFromIcqLink(Uri_, Out uin))
        {
            emit Utils::InterConnector::instance().profileSettingsShow(uin);
        }
        else
        {
            Utils::UrlParser parser;
            parser.process(QStringRef(&Uri_));

            if (parser.hasUrl())
            {
                auto url = parser.getUrl().url_;
                QDesktopServices::openUrl(QString::fromUtf8(url.c_str(), url.size()));
            }

        }

        return;
    }

    GenericBlock::mouseReleaseEvent(event);

    return;
}

void LinkPreviewBlock::showEvent(QShowEvent *event)
{
    GenericBlock::showEvent(event);

    // text controls creation postponing

    const auto hasTextControls = (Title_ || Annotation_);
    if (hasTextControls)
    {
        return;
    }

    const auto hasText = (!Meta_.getTitle().isEmpty() || !Meta_.getDescription().isEmpty());
    if (!hasText)
    {
        return;
    }

    const auto &blockGeometry = Layout_->asBlockLayout()->getBlockGeometry();
    assert(blockGeometry.width() > 0);

    createTextControls(blockGeometry);
}

void LinkPreviewBlock::onMenuCopyLink()
{
    assert(!Uri_.isEmpty());

    QApplication::clipboard()->setText(Uri_);
}

bool LinkPreviewBlock::drag()
{
    return Utils::dragUrl(this, PreviewImage_, Uri_);
}

void LinkPreviewBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color)
{
    if (isInPreloadingState())
    {
        drawPreloader(p);
    }
    else
    {
        drawPreview(p);

        drawFavicon(p);

        drawSiteName(p);
    }

    if (IsSelected_)
    {
        const QBrush brush(Utils::getSelectionColor());
        p.fillRect(rect(), brush);
    }

    GenericBlock::drawBlock(p, _rect, quate_color);
}

void LinkPreviewBlock::onLinkMetainfoMetaDownloaded(int64_t seq, bool success, Data::LinkMetadata meta)
{
    assert(seq > 0);

    const auto skipSeq = (seq != RequestId_);
    if (skipSeq)
    {
        return;
    }

    Meta_ = std::move(meta);

    // 1. for the better performance disconnect signals if possible and stop the animation

    assert(!MetaDownloaded_);
    MetaDownloaded_ = true;
    updateRequestId();

    assert(PreloadingTickerAnimation_);
    PreloadingTickerAnimation_->stop();

    // 2. on error replace the current block with the text one and exit

    const auto &title = Meta_.getTitle();
    const auto &description = Meta_.getDescription();

    const auto isDescriptionEmpty = (!success || (title.isEmpty() && description.isEmpty()));
    if (isDescriptionEmpty)
    {
        if (isHasLinkInMessage())
            getParentComplexMessage()->removeBlock(this);
        else
            getParentComplexMessage()->replaceBlockWithSourceText(this);

        return;
    }

    // 3. if succeed then extract valuable data from the metainfo

    ContentType_ = Meta_.getContentType();

    const auto &previewSize = Meta_.getPreviewSize();
    assert(previewSize.width() >= 0);
    assert(previewSize.height() >= 0);

    PreviewSize_ = scalePreviewSize(previewSize);

    SiteName_ = Meta_.getSiteName();
    assert(!SiteName_.isEmpty());

    // 4. make a proper layout for the content type

    assert(Layout_);
    const auto existingGeometry = Layout_->asBlockLayout()->getBlockGeometry();

    Layout_ = std::unique_ptr<ILinkPreviewBlockLayout>(
        new YoutubeLinkPreviewBlockLayout());;
    setLayout(Layout_->asQLayout());

    // 5. postpone text processing if geometry is not ready

    if (existingGeometry.isEmpty())
    {
        return;
    }

    createTextControls(existingGeometry);
}

void LinkPreviewBlock::onLinkMetainfoFaviconDownloaded(int64_t seq, bool success, QPixmap image)
{
    assert(seq > 0);

    const auto skipSeq = (seq != RequestId_);
    if (skipSeq)
    {
        return;
    }

    assert(!FaviconDownloaded_);
    FaviconDownloaded_ = true;
    updateRequestId();

    if (!success)
    {
        return;
    }

    assert(!image.isNull());

    FavIcon_ = image;

    Utils::check_pixel_ratio(FavIcon_);

    notifyBlockContentsChanged();
}

void LinkPreviewBlock::onLinkMetainfoImageDownloaded(int64_t seq, bool success, QPixmap image)
{
    assert(seq > 0);

    const auto skipSeq = (seq != RequestId_);
    if (skipSeq)
    {
        return;
    }

    assert(!ImageDownloaded_);
    ImageDownloaded_ = true;
    updateRequestId();

    if (!success)
    {
        return;
    }

    assert(!image.isNull());

    scalePreview(image);
}

void LinkPreviewBlock::onSnapMetainfoDownloaded(int64_t seq, bool success, uint64_t snap_id)
{
    assert(seq > 0);
    assert(!success || (snap_id > 0));

    if (SnapMetainfoRequestId_ != seq)
    {
        return;
    }

    SnapMetainfoRequestId_ = -1;

    if (success)
    {
        assert(SnapId_ == 0);
        SnapId_ = snap_id;
    }
}

bool LinkPreviewBlock::createDescriptionControl(const QString &description)
{
    const auto createDescription = (
        !description.isEmpty() &&
        Layout_->isAnnotationVisible());

    if (!createDescription)
    {
        return false;
    }

    auto annotationFont = Layout_->getAnnotationFont();
    if (TextFontSize_ != -1)
        annotationFont.setPixelSize(TextFontSize_);

    assert(!Annotation_);
    Annotation_ = createTextEditControl(description, annotationFont, TextOptions::PlainText);
    Annotation_->show();

    return true;
}

void LinkPreviewBlock::createTextControls(const QRect &blockGeometry)
{
    assert(!Title_);
    assert(!Annotation_);

    const auto &title = Meta_.getTitle();
    const auto &description = Meta_.getDescription();

    auto cutTitle = std::make_shared<QString>();
    auto cutDescription = std::make_shared<QString>();

    const auto hasPreview = !PreviewSize_.isEmpty();
    if (hasPreview)
    {
        Layout_->cutTextByPreferredSize(
            title,
            description,
            blockGeometry,
            Out *cutTitle,
            Out *cutDescription);
    }
    else
    {
        *cutTitle = title;
        *cutDescription = description;
    }

    QTimer::singleShot(
        0,
        Qt::CoarseTimer,
        this,
        [this, cutTitle, cutDescription]
        {
            const auto isTitleCreated = createTitleControl(*cutTitle);

            QTimer::singleShot(
                0,
                Qt::CoarseTimer,
                this,
                [this, isTitleCreated, cutDescription]
                {
                    const auto isDescriptionCreated = createDescriptionControl(*cutDescription);

                    QTimer::singleShot(
                        0,
                        Qt::CoarseTimer,
                        this,
                        [this, isTitleCreated, isDescriptionCreated]
                        {
                            const auto isContentsChanged = (isTitleCreated || isDescriptionCreated);
                            if (isContentsChanged)
                            {
                                notifyBlockContentsChanged();
                            }
                        });
                });
        });
}

bool LinkPreviewBlock::createTitleControl(const QString &title)
{
    if (title.isEmpty())
    {
        return false;
    }

    auto titleFont = Layout_->getTitleFont();
    if (TextFontSize_ != -1)
        titleFont.setPixelSize(Utils::scale_value(TextFontSize_));

    assert(!Title_);
    Title_ = createTextEditControl(title, titleFont, TextOptions::PlainText);
    Title_->show();

    return true;
}

int LinkPreviewBlock::getPreloadingTickerValue() const
{
    assert(PreloadingTickerValue_ >= 0);
    assert(PreloadingTickerValue_ <= 100);

    return PreloadingTickerValue_;
}

void LinkPreviewBlock::requestSnapMetainfo()
{
    assert(SnapMetainfoRequestId_ == -1);

    const auto fileSharingId = extractIdFromFileSharingUri(Uri_);
    if (fileSharingId.isEmpty())
    {
        return;
    }

    const auto fileSharingType = extractContentTypeFromFileSharingId(fileSharingId);

    const auto isSnap = (
        (fileSharingType == core::file_sharing_content_type::snap_gif) ||
        (fileSharingType == core::file_sharing_content_type::snap_image) ||
        (fileSharingType == core::file_sharing_content_type::snap_video));

    if (!isSnap)
    {
        return;
    }

    SnapMetainfoRequestId_ = GetDispatcher()->download_snap_metainfo(getChatAimid(), fileSharingId);
}

void LinkPreviewBlock::setPreloadingTickerValue(const int32_t _val)
{
    assert(_val >= 0);
    assert(_val <= 100);

    PreloadingTickerValue_ = _val;

    update();
}

void LinkPreviewBlock::connectSignals(const bool isConnected)
{
    if (isConnected)
    {
        auto connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::linkMetainfoMetaDownloaded,
            this,
            &LinkPreviewBlock::onLinkMetainfoMetaDownloaded);
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::linkMetainfoImageDownloaded,
            this,
            &LinkPreviewBlock::onLinkMetainfoImageDownloaded);
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::linkMetainfoFaviconDownloaded,
            this,
            &LinkPreviewBlock::onLinkMetainfoFaviconDownloaded);
        assert(connection);

        connection = QObject::connect(
            GetDispatcher(),
            &core_dispatcher::snapMetainfoDownloaded,
            this,
            &LinkPreviewBlock::onSnapMetainfoDownloaded);
        assert(connection);

        return;
    }

    auto disconnected = QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::linkMetainfoMetaDownloaded,
        this,
        &LinkPreviewBlock::onLinkMetainfoMetaDownloaded);
    assert(disconnected);

    disconnected = QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::linkMetainfoImageDownloaded,
        this,
        &LinkPreviewBlock::onLinkMetainfoImageDownloaded);
    assert(disconnected);

    disconnected = QObject::disconnect(
        GetDispatcher(),
        &core_dispatcher::linkMetainfoFaviconDownloaded,
        this,
        &LinkPreviewBlock::onLinkMetainfoFaviconDownloaded);
    assert(disconnected);
}

TextEditEx* LinkPreviewBlock::createTextEditControl(const QString &text, const QFont &font, TextOptions options)
{
    assert(!text.isEmpty());

    blockSignals(true);
    setUpdatesEnabled(false);

    auto textControl = new TextEditEx(
        this,
        font,
        MessageStyle::getTextColor(TextOpacity_),
        false,
        false);

    textControl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    textControl->setStyle(QApplication::style());
    textControl->setFrameStyle(QFrame::NoFrame);
    textControl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setOpenLinks(false);
    textControl->setOpenExternalLinks(false);
    textControl->setWordWrapMode(QTextOption::WordWrap);
    textControl->setStyleSheet("background: transparent");
    textControl->setContextMenuPolicy(Qt::NoContextMenu);
    textControl->setReadOnly(true);
    textControl->setUndoRedoEnabled(false);

    textControl->verticalScrollBar()->blockSignals(true);

    const bool makeClickableLinks = (options == TextOptions::ClickableLinks);
    Logic::Text4Edit(text, *textControl, Logic::Text2DocHtmlMode::Escape, makeClickableLinks, true);

    textControl->document()->setDocumentMargin(0);

    textControl->viewport()->setCursor(Qt::PointingHandCursor);

    textControl->verticalScrollBar()->blockSignals(false);

    setUpdatesEnabled(true);
    blockSignals(false);

    return textControl;
}

void LinkPreviewBlock::drawFavicon(QPainter &p)
{
    if (FavIcon_.isNull())
    {
        return;
    }

    const auto &imageRect = Layout_->getFaviconImageRect();

    p.drawPixmap(imageRect, FavIcon_);
}

void LinkPreviewBlock::drawPreloader(QPainter &p)
{
    assert(isInPreloadingState());
    assert(Layout_);

    const auto &preloaderBrush = Style::Snippet::getPreloaderBrush();
    p.setBrush(preloaderBrush);

    const auto width = getBlockLayout()->getBlockGeometry().width();
    const auto brushX = ((width * getPreloadingTickerValue()) / 50);
    p.setBrushOrigin(brushX, 0);

    const auto &imageRect = Layout_->getPreviewImageRect();
    p.drawRect(imageRect);

    const auto &faviconRect = Layout_->getFaviconImageRect();
    p.drawRect(faviconRect);

    const auto &siteNameRect = Layout_->getSiteNameRect();
    p.drawRect(siteNameRect);

    const auto &titleRect = Layout_->getTitleRect();
    p.drawRect(titleRect);

    const auto &annotationRect = Layout_->getAnnotationRect();
    p.drawRect(annotationRect);
}

QPainterPath LinkPreviewBlock::evaluateClippingPath(const QRect &imageRect) const
{
    assert(!imageRect.isEmpty());

    return Utils::renderMessageBubble(imageRect, Ui::MessageStyle::getBorderRadius(), false);
}


void LinkPreviewBlock::drawPreview(QPainter &p)
{
    const auto &imageRect = Layout_->getPreviewImageRect();

    if (Style::isBlocksGridEnabled())
    {
        p.save();

        p.setPen(Qt::magenta);
        p.drawRect(imageRect);

        p.restore();
    }

    if (PreviewImage_.isNull())
    {
        return;
    }

    const auto updateClippingPath = (previewClippingPathRect_ != imageRect);

    if (updateClippingPath)
    {
        previewClippingPathRect_ = imageRect;
        previewClippingPath_ = evaluateClippingPath(imageRect);
    }

    p.save();

    p.setClipPath(previewClippingPath_);

    p.drawPixmap(imageRect, PreviewImage_);

    p.restore();
}

void LinkPreviewBlock::drawSiteName(QPainter &p)
{
    if (SiteName_.isEmpty())
    {
        return;
    }

    p.save();

    p.setPen(Style::Snippet::getSiteNameColor());
    p.setFont(getSiteNameFont());

    const auto &siteNamePos = Layout_->getSiteNamePos();

    p.drawText(siteNamePos, SiteName_);

    p.restore();
}

void LinkPreviewBlock::initialize()
{
    GenericBlock::initialize();

    connectSignals(true);

    assert(RequestId_ == -1);
    RequestId_ = GetDispatcher()->downloadLinkMetainfo(
        getChatAimid(),
        Uri_,
        0,
        0);

    SiteNameFont_ = Style::Snippet::getSiteNameFont();

    assert(!PreloadingTickerAnimation_);
    PreloadingTickerAnimation_ = new QPropertyAnimation(this, "PreloadingTicker");

    PreloadingTickerAnimation_->setDuration(4000);
    PreloadingTickerAnimation_->setLoopCount(-1);
    PreloadingTickerAnimation_->setStartValue(0);
    PreloadingTickerAnimation_->setEndValue(100);

    PreloadingTickerAnimation_->start();

    requestSnapMetainfo();
}

void LinkPreviewBlock::initializeActionButton()
{
    assert(!PreviewImage_.isNull());
    assert(!ActionButton_);
    assert(!ContentType_.isEmpty());

    const auto isVideo = (ContentType_ == "article-video");
    const auto isGif = (ContentType_ == "article-gif");

    const auto isPlayablePreview = (isVideo || isGif);
    if (!isPlayablePreview)
    {
        return;
    }

    const auto &resourcesSet = (
        isGif ?
            ActionButtonWidget::ResourceSet::Gif_ :
            ActionButtonWidget::ResourceSet::Play_);

    ActionButton_ = new ActionButtonWidget(resourcesSet, this);

    QObject::connect(
        ActionButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        (QPoint)
        {
            QDesktopServices::openUrl(Uri_);
        });
}

bool LinkPreviewBlock::isOverSiteLink(const QPoint p) const
{
    return rect().contains(p);
}

bool LinkPreviewBlock::isTitleClickable() const
{
    return (Title_ && PreviewImage_.isNull());
}

void LinkPreviewBlock::scalePreview(QPixmap &image)
{
    assert(PreviewImage_.isNull());
    assert(!image.isNull());

    const auto maxSize = Layout_->getMaxPreviewSize();
    assert(maxSize.width() > 0);

    const auto previewSize = image.size();
    const auto scaledPreviewSize = scalePreviewSize(previewSize);

    const auto shouldScalePreviewDown = (previewSize != scaledPreviewSize);
    if (!shouldScalePreviewDown)
    {
        PreviewImage_ = image;

        Utils::check_pixel_ratio(PreviewImage_);

        initializeActionButton();

        notifyBlockContentsChanged();

        return;
    }

    auto task = new HistoryControl::ResizePixmapTask(image, scaledPreviewSize);

    const auto succeed = QObject::connect(
        task,
        &HistoryControl::ResizePixmapTask::resizedSignal,
        this,
        [this](QPixmap scaled)
        {
            PreviewImage_ = scaled;

            Utils::check_pixel_ratio(PreviewImage_);

            initializeActionButton();

            notifyBlockContentsChanged();
        });
    assert(succeed);

    QThreadPool::globalInstance()->start(task);
}

QSize LinkPreviewBlock::scalePreviewSize(const QSize &size) const
{
    assert(Layout_);

    const auto maxSize = Layout_->getMaxPreviewSize();
    assert(maxSize.width() > 0);

    auto result(size);

    const auto maxWidth = maxSize.width();
    assert(maxWidth > 0);

    const auto needScaleDownWidth = (result.width() > maxWidth);
    if (needScaleDownWidth)
    {
        const auto scaleDownK = ((double)maxWidth / (double)(result.width() + 1));
        result *= scaleDownK;
    }

    const auto maxHeight = maxSize.height();

    const auto needScaleDownHeight = (result.height() > maxHeight);
    if (needScaleDownHeight)
    {
        const auto scaleDownK = ((double)maxHeight / (double)(result.height() + 1));
        result *= scaleDownK;
    }

    return result;
}

void LinkPreviewBlock::updateRequestId()
{
    const auto pendingDataExists = (!MetaDownloaded_ || !ImageDownloaded_ || !FaviconDownloaded_);
    if (pendingDataExists)
    {
        return;
    }

    RequestId_ = -1;

    connectSignals(false);
}

void LinkPreviewBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    if (Annotation_ && hover)
    {
        Annotation_->installEventFilter(Annotation_);
        Annotation_->setMouseTracking(true);
    }
    if (Title_ && hover)
    {
        Title_->installEventFilter(Annotation_);
        Title_->setMouseTracking(true);
    }
    if (hover)
    {
        installEventFilter(hover);
        raise();
    }
}

void LinkPreviewBlock::setQuoteSelection()
{
    GenericBlock::setQuoteSelection();
    emit setQuoteAnimation();
}

bool LinkPreviewBlock::isHasLinkInMessage() const
{
    return hasLinkInMessage_;
}

int LinkPreviewBlock::getMaxWidth() const
{
    return MessageStyle::getSnippetMaxWidth();
}
UI_COMPLEX_MESSAGE_NS_END
