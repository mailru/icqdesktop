#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "FileSharingUtils.h"

#include "../../../utils/utils.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    typedef std::vector<int32_t> ReverseIndexMap;

    int32_t decodeDuration(const QStringRef &str);

    int32_t decodeSize(const QChar qch0, const QChar qch1);

    const ReverseIndexMap& getReverseIndexMap();
}

core::file_sharing_content_type extractContentTypeFromFileSharingId(const QString &id)
{
    if (id.isEmpty())
        return core::file_sharing_content_type::undefined;
    
    const auto id0 = id[0].toLatin1();

    const auto is_snap_image = (id0 == '1');
    if (is_snap_image)
    {
        return core::file_sharing_content_type::snap_image;
    }

    const auto is_snap_gif = (id0 == '5');
    if (is_snap_gif)
    {
        return core::file_sharing_content_type::snap_gif;
    }

    const auto is_gif = (id0 == '4');
    if (is_gif)
    {
        return core::file_sharing_content_type::gif;
    }

    const auto is_snap_video = (id0 == '9');
    if (is_snap_video)
    {
        return core::file_sharing_content_type::snap_video;
    }

    const auto is_ppt = ((id0 == 'I') || (id0 == 'J'));
    if (is_ppt)
    {
        return core::file_sharing_content_type::ptt;
    }

    const auto is_image = ((id0 >= '0') && (id0 <= '7'));
    if (is_image)
    {
        return core::file_sharing_content_type::image;
    }

    const auto is_video = (
        ((id0 >= '8') && (id0 <= '9')) ||
        ((id0 >= 'A') && (id0 <= 'F')));
    if (is_video)
    {
        return core::file_sharing_content_type::video;
    }

    return core::file_sharing_content_type::undefined;
}

int32_t extractDurationFromFileSharingId(const QString &id)
{
    const auto isValidId = (id.length() >= 5);
    if (!isValidId)
    {
        assert(!"invalid file sharing id");
        return -1;
    }

    const auto isPtt = extractContentTypeFromFileSharingId(id) == core::file_sharing_content_type::ptt;
    if (!isPtt)
    {
        return -1;
    }

    return decodeDuration(id.midRef(1, 4));
}

QSize extractSizeFromFileSharingId(const QString &id)
{
    const auto isValidId = (id.length() > 5);
    if (!isValidId)
    {
        assert(!"invalid id");
        return QSize();
    }

    const auto width = decodeSize(id[1], id[2]);
    assert(width >= 0);

    const auto height = decodeSize(id[3], id[4]);
    assert(height >= 0);

    return QSize(width, height);
}

QString extractIdFromFileSharingUri(const QString &uri)
{
    assert(!uri.isEmpty());
    if (uri.isEmpty())
    {
        return QString();
    }

    QString normalizedUri = Utils::normalizeLink(uri);

    static const QRegularExpression re(
        "^"
        "http(s?)://"
        "("
            "(files\\.icq\\.net/get)"
            "|"
            "(icq\\.com/files)"
        ")/"
        "(?P<id>\\w{33,})"
        "$"
    );

    auto match = re.match(normalizedUri);
    if (!match.hasMatch())
    {
        return QString();
    }

    auto id = match.captured("id");

    return id;
}

namespace
{
    const auto ASCII_MAX = 128;

    const auto INDEX_DIVISOR = 62;

    int32_t decodeDuration(const QStringRef &str)
    {
        assert(!str.isEmpty());

        auto result = 0;

        for (const auto qch : str)
        {
            const auto ch = qch.toLatin1();

            if ((ch >= '0') && (ch <= '9'))
            {
                const auto MAGIC_0 = 48;
                result += (ch - MAGIC_0);
                continue;
            }

            if((ch >= 'a') && (ch <= 'z'))
            {
                const auto MAGIC_1 = 87;
                result += (ch - MAGIC_1);
                continue;
            }

            const auto MAGIC_2 = 29;
            result += (ch - MAGIC_2);
        }

        return result;
    }

    int32_t decodeSize(const QChar qch0, const QChar qch1)
    {
        const auto ch0 = (size_t)qch0.toLatin1();
        const auto ch1 = (size_t)qch1.toLatin1();

        const auto &map = getReverseIndexMap();

        if (ch0 >= map.size())
        {
            assert(!"invalid first character");
            return 0;
        }

        if (ch1 >= map.size())
        {
            assert(!"invalid first character");
            return 0;
        }

        const auto index0 = map[ch0];
        assert(index0 >= 0);

        const auto index1 = map[ch1];
        assert(index1 >= 0);

        auto size = (index0 * INDEX_DIVISOR);
        size += index1;

        assert(size > 0);
        return size;
    }

    const ReverseIndexMap& getReverseIndexMap()
    {
        static ReverseIndexMap map;

        if (!map.empty())
        {
            return map;
        }

        map.resize(ASCII_MAX);

        auto index = 0;

        const auto fillMap =
            [&index]
            (const char from, const char to)
            {
                for (auto ch = from; ch <= to; ++ch, ++index)
                {
                    map[ch] = index;
                }
            };

        fillMap('0', '9');
        fillMap('a', 'z');
        fillMap('A', 'Z');

        return map;
    }
}

UI_COMPLEX_MESSAGE_NS_END
