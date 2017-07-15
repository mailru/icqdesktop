#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "LinkPreviewBlock.h"
#include "Style.h"

#include "YoutubeLinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

YoutubeLinkPreviewBlockLayout::YoutubeLinkPreviewBlockLayout()
{
}

YoutubeLinkPreviewBlockLayout::~YoutubeLinkPreviewBlockLayout()
{
}

IItemBlockLayout* YoutubeLinkPreviewBlockLayout::asBlockLayout()
{
    return this;
}

QLayout* YoutubeLinkPreviewBlockLayout::asQLayout()
{
    return this;
}

void YoutubeLinkPreviewBlockLayout::cutTextByPreferredSize(
    const QString &title,
    const QString &annotation,
    const QRect& /*widgetLtr*/,
    Out QString &cutTitle,
    Out QString &cutAnnotation) const
{
    Out cutTitle = title;
    Out cutAnnotation = annotation;
}

QFont YoutubeLinkPreviewBlockLayout::getAnnotationFont() const
{
    return QFont();
}

QRect YoutubeLinkPreviewBlockLayout::getAnnotationRect() const
{
    return QRect();
}

const IItemBlockLayout::IBoxModel& YoutubeLinkPreviewBlockLayout::getBlockBoxModel() const
{
    static const BoxModel boxModel(
        true,
        Style::getDefaultBlockBubbleMargins());

    return boxModel;
}

QSize YoutubeLinkPreviewBlockLayout::getMaxPreviewSize() const
{
    return QSize(
        Style::Preview::getImageWidthMax(),
        Style::Snippet::getLinkPreviewHeightMax());
}

QRect YoutubeLinkPreviewBlockLayout::getFaviconImageRect() const
{
    return FaviconImageRect_;
}

QRect YoutubeLinkPreviewBlockLayout::getPreviewImageRect() const
{
    return PreviewImageRect_;
}

QPoint YoutubeLinkPreviewBlockLayout::getSiteNamePos() const
{
    return SiteNameGeometry_.bottomLeft();
}

QRect YoutubeLinkPreviewBlockLayout::getSiteNameRect() const
{
    return SiteNameGeometry_;
}

QFont YoutubeLinkPreviewBlockLayout::getTitleFont() const
{
    return Style::Snippet::getYoutubeTitleFont();
}

QRect YoutubeLinkPreviewBlockLayout::getTitleRect() const
{
    return TitleGeometry_;
}

bool YoutubeLinkPreviewBlockLayout::isAnnotationVisible() const
{
    return false;
}

void YoutubeLinkPreviewBlockLayout::setActionButtonGeometry(const QRect &previewRect, LinkPreviewBlock &block)
{
    if (!block.hasActionButton())
    {
        return;
    }

    assert(!PreviewImageRect_.isEmpty());

    const auto actionButtonSize = block.getActionButtonSize();

    const auto minSizeToShowActionButton = (actionButtonSize * 2);

    const auto actionButtonVisible = (
        (previewRect.width() > minSizeToShowActionButton.width()) ||
        (previewRect.height() > minSizeToShowActionButton.height()));
    if (!actionButtonVisible)
    {
        block.hideActionButton();
        return;
    }

    QRect actionButtonRect(QPoint(0, 0), actionButtonSize);

    auto actionButtonCenter = PreviewImageRect_.center();

    const auto actionButtonCenteringOffsetY = (
        actionButtonRect.center().y() -
        block.getActionButtonLogicalCenter().y());
    actionButtonCenter.ry() += actionButtonCenteringOffsetY;

    actionButtonRect.moveCenter(actionButtonCenter);

    block.showActionButton(actionButtonRect);
}

QSize YoutubeLinkPreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    assert(geometry.height() >= 0);

    const auto enoughSpace = (geometry.width() > 0);
    if (!enoughSpace)
    {
        return QSize(0, 0);
    }

    auto &block = *blockWidget<LinkPreviewBlock>();

    const auto isPreloader = block.isInPreloadingState();

    auto previewContentWidth = std::min(
        geometry.width(),
        Style::Preview::getImageWidthMax());

    if (block.getMaxPreviewWidth() != 0)
        previewContentWidth = std::min(previewContentWidth, block.getMaxPreviewWidth());

    const QRect previewContentLtr(
        geometry.topLeft(),
        QSize(previewContentWidth, 0));

    PreviewImageRect_ = evaluatePreviewImageRect(block, previewContentLtr);

    auto titleWidth = std::min(
        geometry.width(),
        Style::Preview::getImageWidthMax());

    const QRect titleContentLtr(
        geometry.topLeft(),
        QSize(titleWidth, 0));

    const auto titleLtr = evaluateTitleLtr(titleContentLtr, PreviewImageRect_, isPreloader);

    TitleGeometry_ = setTitleGeometry(block, titleLtr);

    FaviconImageRect_ = evaluateFaviconImageRect(block, TitleGeometry_);

    SiteNameGeometry_ = evaluateSiteNameGeometry(block, TitleGeometry_, FaviconImageRect_);

    const auto blockHeight = evaluateWidgetHeight(
        PreviewImageRect_,
        FaviconImageRect_,
        SiteNameGeometry_,
        isPreloader);
    assert(blockHeight >= MessageStyle::getMinBubbleHeight());

    setActionButtonGeometry(PreviewImageRect_, block);

    const QSize blockSize(geometry.width(), blockHeight);

    return blockSize;
}

QRect YoutubeLinkPreviewBlockLayout::evaluateFaviconImageRect(const LinkPreviewBlock &block, const QRect &titleGeometry) const
{
    QRect faviconGeometry(
        titleGeometry.left(),
        titleGeometry.bottom() + 1,
        0,
        0);

    const auto hasTextAbove = !titleGeometry.isEmpty();
    if (hasTextAbove)
    {
        faviconGeometry.translate(0, Style::Snippet::getFaviconTopPadding());
    }

    const auto faviconSize =
        block.isInPreloadingState() ?
            Style::Snippet::getFaviconPlaceholderSize() :
            block.getFaviconSizeUnscaled();

    faviconGeometry.setSize(faviconSize);

    return Utils::scale_value(faviconGeometry);
}

QRect YoutubeLinkPreviewBlockLayout::evaluatePreviewImageRect(const LinkPreviewBlock &block, const QRect &previewContentLtr) const
{
    assert(previewContentLtr.isEmpty());

    auto previewImageSize = (
        block.isInPreloadingState() ?
        Style::Snippet::getImagePreloaderSizeDip() :
            block.getPreviewImageSize());

    if (previewImageSize.isEmpty())
    {
        return QRect(
            previewContentLtr.topLeft(),
            QSize(0, 0));
    }

    previewImageSize = Utils::scale_value(previewImageSize);

    const QSize maxSize(
        previewContentLtr.width(),
        Style::Snippet::getLinkPreviewHeightMax());

    previewImageSize = limitSize(previewImageSize, maxSize);

    const QRect previewImageRect(
        previewContentLtr.topLeft(),
        previewImageSize);
    assert(!previewImageRect.isEmpty());

    return previewImageRect;
}

QRect YoutubeLinkPreviewBlockLayout::evaluateSiteNameGeometry(const LinkPreviewBlock &block, const QRect &titleGeometry, const QRect &faviconGeometry) const
{
    auto siteNameY = (titleGeometry.bottom() + 1);

    const auto hasTextAbove = !titleGeometry.isEmpty();
    if (hasTextAbove)
    {
        siteNameY += Style::Snippet::getSiteNameTopPadding();
    }

    auto siteNameX = (faviconGeometry.right() + 1);

    const auto hasFavicon = !faviconGeometry.isEmpty();
    if (hasFavicon)
    {
        siteNameX += Style::Snippet::getSiteNameLeftPadding();
    }

    QSize siteNameSize(0, 0);

    const auto &siteName = block.getSiteName();
    if (!siteName.isEmpty())
    {
        QFontMetrics m(block.getSiteNameFont());

        siteNameSize = m.tightBoundingRect(siteName).size();

        if (hasTextAbove)
        {
            siteNameY -= siteNameSize.height();
        }
    }
    else if (block.isInPreloadingState())
    {
        assert(!faviconGeometry.isEmpty());
        siteNameSize = Style::Snippet::getSiteNamePlaceholderSize();
        siteNameY = faviconGeometry.top();
    }

    const QPoint leftTop(siteNameX, siteNameY);

    return QRect(leftTop, siteNameSize);
}

QRect YoutubeLinkPreviewBlockLayout::evaluateTitleLtr(const QRect &previewContentLtr, const QRect &previewImageRect, const bool isPlaceholder)
{
    assert(previewContentLtr.width() > 0);
    assert(previewContentLtr.height() == 0);

    QRect titleLtr(
        previewContentLtr.left(),
        previewImageRect.bottom() + 1,
        previewContentLtr.width(),
        0);

    const auto hasPreview = !previewImageRect.isEmpty();

    if (hasPreview)
    {
        const auto topOffset = (isPlaceholder ? Style::Snippet::getTitlePlaceholderTopOffset() : Style::Snippet::getTitleTopOffset());
        titleLtr.translate(0, topOffset);
    }

    return titleLtr;
}

int32_t YoutubeLinkPreviewBlockLayout::evaluateWidgetHeight(
    const QRect &previewImageGeometry,
    const QRect &faviconGeometry,
    const QRect &siteNameGeometry,
    const bool isPlaceholder)
{
    auto bottom = (previewImageGeometry.bottom() + 1);
    bottom = std::max(bottom, faviconGeometry.bottom());

    auto siteNameBottom = (siteNameGeometry.bottom() + 1);

    const auto hasSiteName = !siteNameGeometry.isEmpty();
    const auto applyBaselineFix = (hasSiteName && !isPlaceholder);
    if (applyBaselineFix)
    {
        // an approximation of the gap between font baseline and font bottom line
        siteNameBottom += (siteNameGeometry.height() / 3);
    }

    bottom = std::max(bottom, siteNameBottom);

    auto top = previewImageGeometry.top();

    auto contentHeight = (bottom - top);
    assert(contentHeight >= 0);

    const auto height = std::max(MessageStyle::getMinBubbleHeight(), contentHeight);

    return height;
}

QRect YoutubeLinkPreviewBlockLayout::setTitleGeometry(LinkPreviewBlock &block, const QRect &titleLtr)
{
    assert(titleLtr.isEmpty());

    if (block.isInPreloadingState())
    {
        QRect preloaderGeometry(
            titleLtr.topLeft(),
            Style::Snippet::getTitlePlaceholderSize());

        return preloaderGeometry;
    }

    if (!block.hasTitle())
    {
        return titleLtr;
    }

    const auto textWidth = titleLtr.width();
    assert(textWidth > 0);

    const auto widthChanged = (textWidth != CurrentTitleGeometry_.width());
    if (widthChanged)
    {
        block.setTitleWidth(textWidth);
    }

    const auto textX = titleLtr.left();
    assert(textX >= 0);

    const auto textY = titleLtr.top();
    assert(textY >= 0);

    const QRect titleGeometry(
        textX,
        textY,
        textWidth,
        block.getTitleTextHeight());

    const auto geometryChanged = (titleGeometry != CurrentTitleGeometry_);
    if (geometryChanged)
    {
        block.setTitleGeometry(titleGeometry);

        CurrentTitleGeometry_ = titleGeometry;
    }

    return titleGeometry;
}

UI_COMPLEX_MESSAGE_NS_END