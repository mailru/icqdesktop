#include "stdafx.h"

#include "HistoryButtonDown.h"
#include "../../utils/utils.h"
#include "../../fonts.h"

UI_NS_BEGIN

HistoryButtonDown::HistoryButtonDown(QWidget* _parent, const QString& _imageName) :
	CustomButton(_parent, _imageName),
	numUnreads_(0)
{
}

HistoryButtonDown::HistoryButtonDown(QWidget* _parent, const QPixmap& _pixmap) :
	CustomButton(_parent, _pixmap),
	numUnreads_(0)
{
}

void HistoryButtonDown::paintEvent(QPaintEvent *_event)
{
	CustomButton::paintEvent(_event);

	if (numUnreads_ > 0)
	{
		/// paint circle
		QPainter painter(this);
		QPoint size = Utils::getUnreadsSize(
					&painter,
					Fonts::appFontScaled(13, Fonts::FontWeight::Medium),
					true,
					numUnreads_,
					Utils::scale_value(16));

		int btn_size = Utils::scale_value(40);
		int area_size = Utils::scale_value(60);
		int end_x = area_size/2 + btn_size/2 + Utils::scale_value(numUnreads_ > 9 ? 8 : 3);
		int begin_x = end_x - size.x();

		int end_y = area_size/2 + btn_size/2 -  + Utils::scale_value(27);
		int begin_y = end_y - size.y();

		const auto borderColor = QColor("transparent");
		const auto bgColor = QColor("#579e1c");
		const auto textColor = QColor("#ffffff");
    
		Utils::drawUnreads(
					&painter, Fonts::appFontScaled(13, Fonts::FontWeight::Medium),
					&bgColor,
					&textColor,
					&borderColor,
					numUnreads_,
					Utils::scale_value(/*balloon_size*/16),
					begin_x,
					begin_y
				);
	}
}

void HistoryButtonDown::leaveEvent(QEvent *_event)
{
	CustomButton::leaveEvent(_event);
}

void HistoryButtonDown::enterEvent(QEvent *_event)
{
	CustomButton::enterEvent(_event);
}

void HistoryButtonDown::mousePressEvent(QMouseEvent *_event)
{
	CustomButton::mousePressEvent(_event);
}

void HistoryButtonDown::mouseReleaseEvent(QMouseEvent *_event)
{
	CustomButton::mouseReleaseEvent(_event);
}

void HistoryButtonDown::setUnreadMessages(int num_unread)
{
	numUnreads_ = num_unread;
	repaint();
}

void HistoryButtonDown::addUnreadMessages(int num_add)
{
	numUnreads_ += num_add;
	repaint();
}

void HistoryButtonDown::wheelEvent(QWheelEvent * _event)
{
    emit sendWheelEvent(_event);
}

UI_NS_END