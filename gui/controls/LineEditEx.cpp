#include "stdafx.h"
#include "LineEditEx.h"
#include "ContextMenu.h"
#include "../utils/utils.h"


namespace Ui
{
	LineEditEx::LineEditEx(QWidget* _parent)
		: QLineEdit(_parent)
	{

	}

	void LineEditEx::focusInEvent(QFocusEvent* _event)
	{
		emit focusIn();
		QLineEdit::focusInEvent(_event);
	}

	void LineEditEx::focusOutEvent(QFocusEvent* _event)
	{
		emit focusOut();
		QLineEdit::focusOutEvent(_event);
	}

	void LineEditEx::mousePressEvent(QMouseEvent* _event)
	{
		emit clicked();
		QLineEdit::mousePressEvent(_event);
	}

	void LineEditEx::keyPressEvent(QKeyEvent* _event)
	{
		if (_event->key() == Qt::Key_Backspace && text().isEmpty())
			emit emptyTextBackspace();

		if (_event->key() == Qt::Key_Escape)
        {
			emit escapePressed();
            return;
        }

		if (_event->key() == Qt::Key_Up)
			emit upArrow();

		if (_event->key() == Qt::Key_Down)
			emit downArrow();

		if (_event->key() == Qt::Key_Return && _event->modifiers() == Qt::NoModifier)
			emit enter();

		QLineEdit::keyPressEvent(_event);
	}

    void LineEditEx::contextMenuEvent(QContextMenuEvent *e)
    {
        auto menu = createStandardContextMenu();
        ContextMenu::applyStyle(menu, false, Utils::scale_value(14), Utils::scale_value(24));
        menu->exec(e->globalPos());
        menu->deleteLater();
    }

}
