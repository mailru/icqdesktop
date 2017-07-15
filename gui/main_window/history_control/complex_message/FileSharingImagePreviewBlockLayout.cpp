#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "FileSharingBlock.h"
#include "Style.h"

#include "FileSharingImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingImagePreviewBlockLayout::FileSharingImagePreviewBlockLayout()
{

}

FileSharingImagePreviewBlockLayout::~FileSharingImagePreviewBlockLayout()
{

}

QSize FileSharingImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    auto &block = *blockWidget<FileSharingBlock>();

    auto previewSize = (
        block.isFailedSnap() ?
            Style::Snaps::getFailedSnapSizeMax() :
            Utils::scale_value(block.getOriginalPreviewSize()));

    const auto maxSizeWidth = std::min(Utils::scale_value(maxWidth), Style::Preview::getImageWidthMax());
    const QSize maxSize(maxSizeWidth, Style::Preview::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = Style::Preview::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QSize blockSize(previewSize);

    if (block.isAuthorVisible())
    {
        AuthorAvatarRect_ = evaluateAuthorAvatarRect(block.isStandalone());

        auto authorHeight = getAuthorAvatarRect().bottom();
        authorHeight += Style::Snaps::getAuthorAvatarBottomMargin();

        blockSize.rheight() += authorHeight;
    }

    return blockSize;
}

QRect FileSharingImagePreviewBlockLayout::getAuthorAvatarRect() const
{
    return AuthorAvatarRect_;
}

QRect FileSharingImagePreviewBlockLayout::getAuthorNickRect() const
{
    return AuthorNickRect_;
}

const QRect& FileSharingImagePreviewBlockLayout::getContentRect() const
{
    return PreviewRect_;
}

const QRect& FileSharingImagePreviewBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QRect FileSharingImagePreviewBlockLayout::getFileSizeRect() const
{
    return QRect();
}

QRect FileSharingImagePreviewBlockLayout::getShowInDirLinkRect() const
{
    return QRect();
}

const IItemBlockLayout::IBoxModel& FileSharingImagePreviewBlockLayout::getBlockBoxModel() const
{
    static const QMargins margins(
        Utils::scale_value(16),
        Utils::scale_value(12),
        Utils::scale_value(8),
        Utils::scale_value(16));

    static const BoxModel boxModel(
        true,
        margins);

    return boxModel;
}

QSize FileSharingImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<FileSharingBlock>();

    if (block.isAuthorVisible())
    {
        AuthorAvatarRect_ = evaluateAuthorAvatarRect(block.isStandalone());
        AuthorNickRect_ = evaluateAuthorNickRect(block.isStandalone(), AuthorAvatarRect_, geometry.width());
    }

    PreviewRect_ = evaluatePreviewRect(block, geometry.width());
    setCtrlButtonGeometry(block, PreviewRect_);
    
    if (block.isAuthorVisible())
    {
        AuthorNickRect_.setRight(PreviewRect_.right());
        block.setAuthorNickGeometry(AuthorNickRect_);
    }

    auto blockSize = PreviewRect_.size();

    if (block.isAuthorVisible())
    {
        blockSize.rheight() += AuthorAvatarRect_.bottom();
        blockSize.rheight() += Style::Snaps::getAuthorAvatarBottomMargin();
    }

    return blockSize;
}

QRect FileSharingImagePreviewBlockLayout::evaluateAuthorAvatarRect(const bool isStandalone) const
{
    const auto top = (isStandalone ? Style::Snaps::getAuthorAvatarLeft(isStandalone) : 0);

    const QPoint leftTop(Style::Snaps::getAuthorAvatarLeft(isStandalone), top);

    QRect result(leftTop, Style::Snaps::getAuthorAvatarSizeInLayout());

    return result;
}

QRect FileSharingImagePreviewBlockLayout::evaluateAuthorNickRect(const bool isStandalone, const QRect &authorAvatarRect, const int32_t blockWidth) const
{
    assert(authorAvatarRect.isValid());
    assert(blockWidth > 0);

    auto nickLeft = authorAvatarRect.right();
    nickLeft += Style::Snaps::getAuthorNickLeftMargin();

    const auto nickRight = blockWidth;

    const auto nickTop = Style::Snaps::getAuthorNickTopMargin(isStandalone);

    QRect result(
        nickLeft, nickTop,
        0, Style::Snaps::getAuthorNickBaseline());
    result.setRight(nickRight);

    return result;
}

QRect FileSharingImagePreviewBlockLayout::evaluatePreviewRect(const FileSharingBlock &block, const int32_t blockWidth) const
{
    assert(blockWidth > 0);

    auto previewSize = (
        block.isFailedSnap() ?
            Style::Snaps::getFailedSnapSizeMax() :
            Utils::scale_value(block.getOriginalPreviewSize()));

    auto maxSizeWidth = std::min(blockWidth, previewSize.width());
    if (block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, block.getMaxPreviewWidth());
    const QSize maxSize(maxSizeWidth, Style::Preview::getImageHeightMax());
    previewSize = limitSize(previewSize, maxSize);

    const auto minPreviewSize = Style::Preview::getMinPreviewSize();

    const auto shouldScaleUp =
        (previewSize.width() < minPreviewSize.width()) &&
        (previewSize.height() < minPreviewSize.height());
    if (shouldScaleUp)
    {
        previewSize = previewSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
    }

    QPoint leftTop;

    if (block.isAuthorVisible())
    {
        const auto authorHeight = (
            AuthorAvatarRect_.bottom() +
            Style::Snaps::getAuthorAvatarBottomMargin());

        leftTop.ry() = authorHeight;
    }

    return QRect(leftTop, previewSize);
}

void FileSharingImagePreviewBlockLayout::setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect)
{
    assert(!previewRect.isEmpty());
    if (previewRect.isEmpty())
    {
        return;
    }

    if (block.isFailedSnap())
    {
        return;
    }

    const auto buttonSize = block.getCtrlButtonSize();

    assert(!buttonSize.isEmpty());
    if (buttonSize.isEmpty())
    {
        return;
    }

    QRect buttonRect(QPoint(), buttonSize);

    buttonRect.moveCenter(previewRect.center());

    block.setCtrlButtonGeometry(buttonRect);
}

UI_COMPLEX_MESSAGE_NS_END
