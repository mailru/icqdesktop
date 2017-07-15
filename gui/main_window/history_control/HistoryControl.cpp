#include "stdafx.h"
#include "HistoryControl.h"

#include "HistoryControlPage.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"
#include "../contact_list/RecentsModel.h"
#include "../../gui_settings.h"

namespace Ui
{
	HistoryControl::HistoryControl(QWidget* parent)
		: QWidget(parent)
		, timer_(new QTimer(this))
	{
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        vertical_layout_ = Utils::emptyVLayout(this);
        stacked_widget_ = new QStackedWidget(this);
        stacked_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        page_ = new QWidget();
        page_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        vertical_layout_2_ = Utils::emptyVLayout(page_);
        stacked_widget_->addWidget(page_);
        vertical_layout_->addWidget(stacked_widget_);
        QMetaObject::connectSlotsByName(this);

        if (build::is_debug())
            timer_->setInterval(250);
        else
            timer_->setInterval(60000);

		timer_->setSingleShot(false);
		connect(timer_, SIGNAL(timeout()), this, SLOT(updatePages()), Qt::QueuedConnection);
		timer_->start();

        connect(Logic::getContactListModel(), SIGNAL(leave_dialog(QString)), this, SLOT(leave_dialog(QString)), Qt::QueuedConnection);
		connect(Logic::getContactListModel(), SIGNAL(contact_removed(QString)), this, SLOT(close_dialog(QString)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(activeDialogHide(QString)), this, SLOT(close_dialog(QString)), Qt::QueuedConnection);
	}

	HistoryControl::~HistoryControl()
	{
	}

    void HistoryControl::cancelSelection()
    {
        for (auto &page : pages_)
        {
            assert(page);
            page->cancelSelection();
        }
    }

    HistoryControlPage* HistoryControl::getHistoryPage(const QString& aimId) const
    {
        QMap<QString, HistoryControlPage*>::const_iterator iter = pages_.find(aimId);
        if (iter == pages_.end())
        {
            return 0;
        }

        return *iter;
    }

    void HistoryControl::updateCurrentPage()
    {
        auto page = getCurrentPage();
        if (page)
        {
            page->updateMoreButton();
        }
    }

    void HistoryControl::notifyApplicationWindowActive(const bool isActive)
    {
        for (auto &page : pages_)
        {
            assert(page);

            if (isActive)
            {
                page->resumeVisibleItems();
            }
            else
            {
                page->suspendVisisbleItems();
            }
        }
    }

    void HistoryControl::scrollHistoryToBottom(QString _contact)
    {
        auto page = getHistoryPage(_contact);
        if (page)
            page->scrollToBottom();
    }

    void HistoryControl::mouseReleaseEvent(QMouseEvent *e)
    {
        QWidget::mouseReleaseEvent(e);
        HistoryControlPage* page = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
        if (page)
            emit clicked();
    }

	void HistoryControl::updatePages()
	{
		for (QMap<QString, QTime>::iterator iter = times_.begin(); iter != times_.end(); )
		{
            int time = build::is_debug() ? 1 : 300;
			if (iter.value().secsTo(QTime::currentTime()) >= time && iter.key() != current_)
			{
				close_dialog(iter.key());
 				iter = times_.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

    void HistoryControl::contactSelected(QString _aimId, qint64 _messageId ,qint64 _quoteId)
	{
        assert(!_aimId.isEmpty());
        if (!Logic::getContactListModel()->getContactItem(_aimId))
            return;
        
		Data::DlgState data = Logic::getRecentsModel()->getDlgState(_aimId);
		qint64 last_read_msg = data.UnreadCount_ > 0 ? data.YoursLastRead_ : -1;

        if (_messageId != -1)
        {
            Logic::GetMessagesModel()->messagesDeletedUpTo(_aimId, -1 /* _id */);
            Logic::GetMessagesModel()->setRecvLastMsg(_aimId, false);
        }
        
        HistoryControlPage* oldPage = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
		if (oldPage)
		{
			oldPage->setQuoteId(_quoteId);

			if (_messageId == -1)
			{
				if (oldPage->aimId() == _aimId)
				{
					Utils::InterConnector::instance().setFocusOnInput();
					return;
				}

				oldPage->updateState(true);
			}
		}

        emit Utils::InterConnector::instance().historyControlReady(_aimId, _messageId, -1);//last_read_msg);

		auto iter = pages_.find(_aimId);
        const auto createNewPage = (iter == pages_.end());
		if (createNewPage)
		{
            auto newPage = new HistoryControlPage(this, _aimId);
			newPage->setQuoteId(_quoteId);

			iter = pages_.insert(_aimId, newPage);
			connect((*iter), SIGNAL(quote(QList<Data::Quote>)), this, SIGNAL(quote(QList<Data::Quote>)), Qt::QueuedConnection);
            //connect((*iter), SIGNAL(forward(QList<Data::Quote>)), this, SIGNAL(forward(QList<Data::Quote>)), Qt::QueuedConnection);

			stacked_widget_->addWidget(*iter);

            Logic::getContactListModel()->setCurrentCallbackHappened(newPage);
		}
		//else if (last_read_msg != -1 && _messageId == -1)
		//{
		//	contactSelectedToLastMessage(_aimId, last_read_msg);
		//}

        auto contactPage = *iter;

		stacked_widget_->setCurrentWidget(contactPage);
		contactPage->updateState(false);
		contactPage->open();

        for (auto page : pages_)
        {
            const auto isBackgroundPage = (contactPage != page);
            if (isBackgroundPage)
            {
                page->suspendVisisbleItems();
            }
        }

		if (!current_.isEmpty())
			times_[current_] = QTime::currentTime();
		current_ = _aimId;

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core("dialogs/add", collection.get());

        if (_quoteId > 0)
            Logic::GetMessagesModel()->emitQuote(_quoteId);
	}

	void HistoryControl::contactSelectedToLastMessage(QString _aimId, qint64 _messageId)
	{
		//const auto bMoveHistory = Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false);
		//if (bMoveHistory && _messageId > 0)
		//{
  //          HistoryControlPage* page = qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
  //          if (page && page->aimId() == _aimId)
  //          {
  //              page->showNewMessageForce();
  //              emit contactSelected(_aimId, _messageId, -1);
  //          }
		//}
	}

    void HistoryControl::leave_dialog(const QString& _aimId)
    {
        auto iter_page = pages_.find(_aimId);
        if (iter_page == pages_.end())
        {
            return;
        }
        
        if (current_ == _aimId)
        {
            stacked_widget_->setCurrentIndex(0);
            Ui::MainPage::instance()->hideInput();
        }
    }

	void HistoryControl::close_dialog(const QString& _aimId)
	{
		auto iter_page = pages_.find(_aimId);
		if (iter_page == pages_.end())
		{
			return;
		}

		gui_coll_helper collection(GetDispatcher()->create_collection(), true);
		collection.set_value_as_qstring("contact", _aimId);
		GetDispatcher()->post_message_to_core("dialogs/remove", collection.get());
		Logic::GetMessagesModel()->removeDialog(_aimId);
		stacked_widget_->removeWidget(iter_page.value());

        if (current_ == _aimId)
        {
            stacked_widget_->setCurrentIndex(0);
            Ui::MainPage::instance()->hideInput();
        }

		delete iter_page.value();
		pages_.erase(iter_page);

        emit Utils::InterConnector::instance().dialogClosed(_aimId);
	}

    HistoryControlPage* HistoryControl::getCurrentPage() const
    {
        assert(stacked_widget_);

        return qobject_cast<HistoryControlPage*>(stacked_widget_->currentWidget());
    }
}