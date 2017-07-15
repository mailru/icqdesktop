#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

namespace ComplexMessageItemBuilder
{
    ComplexMessageItem* makeComplexItem(
        QWidget *parent,
        const int64_t id,
        const QDate date,
        const int64_t prev,
        const QString &text,
        const QString &chatAimid,
        const QString &senderAimid,
        const QString &senderFriendly,
        const QList<Data::Quote>& quotes,
        HistoryControl::StickerInfoSptr sticker,
        const bool isOutgoing,
        const bool isNotAuth);

}

UI_COMPLEX_MESSAGE_NS_END