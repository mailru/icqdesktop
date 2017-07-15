#pragma once

#define UI_MESSAGE_STYLE_NS_BEGIN namespace Ui { namespace MessageStyle {
#define UI_MESSAGE_STYLE_NS_END } }

UI_MESSAGE_STYLE_NS_BEGIN

QFont getTextFont(int size = -1);

QColor getTextColor(double opacity = 1.0);

QColor getTimeColor();
QColor getChatEventColor();
QColor getTypingColor();

QColor getSenderColor();
QFont getSenderFont();

QFont getTimeFont();

int32_t getTimeMarginX();
int32_t getTimeMarginY();

int32_t getTimeMaxWidth();

QColor getIncomingBodyColorA();

QColor getOutgoingBodyColorA();

QColor getIncomingBodyColorB();

QColor getOutgoingBodyColorB();

QString getLinkColorS();

QColor getLinkColor();

QString getMessageStyle();

QBrush getBodyBrush(const bool isOutgoing, const bool isSelected, const int theme_id);

int32_t getMinBubbleHeight();

int32_t getBorderRadius();

int32_t getTopMargin(const bool hasTopMargin);

int32_t getLeftMargin(const bool isOutgoing);

int32_t getRightMargin(const bool isOutgoing);

int32_t getAvatarSize();

int32_t getAvatarRightMargin();

int32_t getBubbleHorPadding();

int32_t getLastReadAvatarSize();

int32_t getLastReadAvatarMargin();

int32_t getHistoryWidgetMaxWidth();

QFont getRotatingProgressBarTextFont();

QPen getRotatingProgressBarTextPen();

int32_t getRotatingProgressBarTextTopMargin();

int32_t getRotatingProgressBarPenWidth();

QPen getRotatingProgressBarPen();

int32_t getSenderBottomMargin();

int32_t getSenderHeight();

int32_t getTextWidthStep();

int32_t roundTextWidthDown(const int32_t width);

int32_t getSnippetMaxWidth();

UI_MESSAGE_STYLE_NS_END