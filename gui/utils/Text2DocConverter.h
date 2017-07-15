#pragma once

#include "../cache/emoji/Emoji.h"

namespace Ui
{
	class TextEditEx;
}

namespace Logic
{
	typedef std::function<void(const QString &uri, const int startPos)> Text2HtmlUriCallback;
	typedef std::map<QString, QString>	ResourceMap;

	enum class Text2DocHtmlMode
	{
		Invalid,
		Min,

		Escape = Min,
		Pass,

		Max = Pass
	};

	void FormatDocument(QTextDocument &doc, const int lineHeight);

	void Text4Edit(
        const QString& _text,
		Ui::TextEditEx& _edit,
		const Text2DocHtmlMode _htmlMode = Text2DocHtmlMode::Escape,
		const bool _convertLinks = true,
        const bool _breakDocument = false,
		const Text2HtmlUriCallback _uriCallback = nullptr,
		const Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto,
        const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBaseline);

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize);
    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, QTextCursor& _cursor, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize);
    
    void Text4EditEmoji(const QString& text, Ui::TextEditEx& _edit, Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto, const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBaseline);

	void Text2Doc(
		const QString &text,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode = Text2DocHtmlMode::Escape,
		const bool convertLinks = true,
		const Text2HtmlUriCallback uriCallback = nullptr,
        const Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto);

    void CutText(
        const QString &text,
        const QString &_term,
        const int _width,
        QFontMetrics _base_text_metrics,
        QFontMetrics _term_metrics,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode,
		const bool convertLinks,
		const Text2HtmlUriCallback uriCallback,
        QString& leftPart,
        QString& rightPart,
        QString& termPart);

	ResourceMap InsertEmoji(int _main, int _ext, Ui::TextEditEx& _edit);
}
