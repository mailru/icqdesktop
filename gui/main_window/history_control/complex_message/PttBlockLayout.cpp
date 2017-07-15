#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "PttBlock.h"
#include "Style.h"

#include "PttBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

PttBlockLayout::PttBlockLayout()
{

}

PttBlockLayout::~PttBlockLayout()
{
}

QSize PttBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &pttBlock = *blockWidget<PttBlock>();

    const auto bubbleWidth = std::min(geometry.width(), Style::getBlockMaxWidth());

    const auto textSize = setDecodedTextHorGeometry(pttBlock, bubbleWidth);

    QSize bubbleSize(bubbleWidth, Style::Ptt::getPttBubbleHeight());

    const auto hasVisisbleDecodedText = (!textSize.isEmpty() && !pttBlock.isDecodedTextCollapsed());
    if (hasVisisbleDecodedText)
    {
        bubbleSize.rheight() += (Style::Ptt::getDecodedTextVertPadding() * 2);
        bubbleSize.rheight() += textSize.height();
    }

    contentRect_ = QRect(geometry.topLeft(), bubbleSize);

    ctrlButtonRect_ = setCtrlButtonGeometry(pttBlock, contentRect_);

    textButtonRect_ = setTextButtonGeometry(pttBlock, contentRect_);

    if (hasVisisbleDecodedText)
    {
        setDecodedTextGeometry(pttBlock, contentRect_, textSize);
    }

    return contentRect_.size();
}

QRect PttBlockLayout::setCtrlButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry)
{
    assert(!bubbleGeometry.isEmpty());

    const QPoint btnPos(
        Style::Ptt::getCtrlButtonMarginLeft(),
        Style::Ptt::getCtrlButtonMarginTop());

    const QRect btnRect(btnPos, pttBlock.getCtrlButtonSize());

    pttBlock.setCtrlButtonGeometry(btnRect);

    return btnRect;
}

QSize PttBlockLayout::setDecodedTextHorGeometry(PttBlock &pttBlock, const int32_t bubbleWidth)
{
    assert(bubbleWidth > 0);

    if (!pttBlock.hasDecodedText())
    {
        return QSize(0, 0);
    }

    auto textWidth = bubbleWidth;
    textWidth -= (Style::Ptt::getDecodedTextHorPadding() * 2);

    const auto textHeight = pttBlock.setDecodedTextWidth(textWidth);

    return QSize(textWidth, textHeight);
}

void PttBlockLayout::setDecodedTextGeometry(PttBlock &_pttBlock, const QRect &_contentRect, const QSize &_textSize)
{
    assert(!_contentRect.isEmpty());
    assert(!_textSize.isEmpty());

    const QPoint textPos(Style::Ptt::getDecodedTextHorPadding(), 0);

    QRect textRect(textPos, _textSize);

    auto textBottomY = (_contentRect.bottom() + 1);
    textBottomY -= Style::Ptt::getDecodedTextVertPadding();
    textRect.moveBottom(textBottomY);

    _pttBlock.setDecodedTextGeometry(textRect);
}

QRect PttBlockLayout::setTextButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry)
{
    assert(!bubbleGeometry.isEmpty());

    const auto buttonSize = pttBlock.getTextButtonSize();

    auto buttonX = (bubbleGeometry.right() + 1);
    buttonX -= buttonSize.width();

    const auto rightMargin = Style::Ptt::getTextButtonMarginRight();
    buttonX -= rightMargin;

    const auto topMargin = Style::Ptt::getTextButtonMarginTop();

    const auto buttonY = topMargin;

    const QPoint buttonPos(buttonX, buttonY);

    const QRect btnRect(buttonPos, buttonSize);

    pttBlock.setTextButtonGeometry(btnRect);

    return btnRect;
}

QRect PttBlockLayout::getAuthorAvatarRect() const
{
    assert(!"method is not expected to be called");
    return QRect();
}

QRect PttBlockLayout::getAuthorNickRect() const
{
    assert(!"method is not expected to be called");
    return QRect();
}

const QRect& PttBlockLayout::getContentRect() const
{
    assert(!contentRect_.isEmpty());
    return contentRect_;
}

const QRect& PttBlockLayout::getCtrlButtonRect() const
{
    assert(!ctrlButtonRect_.isEmpty());
    return ctrlButtonRect_;
}

const QRect& PttBlockLayout::getTextButtonRect() const
{
    assert(!textButtonRect_.isEmpty());
    return textButtonRect_;
}

int32_t PttBlockLayout::getDecodedTextSeparatorY() const
{
    return Style::Ptt::getPttBubbleHeight();
}

const QRect& PttBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QRect PttBlockLayout::getFileSizeRect() const
{
    static QRect empty;
    return empty;
}

QRect PttBlockLayout::getShowInDirLinkRect() const
{
    static QRect empty;
    return empty;
}

UI_COMPLEX_MESSAGE_NS_END