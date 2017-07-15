#include "stdafx.h"

#include "utils.h"

#include "PainterPath.h"

namespace Utils
{

    QPainterPath renderMessageBubble(
        const QSize &size,
        const int32_t borderRadius,
        const bool isOutgoing,
        const RenderBubbleFlags flags)
    {
        assert(size.isValid());

        return renderMessageBubble(
            QRect(QPoint(0, 0), size),
            borderRadius,
            isOutgoing,
            flags);
    }

    QPainterPath renderMessageBubble(
        const QRect &rect,
        const int32_t borderRadius,
        const bool isOutgoing,
        const RenderBubbleFlags flags)
    {
        (void)isOutgoing;

        assert(rect.isValid());
        assert(borderRadius > 0);
        assert((borderRadius * 2) <= rect.width());
        assert((borderRadius * 2) <= rect.height());

        const auto borderDiameter = (borderRadius * 2);

        const auto x2 = rect.left() + rect.width();
        const auto y2 = rect.top() + rect.height();

        const auto bottomMinusBorder = (y2 - borderDiameter);
        const auto rightMinusBorder = (x2 - borderDiameter);

        assert(bottomMinusBorder >= 0);
        assert(rightMinusBorder >= 0);

        QPainterPath clipPath;

        auto x = rect.left();
        auto y = rect.top();

        // lt

        const auto isLeftTopRoundingEnabled = ((flags & RenderBubble_LeftTopRounded) != 0);
        if (isLeftTopRoundingEnabled)
        {
            x += borderRadius;

            clipPath.moveTo(x, y);

            clipPath.arcTo(rect.left(), rect.top(), borderDiameter, borderDiameter, 90, 90);
        }
        else
        {
            clipPath.moveTo(x, y);
        }

        // lb

        const auto isLeftBottomRoundingEnabled = ((flags & RenderBubble_LeftBottomRounded) != 0);
        if (isLeftBottomRoundingEnabled)
        {
            x = rect.left();
            y = bottomMinusBorder;
            clipPath.lineTo(x, y);

            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 180, 90);
        }
        else
        {
            x = rect.left();
            y = y2;
            clipPath.lineTo(x, y);
        }

        // rb

        const auto isRightBottomRoundingEnabled = ((flags & RenderBubble_RightBottomRounded) != 0);
        if (isRightBottomRoundingEnabled)
        {
            x = rightMinusBorder;
            y = y2;
            clipPath.lineTo(x, y);

            y -= borderDiameter;
            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 270, 90);
        }
        else
        {
            x = x2;
            y = y2;
            clipPath.lineTo(x, y);
        }

        // rt

        const auto isRightTopRoundingEnabled = ((flags & RenderBubble_RightTopRounded) != 0);
        if (isRightTopRoundingEnabled)
        {
            x = x2;
            y = (rect.top() + borderDiameter);
            clipPath.lineTo(x, y);

            x -= borderDiameter;
            y -= borderDiameter;
            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 0, 90);
        }
        else
        {
            x = x2;
            y = rect.top();

            clipPath.lineTo(x, y);
        }

        // the final closure

        if (isLeftBottomRoundingEnabled)
        {
            x = rect.left();
            y = rect.top();
            clipPath.lineTo(x, y);
        }
        else
        {
            x = (rect.left() + borderRadius);
            y = rect.top();
            clipPath.lineTo(x, y);
        }

        return clipPath;
    }

}