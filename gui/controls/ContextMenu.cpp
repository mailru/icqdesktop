#include "stdafx.h"

#include "ContextMenu.h"

#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    const QString MENU_STYLE =
        "QMenu { background-color: #ffffff; border: 1px solid #d7d7d7; }"
        "QMenu::item { background-color: transparent;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::item:selected { background-color: #ebebeb;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::item:disabled { background-color: transparent; color: #999999;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::item:disabled:selected { background-color: transparent; color: #999999;"
        "height: %2; padding-left: %1; padding-right: 12dip; }"
        "QMenu::icon { padding-left: 22dip; }";
}

namespace Ui
{
    int MenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        int s = QProxyStyle::pixelMetric(_metric, _option, _widget);
        if (_metric == QStyle::PM_SmallIconSize)
        {
            s = Utils::scale_value(20);
        }
        return s;
    }

    ContextMenu::ContextMenu(QWidget* _parent)
        : QMenu(_parent)
        , InvertRight_(false)
        , Indent_(0)
    {
        applyStyle(this, true, Utils::scale_value(16), Utils::scale_value(36));
    }

    void ContextMenu::applyStyle(QMenu* menu, bool withPadding, int fonSize, int height)
    {
        menu->setWindowFlags(menu->windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);
        menu->setStyle(new MenuStyle());
        QString style = MENU_STYLE.arg(withPadding ? Utils::scale_value(40) : Utils::scale_value(12)).arg(height);
        Utils::ApplyStyle(menu, style);
        auto font = Fonts::appFont(fonSize);
        menu->setFont(font);
        if (platform::is_apple())
        {
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupMenu()), menu, SLOT(close()));
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data)
    {
        QAction* action = addAction(_icon, _name);
        action->setData(_data);
        return action;
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QObject* _receiver, const char* _member)
    {
        return addAction(_icon, _name, _receiver, _member);
    }

    QAction* ContextMenu::addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        return addActionWithIcon(QIcon(Utils::parse_image_name(_iconPath)), _name, _data);
    }

    bool ContextMenu::hasAction(const QString& _command)
    {
        assert(!_command.isEmpty());

        for (const auto action : actions())
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find("command");
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                return true;
            }
        }

        return false;
    }

    void ContextMenu::removeAction(const QString& _command)
    {
        assert(!_command.isEmpty());

        for (auto action : actions())
        {
            const auto actionParams = action->data().toMap();

            const auto commandIter = actionParams.find("command");
            if (commandIter == actionParams.end())
            {
                continue;
            }

            const auto actionCommand = commandIter->toString();
            if (actionCommand == _command)
            {
                QWidget::removeAction(action);

                return;
            }
        }
    }

    void ContextMenu::invertRight(bool _invert)
    {
        InvertRight_ = _invert;
    }

    void ContextMenu::setIndent(int _indent)
    {
        Indent_ = _indent;
    }

	void ContextMenu::popup(const QPoint& _pos, QAction* _at)
	{
		Pos_ = _pos;
		QMenu::popup(_pos, _at);
	}

    void ContextMenu::clear()
    {
        QMenu::clear();
    }

    void ContextMenu::hideEvent(QHideEvent* _e)
    {
        QMenu::hideEvent(_e);
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
		if (InvertRight_ || Indent_ != 0)
		{
			QPoint p;
			if (pos().x() != Pos_.x())
				p = Pos_;
			else
				p = pos();

			if (InvertRight_)
				p.setX(p.x() - width() - Indent_);
			else
				p.setX(p.x() + Indent_);
			move(p);
		}
    }

    void ContextMenu::focusOutEvent(QFocusEvent *_e)
    {
        if (parentWidget() && !parentWidget()->isActiveWindow())
        {
            close();
            return;
        }
        QMenu::focusOutEvent(_e);
    }
    
}
