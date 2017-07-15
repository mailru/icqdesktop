#include "stdafx.h"

#include "platform.h"
#include "utils/utils.h"

#include "fonts.h"

FONTS_NS_BEGIN

namespace
{
    typedef std::unordered_map<FontWeight, QString> QssWeightsMapT;

    const auto QFONT_WEIGHT_MEDIUM = (QFont::Weight)57;

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font);

    void applyFontWeight(
        const FontFamily _fontFamily,
        const FontWeight _fontWeight,
        const int32_t _sizePx,
        Out QFont &_font);

    const QssWeightsMapT& getCurrentWeightsMap();

    const QssWeightsMapT& getPreSierraWeightsMap();

    const QssWeightsMapT& getSierraWeightsMap();

    const QssWeightsMapT& getWindowsWeightsMap();

    const QssWeightsMapT& getArialWeightsMap();

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontStyle);

    QFont::Weight icqWeight2QtWeight(const FontWeight _internalWeight);

    QString segoeUiFamilyName(const FontWeight _weight);

    namespace san_francisco
    {
        const QString fontFamily = "";

        namespace weight
        {
            const QString mediumQss = "504";

            int32_t semibold(const int32_t _size);

            const QString boldQss = "696";

            const int bold = 87;
        }
    }

}

QFont appFont(const int32_t _sizePx)
{
    return appFont(_sizePx, defaultAppFontFamily(), defaultAppFontWeight());
}

QFont appFont(const int32_t _sizePx, const FontFamily _family)
{
    return appFont(_sizePx, _family, defaultAppFontWeight());
}

QFont appFont(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(_sizePx, defaultAppFontFamily(), _weight);
}

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    assert(_sizePx > 0);
    assert(_sizePx < 500);
    assert(_family > FontFamily::MIN);
    assert(_family < FontFamily::MAX);
    assert(_weight > FontWeight::Min);
    assert(_weight < FontWeight::Max);

    QFont result;

    result.setPixelSize(_sizePx);

    applyFontFamily(_family, Out result);

    applyFontWeight(_family, _weight, _sizePx, Out result);

    return result;
}

QFont appFontScaled(const int32_t _sizePx)
{
    return appFont(Utils::scale_value(_sizePx));
}

QFont appFontScaled(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _weight);
}

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _family, _weight);
}

FontFamily defaultAppFontFamily()
{
    if (platform::is_apple())
    {
        if (platform::osxVersion() < platform::OsxVersion::MV_10_10)
        {
            return FontFamily::LUCIDA_GRANDE;
        }

        return FontFamily::HELVETICA_NEUE;
    }

    if (platform::is_windows_vista_or_late())
    {
        return FontFamily::SEGOE_UI;
    }

    return FontFamily::ARIAL;
}

FontWeight defaultAppFontWeight()
{
    return FontWeight::Normal;
}

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    assert(_sizePx > 0);
    assert(_sizePx < 1000);
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);
    assert(_fontWeight > FontWeight::Min);
    assert(_fontWeight < FontWeight::Max);

    QString result;
    result.reserve(512);

    result += "font-size: ";
    result += QString::number(_sizePx);
    result += "px; font-family: \"";
    result += appFontFamilyNameQss(_fontFamily, _fontWeight);
    result += "\"";

    const auto weight = evalQssFontWeight(_fontFamily, _fontWeight);
    if (!weight.isEmpty())
    {
        result += "; font-weight: ";
        result += weight;
    }

    return result;
}

QString appFontFamilyNameQss(const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    assert(_fontFamily > FontFamily::MIN);
    assert(_fontFamily < FontFamily::MAX);

    switch (_fontFamily)
    {
        case FontFamily::ARIAL:
            return "Arial";

        case FontFamily::HELVETICA_NEUE:
            return "Helvetica Neue";

        case FontFamily::LUCIDA_GRANDE:
            return "Lucida Grande";

        case FontFamily::SAN_FRANCISCO:
            return san_francisco::fontFamily;

        case FontFamily::SEGOE_UI:
            return segoeUiFamilyName(_fontWeight);
    }

    assert(!"unexpected font family");
    return QString("Comic Sans");
}

QString appFontWeightQss(const FontWeight _weight)
{
    const auto &weightMap = getCurrentWeightsMap();

    const auto iter = weightMap.find(_weight);
    if (iter == weightMap.end())
    {
        assert(!"unknown font weight");
        return defaultAppFontQssWeight();
    }

    const auto &fontWeight = iter->second;

    return fontWeight;
}

QString defaultAppFontQssName()
{
    return appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
}

QString defaultAppFontQssWeight()
{
    const auto &weights = getCurrentWeightsMap();

    auto iter = weights.find(defaultAppFontWeight());
    assert(iter != weights.end());

    return iter->second;
}

QString SetFont(const QString& _qss)
{
    QString result(_qss);

    const auto fontFamily = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
    const auto fontFamilyBold = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Bold);
    const auto fontFamilyMedium = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Medium);
    const auto fontFamilyLight = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Light);

    result.replace("%FONT_FAMILY%", fontFamily);
    result.replace("%FONT_FAMILY_BOLD%", fontFamilyBold);
    result.replace("%FONT_FAMILY_MEDIUM%", fontFamilyMedium);
    result.replace("%FONT_FAMILY_LIGHT%", fontFamilyLight);

    const auto fontWeightQss = appFontWeightQss(FontWeight::Normal);
    const auto fontWeightBold = appFontWeightQss(FontWeight::Bold);
    const auto fontWeightMedium = appFontWeightQss(FontWeight::Medium);
    const auto fontWeightLight = appFontWeightQss(FontWeight::Light);

    result.replace("%FONT_WEIGHT%", fontWeightQss);
    result.replace("%FONT_WEIGHT_BOLD%", fontWeightBold);
    result.replace("%FONT_WEIGHT_MEDIUM%", fontWeightMedium);
    result.replace("%FONT_WEIGHT_LIGHT%", fontWeightLight);

    return result;
}

namespace
{
    const QssWeightsMapT& getCurrentWeightsMap()
    {
        if (platform::is_apple())
        {
            const auto isPreSierraVersion = (platform::osxVersion() <= platform::OsxVersion::MV_10_11);
            if (isPreSierraVersion)
            {
                return getPreSierraWeightsMap();
            }

            return getSierraWeightsMap();
        }

        if (platform::is_windows_vista_or_late())
            return getWindowsWeightsMap();

        return getArialWeightsMap();
    }

    const QssWeightsMapT& getPreSierraWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, "200");
            weightMap.emplace(FontWeight::Normal, "400");
            weightMap.emplace(FontWeight::Medium, "450");
            weightMap.emplace(FontWeight::Bold, "800");
        }

        return weightMap;
    }

    const QssWeightsMapT& getSierraWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, "300");
            weightMap.emplace(FontWeight::Normal, "400");
            weightMap.emplace(FontWeight::Medium, "450");
            weightMap.emplace(FontWeight::Bold, "800");
        }

        return weightMap;
    }

    const QssWeightsMapT& getWindowsWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, "200");
            weightMap.emplace(FontWeight::Normal, "400");
            weightMap.emplace(FontWeight::Medium, "450");
            weightMap.emplace(FontWeight::Bold, "800");
        }

        return weightMap;
    }

    const QssWeightsMapT& getArialWeightsMap()
    {
        static QssWeightsMapT weightMap;

        if (weightMap.empty())
        {
            weightMap.emplace(FontWeight::Light, "200");
            weightMap.emplace(FontWeight::Normal, "400");
            weightMap.emplace(FontWeight::Medium, "550");
            weightMap.emplace(FontWeight::Bold, "700");
        }

        return weightMap;
    }

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontWeight)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontWeight > FontWeight::Min);
        assert(_fontWeight < FontWeight::Max);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            return QString();
        }

        if (_fontFamily == FontFamily::ARIAL)
        {
            return appFontWeightQss(_fontWeight);
        }

        if (_fontFamily == FontFamily::HELVETICA_NEUE)
        {
            return appFontWeightQss(_fontWeight);
        }

        if (_fontFamily == FontFamily::SAN_FRANCISCO)
        {
            if (_fontWeight == FontWeight::Light)
                return QString();

            if (_fontWeight == FontWeight::Medium)
                return san_francisco::weight::mediumQss;

            return QString();
        }

        assert(!"unknown font family / style comnbination");
        return defaultAppFontQssWeight();
    }

    QFont::Weight icqWeight2QtWeight(const FontWeight _internalWeight)
    {
        assert(_internalWeight > FontWeight::Min);
        assert(_internalWeight < FontWeight::Max);

        switch (_internalWeight)
        {
            case FontWeight::Light: return QFont::Weight::Light;

            case FontWeight::Normal: return QFont::Weight::Normal;

            #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
            case FontWeight::Medium: return QFont::Weight::Medium;
            #else
            case FontWeight::Medium: return QFONT_WEIGHT_MEDIUM;
            #endif

            case FontWeight::Bold: return QFont::Weight::Bold;
        }

        assert(!"unknown enum value");
        return QFont::Weight::Normal;
    }

    QString segoeUiFamilyName(const FontWeight _weight)
    {
        assert(_weight > FontWeight::Min);
        assert(_weight < FontWeight::Max);

        QString familyName;
        familyName.reserve(1024);

        familyName += "Segoe UI";

        switch (_weight)
        {
            case FontWeight::Light:
                familyName += " Light";
                break;

            case FontWeight::Normal:
                break;

            case FontWeight::Medium:
                familyName += " Semibold";
                break;

            case FontWeight::Bold:
                break;

            default:
                assert(!"unknown font weight");
                break;
        }

        return familyName;
    }

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);

        switch (_fontFamily)
        {
            case FontFamily::SEGOE_UI:
                _font.setFamily("Segoe UI");
                return;

            case FontFamily::SAN_FRANCISCO:
                _font.setFamily(san_francisco::fontFamily);
                return;

            case FontFamily::HELVETICA_NEUE:
                _font.setFamily("Helvetica Neue");
                return;

            case FontFamily::ARIAL:
                _font.setFamily("Arial");
                return;

            default:
                assert(!"unexpected font family");
                return;
        }
    }

    void applyFontWeight(
        const FontFamily _fontFamily,
        const FontWeight _fontWeight,
        int32_t _sizePx,
        Out QFont &_font)
    {
        assert(_fontFamily > FontFamily::MIN);
        assert(_fontFamily < FontFamily::MAX);
        assert(_fontWeight > FontWeight::Min);
        assert(_fontWeight < FontWeight::Max);
        assert(_sizePx > 0);
        assert(_sizePx < 1000);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            switch (_fontWeight)
            {
                case FontWeight::Light:
                    _font.setFamily("Segoe UI Light");
                    return;

                case FontWeight::Normal:
                    return;

                case FontWeight::Medium:
                    _font.setFamily("Segoe UI Semibold");
                    return;

                case FontWeight::Bold:
                    _font.setWeight(
                        icqWeight2QtWeight(FontWeight::Bold));
                    return;

                default:
                    assert(!"unexpected font style");
                    return;
            }
        }

        if (_fontFamily == FontFamily::SAN_FRANCISCO)
        {
            return;
        }

        if (_fontFamily == FontFamily::HELVETICA_NEUE)
        {
            switch (_fontWeight)
            {
                case FontWeight::Normal:
                    _font.setWeight(QFont::Weight::Normal);
                    return;

                case FontWeight::Light:
                    _font.setWeight(QFont::Weight::Light);
                    return;

                case FontWeight::Medium:
                    _font.setWeight(QFont::Weight::DemiBold);
                    return;

                case FontWeight::Bold:
                    _font.setWeight(icqWeight2QtWeight(FontWeight::Bold));
                    return;
            }

            assert(!"unexpected font style");
            return;
        }

        if (_fontFamily == FontFamily::ARIAL)
        {
            switch (_fontWeight)
            {
            case FontWeight::Normal:
                _font.setWeight(QFont::Weight::Normal);
                return;

            case FontWeight::Light:
                _font.setWeight(QFont::Weight::Light);
                return;

            case FontWeight::Medium:
                _font.setWeight(QFont::Weight::DemiBold);
                return;

            case FontWeight::Bold:
                _font.setWeight(QFont::Weight::Bold);
                return;
            }

            assert(!"unexpected font style");
            return;
        }

        assert(!"unexpected font family");
        return;
    }

    namespace san_francisco
    {
        namespace weight
        {
            int32_t semibold(const int32_t _size)
            {
                assert(_size > 0);

                if (_size <= 16)
                {
                    return 64;
                }

                return 63;
            }
        }
    }
}

FONTS_NS_END
