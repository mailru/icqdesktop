#include "stdafx.h"
#include "HistoryControlPage.h"

#include "HistoryControlPageThemePanel.h"
#include "MessageItem.h"
#include "MessagesModel.h"
#include "MessageStyle.h"
#include "MessagesScrollArea.h"
#include "ServiceMessageItem.h"
#include "ChatEventItem.h"
#include "VoipEventItem.h"
#include "auth_widget/AuthWidget.h"
#include "complex_message/ComplexMessageItem.h"
#include "selection_panel/SelectionPanel.h"
#include "../ContactDialog.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ContactList.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../sidebar/Sidebar.h"
#include "../../core_dispatcher.h"
#include "../../theme_settings.h"
#include "../../cache/themes/themes.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../../utils/SChar.h"
#include "../../my_info.h"
#include "HistoryButtonDown.h"

#ifdef __APPLE__
#include "../../utils/macos/mac_support.h"
#endif

const QString button_down_imageDefault = ":/themes/standard/100/history_down/history_down.png";
const QString button_down_imageOver = ":/themes/standard/100/history_down/history_down_hover.png";
const auto button_down_size = 60;
const auto button_down_offset_y = 0;
const auto button_down_offset_x = 12;//44;//16*2;
const auto button_shift = 100;

namespace
{
	bool isRemovableWidget(QWidget *w);
    bool isUpdateableWidget(QWidget *w);
    int name_fixed_height = 48;
    int name_padding = 1;
    int scroll_by_key_delta = 20;

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimId)
    {
        QMap<QString, QVariant> result;
        result["command"] = _command;
        result["aimid"] = _aimId;
        return result;
    }
}

namespace Ui
{

    enum class HistoryControlPage::WidgetRemovalResult
    {
        Min,

        Removed,
        NotFound,
        PersistentWidget,

        Max
    };

    ClickWidget::ClickWidget(QWidget* _parent)
        : QWidget(_parent)
    {
    }

    void ClickWidget::mouseReleaseEvent(QMouseEvent *_e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(_e);
    }

    TopWidget::TopWidget(QWidget* parent)
        : QStackedWidget(parent)
        , lastIndex_(0)
    {
    }

    void TopWidget::showThemeWidget(bool _toCurrent, ThemePanelCallback _callback)
    {
        auto theme = qobject_cast<HistoryControlPageThemePanel*>(widget(Theme));
        if (!theme)
            return;

        theme->setCallback(_callback);
        theme->setSelectionToAll(!_toCurrent);
        theme->setShowSetThemeButton(_toCurrent);

        setCurrentIndex(Theme);
    }

    void TopWidget::showSelectionWidget()
    {
        if (currentIndex() == Selection)
            return;

        lastIndex_ = currentIndex();
        setCurrentIndex(Selection);
    }

    void TopWidget::hideSelectionWidget()
    {
        if (currentIndex() == Theme)
            if (auto theme = qobject_cast<HistoryControlPageThemePanel*>(widget(Theme)))
                theme->cancelThemePressed();
        setCurrentIndex(lastIndex_);
    }

	MessagesWidgetEventFilter::MessagesWidgetEventFilter(
        QWidget* _buttonsWidget,
        const QString& _contactName,
        TextEmojiWidget* _contactNameWidget,
		MessagesScrollArea *_scrollArea,
        QWidget* _firstOverlay,
        QWidget* _secondOverlay,
        HistoryControlPage* _dialog
    )
		: QObject(_dialog)
        , ButtonsWidget_(_buttonsWidget)
        , ContactName_(_contactName)
        , ContactNameWidget_(_contactNameWidget)
        , Dialog_(_dialog)
        , FirstOverlay_(_firstOverlay)
		, NewPlateShowed_(false)
        , SecondOverlay_(_secondOverlay)
        , ScrollArea_(_scrollArea)
        , ScrollDirectionDown_(false)
		, Timer_(new QTimer(this))
        , Width_(0)
    {
		assert(ContactNameWidget_);
		assert(!ContactName_.isEmpty());
        assert(ScrollArea_);

		Timer_->setSingleShot(false);
		Timer_->setInterval(100);

        ContactNameWidget_->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

        FirstOverlay_->stackUnder(SecondOverlay_);
        ScrollArea_->stackUnder(FirstOverlay_);
	}

    void MessagesWidgetEventFilter::resetNewPlate()
	{
		NewPlateShowed_ = false;
	}

    QString MessagesWidgetEventFilter::getContactName() const
    {
        return ContactName_;
    }

	bool MessagesWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
	{
		if (_event->type() == QEvent::Resize)
		{
            const auto rect = qobject_cast<QWidget*>(_obj)->contentsRect();

            if (rect.isValid())
            {
                updateSizes();
                ScrollArea_->setGeometry(rect);

				const auto size = Utils::scale_value(button_down_size);
				const auto x = Utils::scale_value(button_down_offset_x);
				const auto y = Utils::scale_value(button_down_offset_y);

				/// 0.35 is experimental offset to fully hided
				Dialog_->setButtonDownPositions(rect.width() - size - x, rect.height() - size - y, rect.height() + 0.35*size - y);
			}

			qobject_cast<Ui::ServiceMessageItem*>(SecondOverlay_)->setNew();
			if (Width_ != rect.width())
			{
				Logic::GetMessagesModel()->setItemWidth(rect.width());
			}

			Width_ = rect.width();

			ResetContactName(ContactName_);
		}
		else if (_event->type() == QEvent::Paint)
		{
			QDate date;
			auto newFound = false;
			auto dateVisible = true;
			qint64 firstVisibleId = -1;

            ScrollArea_->enumerateWidgets(
                [this, &firstVisibleId, &newFound, &date, &dateVisible]
                (QWidget *widget, const bool isVisible)
                {
                    if (widget->visibleRegion().isEmpty() || !isVisible)
                    {
                        return true;
                    }

                    if (auto msgItem = qobject_cast<Ui::MessageItem*>(widget))
                    {
                        if (firstVisibleId == -1)
                        {
                            date = msgItem->date();
                            firstVisibleId = msgItem->getId();
                        }

                        return true;
                    }

                    if (auto complexMsgItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                    {
                        if (firstVisibleId == -1)
                        {
                            date = complexMsgItem->getDate();
                            firstVisibleId = complexMsgItem->getId();
                        }

                        return true;
                    }

                    if (auto serviceItem = qobject_cast<Ui::ServiceMessageItem*>(widget))
                    {
                        if (serviceItem->isNew())
                        {
                            newFound = true;
                            NewPlateShowed_ = true;
                        }
                        else
                        {
                            dateVisible = false;
                        }
                    }

                    return true;
                },
                false
            );

			if (!newFound && NewPlateShowed_)
			{
				Dialog_->newPlateShowed();
				NewPlateShowed_ = false;
			}

			bool visible = date.isValid();

			if (date != Date_)
			{
				Date_ = date;
				qobject_cast<Ui::ServiceMessageItem*>(FirstOverlay_)->setDate(Date_);
				FirstOverlay_->adjustSize();
			}

            const auto isFirstOverlayVisible = (dateVisible && visible);
 			FirstOverlay_->setAttribute(Qt::WA_WState_Hidden, !isFirstOverlayVisible);
			FirstOverlay_->setAttribute(Qt::WA_WState_Visible, isFirstOverlayVisible);

			qint64 newPlateId = Dialog_->getNewPlateId();
			bool newPlateOverlay = newPlateId != -1 && newPlateId < firstVisibleId && !newFound && !NewPlateShowed_;

            if (newPlateOverlay && visible)
            {
                if (!SecondOverlay_->testAttribute(Qt::WA_WState_Visible))
                    SecondOverlay_->show();
            }
            else
            {
                if (!SecondOverlay_->testAttribute(Qt::WA_WState_Hidden))
                    SecondOverlay_->hide();
            }
		}
		else if (_event->type() == QEvent::MouseButtonRelease)
		{
			Timer_->stop();
		}
		else if (_event->type() == QEvent::KeyPress)
		{
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
            bool applePageUp = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Up);
            bool applePageDown = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Down);
            bool applePageEnd = (platform::is_apple() && ((keyEvent->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && keyEvent->key() == Qt::Key_Right) || keyEvent->key() == Qt::Key_End));
			if (keyEvent->matches(QKeySequence::Copy))
			{
				const auto result = ScrollArea_->getSelectedText();

				if (!result.isEmpty())
                {
					QApplication::clipboard()->setText(result);
                }
			}
            else if (keyEvent->matches(QKeySequence::Paste) || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
            {
                auto contactDialog = Utils::InterConnector::instance().getContactDialog();
                if (contactDialog)
                {
                    if (auto inputWidget = contactDialog->getInputWidget())
                    {
                        QApplication::sendEvent((QObject*)inputWidget, _event);
                    }
                }
            }
            if (keyEvent->key() == Qt::Key_Up && !applePageUp)
            {
                ScrollArea_->scroll(UP, Utils::scale_value(scroll_by_key_delta));
            }
            else if (keyEvent->key() == Qt::Key_Down && !applePageDown)
            {
                ScrollArea_->scroll(DOWN, Utils::scale_value(scroll_by_key_delta));
            }
#ifndef __APPLE__
            else if (keyEvent->modifiers() == Qt::CTRL && keyEvent->key() == Qt::Key_End)
            {
                Dialog_->scrollToBottom();
            }
#endif //__APPLE__
            else if (applePageEnd)
            {
                Dialog_->scrollToBottom();
            }
            else if (keyEvent->key() == Qt::Key_PageUp || applePageUp)
            {
                ScrollArea_->scroll(UP, Dialog_->height());
            }
            else if (keyEvent->key() == Qt::Key_PageDown || applePageDown)
            {
                ScrollArea_->scroll(DOWN, Dialog_->height());
            }
		}

		return QObject::eventFilter(_obj, _event);
	}

	void MessagesWidgetEventFilter::ResetContactName(QString _contactName)
	{
		if (!ContactNameWidget_)
			return;

        ContactName_ = _contactName;

		auto contactNameMaxWidth = Utils::InterConnector::instance().getContactDialog()->width();
		contactNameMaxWidth -= ButtonsWidget_->width();

        int diff = ContactNameWidget_->rect().width() - ContactNameWidget_->contentsRect().width();
        contactNameMaxWidth -= diff;
        contactNameMaxWidth -= Utils::scale_value(20);

        QFontMetrics m(ContactNameWidget_->font());
        QString elidedString = m.elidedText(ContactName_, Qt::ElideRight, contactNameMaxWidth).simplified();

        ContactNameWidget_->setText(elidedString);
        ContactNameWidget_->setFixedWidth(ContactNameWidget_->getCompiledWidth());
        ContactNameWidget_->setFixedHeight(Utils::scale_value(name_fixed_height));
    }

    void MessagesWidgetEventFilter::updateSizes()
    {
        FirstOverlay_->setFixedWidth(Dialog_->rect().width());
        FirstOverlay_->move(Dialog_->rect().topLeft());
        SecondOverlay_->setGeometry(Dialog_->rect().x(), Dialog_->rect().y() + FirstOverlay_->height() * 0.7, Dialog_->rect().width(), Dialog_->rect().height());
    }

    enum class HistoryControlPage::State
    {
        Min,

        Idle,
        Fetching,
        Inserting,

        Max
    };

    QTextStream& operator<<(QTextStream& _oss, const HistoryControlPage::State _arg)
    {
        switch (_arg)
        {
            case HistoryControlPage::State::Idle: _oss << "IDLE"; break;
            case HistoryControlPage::State::Fetching: _oss << "FETCHING"; break;
            case HistoryControlPage::State::Inserting: _oss << "INSERTING"; break;

            default:
                assert(!"unexpected state value");
                break;
        }

        return _oss;
    }

	HistoryControlPage::HistoryControlPage(QWidget* _parent, QString _aimId)
		: QWidget(_parent)
		, aimId_(_aimId)
        , authWidget_(nullptr)
        , chatInfoSequence_(-1)
        , chatMembersModel_(NULL)
        , contactStatus_(new LabelEx(this))
        , isContactStatusClickable_(false)
        , isMessagesRequestPostponed_(false)
        , isMessagesRequestPostponedDown_(false)
        , isPublicChat_(false)
        , messagesOverlayFirst_(new ServiceMessageItem(this, true))
		, messagesOverlaySecond_(new ServiceMessageItem(this, true))
        , messagesArea_(new MessagesScrollArea(this, typingWidget_))
		, newPlatePosition_(-1)
        , setThemeId_(-1)
        , state_(State::Idle)
        , topWidget_(new TopWidget(this))
        , typingWidget_(new QWidget(this))
        , seenMsgId_(-1)
		, buttonDown_(nullptr)
		, buttonDir_(0)
		, buttonDownCurve_(QEasingCurve::InSine)
		, bNewMessageForceShow_(false)
        , buttonDownCurrentTime_(0)
		, quoteId_(-1)
	{
		const auto scale = QString::number(Utils::scale_bitmap(Utils::scale_value(100)));

		buttonDown_ = new HistoryButtonDown(this, button_down_imageDefault);
		buttonDown_->setHoverImage(button_down_imageOver);
		buttonDown_->setActiveImage(button_down_imageOver);
		buttonDown_->setDisabledImage(button_down_imageOver);
		buttonDown_->setPressedImage(button_down_imageOver);

        connect(buttonDown_, &HistoryButtonDown::clicked, messagesArea_, &MessagesScrollArea::buttonDownClicked, Qt::DirectConnection);
        connect(buttonDown_, &HistoryButtonDown::clicked, this, &HistoryControlPage::onButtonDownClicked, Qt::DirectConnection);
        connect(buttonDown_, &HistoryButtonDown::sendWheelEvent, messagesArea_, &MessagesScrollArea::onWheelEvent, Qt::DirectConnection);

	    const auto size = Utils::scale_value(button_down_size);
		buttonDown_->setFixedSize(size, size);
		buttonDown_->setCursor(Qt::PointingHandCursor);

		int x=0;
		int y=0;
		buttonDown_->move(x, y);
		QObject::connect(buttonDown_, SIGNAL(clicked()), this, SLOT(scrollToBottomByButton()));
		QObject::connect(messagesArea_, SIGNAL(updateHistoryPosition(int32_t, int32_t)), this, SLOT(onUpdateHistoryPosition(int32_t, int32_t)));
		
		buttonDown_->hide();
		buttonDownTimer_ = new QTimer(this);
		QObject::connect(buttonDownTimer_, SIGNAL(timeout()), this, SLOT(onButtonDownMove()));
		/// 60 fps
		buttonDownTimer_->start(16);

        messagesOverlayFirst_->setContact(_aimId);
        messagesOverlaySecond_->setContact(_aimId);

        const auto style = Utils::LoadStyle(":/main_window/history_control/history_control.qss");
		setStyleSheet(style);
        auto mainTopWidget = new QWidget(this);
        mainTopWidget->setStyleSheet(style);
        mainTopWidget->setFixedHeight(CommonStyle::getTopPanelHeight());
        mainTopWidget->setObjectName("topWidget");

        auto topLayout = Utils::emptyHLayout(mainTopWidget);
        contactWidget_ = new QWidget(mainTopWidget);
        {
            QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
            sizePolicy.setHorizontalStretch(1);
            contactWidget_->setSizePolicy(sizePolicy);
        }
        auto nameStatusVerLayout = Utils::emptyVLayout(contactWidget_);
        nameStatusVerLayout->setSizeConstraint(QLayout::SetDefaultConstraint);

        QHBoxLayout* nameLayout = new QHBoxLayout(contactWidget_);
        nameWidget_ = new ClickWidget(contactWidget_);
        auto v = Utils::emptyVLayout(nameWidget_);

        contactName_ = new TextEmojiWidget(contactWidget_, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(24));
        contactName_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        contactName_->setAttribute(Qt::WA_TransparentForMouseEvents);
        v->addWidget(contactName_);
        nameLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(16), 0, QSizePolicy::Fixed));
        nameLayout->addWidget(nameWidget_);
        nameWidget_->setCursor(Qt::PointingHandCursor);

        connect(nameWidget_, SIGNAL(clicked()), this, SLOT(nameClicked()), Qt::UniqueConnection);

        nameLayout->addSpacerItem(new QSpacerItem(QWIDGETSIZE_MAX, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        nameStatusVerLayout->addLayout(nameLayout);

        auto statusHorLayout = Utils::emptyHLayout();
        auto contactStatusLayout = new QHBoxLayout();
        contactStatusLayout->setContentsMargins(Utils::scale_value(16), 0, 0, Utils::scale_value(4));
        contactStatusWidget_ = new QWidget(this);
        statusHorLayout->addWidget(contactStatusWidget_);

        setContactStatusClickable(false);

        nameStatusVerLayout->addLayout(statusHorLayout);
        verticalSpacer_ = new QSpacerItem(Utils::scale_value(20), Utils::scale_value(0), QSizePolicy::Minimum, QSizePolicy::Expanding);
        nameStatusVerLayout->addItem(verticalSpacer_);
        topLayout->addWidget(contactWidget_);

        auto buttonsWidget = new QWidget(mainTopWidget);
        auto buttonsLayout = Utils::emptyHLayout(buttonsWidget);
        buttonsLayout->setContentsMargins(0, 0, Utils::scale_value(8), 0);
        buttonsLayout->setSpacing(Utils::scale_value(16));
        callButton_ = new QPushButton(buttonsWidget);
        callButton_->setObjectName("callButton");
        buttonsLayout->addWidget(callButton_, 0, Qt::AlignRight);

        videoCallButton_ = new QPushButton(buttonsWidget);
        videoCallButton_->setObjectName("videoCallButton");
        buttonsLayout->addWidget(videoCallButton_, 0, Qt::AlignRight);

        addMemberButton_ = new QPushButton(buttonsWidget);
        addMemberButton_->setObjectName("addMemberButton");
        Testing::setAccessibleName(addMemberButton_, "AddContactToChat");
        buttonsLayout->addWidget(addMemberButton_, 0, Qt::AlignRight);

        moreButton_ = new QPushButton(buttonsWidget);
        moreButton_->setObjectName("optionButton");
        moreButton_->setCheckable(true);
        Testing::setAccessibleName(moreButton_, "ShowChatMenu");
        buttonsLayout->addWidget(moreButton_, 0, Qt::AlignRight);
        buttonsLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
        topLayout->addWidget(buttonsWidget, 0, Qt::AlignRight);

        auto selection = new Selection::SelectionPanel(messagesArea_, this);
        connect(messagesArea_, &MessagesScrollArea::messagesSelected, [this]() { topWidget_->showSelectionWidget(); });
        connect(messagesArea_, &MessagesScrollArea::messagesDeselected, [this]() { topWidget_->hideSelectionWidget(); });

        topWidget_->insertWidget(TopWidget::Main, mainTopWidget);
        topWidget_->insertWidget(TopWidget::Theme, new HistoryControlPageThemePanel(this));
        topWidget_->insertWidget(TopWidget::Selection, selection);
        topWidget_->setCurrentIndex(TopWidget::Main);

        messagesOverlayFirst_->setAttribute(Qt::WA_TransparentForMouseEvents);
		eventFilter_ = new MessagesWidgetEventFilter(
            buttonsWidget,
			Logic::getContactListModel()->selectedContactName(),
            contactName_,
            messagesArea_,
            messagesOverlayFirst_,
            messagesOverlaySecond_,
			this);
		installEventFilter(eventFilter_);

        typingWidget_->setObjectName("typingWidget");
        {
            auto twl = new QHBoxLayout(typingWidget_);
            twl->setContentsMargins(Utils::scale_value(24), 0, 0, 0);
            twl->setSpacing(Utils::scale_value(7));
            twl->setAlignment(Qt::AlignLeft);
            {
                typingWidgets_.twt = new TextEmojiWidget(typingWidget_, Fonts::appFontScaled(12), MessageStyle::getTypingColor(), Utils::scale_value(20));
                typingWidgets_.twt->setSizePolicy(QSizePolicy::Policy::Preferred, typingWidgets_.twt->sizePolicy().verticalPolicy());
                typingWidgets_.twt->setText(" ");
                typingWidgets_.twt->setVisible(false);

                twl->addWidget(typingWidgets_.twt);

                typingWidgets_.twa = new QLabel(typingWidget_);
                typingWidgets_.twa->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
                typingWidgets_.twa->setContentsMargins(0, Utils::scale_value(11), 0, 0);

                typingWidgets_.twm = new QMovie(Utils::parse_image_name(":/resources/gifs/typing_animation100.gif"));
                typingWidgets_.twa->setMovie(typingWidgets_.twm);
                typingWidgets_.twa->setVisible(false);
                twl->addWidget(typingWidgets_.twa);

                connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &HistoryControlPage::typingStatus);
            }
        }
        typingWidget_->show();

		connect(addMemberButton_, SIGNAL(clicked()), this, SLOT(addMember()), Qt::QueuedConnection);
		connect(videoCallButton_, SIGNAL(clicked()), this, SLOT(callVideoButtonClicked()), Qt::QueuedConnection);
        connect(callButton_, SIGNAL(clicked()), this, SLOT(callAudioButtonClicked()), Qt::QueuedConnection);
        connect(moreButton_, SIGNAL(clicked()), this, SLOT(moreButtonClicked()), Qt::QueuedConnection);

        moreButton_->setFocusPolicy(Qt::NoFocus);
        moreButton_->setCursor(Qt::PointingHandCursor);
        videoCallButton_->setFocusPolicy(Qt::NoFocus);
        videoCallButton_->setCursor(Qt::PointingHandCursor);
        callButton_->setFocusPolicy(Qt::NoFocus);
        callButton_->setCursor(Qt::PointingHandCursor);
        addMemberButton_->setFocusPolicy(Qt::NoFocus);
        addMemberButton_->setCursor(Qt::PointingHandCursor);

        messagesArea_->setFocusPolicy(Qt::StrongFocus);

        QString contact_status_style = "font-size: 14dip; color: #767676; background-color: transparent;";
        Utils::ApplyStyle(contactStatus_, contact_status_style);

        officialMark_ = new QPushButton(contactWidget_);
        officialMark_->setObjectName("officialMark");
        officialMark_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        contactStatusLayout->addWidget(officialMark_);
        officialMark_->setVisible(Logic::getContactListModel()->isOfficial(_aimId) && !Logic::getContactListModel()->isChat(_aimId));

        contactStatusLayout->addWidget(contactStatus_);

        contactStatusWidget_->setLayout(contactStatusLayout);

        auto contact = Logic::getContactListModel()->getContactItem(_aimId);
        assert(contact);
#ifndef STRIP_VOIP
        if (contact && contact->is_chat())
        {
#endif //STRIP_VOIP
            callButton_->hide();
            videoCallButton_->hide();
#ifndef STRIP_VOIP
        }
        else
        {
            addMemberButton_->hide();
        }
#endif //STRIP_VOIP

#ifdef STRIP_VOIP
        // TODO: Should remove the lines below when STRIP_VOIP is removed.
        if (contact && !contact->is_chat())
        {
            addMemberButton_->hide();
        }
#endif //STRIP_VOIP

        QObject::connect(
            Logic::getContactListModel(),
            &Logic::ContactListModel::contactChanged,
            this,
            &HistoryControlPage::contactChanged,
            Qt::QueuedConnection);

		QObject::connect(Logic::GetMessagesModel(), &Logic::MessagesModel::ready, this, &HistoryControlPage::sourceReady, Qt::QueuedConnection);
        QObject::connect(Logic::GetMessagesModel(), SIGNAL(canFetchMore(QString)), this, SLOT(fetchMore(QString)), Qt::QueuedConnection);

		QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::updated,
            this,
            &HistoryControlPage::updated,
            (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

		QObject::connect(Logic::GetMessagesModel(), SIGNAL(deleted(QList<Logic::MessageKey>, QString)), this, SLOT(deleted(QList<Logic::MessageKey>, QString)), Qt::QueuedConnection);
        QObject::connect(Logic::GetMessagesModel(), SIGNAL(messageIdFetched(QString, Logic::MessageKey)), this, SLOT(messageKeyUpdated(QString, Logic::MessageKey)), Qt::QueuedConnection);

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::indentChanged,
            this,
            &HistoryControlPage::indentChanged
        );

        QObject::connect(
            Logic::GetMessagesModel(),
            &Logic::MessagesModel::hasAvatarChanged,
            this,
            &HistoryControlPage::hasAvatarChanged
            );

		QObject::connect(Logic::getRecentsModel(), SIGNAL(readStateChanged(QString)), this, SLOT(update(QString)), Qt::QueuedConnection);
        QObject::connect(Logic::getUnknownsModel(), SIGNAL(readStateChanged(QString)), this, SLOT(update(QString)), Qt::QueuedConnection);

		QObject::connect(
            this, &HistoryControlPage::requestMoreMessagesSignal,
            this, &HistoryControlPage::requestMoreMessagesSlot,
            Qt::QueuedConnection
        );

        QObject::connect(
            messagesArea_, &MessagesScrollArea::fetchRequestedEvent,
            this, &HistoryControlPage::onReachedFetchingDistance
        );

        QObject::connect(messagesArea_, &MessagesScrollArea::needCleanup, this, &HistoryControlPage::unloadWidgets);

        QObject::connect(messagesArea_, &MessagesScrollArea::scrollMovedToBottom, this, &HistoryControlPage::scrollMovedToBottom);

		QObject::connect(this, &HistoryControlPage::insertNextMessageSignal, this, &HistoryControlPage::insertNextMessageSlot, Qt::DirectConnection);

		QObject::connect(
            this,
            &HistoryControlPage::needRemove,
            this,
            &HistoryControlPage::removeWidget,
            Qt::QueuedConnection
        );

        if (!contact->is_chat())
        {
            QObject::connect(Logic::GetMessagesModel(), SIGNAL(readByClient(QString, qint64)), this, SLOT(readByClient(QString, qint64)), Qt::QueuedConnection);
        }

        connect(Ui::GetDispatcher(), SIGNAL(setChatRoleResult(int)), SLOT(actionResult(int)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(blockMemberResult(int)), SLOT(actionResult(int)), Qt::QueuedConnection);

        Utils::InterConnector::instance().insertTopWidget(_aimId, topWidget_);
    }

    void HistoryControlPage::updateWidgetsTheme()
    {
        messagesArea_->enumerateWidgets(
            [this](QWidget *widget, const bool)
            {
                if (auto item = qobject_cast<Ui::MessageItem*>(widget))
                {
                    item->updateData();
                    return true;
                }

                if (auto item = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                {
                    item->onStyleChanged();
                    return true;
                }

                if (auto item = qobject_cast<Ui::ServiceMessageItem*>(widget))
                {
                    item->updateStyle();
                    return true;
                }

                if (auto item = qobject_cast<Ui::VoipEventItem*>(widget))
                {
                    item->updateStyle();
                    return true;
                }

                return true;
            }, false);

        messagesOverlayFirst_->updateStyle();
        messagesOverlaySecond_->updateStyle();
        auto theme = get_qt_theme_settings()->themeForContact(aimId_);
        if (theme)
        {
            QColor typingColor = theme->typing_.text_color_;
            typingWidgets_.twt->setColor(typingColor);

            if (theme->typing_.light_gif_ == 0)
            {
                typingWidgets_.twm = new QMovie(Utils::parse_image_name(":/resources/gifs/typing_animation100.gif"));
            }
            else
            {
                typingWidgets_.twm = new QMovie(Utils::parse_image_name(":/resources/gifs/typing_animation100_white.gif"));
            }
            typingWidgets_.twm->setScaledSize(QSize(Utils::scale_value(16), Utils::scale_value(8)));
            typingWidgets_.twa->setMovie(typingWidgets_.twm);
            updateTypingWidgets();
        }
    }

    void HistoryControlPage::typingStatus(Logic::TypingFires _typing, bool _isTyping)
    {
        if (_typing.aimId_ != aimId_)
            return;

        QString name = _typing.getChatterName();

        // if not from multichat

        if (_isTyping)
        {
            typingChattersAimIds_.insert(name);
            updateTypingWidgets();
        }
        else
        {
            typingChattersAimIds_.remove(name);
            if (typingChattersAimIds_.empty())
            {
                hideTypingWidgets();
            }
            else
            {
                updateTypingWidgets();
            }
        }
    }

    void HistoryControlPage::indentChanged(Logic::MessageKey _key, bool _indent)
    {
        assert(!_key.isEmpty());

        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            return;
        }

        auto pageItem = qobject_cast<HistoryControlPageItem*>(widget);
        if (!pageItem)
        {
            return;
        }

        pageItem->setTopMargin(_indent);
    }

    void HistoryControlPage::hasAvatarChanged(Logic::MessageKey _key, bool _hasAvatar)
    {
        assert(!_key.isEmpty());

        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            return;
        }

        auto pageItem = qobject_cast<HistoryControlPageItem*>(widget);
        if (!pageItem)
        {
            return;
        }

        pageItem->setHasAvatar(_hasAvatar);
    }

    void HistoryControlPage::updateTypingWidgets()
    {
        if (!typingChattersAimIds_.empty() && typingWidgets_.twa && typingWidgets_.twm && typingWidgets_.twt)
        {
            QString named;
            if (Logic::getContactListModel()->isChat(aimId_))
            {
                for (auto chatter: typingChattersAimIds_)
                {
                    if (named.length())
                        named += ", ";
                    named += chatter;
                }
            }
            typingWidgets_.twa->setVisible(true);
            typingWidgets_.twm->start();
            typingWidgets_.twt->setVisible(true);
            if (named.length() && typingChattersAimIds_.size() == 1)
                typingWidgets_.twt->setText(named + " " + QT_TRANSLATE_NOOP("chat_page", "typing"));
            else if (named.length() && typingChattersAimIds_.size() > 1)
                typingWidgets_.twt->setText(named + " " + QT_TRANSLATE_NOOP("chat_page", "are typing"));
            else
                typingWidgets_.twt->setText(QT_TRANSLATE_NOOP("chat_page", "typing"));
        }
    }
    void HistoryControlPage::hideTypingWidgets()
    {
        if (typingWidgets_.twa && typingWidgets_.twm && typingWidgets_.twt)
        {
            typingWidgets_.twa->setVisible(false);
            typingWidgets_.twm->stop();
            typingWidgets_.twt->setVisible(false);
            typingWidgets_.twt->setText("");
        }
    }

	HistoryControlPage::~HistoryControlPage()
	{
        Utils::InterConnector::instance().removeTopWidget(aimId_);
	}

    void HistoryControlPage::appendAuthControlIfNeed()
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(aimId_);
        if (contactItem && contactItem->is_chat())
            return;

        if (authWidget_)
            return;

        if (!contactItem || contactItem->is_not_auth())
        {
            authWidget_ = new AuthWidget(messagesArea_, aimId_);
            authWidget_->setProperty("permanent", true);
            messagesArea_->insertWidget(Logic::MessageKey::MIN, authWidget_);

            connect(authWidget_, SIGNAL(addContact(QString)), this, SLOT(authAddContact(QString)));
            connect(authWidget_, SIGNAL(spamContact(QString)), this, SLOT(authBlockContact(QString)));
            connect(authWidget_, SIGNAL(deleteContact(QString)), this, SLOT(authDeleteContact(QString)));

            connect(Logic::getContactListModel(), SIGNAL(contact_added(QString, bool)), this, SLOT(contactAuthorized(QString, bool)));
        }
    }

	bool HistoryControlPage::isScrolling() const
	{
        return !messagesArea_->isScrollAtBottom();
	}

	QWidget* HistoryControlPage::getWidgetByKey(const Logic::MessageKey& _key)
	{
 		return messagesArea_->getItemByKey(_key);
	}

	HistoryControlPage::WidgetRemovalResult HistoryControlPage::removeExistingWidgetByKey(const Logic::MessageKey& _key)
	{
		auto widget = messagesArea_->getItemByKey(_key);
		if (!widget)
		{
			return WidgetRemovalResult::NotFound;
        }

		if (!isRemovableWidget(widget))
		{
			return WidgetRemovalResult::PersistentWidget;
		}

        messagesArea_->removeWidget(widget);

		return WidgetRemovalResult::Removed;
	}

    void HistoryControlPage::replaceExistingWidgetByKey(const Logic::MessageKey& _key, QWidget* _widget)
    {
        assert(_key.hasId());
        assert(_widget);

        messagesArea_->replaceWidget(_key, _widget);
    }

	void HistoryControlPage::contactAuthorized(QString _aimId, bool _res)
	{
		if (_res)
		{
            if (aimId_ == _aimId && authWidget_)
            {
                messagesArea_->removeWidget(authWidget_);

                authWidget_ = nullptr;
            }
		}
	}

	void HistoryControlPage::authAddContact(QString _aimId)
	{
		Logic::getContactListModel()->addContactToCL(_aimId);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::add_user_auth_widget);
	}

    void HistoryControlPage::authBlockContact(QString _aimId)
    {
        Logic::getContactListModel()->blockAndSpamContact(_aimId, false);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_auth_widget);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_auth_widget);

        emit Utils::InterConnector::instance().profileSettingsBack();
    }

	void HistoryControlPage::authDeleteContact(QString _aimId)
	{
		Logic::getContactListModel()->removeContactFromCL(_aimId);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_auth_widget);
	}

	void HistoryControlPage::newPlateShowed()
	{
		qint64 oldNew = newPlatePosition_;
        newPlatePosition_ = -1;
		Logic::GetMessagesModel()->updateNew(aimId_, oldNew, true);
	}

	void HistoryControlPage::update(QString _aimId)
	{
        if (_aimId != aimId_)
            return;

		if (!isVisible())
		{
			Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_);

			Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId_);
            if (state.AimId_ != aimId_)
                state = Logic::getUnknownsModel()->getDlgState(aimId_);
            newPlatePosition_ = state.LastMsgId_ == state.YoursLastRead_ ? -1 : state.YoursLastRead_;
		}
	}

    void HistoryControlPage::updateMoreButton()
    {
        moreButton_->setChecked(Utils::InterConnector::instance().isSidebarVisible());
    }

    void HistoryControlPage::updateItems()
    {
        messagesArea_->updateItems();
    }

    void HistoryControlPage::copy(QString _text)
    {
        const auto selectionText = messagesArea_->getSelectedText();

#ifdef __APPLE__

        if (!selectionText.isEmpty())
            MacSupport::replacePasteboard(selectionText);
        else
            MacSupport::replacePasteboard(_text);

#else

        if (!selectionText.isEmpty())
            QApplication::clipboard()->setText(selectionText);
        else
            QApplication::clipboard()->setText(_text);

#endif
    }

    void HistoryControlPage::quoteText(QList<Data::Quote> q)
	{
        QList<Data::Quote> quotes = messagesArea_->getQuotes();
        if (quotes.isEmpty())
            quotes.append(q);

        messagesArea_->clearSelection();
		emit quote(quotes);
	}

    void HistoryControlPage::forwardText(QList<Data::Quote> q)
    {
        QList<Data::Quote> quotes = messagesArea_->getQuotes();
        if (quotes.isEmpty())
            quotes.append(q);
        
        messagesArea_->clearSelection();
        forwardMessage(quotes, true);
    }

	void HistoryControlPage::updateState(bool _close)
	{
        Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_, _close);

        Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId_, !_close);
        if (state.AimId_ != aimId_)
            state = Logic::getUnknownsModel()->getDlgState(aimId_, !_close);

        if (_close)
        {
            newPlatePosition_ = state.LastMsgId_;
            eventFilter_->resetNewPlate();
        }
        else
        {
            if (state.UnreadCount_ == 0)
            {
                newPlatePosition_ = -1;
            }
            else
            {
                qint64 normalized = Logic::GetMessagesModel()->normalizeNewMessagesId(aimId_, state.YoursLastRead_);
                newPlatePosition_ = normalized;
                if (normalized != state.YoursLastRead_)
                    Logic::GetMessagesModel()->updateNew(aimId_, normalized, _close);
            }
        }

        updateItems();
	}

	qint64 HistoryControlPage::getNewPlateId() const
	{
		return newPlatePosition_;
	}

    void HistoryControlPage::sourceReady(QString _aimId, bool _is_search, int64_t _mess_id)
	{
        if (_aimId != aimId_)
		{
			return;
		}

        //assert(isStateIdle());
        assert(itemsData_.empty());

        messagesArea_->setIsSearch(_is_search);

        switchToFetchingState(__FUNCLINEA__);

        auto widgets = Logic::GetMessagesModel()->tail(aimId_, messagesArea_, _is_search, _mess_id);

        //qDebug("sourceReady %i", widgets.size());

        QMapIterator<Logic::MessageKey, Ui::HistoryControlPageItem*> iter(widgets);
		iter.toBack();
		while (iter.hasPrevious())
		{
			iter.previous();
            if (iter.value())
                itemsData_.emplace_back(iter.key(), iter.value(), Logic::MessagesModel::REQUESTED, iter.value()->isDeleted());
		}

        if (!itemsData_.empty())
        {
            switchToInsertingState(__FUNCLINEA__);

            postInsertNextMessageSignal(__FUNCLINEA__, !_is_search, _mess_id);
        }

		if (widgets.count() < (signed)Logic::GetMessagesModel()->preloadCount())
        {
			requestMoreMessagesAsync(__FUNCLINEA__);
        }

		if (bNewMessageForceShow_)
		{
			newPlatePosition_ = _mess_id;
			if (_mess_id != -1)
				bNewMessageForceShow_ = false;
		}

		Logic::GetMessagesModel()->updateNew(aimId_, newPlatePosition_);
	}

    void HistoryControlPage::insertNextMessageSlot(bool _isMoveToBottomIfNeed, int64_t _mess_id)
	{
        __INFO("smooth_scroll", "entering signal handler\n""    type=<insertNextMessageSlot>\n""    state=<" << state_ << ">\n""    items size=<" << itemsData_.size() << ">");

        if (itemsData_.empty())
        {
            switchToIdleState(__FUNCLINEA__);

            if (isMessagesRequestPostponed_)
            {
                __INFO("smooth_scroll", "resuming postponed messages request\n""    requested at=<" << dbgWherePostponed_ << ">");

                isMessagesRequestPostponed_ = false;
                dbgWherePostponed_ = nullptr;

                requestMoreMessagesAsync(__FUNCLINEA__, isMessagesRequestPostponedDown_);
            }

            return;
        }

        bool isScrollAtBottom = messagesArea_->isScrollAtBottom();
        int unreadCount = 0;

        auto update_unreads = [this, isScrollAtBottom, &unreadCount](const ItemData& _data)
        {
            if (!isScrollAtBottom && _data.Mode_ == Logic::MessagesModel::BASE && !_data.Key_.isOutgoing())
                unreadCount++;
        };

        MessagesScrollArea::WidgetsList insertWidgets;
        while (!itemsData_.empty())
        {
            auto data = itemsData_.front();
            itemsData_.pop_front();

            if (data.IsDeleted_)
            {
                continue;
            }

            __INFO(
                "history_control",
                "inserting widget\n"
                "    key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");

            auto existing = getWidgetByKey(data.Key_);
            if (existing)
            {
                auto isNewItemChatEvent = qobject_cast<Ui::ChatEventItem*>(data.Widget_);

                if (!isUpdateableWidget(existing) && !isNewItemChatEvent)
                {
                    {
                        auto messageItem = qobject_cast<Ui::MessageItem*>(existing);
                        auto newMessageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);

                        if (messageItem)
                            messageItem->setDeliveredToServer(!data.Key_.isPending());

                        if (messageItem && newMessageItem)
                        {
                            messageItem->updateWith(*newMessageItem);
                        }
                    }

                    {
                        auto messageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                        auto newMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);

                        if (messageItem && newMessageItem)
                        {
                            messageItem->setDeliveredToServer(!data.Key_.isPending(), false);
                            messageItem->updateWith(*newMessageItem);
                        }
                    }

                    __INFO("history_control","widget insertion discarded (persistent widget)\n""	key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");
                    data.Widget_->deleteLater();

                    continue;
                }

                const auto isNewTabletReplacement = (
                    existing->property("New").toBool() ||
                    data.Widget_->property("New").toBool());

                if (isNewTabletReplacement)
                {
                    removeExistingWidgetByKey(data.Key_);
                }
                else
                {
                    auto messageItem = qobject_cast<Ui::MessageItem*>(existing);
                    auto newMessageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);

                    if (messageItem && newMessageItem)
                    {
                        messageItem->setDeliveredToServer(!data.Key_.isPending());
                        messageItem->updateWith(*newMessageItem);

                        data.Widget_->deleteLater();

                        if (!data.Key_.isOutgoing() && (data.Mode_ == Logic::MessagesModel::BASE))
                        {
                            auto name = newMessageItem->getMchatSenderAimId();
                            const auto contact = Logic::getContactListModel()->getContactItem(name);
                            if (contact)
                                name = contact->Get()->GetDisplayName();
                            typingChattersAimIds_.remove(name);
                            if (typingChattersAimIds_.empty())
                                hideTypingWidgets();
                            else
                                updateTypingWidgets();
                        }

                        update_unreads(data);

                        continue;
                    }

                    auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                    auto newComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);

                    if (complexMessageItem && newComplexMessageItem)
                    {
                        complexMessageItem->setDeliveredToServer(!data.Key_.isPending(), false);
                        complexMessageItem->updateWith(*newComplexMessageItem);

                        data.Widget_->deleteLater();

                        continue;
                    }

                    if (data.Key_.isDate())
                    {
                        data.Widget_->deleteLater();

                        continue;
                    }

                    removeExistingWidgetByKey(data.Key_);
                }
            }
            else
            {
                update_unreads(data);
            }

            __TRACE("history_control", "inserting widget position info\n""	key=<" << data.Key_.getId() << ";" << data.Key_.getInternalId() << ">");

            // prepare widget for insertion

            auto messageItem = qobject_cast<Ui::MessageItem*>(data.Widget_);
            if (messageItem)
            {
                connect(messageItem, SIGNAL(copy(QString)), this, SLOT(copy(QString)), Qt::QueuedConnection);
                connect(messageItem, SIGNAL(quote(QList<Data::Quote>)), this, SLOT(quoteText(QList<Data::Quote>)), Qt::QueuedConnection);
                connect(messageItem, SIGNAL(forward(QList<Data::Quote>)), this, SLOT(forwardText(QList<Data::Quote>)), Qt::QueuedConnection);
                connect(messageItem, SIGNAL(adminMenuRequest(QString)), this, SLOT(adminMenuRequest(QString)), Qt::QueuedConnection);

                if (!data.Key_.isOutgoing() && data.Mode_ == Logic::MessagesModel::BASE)
                {
                    auto name = messageItem->getMchatSenderAimId();
                    auto contact = Logic::getContactListModel()->getContactItem(name);
                    if (contact)
                        name = contact->Get()->GetDisplayName();
                    typingChattersAimIds_.remove(name);
                    if (typingChattersAimIds_.empty())
                        hideTypingWidgets();
                    else
                        updateTypingWidgets();
                }
            }
            else
            {
                auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(data.Widget_);
                if (complexMessageItem)
                {
                    connect(complexMessageItem, SIGNAL(copy(QString)), this, SLOT(copy(QString)), Qt::QueuedConnection);
                    connect(complexMessageItem, SIGNAL(quote(QList<Data::Quote>)), this, SLOT(quoteText(QList<Data::Quote>)), Qt::QueuedConnection);
                    connect(complexMessageItem, SIGNAL(forward(QList<Data::Quote>)), this, SLOT(forwardText(QList<Data::Quote>)), Qt::QueuedConnection);
                    connect(complexMessageItem, SIGNAL(adminMenuRequest(QString)), this, SLOT(adminMenuRequest(QString)), Qt::QueuedConnection);
                }
                else if (data.Widget_->layout())
                {
                    auto layout = data.Widget_->layout();

                    auto index = 0;
                    while (auto child = layout->itemAt(index++))
                    {
                        if (auto messageItem = qobject_cast<Ui::MessageItem*>(child->widget()))
                        {
                            connect(messageItem, SIGNAL(copy(QString)), this, SLOT(copy(QString)), Qt::QueuedConnection);
                            connect(messageItem, SIGNAL(quote(QList<Data::Quote>)), this, SLOT(quoteText(QList<Data::Quote>)), Qt::QueuedConnection);
                            connect(messageItem, SIGNAL(forward(QList<Data::Quote>)), this, SLOT(forwardText(QList<Data::Quote>)), Qt::QueuedConnection);
                            connect(messageItem, SIGNAL(adminMenuRequest(QString)), this, SLOT(adminMenuRequest(QString)), Qt::QueuedConnection);
                        }
                        else if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(child->widget()))
                        {
                            connect(complexMessageItem, SIGNAL(copy(QString)), this, SLOT(copy(QString)), Qt::QueuedConnection);
                            connect(complexMessageItem, SIGNAL(quote(QList<Data::Quote>)), this, SLOT(quoteText(QList<Data::Quote>)), Qt::QueuedConnection);
                            connect(complexMessageItem, SIGNAL(forward(QList<Data::Quote>)), this, SLOT(forwardText(QList<Data::Quote>)), Qt::QueuedConnection);
                            connect(complexMessageItem, SIGNAL(adminMenuRequest(QString)), this, SLOT(adminMenuRequest(QString)), Qt::QueuedConnection);
                        }
                    }
                }
            }

            // insert and display the widget

            insertWidgets.emplace_back(
                std::make_pair(data.Key_, data.Widget_)
            );
        }

        //qDebug("insertWidgets:%i", insertWidgets.size());

        if (insertWidgets.size())
            messagesArea_->insertWidgets(insertWidgets, _isMoveToBottomIfNeed, _mess_id);

        if (unreadCount)
        {
			buttonDown_->addUnreadMessages(unreadCount);
        }
        else
        {
			buttonDown_->setUnreadMessages(0);
        }

        postInsertNextMessageSignal(__FUNCLINEA__, _isMoveToBottomIfNeed);

        if (!_isMoveToBottomIfNeed)
        {
            readByClient(aimId_, seenMsgId_);
        }

	}

	void HistoryControlPage::removeWidget(Logic::MessageKey _key)
	{
		if (isScrolling())
		{
			emit needRemove(_key);
			return;
		}

		__TRACE(
			"history_control",
			"requested to remove the widget\n"
			"	key=<" << _key.getId() << ";" << _key.getInternalId() << ">");


        removeRequests_.erase(_key);

		const auto result = removeExistingWidgetByKey(_key);
        assert(result > WidgetRemovalResult::Min);
        assert(result < WidgetRemovalResult::Max);
	}

    bool HistoryControlPage::touchScrollInProgress() const
    {
        return messagesArea_->touchScrollInProgress();
    }

    void HistoryControlPage::unloadWidgets(QList<Logic::MessageKey> _keysToUnload)
    {
        assert(!_keysToUnload.empty());

        if (_keysToUnload.empty())
            return;

        std::sort(_keysToUnload.begin(), _keysToUnload.end(), qLess<Logic::MessageKey>());

        const auto &lastKey = _keysToUnload.last();
        assert(!lastKey.isEmpty());

        const auto keyAfterLast = Logic::GetMessagesModel()->findFirstKeyAfter(aimId_, lastKey);

        Logic::MessageKey last_key = _keysToUnload.back();
        _keysToUnload.pop_back();

        for (const auto &key : _keysToUnload)
        {
            assert(!key.isEmpty());

            emit needRemove(key);
        }

        if (!keyAfterLast.isEmpty())
        {
            Logic::GetMessagesModel()->setLastKey(keyAfterLast, aimId_);
        }
    }

    void HistoryControlPage::loadChatInfo(bool _isFullListLoaded)
    {
        _isFullListLoaded |= Utils::InterConnector::instance().isSidebarVisible();
        if (chatMembersModel_ && (chatMembersModel_->isAdmin() || chatMembersModel_->isModer()))
        {
            chatInfoSequence_ = Logic::ChatMembersModel::loadAllMembers(aimId_, chatMembersModel_->getMembersCount(), this);
        }
        else
        {
            chatInfoSequence_ = Logic::ChatMembersModel::loadAllMembers(aimId_, _isFullListLoaded ? Logic::MaxMembersLimit : Logic::InitMembersLimit, this);
        }
    }

	void HistoryControlPage::initStatus()
	{
		Logic::ContactItem* contact = Logic::getContactListModel()->getContactItem(aimId_);
		if (!contact)
			return;

		if (contact->is_chat())
		{
            loadChatInfo(false);
            officialMark_->setVisible(false);
        }
		else
		{
            contactStatus_->setText(Logic::getContactListModel()->getStatusString(aimId_));

            officialMark_->setVisible(Logic::getContactListModel()->isOfficial(aimId_));
		}
	}

    void HistoryControlPage::updateName()
    {
        Logic::ContactItem* contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (contact)
            eventFilter_->ResetContactName(contact->Get()->GetDisplayName());
    }

    void HistoryControlPage::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode)
    {
        if (Logic::ChatMembersModel::receiveMembers(chatInfoSequence_, _seq, this))
        {
            if (_errorCode == core::group_chat_info_errors::not_in_chat || _errorCode == core::group_chat_info_errors::blocked)
            {
                contactStatus_->setText(QT_TRANSLATE_NOOP("groupchats","You are not a member of this chat"));
                Logic::getContactListModel()->setYourRole(aimId_, "notamember");
                addMemberButton_->hide();
                nameWidget_->setCursor(Qt::ArrowCursor);
                disconnect(nameWidget_, SIGNAL(clicked()), this, SLOT(nameClicked()));
            }
        }
    }

	void HistoryControlPage::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
	{
        if (!Logic::ChatMembersModel::receiveMembers(chatInfoSequence_, _seq, this))
        {
            return;
        }

        __INFO(
            "chat_info",
            "incoming chat info event\n"
            "    contact=<" << aimId_ << ">\n"
            "    your_role=<" << _info->YourRole_ << ">"
        );

        eventFilter_->ResetContactName(_info->Name_);
        setContactStatusClickable(true);

        QString state = QString("%1").arg(_info->MembersCount_) + QString(" ")
            + Utils::GetTranslator()->getNumberString(
                _info->MembersCount_,
                QT_TRANSLATE_NOOP3("chat_page", "member", "1"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "2"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "5"),
                QT_TRANSLATE_NOOP3("chat_page", "members", "21")
                );
        contactStatus_->setText(state);
        isPublicChat_ = _info->Public_;
        if (chatMembersModel_ == NULL)
        {
            chatMembersModel_ = new Logic::ChatMembersModel(_info, this);
            if (chatMembersModel_->isAdmin() || chatMembersModel_->isModer())
                loadChatInfo(false);
        }
        else
        {
            chatMembersModel_->updateInfo(_info, false);
            emit updateMembers();
        }

        nameWidget_->setCursor(Qt::PointingHandCursor);
        connect(nameWidget_, SIGNAL(clicked()), this, SLOT(nameClicked()), Qt::UniqueConnection);
	}

	void HistoryControlPage::contactChanged(QString _aimId)
	{
        assert(!aimId_.isEmpty());

        const auto isMyAimid = (_aimId == aimId_);
		if (!isMyAimid)
        {
            return;
        }

		initStatus();
        updateName();
	}

    void HistoryControlPage::editMembers()
    {
        Utils::InterConnector::instance().showSidebar(aimId_, SidebarPages::all_members);
    }

	void HistoryControlPage::open()
	{
        Utils::InterConnector::instance().insertTopWidget(aimId_, topWidget_);
		initStatus();
        moreButton_->setChecked(Utils::InterConnector::instance().isSidebarVisible());
        int new_theme_id = get_qt_theme_settings()->contactOpenned(aimId());
        if (setThemeId_ != new_theme_id)
        {
            updateWidgetsTheme();
            setThemeId_ = new_theme_id;
        }
        else
        {
            messagesOverlayFirst_->updateStyle();
            messagesOverlaySecond_->updateStyle();
        }

        eventFilter_->updateSizes();

        resumeVisibleItems();
	}

    void HistoryControlPage::showMainTopPanel()
    {
        topWidget_->setCurrentIndex(TopWidget::Main);
    }

    void HistoryControlPage::showThemesTopPanel(bool _showSetToCurrent, ThemePanelCallback _callback)
    {
        Logic::getRecentsModel()->sendLastRead(aimId_);
        topWidget_->showThemeWidget(_showSetToCurrent, _callback);
    }

    bool HistoryControlPage::requestMoreMessagesAsync(const char* _dbgWhere, bool _isMoveToBottomIfNeed)
    {
        if (isStateFetching())
        {
            __INFO(
                "smooth_scroll",
                "requesting more messages\n"
                "    status=<cancelled>\n"
                "    reason=<already fetching>\n"
                "    from=<" << _dbgWhere << ">"
            );

            return false;
        }

        if (isStateInserting())
        {
            __INFO(
                "smooth_scroll",
                "requesting more messages\n"
                "    status=<postponed>\n"
                "    reason=<inserting>\n"
                "    from=<" << _dbgWhere << ">"
            );

            postponeMessagesRequest(_dbgWhere, _isMoveToBottomIfNeed);

            return true;
        }

        __INFO(
            "smooth_scroll",
            "requesting more messages\n"
            "    status=<requested>\n"
            "    from=<" << _dbgWhere << ">"
        );

        switchToFetchingState(__FUNCLINEA__);

        emit requestMoreMessagesSignal(_isMoveToBottomIfNeed);

        return true;
    }

    QString HistoryControlPage::aimId() const
    {
        return aimId_;
    }

    void HistoryControlPage::cancelSelection()
    {
        assert(messagesArea_);
        messagesArea_->cancelSelection();
    }

	void HistoryControlPage::messageKeyUpdated(QString _aimId, Logic::MessageKey _key)
	{
		assert(_key.hasId());

		if (_aimId != aimId_)
		{
			return;
		}

        messagesArea_->updateItemKey(_key);
        Ui::HistoryControlPageItem* msg = Logic::GetMessagesModel()->getById(aimId_, _key, messagesArea_);
        if (msg)
        {
            itemsData_.emplace_back(_key, msg, Logic::MessagesModel::PENDING, msg->isDeleted());
            if (!isStateInserting())
            {
                if (isStateIdle())
                {
                    switchToFetchingState(__FUNCLINEA__);
                }
                switchToInsertingState(__FUNCLINEA__);
                postInsertNextMessageSignal(__FUNCLINEA__);
            }
        }
        else
        {
            removeExistingWidgetByKey(_key);
        }
	}

	void HistoryControlPage::updated(QList<Logic::MessageKey> _list, QString _aimId, unsigned _mode)
	{
		if (_aimId != aimId_)
        {
			return;
        }

        const auto isHole = (_mode == Logic::MessagesModel::HOLE);
        if (isHole)
        {
            eventFilter_->resetNewPlate();
        }

		for (auto key : _list)
		{
            auto msg = Logic::GetMessagesModel()->getById(aimId_, key, messagesArea_);
			if (!msg)
            {
                continue;
            }

            itemsData_.emplace_back(key, msg, _mode, msg->isDeleted());

            if (key.getType() == core::message_type::chat_event)
            {
                updateChatInfo();
            }
		}

        if (!itemsData_.empty() && !isStateInserting())
        {
            if (isStateIdle())
            {
                switchToFetchingState(__FUNCLINEA__);
            }

            switchToInsertingState(__FUNCLINEA__);

            postInsertNextMessageSignal(__FUNCLINEA__);
        }

        if (!messagesArea_->isViewportFull())
        {
            requestMoreMessagesAsync(__FUNCLINEA__);
        }
	}

	void HistoryControlPage::deleted(QList<Logic::MessageKey> _list, QString _aimId)
	{
		if (_aimId != aimId_)
        {
			return;
        }

		for (auto keyToRemove : _list)
		{
			removeExistingWidgetByKey(keyToRemove);

            for (auto itemIter = itemsData_.cbegin(); itemIter != itemsData_.cend();)
            {
                if (itemIter->Key_ != keyToRemove)
                {
                    ++itemIter;
                    continue;
                }

                itemIter = itemsData_.erase(itemIter);
            }
		}
	}

	void HistoryControlPage::requestMoreMessagesSlot(bool _isMoveToBottomIfNeed)
	{
		auto widgets = Logic::GetMessagesModel()->more(aimId_, messagesArea_, _isMoveToBottomIfNeed);

        QMapIterator<Logic::MessageKey, Ui::HistoryControlPageItem*> iter(widgets);
		iter.toBack();
		while (iter.hasPrevious())
		{
			iter.previous();
            if (iter.value())
                itemsData_.emplace_back(iter.key(), iter.value(), Logic::MessagesModel::REQUESTED, iter.value()->isDeleted());
		}

        switchToInsertingState(__FUNCLINEA__);

        postInsertNextMessageSignal(__FUNCLINEA__, _isMoveToBottomIfNeed);

        if (!messagesArea_->isViewportFull() && !widgets.isEmpty())
        {
            requestMoreMessagesAsync(__FUNCLINEA__, _isMoveToBottomIfNeed);
        }

        //if (!messagesArea_->isViewportFull() && widgets.isEmpty() && messagesArea_->getIsSearch())
        //{
        //    requestMoreMessagesAsync(__FUNCLINEA__, !_isMoveToBottomIfNeed);
        //}
	}

	void HistoryControlPage::downPressed()
	{
		buttonDown_->setUnreadMessages(0);
        scrollToBottom();
	}

    void HistoryControlPage::scrollMovedToBottom()
    {
		buttonDown_->setUnreadMessages(0);
    }

	void HistoryControlPage::autoScroll(bool _enabled)
	{
		if (!_enabled)
		{
			return;
		}
	//	unloadWidgets();
		buttonDown_->setUnreadMessages(0);
	}

	void HistoryControlPage::callAudioButtonClicked()
    {
		Ui::GetDispatcher()->getVoipController().setStartA(aimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance())
        {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_from_chat);
	}
	void HistoryControlPage::callVideoButtonClicked()
    {
		Ui::GetDispatcher()->getVoipController().setStartV(aimId_.toUtf8(), false);
        if (MainPage* mainPage = MainPage::instance())
        {
            mainPage->raiseVideoWindow();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::videocall_from_chat);
	}

	void HistoryControlPage::moreButtonClicked()
	{
        if (Utils::InterConnector::instance().isSidebarVisible())
        {
            moreButton_->setChecked(false);
            Utils::InterConnector::instance().setSidebarVisible(false);
        }
        else
        {
            moreButton_->setChecked(true);
            Utils::InterConnector::instance().showSidebar(aimId_, SidebarPages::menu_page);
        }
	}

    void HistoryControlPage::focusOutEvent(QFocusEvent* _event)
	{
		QWidget::focusOutEvent(_event);
	}

    void HistoryControlPage::wheelEvent(QWheelEvent* _event)
    {
        if (!hasFocus())
        {
            return;
        }

        return QWidget::wheelEvent(_event);
    }

    void HistoryControlPage::addMember()
    {
        auto contact = Logic::getContactListModel()->getContactItem(aimId_);
        if (!contact)
            return;

        if (!contact->is_chat() || !chatMembersModel_)
            return;

        assert(!!chatMembersModel_);

        Logic::setChatMembersModel(chatMembersModel_);

        if (!chatMembersModel_->isFullListLoaded_)
        {
            chatMembersModel_->loadAllMembers();
        }

        SelectContactsWidget select_members_dialog(NULL, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("groupchats", "Add to chat"), QT_TRANSLATE_NOOP("groupchats", "Done"), QString(), this);
        emit Utils::InterConnector::instance().searchEnd();
        connect(this, &HistoryControlPage::updateMembers, &select_members_dialog, &SelectContactsWidget::UpdateMembers, Qt::QueuedConnection);

        if (select_members_dialog.show() == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(aimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_add_member_dialog);
        }
        else
        {
            Logic::getContactListModel()->clearChecked();
        }
        Logic::setChatMembersModel(NULL);
    }

    void HistoryControlPage::renameContact()
    {
        QString result_chat_name;

        auto result = Utils::NameEditor(
            this,
            Logic::getContactListModel()->getDisplayName(aimId_),
            QT_TRANSLATE_NOOP("popup_window","Save"),
            QT_TRANSLATE_NOOP("popup_window", "Contact name"),
            result_chat_name);

        if (result && !result_chat_name.isEmpty())
        {
            Logic::getContactListModel()->renameContact(aimId_, result_chat_name);
        }
    }

    void HistoryControlPage::setState(const State _state, const char* _dbgWhere)
    {
        assert(_state > State::Min);
        assert(_state < State::Max);
        assert(state_ > State::Min);
        assert(state_ < State::Max);

        __INFO(
            "smooth_scroll",
            "switching state\n"
            "    from=<" << state_ << ">\n"
            "    to=<" << _state << ">\n"
            "    where=<" << _dbgWhere <<">\n"
            "    items num=<" << itemsData_.size() << ">"
        );

        state_ = _state;
    }

    bool HistoryControlPage::isState(const State _state) const
    {
        assert(state_ > State::Min);
        assert(state_ < State::Max);
        assert(_state > State::Min);
        assert(_state < State::Max);

        return (state_ == _state);
    }

    bool HistoryControlPage::isStateFetching() const
    {
        return isState(State::Fetching);
    }

    bool HistoryControlPage::isStateIdle() const
    {
        return isState(State::Idle);
    }

    bool HistoryControlPage::isStateInserting() const
    {
        return isState(State::Inserting);
    }

    void HistoryControlPage::postInsertNextMessageSignal(const char * _dbgWhere, bool _isMoveToBottomIfNeed, int64_t _mess_id)
    {
        __INFO(
            "smooth_scroll",
            "posting signal\n"
            "    type=<insertNextMessageSignal>\n"
            "    from=<" << _dbgWhere << ">"
        );

        //qDebug("postInsertNextMessageSignal %s", _dbgWhere);
        emit insertNextMessageSignal(_isMoveToBottomIfNeed, _mess_id);
    }

    void HistoryControlPage::postponeMessagesRequest(const char *_dbgWhere, bool _isDown)
    {
        assert(isStateInserting());

        dbgWherePostponed_ = _dbgWhere;
        isMessagesRequestPostponed_ = true;
        isMessagesRequestPostponedDown_ = _isDown;
    }

    void HistoryControlPage::switchToIdleState(const char* _dbgWhere)
    {
        //assert(isStateInserting());

        setState(State::Idle, _dbgWhere);
    }

    void HistoryControlPage::switchToInsertingState(const char* _dbgWhere)
    {
        //assert(isStateFetching());

        setState(State::Inserting, _dbgWhere);
    }

    void HistoryControlPage::switchToFetchingState(const char* _dbgWhere)
    {
      //  assert(isStateIdle() || isStateInserting());

        setState(State::Fetching, _dbgWhere);
    }

    void HistoryControlPage::setContactStatusClickable(bool _isEnabled)
    {
        if (_isEnabled)
        {
            contactStatusWidget_->setCursor(Qt::PointingHandCursor);
            connect(contactStatus_, SIGNAL(clicked()), this, SLOT(editMembers()), Qt::QueuedConnection);
        }
        else
        {
            contactStatusWidget_->setCursor(Qt::ArrowCursor);
            disconnect(contactStatus_, SIGNAL(clicked()), this, SLOT(editMembers()));
        }
        isContactStatusClickable_ = _isEnabled;
    }

    void HistoryControlPage::updateChatInfo()
    {
        if (!Logic::getContactListModel()->isChat(aimId_))
            return;

        if (chatMembersModel_ == NULL)
        {
            loadChatInfo(false);
        }
        else
        {
            loadChatInfo(chatMembersModel_->isFullListLoaded_);
        }
    }

    void HistoryControlPage::onReachedFetchingDistance(bool _isMoveToBottomIfNeed)
    {
        __INFO(
            "smooth_scroll",
            "initiating messages preloading..."
        );
        
        requestMoreMessagesAsync(__FUNCLINEA__, _isMoveToBottomIfNeed);
    }

    void HistoryControlPage::fetchMore(QString _aimId)
    {
        if (_aimId != aimId_)
        {
            return;
        }

        requestMoreMessagesAsync(__FUNCLINEA__);
    }

    void HistoryControlPage::nameClicked()
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else
            Utils::InterConnector::instance().showSidebar(aimId_, menu_page);
    }

    void HistoryControlPage::showEvent(QShowEvent* _event)
    {
        appendAuthControlIfNeed();

        QWidget::showEvent(_event);
    }


	void HistoryControlPage::setButtonDownPositions(int x_showed, int y_showed, int y_hided)
	{
		buttonDownShowPosition_.setX(x_showed);
		buttonDownShowPosition_.setY(y_showed);

		buttonDownHidePosition_.setX(x_showed);
		buttonDownHidePosition_.setY(y_hided);

		if (buttonDown_->isVisible())
			buttonDown_->move(buttonDownShowPosition_);
	}

    void HistoryControlPage::readByClient(QString _aimid, qint64 _id)
    {
        if (_aimid != aimId_)
            return;

        if (_id == -1)
            return;

        seenMsgId_ = _id;

        Ui::HistoryControlPageItem* lastReadItem = nullptr;

        messagesArea_->enumerateWidgets([_id, &lastReadItem](QWidget* _item, const bool)
        {
            if (qobject_cast<Ui::ServiceMessageItem*>(_item) /*|| qobject_cast<Ui::ChatEventItem*>(_item)*/)
            {
                return true;
            }

            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
            {
                return true;
            }

            auto isOutgoing = false;

            if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
            {
                isOutgoing = messageItem->isOutgoing();
            }
            else if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {
                isOutgoing = complexMessageItem->isOutgoing();
            }
            else if (auto voipMessageItem = qobject_cast<Ui::VoipEventItem*>(_item))
            {
                isOutgoing = voipMessageItem->isOutgoing();
            }


            if (lastReadItem)
            {
                pageItem->setLastRead(false);
                return true;
            }

            const auto itemId = pageItem->getId();
            assert(itemId >= -1);

            const bool isChatEvent = !!qobject_cast<Ui::ChatEventItem*>(_item);
            const auto itemHasId = (itemId != -1);
            const auto isItemRead = (itemHasId && (itemId <= _id));

            const auto markItemAsLastRead = ((!isOutgoing && !isChatEvent) || isItemRead);
            if (markItemAsLastRead)
            {
                lastReadItem = pageItem;
                return true;
            }

            pageItem->setLastRead(false);

            return true;

        }, false);

        if (lastReadItem)
        {
            lastReadItem->setLastRead(true);
        }
    }

    void HistoryControlPage::adminMenuRequest(QString _aimId)
    {
        if (!chatMembersModel_)
            return;

        if (chatMembersModel_->isModer() || chatMembersModel_->isAdmin())
        {
            auto cont = chatMembersModel_->getMemberItem(_aimId);
            if (!cont)
                return;

            auto menu = new ContextMenu(this);
            bool myInfo = cont->AimId_ == MyInfo()->aimId();
            if (cont->Role_ != "admin" && chatMembersModel_->isAdmin())
            {
                if (cont->Role_ == "moder")
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/removeking_100.png")), QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"), makeData("revoke_admin", _aimId));
                else
                    menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/king_100.png")), QT_TRANSLATE_NOOP("sidebar", "Make admin"), makeData("make_admin", _aimId));
            }

            if (cont->Role_ == "member")
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/onlyread_100.png")), QT_TRANSLATE_NOOP("sidebar", "Ban to write"), makeData("make_readonly", _aimId));
            else if (cont->Role_ == "readonly")
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/onlyread_off_100.png")), QT_TRANSLATE_NOOP("sidebar", "Allow to write"), makeData("revoke_readonly", _aimId));

            if ((cont->Role_ != "admin" && cont->Role_ != "moder") || myInfo)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/delete_100.png")), QT_TRANSLATE_NOOP("sidebar", "Delete from chat"), makeData("remove", _aimId));
            if (!myInfo)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/profile_100.png")), QT_TRANSLATE_NOOP("sidebar", "Profile"), makeData("profile", _aimId));
            if (cont->Role_ != "admin" && cont->Role_ != "moder")
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/block_100.png")), QT_TRANSLATE_NOOP("sidebar", "Block"), makeData("block", _aimId));
            if (!myInfo)
                menu->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/spam_100.png")), QT_TRANSLATE_NOOP("sidebar", "Report spam"), makeData("spam", _aimId));

            connect(menu, &ContextMenu::triggered, this, &HistoryControlPage::adminMenu, Qt::QueuedConnection);
            menu->popup(QCursor::pos());
        }
    }

    void HistoryControlPage::adminMenu(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const auto command = params["command"].toString();
        const auto aimId = params["aimid"].toString();

        if (command == "remove")
        {
            deleteMemberDialog(chatMembersModel_, aimId, Logic::MEMBERS_LIST, this);
        }

        if (command == "profile")
        {
            Utils::InterConnector::instance().showSidebar(aimId, profile_page);
        }

        if (command == "spam")
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_chat_avatar);
            }
        }

        if (command == "block")
        {
            blockUser(aimId, true);
        }

        if (command == "make_admin")
        {
            changeRole(aimId, true);
        }

        if (command == "make_readonly")
        {
            readonly(aimId, true);
        }

        if (command == "revoke_readonly")
        {
            readonly(aimId, false);
        }

        if (command == "revoke_admin")
        {
            changeRole(aimId, false);
        }
    }

    void HistoryControlPage::blockUser(const QString& _aimId, bool _blockUser)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _blockUser ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to block user in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_bool("block", _blockUser);
            Ui::GetDispatcher()->post_message_to_core("chats/block", collection.get());
        }
    }

    void HistoryControlPage::changeRole(const QString& _aimId, bool _moder)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _moder ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _moder ? "moder" : "member");
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }
    }

    void HistoryControlPage::readonly(const QString& _aimId, bool _readonly)
    {
        auto cont = chatMembersModel_->getMemberItem(_aimId);
        if (!cont)
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _readonly ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to ban user to write in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to allow user to write in this chat?"),
            cont->getFriendly(),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _readonly ? "readonly" : "member");
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }
    }

    void HistoryControlPage::actionResult(int)
    {
        if (chatMembersModel_)
            chatMembersModel_->loadAllMembers();
    }

    bool HistoryControlPage::contains(const QString& _aimId) const
    {
        return messagesArea_->contains(_aimId);
    }

    void HistoryControlPage::resumeVisibleItems()
    {
        assert(messagesArea_);
        if (messagesArea_)
        {
            messagesArea_->resumeVisibleItems();
        }
    }

    void HistoryControlPage::suspendVisisbleItems()
    {
        assert(messagesArea_);

        if (messagesArea_)
        {
            messagesArea_->suspendVisibleItems();
        }
    }

	void HistoryControlPage::scrollToBottomByButton()
	{
		GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_down_button);
		scrollToBottom();
	}

    void HistoryControlPage::scrollToBottom()
    {
        if (messagesArea_->getIsSearch())
        {
            Logic::GetMessagesModel()->tail(aimId_, messagesArea_, false /* _is_search */, -1 /* mess_id */, true /* _is_jump_to_bottom */);
        }

        messagesArea_->scrollToBottom();
    }

	void HistoryControlPage::onUpdateHistoryPosition(int32_t position, int32_t offset)
	{
		const auto shift = Utils::scale_value(button_shift);
		if (offset - position > shift)
			startShowButtonDown();
		else
			startHideButtonDown();
	}

	void HistoryControlPage::startShowButtonDown()
	{
		if (buttonDir_ != -1)
		{
			buttonDir_ = -1;
			buttonDown_->show();
            
            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();
		}
	}

	void HistoryControlPage::startHideButtonDown()
	{
		if (buttonDir_ != 1)
        {
			buttonDir_ = 1;
            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();
        }
	}

	void HistoryControlPage::onButtonDownMove()
	{
		if (buttonDir_ != 0)
		{
            qint64 cur = QDateTime::currentMSecsSinceEpoch();
            qint64 dt = cur - buttonDownCurrentTime_;
            buttonDownCurrentTime_ = cur;
            
            /// 16 ms = 1... 32ms = 2..
			buttonDownTime_ += (dt/16.f) * buttonDir_ * 1.f/15.f;
			if (buttonDownTime_ > 1.f)
				buttonDownTime_ = 1.f;
			else if (buttonDownTime_ < 0.f)
				buttonDownTime_ = 0.f;

			/// interpolation
			float val = buttonDownCurve_.valueForProgress(buttonDownTime_);
			QPoint p = buttonDownShowPosition_ + val * (buttonDownHidePosition_ - buttonDownShowPosition_);

			buttonDown_->move(p);

			if (!(buttonDownTime_ > 0.f && buttonDownTime_< 1.f))
			{
				if (buttonDir_ == 1)
					buttonDown_->hide();

				buttonDir_ = 0;
			}
		}
	}

    void HistoryControlPage::onButtonDownClicked()
    {
        /// subscribe message model to aimId_
        /// emit subscribe(aimId_);
    }

    void HistoryControlPage::showNewMessageForce()
	{
		bNewMessageForceShow_ = true;
	}

	void HistoryControlPage::setQuoteId(qint64 _quote_id)
	{
		quoteId_ = _quote_id;
	}
}

namespace
{
    bool isRemovableWidget(QWidget *_w)
	{
        assert(_w);

		const auto messageItem = qobject_cast<Ui::MessageItem*>(_w);
		if (!messageItem)
		{
			return true;
		}

		return messageItem->isRemovable();
	}

    bool isUpdateableWidget(QWidget* _w)
    {
        assert(_w);

        const auto messageItem = qobject_cast<Ui::MessageItem*>(_w);
        if (messageItem)
        {
            return messageItem->isUpdateable();
        }

        const auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_w);
        if (complexItem)
        {
            return complexItem->isUpdateable();
        }

        return true;
    }
}
