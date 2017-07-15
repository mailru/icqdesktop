#include "stdafx.h"

#include "SChar.h"
#include "../cache/emoji/EmojiDb.h"

namespace
{
	const QChar CH_KEYCAP(0x20E3);

	bool IsSingleCharacterEmoji(const uint32_t main);

	bool IsTwoCharacterEmoji(const uint32_t main, const uint32_t ext);

	uint32_t ReadNextCodepoint(QTextStream &s);
}

namespace Utils
{

	const SChar SChar::Null(0, 0);

	SChar::SChar(const uint32_t base, const uint32_t ext)
		: Main_(base)
		, Ext_(ext)
	{
		assert(!HasExt() || HasMain());
	}

	bool SChar::EqualTo(const char ch) const
	{
		return (!HasExt() && (Main_ == ch));
	}

	bool SChar::EqualToI(const char ch) const
	{
		return EqualTo(QChar(ch).toLower().unicode());
	}

	uint32_t SChar::Ext() const
	{
		return Ext_;
	}

	bool SChar::HasMain() const
	{
		return (Main_ > 0);
	}

	bool SChar::HasExt() const
	{
		return (Ext_ > 0);
	}

	bool SChar::IsCarriageReturn() const
	{
		return (!HasExt() && (Main_ == '\r'));
	}

    bool SChar::IsQuot() const
    {
        return (!HasExt() && (Main_ == '\"'));
    }

	bool SChar::IsComplex() const
	{
		return (LengthQChars() > 1);
	}

	bool SChar::IsEmailCharacter() const
	{
		if (HasExt())
		{
			return false;
		}

		return (
			QChar::isLetterOrNumber(Main_) ||
			(Main_ == '_') ||
			(Main_ == '.') ||
			(Main_ == '-')
		);
	}

	bool SChar::IsEmoji() const
	{
        if (IsNull())
        {
            return false;
        }

		return (IsTwoCharacterEmoji() || IsSingleCharacterEmoji());
	}

	bool SChar::IsSimple() const
	{
		return (
            (LengthQChars() == 1) &&
            !IsSingleCharacterEmoji()
        );
	}

	bool SChar::IsSingleCharacterEmoji() const
	{
		return ::IsSingleCharacterEmoji(Main_);
	}

	bool SChar::IsSpace() const
	{
		return (IsSimple() && QChar::isSpace(Main_));
	}

	bool SChar::IsTwoCharacterEmoji() const
	{
		return ::IsTwoCharacterEmoji(Main_, Ext_);
	}

	QString::size_type SChar::LengthQChars() const
	{
		if (IsNull())
		{
			return 0;
		}

		QString::size_type length = 0;

		if (QChar::requiresSurrogates(Main_))
		{
			length += 2;
		}
		else
		{
			length += 1;
		}

		if (!HasExt())
		{
			return length;
		}

		if (QChar::requiresSurrogates(Ext_))
		{
			length += 2;
		}
		else
		{
			length += 1;
		}

		return length;
	}

	uint32_t SChar::Main() const
	{
		return Main_;
	}

	QChar SChar::ToQChar() const
	{
#ifdef __APPLE__
        if (Main_ == 0x0000FFFC)
        {
            return QChar();
        }
#endif

		if (LengthQChars() > 1)
		{
			assert(!"conversion not possible");
			return QChar();
		}

		return QChar(Main_);
	}

	QString SChar::ToQString() const
	{
		QString result;

#ifdef __APPLE__
        if (Main_ == 0x0000FFFC)
        {
            return QString();
        }
#endif
        
		const auto MAX_LENGTH = 4;
		result.reserve(MAX_LENGTH);

		if (IsNull())
		{
            assert(result.length() <= MAX_LENGTH);
			return result;
		}

		if (QChar::requiresSurrogates(Main_))
		{
			result += QChar(QChar::highSurrogate(Main_));
			result += QChar(QChar::lowSurrogate(Main_));
		}
		else
		{
			result += QChar(Main_);
		}

		if (!HasExt())
		{
            assert(result.length() <= MAX_LENGTH);
			return result;
		}

		if (QChar::requiresSurrogates(Ext_))
		{
			result += QChar(QChar::highSurrogate(Ext_));
			result += QChar(QChar::lowSurrogate(Ext_));
		}
		else
		{
			result += QChar(Ext_);
		}

        assert(result.length() <= MAX_LENGTH);
		return result;
	}

	bool SChar::IsNewline() const
	{
		return (!HasExt() && (Main_ == '\n'));
	}

	bool SChar::IsNull() const
	{
		return (!HasExt() && !HasMain());
	}

	bool SChar::IsValidInUrl() const
	{
		if (IsComplex() || IsNull())
		{
			return false;
		}

		static QString invalidChars(" \r\n\t$<>^\\{}|\"");
		return !invalidChars.contains(ToQChar());
	}

    bool SChar::IsColon() const
    {
        return (!HasExt() && (Main_ == ':'));
    }

	bool SChar::IsValidOnUrlEnd() const
	{
		if (IsComplex() || IsNull())
		{
			return false;
		}

		static QString invalidChars(" \r\n\t;,'");
		return !invalidChars.contains(ToQChar());
	}

    bool SChar::IsDelimeter() const
    {
        if (HasExt())
        {
            return false;
        }

        return (
            (Main_ == '>') ||
            (Main_ == '<')
            );
    }

	SChar ReadNextSuperChar(QTextStream &s)
	{
		const auto main = ReadNextCodepoint(s);
		if (main == 0)
		{
			return SChar::Null;
		}

		const auto pos = s.pos();

		const auto ext = ReadNextCodepoint(s);
		if (ext == 0)
		{
			s.seek(pos);
			return SChar(main, 0);
		}

		if (IsTwoCharacterEmoji(main, ext))
		{
			return SChar(main, ext);
		}

		s.seek(pos);
		return SChar(main, 0);
	}

	SChar PeekNextSuperChar(QTextStream &s)
	{
		const auto pos = s.pos();
		const auto ch = ReadNextSuperChar(s);
		s.seek(pos);

		return ch;
	}

	SChar PeekNextSuperChar(const QString &s, const QString::size_type offset)
	{
		assert(offset < s.length());

		QTextStream input((QString*)&s, QIODevice::ReadOnly);

		const auto success = input.seek(offset);
        if (!success)
        {
            assert(success);
        }

		return ReadNextSuperChar(input);
	}

}

namespace
{
	bool IsSingleCharacterEmoji(const uint32_t main)
	{
        return Emoji::GetEmojiInfoByCodepoint(main, 0) != Emoji::EmptyEmoji;
	}

	bool IsTwoCharacterEmoji(const uint32_t main, const uint32_t ext)
	{
		 return Emoji::GetEmojiInfoByCodepoint(main, ext) != Emoji::EmptyEmoji;
	}

	uint32_t ReadNextCodepoint(QTextStream &s)
	{
		if (s.atEnd())
		{
			return 0;
		}

		QChar high;
		s >> high;

		if (s.atEnd() || !high.isHighSurrogate())
		{
			return high.unicode();
		}

		const auto pos = s.pos();

		QChar low;
		s >> low;

		if (!low.isLowSurrogate())
		{
			const auto seekSucceed = s.seek(pos);
            if (!seekSucceed)
            {
			    assert(seekSucceed);
            }

			return high.unicode();
		}

		return QChar::surrogateToUcs4(high, low);
	}
}
