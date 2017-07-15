#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../controls/TextEditEx.h"

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "ImagePreviewBlock.h"
#include "Style.h"

#include "ImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ImagePreviewBlockLayout::ImagePreviewBlockLayout()
{
}

ImagePreviewBlockLayout::~ImagePreviewBlockLayout()
{
}

QSize ImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth);

    const auto &block = *blockWidget<ImagePreviewBlock>();

    auto previewSize = (
        block.hasPreview() ?
            Utils::scale_value(block.getPreviewSize()) :
            Style::Preview::getImagePlaceholderSize());

    const auto maxSizeWidth = std::min(maxWidth, Style::Preview::getImageWidthMax());

    const QSize maxSize(
        maxSizeWidth,
        Style::Preview::getImageHeightMax());

    previewSize = limitSize(previewSize, maxSize);

    return previewSize;
}

void ImagePreviewBlockLayout::setActionButtonGeometry(const QRect &previewRect, ImagePreviewBlock &block)
{
    if (!block.hasActionButton())
    {
        return;
    }

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

    auto actionButtonCenter = previewRect_.center();

    const auto actionButtonCenteringOffsetY = (actionButtonRect.center().y() - block.getActionButtonLogicalCenter().y());
    actionButtonCenter.ry() += actionButtonCenteringOffsetY;

    actionButtonRect.moveCenter(actionButtonCenter);

    block.showActionButton(actionButtonRect);
}

QSize ImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &blockLtr)
{
    assert(blockLtr.width() > 0);

    auto &block = *blockWidget<ImagePreviewBlock>();

    setPreviewGeometry(blockLtr, block);

    setTextControlGeometry(blockLtr);

    setActionButtonGeometry(previewRect_, block);

    blockRect_ = QRect(previewRect_.topLeft(), 
        QPoint(std::max(previewRect_.right(), textCtrlBubbleRect_.right()), previewRect_.bottom() + textCtrlBubbleRect_.height()));

    return blockRect_.size();
}

void ImagePreviewBlockLayout::setPreviewGeometry(const QRect &blockLtr, ImagePreviewBlock &block)
{
    assert(blockLtr.width() > 0);

    auto previewSize = (
        block.hasPreview() ?
            Utils::scale_value(block.getPreviewSize()) :
            Style::Preview::getImagePlaceholderSize());

    auto maxSizeWidth = std::min(blockLtr.width(), Style::Preview::getImageWidthMax());
    if (block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, block.getMaxPreviewWidth());

    const QSize maxSize(
        maxSizeWidth,
        Style::Preview::getImageHeightMax());

    previewSize = limitSize(previewSize, maxSize);

    previewRect_ = QRect(
        QPoint(0, 0),
        previewSize);
}

QRect ImagePreviewBlockLayout::setTextControlGeometry(const QRect &contentLtr)
{
    assert(contentLtr.width() > 0);

    auto &block = *blockWidget<ImagePreviewBlock>();

    if (!block.link_)
    {
        return contentLtr;
    }

    auto &textCtrl = *block.link_;

    const int leftMargin = (block.isSingle() ? Utils::scale_value(16) : 0);
    const int rightMargin = (block.isSingle() ? Utils::scale_value(16) : 0);
    const int topMargin = Utils::scale_value(4);
    const int bottomMargin = Utils::scale_value(8);

    auto textWidth = (block.isSingle() ? previewRect_.width() : contentLtr.width());
    auto textWidthWithMargins = textWidth - leftMargin - rightMargin;

    textWidthWithMargins = MessageStyle::roundTextWidthDown(textWidthWithMargins);

    const auto widthChanged = (textWidthWithMargins != currentTextCtrlGeometry_.width());
    if (widthChanged)
    {
        textCtrl.setFixedWidth(textWidthWithMargins);
        textCtrl.document()->setTextWidth(textWidthWithMargins);
    }

    const QSize textSize(textWidthWithMargins, textCtrl.getTextHeight());

    const QRect textCtrlGeometry(previewRect_.left() + leftMargin, previewRect_.bottom() + topMargin, textSize.width(), textSize.height());

    const auto geometryChanged = (textCtrlGeometry != currentTextCtrlGeometry_);

    if (geometryChanged)
    {
        textCtrl.setGeometry(textCtrlGeometry);

        currentTextCtrlGeometry_ = textCtrlGeometry;

        textCtrlBubbleRect_ = QRect(previewRect_.left(), previewRect_.bottom(), textWidth, textSize.height() + topMargin + bottomMargin);
    }

    return textCtrlGeometry;
}


const QRect& ImagePreviewBlockLayout::getPreviewRect() const
{
    return previewRect_;
}

const QRect& ImagePreviewBlockLayout::getTextBlockRect() const
{
    return currentTextCtrlGeometry_;
}

const QRect& ImagePreviewBlockLayout::getBlockRect() const
{
    return blockRect_;
}

UI_COMPLEX_MESSAGE_NS_END