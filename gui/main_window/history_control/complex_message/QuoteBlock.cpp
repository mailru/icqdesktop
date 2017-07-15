#include "stdafx.h"

#include "ComplexMessageItem.h"
#include "../MessageStyle.h"
#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../controls/PictureWidget.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../my_info.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../contextMenuEvent.h"
#include "../MessagesModel.h"

#include "QuoteBlockLayout.h"
#include "Selection.h"
#include "Style.h"
#include "QuoteBlock.h"
#include "TextBlock.h"
#include "../../../gui/controls/TextEditEx.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlockHoverPainter::QuoteBlockHoverPainter(QWidget* parent) :
	QWidget(parent),
	Opacity_(0.0)
{
	QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect(this);
	Opacity_ = 0.0;
	effect->setOpacity(0.0);
	setGraphicsEffect(effect);
}

void QuoteBlockHoverPainter::paintEvent(QPaintEvent * e)
{
	QPainter p(this);

	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);

	QPen pen(Qt::NoPen);
	p.setPen(pen);

	QBrush b(QColor(0, 0, 0, 25));
	p.setBrush(b);

	int radius = Utils::scale_value(8);
	p.drawRoundedRect(rect(), radius, radius);
}

void QuoteBlockHoverPainter::startAnimation(qreal begin, qreal end_opacity)
{
	QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(eff);

	QPropertyAnimation *a = new QPropertyAnimation(eff,"opacity");

	int duration = abs(begin - end_opacity) * 20;
	if (duration < 5)
		duration = 5;

	a->setDuration(duration);
	a->setStartValue(begin);
	a->setEndValue(end_opacity);
	a->setEasingCurve(QEasingCurve::InBack);
	a->start(QPropertyAnimation::DeleteWhenStopped);

	connect(eff, &QGraphicsOpacityEffect::opacityChanged, this, &QuoteBlockHoverPainter::onOpacityChanged);
}

void QuoteBlockHoverPainter::startAnimation(qreal end_opacity)
{
    if (fabs(Opacity_ - end_opacity) > 0.02)
	    startAnimation(Opacity_, end_opacity);
}

void QuoteBlockHoverPainter::onOpacityChanged(qreal o)
{
	Opacity_ = o;
}

/////////////////////////////////////////////////////////////////////
QuoteBlockHover::QuoteBlockHover(QuoteBlockHoverPainter* painter, QWidget* parent, QuoteBlock* block) :
	QWidget(parent),
	Painter_(painter),
	Block_(block),
    Text_(nullptr),
    bAnchorClicked_(false)
{
    setMouseTracking(true);
}

bool QuoteBlockHover::eventFilter(QObject * obj, QEvent * e)
{
    if (e->type() == QEvent::MouseMove ||
        e->type() == QEvent::Leave ||
        e->type() == ContextMenuCreateEvent::type() ||
        e->type() == ContextMenuDestroyEvent::type())
    {
        auto pos = this->mapFromGlobal(QCursor::pos());
        bool bInside = rect().contains(pos);

        if (bInside)
            Painter_->startAnimation(1.0);
        else
            Painter_->startAnimation(0.0);
    }
    return false;
}

void QuoteBlockHover::mouseMoveEvent(QMouseEvent * e)
{
    e->ignore();
}

void QuoteBlockHover::mousePressEvent(QMouseEvent * e)
{
    bAnchorClicked_ = false;

    if (Text_)
    {
        auto pos = Text_->mapFromGlobal(e->globalPos());
        QMouseEvent ev = QMouseEvent(QEvent::MouseButtonPress, pos, e->button(), e->buttons(), e->modifiers());
        Text_->mousePress(&ev);
    }

	if ((e->buttons() & Qt::RightButton) ||
		(e->button() == Qt::RightButton))
	{
		/// context menu
		const auto globalPos = e->globalPos();
		emit contextMenu(globalPos);

        e->accept();
	}
    else
        e->ignore();
}

void QuoteBlockHover::mouseReleaseEvent(QMouseEvent * e)
{
    if (Text_)
    {
        auto pos = Text_->mapFromGlobal(e->globalPos());
        QMouseEvent ev = QMouseEvent(QEvent::MouseButtonRelease, pos, e->button(), e->buttons(), e->modifiers());
        Text_->mouseRelease(&ev);
    }

	/// press when below avatar_name
	int max_y = Utils::scale_value(0); //Change to 35 to create an AvatarClick action (by s.skakov)
    if (!Block_->isSelected() && !bAnchorClicked_)
    {
	    if ((e->buttons() & Qt::LeftButton) || 
		    (e->button() == Qt::LeftButton))
	    {
		    if (e->localPos().y() > max_y)
		    {
			    /// press to message
			    emit openMessage();
		    }
		    else
		    {
			    /// press to avatar zone
			    emit openAvatar();
		    }
		    e->accept();
	    }
        else
            e->ignore();
    }
    else
        e->ignore();

    bAnchorClicked_ = false;
}

void Ui::ComplexMessage::QuoteBlockHover::onLeave()
{
    Painter_->startAnimation(0.0);
}

void Ui::ComplexMessage::QuoteBlockHover::onAnchorClicked(const QUrl&)
{
    bAnchorClicked_ = true;
}

void Ui::ComplexMessage::QuoteBlockHover::onSetTextEditEx(Ui::TextEditEx* text)
{
    Text_ = text;
    text->installEventFilter(this);
    connect(text, &Ui::TextEditEx::anchorClicked, this, &QuoteBlockHover::onAnchorClicked);
}

void Ui::ComplexMessage::QuoteBlockHover::onEventFilterRequest(QWidget* w)
{
    w->installEventFilter(this);
    w->setMouseTracking(true);
}

/////////////////////////////////////////////////////////////////////
QuoteBlock::QuoteBlock(ComplexMessageItem *parent, const Data::Quote& quote)
    : GenericBlock(parent, quote.senderFriendly_, MenuFlagNone, false)
    , Quote_(quote)
    , Layout_(nullptr)
    , TextCtrl_(nullptr)
    , ForwardLabel_(nullptr)
    , Selection_(BlockSelectionType::None)
    , Parent_(parent)
    , ReplyBlock_(nullptr)
    , ForwardIcon_(nullptr)
	, QuoteHover_(nullptr)
	, QuoteHoverPainter_(nullptr)
	, MessagesCount_(0)
	, MessageIndex_(0)
{
    Layout_ = new QuoteBlockLayout();
    setLayout(Layout_);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    ForwardLabel_ = new QLabel(this);
    ForwardLabel_->setFont(Fonts::appFontScaled(15));
    QPalette p;
    p.setColor(QPalette::Foreground, QColor("#579e1c"));
    ForwardLabel_->setPalette(p);
    ForwardLabel_->setText(QT_TRANSLATE_NOOP("chat_page", "forwarded messages"));
    ForwardLabel_->adjustSize();
    ForwardLabel_->setVisible(needForwardBlock());
    ForwardIcon_ = new PictureWidget(this, ":/resources/content_forwardmsg_100.png");
    ForwardIcon_->setVisible(needForwardBlock());
    ForwardIcon_->setFixedSize(Utils::scale_value(12), Utils::scale_value(12));

    connect(this, &QuoteBlock::observeToSize, parent, &ComplexMessageItem::onObserveToSize);
}

QuoteBlock::~QuoteBlock()
{

}

void QuoteBlock::clearSelection()
{
    for (auto b : Blocks_)
    {
        b->clearSelection();
    }
    update();
}

IItemBlockLayout* QuoteBlock::getBlockLayout() const
{
    return Layout_;
}

QString QuoteBlock::getSelectedText(bool isFullSelect) const
{
    QString result;
    for (auto b : Blocks_)
    {
        if (!b->isSelected())
            continue;

        if (isFullSelect)
            result += QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));

        QString source = b->getSelectedText();
        result += source;
        result += QChar::LineFeed;
    }

    return result;
}

QString QuoteBlock::getSourceText() const
{
    QString result;
    int i = 0;
    for (auto b : Blocks_)
    {
        ++i;
        result += QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));
        QString source = b->getSourceText();
        result += source;
        result += QChar::LineFeed;
    }
    
    return result;
}

QString QuoteBlock::formatRecentsText() const
{
    if (standaloneText())
        return ReplyBlock_ ? ReplyBlock_->formatRecentsText() : QString();

    QString result;
    for (auto b : Blocks_)
    {
        auto format = QString("> %1 (%2): ").arg(Quote_.senderFriendly_, QDateTime::fromTime_t(Quote_.time_).toString("dd.MM.yyyy hh:mm"));
        format += b->getSourceText();
        result += format;
        result += QChar::LineFeed;
    }
    return result;
}

bool QuoteBlock::standaloneText() const
{
    return !quoteOnly() && !Quote_.isForward_;
}

Data::Quote QuoteBlock::getQuote() const
{
    Data::Quote quote;
    if (quoteOnly() || Quote_.isForward_)
        quote = Quote_;
    else
        quote.id_ = Quote_.id_;

   return quote;
}

IItemBlock* QuoteBlock::findBlockUnder(const QPoint &pos) const
{
    for (auto block : Blocks_)
    {
        assert(block);
        if (!block)
        {
            continue;
        }

        const auto blockLayout = block->getBlockLayout();
        assert(blockLayout);

        if (!blockLayout)
        {
            continue;
        }

        const auto &blockGeometry = blockLayout->getBlockGeometry();

        const auto topY = blockGeometry.top();
        const auto bottomY = blockGeometry.bottom();
        const auto posY = pos.y();

        const auto isPosOverBlock = ((posY >= topY) && (posY <= bottomY));
        if (isPosOverBlock)
        {
            return block;
        }
    }

    return nullptr;
}

bool QuoteBlock::isSelected() const
{
    for (auto b : Blocks_)
    {
        if (b->isSelected() && !b->getSelectedText().isEmpty())
            return true;
    }
    
    return false;
}

void QuoteBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    for (auto b : Blocks_)
    {
        b->selectByPos(from, to, selection);
    }
}

bool QuoteBlock::needForwardBlock() const
{
    return Quote_.isForward_ && Quote_.isFirstQuote_;
}

void QuoteBlock::setReplyBlock(GenericBlock* block)
{
    if (!ReplyBlock_)
        ReplyBlock_ = block;
}

void QuoteBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onVisibilityChanged(isVisible);
    }
}

void QuoteBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    GenericBlock::onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    for (auto block : Blocks_)
    {
        block->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    }
}

QRect QuoteBlock::setBlockGeometry(const QRect &ltr)
{
    QRect b = ltr;

    b.moveLeft(b.x() + Style::Quote::getQuoteOffsetLeft());
    QRect r = b;
    b.moveTop(b.y() + Style::Quote::getQuoteOffsetTop());
    b.moveTopLeft(GenericBlock::setBlockGeometry(b).bottomLeft());

    if (needForwardBlock())
       b.moveTop(b.y() + ForwardLabel_->height() + Style::Quote::getForwardLabelBottomMargin());

    int i = 1;
    for (auto block : Blocks_)
    {
        block->setMaxPreviewWidth(Style::Quote::getMaxImageWidthInQuote());
        auto blockGeometry = block->setBlockGeometry(b);
        auto bl = QPoint(
            blockGeometry.bottomLeft().x(),
            blockGeometry.bottomLeft().y() + Style::Quote::getQuoteBlockSpacing());
        b.moveTopLeft(bl);
        r.setBottomLeft(i == Blocks_.size() ? blockGeometry.bottomLeft() : bl);
        ++i;
    }
    r.moveLeft(r.x() - Style::Quote::getQuoteOffsetLeft());
    r.moveLeft(r.x() - Style::Quote::getForwardIconOffset());

    if (!quoteOnly())
    {
        if (Quote_.isLastQuote_)
            r.setHeight(r.height() + Style::Quote::getQuoteOffsetBottom());
    }

    r.setHeight(r.height() - Style::Quote::getQuoteSpacing());

	int offset = 0;
	int offset_hover = 6;
	if (MessagesCount_ > 1)
	{
		if (ReplyBlock_)
		{
			if (MessagesCount_ == MessageIndex_ + 2)
				offset = 0;
			else
			{
				offset = 4;
				offset_hover = -4;
			}
		}
		else
		{
			if (MessagesCount_ == MessageIndex_ + 1)
			{
				/// last
				offset = 15;
			}
			else
			{
				offset_hover = 6;
				offset = 4;
			}
		}
	}
	else if (!ReplyBlock_)
		offset = 15;

	if (QuoteHover_ && QuoteHoverPainter_)
	{
		auto rect = QRect(ltr.left() + Utils::scale_value(6), 
			ltr.top() + Utils::scale_value(4), 
			ltr.right() - ltr.left(), 
			quoteOnly() ? r.height() - Utils::scale_value(6) + Style::Quote::getQuoteOffsetBottom() : r.height() - Utils::scale_value(offset_hover));

		QuoteHover_->setGeometry(rect);
		QuoteHoverPainter_->setGeometry(rect);
	}

	QRect ret(
		r.left(),
		r.top(),
		r.width(),
		r.height() + Utils::scale_value(offset));

	QRect set_rect(
		r.left(),
		r.top(),
		r.width(),
		r.height() + Utils::scale_value(offset+4));

	Geometry_ = set_rect;
	setGeometry(set_rect);

    return ret;
}

void QuoteBlock::onActivityChanged(const bool isVisible)
{
    GenericBlock::onActivityChanged(isVisible);
    for (auto block : Blocks_)
    {
        block->onActivityChanged(isVisible);
    }
}

void QuoteBlock::addBlock(GenericBlock* block)
{
    block->setBubbleRequired(false);
    block->setFontSize(Style::Quote::getQuoteFont().pixelSize());
    block->setTextOpacity(0.7);
    Blocks_.push_back(block);

    /// connect(block, SIGNAL(clicked()), this, SLOT(blockClicked()), Qt::QueuedConnection);
}

bool QuoteBlock::quoteOnly() const
{
    return !ReplyBlock_;
}

void QuoteBlock::blockClicked()
{
    if (!Quote_.isForward_)
    {
        Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), Quote_.msgId_, true,
			false, nullptr, Quote_.msgId_);

        if (Quote_.msgId_ > 0)
            Logic::GetMessagesModel()->emitQuote(Quote_.msgId_);
    }
}

void QuoteBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color)
{
    auto pen = Style::Quote::getQuoteSeparatorPen();
    p.save();
    p.setPen(pen);
    auto end = rect().bottomLeft();
    end.setX(end.x() + Style::Quote::getLineOffset());
    if (Quote_.isLastQuote_)
	{
		end.setY(end.y() - Style::Quote::getQuoteOffsetBottom() - Utils::scale_value(4));
	}
    
    auto begin = rect().topLeft();
    begin.setX(begin.x() + Style::Quote::getLineOffset());
    if (Quote_.isFirstQuote_)
    {
        begin.setY(begin.y() + Style::Quote::getFirstQuoteOffset());
        if (needForwardBlock())
          begin.setY(begin.y() + ForwardLabel_->height() + Style::Quote::getForwardLabelBottomMargin());
    }

    p.drawLine(begin, end);

	p.restore();
}

void QuoteBlock::initialize()
{
    GenericBlock::initialize();

    TextCtrl_ = new TextEmojiWidget(this, Fonts::appFontScaled(12), QColor("#579e1c"));

    Avatar_ = new ContactAvatarWidget(this, Quote_.senderId_, Quote_.senderFriendly_, Utils::scale_value(20), true);
    
    TextCtrl_->setText(Quote_.senderFriendly_);
    TextCtrl_->show();
    Avatar_->show();
}

bool QuoteBlock::replaceBlockWithSourceText(IItemBlock *block)
{
    auto iter = std::find(Blocks_.begin(), Blocks_.end(), block);
    
    if (iter == Blocks_.end())
    {
        return false;
    }
    
    auto &existingBlock = *iter;
    assert(existingBlock);
    
    auto textBlock = new TextBlock(Parent_, existingBlock->getSourceText());
    emit observeToSize();
    
    textBlock->onVisibilityChanged(true);
    textBlock->onActivityChanged(true);
    
    textBlock->show();
    
    connect(textBlock, SIGNAL(clicked()), this, SLOT(blockClicked()), Qt::QueuedConnection);

    existingBlock->deleteLater();
    existingBlock = textBlock;

    textBlock->connectToHover(QuoteHover_);
    
    return true;
}

bool QuoteBlock::isSharingEnabled() const
{
    return false;
}

bool QuoteBlock::containSharingBlock() const
{
    for (auto b : Blocks_)
    {
        if (b->isSharingEnabled())
            return true;
    }

    return false;
}

void QuoteBlock::createQuoteHover(ComplexMessage::ComplexMessageItem* complex_item)
{
	if (!needForwardBlock())
	{
		QuoteHoverPainter_ = new QuoteBlockHoverPainter((QWidget*)parent());
		QuoteHoverPainter_->lower();

		QuoteHover_ = new QuoteBlockHover(QuoteHoverPainter_, (QWidget*)parent(), this);
		QuoteHover_->raise();

		connect(QuoteHover_, &QuoteBlockHover::openMessage, this, &QuoteBlock::blockClicked);
		connect(QuoteHover_, &QuoteBlockHover::contextMenu, complex_item, &ComplexMessageItem::trackMenu);
        connect(complex_item, &ComplexMessage::ComplexMessageItem::eventFilterRequest, QuoteHover_, &QuoteBlockHover::onEventFilterRequest);

        for (auto& val : Blocks_)
        {
            val->connectToHover(QuoteHover_);
        }

        Parent_->installEventFilter(QuoteHover_);
        QuoteHover_->setCursor(Qt::PointingHandCursor);

        connect(complex_item, &ComplexMessage::ComplexMessageItem::setTextEditEx, QuoteHover_, &QuoteBlockHover::onSetTextEditEx);
        connect(complex_item, &ComplexMessage::ComplexMessageItem::leave, QuoteHover_, &QuoteBlockHover::onLeave);
	}
}

void QuoteBlock::setMessagesCountAndIndex(int count, int index)
{
	MessagesCount_ = count;
	MessageIndex_ = index;
}

UI_COMPLEX_MESSAGE_NS_END
