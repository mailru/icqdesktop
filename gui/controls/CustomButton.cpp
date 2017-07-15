#include "stdafx.h"
#include "CustomButton.h"
#include "../utils/utils.h"

#define NO_VALUE -100

Ui::CustomButton::CustomButton(QWidget* _parent, const QString& _imageName)
    : QPushButton(_parent)
    , activeState_(false)
{
	init();
    auto px = QPixmap(Utils::parse_image_name(_imageName));
    if (!px.isNull())
    {
        pixmapDefault_ = px;
        pixmapToDraw_ = pixmapDefault_;
    }
    setFlat(true);
    QString stylesheet = "QPushButton:pressed { background-color: transparent; } "
                         "QPushButton:focus { border: none; outline: none; }";
    setStyleSheet(stylesheet);

};

Ui::CustomButton::CustomButton(QWidget* _parent, const QPixmap& _pixmap) : QPushButton(_parent)
{
    init();
    pixmapDefault_ = QPixmap(_pixmap);
    pixmapToDraw_ = pixmapDefault_;
    setFlat(true);
}

void Ui::CustomButton::init()
{
    x_ = 0;
    y_ = 0;
    xForActive_ = NO_VALUE;
    yForActive_ = NO_VALUE;
    align_ = Qt::AlignCenter;
    fillColor_ = QColor();
}

void Ui::CustomButton::paintEvent(QPaintEvent *_e)
{
    QPushButton::paintEvent(_e);

    QPainter painter(this);
	if (fillColor_.isValid())
	{
		painter.fillRect(rect(), fillColor_);
	}
    Utils::check_pixel_ratio(pixmapToDraw_);
    Utils::check_pixel_ratio(pixmapDisabled_);
    double ratio = Utils::scale_bitmap(1);
    int x = (this->rect().width() / 2) - (pixmapToDraw_.width() / 2. / ratio);
    int y = (this->rect().height() / 2) - (pixmapToDraw_.height() / 2. / ratio);
    if ((align_ & Qt::AlignLeft) == Qt::AlignLeft)
        x = 0;
    else if ((align_ & Qt::AlignRight) == Qt::AlignRight)
        x = this->rect().width() - (pixmapToDraw_.width() / ratio);
    if ((align_ & Qt::AlignTop) == Qt::AlignTop)
        y = 0;
    else if ((align_ & Qt::AlignBottom) == Qt::AlignBottom)
        y = this->rect().height() - (pixmapToDraw_.height() / ratio);
	
	if (activeState_ && xForActive_ != NO_VALUE && yForActive_ != NO_VALUE)
	{
		x += xForActive_;
		y += yForActive_;
	}
	else
	{
		x += x_;
		y += y_;
	}

    painter.drawPixmap(x, y, (!isEnabled() && !pixmapDisabled_.isNull()) ? pixmapDisabled_ : pixmapToDraw_);
}

void Ui::CustomButton::leaveEvent(QEvent* _e)
{
    if (!pixmapHover_.isNull() && !activeState_)
    {
        pixmapToDraw_ = pixmapDefault_;
        QPushButton::leaveEvent(_e);
    }
    update();
}

void Ui::CustomButton::enterEvent(QEvent* _e)
{
    if (!pixmapHover_.isNull() && !activeState_)
    {
        pixmapToDraw_ = pixmapHover_;
        QPushButton::enterEvent(_e);
    }
    update();
}

void Ui::CustomButton::setMenu(QMenu* menu)
{
    QPushButton::setMenu(menu);
}

void Ui::CustomButton::mousePressEvent(QMouseEvent * _e)
{
    if (!pixmapPressed_.isNull())
    {
        pixmapToDraw_ = pixmapPressed_;
        update();
    }
    QPushButton::mousePressEvent(_e);
}

void Ui::CustomButton::mouseReleaseEvent(QMouseEvent * _e)
{
    if (!pixmapPressed_.isNull())
    {
        pixmapToDraw_ = pixmapHover_;
        update();
    }
    QPushButton::mouseReleaseEvent(_e);
}

void Ui::CustomButton::setAlign(int _flags)
{
    align_ = _flags;
}

void Ui::CustomButton::setOffsets(int _x, int _y)
{
    x_ = _x;
    y_ = _y;
    update();
}

void Ui::CustomButton::setOffsetsForActive(int _x, int _y)
{
	xForActive_ = _x;
	yForActive_ = _y;
	update();
}

void Ui::CustomButton::setImage(const QString& _imageName)
{
    setImage(QPixmap(Utils::parse_image_name(_imageName)));
}

void Ui::CustomButton::setHoverImage(const QString& _imageName)
{
    setHoverImage(QPixmap(Utils::parse_image_name(_imageName)));
}

void Ui::CustomButton::setActiveImage(const QString& _imageName)
{
    setActiveImage(QPixmap(Utils::parse_image_name(_imageName)));
}

void Ui::CustomButton::setDisabledImage(const QString& _imageName)
{
    setDisabledImage(QPixmap(Utils::parse_image_name(_imageName)));
}

void Ui::CustomButton::setPressedImage(const QString& _imageName)
{
    setPressedImage(QPixmap(Utils::parse_image_name(_imageName)));
}

void Ui::CustomButton::setImage(const QPixmap& _px)
{
    if (!_px.isNull())
    {
        pixmapDefault_ = _px;//platform::is_apple() ? px.scaled(Utils::scale_value(px.width()), Utils::scale_value(px.width()), Qt::KeepAspectRatio, Qt::SmoothTransformation) : px;
        pixmapToDraw_ = pixmapDefault_;
    }
    update();
}

void Ui::CustomButton::setHoverImage(const QPixmap& _px)
{
    if (!_px.isNull())
        pixmapHover_ = _px;//platform::is_apple() ? px.scaled(Utils::scale_value(px.width()), Utils::scale_value(px.width()), Qt::KeepAspectRatio, Qt::SmoothTransformation) : px;
}

void Ui::CustomButton::setActiveImage(const QPixmap& _px)
{
    if (!_px.isNull())
        pixmapActive_ = _px;//platform::is_apple() ? px.scaled(Utils::scale_value(px.width()), Utils::scale_value(px.width()), Qt::KeepAspectRatio, Qt::SmoothTransformation) : px;
}

void Ui::CustomButton::setDisabledImage(const QPixmap& _px)
{
    if (!_px.isNull())
        pixmapDisabled_ = _px;//platform::is_apple() ? px.scaled(Utils::scale_value(px.width()), Utils::scale_value(px.width()), Qt::KeepAspectRatio, Qt::SmoothTransformation) : px;
}

void Ui::CustomButton::setPressedImage(const QPixmap& _px)
{
    if (!_px.isNull())
        pixmapPressed_ = _px;//platform::is_apple() ? px.scaled(Utils::scale_value(px.width()), Utils::scale_value(px.width()), Qt::KeepAspectRatio, Qt::SmoothTransformation) : px;
}

void Ui::CustomButton::setActive(bool _isActive)
{
    activeState_ = _isActive;

    if (pixmapActive_.isNull())
    {
        return;
    }
    if (_isActive)
    {
        pixmapToDraw_ = pixmapActive_;
    }
    else
    {
        pixmapToDraw_ = pixmapDefault_;
    }
    update();
}

void Ui::CustomButton::setFillColor(QColor _color)
{
	fillColor_ = _color;
	update();
}

void Ui::CustomButton::setTextColor(const QString& _color)
{
    QString stylesheet = QString(
        "QPushButton { color: %1 } "
        "QPushButton:pressed { background-color: transparent; }"
        "QPushButton:focus { border: none; outline: none; }"
        ).arg(_color);

    setStyleSheet(stylesheet);
    setStyle(QApplication::style());
}

QSize Ui::CustomButton::sizeHint() const
{
    QFontMetrics fm(font());
    int width = fm.width(text());
    int height = fm.height();
    return QSize(width, height);
}

