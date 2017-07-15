#pragma once

#include "../fonts.h"
#include "../utils/utils.h"

#include "LabelEx.h"

namespace Ui
{
	class TewLex;
	struct paint_info;
    class LabelEx;

	typedef std::list<std::shared_ptr<TewLex>> TewTexText;

	class CompiledText
	{
        TewTexText	lexs_;

		int				width_;
        int				height_;
        int             kerning_;

	public:
		CompiledText();

		void push_back(std::shared_ptr<TewLex> _lex);
		int draw(QPainter& _painter, int _x, int _y, int _w);
        int draw(QPainter& _painter, int _x, int _y, int _w, int _h);
		int width(const QFontMetrics& _painter);
        int height(const QFontMetrics& _painter);
        void setKerning(int _kerning) { kerning_ = _kerning; }

        static bool compileText(const QString& _text, CompiledText& _result, bool _multiline, bool _ellipsis);
	};

	enum TextEmojiAlign
	{
		allign_left,
		allign_right,
		allign_center
	};

    class TextEmojiWidgetEvents;
	class TextEmojiWidget : public QWidget
	{
		Q_OBJECT

	protected:
		QFont							font_;
		QColor							color_;
		QString							text_;
		TextEmojiAlign					align_;
		std::unique_ptr<CompiledText>	compiledText_;
        QString                         sourceText_;
		int								sizeToBaseline_;
		int								descent_;
        bool                            fading_;
        bool                            ellipsis_;
        bool                            multiline_;
        bool                            selectable_;

        QPoint                          selection_;

		virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent* _e) override;

        virtual void mousePressEvent(QMouseEvent *_e) override;
        virtual void mouseDoubleClickEvent(QMouseEvent *_e) override;
        virtual void keyPressEvent(QKeyEvent *_e) override;
        virtual void focusOutEvent(QFocusEvent *_e) override;

    Q_SIGNALS:
        void clicked();
        void rightClicked();
        void setSize(int, int);

    public:
        TextEmojiWidget(QWidget* _parent, const QFont& _font, const QColor& _color, int _sizeToBaseline = -1);
		virtual ~TextEmojiWidget();

		int ascent();
		int descent();

		void setText(const QString& _text, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        void setText(const QString& _text, const QColor& _color, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        inline QString text() const { return text_; }

        void setColor(const QColor& _color);

        void setFading(bool _v);
        void setEllipsis(bool _v);
        void setMultiline(bool _v);
        void setSelectable(bool _v);

        void setSizeToBaseline(int v);

        void setSizePolicy(QSizePolicy::Policy _hor, QSizePolicy::Policy _ver);

        void setSourceText(const QString& source);

        int getCompiledWidth() const;

    private:
        static TextEmojiWidgetEvents& events();
	};

    class TextEmojiWidgetEvents : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void selected(TextEmojiWidget*);

    private:
        TextEmojiWidgetEvents() { ; }
        TextEmojiWidgetEvents(const TextEmojiWidgetEvents&);
        TextEmojiWidgetEvents(TextEmojiWidgetEvents&&);

        friend class TextEmojiWidget;
    };

    //

    class TextEmojiLabel : public LabelEx
    {
    private:
        std::unique_ptr< CompiledText >    compiledText_;
        int                                 sizeToBaseline_;
        int                                 ascent_;
        int                                 descent_;
        int                                 leftOffset_;
        int                                 kerning_;

    protected:
        void internalDraw(QPainter& _painter, const QRect& _rc);
        int internalWidth(QPainter& _painter);
        void paintEvent(QPaintEvent* _event) override;

    public:
        TextEmojiLabel(QWidget* _parent = nullptr);
        ~TextEmojiLabel();

        void setSizeToBaseline(int _size);
        void setText(const QString& _text);
        void setSizeToOffset(int _size);
        void setKerning(int _kerning);

        int ascent() const
        {
            return ascent_;
        }
        int descent() const
        {
            return descent_;
        }
    };
    
}


