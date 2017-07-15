#include "stdafx.h"
#include "TextEmojiWidget.h"
#include "../utils/SChar.h"
#include "../cache/emoji/Emoji.h"

#include <thread>

namespace Ui
{
	class TewLex
	{
	protected:
		int		width_;
		int     height_;

	public:
        TewLex() : width_(-1), height_(-1) {}

		virtual int draw(QPainter& _painter, int _x, int _y) = 0;
		virtual int width(const QFontMetrics& _fontMetrics) = 0;
		virtual int height(const QFontMetrics& _fontMetrics) = 0;

		virtual bool isSpace() const = 0;
		virtual bool isEol() const = 0;
	};

	class TewLexText : public TewLex
	{
		QString		text_;

		virtual int draw(QPainter& _painter, int _x, int _y) override
		{
            auto fm = _painter.fontMetrics();
			_painter.fillRect(_x, _y - height(fm) + 1, width(fm) + 1, height(fm) + 1, _painter.brush());
			_painter.drawText(_x, _y, text_);
			return width(fm);
		}

		virtual int width(const QFontMetrics& _fontMetrics) override
		{
			if (width_ == -1)
				width_ = _fontMetrics.width(text_);
			return width_;
		}

		virtual int height(const QFontMetrics& _fontMetrics) override
		{
			if (height_ == -1)
				height_ = _fontMetrics.height();
			return height_;
		}

		virtual bool isSpace() const override
		{
			return text_ == " ";
		}

		virtual bool isEol() const override
		{
			return text_ == "\n";
		}

	public:

        TewLexText()
		{
			text_.reserve(50);
		}

		void append(const QString& _text)
		{
			text_.append(_text);
		}
	};

	class TewLexEmoji : public TewLex
	{
		int			mainCode_;
		int			extCode_;

		QImage		image_;

		const QImage& getImage(const QFontMetrics& _m)
		{
			if (image_.isNull())
			{
				image_ = Emoji::GetEmoji(mainCode_, extCode_, Emoji::GetFirstLesserOrEqualSizeAvailable(_m.ascent() - _m.descent()));
			}
			return image_;
		}

		virtual int draw(QPainter& _painter, int _x, int _y) override
		{
			int imageHeight = width(_painter.fontMetrics());

			QRect drawRect(_x, _y - imageHeight, imageHeight, imageHeight);

			QFontMetrics m(_painter.font());

			_painter.fillRect(drawRect, _painter.brush());
			_painter.drawImage(drawRect, getImage(m));

			return imageHeight;
		}

		virtual int width(const QFontMetrics& _fontMetrics) override
		{
			if (width_ == -1)
			{
				int imageMaxHeight = _fontMetrics.ascent();
				int imageHeight = getImage(_fontMetrics).height();
				if (imageHeight > imageMaxHeight)
                    imageHeight = imageMaxHeight;
				width_ = imageHeight;
			}
			return width_;
		}

		virtual int height(const QFontMetrics& _fontMetrics) override
		{
			if (height_ == -1)
			{
				int imageMaxHeight = _fontMetrics.ascent();
				int imageHeight = getImage(_fontMetrics).height();
				if (imageHeight > imageMaxHeight)
                    imageHeight = imageMaxHeight;
				height_ = imageHeight;
			}
			return height_;
		}

		virtual bool isSpace() const override
		{
			return false;
		}

		virtual bool isEol() const override
		{
			return false;
		}

	public:

        TewLexEmoji(int _mainCode, int _extCode)
			:	mainCode_(_mainCode),
			extCode_(_extCode)
		{
		}
	};

    CompiledText::CompiledText()
		:	width_(-1), height_(-1), kerning_(0)
	{
	}

	int CompiledText::width(const QFontMetrics& _painter)
	{
		if (width_ == -1)
        {
			for (auto lex : lexs_)
            {
				width_ += lex->width(_painter);
            }
            width_ += lexs_.empty() ? 0 : (lexs_.size() - 1) * kerning_;
        }
		return width_ + 1;
	}

	int CompiledText::height(const QFontMetrics& _painter)
	{
		if (height_ == -1)
			for (auto lex : lexs_)
				height_ = std::max(height_, lex->height(_painter));
		return height_ + 1;
	}

	void CompiledText::push_back(std::shared_ptr<TewLex> _lex)
	{
		lexs_.push_back(_lex);
	}

	int CompiledText::draw(QPainter& _painter, int _x, int _y, int _w)
	{
        const static std::string ellipsis = "...";
		auto xmax = (_w + _x);
		if (_w > 0)
		{
            xmax -= ((_painter.fontMetrics().averageCharWidth() + kerning_) * ellipsis.length());
		}
		int i = 0;
		for (auto lex : lexs_)
		{
			_x += lex->draw(_painter, _x, _y) + kerning_;
			if (xmax > 0 && _x >= xmax && i < ((int)lexs_.size() - 1))
			{
				_painter.drawText(_x, _y, ellipsis.c_str());
				break;
			}
			++i;
		}
		return _x;
	}

	int CompiledText::draw(QPainter& _painter, int _x, int _y, int _w, int _dh)
	{
		auto h = _y + _dh;
		for (auto lex : lexs_)
		{
			auto lexw = lex->width(_painter.fontMetrics());
			if ((_x + lexw) >= _w || lex->isEol())
			{
				_x = 0;
				_y += height(_painter.fontMetrics());
			}
			h = _y + _dh;
			if (!(lex->isSpace() && _x == 0))
				_x += lex->draw(_painter, _x, _y) + kerning_;
		}
		return h;
	}

	bool CompiledText::compileText(const QString& _text, CompiledText& _result, bool _multiline, bool _ellipsis)
	{
		QTextStream inputStream(const_cast<QString*>(&_text), QIODevice::ReadOnly);

		std::shared_ptr<TewLexText> lastTextLex;

		auto is_eos = [&](int _offset)
		{
			assert(_offset >= 0);
			assert(inputStream.string());

			const auto &text = *inputStream.string();
			return ((inputStream.pos() + _offset) >= text.length());
		};

		auto peekSchar = [&]
		{
			return Utils::PeekNextSuperChar(inputStream);
		};

		auto readSchar = [&]
		{
			return Utils::ReadNextSuperChar(inputStream);
		};

		auto convertPlainCharacter = [&]
		{
			if (!lastTextLex)
			{
                lastTextLex = std::make_shared<TewLexText>();
				_result.push_back(lastTextLex);
			}
            lastTextLex->append(readSchar().ToQString());
            if (_ellipsis)
            {
                lastTextLex.reset();
            }
		};

		auto convertEolSpace = [&]
		{
			const auto ch = peekSchar();
			if (ch.IsNull())
			{
                readSchar();
                lastTextLex.reset();
				return true;
			}
			if (ch.IsNewline() || ch.IsSpace())
			{
                readSchar();
                lastTextLex.reset();
                lastTextLex = std::make_shared<TewLexText>();
				_result.push_back(lastTextLex);
                lastTextLex->append(ch.ToQString());
                lastTextLex.reset();
				return true;
			}
			return false;
		};

		auto convertEmoji = [&]()
		{
			const auto ch = peekSchar();
			if (ch.IsEmoji())
			{
#ifdef __APPLE__
                lastTextLex.reset();
#else
                readSchar();
				_result.push_back(std::make_shared<TewLexEmoji>(ch.Main(), ch.Ext()));
                lastTextLex.reset();
                return true;
#endif
			}
			return false;
		};

		while (!is_eos(0))
		{
			if (_multiline && convertEolSpace())
				continue;

			if (convertEmoji())
				continue;

			convertPlainCharacter();
		}

		return true;
	}

	TextEmojiWidgetEvents& TextEmojiWidget::events()
	{
		static TextEmojiWidgetEvents e;
		return e;
	}

    TextEmojiWidget::TextEmojiWidget(QWidget* _parent, const QFont& _font, const QColor& _color, int _sizeToBaseline)
		:	QWidget(_parent),
		align_(TextEmojiAlign::allign_left),
		color_(_color),
		sizeToBaseline_(_sizeToBaseline),
		fading_(false),
        ellipsis_(false),
		multiline_(false),
		selectable_(false)
	{
        font_ = _font;

        setFont(font_);

		QFontMetrics metrics = QFontMetrics(font_);

		int ascent = metrics.ascent();
		descent_ = metrics.descent();

		int height = metrics.height();

		if (_sizeToBaseline != -1)
		{
			if (_sizeToBaseline > ascent)
			{
				height = sizeToBaseline_ + descent_;
			}
		}
		else
		{
            sizeToBaseline_ = ascent;
		}

		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setFixedHeight(height);

		setFocusPolicy(Qt::StrongFocus);
	}

	int TextEmojiWidget::ascent()
	{
		return sizeToBaseline_;
	}

	int TextEmojiWidget::descent()
	{
		return descent_;
	}

    void TextEmojiWidget::setSizeToBaseline(int _v)
    {
        sizeToBaseline_ = _v;
        auto metrics = QFontMetrics(font_);
        auto ascent = metrics.ascent();
        auto height = metrics.height();
        if (_v > ascent)
        {
            height = sizeToBaseline_ + descent_;
        }
        setFixedHeight(height);
    }

    TextEmojiWidget::~TextEmojiWidget(void)
	{
	}

	void TextEmojiWidget::setText(const QString& _text, TextEmojiAlign _align)
	{
		align_ = _align;
		text_ = _text;
		compiledText_.reset(new CompiledText());
		if (!CompiledText::compileText(text_, *compiledText_, multiline_, ellipsis_))
			assert(!"TextEmojiWidget: couldn't compile text.");
        QWidget::update();
	}

    void TextEmojiWidget::setText(const QString& _text, const QColor& _color, TextEmojiAlign _align)
    {
        color_ = _color;
        setText(_text, _align);
    }

    void TextEmojiWidget::setColor(const QColor &_color)
    {
        color_ = _color;
        setText(text_);
    }

	void TextEmojiWidget::setFading(bool _v)
	{
        if (fading_ != _v)
        {
            if (_v)
            {
                ellipsis_ = false;
            }
            fading_ = _v;
            setText(text_);
        }
	}

    void TextEmojiWidget::setEllipsis(bool _v)
    {
        if (ellipsis_ != _v)
        {
            if (_v)
            {
                fading_ = false;
            }
            ellipsis_ = _v;
            setText(text_);
        }
    }

	void TextEmojiWidget::setMultiline(bool _v)
	{
		if (multiline_ != _v)
		{
			if (_v)
            {
				fading_ = false;
                ellipsis_ = false;
            }
			multiline_ = _v;
			setText(text_);
		}
	}

	void TextEmojiWidget::setSelectable(bool _v)
	{
		if (selectable_ != _v)
		{
            static QMetaObject::Connection con;
			if (_v)
			{
				con = connect(&events(), &TextEmojiWidgetEvents::selected, this, [this](TextEmojiWidget* _obj)
				{
					if ((TextEmojiWidget*)this != _obj)
					{
						selection_ = QPoint();
                        QWidget::update();
					}
				});
			}
			else if (con)
			{
                disconnect(con);
			}
			selectable_ = _v;
			selection_ = QPoint();
            QWidget::update();
		}
	}

	void TextEmojiWidget::setSizePolicy(QSizePolicy::Policy _hor, QSizePolicy::Policy _ver)
	{
		if (_hor == QSizePolicy::Preferred)
        {
            setFixedWidth(geometry().width());
            QWidget::update();
        }

		return QWidget::setSizePolicy(_hor, _ver);
	}

    void TextEmojiWidget::setSourceText(const QString& source)
    {
        sourceText_ = source;
    }

    int TextEmojiWidget::getCompiledWidth() const
    {
        return compiledText_.get() ? compiledText_->width(fontMetrics()) : width();
    }

	void TextEmojiWidget::paintEvent(QPaintEvent* _e)
	{
		QPainter painter(this);
		painter.save();
		painter.setFont(font_);
		painter.setPen(color_);
		if (selection_.y() > 0)
			painter.setBrush(QColor("#cae6b3"));

		QStyleOption opt;
		opt.init(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

		int offsetX = 0;

		int width = compiledText_->width(painter.fontMetrics());

		QRect rc = geometry();

		switch (align_)
		{
		case Ui::allign_left:
            offsetX = 0;
			break;
		case Ui::allign_right:
            offsetX = rc.width() - width;
			break;
		case Ui::allign_center:
            offsetX = (rc.width() - width)/2;
			break;
		default:
			assert(false);
			break;
		}

		int offsetY = sizeToBaseline_;

		if (fading_ || ellipsis_ || !multiline_)
		{
            int w = 0;
            if (fading_)
            {
                QLinearGradient gradient(contentsRect().topLeft(), contentsRect().topRight());
                gradient.setColorAt(0.7, color_);
                gradient.setColorAt(1.0, palette().color(QPalette::Window));
                QPen pen;
                pen.setBrush(QBrush(gradient));
                painter.setPen(pen);

                w = compiledText_->draw(painter, offsetX, offsetY, geometry().width());
            }
            else if (ellipsis_)
            {
                w = compiledText_->draw(painter, offsetX, offsetY, geometry().width());
            }
            else
            {
                w = compiledText_->draw(painter, offsetX, offsetY, -1);
            }
            if (sizePolicy().horizontalPolicy() == QSizePolicy::Preferred && w != geometry().width())
                setFixedWidth(w);
		}
		else
		{
			auto h = compiledText_->draw(painter, offsetX, offsetY, geometry().width(), descent());
			if (h != geometry().height())
            {
                QFontMetrics metrics = QFontMetrics(font_);
				setFixedHeight(std::max(metrics.height(), h));
            }
		}

		painter.restore();
		QWidget::paintEvent(_e);
	}

	void TextEmojiWidget::resizeEvent(QResizeEvent *_e)
	{
		QWidget::resizeEvent(_e);
        if (_e->oldSize().width() != -1 && _e->oldSize().height() != -1)
            emit setSize(_e->size().width() - _e->oldSize().width(), _e->size().height() - _e->oldSize().height());
	}

	void TextEmojiWidget::mousePressEvent(QMouseEvent *_e)
	{
        if (_e->button() == Qt::RightButton)
        {
            emit rightClicked();
            return QWidget::mousePressEvent(_e);
        }

		if (selectable_)
		{
			selection_ = QPoint();
            QWidget::update();
			return;
		}
        emit clicked();
		return QWidget::mousePressEvent(_e);
	}

	void TextEmojiWidget::mouseDoubleClickEvent(QMouseEvent *_e)
	{
		if (selectable_)
		{
			selection_ = QPoint(0, text_.count());
            QWidget::update();
			emit events().selected(this);
			return;
		}
		return QWidget::mouseDoubleClickEvent(_e);
	}

	void TextEmojiWidget::keyPressEvent(QKeyEvent *_e)
	{
		if (selectable_)
		{
			if (_e->matches(QKeySequence::Copy))
			{
				qApp->clipboard()->setText(sourceText_.isEmpty() ? text() : sourceText_);
				selection_ = QPoint();
                QWidget::update();
				return;
			}
		}

        return QWidget::keyPressEvent(_e);
	}

	void TextEmojiWidget::focusOutEvent(QFocusEvent *_e)
	{
		if (selectable_)
		{
			selection_ = QPoint();
            QWidget::update();
		}
		return QWidget::focusOutEvent(_e);
	}

	////

	TextEmojiLabel::TextEmojiLabel(QWidget* _parent)
        : LabelEx(_parent)
        , kerning_(0)
	{
		setSizeToBaseline(-1);
        setSizeToOffset(5);
	}

	TextEmojiLabel::~TextEmojiLabel()
	{
		//
	}

    void TextEmojiLabel::setSizeToOffset(int _size)
    {
       leftOffset_ = _size;
    }

	void TextEmojiLabel::setSizeToBaseline(int _size)
	{
		QFontMetrics metrics = QFontMetrics(QLabel::font());
		int height = metrics.height();
		ascent_ = metrics.ascent();
		descent_ = metrics.descent();

        sizeToBaseline_ = _size;
		if (_size != -1)
		{
			if (_size > ascent_)
				height = sizeToBaseline_ + descent_;
		}
		else
            sizeToBaseline_ = ascent_;

        QWidget::update();
	}

	void TextEmojiLabel::setText(const QString& _text)
	{
		QLabel::setText(_text);
        compiledText_.reset(new CompiledText());
        compiledText_->setKerning(kerning_);
		if (!CompiledText::compileText(QLabel::text(), *compiledText_, false, false))
			assert(false && "TextEmojiLabel::setText: couldn't compile text.");
	}

	void TextEmojiLabel::paintEvent(QPaintEvent* _event)
	{
		QPainter painter(this);
		internalDraw(painter, geometry());
		QWidget::paintEvent(_event);
	}

	int TextEmojiLabel::internalWidth(QPainter& _painter)
    {
		return !!compiledText_ ? compiledText_->width(_painter.fontMetrics()) + 5 : 0;
	}

	void TextEmojiLabel::internalDraw(QPainter& _painter, const QRect& _rectDraw)
    {
		if (!compiledText_)
        {
			//assert(!"error: compiled text is not initialized");
			return;
		}

		QStyleOption opt;
		opt.init(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &_painter, this);

		int width = compiledText_->width(_painter.fontMetrics());

		int offsetX = leftOffset_;//geometry().x();
		if (alignment() == Qt::AlignRight)
            offsetX = _rectDraw.width() - width;
		else if (alignment() == Qt::AlignCenter)
            offsetX = (_rectDraw.width() - width)/2;

		int offsetY = sizeToBaseline_;

        compiledText_->draw(_painter, offsetX, offsetY, -1);
	}

    void TextEmojiLabel::setKerning(int _kerning)
    {
        kerning_ = _kerning;
        if (!!compiledText_)
        {
            compiledText_->setKerning(_kerning);
        }
    }
}

