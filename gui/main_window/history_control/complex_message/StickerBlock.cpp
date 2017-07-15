#include "stdafx.h"

#include "../MessageStyle.h"
#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../my_info.h"
#include "../../../themes/ResourceIds.h"
#include "../../../cache/stickers/stickers.h"

#include "StickerBlockLayout.h"
#include "Selection.h"
#include "Style.h"
#include "StickerBlock.h"

using namespace Ui::Stickers;

namespace
{
    qint32 getStickerMaxHeight()
    {
        const auto scalePercents = (int)Utils::scale_value(100);

#ifdef __APPLE__
        if (Utils::is_mac_retina())
        {
            return 65;
        }
#endif

        switch (scalePercents)
        {
            case 100: return Utils::scale_value(150);
            case 125: return Utils::scale_value(225 * 0.8);
            case 150: return Utils::scale_value(225);
            case 200: return Utils::scale_value(300);
        }

        assert(!"unexpected scale value");
        return Utils::scale_value(150);
    }

    core::sticker_size getStickerSize()
    {
        const auto scalePercents = (int)Utils::scale_value(100);

#ifdef __APPLE__
        if (Utils::is_mac_retina())
        {
            return core::sticker_size::medium;
        }
#endif

        switch (scalePercents)
        {
            case 100: return core::sticker_size::small;
            case 125: return core::sticker_size::medium;
            case 150: return core::sticker_size::medium;
            case 200: return core::sticker_size::large;
        }

        assert(!"unexpected scale");
        return core::sticker_size::small;
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

StickerBlock::StickerBlock(ComplexMessageItem *parent,  const HistoryControl::StickerInfoSptr& _info)
    : GenericBlock(parent, QT_TRANSLATE_NOOP("contact_list", "Sticker"), MenuFlagNone, false)
    , Layout_(nullptr)
    , IsSelected_(false)
    , Info_(_info)
{
    Layout_ = new StickerBlockLayout();
    setLayout(Layout_);

    Placeholder_ = Themes::GetPixmap(Themes::PixmapResourceId::StickerHistoryPlaceholder);
    LastSize_ = Placeholder_->GetSize();

    QuoteAnimation_.setSemiTransparent();
}

StickerBlock::~StickerBlock()
{

}

void StickerBlock::clearSelection()
{
    IsSelected_ = false;
    update();
}

IItemBlockLayout* StickerBlock::getBlockLayout() const
{
    return Layout_;
}

QString StickerBlock::getSelectedText(bool isFullSelect) const
{
    return IsSelected_ ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : QString();
}

QString StickerBlock::getSourceText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

QString StickerBlock::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

bool StickerBlock::isSelected() const
{
    return IsSelected_;
}

void StickerBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType /*selection*/)
{
    const QRect globalWidgetRect(
        mapToGlobal(rect().topLeft()),
        mapToGlobal(rect().bottomRight()));

    auto selectionArea(globalWidgetRect);
    selectionArea.setTop(from.y());
    selectionArea.setBottom(to.y());
    selectionArea = selectionArea.normalized();

    const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);
    assert(selectionOverlap.height() >= 0);

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

QRect StickerBlock::setBlockGeometry(const QRect &ltr)
{
    auto r = GenericBlock::setBlockGeometry(ltr);
    r.setWidth(LastSize_.width());
    r.setHeight(LastSize_.height());
    setGeometry(r);
    Geometry_ = r;
    return Geometry_;
}

void StickerBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color)
{
    p.save();

    if (Sticker_.isNull())
    {
        assert(Placeholder_);

        const auto offset = (isOutgoing() ? (width() - Placeholder_->GetWidth()) : 0);
        Placeholder_->Draw(p, offset, 0);
    }
    else
    {
        updateStickerSize();

        const auto offset = (isOutgoing() ? (width() - LastSize_.width()) : 0);

        p.drawImage(
            QRect(
                offset, 0,
                LastSize_.width(), LastSize_.height()),
            Sticker_);

		if (quate_color.isValid())
		{
			p.setBrush(QBrush(quate_color));
			p.drawRoundRect(
				QRect(
				offset, 0,
				LastSize_.width(), LastSize_.height()),
				Utils::scale_value(8),
				Utils::scale_value(8));
		}
    }

    if (isSelected())
    {
        renderSelected(p);
    }

    p.restore();
}

void StickerBlock::initialize()
{
    connectStickerSignal(true);

    requestSticker();
}

void StickerBlock::connectStickerSignal(const bool _isConnected)
{
    if (_isConnected)
    {
        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::onSticker,
            this,
            &StickerBlock::onSticker);

        QObject::connect(
            Ui::GetDispatcher(),
            &Ui::core_dispatcher::onStickers,
            this,
            &StickerBlock::onStickers);

        return;
    }

    QObject::disconnect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::onSticker,
        this,
        &StickerBlock::onSticker);

    QObject::disconnect(
        Ui::GetDispatcher(),
        &Ui::core_dispatcher::onStickers,
        this,
        &StickerBlock::onStickers);
}

void StickerBlock::loadSticker()
{
    assert(Sticker_.isNull());

    const auto sticker = getSticker(Info_->SetId_, Info_->StickerId_);
    if (!sticker)
    {
        return;
    }

    Sticker_ = sticker->getImage(getStickerSize());

    if (Sticker_.isNull())
    {
        return;
    }

    connectStickerSignal(false);

    updateGeometry();
    update();
}

void StickerBlock::renderSelected(QPainter& _p)
{
    assert(isSelected());

    const QBrush brush(Utils::getSelectionColor());
    _p.fillRect(rect(), brush);
}

void StickerBlock::requestSticker()
{
    assert(Sticker_.isNull());

    Ui::GetDispatcher()->getSticker(Info_->SetId_, Info_->StickerId_, getStickerSize());
}

void StickerBlock::updateStickerSize()
{
    auto stickerSize = Sticker_.size();

    const auto scaleDown = (stickerSize.height() > getStickerMaxHeight());

    if (!scaleDown)
    {
        if (LastSize_ != stickerSize)
        {
            LastSize_ = stickerSize;
            notifyBlockContentsChanged();
        }

        return;
    }

    const auto aspectRatio = ((double)stickerSize.width() / (double)stickerSize.height());
    const auto fixedStickerSize =
        Utils::scale_bitmap(
            QSize(
                getStickerMaxHeight() * aspectRatio,
                getStickerMaxHeight()
            )
        );

    const auto isHeightChanged = (LastSize_.height() != fixedStickerSize.height());
    if (isHeightChanged)
    {
        LastSize_ = fixedStickerSize;
        notifyBlockContentsChanged();
    }
}

void StickerBlock::onSticker(qint32 _setId, qint32 _stickerId)
{
    assert(_setId > 0);
    assert(_stickerId > 0);

    const auto isMySticker = ((Info_->SetId_ == _setId) && (Info_->StickerId_ == _stickerId));
    if (!isMySticker)
    {
        return;
    }

    if (!Sticker_.isNull())
    {
        return;
    }

    loadSticker();
}

void StickerBlock::onStickers()
{
    if (Sticker_.isNull())
    {
        requestSticker();
    }
}

UI_COMPLEX_MESSAGE_NS_END