#include "stdafx.h"

#include "../../../controls/TextEmojiWidget.h"

#include "../../../controls/PictureWidget.h"

#include "../../../controls/ContactAvatarWidget.h"

#include "../MessageStyle.h"

#include "QuoteBlock.h"

#include "QuoteBlockLayout.h"

#include "Style.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlockLayout::QuoteBlockLayout()
{

}

QuoteBlockLayout::~QuoteBlockLayout()
{

}

const IItemBlockLayout::IBoxModel& QuoteBlockLayout::getBlockBoxModel() const
{
    static const QMargins bubbleMargins(
        0,
        0,
        0,
        Utils::scale_value(16)
    );

    static const BoxModel boxModel(
        false,
        bubbleMargins);

    return boxModel;

}

QRect QuoteBlockLayout::getBlockGeometry() const
{
    auto &block = *blockWidget<QuoteBlock>();
    return block.Geometry_;
}

QSize QuoteBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
{
    QLayout::setGeometry(widgetGeometry);

    const auto contentLtr = evaluateContentLtr(widgetGeometry);

    const auto enoughSpace = (contentLtr.width() > 0);
    if (!enoughSpace)
    {
        return widgetGeometry.size();
    }

    const auto textCtrlGeometry = setTextControlGeometry(contentLtr);

    return QSize(
        widgetGeometry.width(),
        textCtrlGeometry.height());
}

QRect QuoteBlockLayout::evaluateContentLtr(const QRect &widgetGeometry) const
{
    return widgetGeometry;
}

QRect QuoteBlockLayout::setTextControlGeometry(const QRect &contentLtr)
{
    assert(contentLtr.width() > 0);

    auto &block = *blockWidget<QuoteBlock>();

    if (!block.TextCtrl_)
    {
        return contentLtr;
    }

    auto &textCtrl = *block.TextCtrl_;

    const auto textWidth = MessageStyle::roundTextWidthDown(contentLtr.width());

    const auto widthChanged = (textWidth != CurrentTextCtrlGeometry_.width());
    if (widthChanged)
    {
        textCtrl.setFixedWidth(textWidth);
    }

    QRect avatarGeometry(
        QPoint(
            contentLtr.topLeft().x() + Style::Quote::getQuoteOffsetLeft() + Style::Quote::getForwardIconOffset(),
            contentLtr.topLeft().y() + Style::Quote::getFirstQuoteOffset()
        ), Style::Quote::getQuoteAvatarSize());

    const QSize textSize(textWidth, textCtrl.height());

    QRect textCtrlGeometry(
        QPoint(
            contentLtr.topLeft().x() + Style::Quote::getQuoteAvatarOffset() + Style::Quote::getQuoteOffsetLeft() + Style::Quote::getForwardIconOffset(),
            contentLtr.topLeft().y() + Style::Quote::getQuoteUsernameOffsetTop() + Style::Quote::getFirstQuoteOffset()
        ), textSize);

    if (block.needForwardBlock())
    {
        const QRect forwardIconRect(
            QPoint(
                contentLtr.topLeft().x(),
                contentLtr.topLeft().y() + Style::Quote::getFirstQuoteOffset() + Utils::scale_value(4)
            ), block.ForwardIcon_->size());
        block.ForwardIcon_->setGeometry(forwardIconRect);

        const QRect forwardLabelGeometry(
            QPoint(
                avatarGeometry.x(),
                contentLtr.topLeft().y() + Style::Quote::getFirstQuoteOffset()
            ), block.ForwardLabel_->size());
        block.ForwardLabel_->setGeometry(forwardLabelGeometry);

        textCtrlGeometry.moveTop(
            forwardLabelGeometry.bottomLeft().y()
            + Style::Quote::getForwardLabelBottomMargin()
            + Style::Quote::getQuoteUsernameOffsetTop());

        avatarGeometry.moveTop(
            forwardLabelGeometry.bottomLeft().y()
            + Style::Quote::getForwardLabelBottomMargin());
    }

    const auto geometryChanged = (textCtrlGeometry != CurrentTextCtrlGeometry_);
    if (geometryChanged)
    {
        assert(widthChanged);

        textCtrl.setGeometry(textCtrlGeometry);

        block.Avatar_->setGeometry(avatarGeometry);

        CurrentTextCtrlGeometry_ = textCtrlGeometry;
    }

    return textCtrlGeometry;
}

UI_COMPLEX_MESSAGE_NS_END
