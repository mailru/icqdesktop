#include "stdafx.h"

#include "Emoji.h"
#include "EmojiDb.h"

#include "../../utils/SChar.h"
#include "../../utils/utils.h"

namespace
{
	using namespace Emoji;

	struct EmojiSetMeta
	{
		EmojiSetMeta(const int32_t _sizePx, const QString& _resourceName);

		const int32_t SizePx_;

		const QString ResourceName_;
	};

	static const std::array<EmojiSizePx, 8> AvailableSizes_ =
	{
		EmojiSizePx::_16,
		EmojiSizePx::_22,
		EmojiSizePx::_27,
		EmojiSizePx::_32,
		EmojiSizePx::_40,
		EmojiSizePx::_44,
		EmojiSizePx::_48,
		EmojiSizePx::_64
	};

	QFuture<bool> Loading_;

	typedef std::unordered_map<int32_t, const QImage> EmojiSetsMap;

	typedef EmojiSetsMap::iterator EmojiSetsIter;

	EmojiSetsMap EmojiSetBySize_;

	std::unordered_map<int64_t, const QImage> EmojiCache_;

	int32_t GetEmojiSizeForCurrentUiScale();

	const EmojiSetMeta& GetMetaForCurrentUiScale();

	const EmojiSetMeta& GetMetaBySize(const int32_t _sizePx);

	EmojiSetsIter LoadEmojiSetForSizeIfNeeded(const EmojiSetMeta& _meta);

	int64_t MakeCacheKey(const int32_t _index, const int32_t _sizePx);

}

namespace Emoji
{

	void InitializeSubsystem()
	{
		InitEmojiDb();

		const auto load = []
		{
			const auto &meta = GetMetaForCurrentUiScale();

			LoadEmojiSetForSizeIfNeeded(meta);

			return true;
		};

		Loading_ = QtConcurrent::run(QThreadPool::globalInstance(), load);
	}

	void Cleanup()
	{
		EmojiCache_.clear();
		EmojiSetBySize_.clear();
	}

	const QImage& GetEmoji(const uint32_t _main, const uint32_t _ext, const EmojiSizePx _size)
	{
		assert(_main > 0);
		assert(_size >= EmojiSizePx::Min);
		assert(_size <= EmojiSizePx::Max);

		static QImage empty;

		Loading_.waitForFinished();
		if (!Loading_.result())
		{
			return empty;
		}

		const auto sizeToSearch = ((_size == EmojiSizePx::Auto) ? GetEmojiSizeForCurrentUiScale() : (int32_t)_size);

		const auto info = GetEmojiInfoByCodepoint(_main, _ext);
		if (!info)
		{
			return empty;
		}
		assert(info->Index_ >= 0);

		const auto key = MakeCacheKey(info->Index_, sizeToSearch);
		auto cacheIter = EmojiCache_.find(key);
		if (cacheIter != EmojiCache_.end())
		{
			assert(!cacheIter->second.isNull());
			return cacheIter->second;
		}

        QImage image;

        if (platform::is_apple())
        {
            auto pixelRatio = qApp->primaryScreen()->devicePixelRatio();
            
            QFont font(QStringLiteral("AppleColorEmoji"), sizeToSearch - Utils::scale_value(6));
            QFontMetrics metrics(font);
            QString s = Utils::SChar(_main, _ext).ToQString();
            
            QImage imageOut(QSize(sizeToSearch * pixelRatio, sizeToSearch * pixelRatio), QImage::Format_ARGB32);
            imageOut.fill(Qt::transparent);
            imageOut.setDevicePixelRatio(pixelRatio);
            
            QPainter painter(&imageOut);
            painter.setFont(font);
            painter.drawText(QRect(0, 0, sizeToSearch, sizeToSearch), Qt::AlignCenter, s);
            painter.end();
            
            image = imageOut;

            /*
            auto pixelRatio = qApp->primaryScreen()->devicePixelRatio();
            
            QImage imageOut(QSize(sizeToSearch * pixelRatio, sizeToSearch * pixelRatio), QImage::Format_ARGB32);
            imageOut.fill(Qt::transparent);
            imageOut.setDevicePixelRatio(pixelRatio);
            
            QPainter painter(&imageOut);
            painter.setFont(QFont(QStringLiteral("AppleColorEmoji"), sizeToSearch));
            painter.drawText(QRect(0, -2, sizeToSearch, sizeToSearch + 2), Qt::AlignHCenter, Utils::SChar(_main, _ext).ToQString());
            painter.end();
            
            image = imageOut;
            */
        }
        else
        {
            const auto &meta = GetMetaBySize(sizeToSearch);

            const auto emojiSetIter = LoadEmojiSetForSizeIfNeeded(meta);

            const auto isSetMissing = (emojiSetIter == EmojiSetBySize_.end());
            if (isSetMissing)
            {
                return empty;
            }

            QRect r(0, 0, meta.SizePx_, meta.SizePx_);
            const auto offsetX = (info->Index_ * meta.SizePx_);
            r.translate(offsetX, 0);

            image = emojiSetIter->second.copy(r);
        }

        const auto result = EmojiCache_.emplace(key, std::move(image));
        assert(result.second);

		return result.first->second;
	}

	EmojiSizePx GetNearestSizeAvailable(const int32_t _sizePx)
	{
		assert(_sizePx > 0);

		auto nearest = std::make_tuple(EmojiSizePx::_16, std::numeric_limits<int32_t>::max());

		for (const auto &size : AvailableSizes_)
		{
			const auto diff = std::abs(_sizePx - (int32_t)size);
			const auto minDiff = std::get<1>(nearest);
			if (diff <= minDiff)
			{
				nearest = std::make_tuple(size, diff);
			}
		}

		return std::get<0>(nearest);
	}

	EmojiSizePx GetFirstLesserOrEqualSizeAvailable(const int32_t _sizePx)
	{
		assert(_sizePx > 0);

		const auto sizeIter = std::find_if(
			AvailableSizes_.rbegin(),
			AvailableSizes_.rend(),
			[_sizePx](const EmojiSizePx availableSize)
			{
				return (((int)availableSize) <= _sizePx);
			}
		);

		if (sizeIter != AvailableSizes_.rend())
		{
			return *sizeIter;
		}

		return EmojiSizePx::_16;
	}

}

namespace
{
	EmojiSetMeta::EmojiSetMeta(const int32_t _sizePx, const QString& _resourceName)
		: SizePx_(_sizePx)
		, ResourceName_(_resourceName)
	{
		assert(SizePx_ > 0);
		assert(!ResourceName_.isEmpty());
	}

	int32_t GetEmojiSizeForCurrentUiScale()
	{
		static std::unordered_map<int32_t, int32_t> info;
		if (info.empty())
		{
			info.emplace(100, 22);
			info.emplace(125, 27);
			info.emplace(150, 32);
			info.emplace(200, 44);
		}

		const auto k = (int32_t)(Utils::getScaleCoefficient() * 100);
		assert(info.count(k));

		const auto size = info[k];
		assert(size > 0);

		return size;
	}

	const EmojiSetMeta& GetMetaForCurrentUiScale()
	{
		const auto size = GetEmojiSizeForCurrentUiScale();
		assert(size > 0);

		return GetMetaBySize(size);
	}

	const EmojiSetMeta& GetMetaBySize(const int32_t _sizePx)
	{
		assert(_sizePx > 0);

		typedef std::shared_ptr<EmojiSetMeta> EmojiSetMetaSptr;

		static std::unordered_map<int32_t, EmojiSetMetaSptr> info;
		if (info.empty())
		{
			info.emplace(16, std::make_shared<EmojiSetMeta>(16, ":/emoji/16.png"));
			info.emplace(22, std::make_shared<EmojiSetMeta>(22, ":/emoji/22.png"));
			info.emplace(27, std::make_shared<EmojiSetMeta>(27, ":/emoji/27.png"));
			info.emplace(32, std::make_shared<EmojiSetMeta>(32, ":/emoji/32.png"));
			info.emplace(40, std::make_shared<EmojiSetMeta>(40, ":/emoji/40.png"));
			info.emplace(44, std::make_shared<EmojiSetMeta>(44, ":/emoji/44.png"));
			info.emplace(48, std::make_shared<EmojiSetMeta>(48, ":/emoji/48.png"));
			info.emplace(64, std::make_shared<EmojiSetMeta>(64, ":/emoji/64.png"));
		}

		assert(info.count(_sizePx));
		return *info[_sizePx];
	}

	EmojiSetsIter LoadEmojiSetForSizeIfNeeded(const EmojiSetMeta& _meta)
	{
		assert(_meta.SizePx_ > 0);

		auto emojiSetIter = EmojiSetBySize_.find(_meta.SizePx_);
		if (emojiSetIter != EmojiSetBySize_.end())
		{
			return emojiSetIter;
		}

		QImage setImg;
		const auto success = setImg.load(_meta.ResourceName_);
		if (!success)
		{
			return EmojiSetBySize_.end();
		}

		return EmojiSetBySize_.emplace(_meta.SizePx_, std::move(setImg)).first;
	}

	int64_t MakeCacheKey(const int32_t _index, const int32_t _sizePx)
	{
		return ((int64_t)_index | ((int64_t)_sizePx << 32));
	}
}
