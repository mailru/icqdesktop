#include "stdafx.h"
#include "NameAndStatusWidget.h"

#include "VoipTools.h"
#include "../controls/TextEmojiWidget.h"
#include "../utils/utils.h"

template<typename __Base>
Ui::ShadowedWidget<__Base>::ShadowedWidget(QWidget* _parent, int _tailLen, double _alphaFrom, double _alphaTo)
    : __Base(_parent)
    , tailLenPx_(_tailLen)
{

    QGradientStops stops;
    QColor voipShadowColor("#000000");
    voipShadowColor.setAlphaF(_alphaFrom);
    stops.append(qMakePair(0.0f, voipShadowColor));
    voipShadowColor.setAlphaF(_alphaTo);
    stops.append(qMakePair(1.0f, voipShadowColor));

    linearGradient_.setStops(stops);
}

template<typename __Base>
Ui::ShadowedWidget<__Base>::~ShadowedWidget()
{
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::resizeEvent(QResizeEvent* _e)
{
    __Base::resizeEvent(_e);

    const QRect rc = __Base::rect();
    linearGradient_.setStart(rc.right() - tailLenPx_, 0.0f);
    linearGradient_.setFinalStop(rc.right(), 0.0f);
}

template<typename __Base>
void Ui::ShadowedWidget<__Base>::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.save();

    const QRect rc = __Base::rect();
    if (__Base::internalWidth(painter) <= rc.width())
    {
        __Base::internalDraw(painter, rc);
        painter.restore();
        return;
    }

    QPixmap pixmap(QSize(rc.width(), rc.height()));
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setPen(painter.pen());
    p.setFont(painter.font());

    __Base::internalDraw(p, rc);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    p.fillRect(QRect(rc.right() - tailLenPx_ + 1, 0, tailLenPx_, pixmap.rect().height()), linearGradient_);
    p.end();

    painter.drawPixmap(QPoint(rc.left(), rc.top()), pixmap);
    painter.restore();
}

Ui::NameAndStatusWidget::NameAndStatusWidget(QWidget* _parent, int _nameBaseline, int _statusBaseline)
    : QWidget(_parent)
    , name_(NULL)
    , status_(NULL)
{
    setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignLeft);
    setLayout(rootLayout);

    rootLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);

    name_ = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    name_->setFont(font);
    name_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    name_->setSizeToBaseline(_nameBaseline); // SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(name_);

    status_ = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    status_->setFont(font);
    status_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    status_->setSizeToBaseline(_statusBaseline);// SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(status_);

    rootLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

Ui::NameAndStatusWidget::~NameAndStatusWidget()
{
    
}

void Ui::NameAndStatusWidget::setNameProperty(const char* _propName, bool _val)
{
    name_->setProperty(_propName, _val);
	name_->style()->unpolish(name_);
	name_->style()->polish(name_);
}

void Ui::NameAndStatusWidget::setStatusProperty(const char* _propName, bool _val)
{
    status_->setProperty(_propName, _val);
	status_->style()->unpolish(status_);
	status_->style()->polish(status_);
}

void Ui::NameAndStatusWidget::setName(const char* _name)
{
    assert(!!_name);
    if (_name && name_)
    {
        name_->setText(_name);
    }
}

void Ui::NameAndStatusWidget::setStatus(const char* _status)
{
    assert(!!_status);
    if (_status && status_)
    {
        status_->setText(_status);
    }
}

//----

Ui::NameWidget::NameWidget(QWidget* _parent, int _nameBaseline)
    : QWidget(_parent)
    , name_(NULL)
{
    setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignLeft);
    setLayout(rootLayout);
    
    rootLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    
    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);
    
    name_ = new voipTools::BoundBox<ShadowedWidget<TextEmojiLabel> >(this);
    name_->setFont(font);
    name_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    name_->setSizeToBaseline(_nameBaseline); // SORRY, MANKIND!!! THIS IS NECCESSARY EVIL BECAUSE TextEmojiLabel IS TERRIBLE!!!
    layout()->addWidget(name_);
    
    rootLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

Ui::NameWidget::~NameWidget()
{
    
}

void Ui::NameWidget::setNameProperty(const char* _propName, bool _val)
{
    name_->setProperty(_propName, _val);
}

void Ui::NameWidget::setName(const char* _name)
{
    assert(!!_name);
    if (_name && name_)
    {
        name_->setText(_name);
    }
}
