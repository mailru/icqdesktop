#pragma once

#include "search_params.h"

namespace Ui
{
    class LineEditEx;

    class SearchFilters : public QWidget
    {
        Q_OBJECT

            LineEditEx*	keyword_;
        QPushButton*	searchButton_;

Q_SIGNALS:

        void onSearch(search_params _filters);
        void clicked();

        private Q_SLOTS:

            void onSearchButtonClicked();

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        void onFocus();
        void onSearchResults();

        SearchFilters(QWidget* _parent);
        virtual ~SearchFilters();
    };


}

