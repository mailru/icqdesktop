#include "stdafx.h"
#include "../utils/utils.h"
#include "PictureWidget.h"

namespace Ui
{

    PictureWidget::PictureWidget(QWidget* _parent, const QString& _imageName)
        : QWidget(_parent), x_(0), y_(0), align_(Qt::AlignCenter)
    {
        if (!_imageName.isEmpty())
            setImage(_imageName);
    };

    void PictureWidget::paintEvent(QPaintEvent *_e)
    {
        QWidget::paintEvent(_e);

        if (pixmapToDraw_.isNull())
            return;

        QPainter painter(this);
        Utils::check_pixel_ratio(pixmapToDraw_);
        double ratio = Utils::scale_bitmap(1);
        int x = (rect().width() / 2) - (pixmapToDraw_.width() / 2. / ratio);
        int y = (rect().height() / 2) - (pixmapToDraw_.height() / 2. / ratio);
        if ((align_ & Qt::AlignLeft) == Qt::AlignLeft)
            x = 0;
        else if ((align_ & Qt::AlignRight) == Qt::AlignRight)
            x = rect().width() - (pixmapToDraw_.width() / ratio);
        if ((align_ & Qt::AlignTop) == Qt::AlignTop)
            y = 0;
        else if ((align_ & Qt::AlignBottom) == Qt::AlignBottom)
            y = rect().height() - (pixmapToDraw_.height() / ratio);

        x += x_;
        y += y_;

        painter.drawPixmap(x, y, pixmapToDraw_);
    }

    void PictureWidget::setImage(const QString& _imageName)
    {
        pixmapToDraw_ = QPixmap(Utils::parse_image_name(_imageName));
        update();
    }

    void PictureWidget::setImage(const QPixmap& _pixmap, int radius)
    {
        pixmapToDraw_ = _pixmap;
        
        if (radius != -1)
        {
            auto borderPixmap = QPixmap(pixmapToDraw_.size());
            borderPixmap.fill(Qt::transparent);
            QPainter pixPainter(&borderPixmap);
            pixPainter.setBrush(pixmapToDraw_);
            pixPainter.setPen(Qt::transparent);
            pixPainter.setRenderHint(QPainter::Antialiasing);
            pixPainter.drawRoundedRect(0, 0, pixmapToDraw_.width(), pixmapToDraw_.height(), radius, radius);
            Utils::check_pixel_ratio(pixmapToDraw_);
            borderPixmap.setDevicePixelRatio(pixmapToDraw_.devicePixelRatio());
            pixmapToDraw_ = borderPixmap;
        }
        
        update();
    }
}
