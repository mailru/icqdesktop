#pragma once

namespace Ui
{
	class HorScrollableView : public QTableView
	{
		Q_OBJECT

    Q_SIGNALS:
        void enter();
        void leave();

    public:
        HorScrollableView(QWidget* _parent);

    public Q_SLOTS: 
        void showScroll();
        void hideScroll();

	protected:
		virtual void wheelEvent(QWheelEvent*);
        virtual void enterEvent(QEvent *);
        virtual void leaveEvent(QEvent *);
        virtual void mouseMoveEvent(QMouseEvent *);

    private:
        QGraphicsOpacityEffect* opacityEffect_;
        QPropertyAnimation* fadeAnimation_;
        QTimer* timer_;
	};
}