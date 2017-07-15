#pragma once

#include "Common.h"

namespace Logic
{
	class SnapItemDelegate : public QItemDelegate
	{
	public:
		SnapItemDelegate(QObject* parent);

		virtual ~SnapItemDelegate();

		void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

		QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        static QSize getSnapItemSize();

        static QSize getSnapPreviewItemSize();

        static int getFirstAndLastOffset();

        static int getGradientHeight();

    private:
        int fixedWidth_;
    };
}
