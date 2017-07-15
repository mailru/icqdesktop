#include "stdafx.h"
#include "FlatMenu.h"
#include "TransparentScrollBar.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace Ui
{
    namespace L
    {
        const auto backgroundColor = QColor(0xd9, 0xdd, 0xdd, (int32_t)(0.5 * 255));
        const auto buttonColor = QColor(0x76, 0x76, 0x76, (int32_t)(0.5 * 255));
        const auto hoverButtonColor = QColor("#767676");
    
        const auto disappearTime = 2000; // ms
        const auto wideTime = 250; // ms
        const auto thinTime = 250; // ms
        const auto fadeInTime = 250; // ms
        const auto fadeOutTime = 250; // ms

        const auto minOpacity = 0;
        const auto maxOpacity = 1;

        const auto minScrollButtonWidth_dip = 4;
        const auto maxScrollButtonWidth_dip = 4;
        const auto minScrollButtonHeight_dip = 50;

        const auto backgroundWidth_dip = 4;
        const auto backgroundRightMargin_dip = 4;
        const auto backgroundUpMargin_dip = 4;
        const auto backgroundDownMargin_dip = 4;

        const auto is_show_with_small_content = false;
    }

    TransparentAnimation::TransparentAnimation(float _minOpacity, float _maxOpacity, QWidget* _host)
        : QObject(_host)
        , minOpacity_(_minOpacity)
        , maxOpacity_(_maxOpacity)
        , host_(_host)
    {
        opacityEffect_ = new QGraphicsOpacityEffect(host_);
        opacityEffect_->setOpacity(minOpacity_);
        host_->setGraphicsEffect(opacityEffect_);

        fadeAnimation_ = new QPropertyAnimation(opacityEffect_, "opacity");

        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, &TransparentAnimation::fadeOut);
        timer_->start(L::disappearTime);
    }

    TransparentAnimation::~TransparentAnimation()
    {
        delete opacityEffect_;
    }

    void TransparentAnimation::fadeIn()
    {
        timer_->start(L::disappearTime);
        fadeAnimation_->stop();
        fadeAnimation_->setDuration(L::fadeInTime);
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(maxOpacity_);
        fadeAnimation_->start();
    }

    void TransparentAnimation::fadeOut()
    {
        emit fadeOutStarted();
        fadeAnimation_->stop();
        fadeAnimation_->setDuration(L::fadeOutTime);
        fadeAnimation_->setStartValue(opacityEffect_->opacity());
        fadeAnimation_->setEndValue(minOpacity_);
        fadeAnimation_->start();
    }

    // TransparentScrollButton

    TransparentScrollButton::TransparentScrollButton(QWidget *parent) 
        : QWidget(parent)
        , minScrollButtonWidth_(Utils::scale_value(L::minScrollButtonWidth_dip))
        , maxScrollButtonWidth_(Utils::scale_value(L::maxScrollButtonWidth_dip))
        , minScrollButtonHeight_(Utils::scale_value(L::minScrollButtonHeight_dip))
        , transparentAnimation_(new TransparentAnimation(L::minOpacity, L::maxOpacity, this))
        , isHovered_(false)
    {
        maxSizeAnimation_ = new QPropertyAnimation(this, "maximumWidth");
        minSizeAnimation_ = new QPropertyAnimation(this, "minimumWidth");

        setMaximumHeight(minScrollButtonHeight_);
        setMinimumHeight(minScrollButtonHeight_);
        setMaximumWidth(minScrollButtonWidth_);
        setMinimumWidth(minScrollButtonWidth_);

        connect(transparentAnimation_, &TransparentAnimation::fadeOutStarted, this, &TransparentScrollButton::fadeOutStarted);
    }

    TransparentScrollButton::~TransparentScrollButton()
    {
        delete maxSizeAnimation_;
    }

    void TransparentScrollButton::mousePressEvent(QMouseEvent* /* event */)
    {
    }

    int TransparentScrollButton::getMinHeight()
    {
        return minScrollButtonHeight_;
    }

    int TransparentScrollButton::getMinWidth()
    {
        return minScrollButtonWidth_;
    }

    int TransparentScrollButton::getMaxWidth()
    {
        return maxScrollButtonWidth_;
    }

    void TransparentScrollButton::mouseMoveEvent(QMouseEvent *event)
    {
        emit moved(event->globalPos());
    }

    void TransparentScrollButton::hoverOn()
    {
        isHovered_ = true;
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(L::wideTime);
        maxSizeAnimation_->setStartValue(maximumWidth());
        maxSizeAnimation_->setEndValue(maxScrollButtonWidth_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(L::wideTime);
        minSizeAnimation_->setStartValue(maximumWidth());
        minSizeAnimation_->setEndValue(maxScrollButtonWidth_);
        minSizeAnimation_->start();
        update();
    }

    void TransparentScrollButton::hoverOff()
    {
        isHovered_ = false;
        maxSizeAnimation_->stop();
        maxSizeAnimation_->setDuration(L::thinTime);
        maxSizeAnimation_->setStartValue(maximumWidth());
        maxSizeAnimation_->setEndValue(minScrollButtonWidth_);
        maxSizeAnimation_->start();

        minSizeAnimation_->stop();
        minSizeAnimation_->setDuration(L::thinTime);
        minSizeAnimation_->setStartValue(maximumWidth());
        minSizeAnimation_->setEndValue(minScrollButtonWidth_);
        minSizeAnimation_->start();
        update();
    }

    void TransparentScrollButton::fadeIn()
    {
        if (isEnabled())
        {
            transparentAnimation_->fadeIn();
        }
    }

    void TransparentScrollButton::fadeOut()
    {
        transparentAnimation_->fadeOut();
    }

    void TransparentScrollButton::paintEvent(QPaintEvent* /* event */)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, rect().width(), rect().height()), 0, 0);
        p.fillPath(path, isHovered_ ? L::hoverButtonColor : L::buttonColor);
    }

    // TransparentScrollBar

    TransparentScrollBar::TransparentScrollBar()
        : QWidget(nullptr, Qt::FramelessWindowHint)
        , view_(nullptr)
        , transparentAnimation_(new TransparentAnimation(L::minOpacity, L::maxOpacity, this))
    {
        setAttribute(Qt::WA_TranslucentBackground);
        //    setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void TransparentScrollBar::setGetContentHeightFunc(std::function<int()> _getContentHeight)
    {
        getContentHeight_ = _getContentHeight;
    }

    QScrollBar* TransparentScrollBar::getDefaultScrollBar() const
    {
        return scrollBar_;
    }

    void TransparentScrollBar::setDefaultScrollBar(QScrollBar* _scrollBar)
    {
        scrollBar_ = _scrollBar;
    }

    void TransparentScrollBar::setScrollArea(QAbstractScrollArea* _view)
    {
        view_ = _view;
        setDefaultScrollBar(_view->verticalScrollBar());

        setParent(view_);
        init();
    }

    void TransparentScrollBar::setParentWidget(QWidget* _view)
    {
        view_ = _view;

        setParent(view_);
        init();
    }

    void TransparentScrollBar::init()
    {
        scrollButton_ = new TransparentScrollButton(view_);
        resize(Utils::scale_value(L::backgroundWidth_dip), 0);

        scrollButton_->installEventFilter(this);
        view_->installEventFilter(this);
        installEventFilter(this);

        connect(getDefaultScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updatePos()));
        connect(scrollButton_, SIGNAL(moved(QPoint)), this, SLOT(onScrollBtnMoved(QPoint)));

        connect(scrollButton_, &TransparentScrollButton::fadeOutStarted, transparentAnimation_, &TransparentAnimation::fadeOut);
    }

    TransparentScrollBar::~TransparentScrollBar()
    {
        removeEventFilter(scrollButton_);
        removeEventFilter(view_);
        removeEventFilter(this);
    }

    void TransparentScrollBar::fadeIn()
    {
        if (scrollButton_ && (L::is_show_with_small_content || calcScrollBarRatio() < 1))
        {
            scrollButton_->fadeIn();
            transparentAnimation_->fadeIn();
        }
    }

    void TransparentScrollBar::fadeOut()
    {
        if (scrollButton_ && (L::is_show_with_small_content || calcScrollBarRatio() < 1))
        {
            scrollButton_->fadeOut();
            transparentAnimation_->fadeOut();
        }
    }

    void TransparentScrollBar::mousePressEvent(QMouseEvent *event)
    {
        moveToGlobalPos(event->globalPos());
    }

    void TransparentScrollBar::moveToGlobalPos(QPoint moveTo)
    {
        auto scrollBar = getDefaultScrollBar();
        float max = scrollBar->maximum();
        QPoint viewGlobalPos = view_->parentWidget()->mapToGlobal(view_->pos());

        float minPx = viewGlobalPos.y();
        float maxPx = viewGlobalPos.y() + view_->height();

        // TODO (*) : use init relation position on scrollBar
        float ratio = (moveTo.y() - minPx) / (maxPx - minPx - scrollButton_->height());
        float value = ratio * max;

        scrollBar->setValue(value);
    }

    bool TransparentScrollBar::eventFilter(QObject *obj, QEvent *event)
    {
        switch (event->type())
        {
        case QEvent::Enter:
            setMouseTracking(true);
            if (obj == view_)
            {
                fadeIn();
            }
            else if (obj == this)
            {
                scrollButton_->hoverOn();
            }
            else if (obj == scrollButton_) 
            {
                scrollButton_->hoverOn();
            }
            break;

        case QEvent::Leave:
            if (obj == view_) 
            {
                scrollButton_->fadeOut();
            }
            else if (obj == this) 
            {
                scrollButton_->hoverOff();
            }
            else if (obj == scrollButton_) 
            {
                scrollButton_->hoverOff();
            }
            break;

        case QEvent::Resize:
            if (obj == view_)
            {
                onResize(static_cast<QResizeEvent*>(event));
            }
            break;
        case QEvent::MouseMove:
            fadeIn();
            break;
        default:
            ;
        }
        return QWidget::eventFilter( obj, event );
    }

    void TransparentScrollBar::onResize(QResizeEvent *e)
    {
        const auto x = e->size().width() - width() - Utils::scale_value(L::backgroundRightMargin_dip);
        const auto y = Utils::scale_value(L::backgroundUpMargin_dip);
        const auto w = width();
        const auto h = e->size().height() - Utils::scale_value(L::backgroundDownMargin_dip) - Utils::scale_value(L::backgroundUpMargin_dip);

        move(x, y);
        resize(w, h);

        updatePos();
    }

    void TransparentScrollBar::onScrollBtnMoved(QPoint point) 
    {
        moveToGlobalPos(point);
    }

    double TransparentScrollBar::calcScrollBarRatio()
    {
        const auto viewportHeight = view_->height();
        const auto contentHeight = getContentHeight_();

        if (contentHeight < 0)
            return -1;

        return 1.0 * viewportHeight / contentHeight;
    }

    double TransparentScrollBar::calcButtonHeight()
    {
        return std::max((int)(calcScrollBarRatio() * view_->height()), scrollButton_->getMinHeight());
    }

    void TransparentScrollBar::updatePos()
    {
        const auto ratio = calcScrollBarRatio();

        if (ratio < 0)
        {
            scrollButton_->hide();
            hide();
            return;
        }

        if (!L::is_show_with_small_content && ratio >= 1)
        {
            scrollButton_->hide();
            hide();
            return;
        }
        else
        {
            scrollButton_->show();
            show();
            if (!scrollButton_->isVisible())
            {
                fadeIn();
            }
        }
        
        scrollButton_->setFixedSize(scrollButton_->width(), calcButtonHeight());
        const auto scrollBar = getDefaultScrollBar();
        const auto val = scrollBar->value();
        const auto max = scrollBar->maximum();
        const auto x = pos().x() + (width() - scrollButton_->getMaxWidth()) / 2;

        if (max == 0)
        {
            scrollButton_->move(x, pos().y());
            return;
        }

        const auto maxY = height() - scrollButton_->height();
        const auto y = (maxY * val) / max + pos().y();

        scrollButton_->move(x, y);
    }

    void TransparentScrollBar::paintEvent(QPaintEvent * /* event */)
    {
        QPainter paiter(this);
        QRect rect(0, 0, this->rect().width() - 1, this->rect().height() - 1);
        paiter.fillRect(rect, L::backgroundColor);
    }

    void TransparentScrollBar::resizeEvent(QResizeEvent * /* event */)
    {
        updatePos();
    }

    // Abs widget with transp scrollbar

    AbstractWidgetWithScrollBar::AbstractWidgetWithScrollBar()
        : scrollBar_(nullptr)
    {
    
    }

    TransparentScrollBar* AbstractWidgetWithScrollBar::getScrollBar() const
    {
        return scrollBar_;
    }

    void AbstractWidgetWithScrollBar::setScrollBar(TransparentScrollBar* _scrollBar)
    {
        scrollBar_ = _scrollBar;
    }

    void AbstractWidgetWithScrollBar::mouseMoveEvent(QMouseEvent* /* event */)
    {
        if (getScrollBar())
        {
            getScrollBar()->fadeIn();
        }
    }

    void AbstractWidgetWithScrollBar::wheelEvent(QWheelEvent* /* event */)
    {
        if (getScrollBar())
        {
            getScrollBar()->fadeIn();
        }
    }

    void AbstractWidgetWithScrollBar::updateGeometries()
    {
        if (getScrollBar())
        {
            getScrollBar()->updatePos();
        }
    }

    // ListViewWithTrScrollBar

    ListViewWithTrScrollBar::ListViewWithTrScrollBar(QWidget *parent)
        : FocusableListView(parent)
    {
        setMouseTracking(true);
    }

    ListViewWithTrScrollBar::~ListViewWithTrScrollBar()
    {
    }

    QSize ListViewWithTrScrollBar::contentSize() const
    {
        return FocusableListView::contentsSize();
    }

    void ListViewWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        FocusableListView::mouseMoveEvent(event);
    }

    void ListViewWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        FocusableListView::wheelEvent(event);
    }

    void ListViewWithTrScrollBar::updateGeometries()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        FocusableListView::updateGeometries();
    }

    void ListViewWithTrScrollBar::setScrollBar(TransparentScrollBar* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBar(_scrollBar);
        getScrollBar()->setScrollArea(this);
    }

    // MouseMoveEventFilter
    MouseMoveEventFilterForTrScrollBar::MouseMoveEventFilterForTrScrollBar(TransparentScrollBar* _scrollbar)
        : scrollbar_(_scrollbar)
    {
    }

    bool MouseMoveEventFilterForTrScrollBar::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::MouseMove)
        {
            if (scrollbar_)
                scrollbar_->updatePos();
        }
        return QObject::eventFilter(_obj, _event);
    }

    // ScrollAreaWithTrScrollBar

    ScrollAreaWithTrScrollBar::ScrollAreaWithTrScrollBar(QWidget *parent)
        : QScrollArea(parent)
    {
        setMouseTracking(true);
        installEventFilter(this);
    }

    ScrollAreaWithTrScrollBar::~ScrollAreaWithTrScrollBar()
    {
    }

    QSize ScrollAreaWithTrScrollBar::contentSize() const
    {
        return QScrollArea::widget()->size();
    }

    void ScrollAreaWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        QScrollArea::mouseMoveEvent(event);
    }

    void ScrollAreaWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        QScrollArea::wheelEvent(event);
    }

    void ScrollAreaWithTrScrollBar::resizeEvent(QResizeEvent *event)
    {
        QScrollArea::resizeEvent(event);
        emit resized();
    }

    void ScrollAreaWithTrScrollBar::updateGeometry()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        QScrollArea::updateGeometry();
    }

    void ScrollAreaWithTrScrollBar::setScrollBar(TransparentScrollBar* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBar(_scrollBar);
        getScrollBar()->setScrollArea(this);
    }

    bool ScrollAreaWithTrScrollBar::eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::Resize)
        {
            if (getScrollBar())
                getScrollBar()->updatePos();
        }
        return QScrollArea::eventFilter( obj, event );
    }

    void ScrollAreaWithTrScrollBar::setWidget(QWidget* widget)
    {
        auto filter = new MouseMoveEventFilterForTrScrollBar(getScrollBar());
        widget->setMouseTracking(true);
        widget->installEventFilter(filter);

        QScrollArea::setWidget(widget);
    }

    // TextEditExWithScrollBar

    TextEditExWithTrScrollBar::TextEditExWithTrScrollBar(QWidget *parent)
        : QTextBrowser(parent)
    {
        setMouseTracking(true);
    }

    TextEditExWithTrScrollBar::~TextEditExWithTrScrollBar()
    {
    }

    QSize TextEditExWithTrScrollBar::contentSize() const
    {
        QSize sizeRect(document()->size().width(), document()->size().height());
        return sizeRect;
    }

    void TextEditExWithTrScrollBar::mouseMoveEvent(QMouseEvent* event)
    {
        AbstractWidgetWithScrollBar::mouseMoveEvent(event);
        QTextBrowser::mouseMoveEvent(event);
    }

    void TextEditExWithTrScrollBar::wheelEvent(QWheelEvent *event)
    {
        AbstractWidgetWithScrollBar::wheelEvent(event);
        QTextBrowser::wheelEvent(event);
    }

    void TextEditExWithTrScrollBar::updateGeometry()
    {
        AbstractWidgetWithScrollBar::updateGeometries();
        QTextBrowser::updateGeometry();
    }

    void TextEditExWithTrScrollBar::setScrollBar(TransparentScrollBar* _scrollBar)
    {
        AbstractWidgetWithScrollBar::setScrollBar(_scrollBar);
        getScrollBar()->setScrollArea(this);
    }

    // HELPERS

    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBar(QWidget* parent)
    {
        auto result = new ScrollAreaWithTrScrollBar(parent);
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto scrollBar = new TransparentScrollBar();
        result->setScrollBar(scrollBar);
        scrollBar->setGetContentHeightFunc([result](){ return result->contentSize().height(); });

        return result;
    }

    ListViewWithTrScrollBar* CreateFocusableViewAndSetTrScrollBar(QWidget* parent)
    {
        auto result = new ListViewWithTrScrollBar(parent);
        auto scrollBar = new TransparentScrollBar();
        result->setScrollBar(scrollBar);
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        result->setFocusPolicy(Qt::NoFocus);
        scrollBar ->setGetContentHeightFunc([result](){ return result->contentSize().height(); });

        return result;
    }

    QTextBrowser* CreateTextEditExWithTrScrollBar(QWidget* parent)
    {
        auto result = new TextEditExWithTrScrollBar(parent);
        
        auto scrollBar = new TransparentScrollBar();
        result->setScrollBar(scrollBar );
        result->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollBar->setGetContentHeightFunc([result](){ return result->contentSize().height(); });

        return result;
    }

    // ListViewWithTrScrollBar

    FocusableListView::FocusableListView(QWidget *_parent/* = 0*/)
        : QListView(_parent)
    {
    }
    
    FocusableListView::~FocusableListView()
    {
    }

    void FocusableListView::enterEvent(QEvent *_e)
    {
        QListView::enterEvent(_e);
        if (platform::is_apple() && !FlatMenu::shown())
            emit Utils::InterConnector::instance().forceRefreshList(model(), true);
    }

    void FocusableListView::leaveEvent(QEvent *_e)
    {
        QListView::leaveEvent(_e);
        if (platform::is_apple() && !FlatMenu::shown())
            emit Utils::InterConnector::instance().forceRefreshList(model(), false);
    }

    QItemSelectionModel::SelectionFlags FocusableListView::selectionCommand(const QModelIndex & index, const QEvent * event) const
    {
        QItemSelectionModel::SelectionFlags flags = QAbstractItemView::selectionCommand(index, event);

        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove)
        {
            flags = QItemSelectionModel::NoUpdate;
        }

        return flags;
    }
}
