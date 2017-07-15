#include "stdafx.h"
#include "WidgetsNavigator.h"

namespace Ui
{
    WidgetsNavigator::WidgetsNavigator(QWidget* _parent/* = nullptr*/):
        QStackedWidget(_parent)
    {
    }
    WidgetsNavigator::~WidgetsNavigator()
    {
    }
    
    void WidgetsNavigator::insertWidget(int _index, QWidget* _widget)
    {
        widgetToIndex_.insert(std::make_pair(_widget, _index));
        QStackedWidget::insertWidget(_index, _widget);

        if (widgetsAddedOrderByIndexes_.count(_index) == 0)
        {
            widgetsAddedOrderByIndexes_.insert(std::make_pair(_index, std::vector<QWidget*>()));
        }
        widgetsAddedOrderByIndexes_[_index].push_back(_widget);
    }

    int WidgetsNavigator::addWidget(QWidget* _widget)
    {
        static int defaultIndex = 100;
        insertWidget(defaultIndex, _widget);
        return defaultIndex;
    }

    void WidgetsNavigator::push(QWidget* _widget)
    {
        assert(widgetToIndex_.count(_widget) != 0);
        if (currentWidget() != _widget)
        {
            setCurrentWidget(_widget);
            history_.push(_widget);
        }
    }
    
    QWidget* WidgetsNavigator::getFirstWidget() const
    {
        if (widgetsAddedOrderByIndexes_.empty())
        {
            return nullptr;
        }
        else
        {
            return *(widgetsAddedOrderByIndexes_.begin()->second.begin());
        }
    }

    void WidgetsNavigator::pop()
    {
        if (!history_.empty())
            history_.pop();
        if (!history_.empty())
        {
            setCurrentWidget(history_.top());
        }
        else
            setCurrentWidget(getFirstWidget());
    }
    
    void WidgetsNavigator::poproot()
    {
        while (history_.size()) 
            history_.pop(); // idk, maybe some intermediate stuff will be done in each pop.
        setCurrentWidget(getFirstWidget());
    }

    void WidgetsNavigator::removeWidget(QWidget * _widget)
    {
        if (widgetToIndex_.count(_widget) != 0)
        {
            auto indexToDelete = widgetToIndex_[_widget];
            if (widgetsAddedOrderByIndexes_.count(indexToDelete) != 0)
            {
                auto& widgetsByIndex = widgetsAddedOrderByIndexes_[indexToDelete];
                widgetsByIndex.erase(std::remove(widgetsByIndex.begin(), widgetsByIndex.end(), _widget), widgetsByIndex.end());
                if (widgetsByIndex.empty())
                {
                    widgetsAddedOrderByIndexes_.erase(indexToDelete);
                }
            }
            widgetToIndex_.erase(_widget);
        }
        QStackedWidget::removeWidget(_widget);
    }
}
