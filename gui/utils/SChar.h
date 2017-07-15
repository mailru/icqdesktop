#pragma once

namespace Utils
{

	class SChar
	{
	public:
		static const SChar Null;

		SChar(const uint32_t base, const uint32_t ext);

		bool EqualTo(const char ch) const;

		bool EqualToI(const char ch) const;

		uint32_t Ext() const;

		bool HasMain() const;

		bool HasExt() const;

		bool IsCarriageReturn() const;

        bool IsQuot() const;

		bool IsComplex() const;

		bool IsEmailCharacter() const;

		bool IsEmoji() const;

		bool IsSimple() const;

		bool IsSingleCharacterEmoji() const;

		bool IsSpace() const;

		bool IsTwoCharacterEmoji() const;

		bool IsNewline() const;

        bool IsColon() const;

		bool IsNull() const;

		bool IsValidInUrl() const;

		bool IsValidOnUrlEnd() const;

        bool IsDelimeter() const;

		QString::size_type LengthQChars() const;

		uint32_t Main() const;

		QChar ToQChar() const;

		QString ToQString() const;

	private:
		const uint32_t Main_;

		const uint32_t Ext_;
	};

	SChar ReadNextSuperChar(QTextStream &s);

	SChar PeekNextSuperChar(QTextStream &s);

	SChar PeekNextSuperChar(const QString &s, const QString::size_type offset = 0);

}
