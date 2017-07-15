#pragma once

#include "MessagesModel.h"

namespace Ui
{

    class MessagesScrollbar;
    class MessagesScrollArea;

    class MessagesScrollAreaLayout : public QLayout
    {
		Q_OBJECT;
        enum class SlideOp;

        friend QTextStream& operator <<(QTextStream &lhs, const SlideOp slideOp);

    public:
        typedef std::function<bool(Ui::MessageItem*, const bool)> MessageItemVisitor;

        typedef std::function<bool(QWidget*, const bool)> WidgetVisitor;

        typedef std::pair<Logic::MessageKey, QWidget*> PositionWidget;

        typedef std::list<PositionWidget> WidgetsList;

        // the left value is inclusive, the right value is exclusive
        typedef std::pair<int32_t, int32_t> Interval;

        MessagesScrollAreaLayout(
            MessagesScrollArea *scrollArea,
            MessagesScrollbar *messagesScrollbar,
            QWidget *typingWidget
        );

        virtual ~MessagesScrollAreaLayout();

        virtual void setGeometry(const QRect &r) override;

        virtual void addItem(QLayoutItem *item) override;

        virtual QLayoutItem *itemAt(int index) const override;

        virtual QLayoutItem *takeAt(int index) override;

        virtual int count() const override;

        virtual QSize sizeHint() const override;

        virtual void invalidate() override;

        QPoint absolute2Viewport(const QPoint absPos) const;

        bool containsWidget(QWidget *widget) const;

        QWidget* getItemByPos(const int32_t pos) const;

        QWidget* getItemByKey(const Logic::MessageKey &key) const;

        int32_t getItemsCount() const;

        int32_t getItemsHeight() const;

        Logic::MessageKeyVector getItemsKeys() const;

        int32_t getViewportAbsY() const;

        Interval getViewportScrollBounds() const;

        int32_t getViewportHeight() const;

        QList<Logic::MessageKey> getWidgetsOverBottomOffset(const int32_t offset) const;

        void insertWidgets(const WidgetsList& _widgets, bool _isMoveToButtonIfNeed, int64_t _mess_id);

        void removeWidget(QWidget *widget);

        void setViewportByOffset(const int32_t bottomOffset);

        int32_t shiftViewportAbsY(const int32_t delta);

        void updateItemKey(const Logic::MessageKey &key);

        void enumerateMessagesItems(const MessageItemVisitor visitor, const bool reversed);

        void enumerateWidgets(const WidgetVisitor visitor, const bool reversed);

        bool isViewportAtBottom() const;

        void resumeVisibleItems();

        void updateDistanceForViewportItems();

        void suspendVisibleItems();

        void updateItemsWidth();

        QPoint viewport2Absolute(const QPoint viewportPos) const;

		void updateBounds();
		void updateScrollbar();

        virtual bool eventFilter(QObject* watcher, QEvent* e) override;

    private:
        class ItemInfo
        {
        public:
            ItemInfo(QWidget *widget, const Logic::MessageKey &key);
            
            QWidget *Widget_;
            
            QRect AbsGeometry_;
            
            Logic::MessageKey Key_;
            
            bool IsGeometrySet_;
            
            bool IsHovered_;
            
            bool IsActive_;
            
            bool IsVisible_;
        };

        typedef std::unique_ptr<ItemInfo> ItemInfoUptr;

        typedef std::deque<ItemInfoUptr> ItemsInfo;

        typedef ItemsInfo::iterator ItemsInfoIter;

        std::set<QWidget*> Widgets_;

        ItemsInfo LayoutItems_;

        MessagesScrollbar *Scrollbar_;

        MessagesScrollArea *ScrollArea_;

        QWidget *TypingWidget_;

        QRect LayoutRect_;

        QSize ViewportSize_;

        int32_t ViewportAbsY_;

        bool IsDirty_;

        bool UpdatesLocked_;

        QRect absolute2Viewport(const QRect &absolute) const;

        void applyItemsGeometry();

        void applyTypingWidgetGeometry();

        QRect calculateInsertionRect(const ItemsInfoIter &itemInfoIter, Out SlideOp &slideOp);

        void debugValidateGeometry();

        void dumpGeometry(const QString &notes);

        int32_t evalViewportAbsMiddleY() const;

        QRect evalViewportAbsRect() const;

        Interval getItemsAbsBounds() const;

        int32_t getRelY(const int32_t y) const;

        int32_t getTypingWidgetHeight() const;

        ItemsInfoIter insertItem(QWidget *widget, const Logic::MessageKey &key);

        void onItemActivityChanged(QWidget *widget, const bool isActive);

        void onItemVisibilityChanged(QWidget *widget, const bool isVisible);

        void onItemDistanseToViewPortChanged(QWidget *widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

        bool setViewportAbsY(const int32_t absY);

        void simulateMouseEvents(ItemInfo &itemInfo, const QRect &scrollAreaWidgetGeometry, const QPoint &globalMousePos, const QPoint &scrollAreaMousePos);

        bool slideItemsApart(const ItemsInfoIter &changedItemIter, const int slideY, const SlideOp slideOp);

        void updateItemsGeometry();

        void moveViewportToBottom();

        int getWidthForItem() const;

        int getXForItem() const;

        bool isVisibleEnoughForPlay(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const;

        std::list<QWidget*> ScrollingItems_;

        /// observe to scrolling item position
        void moveViewportUpByScrollingItems();

        int64_t QuoteId_;    /// for loaded messages

private slots:
        void onDeactivateScrollingItems(QObject* obj);
        /// void onDestroyItemAndAlign(QObject* obj);

        void onButtonDownClicked();

        void onQuote(int64_t quote_id);
        void onMoveViewportUpByScrollingItem(QWidget* widget);

Q_SIGNALS:
		void updateHistoryPosition(int32_t position, int32_t offset);
        void recreateAvatarRect();
        void moveViewportUpByScrollingItem(QWidget*);
    };
}
