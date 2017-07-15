#pragma once

#define UI_COMMON_STYLE_NS_BEGIN namespace Ui { namespace CommonStyle {
#define UI_COMMON_STYLE_NS_END } }

UI_COMMON_STYLE_NS_BEGIN

QString getCloseButtonStyle();
QString getMinimizeButtonStyle();
QString getMaximizeButtonStyle();
QString getRestoreButtonStyle();

const QColor getContactListHoveredColor();
const QColor getContactListSelectedColor();

QString getDisabledButtonStyle();
QString getGrayButtonStyle();
QString getGreenButtonStyle();

const QColor getFrameColor();
const int getBottomPanelHeight();
const QColor getBottomPanelColor();
const int getTopPanelHeight();
const QColor getTopPanelColor();

const QColor getLinkColor();
const QColor getLinkColorHovered();
const QColor getLinkColorPressed();

const QColor getRedLinkColor();
const QColor getRedLinkColorHovered();

const QColor getTextCommonColor();

QString getLineEditStyle();
QString getTextEditStyle();
QString getLineEditErrorStyle();


UI_COMMON_STYLE_NS_END
