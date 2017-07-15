#pragma once

#include "../../utils/gui_coll_helper.h"
#include "search_params.h"

namespace Ui
{
    class SearchFilters;
    class SearchResults;

    class SearchContactsWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void active();

    private:

        QVBoxLayout* rootLayout_;
        SearchFilters* filtersWidget_;
        SearchResults* resultsWidget_;

        search_params activeFilters_;

        bool requestInProgress_;
        bool noMoreItems_;

        std::shared_ptr<bool>	ref_;

        void onSearchResult(gui_coll_helper _coll);
        void search(const std::string& _keyword, const std::string& _phoneNumber, const std::string& _tag);

    private Q_SLOTS:

        void onSearch(search_params _filters);
        void onNeedMoreResults(int);
        void onAddContact(QString _contact);
        void onMsgContact(QString _contact);
        void onCallContact(QString _contact);
        void onContactInfo(QString _contact);

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void mouseReleaseEvent(QMouseEvent* _e) override;

    public:

        void onFocus();

        SearchContactsWidget(QWidget* _parent);
        virtual ~SearchContactsWidget(void);
    };

}

