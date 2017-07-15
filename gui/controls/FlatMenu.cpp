#include "stdafx.h"

#include "../fonts.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

#include "FlatMenu.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const QString MENU_STYLE =
        "QMenu { background-color: #ffffff; border: 1px solid #d7d7d7; }"
        "QMenu::item { background-color: transparent;"
        "height: 36dip; padding-right: 12dip; }"
        "QMenu::item:selected { background-color: #ebebeb;"
        "height: 36dip; padding-right: 12dip; }";
}

namespace Ui
{
    int FlatMenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        int s = QProxyStyle::pixelMetric(_metric, _option, _widget);
        if (_metric == QStyle::PM_SmallIconSize)
        {
            s = Utils::scale_value(24);
        }
        return s;
    }

    int FlatMenu::shown_ = 0;

    FlatMenu::FlatMenu(QWidget* _parent/* = nullptr*/)
        :QMenu(_parent),
        iconSticked_(false)
    {
        setWindowFlags(windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        setStyle(new FlatMenuStyle());
        Utils::ApplyStyle(this, MENU_STYLE);

        QFont font = Fonts::appFontScaled(16);
        setFont(font);
        if (platform::is_apple())
        {
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupMenu()), this, SLOT(close()));
        }
    }

    FlatMenu::~FlatMenu()
    {
        //
    }

    void FlatMenu::paintEvent(QPaintEvent* _event)
    {
        QMenu::paintEvent(_event);
        QPainter p(this);
        p.setClipRect(0, 0, geometry().width(), geometry().height());
        setMask(p.clipRegion());
    }

    void FlatMenu::hideEvent(QHideEvent* _event)
    {
        QMenu::hideEvent(_event);
        --shown_;
    }

    void FlatMenu::showEvent(QShowEvent* _event)
    {
//        if (parentWidget() && !parentWidget()->isActiveWindow())
//        {
//            close();
//            return;
//        }
        if (!parentWidget())
        {
            return QMenu::showEvent(_event);
        }
        auto posx = pos().x();
        auto posy = pos().y();
        if (iconSticked_)
        {
            if (auto button = qobject_cast<QPushButton *>(parentWidget()))
            {
                posx -= ((button->size().width() - button->iconSize().width()) / 2);
                posy -= ((button->size().height() - button->iconSize().height()) / 2);
                move(posx, posy);
            }
        }
        if ((expandDirection_ & Qt::AlignLeft) == Qt::AlignLeft)
        {
            move(pos().x() + parentWidget()->geometry().width() - geometry().width(), pos().y());
        }
        if ((expandDirection_ & Qt::AlignTop) == Qt::AlignTop)
        {
            move(pos().x(), pos().y() - parentWidget()->geometry().height() - geometry().height());
        }
        ++shown_;
    }

    void FlatMenu::setExpandDirection(Qt::Alignment _direction)
    {
        expandDirection_ = _direction;
    }

    void FlatMenu::stickToIcon()
    {
        iconSticked_ = true;
    }

}
