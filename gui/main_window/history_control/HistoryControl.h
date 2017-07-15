#pragma once

#include "../../types/message.h"

namespace Ui
{
	class history_control;
	class HistoryControlPage;

	class HistoryControl : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
		void quote(QList<Data::Quote>);
        void forward(QList<Data::Quote>);
        void clicked();

	public Q_SLOTS:
		void contactSelected(QString _aimId, qint64 _messageId, qint64 _quoteId);
		void contactSelectedToLastMessage(QString _aimId, qint64 _messageId);

	public:
		HistoryControl(QWidget* parent);
		~HistoryControl();
        void cancelSelection();
        HistoryControlPage* getHistoryPage(const QString& aimId) const;
        void updateCurrentPage();
        void notifyApplicationWindowActive(const bool isActive);
        void scrollHistoryToBottom(QString _contact);

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *);

    private Q_SLOTS:
        void updatePages();
        void close_dialog(const QString& _aimId);
        void leave_dialog(const QString& _aimId);

	private:
        HistoryControlPage* getCurrentPage() const;

		QMap<QString, HistoryControlPage*> pages_;
		QMap<QString, QTime> times_;
		QString current_;
		QTimer* timer_;
        QVBoxLayout *vertical_layout_;
        QVBoxLayout *vertical_layout_2_;
        QStackedWidget *stacked_widget_;
        QWidget* page_;
	};
}