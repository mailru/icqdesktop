#include "stdafx.h"

#include "../../core_dispatcher.h"

#include "../../cache/avatars/AvatarStorage.h"

#include "../../utils/InterConnector.h"
#include "../../utils/PainterPath.h"
#include "../../utils/utils.h"

#include "../../themes/ThemePixmap.h"

#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "VoipEventInfo.h"

#include "VoipEventItem.h"

#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"

namespace Ui
{

    namespace
    {
        int32_t getIconRightPadding();

        int32_t getIconTopPadding();

        int32_t getTextBaselineY();

    }

    VoipEventItem::VoipEventItem(const HistoryControl::VoipEventInfoSptr& eventInfo)
        : MessageItemBase(nullptr)
        , EventInfo_(eventInfo)
        , IsAvatarHovered_(false)
        , IsBubbleHovered_(false)
        , TimeWidget_(nullptr)
        , lastRead_(false)
        , id_(-1)
    {
        assert(EventInfo_);
    }

    VoipEventItem::VoipEventItem(
        QWidget *parent,
        const HistoryControl::VoipEventInfoSptr& eventInfo)
        : MessageItemBase(parent)
        , EventInfo_(eventInfo)
        , IsAvatarHovered_(false)
        , IsBubbleHovered_(false)
        , TimeWidget_(new MessageTimeWidget(this))
        , lastRead_(false)
        , id_(-1)
    {
        assert(EventInfo_);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        if (!EventInfo_->isVisible())
        {
            assert(!"invisible voip events are not allowed in history control");
            setFixedHeight(0);
            return;
        }

        setAttribute(Qt::WA_TranslucentBackground);

        if (EventInfo_->isClickable())
        {
            setMouseTracking(true);
        }

        Icon_ = eventInfo->loadIcon(false);
        HoverIcon_ = eventInfo->loadIcon(true);

        TimeWidget_->setContact(EventInfo_->getContactAimid());
        TimeWidget_->setTime(EventInfo_->getTimestamp());
    }

    QString VoipEventItem::formatRecentsText() const
    {
        return EventInfo_->formatEventText();
    }

    void VoipEventItem::updateHeight()
    {
        int height = MessageStyle::getMinBubbleHeight() + MessageStyle::getTopMargin(hasTopMargin());

        if (lastRead_)
        {
            height += MessageStyle::getLastReadAvatarSize() + 2 * MessageStyle::getLastReadAvatarMargin();
        }

        setFixedHeight(height);
    }

    void VoipEventItem::setTopMargin(const bool value)
    {
        HistoryControlPageItem::setTopMargin(value);

        if (!EventInfo_->isVisible())
        {
            return;
        }

        updateHeight();
    }

    void VoipEventItem::setHasAvatar(const bool value)
    {
        if (!isOutgoing() && value)
        {
            auto isDefault = false;
            Avatar_ = Logic::GetAvatarStorage()->GetRounded(
                EventInfo_->getContactAimid(),
                EventInfo_->getContactFriendly(),
                Utils::scale_bitmap(MessageStyle::getAvatarSize()),
                QString(),
                true,
                Out isDefault,
                false,
                false
            );

            assert(Avatar_);
        }
        else
        {
            Avatar_.reset();
        }

        HistoryControlPageItem::setHasAvatar(value);
    }

    void VoipEventItem::mouseMoveEvent(QMouseEvent *event)
    {
        const auto pos = event->pos();

        IsAvatarHovered_ = isAvatarHovered(pos);

        IsBubbleHovered_ = isBubbleHovered(pos);

        const auto isHovered = (IsAvatarHovered_ || IsBubbleHovered_);

        if (isHovered)
        {
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }

        update();
    }

    void VoipEventItem::mouseReleaseEvent(QMouseEvent *event)
    {
        const auto pos = event->pos();

        if (isBubbleHovered(pos))
        {
            const auto &contactAimid = EventInfo_->getContactAimid();
            assert(!contactAimid.isEmpty());
            Ui::GetDispatcher()->getVoipController().setStartV(contactAimid.toUtf8(), false);

            return;
        }

        if (isAvatarHovered(pos))
        {
            emit Utils::InterConnector::instance().profileSettingsShow(EventInfo_->getContactAimid());
        }
    }

    void VoipEventItem::leaveEvent(QEvent *)
    {
        IsAvatarHovered_ = false;

        IsBubbleHovered_ = false;

        update();
    }

    void VoipEventItem::paintEvent(QPaintEvent *)
    {
        if (!BubbleRect_.isValid())
        {
            return;
        }

        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);

        if (Bubble_.isEmpty())
        {
            Bubble_ = Utils::renderMessageBubble(BubbleRect_, MessageStyle::getBorderRadius(), isOutgoing());
            assert(!Bubble_.isEmpty());
        }

        assert(BubbleRect_.width() > 0);

        int theme_id = get_qt_theme_settings()->themeIdForContact(EventInfo_->getContactAimid());//theme()->get_id();

        const auto bodyBrush = Ui::MessageStyle::getBodyBrush(isOutgoing(), IsBubbleHovered_, theme_id);

        p.fillPath(Bubble_, bodyBrush);

        const auto baseY = BubbleRect_.top();

        auto cursorX = MessageStyle::getLeftMargin(isOutgoing());

        if (!isOutgoing())
        {
            if (Avatar_)
            {
                p.drawPixmap(
                    getAvatarRect(),
                    *Avatar_
                );
            }

            cursorX += MessageStyle::getAvatarSize();
            cursorX += MessageStyle::getAvatarRightMargin();
        }

        auto &icon = (IsBubbleHovered_ ? HoverIcon_ : Icon_);
        if (icon)
        {
            cursorX += MessageStyle::getBubbleHorPadding();

            icon->Draw(p, cursorX, baseY + getIconTopPadding());
            cursorX += icon->GetWidth();

            cursorX += getIconRightPadding();
        }
        else
        {
            cursorX += MessageStyle::getBubbleHorPadding();
        }

        if (TimeWidgetGeometry_.isValid())
        {
            auto eventText = (
                IsBubbleHovered_ ?
                    QT_TRANSLATE_NOOP("chat_event", "Call back") :
                    EventInfo_->formatEventText());

            const auto eventTextFont = MessageStyle::getTextFont();

            auto textWidth = (
                TimeWidgetGeometry_.left() -
                getIconRightPadding() -
                cursorX);
            textWidth = std::max(0, textWidth);

            QFontMetrics fontMetrics(eventTextFont);
            eventText = fontMetrics.elidedText(eventText, Qt::ElideRight, textWidth);

            p.setPen(getTextColor(IsBubbleHovered_));
            p.setFont(eventTextFont);
            p.drawText(
                cursorX,
                baseY + getTextBaselineY(),
                eventText);
        }

        if (lastRead_)
        {
            drawLastReadAvatar(
                p,
                EventInfo_->getContactAimid(),
                EventInfo_->getContactFriendly(),
                MessageStyle::getRightMargin(isOutgoing()),
                MessageStyle::getLastReadAvatarMargin());
        }
    }

    void VoipEventItem::resizeEvent(QResizeEvent *event)
    {
        QRect newBubbleRect(QPoint(0, 0), event->size());

        QMargins margins(
            MessageStyle::getLeftMargin(isOutgoing()),
            MessageStyle::getTopMargin(hasTopMargin()),
            MessageStyle::getRightMargin(isOutgoing()) +
            MessageStyle::getTimeMaxWidth(),
            (lastRead_ ? (MessageStyle::getLastReadAvatarSize() + 2 * MessageStyle::getLastReadAvatarMargin()) : (0) )
        );

        if (!isOutgoing())
        {
            margins.setLeft(
                margins.left() + MessageStyle::getAvatarSize() + MessageStyle::getAvatarRightMargin()
            );
        }

        newBubbleRect = newBubbleRect.marginsRemoved(margins);

        if (BubbleRect_ != newBubbleRect)
        {
            BubbleRect_ = (newBubbleRect.isValid() ? newBubbleRect : QRect());
            Bubble_ = QPainterPath();
        }

        const auto timeWidgetSize = TimeWidget_->sizeHint();

        auto timeX = BubbleRect_.right();
        timeX += MessageStyle::getTimeMarginX();

        auto timeY = BubbleRect_.bottom();
        timeY -= MessageStyle::getTimeMarginY();
        timeY -= timeWidgetSize.height();

        QRect timeWidgetGeometry(
            timeX,
            timeY,
            timeWidgetSize.width(),
            timeWidgetSize.height()
        );

        TimeWidget_->setGeometry(timeWidgetGeometry);
        TimeWidget_->show();

        TimeWidgetGeometry_ = timeWidgetGeometry;

        HistoryControlPageItem::resizeEvent(event);
    }

    QRect VoipEventItem::getAvatarRect() const
    {
        assert(!BubbleRect_.isEmpty());
        assert(hasAvatar());

        QRect result(
            MessageStyle::getLeftMargin(isOutgoing()),
            BubbleRect_.top(),
            MessageStyle::getAvatarSize(),
            MessageStyle::getAvatarSize()
        );

        return result;
    }

    bool VoipEventItem::isAvatarHovered(const QPoint &mousePos) const
    {
        if (!hasAvatar())
        {
            return false;
        }

        return getAvatarRect().contains(mousePos);
    }

    bool VoipEventItem::isBubbleHovered(const QPoint &mousePos) const
    {
        if (!EventInfo_->isClickable())
        {
            return false;
        }

        const auto isHovered = (
            (mousePos.y() > MessageStyle::getTopMargin(hasTopMargin())) &&
            (mousePos.y() < height()) &&
            (mousePos.x() > BubbleRect_.left()) &&
            (mousePos.x() < BubbleRect_.right())
        );

        return isHovered;
    }

    bool VoipEventItem::isOutgoing() const
    {
        return !EventInfo_->isIncomingCall();
    }

    bool VoipEventItem::setLastRead(const bool _isLastRead)
    {
        HistoryControlPageItem::setLastRead(_isLastRead);

        if (_isLastRead != lastRead_)
        {
            lastRead_ = _isLastRead;

            updateHeight();

            return true;
        }

        return false;
    }

    void VoipEventItem::setId(const qint64 _id)
    {
        id_ = _id;
    }

    qint64 VoipEventItem::getId() const
    {
        return id_;
    }

    void VoipEventItem::updateStyle()
    {
        if (TimeWidget_)
            TimeWidget_->update();
        update();
    }

	void VoipEventItem::setQuoteSelection()
	{
		/// TODO-quote
		assert(0);
	}

    QColor VoipEventItem::getTextColor(const bool isHovered)
    {
        QColor textColor = (isHovered ? "#ffffff" : MessageStyle::getTextColor());
        return textColor;
    }

    namespace
    {

        int32_t getIconRightPadding()
        {
            return Utils::scale_value(12);
        }

        int32_t getIconTopPadding()
        {
            return Utils::scale_value(9);
        }

        int32_t getTextBaselineY()
        {
            return Utils::scale_value(21);
        }
    }
}