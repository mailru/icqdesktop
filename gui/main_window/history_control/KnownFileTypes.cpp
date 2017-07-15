#include "stdafx.h"

#include "../../themes/ThemePixmap.h"
#include "../../themes/ResourceIds.h"

#include "KnownFileTypes.h"

namespace
{
	typedef std::map<QString, Themes::PixmapResourceId> Ext2IconMap;

    typedef QSet<QString> QStringSet;

	const Ext2IconMap& GetExt2IconMap();
}

namespace History
{
	const Themes::IThemePixmapSptr& GetIconByFilename(const QString &filename)
	{
		assert(!filename.isEmpty());

        const auto resId = GetIconIdByFilename(filename);
		return Themes::GetPixmap(resId);
	}

    Themes::PixmapResourceId GetIconIdByFilename(const QString &filename)
    {
        assert(!filename.isEmpty());

        QFileInfo fi(filename);

        const auto &map = GetExt2IconMap();

        const auto iter = map.find(fi.suffix());
        if (iter != map.end())
        {
            return iter->second;
        }

        return Themes::PixmapResourceId::FileSharingFileTypeIconUnknown;
    }
}

namespace
{
	const Ext2IconMap& GetExt2IconMap()
	{
        using namespace Themes;

		static Ext2IconMap map;

		if (!map.empty())
		{
            return map;
        }

        map.emplace("exe", PixmapResourceId::FileSharingFileTypeIconExe);

        map.emplace("xls",  PixmapResourceId::FileSharingFileTypeIconExcel);
        map.emplace("xlsx", PixmapResourceId::FileSharingFileTypeIconExcel);
        map.emplace("csv",  PixmapResourceId::FileSharingFileTypeIconExcel);

        map.emplace("html", PixmapResourceId::FileSharingFileTypeIconHtml);
        map.emplace("htm",  PixmapResourceId::FileSharingFileTypeIconHtml);

        map.emplace("png",  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("jpg",  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("jpeg", PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("bmp",  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("gif",  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("tif",  PixmapResourceId::FileSharingFileTypeIconImage);
        map.emplace("tiff", PixmapResourceId::FileSharingFileTypeIconImage);

        map.emplace("pdf", PixmapResourceId::FileSharingFileTypeIconPdf);

        map.emplace("ppt", PixmapResourceId::FileSharingFileTypeIconPpt);

        map.emplace("wav",  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("mp3",  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("ogg",  PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("flac", PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("aac", PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("m4a", PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("aiff", PixmapResourceId::FileSharingFileTypeIconSound);
        map.emplace("ape", PixmapResourceId::FileSharingFileTypeIconSound);

        map.emplace("log", PixmapResourceId::FileSharingFileTypeIconTxt);
        map.emplace("txt", PixmapResourceId::FileSharingFileTypeIconTxt);

        map.emplace("mp4", PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace("avi", PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace("mkv", PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace("wmv", PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace("flv", PixmapResourceId::FileSharingFileTypeIconVideo);
        map.emplace("webm", PixmapResourceId::FileSharingFileTypeIconVideo);

        map.emplace("doc",  PixmapResourceId::FileSharingFileTypeIconWord);
        map.emplace("docx", PixmapResourceId::FileSharingFileTypeIconWord);
        map.emplace("rtf",  PixmapResourceId::FileSharingFileTypeIconWord);

        map.emplace("zip", PixmapResourceId::FileSharingFileTypeIconZip);
        map.emplace("rar", PixmapResourceId::FileSharingFileTypeIconZip);

		return map;
	}

}