#pragma once

#include "../../types/message.h"

class QPainter;

namespace Logic
{
	class RecentItemDelegate;
}

namespace Ui
{
	class MessageAlertWidget : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
		void clicked(QString, QString);

	public:
		MessageAlertWidget(const Data::DlgState& state, Logic::RecentItemDelegate* delegate, QWidget* parent);
		~MessageAlertWidget();

		QString id() const;
        QString mailId() const;

	protected:
		virtual void paintEvent(QPaintEvent*);
		virtual void resizeEvent(QResizeEvent*);
		virtual void enterEvent(QEvent*);
		virtual void leaveEvent(QEvent*);
        virtual void mousePressEvent(QMouseEvent *);
		virtual void mouseReleaseEvent(QMouseEvent *);

    private Q_SLOTS:
        void avatarChanged(QString);

	private:
		Data::DlgState State_;
		Logic::RecentItemDelegate* Delegate_;
		QPainter* Painter_;
		QStyleOptionViewItem Options_;
	};
}