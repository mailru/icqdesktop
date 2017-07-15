#include "stdafx.h"

#include "MessageStyle.h"

#include "../../cache/themes/themes.h"
#include "../../fonts.h"
#include "../../theme_settings.h"
#include "../../utils/utils.h"

UI_MESSAGE_STYLE_NS_BEGIN

QFont getTextFont(int size)
{
    const int defaultFontSize = platform::is_linux() ? 16 : 15;

    const int fontSize = size == -1 ? defaultFontSize : size;

    return Fonts::appFontScaled(fontSize);
}

QColor getTextColor(double opacity)
{
    QColor textColor("#000000");
    textColor.setAlphaF(opacity);
    return textColor;
}

QColor getTimeColor()
{
    return QColor("#999999");
}

QColor getChatEventColor()
{
    return QColor("#999999");
}

QColor getTypingColor()
{
    return QColor("#454545");
}

QColor getSenderColor()
{
    return QColor("#454545");
}

QFont getSenderFont()
{
    return Fonts::appFontScaled(12);
}

QFont getTimeFont()
{
    return Fonts::appFontScaled(10, platform::is_apple() ? Fonts::FontWeight::Normal : Fonts::FontWeight::Medium);
}

int32_t getTimeMarginX()
{
    return Utils::scale_value(4);
}
int32_t getTimeMarginY()
{
    return Utils::scale_value(4);
}

int32_t getTimeMaxWidth()
{
    return Utils::scale_value(32);
}

QColor getIncomingBodyColorA()
{
    return QColor("#ffffff");
}

QColor getOutgoingBodyColorA()
{
    QColor outgoingBodyColorA("#d8d4ce");
    outgoingBodyColorA.setAlphaF(0.9);
    return outgoingBodyColorA;
}

QColor getIncomingBodyColorB()
{
    QColor incomingBodyColorB("#ffffff");
    incomingBodyColorB.setAlphaF(0.72);
    return incomingBodyColorB;
}

QColor getOutgoingBodyColorB()
{
    QColor outgoingBodyColorB("#d5d2ce");
    outgoingBodyColorB.setAlphaF(0.72);
    return outgoingBodyColorB;
}

QString getLinkColorS()
{
    return "#579e1c";
}

QColor getLinkColor()
{
    return QColor(getLinkColorS());
}

QString getMessageStyle()
{
    return QString("a {text-decoration:none;color:") + MessageStyle::getLinkColorS() + ";}";
}

QBrush getBodyBrush(
    const bool isOutgoing,
    const bool isSelected,
    const int _theme_id)
{
    auto _theme = get_qt_theme_settings()->themeForId(_theme_id);

    QLinearGradient grad(0, 0, 1, 0);

    grad.setCoordinateMode(QGradient::ObjectBoundingMode);

    QColor selectionColor("#579e1c");
    auto leftOpacity = isOutgoing ? 0.9 : 1.0;
    auto rightOpacity = 0.72;

    QColor selectedColor0 = selectionColor;
    selectedColor0.setAlphaF(leftOpacity);
    QColor selectedColor1 = selectionColor;
    selectedColor1.setAlphaF(rightOpacity);

    if (isSelected)
    {
        const auto color0 = selectedColor0;
        grad.setColorAt(0, color0);

        const auto color1 = selectedColor1;
        grad.setColorAt(1, color1);
    }
    else
    {
        const auto color0 = isOutgoing ? _theme->outgoing_bubble_.bg1_color_ : _theme->incoming_bubble_.bg1_color_;
        grad.setColorAt(0, color0);

        const auto color1 = isOutgoing ? _theme->outgoing_bubble_.bg2_color_ : _theme->incoming_bubble_.bg2_color_;
        grad.setColorAt(1, color1);
    }

    QBrush result(grad);
    result.setColor(Qt::transparent);

    return result;
}

int32_t getMinBubbleHeight()
{
    return Utils::scale_value(32);
}

int32_t getBorderRadius()
{
    return Utils::scale_value(8);
}

int32_t getTopMargin(const bool hasTopMargin)
{
    return Utils::scale_value(
        hasTopMargin ? 12 : 2
    );
}

int32_t getLeftMargin(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 118 : 24
    );
}

int32_t getRightMargin(const bool isOutgoing)
{
    return Utils::scale_value(
        isOutgoing ? 8 : 72
    );
}

int32_t getAvatarSize()
{
    return Utils::scale_value(32);
}

int32_t getAvatarRightMargin()
{
    return Utils::scale_value(6);
}

int32_t getBubbleHorPadding()
{
    return Utils::scale_value(16);
}

int32_t getLastReadAvatarSize()
{
    return Utils::scale_value(16);
}

int32_t getLastReadAvatarMargin()
{
    return Utils::scale_value(4);
}

int32_t getHistoryWidgetMaxWidth()
{
    return Utils::scale_value(640);
}

int32_t getSenderHeight()
{
    return Utils::scale_value(16);
}

QFont getRotatingProgressBarTextFont()
{
    using namespace Utils;

    return Fonts::appFontScaled(15);
}

QPen getRotatingProgressBarTextPen()
{
    return QPen(QColor("#ffffff"));
}

int32_t getRotatingProgressBarTextTopMargin()
{
    return Utils::scale_value(16);
}

int32_t getRotatingProgressBarPenWidth()
{
    return Utils::scale_value(2);
}

QPen getRotatingProgressBarPen()
{
    return QPen(
        QColor("#579e1c"),
        getRotatingProgressBarPenWidth());
}

int32_t getSenderBottomMargin()
{
    return Utils::scale_value(4);
}

int32_t getTextWidthStep()
{
    return Utils::scale_value(20);
}

int32_t roundTextWidthDown(const int32_t width)
{
    assert(width > 0);

    return ((width / getTextWidthStep()) * getTextWidthStep());
}

int32_t getSnippetMaxWidth()
{
    return Utils::scale_value(420);
}

UI_MESSAGE_STYLE_NS_END