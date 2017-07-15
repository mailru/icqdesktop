#include "stdafx.h"
#include "MessageStatusWidget.h"

#include "../../utils/utils.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "../../theme_settings.h"

namespace Ui
{
    MessageTimeWidget::MessageTimeWidget(HistoryControlPageItem *messageItem)
        : QWidget(messageItem)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void MessageTimeWidget::setTime(const int32_t timestamp)
    {
        TimeText_ =
            QDateTime::fromTime_t(timestamp)
                .time()
                .toString("HH:mm");

        QFontMetrics m(MessageStyle::getTimeFont());

        TimeTextSize_ = m.boundingRect(TimeText_).size();
        assert(TimeTextSize_.isValid());

        if (TimeTextSize_.width() != m.width(TimeText_))
            TimeTextSize_.setWidth(m.width(TimeText_));

        updateGeometry();

        update();
    }

    QSize MessageTimeWidget::sizeHint() const
    {
        assert(!TimeTextSize_.isEmpty());

        return TimeTextSize_;
    }

    void MessageTimeWidget::paintEvent(QPaintEvent *)
    {
        QPainter p(this);

        const auto height = sizeHint().height();

        auto cursorX = 0;

        QColor c(getTimeColor());
        c.setAlphaF(0.8);
        p.setFont(MessageStyle::getTimeFont());
        p.setPen(QPen(c));

        const auto textBaseline = height;

        assert(!TimeText_.isEmpty());
        p.drawText(cursorX, textBaseline, TimeText_);
    }

    QColor MessageTimeWidget::getTimeColor() const
    {
        auto curTheme = Ui::get_qt_theme_settings()->themeForContact(aimId_);
        if (!curTheme)
        {
            return MessageStyle::getTimeColor();
        }

        return curTheme->preview_stickers_.time_color_;
    }
}