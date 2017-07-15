#include "stdafx.h"
#include "CommonStyle.h"

#include "../utils/utils.h"

UI_COMMON_STYLE_NS_BEGIN

QString getCloseButtonStyle()
{
    const QString close_button_style =
        "QPushButton {"
        "background-color: transparent;"
        "background-image: url(:/resources/main_window/contr_close_100.png);"
        "background-position: center; background-repeat: no-repeat; border: none;"
        "padding: 0; margin: 0;"
        "width: 46dip; height: 36dip; }"
        "QPushButton:hover {"
        "background-image: url(:/resources/main_window/contr_close_100_hover.png);"
        "background-color: #e81123; }"
        "QPushButton:hover:pressed { background-color: #e81123; }"
        "QPushButton:focus { outline: none; }";
    return close_button_style;
}

QString getMinimizeButtonStyle()
{
    const QString str =
        "QPushButton {"
        "background-color: transparent;"
        "background-image: url(:/resources/main_window/contr_minimize_100.png);"
        "background-position: center; background-repeat: no-repeat; border: none;"
        "padding: 0; margin: 0;"
        "width: 46dip; height: 36dip; }"
        "QPushButton:hover { background-color: #d3d3d3; }"
        "QPushButton:hover:pressed { background-color: #c8c8c8; }"
        "QPushButton:focus { outline: none; }";
    return str;
}

QString getMaximizeButtonStyle()
{
    const QString str =
        "QPushButton {"
        "background-color: transparent;"
        "background-image: url(:/resources/main_window/contr_bigwindow_100.png);"
        "background-position: center; background-repeat: no-repeat; border: none;"
        "padding: 0; margin: 0;"
        "width: 46dip; height: 36dip; }"
        "QPushButton:hover { background-color: #d3d3d3; }"
        "QPushButton:hover:pressed { background-color: #c8c8c8; }"
        "QPushButton:focus { outline: none; }";
    return str;
}
QString getRestoreButtonStyle()
{
    const QString str =
        "QPushButton { "
        "background-color: transparent;"
        "background-image: url(:/resources/main_window/contr_smallwindow_100.png);"
        "background-position: center; background-repeat: no-repeat; border: none;"
        "padding: 0; margin: 0;"
        "width: 46dip; height: 36dip; }"
        "QPushButton:hover { background-color: #d3d3d3; }"
        "QPushButton:hover:pressed { background-color: #c8c8c8; }"
        "QPushButton:focus { outline: none; }";
    return str;
}

const QColor getContactListHoveredColor() { return QColor("#ebebeb"); }
const QColor getContactListSelectedColor() { return QColor("#84b858"); }

QString getDisabledButtonStyle()
{
    return QString(
        "QPushButton {"
        "color: #ffffff;"
        "font-size: 16dip;"
        "background-color: #cbcbcb;"
        "border-style: none;"
        "margin: 0;"
        "padding-left: 20dip; padding-right: 20dip;"
        "min-width: 100dip;"
        "max-height: 32dip; min-height: 32dip; }"
        "QPushButton:focus { outline: none; }"
    );
}

QString getGrayButtonStyle()
{
    return QString(
        "QPushButton {"
        "font-size: 16dip;"
        "background-color: #d7d7d7;"
        "border-style: none;"
        "margin: 0;"
        "padding-left: 20dip; padding-right: 20dip;"
        "min-width: 100dip;"
        "max-height: 32dip; min-height: 32dip; }"
        "QPushButton:hover { background-color: #ebebeb; }"
        "QPushButton:focus { outline: none; }"
    );
}

QString getGreenButtonStyle()
{
    return QString(
        "QPushButton {"
            "color: #ffffff;"
            "font-size: 16dip;"
            "background-color: #579e1c;"
            "border-style: none;"
            "margin: 0;"
            "padding-left: 20dip; padding-right: 20dip;"
            "min-width: 100dip;"
            "max-height: 32dip; min-height: 32dip;"
            "text-align: center; }"
        "QPushButton:hover { background-color: #57a813; }"
        "QPushButton:pressed { background-color: #498812; }"
        "QPushButton:focus { outline: none; }"
    );
}

const QColor getFrameColor() { return QColor("#ffffff"); }
const int getBottomPanelHeight() { return  Utils::scale_value(64); }
const QColor getBottomPanelColor() { return QColor("#ffffff"); }
const int getTopPanelHeight() { return  Utils::scale_value(56); }
const QColor getTopPanelColor() { return QColor("#ffffff"); }

const QColor getLinkColor() { return QColor("#579e1c"); }
const QColor getLinkColorHovered() { return QColor("#57a813"); }
const QColor getLinkColorPressed() { return QColor("#489b12"); }

const QColor getRedLinkColor() { return QColor("#d0021b"); }
const QColor getRedLinkColorHovered() { return QColor("#e81123"); }

const QColor getTextCommonColor() { return QColor("#000000"); }

QString getLineEditStyle()
{
    return QString(
        "QLineEdit {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: #d7d7d7;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        "QLineEdit:focus {"
            "min-height: 48dip; max-height: 48dip;"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: #579e1c;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
    );
}

QString getTextEditStyle()
{
    return QString(
        "QTextBrowser {"
        "background-color: transparent;"
        "border-style: none;"
        "border-bottom-color: #d7d7d7;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid;}"
        "QTextBrowser:focus {"
        "background-color: transparent;"
        "border-style: none;"
        "border-bottom-color: #579e1c;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }"
        );
}

QString getLineEditErrorStyle()
{
    return QString(
        "QLineEdit {"
        "min-height: 48dip; max-height: 48dip;"
        "background-color: transparent;"
        "color: #d0021b;"
        "border-style: none;"
        "border-bottom-color: #d0021b;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }"
        "QLineEdit:focus {"
        "min-height: 48dip; max-height: 48dip;"
        "background-color: transparent;"
        "color: #d0021b;"
        "border-style: none;"
        "border-bottom-color: #d0021b;"
        "border-bottom-width: 1dip;"
        "border-bottom-style: solid; }");
}

UI_COMMON_STYLE_NS_END
