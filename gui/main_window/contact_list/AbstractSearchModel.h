#pragma once

#include "CustomAbstractListModel.h"

namespace Logic
{
    class AbstractSearchModel : public CustomAbstractListModel
    {
        Q_OBJECT

    public:
        AbstractSearchModel(QObject* _parent = 0);
        virtual void setFocus() = 0;
        virtual void emitChanged(int _first, int _last) = 0;
        virtual void searchPatternChanged(QString _p) = 0;
        virtual void setSelectEnabled(bool) { };
        virtual bool isServiceItem(int i) const = 0;
    
        void setSort(bool _isClSorting);
        bool isClSorting() const;

        virtual QString getCurrentPattern() const = 0;

    private:
        bool isClSorting_;
    };
    
    AbstractSearchModel* getCurrentSearchModel(int _regim);
}