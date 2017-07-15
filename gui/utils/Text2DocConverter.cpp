#include "stdafx.h"

#include "../../common.shared/url_parser/url_parser.h"

#include "Text2DocConverter.h"
#include "../cache/emoji/Emoji.h"
#include "../controls/TextEditEx.h"
#include "../utils/log/log.h"
#include "../utils/SChar.h"

namespace
{
	using namespace Logic;

	class Text2DocConverter
	{
	public:
		Text2DocConverter();

		void Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);
        
        void ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		const ResourceMap& InsertEmoji(const int32_t main, const int32_t ext, QTextCursor& cursor);

		void MakeUniqueResources(const bool make);

		const ResourceMap& GetResources() const;

        static bool AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled);

        int GetSymbolWidth(const QString &text, int& ind, const QFontMetrics& metrics, const Text2DocHtmlMode htmlMode
        , const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment, bool _to_right);

	private:

		bool ConvertEmail();

		bool ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		bool ParseUrl(bool breakDocument);

		bool ConvertNewline();

		void ConvertPlainCharacter(const bool isWordWrapEnabled);

		bool IsEos(const int offset = 0) const;

		bool IsHtmlEscapingEnabled() const;

		bool ExtractUrl(QString &url, const bool isWordWrapEnabled);

		void FlushBuffers();

		Utils::SChar PeekSChar();

		void PushInputCursor();

		void PopInputCursor(const int steps = 1);

		Utils::SChar ReadSChar();

		QString ReadString(const int length);

		void ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		void SaveAsHtml(const QString& _text, const common::tools::url& _url, bool isWordWrapEnabled);

		void WriteEmail();

		QTextStream Input_;

		QTextCursor Writer_;

		QString LastWord_;

        QString Buffer_;

		QString UrlAccum_;

		QString TmpBuf_;

		Text2HtmlUriCallback UriCallback_;

		std::vector<int> InputCursorStack_;

		Text2DocHtmlMode HtmlMode_;

		ResourceMap		Resources_;

		bool			MakeUniqResources_;

        common::tools::url_parser parser_;
	};

	bool ExtractDomain(QTextStream &s, QString &domain);

	void ReplaceUrlSpec(const QString &url, QString &out, bool isWordWrapEnabled = false);

	const QChar CH_KEYCAP(0x20E3);

    const QString SPACE_ENTITY("<span style='white-space: pre'>&#32;</span>");
}

namespace Logic
{
	void FormatDocument(QTextDocument &doc, const int lineHeight)
	{
		assert(lineHeight > 0);

		for (auto block = doc.firstBlock(); block.isValid(); block = block.next())
		{
			QTextCursor c(block);
			auto f = c.blockFormat();
			f.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);
			c.setBlockFormat(f);
		}

		doc.setDocumentMargin(0);
	}

	void Text4Edit(
        const QString& _text,
		Ui::TextEditEx& _edit,
		const Text2DocHtmlMode _htmlMode,
		const bool _convertLinks,
        const bool _breakDocument,
		const Text2HtmlUriCallback _uriCallback,
		const Emoji::EmojiSizePx _emojiSize,
        const QTextCharFormat::VerticalAlignment _aligment)
	{
        _edit.blockSignals(true);
        _edit.document()->blockSignals(true);
        _edit.setUpdatesEnabled(false);

		Text2DocConverter converter;
		converter.MakeUniqueResources(true);

        auto cursor = _edit.textCursor();
        cursor.beginEditBlock();

		converter.Convert(_text, cursor, _htmlMode, _convertLinks, _breakDocument, _uriCallback, _emojiSize, _aligment);

        cursor.endEditBlock();

		_edit.mergeResources(converter.GetResources());

        _edit.setUpdatesEnabled(true);
        _edit.document()->blockSignals(false);
        _edit.blockSignals(false);

        emit (_edit.document()->contentsChanged());
	}

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize)
    {
        auto cur = _edit.textCursor();
        Text4Edit(_text, _edit, cur, _convertLinks, _emojiSize);
    }

    void Text4Edit(const QString& _text, Ui::TextEditEx& _edit, QTextCursor& _cursor, const bool _convertLinks, Emoji::EmojiSizePx _emojiSize)
    {
        _edit.blockSignals(true);
        _edit.document()->blockSignals(true);
        _edit.setUpdatesEnabled(false);

        Text2DocConverter converter;
        converter.MakeUniqueResources(true);

        _cursor.beginEditBlock();

        converter.Convert(_text, _cursor, Text2DocHtmlMode::Escape, _convertLinks, false, nullptr, _emojiSize, QTextCharFormat::AlignBaseline);

        _cursor.endEditBlock();

        _edit.mergeResources(converter.GetResources());

        _edit.setUpdatesEnabled(true);
        _edit.document()->blockSignals(false);
        _edit.blockSignals(false);

        emit (_edit.document()->contentsChanged());
    }
    
    void Text4EditEmoji(const QString& text, Ui::TextEditEx& _edit, Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        _edit.blockSignals(true);
        _edit.document()->blockSignals(true);
        _edit.setUpdatesEnabled(false);
        
        Text2DocConverter converter;
        converter.MakeUniqueResources(true);
        
        auto cursor = _edit.textCursor();
        cursor.beginEditBlock();
        
        converter.ConvertEmoji(text, cursor, _emojiSize, _aligment);
        
        cursor.endEditBlock();
        
        _edit.mergeResources(converter.GetResources());
        
        _edit.setUpdatesEnabled(true);
        _edit.document()->blockSignals(false);
        _edit.blockSignals(false);
        
        emit (_edit.document()->contentsChanged());
    }
    
    void Text2Doc(
        const QString &text,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode,
		const bool convertLinks,
		const Text2HtmlUriCallback uriCallback,
        const Emoji::EmojiSizePx _emojiSize)
	{
		Text2DocConverter converter;
        cursor.document()->blockSignals(true);
		converter.Convert(text, cursor, htmlMode, convertLinks, false, uriCallback, _emojiSize, QTextCharFormat::AlignBaseline);
        cursor.document()->blockSignals(false);
        emit (cursor.document()->contentsChanged());
	}

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
        QString& termPart)
	{
		Text2DocConverter converter;
        auto term_pos = text.indexOf(_term, 0, Qt::CaseInsensitive);

        auto term_in_text = text.mid(term_pos, _term.size());

        // calc part of term
        auto symb_index = term_pos;
        auto term_width = 0;
        while (term_width < _width && symb_index < term_pos + term_in_text.size())
        {
            auto width = converter.GetSymbolWidth(text, symb_index, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline, true);
            term_width += width;
        }

        termPart = text.mid(term_pos, symb_index - term_pos);
        if (term_width >= _width)
        {
            return;
        }

        // left and right part
        if (symb_index == term_in_text.size() + term_pos)
        {
            auto right_width = 0;
            auto left_width = 0;
            auto rigth_pos = term_in_text.size() + term_pos;
            auto left_pos = term_pos;
            
            while (right_width + left_width + term_width < _width)
            {
                auto copy_rigth_pos = rigth_pos;
                if (rigth_pos < text.size())
                {
                    right_width += converter.GetSymbolWidth(text, rigth_pos, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline, true);
                    if (right_width + left_width + term_width > _width)
                    {
                        rigth_pos = copy_rigth_pos;
                        break;
                    }
                }

                auto copy_left_pos = left_pos;
                if (left_pos > 0)
                {
                    left_width += converter.GetSymbolWidth(text, left_pos, _term_metrics, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline, false);
                    if (right_width + left_width + term_width > _width)
                    {
                        left_pos = copy_left_pos;
                        break;
                    }
                }

                if (left_pos == 0 && rigth_pos >= text.size())
                    break;
            }

            leftPart = text.mid(left_pos, term_pos - left_pos);
            rightPart = text.mid(term_in_text.size() + term_pos, rigth_pos - term_in_text.size() - term_pos);
        }
	}

	ResourceMap InsertEmoji(int _main, int _ext, Ui::TextEditEx& _edit)
	{
		Text2DocConverter converter;
		converter.MakeUniqueResources(true);
        QTextCursor cursor = _edit.textCursor();
		return converter.InsertEmoji(_main, _ext, cursor);
	}
}

namespace
{
    const auto WORD_WRAP_LIMIT = 15;

	Text2DocConverter::Text2DocConverter()
		: HtmlMode_(Text2DocHtmlMode::Pass)
		, MakeUniqResources_(false)
	{
		// allow downsizing without reallocation
		const auto DEFAULT_SIZE = 1024;
		UrlAccum_.reserve(DEFAULT_SIZE);
		TmpBuf_.reserve(DEFAULT_SIZE);
		LastWord_.reserve(DEFAULT_SIZE);
        Buffer_.reserve(DEFAULT_SIZE);

		// must be sufficient
		InputCursorStack_.reserve(32);
	}

	void Text2DocConverter::Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
	{
		assert(!UriCallback_);
		assert(htmlMode >= Text2DocHtmlMode::Min);
		assert(htmlMode <= Text2DocHtmlMode::Max);

		Input_.setString((QString*)&text, QIODevice::ReadOnly);
		Writer_ = cursor;
		UriCallback_ = uriCallback;
		HtmlMode_ = htmlMode;

		while (!IsEos())
		{
			InputCursorStack_.resize(0);

			if (ConvertNewline())
			{
				continue;
			}

			if (convertLinks && ParseUrl(breakDocument))
			{
				continue;
			}
            
            if (!platform::is_apple() &&
                ConvertEmoji(_emojiSize, _aligment))
			{
				continue;
			}

			ConvertPlainCharacter(breakDocument);
		}

		FlushBuffers();

		InputCursorStack_.resize(0);
		TmpBuf_.resize(0);
		LastWord_.resize(0);
        Buffer_.resize(0);
		UrlAccum_.resize(0);
		UriCallback_ = nullptr;
	}
    
    void Text2DocConverter::ConvertEmoji(const QString& text, QTextCursor &cursor, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);
        Writer_ = cursor;
        
        while (!IsEos())
        {
            InputCursorStack_.resize(0);
            if (!platform::is_apple() &&
                ConvertEmoji(_emojiSize, _aligment))
            {
                continue;
            }
            
            Buffer_ += ReadSChar().ToQString();
        }
        
        FlushBuffers();
        
        InputCursorStack_.resize(0);
        TmpBuf_.resize(0);
        LastWord_.resize(0);
        Buffer_.resize(0);
        UrlAccum_.resize(0);
    }
    
    int Text2DocConverter::GetSymbolWidth(const QString &text, int& ind, const QFontMetrics& metrics, const Text2DocHtmlMode htmlMode
        , const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback
        , const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment, bool _to_right)
    {
        Input_.setString((QString*)&text, QIODevice::ReadOnly);

        if (_to_right)
        {
            Input_.seek(ind);

            const auto ch = PeekSChar();

            if (!platform::is_apple() && ch.IsEmoji())
		    {
                ind += 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
		        return (int32_t)_emojiSize;
	        }

            auto result = metrics.width(text.mid(ind, 1));
            ind += 1;
            return result;
        }
        else
        {
            if (ind != 1)
            {
                Input_.seek(ind - 2);
                const auto ch = PeekSChar();

                if (!platform::is_apple() && ch.IsEmoji())
		        {
                    ind -= 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
		            return (int32_t)_emojiSize;
	            }
            }

            Input_.seek(ind - 1);
            const auto ch = PeekSChar();

            if (!platform::is_apple() && ch.IsEmoji())
		    {
                ind += 1 + (ch.IsTwoCharacterEmoji() ? 1 : 0);
		        return (int32_t)_emojiSize;
            }

            auto result = metrics.width(text.mid(ind, 1));
            ind -= 1;
            return result;
        }
    }

    bool Text2DocConverter::AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled)
    {
        if (platform::is_apple())
        {
            if (isWordWrapEnabled && !word.isEmpty() && (word.length() % WORD_WRAP_LIMIT == 0))
            {
                output += QChar::SoftHyphen;
                return true;
            }
            return false;
        }
        
        const auto applyHyphen = (isWordWrapEnabled && (word.length() >= WORD_WRAP_LIMIT));
        if (applyHyphen)
        {
            output += QChar::SoftHyphen;
        }
        return applyHyphen;
    }

	const ResourceMap& Text2DocConverter::InsertEmoji(int _main, int _ext, QTextCursor& _cursor)
	{
		Writer_ = _cursor;
        Utils::SChar ch(_main, _ext);

        ReplaceEmoji(ch, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline);

		return Resources_;
	}

	bool Text2DocConverter::ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
	{
		const auto ch = PeekSChar();

		if (ch.IsEmoji())
		{
			ReadSChar();
			ReplaceEmoji(ch, _emojiSize, _aligment);
			return true;
		}

		return false;
	}

	bool Text2DocConverter::ParseUrl(bool breakDocument)
	{
        if (!IsHtmlEscapingEnabled())
        {
            return false;
        }

        const bool isWordWrapEnabled = breakDocument;

        PushInputCursor();

        parser_.reset();

        QString buf;

        auto onUrlFound = [isWordWrapEnabled, this]()
        {
            PopInputCursor();
            const auto& url = parser_.get_url();
            const auto urlAsString = QString::fromUtf8(url.original_.c_str(), url.original_.size());
            const auto charsProcessed = urlAsString.size();
            if (!Input_.seek(Input_.pos() + charsProcessed))
            {
                Input_.readAll(); // end of stream
            }
            SaveAsHtml(urlAsString, url, isWordWrapEnabled);
        };

        auto getChar = [this, &onUrlFound]()
        {
            QString buf;

            QChar c1;
            Input_ >> c1;
            buf.append(c1);

            if (c1.isHighSurrogate())
            {
                assert(!Input_.atEnd());
                if (!Input_.atEnd())
                {
                    QChar c2;
                    Input_ >> c2;
                    buf.append(c2);
                }
            }

            return buf;
        };

        while (true)
        {
            if (Input_.atEnd())
            {
                parser_.finish();

                if (parser_.has_url())
                {
                    onUrlFound();
                    return true;
                }
                else
                {
                    PopInputCursor();
                    return false;
                }
            }

            const auto charAsStr = getChar();
            for (char c : charAsStr.toUtf8())
                parser_.process(c);

            if (parser_.skipping_chars())
            {
                PopInputCursor();
                return false;
            }

            if (parser_.has_url())
            {
                onUrlFound();
                return true;
            }

            buf += charAsStr;
        }
	}

	bool Text2DocConverter::ConvertNewline()
	{
        const auto ch = PeekSChar();

		const auto isNewline = (ch.IsNewline() || ch.IsCarriageReturn());
		if (!isNewline)
		{
			return false;
		}

		ReadSChar();

        auto insertNewLine = [this] ()
        {
            if (IsHtmlEscapingEnabled())
            {
                static const QString HTML_NEWLINE("<br/>");
                Buffer_ += HTML_NEWLINE;
            }
            else
            {
                static const QString TEXT_NEWLINE("\n");
                Buffer_ += TEXT_NEWLINE;
            }
        };

        insertNewLine();
        LastWord_.resize(0);

        if (!platform::is_apple() && PeekSChar().IsEmoji())
            insertNewLine();

		if (!ch.IsCarriageReturn())
		{
			return true;
		}

		const auto hasTrailingNewline = PeekSChar().IsNewline();
		if (hasTrailingNewline)
		{
			// eat a newline after the \r because we had been processed it already
			ReadSChar();
		}

		return true;
	}

	void Text2DocConverter::ConvertPlainCharacter(const bool isWordWrapEnabled)
	{
		const auto nextChar = ReadSChar();

        const auto nextCharStr = nextChar.ToQString();
        
        const auto isNextCharSpace = nextChar.IsSpace();

        const auto isEmoji = nextChar.IsEmoji();

        const auto isDelimeter = nextChar.IsDelimeter();

        if (isNextCharSpace || isDelimeter)
        {
            LastWord_.resize(0);
        }
        else
        {
            if (IsHtmlEscapingEnabled())
            {
                const auto htmlEscaped = nextCharStr.toHtmlEscaped();

                LastWord_ += htmlEscaped;
            }
            else
            {
                LastWord_ += nextCharStr;
            }

            if (!isEmoji && !platform::is_apple())
            {
                AddSoftHyphenIfNeed(LastWord_, LastWord_, isWordWrapEnabled);
            }
        }

		if (IsHtmlEscapingEnabled())
		{
			if (isNextCharSpace)
			{
                Buffer_ += SPACE_ENTITY;
			}
			else
			{
				const auto htmlEscaped = nextCharStr.toHtmlEscaped();
				Buffer_ += htmlEscaped;
			}
		}
		else
		{
            Buffer_ += nextCharStr;
        }

        if (!isEmoji || platform::is_apple())
        {
            AddSoftHyphenIfNeed(Buffer_, LastWord_, isWordWrapEnabled);
        }

        /* this crushes emojis with skin tones
        const auto applyOsxEmojiFix = (
            platform::is_apple() &&
            isEmoji &&
            !Buffer_.endsWith(QChar::SoftHyphen));
        if (applyOsxEmojiFix)
        {
            Buffer_ += QChar::SoftHyphen;
        }
        */

	}

	bool Text2DocConverter::IsEos(const int offset) const
	{
		assert(offset >= 0);
		assert(Input_.string());

		const auto &text = *Input_.string();
		return ((Input_.pos() + offset) >= text.length());
	}

	bool Text2DocConverter::IsHtmlEscapingEnabled() const
	{
        assert(HtmlMode_ >= Text2DocHtmlMode::Min);
		assert(HtmlMode_ <= Text2DocHtmlMode::Max);

		return (HtmlMode_ == Text2DocHtmlMode::Escape);
	}

	void Text2DocConverter::FlushBuffers()
	{
        if (Buffer_.isEmpty())
        {
            return;
        }

		if (IsHtmlEscapingEnabled())
		{
			Writer_.insertHtml(Buffer_);
		}
		else
		{
			Writer_.insertText(Buffer_);
		}

		Buffer_.resize(0);
        LastWord_.resize(0);
	}

	Utils::SChar Text2DocConverter::PeekSChar()
	{
		return Utils::PeekNextSuperChar(Input_);
	}

	void Text2DocConverter::PushInputCursor()
	{
		InputCursorStack_.push_back((int)Input_.pos());
		assert(InputCursorStack_.size() < InputCursorStack_.capacity());
	}

	void Text2DocConverter::PopInputCursor(const int steps)
	{
		assert(!InputCursorStack_.empty());
		assert(steps > 0);
		assert(steps < 100);

		const auto newSize = (InputCursorStack_.size() - (unsigned)steps);
		const auto storedPos = InputCursorStack_[newSize];
		InputCursorStack_.resize(newSize);

		const auto success = Input_.seek(storedPos);
		assert(success);
        (void)success;
	}

	Utils::SChar Text2DocConverter::ReadSChar()
	{
		return Utils::ReadNextSuperChar(Input_);
	}

	QString Text2DocConverter::ReadString(const int length)
	{
		return Input_.read(length);
	}

    void Text2DocConverter::ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
	{
        const unsigned _main = ch.Main();
        const unsigned _ext = ch.Ext();

		assert(_main > 0);

        FlushBuffers();

        const auto charFormat = Writer_.charFormat();

		auto make_from_code = [](int _code)->QString
		{
			QString result;

			if (QChar::requiresSurrogates(_code))
			{
				QChar high = QChar::highSurrogate(_code);
				QChar low = QChar::lowSurrogate(_code);

				result += high;
				result += low;
			}
			else
			{
				result += (QChar) _code;
			}

			return result;
		};
        
		const QImage& img = Emoji::GetEmoji(_main, _ext, _emojiSize);

		QString emoji_code = make_from_code(_main);

		if (_ext)
			emoji_code += make_from_code(_ext);

		static int64_t uniq_index = 0;

		QString resource_name;
		if (!MakeUniqResources_)
		{
			resource_name = QString::number(img.cacheKey());
		}
		else
		{
			resource_name = emoji_code + "_" + QString::number(++uniq_index);
			Resources_[resource_name] = emoji_code;
		}

		Writer_.document()->addResource(
            QTextDocument::ImageResource,
            QUrl(resource_name),
            img
        );

		QTextImageFormat format;
		format.setName(resource_name);
		format.setVerticalAlignment(_aligment);
		Writer_.insertText(QString(QChar::ObjectReplacementCharacter), format);

        Writer_.setCharFormat(charFormat);
	}

    void Text2DocConverter::SaveAsHtml(const QString& _text, const common::tools::url& _url, bool isWordWrapEnabled)
    {
        QString displayText;
        ReplaceUrlSpec(_text, displayText, isWordWrapEnabled);

        const auto bufferPos1 = Buffer_.length();

        if (_url.is_email())
            Buffer_ += "<a type=\"email\" href=\"mailto:";
        else
            Buffer_ += "<a href=\"";

        Buffer_ += QString::fromUtf8(_url.url_.c_str());
        Buffer_ += "\">";
        Buffer_ += displayText;
        Buffer_ += "</a>";

        const auto bufferPos2 = Buffer_.length();

        if (UriCallback_)
        {
            UriCallback_(Buffer_.mid(bufferPos1, bufferPos2 - bufferPos1), Writer_.position());
        }
    }

	void Text2DocConverter::MakeUniqueResources(const bool _make)
	{
		MakeUniqResources_ = _make;
	}

	const ResourceMap& Text2DocConverter::GetResources() const
	{
		return Resources_;
	}
}

namespace
{
	void ReplaceUrlSpec(const QString &url, QString &out, bool isWordWrapEnabled)
	{
		assert(out.isEmpty());
		assert(!url.isEmpty());

        static const QString AMP_ENTITY("&amp;");

		for (const QChar ch : url)
		{
			switch (ch.unicode())
			{
			    case '&':
					out += AMP_ENTITY;
					break;

			    default:
					out += ch;
					break;
			}

            if (isWordWrapEnabled && !ch.isHighSurrogate())
            {
                Text2DocConverter::AddSoftHyphenIfNeed(out, out, isWordWrapEnabled);
            }
		}
	}
}
