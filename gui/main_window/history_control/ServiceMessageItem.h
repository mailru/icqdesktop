#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
	class service_message_item;

	class ServiceMessageItem : public HistoryControlPageItem
	{
		Q_OBJECT

	public:

		ServiceMessageItem(QWidget* parent, bool overlay = false);
		~ServiceMessageItem();

		virtual QString formatRecentsText() const override;

		void setMessage(const QString& message);
		void setWidth(int width);
		void setDate(const QDate& date);
		void setNew();
		bool isNew() const;
        void updateStyle();

		virtual void setQuoteSelection() override;

	private:
		bool new_;
		bool overlay_;
        QHBoxLayout *horizontal_layout_;
        QHBoxLayout *horizontal_layout_2_;
        QHBoxLayout *horizontal_layout_3_;
        QHBoxLayout *horizontal_layout_4_;
        QWidget *left_widget_;
        QWidget *widget_;
        QLabel *message_;
        QWidget *right_widget_;
	};
}