#pragma once

#include "namespaces.h"

FONTS_NS_BEGIN

enum class FontFamily
{
    MIN,

    ARIAL,
    HELVETICA_NEUE,
    LUCIDA_GRANDE,
    SAN_FRANCISCO,
    SEGOE_UI,

    MAX,
};

enum class FontWeight
{
    Min,

    Light,
    Normal,
    Medium,
    Bold,

    Max,
};

QFont appFont(const int32_t _sizePx);

QFont appFont(const int32_t _sizePx, const FontFamily _family);

QFont appFont(const int32_t _sizePx, const FontWeight _weight);

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight);

QFont appFontScaled(const int32_t _sizePx);

QFont appFontScaled(const int32_t _sizePx, const FontWeight _weight);

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight);

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontWeight _weight);

QString appFontFamilyNameQss(const FontFamily _fontFamily, const FontWeight _fontWeight);

QString appFontWeightQss(const FontWeight _weight);

FontFamily defaultAppFontFamily();

QString defaultAppFontQssName();

QString defaultAppFontQssWeight();

FontWeight defaultAppFontWeight();

QString SetFont(const QString& _qss);

FONTS_NS_END

namespace std
{
    template<>
    struct hash<Fonts::FontWeight>
    {
        size_t operator()(const Fonts::FontWeight &_v) const
        {
            return (size_t)_v;
        }
    };
}
