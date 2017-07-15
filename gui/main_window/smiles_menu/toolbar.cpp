#include "stdafx.h"
#include "toolbar.h"

#include "../../controls/CustomButton.h"
#include "../../controls/PictureWidget.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/utils.h"

namespace Ui
{
	using namespace Smiles;

	AttachedView::AttachedView(QWidget* _view, QWidget* _view_parent)
		:	view_(_view), 
			view_parent_(_view_parent)
	{

	}

	QWidget* AttachedView::get_view()
	{
		return view_;
	}

	QWidget* AttachedView::get_view_parent()
	{
		return view_parent_;
	}

	//////////////////////////////////////////////////////////////////////////
	// TabButton class
	//////////////////////////////////////////////////////////////////////////
	TabButton::TabButton(QWidget* _parent)
		:	CustomButton(_parent, nullptr)
			, attached_view_(nullptr, nullptr)
			
	{
		Init();
	}

	TabButton::TabButton(QWidget* _parent, const QString& _resource)
		:	CustomButton(_parent, _resource)
			, resource_(_resource)
			, attached_view_(nullptr)
	{
		Init();
	}

	TabButton::~TabButton()
	{

	}

	void TabButton::Init()
	{
		setObjectName("tab_button_Widget");

		setCursor(QCursor(Qt::PointingHandCursor));
		setCheckable(true);

		setFocusPolicy(Qt::NoFocus);
	}

	void TabButton::AttachView(const AttachedView& _view)
	{
		attached_view_ = _view;
	}

	const AttachedView& TabButton::GetAttachedView() const
	{
		return attached_view_;
	}



	//////////////////////////////////////////////////////////////////////////
	// Toolbar class
	//////////////////////////////////////////////////////////////////////////
	Toolbar::Toolbar(QWidget* _parent, buttons_align _align)
		:	QFrame(_parent),
			align_(_align),
			button_left_(0),
			button_right_(0),
			button_left_cap_(0),
			button_right_cap_(0),
			anim_scroll_(0)
	{
		setObjectName("toolbar_Widget");

		auto root_layout = Utils::emptyHLayout();
		setLayout(root_layout);

		view_area_ = new QScrollArea(this);
		view_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		QWidget* scroll_area_widget = new QWidget(view_area_);
		scroll_area_widget->setObjectName("scroll_area_widget");
		view_area_->setWidget(scroll_area_widget);
		view_area_->setWidgetResizable(true);
		view_area_->setFocusPolicy(Qt::NoFocus);

		Utils::grabTouchWidget(view_area_->viewport(), true);
		Utils::grabTouchWidget(scroll_area_widget);
		connect(QScroller::scroller(view_area_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

		button_left_cap_ = new PictureWidget(this, ":/resources/smiles_menu/picker_shadow_left_100.png");
		button_left_cap_->setFixedSize(Utils::scale_value(32), Utils::scale_value(48));
		button_right_cap_ = new PictureWidget(this, ":/resources/smiles_menu/picker_shadow_right_100.png");
		button_right_cap_->setFixedSize(Utils::scale_value(32), Utils::scale_value(48));

		root_layout->addWidget(view_area_);
		
		hor_layout_ = Utils::emptyHLayout();

		scroll_area_widget->setLayout(hor_layout_);

		QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
		hor_layout_->addSpacerItem(horizontalSpacer);

		if (_align == buttons_align::center)
		{
			QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
			hor_layout_->addSpacerItem(horizontalSpacer);
		}

		initScroll();
	}

	Toolbar::~Toolbar()
	{

	}

	void Toolbar::resizeEvent(QResizeEvent * _e)
	{
		QWidget::resizeEvent(_e);

		if (!button_left_ || !button_right_)
			return;
	
		QRect this_rect = geometry();

		button_left_->move(0, 0);
		button_right_->move(this_rect.width() - button_right_->width(), 0);
		int qtMagicConst = Utils::scale_value(5);
		button_left_cap_->move(button_left_cap_->width() - qtMagicConst, 0);
		button_right_cap_->move(this_rect.width() - button_right_->width() - button_left_cap_->width() + qtMagicConst, 0);

        updateArrowButtonsVisibility();
	}

	void Toolbar::wheelEvent(QWheelEvent* _e)
	{
		int numDegrees = _e->delta() / 8;
		int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;
        
		if (numSteps > 0)
			scrollStep(direction::left);
		else
			scrollStep(direction::right);

        qDebug() << "Toolbar::wheelEvent" << " numDegrees=" << numDegrees << ", numSteps=" << numSteps;
        
		QWidget::wheelEvent(_e);
	}

	void Toolbar::initScroll()
	{
		button_left_ = new CustomButton(this, ":/resources/smiles_menu/picker_arrow_left_100.png");
		button_left_->setActiveImage(":/resources/smiles_menu/picker_arrow_left_100.png");
		button_left_->setFillColor(QColor("#ffffff"));
		button_left_->setFixedSize(Utils::scale_value(32), Utils::scale_value(48));
		button_left_->setStyleSheet("border: none;");
		button_left_->setFocusPolicy(Qt::NoFocus);
		button_left_->setCursor(QCursor(Qt::PointingHandCursor));

		button_right_ = new CustomButton(this, ":/resources/smiles_menu/picker_arrow_right_100.png");
		button_right_->setActiveImage(":/resources/smiles_menu/picker_arrow_right_100.png");
		button_right_->setFillColor(QColor("#ffffff"));
		button_right_->setFixedSize(Utils::scale_value(32), Utils::scale_value(48));
		button_right_->setStyleSheet("border: none;");
		button_right_->setFocusPolicy(Qt::NoFocus);
		button_right_->setCursor(QCursor(Qt::PointingHandCursor));

		connect(button_left_, &TabButton::clicked, [this]()
		{
			scrollStep(Smiles::Toolbar::direction::left);
		});

		connect(button_right_, &TabButton::clicked, [this]()
		{
			scrollStep(Smiles::Toolbar::direction::right);
		});
	}
    
    void Toolbar::updateArrowButtonsVisibility()
    {
        auto scrollbar = view_area_->horizontalScrollBar();
        int max_val = scrollbar->maximum();
        int min_val = scrollbar->minimum();
        int cur_val = scrollbar->value();
        
        showButtons(min_val, max_val, cur_val);
    }

	void Toolbar::showButtons(int _min, int _max, int _cur)
	{
		bool show_left = (_cur > _min);
		bool show_right = (_cur < _max);

		button_left_->setVisible(show_left);
		button_left_cap_->setVisible(show_left);
		button_right_->setVisible(show_right);
		button_right_cap_->setVisible(show_right);
	}

	void Toolbar::touchScrollStateChanged(QScroller::State state)
	{
		for (auto iter : buttons_)
		{
			iter->setAttribute(Qt::WA_TransparentForMouseEvents, state != QScroller::Inactive);
		}
	}

	void Toolbar::scrollStep(direction _direction)
	{
		QRect view_rect = view_area_->viewport()->geometry();
		auto scrollbar = view_area_->horizontalScrollBar();

		int max_val = scrollbar->maximum();
		int min_val = scrollbar->minimum();
		int cur_val = scrollbar->value();

		int step = view_rect.width()/2;

		int to = 0;

		if (_direction == Toolbar::direction::right)
		{
			to = cur_val + step;
			if (to > max_val)
			{
				to = max_val;
			}
		}
		else
		{
			to = cur_val - step;
			if (to < min_val)
			{
				to = min_val;
			}

		}

		showButtons(min_val, max_val, to);

		QEasingCurve easing_curve = QEasingCurve::InQuad;
		int duration = 300;

		if (!anim_scroll_)
			anim_scroll_ = new QPropertyAnimation(scrollbar, "value");

		anim_scroll_->stop();
		anim_scroll_->setDuration(duration);
		anim_scroll_->setStartValue(cur_val);
		anim_scroll_->setEndValue(to);
		anim_scroll_->setEasingCurve(easing_curve);
		anim_scroll_->start();
	}

	void Toolbar::addButton(TabButton* _button)
	{
		_button->setAutoExclusive(true);

		int index = 0;
		if (align_ == buttons_align::left)
			index = hor_layout_->count() - 1;
		else if (align_ == buttons_align::right)
			index = hor_layout_->count();
		else
			index = hor_layout_->count() - 1;


		Utils::grabTouchWidget(_button);
		hor_layout_->insertWidget(index, _button);

		buttons_.push_back(_button);
	}

	TabButton* Toolbar::addButton(const QString& _resource)
	{
		TabButton* button = new TabButton(this, _resource);
		
		addButton(button);

		return button;
	}

	TabButton* Toolbar::addButton(const QPixmap& _icon)
	{
		TabButton* button = new TabButton(this);
		
		button->setIcon(QIcon(_icon));
        
        button->setIconSize(Utils::is_mac_retina()?_icon.size()/2:_icon.size());

		addButton(button);

		return button;
	}

	void Toolbar::paintEvent(QPaintEvent* _e)
	{
		QStyleOption opt;
		opt.init(this);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

		return QWidget::paintEvent(_e);
	}

	const QList<TabButton*>& Toolbar::GetButtons() const
	{
		return buttons_;
	}

	void Toolbar::scrollToButton(TabButton* _button)
	{
		view_area_->ensureWidgetVisible(_button);
        updateArrowButtonsVisibility();
	}
}