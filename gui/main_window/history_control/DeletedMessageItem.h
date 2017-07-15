#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class DeletedMessageItem : public HistoryControlPageItem
    {
        virtual QString formatRecentsText() const { return QString(); }

    public:
        DeletedMessageItem(QWidget* _parent);
        virtual ~DeletedMessageItem();

		virtual void setQuoteSelection() override;
    };

}