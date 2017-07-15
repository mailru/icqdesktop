#pragma once

namespace Emoji
{
	enum class EmojiSizePx
	{
		Invalid,
		Min = 16,

		_16	= Min,
		Auto = 21,
		_22 = 22,
		_27 = 27,
		_32 = 32,
		_40 = 40,
		_44 = 44,
		_48 = 48,
		_64 = 64,

		Max = 64
	};

	void InitializeSubsystem();

	void Cleanup();

	const QImage& GetEmoji(const uint32_t _main, const uint32_t _ext, const EmojiSizePx size = EmojiSizePx::Auto);

	EmojiSizePx GetFirstLesserOrEqualSizeAvailable(const int32_t _sizePx);

	EmojiSizePx GetNearestSizeAvailable(const int32_t _sizePx);

}
