#include "stdafx.h"

#include "../../utils/log/log.h"
#include "../../utils/utils.h"

#include "complex_message/ComplexMessageItem.h"
#include "ContentWidgets/PreviewContentWidget.h"

#include "ChatEventItem.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "ServiceMessageItem.h"
#include "VoipEventItem.h"

#include "MessagesScrollArea.h"
#include "MessagesScrollbar.h"

#include "MessagesScrollAreaLayout.h"

#include "../../utils/InterConnector.h"
#include "../MainWindow.h"
#include "../../gui_settings.h"
#include "complex_message/ComplexMessageItem.h"


namespace Ui
{
    namespace
    {
        void applyWidgetWidth(const int32_t viewportWidth, QWidget *widget, bool traverseLayout);

        int32_t evaluateWidgetHeight(QWidget *widget);
    }

    enum class MessagesScrollAreaLayout::SlideOp
    {
        Min,

        NoSlide,
        SlideUp,
        SlideDown,

        Max,
    };

    QTextStream& operator <<(QTextStream &lhs, const MessagesScrollAreaLayout::SlideOp slideOp)
    {
        assert(slideOp > MessagesScrollAreaLayout::SlideOp::Min);
        assert(slideOp < MessagesScrollAreaLayout::SlideOp::Max);

        switch(slideOp)
        {
            case MessagesScrollAreaLayout::SlideOp::NoSlide: lhs << "no-slide"; break;
            case MessagesScrollAreaLayout::SlideOp::SlideUp: lhs << "slide-up"; break;
            case MessagesScrollAreaLayout::SlideOp::SlideDown: lhs << "slide-down"; break;
            default:
                ;
        }

        return lhs;
    }
        
    MessagesScrollAreaLayout::ItemInfo::ItemInfo(QWidget *widget, const Logic::MessageKey &key)
        : Widget_(widget)
        , Key_(key)
        , IsVisible_(false)
        , IsGeometrySet_(false)
        , IsHovered_(false)
        , IsActive_(false)
    {
        assert(Widget_);
    }

    MessagesScrollAreaLayout::MessagesScrollAreaLayout(
        MessagesScrollArea *scrollArea,
        MessagesScrollbar *messagesScrollbar,
        QWidget *typingWidget)
        : QLayout(scrollArea)
        , Scrollbar_(messagesScrollbar)
        , ScrollArea_(scrollArea)
        , ViewportSize_(0, 0)
        , ViewportAbsY_(0)
        , IsDirty_(false)
        , UpdatesLocked_(false)
        , TypingWidget_(typingWidget)
        , QuoteId_(-1)
    {
        assert(ScrollArea_);
        assert(Scrollbar_);
        assert(TypingWidget_);

        ScrollArea_->installEventFilter(this);
        Utils::InterConnector::instance().getMainWindow()->installEventFilter(this);

        connect(scrollArea, &MessagesScrollArea::buttonDownClicked, this, &MessagesScrollAreaLayout::onButtonDownClicked, Qt::DirectConnection);
        connect(Logic::GetMessagesModel(), &Logic::MessagesModel::quote, this, &MessagesScrollAreaLayout::onQuote, Qt::QueuedConnection);

        connect(this, &MessagesScrollAreaLayout::moveViewportUpByScrollingItem, this, &MessagesScrollAreaLayout::onMoveViewportUpByScrollingItem, Qt::QueuedConnection);
    }

    MessagesScrollAreaLayout::~MessagesScrollAreaLayout()
    {
        ScrollArea_->removeEventFilter(this);
        Utils::InterConnector::instance().getMainWindow()->removeEventFilter(this);
    }

    void MessagesScrollAreaLayout::setGeometry(const QRect &r)
    {
        QLayout::setGeometry(r);

        if (UpdatesLocked_)
        {
            return;
        }

        // -----------------------------------------------------------------------
        // setup initial viewport position if needed

        if (LayoutRect_.isEmpty())
        {
            assert(ViewportAbsY_ == 0);
            ViewportAbsY_ = (-r.height() + getTypingWidgetHeight());
        }

        // ------------------------------------------------------------------------
        // check if the height of some item had been changed

        if (IsDirty_)
        {
            updateItemsGeometry();

            IsDirty_ = false;
        }

		// ------------------------------------------------------------------------
		// update button down in history
		updateBounds();

        // -----------------------------------------------------------------------
        // check if layout rectangle changed

        const auto layoutRectChanged = (r != LayoutRect_);
        if (!layoutRectChanged)
        {
            return;
        }

        const auto widthChanged = (r.width() != LayoutRect_.width());

        LayoutRect_ = r;

        // -----------------------------------------------------------------------
        // set scrollbar geometry

        const auto scrollbarWidth = Scrollbar_->sizeHint().width();
        assert(scrollbarWidth > 0);

        const QRect scrollbarGeometry(
            LayoutRect_.right() - scrollbarWidth,
            LayoutRect_.top(),
            scrollbarWidth,
            LayoutRect_.height()
        );

        Scrollbar_->setGeometry(scrollbarGeometry);

        // -----------------------------------------------------------------------
        // set viewport geometry

        const QSize newViewportSize(
            LayoutRect_.width() - scrollbarWidth,
            LayoutRect_.height()
        );

        __TRACE(
            "geometry",
            "    old-viewport-size=<" << ViewportSize_ << ">\n"
            "    new-viewport-size=<" << newViewportSize << ">\n"
            "    new-layout-rect=<" << LayoutRect_ << ">"
        );

        const auto isAtBottom = isViewportAtBottom();

        ViewportSize_ = newViewportSize;

        // -----------------------------------------------------------------------
        // update items width

        if (widthChanged)
        {
            UpdatesLocked_ = true;

            updateItemsWidth();

            UpdatesLocked_ = false;
        }

        // -----------------------------------------------------------------------
        // lock scroll at bottom (if needed)

        if (isAtBottom)
        {
            moveViewportToBottom();

            UpdatesLocked_ = true;

            applyItemsGeometry();

            applyTypingWidgetGeometry();

            UpdatesLocked_ = false;
        }
    }

    void MessagesScrollAreaLayout::addItem(QLayoutItem* /*item*/)
    {
    }

    QLayoutItem* MessagesScrollAreaLayout::itemAt(int /*index*/) const
    {
        return nullptr;
    }

    QLayoutItem* MessagesScrollAreaLayout::takeAt(int /*index*/)
    {
        return nullptr;
    }

    int MessagesScrollAreaLayout::count() const
    {
        return 0;
    }

    QSize MessagesScrollAreaLayout::sizeHint() const
    {
        return QSize();
    }

    void MessagesScrollAreaLayout::invalidate()
    {
        IsDirty_ = true;

        QLayout::invalidate();
    }

    QPoint MessagesScrollAreaLayout::absolute2Viewport(const QPoint absPos) const
    {
        auto result = absPos;

        // apply the viewport transformation
        result.ry() -= ViewportAbsY_;

        return result;
    }

    bool MessagesScrollAreaLayout::containsWidget(QWidget *widget) const
    {
        assert(widget);

        return (Widgets_.count(widget) > 0);
    }

    QWidget* MessagesScrollAreaLayout::getItemByPos(const int32_t pos) const
    {
        assert(pos >= 0);
        assert(pos < (int32_t)LayoutItems_.size());

        if (pos < 0 || pos >= (int32_t)LayoutItems_.size())
            return nullptr;

        return LayoutItems_[pos]->Widget_;
    }

    QWidget* MessagesScrollAreaLayout::getItemByKey(const Logic::MessageKey &key) const
    {
        for (const auto &layoutItem : LayoutItems_)
        {
            if (layoutItem->Key_ == key)
            {
                return layoutItem->Widget_;
            }
        }

        return nullptr;
    }

    int32_t MessagesScrollAreaLayout::getItemsCount() const
    {
        return (int32_t)LayoutItems_.size();
    }

    int32_t MessagesScrollAreaLayout::getItemsHeight() const
    {
        const auto height = (getItemsAbsBounds().second - getItemsAbsBounds().first);
        assert(height >= 0);

        return height;
    }

    Logic::MessageKeyVector MessagesScrollAreaLayout::getItemsKeys() const
    {
        Logic::MessageKeyVector result;
        result.resize(LayoutItems_.size());

        auto itemIndex = (result.size() - 1);
        for (const auto &layoutItem : LayoutItems_)
        {
            result[itemIndex--] = layoutItem->Key_;
        }

        return result;
    }

    int32_t MessagesScrollAreaLayout::getViewportAbsY() const
    {
        return ViewportAbsY_;
    }

    MessagesScrollAreaLayout::Interval MessagesScrollAreaLayout::getViewportScrollBounds() const
    {
        if (LayoutItems_.empty())
        {
            assert(!ViewportSize_.isEmpty());

            const auto top = (-ViewportSize_.height() + getTypingWidgetHeight());

            return Interval(top, top);
        }

        const auto overallContentHeight = (getItemsHeight() + getTypingWidgetHeight());
        const auto scrollHeight = std::max(
            0,
            overallContentHeight - ViewportSize_.height()
        );

        if (scrollHeight == 0)
        {
            const auto &bottomItem = *LayoutItems_.begin();
            const auto bottom = bottomItem->AbsGeometry_.bottom();
            const auto top = (bottom + getTypingWidgetHeight() - ViewportSize_.height());

            return Interval(top, top);
        }

        const auto &topItem = *(LayoutItems_.end() - 1);
        const auto top = topItem->AbsGeometry_.top();
        const auto bottom = (top + scrollHeight);

        return Interval(top, bottom);
    }

    int32_t MessagesScrollAreaLayout::getViewportHeight() const
    {
        assert(!ViewportSize_.isEmpty());
        return ViewportSize_.height();
    }

    QList<Logic::MessageKey> MessagesScrollAreaLayout::getWidgetsOverBottomOffset(const int32_t offset) const
    {
        assert(offset >= 0);

        if (LayoutItems_.empty())
        {
            return QList<Logic::MessageKey>();
        }

        const auto itemsAbsBounds = getItemsAbsBounds();
        const auto absBottomY = itemsAbsBounds.second;
        const auto absThresholdY = (absBottomY - offset);

        QList<Logic::MessageKey> result;

        for (auto iter = LayoutItems_.crbegin(); iter != LayoutItems_.crend(); ++iter)
        {
            const auto &layoutItem = **iter;

            if (layoutItem.Widget_->property("permanent").toBool())
                continue;

            const auto &itemAbsGeometry = layoutItem.AbsGeometry_;

            const auto isItemUnderThreshold = (itemAbsGeometry.bottom() > absThresholdY);
            if (isItemUnderThreshold)
            {
                break;
            }

            result.push_back(layoutItem.Key_);
        }

        return result;
    }

    void MessagesScrollAreaLayout::insertWidgets(const WidgetsList& _widgets, bool _isMoveToButtonIfNeed, int64_t _mess_id)
    {
        assert(!_widgets.empty());
        UpdatesLocked_ = true;

        const auto isAtBottom = isViewportAtBottom();

        for (const auto& _widget_and_position : _widgets)
        {
            const auto key = _widget_and_position.first;
            auto widget = _widget_and_position.second;

            assert(widget);

            // -----------------------------------------------------------------------
            // check if the widget was already inserted

            if (containsWidget(widget))
            {
                assert(!"the widget is already in the layout");
                continue;
            }

            if (auto messageItem = qobject_cast<MessageItem*>(widget))
            {
                connect(
                    messageItem,
                    &MessageItem::selectionChanged,
                    ScrollArea_,
                    &MessagesScrollArea::notifySelectionChanges);
            }
            else if (auto complexMessage = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget))
            {
                connect(
                    complexMessage,
                    &ComplexMessage::ComplexMessageItem::selectionChanged,
                    ScrollArea_,
                    &MessagesScrollArea::notifySelectionChanges);
            }

            Widgets_.emplace(widget);

            // -----------------------------------------------------------------------
            // apply widget width (if needed)

            applyWidgetWidth(getWidthForItem(), widget, true);

            __TRACE(
                "geometry",
                "    is-at-bottom=<" << logutils::yn(isAtBottom) << ">");

            // -----------------------------------------------------------------------
            // find a proper place for the widget

            auto itemInfoIter = insertItem(widget, key);

            // -----------------------------------------------------------------------
            // show the widget to enable geometry operations on it

            widget->show();

            // -----------------------------------------------------------------------
            // calculate insertion position and make the space to put the widget in

            auto slideOp = SlideOp::NoSlide;

            {
                /// if widget exist in erased - get it

                //auto it = std::find_if(removeWidgets_.begin(), removeWidgets_.end(), [key](ItemInfo& val)->bool
                //{
                //    return val.Key_.getId() == key.getId();
                //});
                //if (it != removeWidgets_.end())
                //{
                //    (*itemInfoIter)->AbsGeometry_ = it->AbsGeometry_;
                //    (*itemInfoIter)->Widget_ = it->Widget_;

                //    auto widget_it = Widgets_.find(widget);
                //    if (widget_it != Widgets_.end())
                //        Widgets_.erase(widget_it);

                //    Widgets_.insert((*itemInfoIter)->Widget_);
                //    removeWidgets_.erase(it);
                //}
                //else
                {
                    const auto absolutePosition = calculateInsertionRect(itemInfoIter, Out slideOp);
                    (*itemInfoIter)->AbsGeometry_ = absolutePosition;
                }
            }

            const auto insertedItemHeight = (*itemInfoIter)->AbsGeometry_.height();
            slideItemsApart(itemInfoIter, insertedItemHeight, slideOp);

            //ServiceMessageItem* service_message = qobject_cast<ServiceMessageItem*>(widget);
            //if (service_message && service_message->isNew())
            //{
            //    const auto bMoveHistory = Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false);
            //    if (bMoveHistory)
            //    {
            //        ScrollingItems_.push_back(service_message);
            //        connect(service_message, &QObject::destroyed, this, &MessagesScrollAreaLayout::onDeactivateScrollingItems, Qt::DirectConnection);
            //    }
            //}
        }

        if (isAtBottom && _isMoveToButtonIfNeed)
        {
            moveViewportToBottom();
        }

        if (_mess_id != -1)
        {
            const auto delta = ViewportSize_.height() / 2;

            auto it = LayoutItems_.begin();

            while (it != LayoutItems_.end() && (*it)->Key_.getId() != _mess_id)
            {
                ++it;
            }

            assert(it != LayoutItems_.end());

            if (it == LayoutItems_.end())
            {
                it = LayoutItems_.begin();
            }

            QRect rect = (*it)->AbsGeometry_;
            ViewportAbsY_ = (rect.y() - delta);
        }

        applyItemsGeometry();

        applyTypingWidgetGeometry();

        UpdatesLocked_ = false;


        /// check new messages or quotes Later->Early
        //int max_items = 40;
        for (auto it = LayoutItems_.rbegin(); it != LayoutItems_.rend(); ++it)
        {
            auto widget = (*it)->Widget_;
            auto& key = (*it)->Key_;
            /// new message item
            //const auto bMoveHistory = Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false);
            //if (bMoveHistory)
            {
                ComplexMessage::ComplexMessageItem* complex_item = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget);
                if (complex_item && complex_item->isObserveToSize())
                    installEventFilter(this);

                /// quote
                if (key.getId() == QuoteId_ && QuoteId_ > 0)
                {
                    if (complex_item)
                    {
                        complex_item->setQuoteSelection();
                        complex_item->installEventFilter(this);

                        ScrollingItems_.push_back(complex_item);
                        connect(complex_item, &QObject::destroyed, this, &MessagesScrollAreaLayout::onDeactivateScrollingItems, Qt::UniqueConnection);
                    }
                    MessageItem* message_item = qobject_cast<MessageItem*>(widget);
                    if (message_item)
                        message_item->setQuoteSelection();
                }
            }
        }
        QuoteId_ = -1;

        if (!ScrollingItems_.empty())
        {
            moveViewportUpByScrollingItems();
        }
        //else if (control_widget)
        //{
        //    checkBounds();
        //}
        //endRemoveWidgets();

        //qDebug("INS :%i %i", LayoutItems_.size(), _widgets.size());
    }

    void MessagesScrollAreaLayout::removeWidget(QWidget* widget)
    {
        assert(widget);
        assert(widget->parent() == ScrollArea_);

        //dumpGeometry(QString().sprintf("before removal of %p", widget));

        // remove the widget from the widgets collection

        if (Widgets_.count(widget) == 0)
        {
            assert(!"the widget is not in the layout");
            return;
        }

        Widgets_.erase(widget);

        UpdatesLocked_ = true;

        const auto isAtBottom = isViewportAtBottom();

        // find the widget in the layout items

        auto iter = LayoutItems_.begin();
        for (; iter != LayoutItems_.end(); ++iter)
        {
            auto &item = **iter;

            if (item.Widget_ == widget)
            {
                break;
            }
        }

        assert(iter != LayoutItems_.end());

        // determine slide operation type

        const auto &layoutItemGeometry = (*iter)->AbsGeometry_;
        assert(layoutItemGeometry.height() >= 0);
        assert(layoutItemGeometry.width() > 0);

        const auto insertBeforeViewportMiddle = (layoutItemGeometry.top() < evalViewportAbsMiddleY());

        auto slideOp = SlideOp::NoSlide;
        if (!layoutItemGeometry.isEmpty())
        {
            slideOp = (insertBeforeViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown);
        }

        const auto itemsSlided = slideItemsApart(iter, -layoutItemGeometry.height(), slideOp);

        widget->hide();
        widget->deleteLater();

        LayoutItems_.erase(iter);

        if (isAtBottom)
        {
            moveViewportToBottom();
        }

        const auto isItemsDirty = (isAtBottom || itemsSlided);
        if (isItemsDirty)
        {
            applyItemsGeometry();
        }

        applyTypingWidgetGeometry();

        UpdatesLocked_ = false;

        //debugValidateGeometry();

        //dumpGeometry(QString().sprintf("after removal of %p", widget));
    }
   
    int32_t MessagesScrollAreaLayout::shiftViewportAbsY(const int32_t delta)
    {
        assert(delta != 0);

        const auto newViewportAbsY_ = (ViewportAbsY_ + delta);

        setViewportAbsY(newViewportAbsY_);

        return ViewportAbsY_;
    }

    QRect MessagesScrollAreaLayout::absolute2Viewport(const QRect &absolute) const
    {
        assert(absolute.isValid());

        auto result = absolute;

        // apply the viewport transformation
        result.translate(0, -ViewportAbsY_);

        result.setX(getXForItem());
        result.setWidth(getWidthForItem());

        return result;
    }

    void MessagesScrollAreaLayout::applyItemsGeometry()
    {
        const auto globalMousePos = QCursor::pos();
        const auto localMousePos = ScrollArea_->mapFromGlobal(globalMousePos);

        const auto viewportAbsRect = evalViewportAbsRect();

        const auto preloadMargin = Utils::scale_value(1900);
        const QMargins preloadMargins(0, preloadMargin, 0, preloadMargin);
        const auto viewportActivityAbsRect = viewportAbsRect.marginsAdded(preloadMargins);

        const auto visibilityMargin = Utils::scale_value(0);
        const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
        const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

        for (auto &item : LayoutItems_)
        {
            const auto &widgetAbsGeometry = item->AbsGeometry_;

            const auto isGeometryActive = viewportActivityAbsRect.intersects(widgetAbsGeometry);

            const auto isActivityChanged = (item->IsActive_ != isGeometryActive);
            if (isActivityChanged)
            {
                item->IsActive_ = isGeometryActive;

                onItemActivityChanged(item->Widget_, isGeometryActive);
            }

            auto isGeometryVisible = isVisibleEnoughForPlay(widgetAbsGeometry, viewportVisibilityAbsRect);

            const auto isInvisibleMovement = ((!isGeometryVisible && !item->IsVisible_) && item->IsGeometrySet_);
            if (isInvisibleMovement)
            {
                //continue;
            }

            const auto isVisibilityChanged = (item->IsVisible_ != isGeometryVisible);
            if (isVisibilityChanged)
            {
                item->IsVisible_ = isGeometryVisible;

                onItemVisibilityChanged(item->Widget_, isGeometryVisible);
            }

            if (!widgetAbsGeometry.isEmpty())
            {
                const auto widgetGeometry = absolute2Viewport(widgetAbsGeometry);

                const auto geometryChanged = (item->Widget_->geometry() != widgetGeometry);
                if (geometryChanged)
                {
                    item->Widget_->setGeometry(widgetGeometry);

                    simulateMouseEvents(*item, widgetGeometry, globalMousePos, localMousePos);
                }

            }

            item->IsGeometrySet_ = true;
        }
    }

    void MessagesScrollAreaLayout::applyTypingWidgetGeometry()
    {
        QRect typingWidgetGeometry(
            getXForItem(),
            getItemsAbsBounds().second,
            getWidthForItem(),
            getTypingWidgetHeight()
        );

        assert(TypingWidget_);
        assert(!typingWidgetGeometry.isEmpty());

        TypingWidget_->setGeometry(
            absolute2Viewport(typingWidgetGeometry)
        );
    }

    QRect MessagesScrollAreaLayout::calculateInsertionRect(const ItemsInfoIter &itemInfoIter, Out SlideOp &slideOp)
    {
        assert(itemInfoIter != LayoutItems_.end());

        Out slideOp = SlideOp::NoSlide;

        const auto widgetHeight = evaluateWidgetHeight((*itemInfoIter)->Widget_);
        assert(widgetHeight >= 0);

        const auto isInitialInsertion = (LayoutItems_.size() == 1);
        if (isInitialInsertion)
        {
            assert(itemInfoIter == LayoutItems_.begin());

            const QRect result(
                getXForItem(),
                -widgetHeight,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "initial insertion\n"
                "    " << result
            );

            return result;
        }

        const auto isAppend = ((itemInfoIter + 1) == LayoutItems_.end());
        if (isAppend)
        {
            // add to geometry top

            const auto &prevItemInfo = *(itemInfoIter - 1);

            const auto &prevItemGeometry = prevItemInfo->AbsGeometry_;
            assert(prevItemGeometry.width() > 0);
            assert(prevItemGeometry.height() >= 0);

            const QRect result(
                getXForItem(),
                prevItemGeometry.top() - widgetHeight,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "added to the top\n"
                "    geometry=<" << result << ">"
            );

            return result;
        }

        const auto isPrepend = (itemInfoIter == LayoutItems_.begin());
        if (isPrepend)
        {
            // add to geometry bottom

            const auto &nextItemInfo = *(itemInfoIter + 1);

            const auto &nextItemGeometry = nextItemInfo->AbsGeometry_;
            assert(nextItemGeometry.width() > 0);
            assert(nextItemGeometry.height() >= 0);

            const QRect result(
                getXForItem(),
                nextItemGeometry.bottom() + 1,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "added to the bottom\n"
                "    "<< result
            );

            return result;
        }

        // insert in the middle

        const auto &overItemInfo = **(itemInfoIter + 1);
        const auto &underItemInfo = **(itemInfoIter - 1);

        const auto insertionY = underItemInfo.AbsGeometry_.top();

        const auto insertBeforeViewportMiddle = (insertionY < evalViewportAbsMiddleY());

        if (insertBeforeViewportMiddle)
        {
            if (widgetHeight > 0)
            {
                Out slideOp =  SlideOp::SlideUp;
            }

            const QRect result(
                getXForItem(),
                overItemInfo.AbsGeometry_.bottom() + 1,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "inserted before viewport middle\n"
                "    geometry=<" << result << ">\n"
                "    height=<" << widgetHeight << ">\n"
                "    widget-over=<" << overItemInfo.Widget_ << ">\n"
                "    widget-over-geometry=<" << overItemInfo.AbsGeometry_ << ">"
            );

            return result;
        }

        if (widgetHeight > 0)
        {
            Out slideOp = SlideOp::SlideDown;
        }

        const QRect result(
            getXForItem(),
            underItemInfo.AbsGeometry_.top(),
            getWidthForItem(),
            widgetHeight
        );

        __TRACE(
            "geometry",
            "inserted after viewport middle\n"
            "    " << result
        );

        return result;
    }

    void MessagesScrollAreaLayout::debugValidateGeometry()
    {
        if (!build::is_debug())
        {
            return;
        }

        if (LayoutItems_.empty())
        {
            return;
        }

        auto iter = LayoutItems_.begin();
        for (; ; ++iter)
        {
            const auto next = (iter + 1);

            if (next == LayoutItems_.end())
            {
                break;
            }

            const auto &geometry = (*iter)->AbsGeometry_;
            const auto &nextGeometry = (*next)->AbsGeometry_;

            if (nextGeometry.bottom() > geometry.top())
            {
                __TRACE(
                    "geometry",
                    "    widget-above=<" << (*next)->Widget_ << ">\n"
                    "    widget-below=<" << (*iter)->Widget_ << ">\n"
                    "    widget-above-bottom=<" << nextGeometry.bottom() << ">\n"
                    "    widget-below-top=<" << geometry.top() << ">\n"
                    "    error=<intersection>"
                );
            }
        }
    }

    void MessagesScrollAreaLayout::dumpGeometry(const QString &notes)
    {
        assert(!notes.isEmpty());

        if (!build::is_debug())
        {
            return;
        }

        __INFO(
            "geometry.dump",
            "************************************************************************\n"
            "*** " << notes
        );

        __INFO(
            "geometry.dump",
            "    scroll-bounds=<" << getViewportScrollBounds() << ">\n"
            "    items-bounds=<" << getItemsAbsBounds() << ">"
        );

        for (auto iter = LayoutItems_.crbegin(); iter != LayoutItems_.crend(); ++iter)
        {
            const auto pos = (iter - LayoutItems_.crbegin());

            const auto widget = (*iter)->Widget_;

            const char *className = widget->metaObject()->className();

            const auto messageItem = qobject_cast<Ui::MessageItem*>(widget);
            const auto contentClassName = (messageItem ? messageItem->contentClass() : QString("no"));

            __INFO(
                "geometry.dump",
                "    index=<" << pos << ">\n"
                "    widget=<" << widget << ">\n"
                "    class=<" << className << ">\n"
                "    content=<" << contentClassName << ">\n"
                "    abs-geometry=<" << (*iter)->AbsGeometry_ << ">\n"
                "    rel-y-inclusive=<" << getRelY((*iter)->AbsGeometry_.top()) << "," << getRelY((*iter)->AbsGeometry_.bottom() + 1) << ">"
            );
        }

        __INFO(
            "geometry.dump",
            "*** " << notes << "\n"
            "************************************************************************"
        );
    }

    int32_t MessagesScrollAreaLayout::evalViewportAbsMiddleY() const
    {
        return (
            ViewportAbsY_ +
            (ViewportSize_.height() / 2)
        );
    }

    QRect MessagesScrollAreaLayout::evalViewportAbsRect() const
    {
        QRect result(
            0,
            ViewportAbsY_,
            ViewportSize_.width(),
            ViewportSize_.height());

        return result;
    }

    MessagesScrollAreaLayout::Interval MessagesScrollAreaLayout::getItemsAbsBounds() const
    {
        if (LayoutItems_.empty())
        {
            return Interval(0, 0);
        }

        const auto &topItem = **(LayoutItems_.cend() - 1);
        const auto &bottomItem = **LayoutItems_.cbegin();

        assert(!ViewportSize_.isEmpty());

        const auto itemsAbsTop = topItem.AbsGeometry_.top();
        const auto itemsAbsBottom = (bottomItem.AbsGeometry_.bottom() + 1);

        assert(itemsAbsBottom >= itemsAbsTop);
        return Interval(itemsAbsTop, itemsAbsBottom);
    }

    int32_t MessagesScrollAreaLayout::getRelY(const int32_t y) const
    {
        const auto itemsRect = getItemsAbsBounds();

        return (itemsRect.second - y);
    }

    int32_t MessagesScrollAreaLayout::getTypingWidgetHeight() const
    {
        assert(TypingWidget_);
        return TypingWidget_->height();
    }

    MessagesScrollAreaLayout::ItemsInfoIter MessagesScrollAreaLayout::insertItem(QWidget *widget, const Logic::MessageKey &key)
    {
        assert(widget);

        ItemInfoUptr info(new ItemInfo(widget, key));

        const auto isInitialInsertion = LayoutItems_.empty();
        if (isInitialInsertion)
        {
            return LayoutItems_.emplace(LayoutItems_.end(), std::move(info));
        }

        {
            // insert to the geometry bottom

            const auto iterFirst = LayoutItems_.begin();
            const auto &keyFirst = (*iterFirst)->Key_;
            const auto isPrepend = (keyFirst < key);
            if (isPrepend)
            {
                LayoutItems_.emplace_front(std::move(info));

                return LayoutItems_.begin();
            }
        }

        {
            // insert to the geometry top

            const auto iterLast = (LayoutItems_.end() - 1);
            const auto &keyLast = (*iterLast)->Key_;
            const auto isAppend = (key < keyLast);
            if (isAppend)
            {
                return LayoutItems_.emplace(LayoutItems_.end(), std::move(info));
            }
        }


        // insert into the middle

        auto iter = LayoutItems_.begin();
        for (; iter != LayoutItems_.end(); ++iter)
        {
            if ((*iter)->Key_ < key)
            {
                break;
            }
        }

        return LayoutItems_.emplace(iter, std::move(info));
    }

    bool MessagesScrollAreaLayout::isViewportAtBottom() const
    {
        if (LayoutItems_.empty())
        {
            return true;
        }

        const auto scrollBounds = getViewportScrollBounds();
        const auto isEmpty = (scrollBounds.second == scrollBounds.first);
        if (isEmpty)
        {
            return true;
        }

        const auto isAtBottom = (
            ViewportAbsY_ >= scrollBounds.second
        );

        __TRACE(
            "geometry",
            "    viewport-y=<" << ViewportAbsY_ << ">\n"
            "    scroll-bounds=<" << scrollBounds << ">"
        );

        return isAtBottom;
    }

    void MessagesScrollAreaLayout::resumeVisibleItems()
    {
        for (auto &item : LayoutItems_)
        {
            if (item->IsGeometrySet_)
            {
                item->IsActive_ = true;

                onItemActivityChanged(item->Widget_, true);

                item->IsVisible_ = true;


                const auto &widgetAbsGeometry = item->AbsGeometry_;
                const auto viewportAbsRect = evalViewportAbsRect();
                const auto isGeometryActive = viewportAbsRect.intersects(widgetAbsGeometry);

                const auto isActivityChanged = (item->IsActive_ != isGeometryActive);
                if (isActivityChanged)
                {
                    item->IsActive_ = isGeometryActive;

                    onItemActivityChanged(item->Widget_, isGeometryActive);
                }

                const auto visibilityMargin = Utils::scale_value(0);
                const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
                const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

                auto isGeometryVisible = isVisibleEnoughForPlay(widgetAbsGeometry, viewportVisibilityAbsRect);

                onItemVisibilityChanged(item->Widget_, isGeometryVisible);
            }
        }
    }

    void MessagesScrollAreaLayout::updateDistanceForViewportItems()
    {
        for (auto &item : LayoutItems_)
        {
            if (item->IsGeometrySet_)
            {
                const auto &widgetAbsGeometry = item->AbsGeometry_;
                const auto viewportAbsRect = evalViewportAbsRect();

                const auto visibilityMargin = Utils::scale_value(0);
                const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
                const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

                onItemDistanseToViewPortChanged(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect);
            }
        }
    }

    void MessagesScrollAreaLayout::suspendVisibleItems()
    {
        for (auto &item : LayoutItems_)
        {
            item->IsVisible_ = false;

            onItemVisibilityChanged(item->Widget_, false);

            item->IsActive_ = false;

            onItemActivityChanged(item->Widget_, false);
        }
    }

    QPoint MessagesScrollAreaLayout::viewport2Absolute(const QPoint viewportPos) const
    {
        auto absolute(viewportPos);

        // apply the absolute transformation
        absolute.ry() += ViewportAbsY_;

        return absolute;
    }

    void MessagesScrollAreaLayout::moveViewportToBottom()
    {
        const auto scrollBounds = getViewportScrollBounds();

        const auto newViewportAbsY = scrollBounds.second;

        __TRACE(
            "geometry",
            "    old-viewport-y=<" << ViewportAbsY_ << ">\n"
            "    new-viewport-y=<" << newViewportAbsY << ">\n"
            "    viewport-height=<" << ViewportSize_.height() << ">"
        );

        ViewportAbsY_ = newViewportAbsY;
    }

    void MessagesScrollAreaLayout::setViewportByOffset(const int32_t bottomOffset)
    {
        assert(bottomOffset >= 0);

        const auto &viewportBounds = getViewportScrollBounds();

        auto newViewportAbsY = viewportBounds.second;
        newViewportAbsY -= bottomOffset;

        setViewportAbsY(newViewportAbsY);
    }

    void MessagesScrollAreaLayout::onItemActivityChanged(QWidget *widget, const bool isActive)
    {
        {
            auto item = qobject_cast<HistoryControlPageItem*>(widget);

            if (item)
            {
                item->onActivityChanged(isActive);
                return;
            }

        }

        auto item = qobject_cast<HistoryControl::PreviewContentWidget*>(widget);

        if (item)
        {
            item->onActivityChanged(isActive);
        }
    }

    void MessagesScrollAreaLayout::onItemVisibilityChanged(QWidget *widget, const bool isVisible)
    {
        {
            auto item = qobject_cast<HistoryControlPageItem*>(widget);

            if (item)
            {
                item->onVisibilityChanged(isVisible);
                return;
            }

        }

        auto item = qobject_cast<HistoryControl::PreviewContentWidget*>(widget);

        if (item)
        {
            item->onActivityChanged(isVisible);
        }
    }

    void MessagesScrollAreaLayout::onItemDistanseToViewPortChanged(QWidget *widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        {
            auto item = qobject_cast<HistoryControlPageItem*>(widget);

            if (item)
            {
                item->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
                return;
            }

        }

        auto item = qobject_cast<HistoryControl::PreviewContentWidget*>(widget);

        if (item)
        {
            item->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
        }
    }

    bool MessagesScrollAreaLayout::setViewportAbsY(const int32_t absY)
    {
        const auto &viewportBounds = getViewportScrollBounds();

        auto newViewportAbsY = absY;

        newViewportAbsY = std::max(newViewportAbsY, viewportBounds.first);
        newViewportAbsY = std::min(newViewportAbsY, viewportBounds.second);

        if (newViewportAbsY == ViewportAbsY_)
        {
            return false;
        }

        ViewportAbsY_ = newViewportAbsY;

        UpdatesLocked_ = true;

        applyItemsGeometry();

        applyTypingWidgetGeometry();

        UpdatesLocked_ = false;

		/// move new offset to HistporyControlPage
		emit updateHistoryPosition(newViewportAbsY, viewportBounds.second);

        return true;
    }

    void MessagesScrollAreaLayout::simulateMouseEvents(
        ItemInfo &itemInfo,
        const QRect &scrollAreaWidgetGeometry,
        const QPoint &globalMousePos,
        const QPoint &scrollAreaMousePos)
    {
        if (scrollAreaWidgetGeometry.isEmpty())
        {
            return;
        }

        if (!ScrollArea_->isVisible())
        {
            return;
        }

        if (!ScrollArea_->isScrolling())
        {
            return;
        }

        auto widget = itemInfo.Widget_;
        assert(widget);

        const auto oldHoveredState = itemInfo.IsHovered_;
        const auto newHoveredState = scrollAreaWidgetGeometry.contains(scrollAreaMousePos);

        itemInfo.IsHovered_ = newHoveredState;

        const auto isMouseLeftWidget = (oldHoveredState && !newHoveredState);
        if (isMouseLeftWidget)
        {
            QEvent leaveEvent(QEvent::Leave);
            QApplication::sendEvent(widget, &leaveEvent);
            return;
        }

        const auto widgetMousePos = widget->mapFromGlobal(globalMousePos);
        QMouseEvent moveEvent(QEvent::MouseMove, widgetMousePos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &moveEvent);
    }

    bool MessagesScrollAreaLayout::slideItemsApart(const ItemsInfoIter &changedItemIter, const int slideY, const SlideOp slideOp)
    {
        assert(slideOp > SlideOp::Min);
        assert(slideOp < SlideOp::Max);
        assert(
            (slideOp == SlideOp::NoSlide) ||
            (slideY != 0));

        __TRACE(
            "geometry",
            "    widget=<" << (*changedItemIter)->Widget_ << ">\n"
            "    slide-y=<" << slideY << ">\n"
            "    slide-op=<" << slideOp << ">"
        );

        const auto nothingToSlide = (
            (slideOp == SlideOp::NoSlide) ||
            (LayoutItems_.size() == 1)
        );
        if (nothingToSlide)
        {
            __TRACE(
                "geometry",
                "    nothing to slide"
            );

            return false;
        }

        if (slideOp == SlideOp::SlideUp)
        {
            (*changedItemIter)->AbsGeometry_.translate(0, -slideY);

            auto iter = (changedItemIter + 1);
            const auto end = LayoutItems_.end();

            if (iter == end)
            {
                __TRACE(
                    "geometry",
                    "    op=<" << slideOp << ">\n"
                    "    nothing to slide except for the changed item"
                );

                // nothing to slide up except for the changed item
                return false;
            }

            for (; iter != end; ++iter)
            {
                const auto &geometry = (*iter)->AbsGeometry_;
                const auto newGeometry = geometry.translated(0, -slideY);

                __TRACE(
                    "geometry",
                    "    op=<" << slideOp << ">\n"
                    "    widget=<" << (*iter)->Widget_ << ">\n"
                    "    old-geometry=<" << geometry << ">\n"
                    "    new-geometry=<" << newGeometry << ">");

                (*iter)->AbsGeometry_ = newGeometry;
            }

            return true;
        }

        assert(slideOp == SlideOp::SlideDown);

        if (changedItemIter == LayoutItems_.begin())
        {
            // nothing to slide down
            return false;
        }

        auto iter = (changedItemIter - 1);
        const auto begin = LayoutItems_.begin();

        for (;; --iter)
        {
            (*iter)->AbsGeometry_.translate(0, slideY);

            if (iter == begin)
            {
                break;
            }
        }

        return true;
    }

    void MessagesScrollAreaLayout::updateItemsWidth()
    {
        const auto viewportAbsMiddleY = evalViewportAbsMiddleY();

        bool atBottom = ScrollArea_->isScrollAtBottom();

        const auto end = LayoutItems_.end();
        for (
            auto iter = LayoutItems_.begin();
            iter != end;
            ++iter
        )
        {
            auto &item = *iter;

            auto widget = item->Widget_;

            applyWidgetWidth(getWidthForItem(), widget, true);

            const auto oldGeometry = item->AbsGeometry_;

            const auto newHeight = evaluateWidgetHeight(widget);

            const QRect newGeometry(
                oldGeometry.left(),
                oldGeometry.top(),
                getWidthForItem(),
                newHeight
            );

            item->AbsGeometry_ = newGeometry;

            const auto deltaY = (newGeometry.height() - oldGeometry.height());
            if (deltaY != 0)
            {
                const auto changeAboveViewportMiddle = (oldGeometry.bottom() < viewportAbsMiddleY);

                const auto slideOp = (
                    changeAboveViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown
                );

                slideItemsApart(iter, deltaY, slideOp);

                if (atBottom)
                    ScrollArea_->scrollToBottom();
            }
        }

        applyItemsGeometry();

        applyTypingWidgetGeometry();

        ScrollArea_->updateScrollbar();
    }

    void MessagesScrollAreaLayout::updateItemsGeometry()
    {
        assert(IsDirty_);
        assert(!UpdatesLocked_);

        if (LayoutItems_.empty())
        {
            return;
        }

        const auto isAtBottom = isViewportAtBottom();

        auto geometryChanged = false;

        UpdatesLocked_ = true;

        for (
                auto iter = LayoutItems_.begin();
                iter != LayoutItems_.end();
                ++iter
            )
        {
            auto &item = **iter;

            const auto itemHeight = evaluateWidgetHeight(item.Widget_);

            const auto &storedGeometry = item.AbsGeometry_;

            const auto storedItemHeight = storedGeometry.height();

            const auto deltaY = (itemHeight - storedItemHeight);
            if (deltaY == 0)
            {
                continue;
            }

            __TRACE(
                "geometry",
                "    widget=<" << item.Widget_ << ">\n"
                "    stored-height=<" << storedItemHeight << ">\n"
                "    new-height=<" << itemHeight << ">\n"
                "    old-geometry=<" << item.AbsGeometry_ << ">\n"
                "    rel-y-inclusive=<" << getRelY(item.AbsGeometry_.top()) << "," << getRelY(item.AbsGeometry_.bottom() + 1) << ">"
            );

            geometryChanged = true;

            const auto changeAboveViewportMiddle = (storedGeometry.bottom() < evalViewportAbsMiddleY());

            const auto slideOp = (
                changeAboveViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown
            );

            item.AbsGeometry_.setHeight(itemHeight);

            slideItemsApart(iter, deltaY, slideOp);

            __TRACE(
                "geometry",
                "    width=<" << item.Widget_ << ">\n"
                "    fixed-geometry=<" << item.AbsGeometry_ << ">"
            );
        }

        if (isAtBottom)
        {
            moveViewportToBottom();
        }

        if (geometryChanged || isAtBottom)
        {
            applyItemsGeometry();
            ScrollArea_->updateScrollbar();
        }

        applyTypingWidgetGeometry();

        UpdatesLocked_ = false;

        //debugValidateGeometry();
    }

    void MessagesScrollAreaLayout::updateItemKey(const Logic::MessageKey &key)
    {
        for (auto &layoutItem : LayoutItems_)
        {
            if (layoutItem->Key_ == key)
            {
                layoutItem->Key_ = key;
                break;
            }
        }
    }

    void MessagesScrollAreaLayout::enumerateMessagesItems(const MessageItemVisitor visitor, const bool reversed)
    {
        assert(visitor);

        const auto widgetVisitor = [this, visitor](QWidget *widget, const bool isVisible)
        {
            if (auto msgItem = qobject_cast<Ui::MessageItem*>(widget))
            {
                return visitor(msgItem, isVisible);
            }

            return true;
        };

        enumerateWidgets(widgetVisitor, reversed);
    }

    void MessagesScrollAreaLayout::enumerateWidgets(const WidgetVisitor visitor, const bool reversed)
    {
        assert(visitor);

        if (LayoutItems_.empty())
        {
            return;
        }

        auto onItemInfo = [this, visitor, reversed](const ItemInfo& itemInfo)->bool
        {
            assert(itemInfo.Widget_);
            if (!itemInfo.Widget_)
            {
                return true;
            }

            const auto &itemGeometry = itemInfo.AbsGeometry_;
            assert(itemGeometry.width() >= 0);
            assert(itemGeometry.height() >= 0);

            const auto isAboveViewport = (itemGeometry.bottom() < ViewportAbsY_);

            const auto viewportBottom = (ViewportAbsY_ + ViewportSize_.height());
            const auto isBelowViewport = (itemGeometry.top() > viewportBottom);

            const auto isHidden = (isAboveViewport || isBelowViewport);

            if (!visitor(itemInfo.Widget_, !isHidden))
                return false;

            auto layout = itemInfo.Widget_->layout();
            if (!layout)
            {
                return true;
            }

            int i = (reversed ? (layout->count() - 1) : 0);
            int i_end = (reversed ? -1 : layout->count());

            while (i != i_end)
            {
                auto item_child = layout->itemAt(i);

                auto widget = item_child->widget();

                if (widget)
                {
                    if (!visitor(widget, !isHidden))
                        return false;
                }

                if (reversed)
                {
                    --i;
                }
                else
                {
                    ++i;
                }
            }

            return true;
        };

        if (!reversed)
        {
            for (auto iter = LayoutItems_.cbegin(); iter != LayoutItems_.cend(); ++iter)
            {
                if (!onItemInfo(**iter))
                {
                    break;
                }
            }

            return;
        }

        for (auto iter = LayoutItems_.crbegin(); iter != LayoutItems_.crend(); ++iter)
        {
            if (!onItemInfo(**iter))
            {
                break;
            }
        }
    }

    int MessagesScrollAreaLayout::getWidthForItem() const
    {
        return std::min(std::max<int>(MessageStyle::getHistoryWidgetMaxWidth(), 0.6 * ViewportSize_.width()), ViewportSize_.width());
    }

    int MessagesScrollAreaLayout::getXForItem() const
    {
        return std::max((ViewportSize_.width() - getWidthForItem()) / 2, 0);
    }

    bool MessagesScrollAreaLayout::isVisibleEnoughForPlay(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const
    {
        auto enough_percent = 0.7;
        auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);
        return 1.0 * intersected.height() / _widgetAbsGeometry.height() > enough_percent;
    }

	void MessagesScrollAreaLayout::updateBounds()
	{
		if (!ViewportSize_.isEmpty())
		{
	        const auto &viewportBounds = getViewportScrollBounds();
			emit updateHistoryPosition(ViewportAbsY_, viewportBounds.second);
		}
	}

	void MessagesScrollAreaLayout::updateScrollbar()
	{
		ScrollArea_->updateScrollbar();
	}

    bool MessagesScrollAreaLayout::eventFilter(QObject* watcher, QEvent* e)
    {
        /// messageScrollArea mainWindow
        if (e->type() == QEvent::KeyPress ||
            e->type() == QEvent::MouseButtonPress || 
            e->type() == QEvent::MouseButtonRelease ||
            e->type() == QEvent::Wheel ||
            e->type() == QEvent::TouchBegin ||
            e->type() == QEvent::TouchUpdate ||
            e->type() == QEvent::TouchEnd || 
            e->type() == QEvent::TouchCancel)
        {
            //ScrollArea_->removeEventFilter(this);
            //Utils::InterConnector::instance().getMainWindow()->removeEventFilter(this);

            ScrollingItems_.clear();
        }
        else if (e->type() == QEvent::Resize)
        {
            if (ScrollingItems_.size() == 1 && ScrollingItems_.back() == watcher)
            {
                emit moveViewportUpByScrollingItem((QWidget*)watcher);
            }
        }
        return false;
    }

    void MessagesScrollAreaLayout::onButtonDownClicked()
    {
        ScrollingItems_.clear();
    }

    void MessagesScrollAreaLayout::onQuote(int64_t quote_id)
    {
        QuoteId_ = quote_id;
    }

    void MessagesScrollAreaLayout::onMoveViewportUpByScrollingItem(QWidget* widget)
    {
        if (!widget)
            return;

        auto r = widget->geometry();

        int new_pos = widget->geometry().top();
        auto type_widget_bottom = TypingWidget_->geometry().bottom();
        auto delta = Utils::scale_value(40);

        /// move new_message to position
        int dpos = new_pos - delta;
        ViewportAbsY_ -= dpos - getTypingWidgetHeight();
        for (auto& val : LayoutItems_)
        {
            val->Widget_->setGeometry(val->Widget_->geometry().translated(0, -dpos));
        }
        TypingWidget_->setGeometry(TypingWidget_->geometry().translated(0, -dpos));

        /// move bottom to edge
        if (TypingWidget_->geometry().bottom() < ViewportSize_.height())
        {
            int dpos = ViewportSize_.height() - TypingWidget_->geometry().bottom();

            for (auto& val : LayoutItems_)
            {
                val->Widget_->setGeometry(val->Widget_->geometry().translated(0, dpos));
            }
            TypingWidget_->setGeometry(TypingWidget_->geometry().translated(0, dpos));

            ViewportAbsY_ = getViewportScrollBounds().second;
        }

        for (auto& val : LayoutItems_)
        {
            val->AbsGeometry_ = val->Widget_->geometry().translated(0, ViewportAbsY_);
        }

        ///  transfer new position to HistporyControlPage (button down)
        emit updateHistoryPosition(ViewportAbsY_, getViewportScrollBounds().second);
        emit recreateAvatarRect();
   }

    void MessagesScrollAreaLayout::moveViewportUpByScrollingItems()
	{
        if (ScrollingItems_.empty() || ScrollingItems_.size() > 1)
            return;

        QWidget* widget = ScrollingItems_.back();
        onMoveViewportUpByScrollingItem(widget);
	}

    void MessagesScrollAreaLayout::onDeactivateScrollingItems(QObject* obj)
    {
        if (obj)
        {
            auto it = std::remove(begin(ScrollingItems_), end(ScrollingItems_), obj);
            ScrollingItems_.erase(it, ScrollingItems_.end());

            moveViewportUpByScrollingItems();
        }
        else
            ScrollingItems_.clear();
    }

    //void MessagesScrollAreaLayout::onDestroyItemAndAlign(QObject* obj)
    //{
    //    QWidget* w = (QWidget*)obj;
    //    auto it = std::find_if(begin(LayoutItems_), end(LayoutItems_), [obj](const ItemInfoUptr& item)->bool
    //    {
    //        return item->Widget_ == obj;
    //    });
    //    if (it != LayoutItems_.end())
    //    {
    //        /// align center
    //        auto rect = w->geometry();
    //        bool bCenter = (rect.bottom() > LayoutRect_.top() && rect.top() < LayoutRect_.bottom());

    //        auto h = rect.height();
    //        for (auto& val : Utils::reverse_iterate(LayoutItems_))
    //        {
    //            if (val->Widget_ == w)
    //            {
    //                break;
    //            }
    //            /// move down full h
    //            val->AbsGeometry_.translate(0, h);
    //            val->Widget_->setGeometry(val->Widget_->geometry().translated(0, h));
    //        }
    //        /// correct ViewportAbsY_
    //        LayoutItems_.erase(it);
    //    }
    //}

    namespace
    {
        void applyWidgetWidth(const int32_t viewportWidth, QWidget *widget, bool traverseLayout)
        {
            assert(widget);
            assert(viewportWidth > 0);

            const auto messageItem = qobject_cast<MessageItem*>(widget);
            if (messageItem)
            {
                // we should provide the message item with geometry prior to calling sizeHint
                messageItem->layout()->setGeometry(
                    QRect(
                        0, 0,
                        viewportWidth, 0
                    )
                );

                return;
            }

            auto chatEventItem = qobject_cast<ChatEventItem*>(widget);
            if (chatEventItem)
            {
                chatEventItem->setFixedWidth(viewportWidth);

                return;
            }

            auto voipEventItem = qobject_cast<VoipEventItem*>(widget);
            if (voipEventItem)
            {
                voipEventItem->setFixedWidth(viewportWidth);

                return;
            }

            auto serviceMessageItem = qobject_cast<ServiceMessageItem*>(widget);
            if (serviceMessageItem)
            {
                serviceMessageItem->setFixedWidth(viewportWidth);

                return;
            }

            auto complexMessageItem = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget);
            if (complexMessageItem)
            {
                complexMessageItem->setFixedWidth(viewportWidth);

                return;
            }

            if (!traverseLayout)
            {
                return;
            }

            auto layout = widget->layout();
            if (!layout)
            {
                return;
            }

            widget->setFixedWidth(viewportWidth);

            auto itemIndex = 0;
            while (auto itemChild = layout->itemAt(itemIndex++))
            {
                auto widget = itemChild->widget();

                if (widget)
                {
                    applyWidgetWidth(viewportWidth, widget, false);
                }
            }
        }

        int32_t evaluateWidgetHeight(QWidget *widget)
        {
            assert(widget);
            //assert(widget->isVisible());

            const auto widgetLayout = widget->layout();
            if (widgetLayout)
            {
                auto height = widgetLayout->sizeHint().height();
                if (height >= 0)
                {
                    return height;
                }
            }

            auto height = widget->sizeHint().height();
            if (height >= 0)
            {
                return height;
            }

            return widget->height();
        }
    }

}
