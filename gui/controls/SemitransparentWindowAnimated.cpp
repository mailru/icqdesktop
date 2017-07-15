#include "stdafx.h"
#include "SemitransparentWindowAnimated.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

namespace
{
    const int min_step = 0;
    const int max_step = 100;
}

namespace Ui
{
    SemitransparentWindowAnimated::SemitransparentWindowAnimated(QWidget* _parent, int _duration)
        : QWidget(_parent)
        , Step_(min_step)
        , main_(false)
        , isMainWindow_(false)
    {
        Animation_ = new QPropertyAnimation(this, "step");
        Animation_->setDuration(_duration);

        updateSize();

        connect(Animation_, SIGNAL(finished()), this, SLOT(finished()), Qt::QueuedConnection);
    }

    SemitransparentWindowAnimated::~SemitransparentWindowAnimated()
    {
        Utils::InterConnector::instance().decSemiwindowsCount();
    }

    void SemitransparentWindowAnimated::Show()
    {
        auto& interConnector = Utils::InterConnector::instance();
        main_ = (interConnector.getSemiwindowsCount() == 0);
        interConnector.incSemiwindowsCount();

        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(min_step);
        show();
        Animation_->setStartValue(min_step);
        Animation_->setEndValue(max_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::Hide()
    {
        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(max_step);
        Animation_->setStartValue(max_step);
        Animation_->setEndValue(min_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::forceHide()
    {
        hide();
        Utils::InterConnector::instance().decSemiwindowsCount();
    }

    void SemitransparentWindowAnimated::finished()
    {
        if (Animation_->endValue() == min_step)
        {
            setStep(min_step);
            forceHide();
        }
    }

    void SemitransparentWindowAnimated::paintEvent(QPaintEvent* _e)
    {
        if (main_)
        {
            QPainter p(this);
            QColor windowOpacity("#000000");
            windowOpacity.setAlphaF(0.7 * (Step_ / (double)max_step));
            p.fillRect(rect(), windowOpacity);
        }
    }

    void SemitransparentWindowAnimated::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        auto& interConnector = Utils::InterConnector::instance();
        if (!interConnector.isSemiWindowsTouchSwallowed())
        {
            interConnector.setSemiwindowsTouchSwallowed(true);
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow();
        }
    }

    bool SemitransparentWindowAnimated::isMainWindow() const
    {
        return isMainWindow_;
    }

    void SemitransparentWindowAnimated::updateSize()
    {
        if (parentWidget())
        {
            const auto rect = parentWidget()->rect();
            auto width = rect.width();
            auto height = rect.height();

            isMainWindow_ = (qobject_cast<MainWindow*>(parentWidget()) != 0);

            auto titleHeight = isMainWindow_ ? Utils::InterConnector::instance().getMainWindow()->getTitleHeight() : 0;
            setFixedHeight(height - titleHeight);
            setFixedWidth(width);

            move(0, titleHeight);
        }
    }
}
