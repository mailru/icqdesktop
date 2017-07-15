#pragma once

#include "../../types/message.h"

class QPainter;

namespace Logic
{
	class RecentItemDelegate;
}

namespace Ui
{
	class RecentMessagesAlert : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void messageClicked(QString, QString);
        void changed();

	public:
		RecentMessagesAlert(Logic::RecentItemDelegate* delegate, bool isMail);
		~RecentMessagesAlert();

		void addAlert(const Data::DlgState& state);
		void markShowed();
        bool updateMailStatusAlert(const Data::DlgState& state);

	protected:
		virtual void enterEvent(QEvent*);
		virtual void leaveEvent(QEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);
        virtual void showEvent(QShowEvent *);
        virtual void hideEvent(QHideEvent *);

	private:
		void init();

	private Q_SLOTS:
		void closeAlert();
        void statsCloseAlert();
		void viewAll();
		void startAnimation();
        void messageAlertClicked(QString, QString);

	private:
		Logic::RecentItemDelegate* Delegate_;
		QVBoxLayout* Layout_;
		unsigned AlertsCount_;
		QPushButton* CloseButton_;
		QWidget* ViewAllWidget_;
		QTimer* Timer_;
		QPropertyAnimation* Animation_;
		int Height_;
        QLabel* EmailLabel_;
        unsigned MaxAlertCount_;
		bool CursorIn_;
		bool ViewAllWidgetVisible_;
        bool IsMail_;
	};
}