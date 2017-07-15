#include "stdafx.h"
#include "HorScrollableView.h"

namespace Ui
{
    HorScrollableView::HorScrollableView(QWidget* _parent)
        : QTableView(_parent)
    {
        opacityEffect_ = new QGraphicsOpacityEffect(this);
        opacityEffect_->setOpacity(0.0);
        horizontalScrollBar()->setGraphicsEffect(opacityEffect_);
        fadeAnimation_ = new QPropertyAnimation(opacityEffect_, "opacity");

        timer_ = new QTimer(this);
        timer_->setSingleShot(true);
        connect(timer_, SIGNAL(timeout()), this, SLOT(hideScroll()), Qt::QueuedConnection);
        timer_->start(2000);

        connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(showScroll()), Qt::QueuedConnection);

        setMouseTracking(true);
    }

	void HorScrollableView::wheelEvent(QWheelEvent* e)
    {
        auto scroll = horizontalScrollBar();
        if (scroll && scroll->minimum() != scroll->maximum())
        {
            QApplication::sendEvent(scroll, e);
            return;
        }

        return QTableView::wheelEvent(e);
    }

    void HorScrollableView::hideScroll()
    {
        fadeAnimation_->stop();
        fadeAnimation_->setDuration(250);
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(0.0);
        fadeAnimation_->start();
    }

    void HorScrollableView::showScroll()
    {
        fadeAnimation_->stop();
        fadeAnimation_->setDuration(250);
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(1.0);
        fadeAnimation_->start();

        timer_->start();
    }

    void HorScrollableView::enterEvent(QEvent *e)
    {
        showScroll();

        emit enter();

        return QTableView::enterEvent(e);
    }

    void HorScrollableView::leaveEvent(QEvent *e)
    {
        hideScroll();

        emit leave();

        return QTableView::leaveEvent(e);
    }

    void HorScrollableView::mouseMoveEvent(QMouseEvent *e)
    {
        showScroll();

        return QTableView::mouseMoveEvent(e);
    }
}