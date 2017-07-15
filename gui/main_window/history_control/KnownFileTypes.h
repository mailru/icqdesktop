#pragma once

namespace Themes
{
    enum class PixmapResourceId;

	class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace History
{
	const Themes::IThemePixmapSptr& GetIconByFilename(const QString &filename);

    Themes::PixmapResourceId GetIconIdByFilename(const QString &filename);
}