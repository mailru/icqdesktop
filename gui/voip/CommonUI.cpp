
#include "stdafx.h"
#include "CommonUI.h"
#include "../gui_settings.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../main_window/MainWindow.h"
#include "SelectionContactsForConference.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/MainPage.h"
#include "../my_info.h"

#define DEFAULT_ANIMATION_TIME  500

const QString vertSoundBg = "QWidget { background : #ffffff; }";
const QString horSoundBg = "QWidget { background : transparent; }";

const QString sliderGreenH =
"QSlider:handle:horizontal { background: solid #579e1c; width: 8dip; height: 8dip; margin-top: -11dip; margin-bottom: -11dip; border-radius: 4dip; }"
"QSlider:handle:horizontal:hover { background: solid #67bc21; border-radius: 4dip;}";

const QString sliderRedH =
"QSlider:handle:horizontal { background: solid #992117; width: 8dip; height: 8dip; margin-top: -11dip; margin-bottom: -11dip; border-radius: 4dip; }"
"QSlider:handle:horizontal:disabled { background: solid #992117; border-radius: 4dip; }";

const QString sliderGreenV =
"QSlider:handle:vertical { background: solid #579e1c; border: 1dip solid #579e1c; width: 8dip; height: 8dip; margin-left: -11dip; margin-right: -11dip; border-radius: 4dip; }"
"QSlider:handle:vertical:hover { background: solid #67bc21; border-radius: 4dip; }";

const QString sliderRedV =
"QSlider:handle:vertical { background: solid #992117; border: 1dip solid #579e1c; width: 8dip; height: 8dip; margin-left: -11dip; margin-right: -11dip; border-radius: 4dip; }"
"QSlider:handle:vertical:disabled { background: solid #992117; border-radius: 4dip; }";


Ui::ResizeEventFilter::ResizeEventFilter(std::vector<BaseVideoPanel*>& panels, 
	ShadowWindow* shadow, 
	QObject* _parent)
    : QObject(_parent) 
    , panels_(panels)
	, shadow_(shadow)
{
}

bool Ui::ResizeEventFilter::eventFilter(QObject* _obj, QEvent* _e)
{
	QWidget* parent = qobject_cast<QWidget*>(_obj);

    if (parent && 
		(_e->type() == QEvent::Resize ||
        _e->type() == QEvent::Move || 
        _e->type() == QEvent::WindowActivate || 
        _e->type() == QEvent::NonClientAreaMouseButtonPress ||
        _e->type() == QEvent::ZOrderChange ||
        _e->type() == QEvent::ShowToParent ||
        _e->type() == QEvent::WindowStateChange ||
        _e->type() == QEvent::UpdateRequest)) {

        const QRect rc = parent->geometry();

		bool bActive = parent->isActiveWindow();

        for (unsigned ix = 0; ix < panels_.size(); ix++)
        {
			BaseVideoPanel* panel = panels_[ix];
            if (!panel)
            {
                continue;
            }

			bActive = bActive || panel->isActiveWindow();

			panel->updatePosition(*parent);
        }

		if (shadow_)
		{
			int shadowWidth = get_gui_settings()->get_shadow_width();
			shadow_->move(rc.topLeft().x() - shadowWidth, rc.topLeft().y() - shadowWidth);
			shadow_->resize(rc.width() + 2 * shadowWidth, rc.height() + 2 * shadowWidth);

			shadow_->setActive(bActive);

#ifdef _WIN32
			SetWindowPos((HWND)shadow_->winId(), (HWND)parent->winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE);
#endif
		}
    }

    return QObject::eventFilter(_obj, _e);
}

void Ui::ResizeEventFilter::addPanel(BaseVideoPanel* _panel)
{
	panels_.push_back(_panel);
}

void  Ui::ResizeEventFilter::removePanel(BaseVideoPanel* _panel)
{
	panels_.erase(std::remove(panels_.begin(), panels_.end(), _panel), panels_.end());
}



Ui::ShadowWindowParent::ShadowWindowParent(QWidget* parent) : shadow_(nullptr)
{
	if (platform::is_windows())
	{
		int shadowWidth = get_gui_settings()->get_shadow_width();

		QBrush b(Qt::transparent);
		QMatrix m;
		m.translate(shadowWidth, shadowWidth);
		b.setMatrix(m);

		shadow_ = new ShadowWindow(b, shadowWidth);

		QPoint pos = parent->mapToGlobal(QPoint(parent->rect().x(), parent->rect().y()));
		shadow_->move(pos.x() - shadowWidth, pos.y() - shadowWidth);
		shadow_->resize(parent->rect().width() + 2 * shadowWidth, parent->rect().height() + 2 * shadowWidth);
	}
}

Ui::ShadowWindowParent::~ShadowWindowParent()
{
    delete shadow_;
}

void Ui::ShadowWindowParent::showShadow()
{
	if (shadow_)
	{
		shadow_->show();
	}
}

void Ui::ShadowWindowParent::hideShadow()
{
	if (shadow_)
	{
		shadow_->hide();
	}
}

Ui::ShadowWindow* Ui::ShadowWindowParent::getShadowWidget()
{
	return shadow_;
}

void Ui::ShadowWindowParent::setActive(bool _value)
{
	if (shadow_)
	{
		shadow_->setActive(_value);
	}
}

Ui::AspectRatioResizebleWnd::AspectRatioResizebleWnd()
    : QWidget(NULL)
    //, firstTimeUseAspectRatio_(true)
    , useAspect_(true)
    , aspectRatio_(0.0f)
{
    selfResizeEffect_ = new UIEffects(*this);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), this, SLOT(onVoipFrameSizeChanged(const voip_manager::FrameSize&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);
}

Ui::AspectRatioResizebleWnd::~AspectRatioResizebleWnd()
{
    
}

bool Ui::AspectRatioResizebleWnd::isInFullscreen() const
{
    return isFullScreen();
}

void Ui::AspectRatioResizebleWnd::switchFullscreen()
{
    if (!isFullScreen())
    {
        showFullScreen();
    }
    else
    {
        showNormal();

        if (aspectRatio_ > 0.001f && selfResizeEffect_)
        {
            const QRect rc = rect();
            const QPoint p = mapToGlobal(rc.topLeft());
            QRect endRc(p.x(), p.y(), rc.width(), rc.width() / aspectRatio_);

            selfResizeEffect_->geometryTo(endRc, 500);
        }
    }
}

void Ui::AspectRatioResizebleWnd::onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs)
{
    if ((quintptr)_fs.hwnd == getContentWinId() && fabs(_fs.aspect_ratio - aspectRatio_) > 0.001f)
    {
        const float wasAr = aspectRatio_;
        aspectRatio_ = _fs.aspect_ratio;
        fitMinimalSizeToAspect();
        applyFrameAspectRatio(wasAr);
        
#ifdef __APPLE__
        platform_macos::setAspectRatioForWindow(*this, aspectRatio_);
#endif
    }
}

void Ui::AspectRatioResizebleWnd::applyFrameAspectRatio(float _wasAr)
{
    if (useAspect_ && aspectRatio_ > 0.001f && selfResizeEffect_ && !isFullScreen())
    {
        QRect rc = rect();

#ifndef __APPLE__
        const QPoint p = mapToGlobal(rc.topLeft());
#else
        // On Mac we have wrong coords with mapToGlobal. Maybe because we attach own view to widnow.
        const QPoint p(x(), y());
#endif
        
        QRect endRc;
        if (_wasAr > 0.001f && fabs((1.0f / aspectRatio_) - _wasAr) < 0.0001f)
        {
            endRc = QRect(p.x(), p.y(), rc.height(), rc.width());
        }
        else
        {
            endRc = QRect(p.x(), p.y(), rc.width(), rc.width() / aspectRatio_);
        }

        const QSize minSize = minimumSize();
        if (endRc.width() < minSize.width())
        {
            const int w = minSize.width();
            const int h = w / aspectRatio_;
            endRc.setRight(endRc.left() + w);
            endRc.setBottom(endRc.top() + h);
        }
        if (endRc.height() < minSize.height())
        {
            const int h = minSize.height();
            const int w = h * aspectRatio_;
            endRc.setRight(endRc.left() + w);
            endRc.setBottom(endRc.top() + h);
        }

        QDesktopWidget dw;
        const auto screenRect = dw.availableGeometry(this);

        if (endRc.right() > screenRect.right())
        {
            const int w = endRc.width();
            endRc.setRight(screenRect.right());
            endRc.setLeft(endRc.right() - w);
        }

        if (endRc.bottom() > screenRect.bottom())
        {
            const int h = endRc.height();
            endRc.setBottom(screenRect.bottom());
            endRc.setTop(endRc.bottom() - h);
        }

        if (screenRect.width() < endRc.width())
        {
            endRc.setLeft(screenRect.left());
            endRc.setRight(screenRect.right());

            const int h = endRc.width() / aspectRatio_;
            endRc.setTop(endRc.bottom() - h);
        }

        if (screenRect.height() < endRc.height())
        {
            endRc.setTop(screenRect.top());
            endRc.setBottom(screenRect.bottom());

            const int w = endRc.height() * aspectRatio_;
            endRc.setLeft(endRc.right() - w);
        }

        //if (firstTimeUseAspectRatio_)
        {
            {
                const int bestW = 0.6f * screenRect.width();
                if (endRc.width() > bestW)
                {
                    const int bestH = bestW / aspectRatio_;
                    endRc.setLeft((screenRect.x() + screenRect.width() - bestW) / 2);
                    endRc.setRight(endRc.left() + bestW);

                    endRc.setTop((screenRect.y() + screenRect.height() - bestH) / 2);
                    endRc.setBottom(endRc.top() + bestH);
                }
            }
            {/* NEED TO EXECUTE 2 TIMES, BECAUSE CALC FOR BEST W NOT MEANS USING BEST H*/
                const int bestH = 0.8f * screenRect.height();
                if (endRc.height() > bestH)
                {
                    const int bestW = bestH * aspectRatio_;
                    endRc.setLeft((screenRect.x() + screenRect.width() - bestW) / 2);
                    endRc.setRight(endRc.left() + bestW);

                    endRc.setTop((screenRect.y() + screenRect.height() - bestH) / 2);
                    endRc.setBottom(endRc.top() + bestH);
                }
            }
            //firstTimeUseAspectRatio_ = false;
        }

        selfResizeEffect_->geometryTo(endRc, 500);
    }
}

void Ui::AspectRatioResizebleWnd::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        escPressed();
    }
}

void Ui::AspectRatioResizebleWnd::useAspect()
{
    useAspect_ = true;
    fitMinimalSizeToAspect();
    applyFrameAspectRatio(0.0f);
#ifdef __APPLE__
    platform_macos::setAspectRatioForWindow(*this, aspectRatio_);
#endif
}

void Ui::AspectRatioResizebleWnd::unuseAspect()
{
    useAspect_ = false;
    setMinimumSize(originMinSize_);
       
#ifdef __APPLE__
    platform_macos::unsetAspectRatioForWindow(*this);
#endif
}

#ifdef _WIN32
bool Ui::AspectRatioResizebleWnd::onWMSizing(RECT& _rc, unsigned _wParam)
{
    const int cw = _rc.right - _rc.left;
    const int ch = _rc.bottom - _rc.top;

    if (!useAspect_ || aspectRatio_ < 0.001f)
    {
        return false;
    }

    switch(_wParam)
    {
    case WMSZ_TOP:
    case WMSZ_BOTTOM:
        {
            int w = ch * aspectRatio_;
            if (w >= minimumWidth())
            {
                _rc.right = _rc.left + w;
            }
            else
            {
                _rc.right = _rc.left + minimumWidth();

                if (_wParam == WMSZ_BOTTOM)
                    _rc.bottom = _rc.top + minimumWidth() / aspectRatio_;
                else
                    _rc.top = _rc.bottom - minimumWidth() / aspectRatio_;
            }
        }
        break;

    case WMSZ_LEFT:
    case WMSZ_RIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.bottom = _rc.top + h;
        }
        break;

    case WMSZ_TOPLEFT:
    case WMSZ_TOPRIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.top = _rc.bottom - h;
        }
        break;

    case WMSZ_BOTTOMLEFT:
    case WMSZ_BOTTOMRIGHT:
        {
            int h = cw / aspectRatio_;
            _rc.bottom = _rc.top + h;
        }
        break;

    default: return false;
    }

    return true;
}
#endif

bool Ui::AspectRatioResizebleWnd::nativeEvent(const QByteArray&, void* _message, long* _result)
{
#ifdef _WIN32
    MSG* msg = reinterpret_cast<MSG*>(_message);
    if (isVisible() && msg->hwnd == (HWND)winId())
    {
        if (msg->message == WM_SIZING)
        {
            if (aspectRatio_ < 0.001f)
            {
                return false;
            }

            RECT* rc = (RECT*)msg->lParam;
            if (!rc)
            {
                return false;
            }

            *_result = TRUE;
            return onWMSizing(*rc, msg->wParam);
        }/* else if (msg->_message == WM_WINDOWPOSCHANGING) {
            if (aspectRatio_ < 0.001f) {
                return false;
            }

            WINDOWPOS* wp = (WINDOWPOS*)msg->lParam;
            if (!wp) {
                return false;
            }

            *_result = 0;
            return _onWindowPosChanging(*wp);
        }*/
    }
#else
#endif
    return false;
}

void Ui::AspectRatioResizebleWnd::saveMinSize(const QSize& size)
{
    originMinSize_ = size;
}

void Ui::AspectRatioResizebleWnd::fitMinimalSizeToAspect()
{
    int height = originMinSize_.width() / aspectRatio_;
    int width  = originMinSize_.height() * aspectRatio_;

    if (height < originMinSize_.height())
    {
        setMinimumSize(width, originMinSize_.height());
    } else
    {
        setMinimumSize(originMinSize_.width(), height);
    }
}

//void Ui::AspectRatioResizebleWnd::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
//{
//    if (_contactEx.connection_count == 1)
//    {
//        firstTimeUseAspectRatio_ = true;
//    }
//}

Ui::UIEffects::UIEffects(QWidget& _obj)
    : fadeEffect_(new QGraphicsOpacityEffect(&_obj))
    , fadedIn_(true)
    , obj_(_obj)
    , resizeAnimation_(new QPropertyAnimation(&_obj, "geometry"))
    , animation_(new QPropertyAnimation(fadeEffect_, "opacity"))
{
    _obj.setGraphicsEffect(fadeEffect_);
    animation_->setEasingCurve(QEasingCurve::InOutQuad);
    resizeAnimation_->setEasingCurve(QEasingCurve::InOutQuad);
}

Ui::UIEffects::~UIEffects()
{
    delete animation_;
}

void Ui::UIEffects::geometryTo(const QRect& _rc, unsigned _interval)
{
    if (resizeAnimation_)
    {
        resizeAnimation_->stop();
        resizeAnimation_->setDuration(_interval);
        resizeAnimation_->setEndValue(_rc);
        resizeAnimation_->start();
    }
}

void Ui::UIEffects::fadeOut(unsigned _interval)
{
#ifdef __APPLE__
    platform_macos::fadeOut(&obj_);
#else
    if (1)
    {//(animation_ && fadedIn_) {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(0.01);
        animation_->start();
    }
#endif

    fadedIn_ = false;
}

void Ui::UIEffects::fadeIn(unsigned _interval)
{
#ifdef __APPLE__
    platform_macos::fadeIn(&obj_);
#else
    if (1)
    {// //if (animation_ && !fadedIn_) {
        animation_->stop();
        animation_->setDuration(_interval);
        animation_->setEndValue(1.0);
        animation_->start();
    }
#endif

    fadedIn_ = true;
}

bool Ui::UIEffects::isFadedIn()
{
    return fadedIn_;
}

Ui::BaseVideoPanel::BaseVideoPanel(QWidget* parent, Qt::WindowFlags f) :
	QWidget(parent, f), grabMouse(false)
{
	hide();
}

void Ui::BaseVideoPanel::fadeIn(unsigned int duration)
{
	if (!effect_)
	{
		effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this));
	}
	
	if (effect_)
	{
		effect_->fadeIn(duration);
	}
}

void Ui::BaseVideoPanel::fadeOut(unsigned int duration)
{
	if (!effect_)
	{
		effect_ = std::unique_ptr<UIEffects>(new(std::nothrow) UIEffects(*this));
	}
	

	if (effect_)
	{
		effect_->fadeOut(duration);
	}
}

bool Ui::BaseVideoPanel::isGrabMouse()
{
    return grabMouse;
}

bool Ui::BaseVideoPanel::isFadedIn()
{
    return effect_ && effect_->isFadedIn();
}


Ui::BaseTopVideoPanel::BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f) : BaseVideoPanel(parent, f) {}

void Ui::BaseTopVideoPanel::updatePosition(const QWidget& parent)
{
    // We have code dublication here,
    // because Mac and Qt have different coords systems.
    // We can convert Mac coords to Qt, but we need to add
    // special cases for multi monitor systems.
#ifdef __APPLE__
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
    platform_macos::setWindowPosition(*this,
        QRect(parentRect.left(),
              parentRect.top() + parentRect.height() - height(),
              parentRect.width(),
              height()));
//    auto rc = platform_macos::getWindowRect(*parentWidget());
#else
    auto rc = parentWidget()->geometry();
    move(rc.x(), rc.y());
    setFixedWidth(rc.width());
#endif
}


Ui::BaseBottomVideoPanel::BaseBottomVideoPanel(QWidget* parent, Qt::WindowFlags f) : BaseVideoPanel(parent, f) {}

void Ui::BaseBottomVideoPanel::updatePosition(const QWidget& parent)
{
    // We have code dublication here,
    // because Mac and Qt have different coords systems.
    // We can convert Mac coords to Qt, but we need to add
    // special cases for multi monitor systems.
#ifdef __APPLE__
	//auto rc = platform_macos::getWindowRect(*parentWidget());
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
    platform_macos::setWindowPosition(*this,
                    QRect(parentRect.left(),
                          parentRect.top(),
                          parentRect.width(),
                          height()));
#else
    auto rc = parentWidget()->geometry();
    move(rc.x(), rc.y() + rc.height() - rect().height());
    setFixedWidth(rc.width());
#endif
}

Ui::PanelBackground::PanelBackground(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background-color: #640000;");
    auto videoPanelEffect_ = new UIEffects(*this);
    videoPanelEffect_->fadeOut(0);
}

void Ui::PanelBackground::updateSizeFromParent()
{
    auto parent = parentWidget();
    if (parent)
    {
        resize(parent->size());
    }
}


Ui::QSliderEx::QSliderEx(
                         Qt::Orientation _orientation,
                         QWidget* _parent)
: QSlider(_orientation, _parent)
{
    
}

Ui::QSliderEx::~QSliderEx()
{
    
}

void Ui::QSliderEx::mousePressEvent(QMouseEvent* _ev)
{
    if (_ev->button() == Qt::LeftButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum()-minimum()) * (height()-_ev->y())) / height() ) ;
        else
            setValue(minimum() + ((maximum()-minimum()) * _ev->x()) / width() ) ;
        
        _ev->accept();
    }
    QSlider::mousePressEvent(_ev);
}

bool Ui::onUnderMouse(QWidget& _widg)
{
    auto rc = _widg.rect();
    auto pg = _widg.mapToGlobal(rc.topLeft());
    QRect rcg(pg.x(), pg.y(), rc.width(), rc.height());
    return rcg.contains(QCursor::pos());
}

Ui::QPushButtonEx::QPushButtonEx(QWidget* _parent)
: QPushButton(_parent)
{
    
}

Ui::QPushButtonEx::~QPushButtonEx()
{
    
}

void Ui::QPushButtonEx::enterEvent(QEvent* _e)
{
    QPushButton::enterEvent(_e);
    emit onHover();
}

Ui::VolumeControl::VolumeControl
(
 QWidget* _parent,
 bool _horizontal,
 bool _onMainWindow,
 const QString& _backgroundStyle,
 const std::function<void(QPushButton&, bool)>& _onChangeStyle)
: QWidget(_parent, (_horizontal ? Qt::Widget : Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint))
, parent_(_parent)
, horizontal_(_horizontal)
, onChangeStyle_(_onChangeStyle)
, rootWidget_(NULL)
, audioPlaybackDeviceMuted_(false)
, background_(_backgroundStyle)
, checkMousePos_(this)
, actualVol_(0)
, onMainWindow_(_onMainWindow)
{
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Utils::LoadStyle(":/voip/volume_control.qss"));
    
    setContentsMargins(0, 0, 0, 0);
    
    if (horizontal_)
    {
        setProperty("VolumeControlHor", true);
    }
    else
    {
        setProperty("VolumeControlVert", true);
    }
    
    btn_ = new QPushButtonEx(this);
    btn_->setCursor(QCursor(Qt::PointingHandCursor));
    btn_->setAttribute(Qt::WA_NoMousePropagation);
    
    slider_ = new QSliderEx(horizontal_ ? Qt::Horizontal : Qt::Vertical, this);
    slider_->setCursor(QCursor(Qt::PointingHandCursor));
    
    rootWidget_ = new QWidget(this);
    auto widg = rootWidget_;
    widg->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    widg->setContentsMargins(0, 0, 0, 0);
    
    QBoxLayout* layout;
    if (horizontal_)
    {
        layout = Utils::emptyHLayout();
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else
    {
        layout = Utils::emptyVLayout();
        layout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    }
    
    if (horizontal_)
    {
        layout->addWidget(btn_);
        layout->addSpacing(Utils::scale_value(10));
        layout->addWidget(slider_);
    }
    else
    {
        layout->addSpacing(Utils::scale_value(12));
        layout->addWidget(slider_);
        layout->addSpacing(Utils::scale_value(8));
        layout->addWidget(btn_);
        layout->addSpacing(Utils::scale_value(8));
    }
    
    widg->setLayout(layout);
    
    Utils::ApplyStyle(widg, background_);
    
    if (onChangeStyle_ != NULL && btn_)
    {
        onChangeStyle_(*btn_, true);
    }
    
    if (horizontal_)
    {
        slider_->setProperty("VolumeSliderHor", true);
        slider_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    }
    else
    {
        slider_->setProperty("VolumeSliderVert", true);
        slider_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    }
    
    auto horLayout = Utils::emptyHLayout();
    horLayout->addWidget(widg);
    setLayout(horLayout);
    
    if (horizontal_)
    {
        slider_->hide();
    }
    
    connect(slider_, SIGNAL(valueChanged(int)), this, SLOT(onVolumeChanged(int)), Qt::QueuedConnection);
    connect(btn_, SIGNAL(clicked()), this, SLOT(onMuteOnOffClicked()), Qt::QueuedConnection);
    connect(&checkMousePos_, SIGNAL(timeout()), this, SLOT(onCheckMousePos()), Qt::QueuedConnection);
    checkMousePos_.setInterval(500);
    
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMuteChanged(const std::string&,bool)), this, SLOT(onVoipMuteChanged(const std::string&,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVolumeChanged(const std::string&,int)), this, SLOT(onVoipVolumeChanged(const std::string&,int)), Qt::DirectConnection);
    
    connect(btn_, &QPushButton::clicked, this, &VolumeControl::clicked);
    
#ifdef __APPLE__
    // This is hack: We have problem with layout under Mac.
    // We need to update style of button a little later.
    // TODO: find better solution.
    QTimer::singleShot(1, [=]()
    {
        if (onChangeStyle_ != NULL && btn_)
        {
            onChangeStyle_(*btn_, audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f);
        }
    });
#endif
}

Ui::VolumeControl::~VolumeControl()
{
    
}

void Ui::VolumeControl::onCheckMousePos()
{
    if (!onUnderMouse(*this) || (parentWidget() && !parentWidget()->isVisible()))
    {
        hideSlider();
    }
}

QPoint Ui::VolumeControl::getAnchorPoint() const
{
    const auto rcb = btn_->rect();
    const auto rcc = rect();
    
    if (horizontal_)
    {
        return QPoint(0, (rcc.height() - rcb.height()) / 2);
    }
    else
    {
        return QPoint((rcc.width() - rcb.width()) / 2, rcc.height() - rcb.height() - Utils::scale_value(8));
    }
}

void Ui::VolumeControl::updateSlider()
{
    if (horizontal_)
    {
        if (audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f)
        {
            Utils::ApplyStyle(slider_, sliderRedH);
        }
        else
        {
            Utils::ApplyStyle(slider_, sliderGreenH);
        }
    }
    else
    {
        if (audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f)
        {
            Utils::ApplyStyle(slider_, sliderRedV);
        }
        else
        {
            Utils::ApplyStyle(slider_, sliderGreenV);
        }
    }
}

void Ui::VolumeControl::onVoipMuteChanged(const std::string& _deviceType, bool _muted)
{
    if (_deviceType == "audio_playback")
    {
        slider_->setEnabled(!_muted);
        //slider_->setVisible(!_muted);
        
        audioPlaybackDeviceMuted_ = _muted;
        updateSlider();
        
        if (onChangeStyle_ != NULL && btn_)
        {
            onChangeStyle_(*btn_, _muted || actualVol_ <= 0.0001f);
        }
        emit onMuteChanged(_muted || actualVol_ <= 0.0001f);
    }
}

void Ui::VolumeControl::onVolumeChanged(int _vol)
{
    const int newVol = std::max(std::min(100, _vol), 0);
    if (actualVol_ != newVol)
    {
        Ui::GetDispatcher()->getVoipController().setVolumeAPlayback(newVol);
    }
}

void Ui::VolumeControl::onMuteOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
}

void Ui::VolumeControl::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
{
    if (_deviceType == "audio_playback")
    {
        actualVol_ = std::max(std::min(100, _vol), 0);
        
        if (onChangeStyle_ != NULL && btn_)
        {
            onChangeStyle_(*btn_, audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f);
        }
        emit onMuteChanged(audioPlaybackDeviceMuted_ || actualVol_ <= 0.0001f);
        
        slider_->blockSignals(true);
        slider_->setValue(actualVol_);
        slider_->blockSignals(false);
        
        updateSlider();
    }
}

void Ui::VolumeControl::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);
    emit controlActivated(true);
    checkMousePos_.start();
}

void Ui::VolumeControl::hideEvent(QHideEvent* _e)
{
    checkMousePos_.stop();
    QWidget::hideEvent(_e);
    emit controlActivated(false);
}

void Ui::VolumeControl::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    hideSlider();
}

void Ui::VolumeControl::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::WindowDeactivate)
    {
        hideSlider();
    }
}

void Ui::VolumeControl::hideSlider()
{
    hide();
}


Ui::VolumeControlHorizontal::VolumeControlHorizontal (QWidget* _parent, bool _onMainWindow,
                                                      const QString& _backgroundStyle,
                                                      const std::function<void(QPushButton&, bool)>& _onChangeStyle)
: VolumeControl(_parent, true, _onMainWindow, _backgroundStyle, _onChangeStyle)
{
    connect(btn_, &QPushButtonEx::onHover, this, &VolumeControlHorizontal::onHoverButton);
}

void Ui::VolumeControlHorizontal::showSlider()
{
    if (!slider_->isVisible())
    {
        slider_->show();
        emit controlActivated(true);
    }
}

QPoint Ui::VolumeControlHorizontal::soundButtonGlobalPosition()
{
    return btn_->mapToGlobal(btn_->rect().topLeft());;
}

void Ui::VolumeControlHorizontal::hideSlider()
{
    if (slider_->isVisible())
    {
        slider_->hide();
        emit controlActivated(false);
    }
}


Ui::VolumeGroup::VolumeGroup (QWidget* parent, bool onMainPage, const std::function<void(QPushButton&, bool)>& _onChangeStyle, int verticalSize) : QWidget(parent), verticalSize(verticalSize)
{
    // We create volume control on parent because they shuold not affect to layout.
    // VolumeGroup is used only for right positioning on panel.
    hVolumeControl = new VolumeControlHorizontal(parent, onMainPage, horSoundBg, _onChangeStyle);
    vVolumeControl = new VolumeControl(parent, false, onMainPage, vertSoundBg, _onChangeStyle);

    setLayout(Utils::emptyHLayout());
    layout()->setAlignment(Qt::AlignVCenter);
    
    auto fakeWidget = new QWidget(this);
    
    // It is hack, because we have very complicated layout.
    // Todo fix this layout.
    fakeWidget->setFixedSize(Utils::scale_value(QSize(38, 38)));
    layout()->addWidget(fakeWidget);

    hVolumeControl->show();
    hVolumeControl->hideSlider();
    
    connect(hVolumeControl, &VolumeControlHorizontal::onHoverButton,    this, &VolumeGroup::showSlider);
    connect(hVolumeControl, &VolumeControl::controlActivated, this, &VolumeGroup::controlActivated);
    connect(vVolumeControl, &VolumeControl::controlActivated, this, &VolumeGroup::controlActivated);
    connect(hVolumeControl, &VolumeControl::clicked, this, &VolumeGroup::clicked);
    
#ifdef __APPLE__
    connect(hVolumeControl, &VolumeControl::clicked, [this]()
            {
                forceShowSlider();
            });
#endif
}

void Ui::VolumeGroup::showSlider()
{
    bool isActive = true;
    isVideoWindowActive(isActive);
    
    if (isActive)
    {
        forceShowSlider();
    }
}

void Ui::VolumeGroup::forceShowSlider()
{
    const auto rc = parentWidget()->rect();
    
    if (rc.width() >= verticalSize)
    {
        hVolumeControl->showSlider();
    }
    else
    {
        updateSliderPosition();
        vVolumeControl->show();
        vVolumeControl->activateWindow();
#ifdef __APPLE__
        vVolumeControl->raise();
#endif
        vVolumeControl->setFocus(Qt::OtherFocusReason);
    }
}

void Ui::VolumeGroup::moveEvent(QMoveEvent * event)
{
    hVolumeControl->move(x(), y() - hVolumeControl->getAnchorPoint().y());
    updateSliderPosition();
}

QWidget* Ui::VolumeGroup::verticalVolumeWidget()
{
    return vVolumeControl;
}

void Ui::VolumeGroup::hideSlider()
{
    hVolumeControl->hideSlider();
    vVolumeControl->hideSlider();
}

void Ui::VolumeGroup::updateSliderPosition()
{
    const auto rc = parentWidget()->rect();
    
    if (rc.width() < verticalSize)
    {
        int xOffset = 0;
#ifdef __APPLE__
        xOffset = Utils::scale_value(1);
#endif
        
        auto p  = vVolumeControl->getAnchorPoint();
        auto p2 = hVolumeControl->soundButtonGlobalPosition();
        
        p2.setX(p2.x() - p.x() + xOffset);
        p2.setY(p2.y() - p.y());
        
        vVolumeControl->move(p2);
    }
}


Ui::TransparentPanel::TransparentPanel(QWidget* _parent, BaseVideoPanel* _eventWidget) : BaseVideoPanel(_parent),
    eventWidget_(_eventWidget)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* rootLayout = Utils::emptyHLayout();
    rootLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setLayout(rootLayout);

#ifndef __APPLE__
    backgroundWidget_ = new PanelBackground(this);
    backgroundWidget_->updateSizeFromParent();
#endif

    auto rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    layout()->addWidget(rootWidget_);
}


Ui::TransparentPanel::~TransparentPanel()
{

}

void Ui::TransparentPanel::updatePosition(const QWidget& parent)
{
    move(eventWidget_->pos());
    setFixedSize(eventWidget_->size());
}

void Ui::TransparentPanel::resizeEvent(QResizeEvent * event)
{
    if (backgroundWidget_)
    {
        backgroundWidget_->updateSizeFromParent();
    }
}

void Ui::TransparentPanel::mouseMoveEvent(QMouseEvent* _e)
{
    resendMouseEventToPanel(_e);
}

void Ui::TransparentPanel::mouseReleaseEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

void Ui::TransparentPanel::mousePressEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

template <typename E> void Ui::TransparentPanel::resendMouseEventToPanel(E* event_)
{
    if (eventWidget_->isVisible() && (eventWidget_->rect().contains(event_->pos()) || eventWidget_->isGrabMouse()))
    {
        QApplication::sendEvent(eventWidget_, event_);
    }
}


Ui::FullVideoWindowPanel::FullVideoWindowPanel(QWidget* parent) :
#ifndef __APPLE__
    Ui::BaseVideoPanel(parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
#else
    Ui::BaseVideoPanel(parent, Qt::Window | Qt::FramelessWindowHint /* | Qt::WindowDoesNotAcceptFocus*/ | Qt::NoDropShadowWindowHint)
#endif
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void Ui::FullVideoWindowPanel::updatePosition(const QWidget& parent)
{
#ifdef __APPLE__
    //auto rc = platform_macos::getWindowRect(*parentWidget());
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
    platform_macos::setWindowPosition(*this,
        QRect(parentRect.left(),
            parentRect.top(),
            parentRect.width(),
            parentRect.height()));
#else
    auto rc = parentWidget()->geometry();
    setFixedSize(rc.width(), rc.height());
	move(rc.x(), rc.y());
#endif
}

void Ui::FullVideoWindowPanel::resizeEvent(QResizeEvent *event)
{
    emit(onResize());
}


void showAddUserToVideoConverenceDialog(QObject* parent, QWidget* parentWindow, 
    std::function< void(Ui::SelectionContactsForConference*) > connectSignal,
    std::function< void() > disconnectSignal)
{
    emit Utils::InterConnector::instance().searchEnd();
    Logic::getContactListModel()->clearChecked();

    Logic::ChatMembersModel model(parent);
    Ui::ContactsForVideoConference modelAll(parent, model);

    Ui::ConferenceSearchMember usersSearchModel;
    usersSearchModel.setChatMembersModel(&modelAll);

    Ui::SelectionContactsForConference contactsWidget(
        &model,
        &modelAll,
        QT_TRANSLATE_NOOP("voip_pages", "Add to call"),
        parentWindow,
        usersSearchModel);

    connectSignal(&contactsWidget);

    contactsWidget.setMaximumSelectedCount(4);

    if (contactsWidget.show() == QDialog::Accepted)
    {
        auto selectedContacts = Logic::getContactListModel()->GetCheckedContacts();
        Logic::getContactListModel()->clearChecked();

        for (const Logic::ContactItem& contact : selectedContacts)
        {
            Ui::GetDispatcher()->getVoipController().setStartV(contact.get_aimid().toUtf8(), true);
        }
    }

    disconnectSignal();

    Logic::getContactListModel()->clearChecked();
}

void Ui::showAddUserToVideoConverenceDialogVideoWindow(QObject* parent, FullVideoWindowPanel* parentWindow)
{
    showAddUserToVideoConverenceDialog(parent, parentWindow, [parentWindow](Ui::SelectionContactsForConference* dialog)
        {
            QObject::connect(parentWindow, &FullVideoWindowPanel::onResize, dialog, &Ui::SelectionContactsForConference::updateSize);
        },
        [parentWindow]()
        {
            parentWindow->disconnect();
        }
    );
}

void Ui::showAddUserToVideoConverenceDialogMainWindow(QObject* parent, QWidget* parentWindow)
{
    showAddUserToVideoConverenceDialog(parent, parentWindow, [](Ui::SelectionContactsForConference* dialog) {}, []() {});
}


Ui::PureClickedWidget::PureClickedWidget(QWidget* parent)
	: QWidget(parent)
	, Enabled_(true)
{
}

void Ui::PureClickedWidget::setEnabled(bool value)
{
	Enabled_ = value;
	update();
}

void Ui::PureClickedWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if (Enabled_)
		emit clicked();
	QWidget::mouseReleaseEvent(e);
}
