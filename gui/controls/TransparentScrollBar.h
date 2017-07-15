#pragma once

namespace Ui
{
    class TransparentAnimation : public QObject
    {
        Q_OBJECT
    public:
        TransparentAnimation(float _minOpacity, float _maxOpacity, QWidget* _host);
        virtual ~TransparentAnimation();

    signals:
        void fadeOutStarted();

    public slots:
        void fadeIn();
        void fadeOut();

    private:
        QGraphicsOpacityEffect*      opacityEffect_;
        QPropertyAnimation*          fadeAnimation_;
        float                        minOpacity_;
        float                        maxOpacity_;
        QWidget*                     host_;
        QTimer*                      timer_;
    };

    class TransparentScrollButton : public QWidget
    {
        Q_OBJECT

    signals:
        void fadeOutStarted();

    public slots:

        void fadeIn();
        void fadeOut();

    public:
        TransparentScrollButton(QWidget *parent);
        virtual ~TransparentScrollButton();

        void mouseMoveEvent(QMouseEvent *);
        void mousePressEvent(QMouseEvent *);

        void hoverOn();
        void hoverOff();

        int getMinHeight();
        int getMinWidth();
        int getMaxWidth();

    signals:
        void moved(QPoint);

    private:
        QPropertyAnimation*          maxSizeAnimation_;
        QPropertyAnimation*          minSizeAnimation_;
        int                          minScrollButtonWidth_;
        int                          maxScrollButtonWidth_;
        int                          minScrollButtonHeight_;
        TransparentAnimation*        transparentAnimation_;
        bool                         isHovered_;

        void paintEvent(QPaintEvent *event);
    };

    class AbstractWidgetWithScrollBar;

    class TransparentScrollBar : public QWidget
    {
        Q_OBJECT

    public:
        TransparentScrollBar();
        virtual ~TransparentScrollBar();

        void fadeIn();
        void fadeOut();
        void setScrollArea(QAbstractScrollArea* _view);
        void setParentWidget(QWidget* _view);
        void setDefaultScrollBar(QScrollBar* _scrollBar);
        void setGetContentHeightFunc(std::function<int()> _getContentHeight);
        
        void init();

    public slots:
        void updatePos();

    private slots:
        void onScrollBtnMoved(QPoint);

    private:
        bool eventFilter(QObject *obj, QEvent *event);
        void onResize(QResizeEvent *e);
        void paintEvent(QPaintEvent *event);
        void resizeEvent(QResizeEvent *event);
        void mousePressEvent(QMouseEvent *event);

        void moveToGlobalPos(QPoint moveTo);
        double calcButtonHeight();
        double calcScrollBarRatio();

        QScrollBar* getDefaultScrollBar() const;

        QPointer<QWidget>                     view_;
        QPointer<TransparentScrollButton>     scrollButton_;
        TransparentAnimation*                 transparentAnimation_;
        QScrollBar*                           scrollBar_;
        std::function<int()>                  getContentHeight_;
    };

    class AbstractWidgetWithScrollBar
    {
    public:
        AbstractWidgetWithScrollBar();
        virtual ~AbstractWidgetWithScrollBar() {};

        virtual QSize contentSize() const = 0;
        virtual TransparentScrollBar* getScrollBar() const;
        virtual void setScrollBar(TransparentScrollBar* _scrollBar);

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event);
        virtual void wheelEvent(QWheelEvent *event);
        virtual void updateGeometries();

    private:
        TransparentScrollBar*    scrollBar_;
    };

    class FocusableListView: public QListView
    {
    public:
        FocusableListView(QWidget *_parent = 0);
        ~FocusableListView();

    protected:
        virtual void enterEvent(QEvent *_e) override;
        virtual void leaveEvent(QEvent *_e) override;
        virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index,
                                                                 const QEvent *event = 0) const override;
    };

    class ListViewWithTrScrollBar : public FocusableListView, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    public:
        explicit ListViewWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~ListViewWithTrScrollBar();

        virtual QSize contentSize() const override;
        virtual void setScrollBar(TransparentScrollBar* _scrollBar) override;

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event) override;
        virtual void wheelEvent(QWheelEvent *event) override;
        virtual void updateGeometries() override;
    };

    class MouseMoveEventFilterForTrScrollBar : public QObject
    {
        Q_OBJECT

    public:
        MouseMoveEventFilterForTrScrollBar(TransparentScrollBar* _scrollbar);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);

    private:
        TransparentScrollBar* scrollbar_;
    };

    class ScrollAreaWithTrScrollBar : public QScrollArea, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized();

    public:
        explicit ScrollAreaWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~ScrollAreaWithTrScrollBar();

        virtual QSize contentSize() const override;
        virtual void setScrollBar(TransparentScrollBar* _scrollBar) override;
        void setWidget(QWidget* widget);

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event) override;
        virtual void wheelEvent(QWheelEvent *event) override;
        virtual void resizeEvent(QResizeEvent *event) override;
        void updateGeometry();
    private:
        bool eventFilter(QObject *obj, QEvent *event) override;
    };

    class TextEditExWithTrScrollBar : public QTextBrowser, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    public:
        explicit TextEditExWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~TextEditExWithTrScrollBar();

        virtual QSize contentSize() const override;
        virtual void setScrollBar(TransparentScrollBar* _scrollBar) override;

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event) override;
        virtual void wheelEvent(QWheelEvent *event) override;
        void updateGeometry();
    };

    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBar(QWidget* parent);
    ListViewWithTrScrollBar* CreateFocusableViewAndSetTrScrollBar(QWidget* parent);
    QTextBrowser* CreateTextEditExWithTrScrollBar(QWidget* parent);
}
