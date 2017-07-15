#include "stdafx.h"
#include "ToolTipEx.h"
#include "../utils/utils.h"


#define TOOLTIP_BORDER         Utils::scale_value(16)
#define TOOLTIP_ARROW_WIDHT    Utils::scale_value(20)
#define TOOLTIP_ARROW_HEIGHT   Utils::scale_value(10)
#define TOOLTIP_ARROW_LEFT_OFFSET   Utils::scale_value(24)

const QString TOOLTIP_STYLE =
    "QLabel { font-size: 14dip; background-color: #ffffff;"
    "padding-left: 14dip; padding-bottom: 14dip; } ";

class ToolTipLabel : public QLabel
{
public:
	ToolTipLabel(QWidget* parent);

protected:

	virtual void paintEvent(QPaintEvent* _e) override;
};



QPolygon getPolygon(int width, int height)
{
	// Setup form.
	//  ____________
	//  |           |
	//  |__  _______|
	//     \/
	//
	int polygon[7][2] = { 0 };
	polygon[0][0] = 0;
	polygon[0][1] = 0;

	polygon[1][0] = 0;
	polygon[1][1] = height - TOOLTIP_ARROW_HEIGHT;

	polygon[2][0] = TOOLTIP_ARROW_LEFT_OFFSET;
	polygon[2][1] = polygon[1][1];

	polygon[3][0] = polygon[2][0] + TOOLTIP_ARROW_WIDHT / 2;
	polygon[3][1] = polygon[2][1] + TOOLTIP_ARROW_HEIGHT;

	polygon[4][0] = polygon[2][0] + TOOLTIP_ARROW_WIDHT;
	polygon[4][1] = polygon[2][1];

	polygon[5][0] = width;
	polygon[5][1] = height - TOOLTIP_ARROW_HEIGHT;

	polygon[6][0] = width;
	polygon[6][1] = 0;

	QPolygon arrow;
	arrow.setPoints(7, &polygon[0][0]);
	return arrow;
}


Ui::ToolTipEx::ToolTipEx(QWidget* parent) : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);  
	
	textLabel = new ToolTipLabel(this);
	textLabel->setText("");

    Utils::ApplyStyle(textLabel, TOOLTIP_STYLE);
	setStyle(QApplication::style());
	textLabel->setStyle(QApplication::style());

	updateMask();
}


Ui::ToolTipEx::~ToolTipEx(void)
{

}

void Ui::ToolTipEx::updateMask()
{
	// Set correct size.
	QFont font = textLabel->font();
	font.setPixelSize(Utils::scale_value(14));
	QFontMetrics fm(font);
	QSize lableRect = fm.size(Qt::TextSingleLine, textLabel->text());
	QSize border(TOOLTIP_BORDER, TOOLTIP_BORDER);
	textLabel->setFixedSize(lableRect + 2 * border);

	updatePosition();

	QPolygon arrow = getPolygon(width(), height());

    QPainterPath path(QPointF(0, 0));
    path.addPolygon(arrow);

    QRegion region(path.toFillPolygon().toPolygon());
    setMask(region);
}

void  Ui::ToolTipEx::setText(const QString& text)
{
	textLabel->setText(text);

	updateMask();
}

void Ui::ToolTipEx::showEvent(QShowEvent* _e)
{
	updateMask();
}

void Ui::ToolTipEx::updatePosition()
{
	// ToolTip position is abow of parent window.
	QPoint tooltipPosition = (parentWidget()->mapToGlobal(QPoint(0, 0)) + QPoint(parentWidget()->width() / 2, 0)) - QPoint(TOOLTIP_ARROW_LEFT_OFFSET + TOOLTIP_ARROW_WIDHT / 2, height());

	move(tooltipPosition);
}


ToolTipLabel::ToolTipLabel(QWidget* parent) : QLabel (parent) {}

void ToolTipLabel::paintEvent(QPaintEvent* _e)
{
	QLabel::paintEvent(_e);

	if (parentWidget())
	{
		// Draw the border.
		QPainterPath path;
		QPolygon polygon = getPolygon(width(), height());

		// the closure of the polygon.
		polygon.push_back(polygon.point(0));

		path.addPolygon(polygon);

		QPainter painter(this);
        painter.strokePath(path, QPen(QColor("#d7d7d7"), Utils::scale_value(2)));
	}
}