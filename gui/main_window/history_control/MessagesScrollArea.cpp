#include "stdafx.h"

#include "../ContactDialog.h"

#include "../smiles_menu/SmilesMenu.h"

#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../my_info.h"

#include "../../main_window/contact_list/ContactListModel.h"

#include "complex_message/ComplexMessageItem.h"

#include "MessageItem.h"
#include "MessagesScrollbar.h"
#include "MessagesScrollAreaLayout.h"

#include "MessagesScrollArea.h"
#include "../../gui_settings.h"

namespace
{
    double getMaximumScrollDistance();

    double getMinimumSpeed();

    double getMaximumSpeed();

    template<class T>
    T sign(const T value)
    {
        static_assert(
            std::is_arithmetic<T>::value,
            "std::is_arithmetic<T>::value");

        static_assert(
            std::numeric_limits<T>::is_signed,
            "std::numeric_limits<T>::is_signed");

        if (value == 0)
        {
            return 0;
        }

        return ((value > 0) ? 1 : -1);
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    const auto MOMENT_UNINITIALIZED = std::numeric_limits<int64_t>::min();

    const auto IDLE_USER_ACTIVITY_TIMEOUT_MS = (build::is_debug() ? 1000 : 10000);

    const auto scrollEasing = expoOut;
}

namespace Ui
{

    enum class MessagesScrollArea::ScrollingMode
    {
        Min,

        Plain,
        Selection,

        Max
    };

    MessagesScrollArea::MessagesScrollArea(QWidget *parent, QWidget *typingWidget)
        : QWidget(parent)
        , Scrollbar_(new MessagesScrollbar(this))
        , ScrollAnimationTimer_(this)
        , LastAnimationMoment_(MOMENT_UNINITIALIZED)
        , IsSelecting_(false)
        , ScrollDistance_(0)
        , Mode_(ScrollingMode::Plain)
        , Resizing_(false)
        , TouchScrollInProgress_(false)
        , IsUserActive_(false)
        , UserActivityTimer_(this)
        , Layout_(new MessagesScrollAreaLayout(this, Scrollbar_, typingWidget))
        , IsSearching_(false)
        , scrollValue_(-1)
    {
        assert(parent);
        assert(Layout_);
        assert(typingWidget);

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setContentsMargins(0, 0, 0, 0);
        setProperty("MessagesWidget", true);
        setLayout(Layout_);

        // initially invisible, will became visible when there will be enough content
        Scrollbar_->setVisible(false);

        ScrollAnimationTimer_.setInterval(10);
        ScrollAnimationTimer_.setTimerType(Qt::CoarseTimer);

        auto success = QObject::connect(
            &ScrollAnimationTimer_, &QTimer::timeout,
            this, &MessagesScrollArea::onAnimationTimer,
            Qt::QueuedConnection
        );
        assert(success);

        WheelEventsBufferResetTimer_.setInterval(300);
        WheelEventsBufferResetTimer_.setTimerType(Qt::CoarseTimer);

        success = QObject::connect(
            &WheelEventsBufferResetTimer_, &QTimer::timeout,
            this, &MessagesScrollArea::onWheelEventResetTimer,
            Qt::QueuedConnection
        );

        success = QObject::connect(
            Scrollbar_, &QScrollBar::sliderPressed,
            this, &MessagesScrollArea::onSliderPressed
        );
        assert(success);

        success = QObject::connect(
            Scrollbar_, &QScrollBar::valueChanged,
            this, &MessagesScrollArea::onSliderValue
        );
        assert(success);

        success = QObject::connect(
            Scrollbar_, &QScrollBar::sliderMoved,
            this, &MessagesScrollArea::onSliderMoved
        );
        assert(success);

        UserActivityTimer_.setInterval(IDLE_USER_ACTIVITY_TIMEOUT_MS);

        connect(&UserActivityTimer_, &QTimer::timeout,
                this, &MessagesScrollArea::onIdleUserActivityTimeout,
                Qt::QueuedConnection);

		connect(Layout_, &MessagesScrollAreaLayout::updateHistoryPosition, this, &MessagesScrollArea::onUpdateHistoryPosition);
        connect(Layout_, &MessagesScrollAreaLayout::recreateAvatarRect, this, &MessagesScrollArea::recreateAvatarRect);
        Utils::grabTouchWidget(this);
    }

	void MessagesScrollArea::onUpdateHistoryPosition(int32_t position, int32_t offset)
	{
		emit updateHistoryPosition(position, offset);
	}

    void MessagesScrollArea::onWheelEvent(QWheelEvent* e)
    {
        QCoreApplication::sendEvent(this, e);
    }

    void MessagesScrollArea::insertWidget(const Logic::MessageKey &key, QWidget *widget)
    {
        WidgetsList widgets;
        widgets.emplace_back(
            std::make_pair(key, widget)
        );
        insertWidgets(widgets, true /* _isMoveToButtonIfNeed */, -1 /* _mess_id */);
    }

    void MessagesScrollArea::insertWidgets(const WidgetsList& _widgets, bool _isMoveToButtonIfNeed, int64_t _mess_id)
    {
        assert(!_widgets.empty());
        Layout_->insertWidgets(_widgets, _isMoveToButtonIfNeed, _mess_id);

        for (auto iter : _widgets)
        {
            if (auto messageItem = qobject_cast<Ui::MessageItem*>(iter.second))
            {
                contacts_.insert(messageItem->getMchatSenderAimId());
                continue;
            }

            if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(iter.second))
            {
                contacts_.insert(complexItem->getSenderAimid());
                continue;
            }
        }

        updateScrollbar();
    }

    bool MessagesScrollArea::isScrolling() const
    {
        return ScrollAnimationTimer_.isActive();
    }

    bool MessagesScrollArea::isSelecting() const
    {
        return IsSelecting_;
    }

    bool MessagesScrollArea::isViewportFull() const
    {
        const auto scrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollRange = (scrollBounds.second - scrollBounds.first);
        assert(scrollRange >= 0);

        return (scrollRange > 0);
    }

    bool MessagesScrollArea::containsWidget(QWidget *widget) const
    {
        assert(widget);

        return Layout_->containsWidget(widget);
    }

    void MessagesScrollArea::eraseContact(QWidget* widget)
    {
        if (auto messageItem = qobject_cast<Ui::MessageItem*>(widget))
        {
            contacts_.erase(messageItem->getMchatSenderAimId());
        }
        else if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
        {
            contacts_.erase(complexItem->getSenderAimid());
        }
    }

    void MessagesScrollArea::removeWidget(QWidget *widget)
    {
        assert(widget);

        eraseContact(widget);

        Layout_->removeWidget(widget);

        updateScrollbar();
    }


    void MessagesScrollArea::replaceWidget(const Logic::MessageKey &key, QWidget *widget)
    {
        assert(key.hasId());
        assert(widget);

        auto existingWidget = getItemByKey(key);
        if (!existingWidget)
        {
            return;
        }

        removeWidget(existingWidget);

        insertWidget(key, widget);
    }

    bool MessagesScrollArea::touchScrollInProgress() const
    {
        return TouchScrollInProgress_ || isScrolling();
    }

    void MessagesScrollArea::scrollToBottom()
    {
        setIsSearch(false);
        stopScrollAnimation();

        if (!Layout_->isViewportAtBottom())
        {
            Layout_->setViewportByOffset(0);
        }

        resetUserActivityTimer();
        updateScrollbar();
    }

    void MessagesScrollArea::updateItemKey(const Logic::MessageKey &key)
    {
        Layout_->updateItemKey(key);
    }

    void MessagesScrollArea::updateScrollbar()
    {
        const auto viewportScrollBounds = Layout_->getViewportScrollBounds();

        auto scrollbarMaximum = (viewportScrollBounds.second - viewportScrollBounds.first);
        assert(scrollbarMaximum >= 0);

        const auto scrollbarVisible = (scrollbarMaximum > 0);
        Scrollbar_->setVisible(scrollbarVisible);

        Scrollbar_->setMaximum(scrollbarMaximum);

        if (!scrollbarVisible)
        {
            return;
        }

        const auto viewportAbsY = Layout_->getViewportAbsY();
        const auto scrollPos = (viewportAbsY - viewportScrollBounds.first);
        //assert(scrollPos >= 0);

        Scrollbar_->setValue(scrollPos);
    }

    void MessagesScrollArea::cancelSelection()
    {
        IsSelecting_ = false;
    }

    void MessagesScrollArea::cancelWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.stop();
    }

    void MessagesScrollArea::enumerateMessagesItems(const MessageItemVisitor visitor, const bool reversed) const
    {
        assert(visitor);
        Layout_->enumerateMessagesItems(visitor, reversed);
    }

    void MessagesScrollArea::enumerateWidgets(const WidgetVisitor visitor, const bool reversed) const
    {
        assert(visitor);
        Layout_->enumerateWidgets(visitor, reversed);
    }

    QWidget* MessagesScrollArea::getItemByPos(const int32_t pos) const
    {
        return Layout_->getItemByPos(pos);
    }

    QWidget* MessagesScrollArea::getItemByKey(const Logic::MessageKey &key) const
    {
        return Layout_->getItemByKey(key);
    }

    int32_t MessagesScrollArea::getItemsCount() const
    {
        return Layout_->getItemsCount();
    }

    Logic::MessageKeyVector MessagesScrollArea::getItemsKeys() const
    {
        return Layout_->getItemsKeys();
    }

    QString MessagesScrollArea::getSelectedText() const
    {
        QString selection_text;
        QString first_message_text_only;
        QString first_message_full;

        enumerateWidgets(
            [&selection_text, &first_message_text_only, &first_message_full]
            (QWidget* _item, const bool)
            {
                QString selected_full;

                if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
                {
                    selected_full = messageItem->selection(false);

                    if (first_message_text_only.isEmpty())
                    {
                        first_message_text_only = messageItem->selection(true);
                        first_message_full = selected_full;

                        return true;
                    }
                }

                if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
                {
                    selected_full = complexItem->getSelectedText(true);

                    if (first_message_text_only.isEmpty())
                    {
                        const auto selected_text = complexItem->getSelectedText(false);
                        first_message_text_only = selected_text;

                        first_message_full = selected_full;

                        return true;
                    }
                }

                if (selected_full.isEmpty())
                {
                    return true;
                }

                if (!selection_text.isEmpty())
                {
                    selection_text += QChar::LineFeed;
                    selection_text += QChar::LineFeed;
                }

                selection_text += selected_full;

                return true;
            }, true
        );

        auto result = (
            selection_text.isEmpty() ?
                first_message_text_only :
                (first_message_full + QChar::LineFeed + QChar::LineFeed + selection_text));

        if (!result.isEmpty() && result.endsWith("\n\n"))
        {
            result = result.left(result.length() - 1);
        }

        return result;
    }

    QList<Data::Quote> MessagesScrollArea::getQuotes() const
    {
        QList<Data::Quote> result;
        enumerateWidgets(
        [&result] (QWidget* _item, const bool)
        {
            //QString selected_full;

            if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
            {
                QString selectedText = messageItem->selection(true);
                if (selectedText.isEmpty())
                    return true;

                result.push_back(messageItem->getQuote());
                return true;
            }
            else if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {

                QString selectedText = complexItem->getSelectedText(false);
                if (selectedText.isEmpty())
                    return true;

                result.append(complexItem->getQuotes());
                return true;
            }

            return true;
        }, true
            );

        return result;
    }

    QList<Logic::MessageKey> MessagesScrollArea::getKeysToUnload() const
    {
        if (!Layout_->isViewportAtBottom())
        {
            return QList<Logic::MessageKey>();
        }

        const auto itemsHeight = Layout_->getItemsHeight();

        const auto triggerThresholdK = 2.5;
        const auto triggerThreshold = (int32_t)(Layout_->getViewportHeight() * triggerThresholdK);
        assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (itemsHeight < triggerThreshold);
        if (triggerThreasholdNotReached)
        {
            return QList<Logic::MessageKey>();
        }

        const auto unloadThresholdK = 2.8;
        const auto unloadThreshold = (int32_t)(Layout_->getViewportHeight() * unloadThresholdK);
        assert(unloadThreshold > 0);

        return Layout_->getWidgetsOverBottomOffset(unloadThreshold);
    }

    void MessagesScrollArea::mouseMoveEvent(QMouseEvent *e)
    {
        QWidget::mouseMoveEvent(e);

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        if (!isLeftButton || !isGenuine)
        {
            return;
        }

        resetUserActivityTimer();

        LastMouseGlobalPos_ = e->globalPos();

        const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

        if (SelectionBeginAbsPos_.isNull())
        {
            SelectionBeginAbsPos_ = mouseAbsPos;
        }

        SelectionEndAbsPos_ = mouseAbsPos;

        applySelection();

        auto mouseY = e->pos().y();

        const auto topScrollStartPosition = 0;

        const auto scrollUp = (mouseY <= topScrollStartPosition);
        if (scrollUp)
        {
            ScrollDistance_ = mouseY;
            startScrollAnimation(ScrollingMode::Selection);
            return;
        }

        const auto bottomScrollStartPosition = (height());
        assert(bottomScrollStartPosition > 0);

        const auto scrollDown = (mouseY >= bottomScrollStartPosition);
        if (scrollDown)
        {
            assert(!scrollUp);

            ScrollDistance_ = (mouseY - bottomScrollStartPosition);
            startScrollAnimation(ScrollingMode::Selection);

            return;
        }

        stopScrollAnimation();
    }

    void MessagesScrollArea::mousePressEvent(QMouseEvent *e)
    {
        resetUserActivityTimer();

        QWidget::mousePressEvent(e);

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        auto smilesMenu = contactDialog->getSmilesMenu();

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        const auto isSelectionStart = (isLeftButton && isGenuine && smilesMenu->IsHidden());

        if (!isSelectionStart)
        {
            return;
        }

        const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

        if (!(e->modifiers() & Qt::ShiftModifier))
        {
            clearSelection();

            LastMouseGlobalPos_ = e->globalPos();

            SelectionBeginAbsPos_ = mouseAbsPos;
            SelectionEndAbsPos_ = mouseAbsPos;

            applySelection(true);
        }
        else
        {
            if (SelectionBeginAbsPos_.isNull())
            {
                SelectionBeginAbsPos_ = mouseAbsPos;
                SelectionEndAbsPos_ = mouseAbsPos;
            }

            auto old = LastMouseGlobalPos_;
            LastMouseGlobalPos_ = e->globalPos();
            applySelection();
            LastMouseGlobalPos_ = old;
        }

        IsSelecting_ = true;
    }

    void MessagesScrollArea::mouseReleaseEvent(QMouseEvent *e)
    {
        if (IsSelecting_)
        {
            notifySelectionChanges();
        }

        resetUserActivityTimer();

        QWidget::mouseReleaseEvent(e);

        IsSelecting_ = false;

        if (e->source() == Qt::MouseEventNotSynthesized)
        {
            stopScrollAnimation();
        }
    }

    void MessagesScrollArea::notifySelectionChanges()
    {
        int selectedItems = 0;

        enumerateWidgets(
            [this, &selectedItems] (QWidget* _item, const bool)
            {
                if (auto messageItem = qobject_cast<Ui::MessageItem*>(_item))
                {
                    if (messageItem->isSelected() || messageItem->isTextSelected())
                        ++selectedItems;
                }
                else if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
                {
                    if (complexItem->isSelected())
                        ++selectedItems;
                }
                return true;
            },
            false);

        if (selectedItems > 0)
            emit messagesSelected();
        else
            emit messagesDeselected();
    }

    void MessagesScrollArea::wheelEvent(QWheelEvent *e)
    {
        e->accept();

        resetUserActivityTimer();

        if (!Scrollbar_->isVisible())
        {
            stopScrollAnimation();
            return;
        }

        cancelWheelBufferReset();

        startScrollAnimation(ScrollingMode::Plain);

        const auto pxDelta = e->pixelDelta();
        const auto isHorMove = (
            !pxDelta.isNull() &&
            (std::abs(pxDelta.x()) >= std::abs(pxDelta.y())));

        if (isHorMove)
        {
            return;
        }

        auto delta = e->delta();
        delta /= Utils::scale_value(1);
        delta = -delta;

        if (platform::is_apple())
        {
            delta /= Utils::scale_value(2);
        }

        if (delta == 0)
        {
            return;
        }

        const auto directionChanged = enqueWheelEvent(delta);
        if (directionChanged)
        {
            ScrollDistance_ = 0;
            WheelEventsBuffer_.clear();
        }

        ScrollDistance_ += delta;

        ScrollDistance_ = std::max(ScrollDistance_, -getMaximumScrollDistance());
        ScrollDistance_ = std::min(ScrollDistance_, getMaximumScrollDistance());
    }

    void MessagesScrollArea::scroll(ScrollDerection direction, int delta)
    {
        if (delta == 0)
        {
            return;
        }

        resetUserActivityTimer();

        if (!Scrollbar_->isVisible())
        {
            stopScrollAnimation();
            return;
        }

        cancelWheelBufferReset();

        startScrollAnimation(ScrollingMode::Plain);

        delta = direction == UP ? -delta : delta;

        ScrollDistance_ += delta;

        ScrollDistance_ = std::max(ScrollDistance_, -getMaximumScrollDistance());
        ScrollDistance_ = std::min(ScrollDistance_, getMaximumScrollDistance());
    }

    bool MessagesScrollArea::contains(const QString& _aimId) const
    {
        return contacts_.find(_aimId) != contacts_.end();
    }

    void MessagesScrollArea::resumeVisibleItems()
    {
        assert(Layout_);

        Layout_->resumeVisibleItems();
    }

    void MessagesScrollArea::suspendVisibleItems()
    {
        assert(Layout_);

        Layout_->suspendVisibleItems();
    }

    bool MessagesScrollArea::event(QEvent *e)
    {
		/// TODO - Check for hiding scrollbar with resize history window
		/*
		if (e->type() == QEvent::Resize)
		{
			Layout_->updateScrollbar();
			Layout_->updateBounds();
		}
		*/
        if (e->type() == QEvent::TouchBegin)
        {
            TouchScrollInProgress_ = true;
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            PrevTouchPoint_ = te->touchPoints().first().pos();
            e->accept();

            return true;
        }
        else if (e->type() == QEvent::TouchUpdate)
        {
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            if (te->touchPointStates() & Qt::TouchPointMoved)
            {
                QPointF newPoint = te->touchPoints().first().pos();
                if (!newPoint.isNull())
                {
                    int touchDelta = newPoint.y() - PrevTouchPoint_.y();
                    PrevTouchPoint_ = newPoint;

                    startScrollAnimation(ScrollingMode::Plain);

                    const auto wheelMultiplier = -1;
                    const auto delta = (touchDelta * wheelMultiplier);
                    assert(delta != 0);

                    const auto wheelDirection = sign(delta);

                    const auto currentDirection = sign(ScrollDistance_);

                    const auto directionChanged = (wheelDirection != currentDirection);
                    if (directionChanged)
                    {
                        ScrollDistance_ = 0;
                    }

                    ScrollDistance_ += delta;
                }
            }

            e->accept();
            return true;
        }
        else if (e->type() == QEvent::TouchEnd || e->type() == QEvent::TouchCancel)
        {
            TouchScrollInProgress_ = false;
            e->accept();

            return true;
        }

        return QWidget::event(e);
    }

    void MessagesScrollArea::onAnimationTimer()
    {
        if (!ScrollAnimationTimer_.isActive())
        {
            return;
        }

        if (ScrollDistance_ == 0)
        {
            stopScrollAnimation();
            return;
        }

        const auto now = QDateTime::currentMSecsSinceEpoch();

        const auto step = evaluateScrollingStep(now);

        const auto emptyStep = ((int32_t)step == 0);
        if (!emptyStep && !Layout_->shiftViewportAbsY(step))
        {
            stopScrollAnimation();
            return;
        }

        updateScrollbar();

        const auto isPlainMode = (Mode_ == ScrollingMode::Plain);
        if (isPlainMode)
        {
            const auto newScrollDistance = (ScrollDistance_ - step);

            const auto stop = (sign(newScrollDistance) != sign(ScrollDistance_));
            if (stop)
            {
                stopScrollAnimation();
                return;
            }

            ScrollDistance_ = newScrollDistance;
        }

        LastAnimationMoment_ = now;
    }

    void MessagesScrollArea::onMessageHeightChanged(QSize oldSize, QSize newSize)
    {
        if (oldSize == newSize)
        {
            assert(false);
        }
    }

    void MessagesScrollArea::onSliderMoved(int value)
    {
        stopScrollAnimation();

        Layout_->setViewportByOffset(
            Scrollbar_->maximum() - value
        );

        resetUserActivityTimer();
    }

    void MessagesScrollArea::onSliderPressed()
    {
        stopScrollAnimation();
    }

    void MessagesScrollArea::onSliderValue(int value)
    {
        assert(value >= 0);

        const auto scrollBounds = Layout_->getViewportScrollBounds();
        const auto absY = Layout_->getViewportAbsY();

        if ((scrollValue_ == -1 || scrollValue_ < value) && IsSearching_ && MessagesScrollbar::getPreloadingDistance() + absY > scrollBounds.second)
        {
            emit fetchRequestedEvent(false /* don't move to bottom */);
        }
        else if ((scrollValue_ == -1 || scrollValue_ > value) && Scrollbar_->isInFetchRange(value))
        {
            emit fetchRequestedEvent(true);
        } 

        if (Mode_ == ScrollingMode::Selection)
        {
            applySelection();
        }

        if (isScrollAtBottom())
        {
            emit scrollMovedToBottom();
        }

        scrollValue_ = value;
        Layout_->updateDistanceForViewportItems();
    }

    void MessagesScrollArea::applySelection(const bool forShift)
    {
        assert(!LastMouseGlobalPos_.isNull());

        const auto selectionBegin = Layout_->absolute2Viewport(SelectionBeginAbsPos_);
        const auto selectionEnd = Layout_->absolute2Viewport(SelectionEndAbsPos_);
        const auto selectionBeginGlobal = mapToGlobal(selectionBegin);
        const auto selectionEndGlobal = mapToGlobal(selectionEnd);

        enumerateWidgets(
            [this, forShift, selectionBeginGlobal, selectionEndGlobal](QWidget *w, const bool)
            {
                if (auto messageItem = qobject_cast<Ui::MessageItem*>(w))
                {
                    messageItem->selectByPos(LastMouseGlobalPos_, forShift);
                }
                else if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(w))
                {
                    complexItem->selectByPos(selectionBeginGlobal, selectionEndGlobal);
                }

                return true;
            },
            false
        );
    }

    void MessagesScrollArea::clearSelection()
    {
        SelectionBeginAbsPos_ = QPoint();
        SelectionEndAbsPos_ = QPoint();

        enumerateWidgets(
            [](QWidget *w, const bool)
            {
                if (auto messageItem = qobject_cast<Ui::MessageItem*>(w))
                {
                    messageItem->clearSelection();
                }
                else if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(w))
                {
                    complexItem->clearSelection();
                }

                return true;
            },
            false
        );

        emit messagesDeselected();
    }

    bool MessagesScrollArea::enqueWheelEvent(const int32_t delta)
    {
        const auto WHEEL_HISTORY_LIMIT = 3;

        const auto directionBefore = evaluateWheelHistorySign();

        WheelEventsBuffer_.emplace_back(delta);

        const auto isHistoryOverwhelmed = (WheelEventsBuffer_.size() > WHEEL_HISTORY_LIMIT);
        if (isHistoryOverwhelmed)
        {
            WheelEventsBuffer_.pop_front();
        }

        const auto directionAfter = evaluateWheelHistorySign();

        assert(WheelEventsBuffer_.size() <= WHEEL_HISTORY_LIMIT);
        const auto isHistoryFilled = (WheelEventsBuffer_.size() == WHEEL_HISTORY_LIMIT);

        const auto isSignChanged = (
            (directionAfter != 0) &&
            (directionBefore != 0) &&
            (directionAfter != directionBefore));

        return (isHistoryFilled && isSignChanged);
    }

    double MessagesScrollArea::evaluateScrollingVelocity() const
    {
        assert(ScrollDistance_ != 0);
        assert(std::abs(ScrollDistance_) <= getMaximumScrollDistance());

        auto speed = scrollEasing(
            std::abs(ScrollDistance_),
            getMaximumScrollDistance(),
            getMinimumSpeed(),
            getMaximumSpeed());

        speed *= sign(ScrollDistance_);

        return speed;
    }

    double MessagesScrollArea::evaluateScrollingStep(const int64_t now) const
    {
        assert(now > 0);

        const auto timeDiff = (now - LastAnimationMoment_);

        const auto velocity = evaluateScrollingVelocity();

        auto step = velocity;
        step *= timeDiff;
        step /= 1000;

        if (step == 0)
        {
            step = sign(velocity);
        }

        return step;
    }

    int32_t MessagesScrollArea::evaluateWheelHistorySign() const
    {
        const auto sum = std::accumulate(
            WheelEventsBuffer_.cbegin(),
            WheelEventsBuffer_.cend(),
            0);

        return sign(sum);
    }

    void MessagesScrollArea::scheduleWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.start();
    }

    void MessagesScrollArea::startScrollAnimation(const ScrollingMode mode)
    {
        assert(mode > ScrollingMode::Min);
        assert(mode < ScrollingMode::Max);

        Mode_ = mode;

        if (isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.start();

        assert(LastAnimationMoment_ == MOMENT_UNINITIALIZED);
        LastAnimationMoment_ = QDateTime::currentMSecsSinceEpoch();
    }

    void MessagesScrollArea::stopScrollAnimation()
    {
        LastAnimationMoment_ = MOMENT_UNINITIALIZED;
        ScrollDistance_ = 0;

        scheduleWheelBufferReset();

        if (!isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.stop();
    }

    void MessagesScrollArea::resetUserActivityTimer()
    {
        UserActivityTimer_.start();
    }

    void MessagesScrollArea::onIdleUserActivityTimeout()
    {
        UserActivityTimer_.stop();

        auto keysToUnload = getKeysToUnload();
        if (keysToUnload.empty())
        {
            return;
        }

        emit needCleanup(keysToUnload);
    }

    void MessagesScrollArea::onWheelEventResetTimer()
    {
        WheelEventsBuffer_.clear();

        WheelEventsBufferResetTimer_.stop();
    }

    bool MessagesScrollArea::isScrollAtBottom() const
    {
        return Layout_->isViewportAtBottom();
    }

    void MessagesScrollArea::setIsSearch(bool _isSearch)
    {
        IsSearching_ = _isSearch;
    }

    bool MessagesScrollArea::getIsSearch() const
    {
        return IsSearching_;
    }

    void MessagesScrollArea::updateItems()
    {
        Layout_->updateItemsWidth();
    }
}

namespace
{
    double getMaximumScrollDistance()
    {
        return Utils::scale_value(3000);
    }

    double getMinimumSpeed()
    {
        return Utils::scale_value(100);
    }

    double getMaximumSpeed()
    {
        return Utils::scale_value(3000);
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#cub2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        t /= d;

        t--;

        return (c * (std::pow(t, 3.0) + 1) + b);
    }

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#expo2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        return (c * (-std::pow(2.0, -10 * t/d) + 1) + b);
    }
}
