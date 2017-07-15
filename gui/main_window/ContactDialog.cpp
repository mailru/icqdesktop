#include "stdafx.h"
#include "ContactDialog.h"

#include "MainWindow.h"
#include "contact_list/ContactListModel.h"
#include "history_control/HistoryControl.h"
#include "input_widget/InputWidget.h"
#include "sidebar/Sidebar.h"
#include "smiles_menu/SmilesMenu.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../controls/CommonStyle.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/InterConnector.h"

namespace
{
    const int sidebar_default_width = 320;
    const int sidebar_max_width = 428;
    const int sidebar_single_width = 650;
    const int sidebar_show_width = 920;
}

namespace Ui
{
    DragOverlayWindow::DragOverlayWindow(ContactDialog* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAcceptDrops(true);
    }

    void DragOverlayWindow::paintEvent(QPaintEvent *)
    {
        QPainter painter(this);

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(QBrush(Qt::transparent));

        QColor overlayColor("#ffffff");
        overlayColor.setAlphaF(0.9);

        painter.fillRect(
            rect().x(),
            rect().y(),
            rect().width(),
            Ui::CommonStyle::getTopPanelHeight(),
            QBrush(Qt::transparent)
        );
        painter.fillRect(
            rect().x(),
            rect().y() + Ui::CommonStyle::getTopPanelHeight(),
            rect().width(),
            rect().height() - Ui::CommonStyle::getTopPanelHeight(),
            QBrush(overlayColor)
        );

        QPen pen (QColor("#579e1c"), Utils::scale_value(2), Qt::DashLine, Qt::RoundCap);
        painter.setPen(pen);
        painter.drawRoundedRect(
            Utils::scale_value(24),
            Ui::CommonStyle::getTopPanelHeight() + Utils::scale_value(24),
            rect().width() - Utils::scale_value(24) * 2,
            rect().height() - Ui::CommonStyle::getTopPanelHeight() - Utils::scale_value(24) * 2,
            Utils::scale_value(8),
            Utils::scale_value(8)
        );

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_main_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (rect().width() / 2) - (p.width() / 2. / ratio);
        int y = (rect().height() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);
        painter.setFont(Fonts::appFontScaled(15));
        Utils::drawText(
            painter,
            QPointF(rect().width() / 2,
                y + p.height() + Utils::scale_value(24)),
            Qt::AlignHCenter | Qt::AlignVCenter,
            QT_TRANSLATE_NOOP("chat_page", "Drop files to place")
        );
    }

    void DragOverlayWindow::dragEnterEvent(QDragEnterEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void DragOverlayWindow::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        hide();
        _e->accept();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlayWindow::dragMoveEvent(QDragMoveEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void DragOverlayWindow::dropEvent(QDropEvent *_e)
    {
        const QMimeData* mimeData = _e->mimeData();

        if (mimeData->hasUrls())
        {
            QList<QUrl> urlList = mimeData->urls();

            QString contact = Logic::getContactListModel()->selectedContact();
            for (QUrl url : urlList)
            {
                if (url.isLocalFile())
                {
                    QFileInfo info(url.toLocalFile());
                    bool canDrop = !(info.isBundle() || info.isDir());
                    if (info.size() == 0)
                        canDrop = false;

                    if (canDrop)
                    {
                        Ui::GetDispatcher()->uploadSharedFile(contact, url.toLocalFile());
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_dialog);
                        Parent_->onSendMessage(contact);
                    }
                }
                else if (url.isValid())
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", contact);
                    QString text = url.toString();
                    collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                    Parent_->onSendMessage(contact);
                }
            }
        }

        _e->acceptProposedAction();
        hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

	ContactDialog::ContactDialog(QWidget* _parent)
		:	QWidget(_parent)
		, historyControlWidget_(new HistoryControl(this))
		, inputWidget_(new InputWidget(this))
		, smilesMenu_(new Smiles::SmilesMenu(this))
        , dragOverlayWindow_(new DragOverlayWindow(this))
        , overlayUpdateTimer_(new QTimer(this))
        , sidebarUpdateTimer_(new QTimer(this))
        , topWidget_(new QStackedWidget(this))
        , sidebar_(new Sidebar(this))
        , rootLayout_(new QVBoxLayout())
        , sidebarVisible_(false)
	{
        setAcceptDrops(true);
        topWidget_->setFixedHeight(Ui::CommonStyle::getTopPanelHeight());
		rootLayout_->setContentsMargins(0, 0, 0, 0);
		rootLayout_->setSpacing(0);
        rootLayout_->addWidget(topWidget_);
        topWidget_->hide();
        auto verticalLayout = Utils::emptyVLayout();
		verticalLayout->addWidget(historyControlWidget_);
		verticalLayout->addWidget(smilesMenu_);
		verticalLayout->addWidget(inputWidget_);
        layout_ = Utils::emptyHLayout();
        layout_->addLayout(verticalLayout);
        layout_->addWidget(sidebar_);
        layout_->addStretch();

        auto wrapper = new QHBoxLayout();
        wrapper->addLayout(layout_);
        wrapper->addStretch();

        rootLayout_->addLayout(wrapper);

        sidebar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sidebar_->setFixedWidth(0);

		QSpacerItem* layoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
		rootLayout_->addSpacerItem(layoutSpacer);

		setLayout(rootLayout_);

		connect(inputWidget_, SIGNAL(smilesMenuSignal()), this, SLOT(onSmilesMenu()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(editFocusOut()), this, SLOT(onInputEditFocusOut()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString)), Qt::QueuedConnection);

        connect(historyControlWidget_, SIGNAL(quote(QList<Data::Quote>)), inputWidget_, SLOT(quote(QList<Data::Quote>)), Qt::QueuedConnection);

		connect(inputWidget_, &InputWidget::ctrlFPressedInInputWidget, this, &ContactDialog::onCtrlFPressedInInputWidget, Qt::QueuedConnection);
		connect(this, &ContactDialog::contactSelected, inputWidget_, &InputWidget::contactSelected, Qt::QueuedConnection);
		connect(this, &ContactDialog::contactSelected, historyControlWidget_, &HistoryControl::contactSelected, Qt::QueuedConnection);
		connect(this, &ContactDialog::contactSelectedToLastMessage, historyControlWidget_, &HistoryControl::contactSelectedToLastMessage, Qt::QueuedConnection);

        connect(historyControlWidget_, SIGNAL(clicked()), this, SLOT(historyControlClicked()), Qt::QueuedConnection);

		initSmilesMenu();
		initInputWidget();

        overlayUpdateTimer_->setInterval(500);
        overlayUpdateTimer_->setSingleShot(false);
        connect(overlayUpdateTimer_, SIGNAL(timeout()), this, SLOT(updateDragOverlay()), Qt::QueuedConnection);

        sidebarUpdateTimer_->setInterval(500);
        sidebarUpdateTimer_->setSingleShot(true);
        
        dragOverlayWindow_->move(0,0);
        dragOverlayWindow_->hide();
	}


	ContactDialog::~ContactDialog()
	{
        delete topWidget_;
        topWidget_ = 0;
	}

    void ContactDialog::showSidebar(const QString& _aimId, int _page)
    {
        if (!layout_->itemAt(1) || !layout_->itemAt(1)->widget())
        {
            layout_->insertWidget(1, sidebar_);
            sidebar_->setFixedWidth(0);
            sidebar_->show();
        }
        bool showSingle = sideBarShowSingle(width());
        bool force = _aimId != sidebar_->currentAimId();
        sidebar_->setSidebarWidth(showSingle ? Utils::scale_value(sidebar_max_width) : Utils::scale_value(sidebar_default_width));
        sidebar_->preparePage(_aimId, _page == all_members ? menu_page : (SidebarPages)_page);
        setSidebarVisible(true, force);
        sidebarUpdateTimer_->start();
        if (_page == all_members)
            sidebar_->showAllMembers();
    }

    void ContactDialog::takeSidebar()
    {
        layout_->takeAt(1);
    }

    Sidebar* ContactDialog::getSidebar() const
    {
        return sidebar_;
    }

    void ContactDialog::setSidebarVisible(bool _show, bool _force)
    {
        if (sidebarVisible_ == _show && !_force)
            return;

        sidebarVisible_ = _show;

        get_gui_settings()->set_value<bool>(settings_sidebar_hide, !_show);

        const auto showSingle = sideBarShowSingle(width());

        auto sidebarWidth = 0;
        if (_show)
        {
            sidebarWidth = showSingle ? width() : Utils::scale_value(sidebar_default_width);
        }

        dragOverlayWindow_->resize((!showSingle && _show) ? width() - sidebarWidth : width(), height());

        if (showSingle)
        {
            if (_show)
            {
                historyControlWidget_->hide();
                inputWidget_->hideNoClear();
                smilesMenu_->hide();
                sidebar_->setFixedWidth(sidebarWidth);
            }
            else
            {
                sidebar_->setFixedWidth(sidebarWidth);
                historyControlWidget_->setFixedWidth(width());
                inputWidget_->setFixedWidth(width());
                smilesMenu_->setFixedWidth(width());
                historyControlWidget_->show();
                inputWidget_->show();
                smilesMenu_->show();
            }
        }
        else
        {
            sidebar_->setFixedWidth(sidebarWidth);
            historyControlWidget_->setFixedWidth(width() - sidebarWidth);
            inputWidget_->setFixedWidth(width() - sidebarWidth);
            smilesMenu_->setFixedWidth(width() - sidebarWidth);
        }

        historyControlWidget_->updateCurrentPage();
    }

    bool ContactDialog::isSidebarVisible() const
    {
        return sidebarVisible_;
    }

    bool ContactDialog::needShowSidebar() const
    {
        return needShowSidebar(width());
    }

    bool ContactDialog::needShowSidebar(int _contactDialogWidth)
    {
        return !get_gui_settings()->get_value<bool>(settings_sidebar_hide, false) && _contactDialogWidth > Utils::scale_value(sidebar_show_width);
    }

    bool ContactDialog::sideBarShowSingle(int _contactDialogWidth)
    {
        return _contactDialogWidth < Utils::scale_value(sidebar_single_width);
    }

    std::string ContactDialog::getSideBarPolicy(int _contactDialogWidth)
    {
        if (needShowSidebar(_contactDialogWidth))
        {
            return "Sidebar_AlwaysShown";
        }
        else if (sideBarShowSingle(_contactDialogWidth))
        {
            return "Sidebar_Fullsize";
        }
        else
        {
            return "Sidebar_Normal";
        }
    }

	void ContactDialog::onContactSelected(QString _aimId, qint64 _messageId, qint64 _quoteId)
	{
		emit contactSelected(_aimId, _messageId, _quoteId);
        if (needShowSidebar())
        {
            if (!isSidebarVisible() || sidebar_->currentAimId() != _aimId)
                showSidebar(_aimId, menu_page);
        }
        else
        {
            setSidebarVisible(false);
        }
	}

	void ContactDialog::onContactSelectedToLastMessage(QString _aimId, qint64 _messageId)
	{
		emit contactSelectedToLastMessage(_aimId, _messageId);
		if (needShowSidebar())
			showSidebar(_aimId, menu_page);
		else
			setSidebarVisible(false);
	}

	void ContactDialog::initSmilesMenu()
	{
		smilesMenu_->setFixedHeight(0);

		connect(smilesMenu_, SIGNAL(emojiSelected(int32_t, int32_t)), inputWidget_, SLOT(insert_emoji(int32_t, int32_t)));
		connect(smilesMenu_, SIGNAL(stickerSelected(int32_t, int32_t)), inputWidget_, SLOT(send_sticker(int32_t, int32_t)));
	}

	void ContactDialog::initInputWidget()
	{
		connect(inputWidget_, &InputWidget::sendMessage, [this]()
		{
			smilesMenu_->Hide();
		});
	}

	void ContactDialog::onSmilesMenu()
	{
		smilesMenu_->ShowHide();
	}

	void ContactDialog::onInputEditFocusOut()
	{
		smilesMenu_->Hide();
	}

    void ContactDialog::hideSmilesMenu()
    {
        smilesMenu_->Hide();
    }

    void ContactDialog::updateDragOverlay()
    {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
        {
            hideDragOverlay();
        }
    }

    void ContactDialog::historyControlClicked()
    {
        if (!needShowSidebar() && !sidebarUpdateTimer_->isActive())
            setSidebarVisible(false);
        emit clicked();
    }

    void ContactDialog::onSendMessage(QString _contact)
    {
        historyControlWidget_->scrollHistoryToBottom(_contact);

        emit sendMessage(_contact);
    }

    void ContactDialog::cancelSelection()
    {
        assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::hideInput()
    {
        setSidebarVisible(false);
        overlayUpdateTimer_->stop();
        inputWidget_->hide();
        topWidget_->hide();
    }

    void ContactDialog::showDragOverlay()
    {
        dragOverlayWindow_->show();
        overlayUpdateTimer_->start();
        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void ContactDialog::hideDragOverlay()
    {
        dragOverlayWindow_->hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void ContactDialog::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        if (!topWidgetsCache_.contains(_aimId))
        {
            topWidgetsCache_.insert(_aimId, _widget);
            topWidget_->addWidget(_widget);
        }

        topWidget_->show();
        topWidget_->setCurrentWidget(topWidgetsCache_[_aimId]);
    }

    void ContactDialog::removeTopWidget(const QString& _aimId)
    {
        if (!topWidget_)
            return;

        if (topWidgetsCache_.contains(_aimId))
        {
            topWidget_->removeWidget(topWidgetsCache_[_aimId]);
            topWidgetsCache_.remove(_aimId);
        }

        if (!topWidget_->currentWidget())
            topWidget_->hide();
    }

    void ContactDialog::dragEnterEvent(QDragEnterEvent *_e)
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty() || !(_e->mimeData() && _e->mimeData()->hasUrls()) || _e->mimeData()->property("icq").toBool())
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        auto role = Logic::getContactListModel()->getYourRole(Logic::getContactListModel()->selectedContact());
        if (role == "notamember" || role == "readonly")
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        Utils::InterConnector::instance().getMainWindow()->closeGallery();
        Utils::InterConnector::instance().getMainWindow()->activate();
        if (!dragOverlayWindow_->isVisible())
            showDragOverlay();
        _e->acceptProposedAction();
    }

    void ContactDialog::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        _e->accept();
    }

    void ContactDialog::dragMoveEvent(QDragMoveEvent *_e)
    {
        _e->acceptProposedAction();
    }

    void ContactDialog::resizeEvent(QResizeEvent* _e)
    {
        dragOverlayWindow_->resize(width(), height());
        
        if (isSidebarVisible())
        {
            int width = _e->size().width();
            bool oldShowSingle = sideBarShowSingle(_e->oldSize().width());
            bool showSingle = sideBarShowSingle(width);
            sidebar_->setSidebarWidth(showSingle ? Utils::scale_value(sidebar_max_width) : Utils::scale_value(sidebar_default_width));
            sidebar_->setFixedWidth(showSingle ? width : Utils::scale_value(sidebar_default_width));

            if (!showSingle)
            {
                historyControlWidget_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
                inputWidget_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
                smilesMenu_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
            }

            if (showSingle && !oldShowSingle)
            {
                historyControlWidget_->hide();
                inputWidget_->hideNoClear();
                smilesMenu_->hide();
            }
            else if (oldShowSingle && !showSingle)
            {
                historyControlWidget_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
                inputWidget_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
                smilesMenu_->setFixedWidth(width - Utils::scale_value(sidebar_default_width));
                historyControlWidget_->show();
                inputWidget_->show();
                smilesMenu_->show();
            }
            return QWidget::resizeEvent(_e);
        }
        else if (needShowSidebar())
        {
            if (!Logic::getContactListModel()->selectedContact().isEmpty())
                showSidebar(Logic::getContactListModel()->selectedContact(), menu_page);
        }
        historyControlWidget_->setFixedWidth(_e->size().width());
        inputWidget_->setFixedWidth(_e->size().width());
        smilesMenu_->setFixedWidth(_e->size().width());
        QWidget::resizeEvent(_e);
    }

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& _aimId) const
    {
        return historyControlWidget_->getHistoryPage(_aimId);
	}

    const Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        assert(smilesMenu_);
        return smilesMenu_;
    }

    void ContactDialog::setFocusOnInputWidget()
    {
        if (!Logic::getContactListModel()->selectedContact().isEmpty())
            inputWidget_->setFocusOnInput();
    }

    Ui::InputWidget* ContactDialog::getInputWidget() const
    {
        return inputWidget_;
    }

    void ContactDialog::notifyApplicationWindowActive(const bool isActive)
    {
        if (historyControlWidget_)
        {
            historyControlWidget_->notifyApplicationWindowActive(isActive);
        }
    }

    void ContactDialog::onCtrlFPressedInInputWidget()
    {
        emit Utils::InterConnector::instance().setSearchFocus();
    }
}

