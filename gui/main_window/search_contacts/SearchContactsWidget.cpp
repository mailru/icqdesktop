#include "stdafx.h"
#include "SearchContactsWidget.h"

#include "SearchFilters.h"
#include "results/SearchResults.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    SearchContactsWidget::SearchContactsWidget(QWidget* _parent)
        :	QWidget(_parent)
        , rootLayout_(new QVBoxLayout(this))
        , filtersWidget_(new SearchFilters(this))
        , resultsWidget_(new SearchResults(this))
        , requestInProgress_(false)
        , noMoreItems_(false)
        , ref_(new bool(false))
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/search_contacts/search_contacts.qss"));

        rootLayout_->setContentsMargins(Utils::scale_value(36), 0, 0, 0);
        rootLayout_->setSpacing(0);
        rootLayout_->setAlignment(Qt::AlignTop);

        rootLayout_->addWidget(filtersWidget_);
        filtersWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        rootLayout_->addWidget(resultsWidget_);
        rootLayout_->setStretch(rootLayout_->count() - 1, 1);

        resultsWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        setLayout(rootLayout_);

        connect(filtersWidget_, SIGNAL(onSearch(search_params)), this, SLOT(onSearch(search_params)));
        connect(filtersWidget_, SIGNAL(clicked()), this, SIGNAL(active()), Qt::QueuedConnection);

        connect(resultsWidget_, SIGNAL(needMore(int)), this, SLOT(onNeedMoreResults(int)), Qt::QueuedConnection);
        connect(resultsWidget_, SIGNAL(addContact(QString)), this, SLOT(onAddContact(QString)), Qt::QueuedConnection);
        connect(resultsWidget_, SIGNAL(msgContact(QString)), this, SLOT(onMsgContact(QString)), Qt::QueuedConnection);
        connect(resultsWidget_, SIGNAL(callContact(QString)), this, SLOT(onCallContact(QString)), Qt::QueuedConnection);
        connect(resultsWidget_, SIGNAL(contactInfo(QString)), this, SLOT(onContactInfo(QString)), Qt::QueuedConnection);
    }


    SearchContactsWidget::~SearchContactsWidget(void)
    {
    }

    void SearchContactsWidget::onFocus()
    {
        filtersWidget_->onFocus();
    }

    void SearchContactsWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void SearchContactsWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        emit active();
        return QWidget::mouseReleaseEvent(_e);
    }

    void SearchContactsWidget::onSearchResult(gui_coll_helper _coll)
    {
        filtersWidget_->onSearchResults();

        profiles_list profiles;
        if (_coll.is_value_exist("data"))
        {
            core::ifptr<core::iarray> res_array(_coll.get_value_as_array("data"), false);

            for (int32_t i = 0; i < res_array->size(); ++i)
            {
                gui_coll_helper coll_profile(res_array->get_at(i)->get_as_collection(), false);

                auto profile = std::make_shared<Logic::contact_profile>();

                if (profile->unserialize2(coll_profile))
                {
                    profiles.push_back(profile);
                }
            }
        }
        
        activeFilters_.setNextTag(_coll.get_value_as_string("next_tag"));
        if (activeFilters_.getNextTag().isEmpty() || _coll.get_value_as_bool("finish"))
            noMoreItems_ = true;
        
        resultsWidget_->insertItems(profiles);
    }

    void SearchContactsWidget::onSearch(search_params _filters)
    {
        emit active();

        if (requestInProgress_)
            return;

        activeFilters_ = _filters;

        noMoreItems_ = false;

        search(activeFilters_.getKeyword().toStdString(), "", "");

        resultsWidget_->clear();
    }

    void SearchContactsWidget::search(const std::string& _keyword, const std::string& _phoneNumber, const std::string& _tag)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("keyword", _keyword);
        collection.set_value_as_string("phonenumber", _phoneNumber);
        collection.set_value_as_string("tag", _tag);
        requestInProgress_ = true;
        
        Ui::GetDispatcher()->post_message_to_core("contacts/search", collection.get(), this, [this](core::icollection* _coll)
        {
            requestInProgress_ = false;
            gui_coll_helper coll(_coll, false);
            onSearchResult(coll);
        });
    }

    void SearchContactsWidget::onNeedMoreResults(int)
    {
        emit active();

        if (requestInProgress_ || noMoreItems_)
            return;

        search("", "", activeFilters_.getNextTag().toStdString());
    }

    void SearchContactsWidget::onAddContact(QString _contact)
    {
        emit active();
        std::weak_ptr<bool> wr_ref = ref_;
        Logic::getContactListModel()->addContactToCL(_contact, [this, wr_ref, _contact](bool _res)
        {
            auto ref = wr_ref.lock();
            if (!ref)
                return;

            resultsWidget_->contactAddResult(_contact, _res);

            Logic::getContactListModel()->setCurrent(_contact, -1, true);			
        });
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_search_results);
    }

    void SearchContactsWidget::onMsgContact(QString _contact)
    {
        emit active();
        Logic::getContactListModel()->setCurrent(_contact, -1, true);
    }

    void SearchContactsWidget::onCallContact(QString _contact)
    {
        emit active();
        Logic::getContactListModel()->setCurrent(_contact, -1, true);
        Ui::GetDispatcher()->getVoipController().setStartA(_contact.toUtf8(), false);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_from_search_results);
    }

    void SearchContactsWidget::onContactInfo(QString _contact)
    {
        emit active();
        emit Utils::InterConnector::instance().profileSettingsShow(_contact);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_search_results);
    }
}
