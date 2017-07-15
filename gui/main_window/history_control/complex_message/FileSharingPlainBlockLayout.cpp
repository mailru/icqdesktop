#include "stdafx.h"

#include "../../../fonts.h"
#include "../../../utils/Text.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "FileSharingBlock.h"
#include "Style.h"

#include "FileSharingPlainBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    QSize evalBubbleSize(const int32_t blockWidth, const bool isSingle);

    QRect evalFilenameGeometry(const QRect &contentRect, const QRect &btnRect, const int32_t textHeight);

    QRect evalFileSizeGeometry(const QRect &contentRect, const QRect &btnRect, const QFontMetrics &m, const QString &text);

    QRect evalShowInDirLinkGeometry(const QRect &fileSizeRect, const QFontMetrics &m, const QString &text);
}

FileSharingPlainBlockLayout::FileSharingPlainBlockLayout()
{

}

FileSharingPlainBlockLayout::~FileSharingPlainBlockLayout()
{

}

QSize FileSharingPlainBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    const auto &block = *blockWidget<FileSharingBlock>();
    const bool isSingle = block.isStandalone();

    return evalBubbleSize(maxWidth, isSingle);
}

QRect FileSharingPlainBlockLayout::getAuthorAvatarRect() const
{
    assert(!"method is not expected to be called");
    return QRect();
}

QRect FileSharingPlainBlockLayout::getAuthorNickRect() const
{
    assert(!"method is not expected to be called");
    return QRect();
}

const QRect& FileSharingPlainBlockLayout::getContentRect() const
{
    return ContentRect_;
}

const QRect& FileSharingPlainBlockLayout::getFilenameRect() const
{
    return FilenameRect_;
}

QRect FileSharingPlainBlockLayout::getFileSizeRect() const
{
    return FileSizeRect_;
}

QRect FileSharingPlainBlockLayout::getShowInDirLinkRect() const
{
    return ShowInDirLinkRect_;
}

QSize FileSharingPlainBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<FileSharingBlock>();

    const auto isSingle = block.isStandalone();

    const auto frameSize = evalBubbleSize(geometry.width(), isSingle);

    ContentRect_ = QRect(geometry.topLeft(), frameSize);

    const auto btnGeometry = setCtrlButtonGeometry(block, ContentRect_);

    {
        const auto filenameHeight = Utils::evaluateActualLineHeight(Style::Files::getFilenameFont());
        FilenameRect_ = evalFilenameGeometry(ContentRect_, btnGeometry, filenameHeight);
    }

    {
        QFontMetrics m(Style::Files::getFileSizeFont());
        FileSizeRect_ = evalFileSizeGeometry(ContentRect_, btnGeometry, m, block.getProgressText());
    }

    {
        QFontMetrics m(Style::Files::getShowInDirLinkFont());
        ShowInDirLinkRect_ = evalShowInDirLinkGeometry(FileSizeRect_, m, block.getShowInDirLinkText());
    }

    return frameSize;
}

QRect FileSharingPlainBlockLayout::setCtrlButtonGeometry(FileSharingBlock &block, const QRect &contentRect)
{
    const auto x = (
        contentRect.left() +
        Style::Files::getCtrlIconLeftMargin() +
        Ui::MessageStyle::getRotatingProgressBarPenWidth());
    const auto y = Utils::scale_value(8);
    const QPoint buttonPos(x, y);

    QRect buttonRect(buttonPos, QSize(0, 0));

    assert(!contentRect.isEmpty());
    if (contentRect.isEmpty())
    {
        return buttonRect;
    }

    const auto buttonSize = block.getCtrlButtonSize();

    assert(!buttonSize.isEmpty());
    if (buttonSize.isEmpty())
    {
        return buttonRect;
    }

    buttonRect.setSize(buttonSize);

    block.setCtrlButtonGeometry(buttonRect);

    return buttonRect;
}

namespace
{
    QSize evalBubbleSize(const int32_t blockWidth, const bool isSingle)
    {
        const auto maxWidth = (isSingle ? QWIDGETSIZE_MAX : Style::getBlockMaxWidth());

        const auto width = std::min(blockWidth, maxWidth);
        assert(width > 0);

        return QSize(width, Style::Files::getFileBubbleHeight());
    }

    QRect evalFilenameGeometry(const QRect &contentRect, const QRect &btnRect, const int32_t textHeight)
    {
        assert(textHeight > 0);

        auto textX = btnRect.right() + 1;
        textX += Style::Files::getFilenameLeftMargin();

        auto textY = Style::Files::getFilenameBaseline() + Utils::scale_value(4);
        textY -= textHeight;

        auto textWidth = (contentRect.right() + 1 - textX);
        textWidth -= Style::Files::getFilenameLeftMargin(); // the right margin is the same as the left one

        return QRect(textX, textY, textWidth, textHeight);
    }

    QRect evalFileSizeGeometry(const QRect &contentRect, const QRect &btnRect, const QFontMetrics &m, const QString &text)
    {
        auto textX = btnRect.right() + 1;
        textX += Style::Files::getFilenameLeftMargin();

        auto textY = Style::Files::getFileSizeBaseline() + Utils::scale_value(4);

        const auto textHeight = m.height();
        textY -= textHeight;

        auto maxTextWidth = (contentRect.right() + 1 - textX);
        maxTextWidth -= Style::Files::getFilenameLeftMargin(); // the right margin is the same as the left one

        auto textWidth = m.width(text);
        textWidth = std::min(textWidth, maxTextWidth);

#ifdef __APPLE__
        textWidth = maxTextWidth;
#endif

        return QRect(textX, textY, textWidth, textHeight);
    }

    QRect evalShowInDirLinkGeometry(const QRect &fileSizeRect, const QFontMetrics &m, const QString &text)
    {
        assert(!text.isEmpty());

        auto textX = (fileSizeRect.right() + 1);
        textX += Style::Files::getShowInDirLinkLeftMargin();

        auto textY = Style::Files::getFileSizeBaseline() + Utils::scale_value(4);

        const auto textHeight = m.height();
        textY -= textHeight;

        const auto textWidth = m.width(text);

        return QRect(textX, textY, textWidth, textHeight);
    }
}

UI_COMPLEX_MESSAGE_NS_END