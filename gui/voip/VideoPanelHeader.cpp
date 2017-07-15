#include "stdafx.h"
#include "VideoPanelHeader.h"

#include "PushButton_t.h"
#include "VoipTools.h"
#include "../controls/CommonStyle.h"
#include "../utils/utils.h"

#define DEFAULT_BORDER Utils::scale_value(12)
#define DEFAULT_WINDOW_ROUND_RECT Utils::scale_value(5)

#define COLOR_SECURE_BTN_ACTIVE   QColor("#ffffff")
#define COLOR_SECURE_BTN_INACTIVE Qt::transparent

const QString secureCallButton = 
    " QPushButton { font-size: 15dip; text-align: center; border-style: none; background-color: transparent; } ";

const QString secureCallButtonClicked = 
    " QPushButton { font-size: 15dip; text-align: center; border-style: none; background-color: #ffffff; } ";

#define SECURE_BTN_BORDER_W    Utils::scale_value(24)
#define SECURE_BTN_ICON_W      Utils::scale_value(16)
#define SECURE_BTN_ICON_H      SECURE_BTN_ICON_W
#define SECURE_BTN_TEXT_W      Utils::scale_value(50)
#define SECURE_BTN_ICON2TEXT_W Utils::scale_value(12)
#define SECURE_BTN_W           (2 * SECURE_BTN_BORDER_W + SECURE_BTN_ICON_W + SECURE_BTN_TEXT_W + SECURE_BTN_ICON2TEXT_W)


std::string Ui::getFotmatedTime(unsigned _ts)
{
    int hours = _ts / (60 * 60);
    int minutes = (_ts / 60) % 60;
    int sec = _ts % 60;

    std::stringstream timeString;
    if (hours > 0)
    {
        timeString << std::setfill('0') << std::setw(2) << hours << ":";
    }
    timeString << std::setfill('0') << std::setw(2) << minutes << ":";
    timeString << std::setfill('0') << std::setw(2) << sec;

    return timeString.str();
}

Ui::MoveablePanel::MoveablePanel(QWidget* _parent)
    : BaseTopVideoPanel(_parent)
    , parent_(_parent)
{
    dragState_.isDraging = false;
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

Ui::MoveablePanel::~MoveablePanel()
{
}

void Ui::MoveablePanel::mouseReleaseEvent(QMouseEvent* /*e*/)
{
    // We check visible flag for drag&drop, because mouse events
    // were be called when window is invisible by VideoWindow.
    if (isVisible())
    {
        grabMouse = false;
        dragState_.isDraging = false;
    }
}

void Ui::MoveablePanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
#ifdef __APPLE__
    const auto rc = parent_->rect();
    if (parent_ && !parent_->isFullScreen())
    {
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), DEFAULT_WINDOW_ROUND_RECT, DEFAULT_WINDOW_ROUND_RECT);
        setMask(path.toFillPolygon().toPolygon());
    }
    else
    {
        clearMask();
    }
#endif
}

void Ui::MoveablePanel::mousePressEvent(QMouseEvent* _e)
{
    if (isVisible() && parent_ && !parent_->isFullScreen() && (_e->buttons() & Qt::LeftButton))
    {
        grabMouse = true;
        dragState_.isDraging = true;
        dragState_.posDragBegin = QCursor::pos();
    }
}

void Ui::MoveablePanel::mouseMoveEvent(QMouseEvent* _e)
{
    if (isVisible() && (_e->buttons() & Qt::LeftButton) && dragState_.isDraging)
    {
        QPoint diff = QCursor::pos() - dragState_.posDragBegin;
        dragState_.posDragBegin = QCursor::pos();
        QPoint newpos = parent_->pos() + diff;

        parent_->move(newpos);
    }
}

void Ui::MoveablePanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);

    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || uiWidgetIsActive())
        {
            if (parent_)
            {
                parent_->raise();
                raise();
            }
        }
    }
}

void Ui::MoveablePanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

bool Ui::VideoPanelHeader::uiWidgetIsActive() const
{
    return lowWidget_->isActiveWindow();
}

void Ui::VideoPanelHeader::_onMinimize()
{
    emit onMinimize();
}

void Ui::VideoPanelHeader::_onMaximize()
{
    emit onMaximize();
}

void Ui::VideoPanelHeader::_onClose()
{
    emit onClose();
}

Ui::VideoPanelHeader::VideoPanelHeader(QWidget* _parent, int _items)
    : MoveablePanel(_parent)
    , callName_(NULL)
    , lowWidget_(NULL)
    , callTime_(NULL)
    , btnMin_(NULL)
    , secureCallEnabled_(false)
    , btnClose_(NULL)
    , itemsToShow_(_items)
{
    setStyleSheet(Utils::LoadStyle(":/voip/video_panel.qss"));
    setProperty("VideoPanelHeader", true);
    setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* layout = Utils::emptyHLayout();
    lowWidget_ = new QWidget(this);
    { // low widget. it makes background panel coloured
        lowWidget_->setContentsMargins(DEFAULT_BORDER, 0, 0, 0);
        lowWidget_->setProperty("VideoPanelHeader", true);
        lowWidget_->setLayout(layout);

        QVBoxLayout* vLayoutParent = Utils::emptyVLayout();
        vLayoutParent->setAlignment(Qt::AlignVCenter);
        vLayoutParent->addWidget(lowWidget_);
        setLayout(vLayoutParent);

        layout->setAlignment(Qt::AlignVCenter);

        //layout->addSpacing(DEFAULT_BORDER);
    }

    auto addWidget = [] (QWidget* _parent)
    {
        QWidget* w = new QWidget(_parent);
        w->setContentsMargins(0, 0, 0, 0);
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (_parent)
        {
            _parent->layout()->addWidget(w);
        }
        return w;
    };

    auto addLayout = [] (QWidget* _w, Qt::Alignment _align)
    {
        assert(_w);
        if (_w)
        {
            QHBoxLayout* layout = Utils::emptyHLayout();
            layout->setAlignment(_align);
            _w->setLayout(layout);
        }
    };

    QWidget* leftWidg    = addWidget(lowWidget_);
    QWidget* centerWidg  = addWidget(lowWidget_);
    QWidget* rightWidg   = addWidget(lowWidget_);

    addLayout(leftWidg,   Qt::AlignLeft | Qt::AlignVCenter);
    addLayout(centerWidg, Qt::AlignCenter);
    addLayout(rightWidg,  Qt::AlignRight | Qt::AlignVCenter);

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    if (itemsToShow_ & kVPH_ShowName)
    {
        callName_ = new NameWidget(leftWidg, Utils::scale_value(15));
        callName_->setFont(font);
        callName_->layout()->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        callName_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
        callName_->setNameProperty("VideoPanelHeaderText", true);

        leftWidg->layout()->addWidget(callName_);
    }

    if (itemsToShow_ & kVPH_ShowTime)
    {
        callTime_ = new voipTools::BoundBox<PushButton_t>(centerWidg);
        callTime_->setPostfixColor(Ui::CommonStyle::getTextCommonColor());
        callTime_->setFont(font);
        callTime_->setEnabled(false);
        callTime_->setAlignment(Qt::AlignCenter);
        callTime_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        callTime_->setFixedWidth(SECURE_BTN_TEXT_W);
        callTime_->setIconSize(SECURE_BTN_ICON_W, SECURE_BTN_ICON_H);

        QObject::connect(callTime_, SIGNAL(clicked()), this, SLOT(_onSecureCallClicked()), Qt::QueuedConnection);
        Utils::ApplyStyle(callTime_, secureCallButton);
        centerWidg->layout()->addWidget(callTime_);
    }

    QWidget* parentWidget = rightWidg;
    auto addButton = [this, parentWidget] (const QString& _propertyName, const char* _slot)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);

        Utils::ApplyStyle(btn, _propertyName);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        parentWidget->layout()->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        return btn;
    };
    if (itemsToShow_ & kVPH_ShowMin)
    {
        btnMin_ = addButton(Ui::CommonStyle::getMinimizeButtonStyle(), SLOT(_onMinimize()));
    }

    if (itemsToShow_ & kVPH_ShowClose)
    {
        btnClose_ = addButton(Ui::CommonStyle::getCloseButtonStyle(), SLOT(_onClose()));
    }
}

Ui::VideoPanelHeader::~VideoPanelHeader()
{
    
}

void Ui::VideoPanelHeader::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    emit onMouseEnter();
}

void Ui::VideoPanelHeader::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    emit onMouseLeave();
}

void Ui::VideoPanelHeader::enableSecureCall(bool _enable)
{
    secureCallEnabled_ = _enable;
    if (callTime_)
    {
        callTime_->setEnabled(_enable);
        callTime_->setOffsets(_enable ? Utils::scale_value(12) : 0);
        callTime_->setImageForState(PushButton_t::normal, _enable ? ":/resources/video_panel/content_securecall_video_100.png" : "");
        callTime_->setFixedWidth(_enable ? SECURE_BTN_W : SECURE_BTN_TEXT_W);

        callTime_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
        callTime_->setColorForState(PushButton_t::hovered, _enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
        callTime_->setColorForState(PushButton_t::pressed, _enable ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

        callTime_->setCursor(_enable ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
    }
}

void Ui::VideoPanelHeader::_onSecureCallClicked()
{
    emit onSecureCallClicked(geometry());
}

void Ui::VideoPanelHeader::setCallName(const std::string& _name)
{
    if (!!callName_)
    {
        callName_->setName(_name.c_str());
    }
}

void Ui::VideoPanelHeader::setFullscreenMode(bool _en)
{
    if (btnMin_)
    {
        if (_en)
        {
            btnMin_->hide();
        }
        else
        {
            btnMin_->show();
        }
    }
}

void Ui::VideoPanelHeader::setSecureWndOpened(const bool _opened)
{
    assert(callTime_);
    if (callTime_)
    {
        if (_opened)
        {
            Utils::ApplyStyle(callTime_, secureCallButtonClicked);

            callTime_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_ACTIVE);
            callTime_->setColorForState(PushButton_t::hovered, COLOR_SECURE_BTN_ACTIVE);
            callTime_->setColorForState(PushButton_t::pressed, COLOR_SECURE_BTN_ACTIVE);
        }
        else
        {
            Utils::ApplyStyle(callTime_, secureCallButton);

            callTime_->setColorForState(PushButton_t::normal,  COLOR_SECURE_BTN_INACTIVE);
            callTime_->setColorForState(PushButton_t::hovered, secureCallEnabled_ ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);
            callTime_->setColorForState(PushButton_t::pressed, secureCallEnabled_ ? COLOR_SECURE_BTN_ACTIVE: COLOR_SECURE_BTN_INACTIVE);

            callTime_->setCursor(secureCallEnabled_ ? QCursor(Qt::PointingHandCursor) : QCursor(Qt::ArrowCursor));
            
#ifdef __APPLE__
            // On mac button does not recive mouse leave event.
            // Reset state force.
            callTime_->setState(PushButton_t::normal);
#endif
        }
    }
}

void Ui::VideoPanelHeader::setTime(unsigned _ts, bool _hasCall)
{
    if (!_hasCall)
    {
        if (!!callTime_)
        {
            callTime_->setText(QString(), "");
        }
    }
    else
    {
        if (!!callTime_)
        {
            callTime_->setText(QString(), getFotmatedTime(_ts).c_str());
        }
    }
}
