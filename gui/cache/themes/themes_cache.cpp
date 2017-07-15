#include "stdafx.h"
#include "themes.h"
#include "../../theme_settings.h"
#include "../../main_window/history_control/MessageStyle.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"

UI_THEMES_NS_BEGIN
std::unique_ptr<cache> t_cache;

QPixmap CopyOnWriteLoadFromData(const QByteArray& _data, const char* _format = 0, Qt::ImageConversionFlags _flags = Qt::AutoColor)
{
    QPixmap pixmap;
    pixmap.loadFromData(_data, _format, _flags);
    return pixmap;
}

QPixmap CopyOnWriteLoadFromData(const uchar* _data, uint _len, const char* _format = 0, Qt::ImageConversionFlags _flags = Qt::AutoColor)
{
    QPixmap pixmap;
    pixmap.loadFromData(_data, _len, _format, _flags);
    return pixmap;
}

theme::theme(int _id, QString _tintColorString, QByteArray& _imageData, QByteArray& _thumbData, const bool _tile) 
    : position_(0)
    , isImageLoaded_(false)
{
    *this = getDefaultTheme();
    tile_ = _tile;
    id_ = _id;

    tint_color_ = colorFromString(_tintColorString.toLatin1().data());
    image_ = CopyOnWriteLoadFromData(_imageData);
    
    if (!image_.isNull())
    {
        isImageLoaded_ = true;
    }
    
    thumb_ = CopyOnWriteLoadFromData(_thumbData);
    Utils::check_pixel_ratio(thumb_);
}

QColor theme::colorFromString(const char* _colorString)
{
    return QColor("#" + QString(_colorString));
}

theme::theme() : image_(QPixmap()), thumb_(QPixmap()), tile_(false), id_(0), position_(0), isImageLoaded_(true)
{
    *this = getDefaultTheme();
}

theme::theme(int) 
{}

theme theme::getDefaultTheme()
{
    static bool isDefaultThemeConstructed = false;

    static theme defaultTheme(0);
    if (!isDefaultThemeConstructed)
    {
        defaultTheme.tile_ = true;
        defaultTheme.tint_color_ = Qt::transparent;

        defaultTheme.incoming_bubble_.bg1_color_ = MessageStyle::getIncomingBodyColorA();
        defaultTheme.incoming_bubble_.bg2_color_ = MessageStyle::getIncomingBodyColorB();
        defaultTheme.incoming_bubble_.time_color_ = MessageStyle::getTimeColor();
    
        defaultTheme.outgoing_bubble_.bg1_color_ = MessageStyle::getOutgoingBodyColorA();
        defaultTheme.outgoing_bubble_.bg2_color_ = MessageStyle::getOutgoingBodyColorB();
        defaultTheme.outgoing_bubble_.time_color_ = MessageStyle::getTimeColor();
    
        defaultTheme.preview_stickers_.time_color_ = MessageStyle::getTimeColor();
    
        defaultTheme.date_.bg_color_ = QColor("#767676");
        defaultTheme.date_.text_color_ = QColor("#ffffff");
    
        defaultTheme.chat_event_.bg_color_ = Qt::transparent;
        defaultTheme.chat_event_.text_color_ = QColor("#767676");
    
        defaultTheme.contact_name_.text_color_ = MessageStyle::getSenderColor();

        QColor newMessagesColor("#579e1c");
        newMessagesColor.setAlphaF(0.5);
        defaultTheme.new_messages_plate_.bg_color_ = newMessagesColor;
        defaultTheme.new_messages_plate_.text_color_ = QColor("#ffffff");
    
        defaultTheme.typing_.text_color_ = MessageStyle::getTypingColor();
        defaultTheme.typing_.light_gif_ = 0;
    
        defaultTheme.image_ = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_100.png"));
        defaultTheme.thumb_ = QPixmap(Utils::parse_image_name(":/resources/main_window/pat_thumb_100.png"));
    
        Utils::check_pixel_ratio(defaultTheme.image_);
        Utils::check_pixel_ratio(defaultTheme.thumb_);
        
        isDefaultThemeConstructed = true;
    }
    return defaultTheme;
}

QColor theme::get_tint_color()
{
    return tint_color_;
}

void theme::date::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
}

void theme::bubble::unserialize(Ui::gui_coll_helper& _coll)
{
    bg1_color_ = colorFromString(_coll.get_value_as_string("bg1_color"));
    bg2_color_ = colorFromString(_coll.get_value_as_string("bg2_color"));
    time_color_ = colorFromString(_coll.get_value_as_string("time_color"));
}

void theme::preview_stickers::unserialize(Ui::gui_coll_helper& _coll)
{
    time_color_ = colorFromString(_coll.get_value_as_string("time_color"));
}

void theme::chat_event::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
}

void theme::contact_name::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
}

void theme::new_messages_plate::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    bg_color_ = colorFromString(_coll.get_value_as_string("bg_color"));
}

void theme::typing::unserialize(Ui::gui_coll_helper& _coll)
{
    text_color_ = colorFromString(_coll.get_value_as_string("text_color"));
    light_gif_ = _coll.get_value_as_int("light_gif");
}

void theme::unserialize(core::coll_helper _coll)
{
    id_ = _coll.get_value_as_int("id");
    position_ = _coll.get_value_as_int("position");
    isImageLoaded_ = false;
    tint_color_ = colorFromString(_coll.get_value_as_string("tint_color"));
    tile_ = _coll.get_value_as_bool("tile");
    
    if (_coll.is_value_exist("thumb"))
    {
        core::istream* thumbStream = _coll.get_value_as_stream("thumb");
        if (thumbStream)
        {
            int32_t thumbSize = thumbStream->size();
            loadThumb((char*)thumbStream->read(thumbSize), thumbSize);
        }
    }

    Ui::gui_coll_helper coll_incoming_bubble(_coll.get_value_as_collection("incoming_bubble"), false);
    incoming_bubble_.unserialize(coll_incoming_bubble);
    
    Ui::gui_coll_helper coll_outgoing_bubble(_coll.get_value_as_collection("outgoing_bubble"), false);
    outgoing_bubble_.unserialize(coll_outgoing_bubble);
    
    Ui::gui_coll_helper coll_preview_stickers(_coll.get_value_as_collection("preview_stickers"), false);
    preview_stickers_.unserialize(coll_preview_stickers);
    
    Ui::gui_coll_helper coll_data(_coll.get_value_as_collection("date"), false);
    date_.unserialize(coll_data);
    
    Ui::gui_coll_helper coll_chat_event(_coll.get_value_as_collection("chat_event"), false);
    chat_event_.unserialize(coll_chat_event);
    
    Ui::gui_coll_helper coll_contact_name(_coll.get_value_as_collection("contact_name"), false);
    contact_name_.unserialize(coll_contact_name);
    
    Ui::gui_coll_helper coll_new_messages_plate(_coll.get_value_as_collection("new_messages_plate"), false);
    new_messages_plate_.unserialize(coll_new_messages_plate);
    
    Ui::gui_coll_helper coll_typing(_coll.get_value_as_collection("typing"), false);
    typing_.unserialize(coll_typing);
}

void cache::unserialize(const core::coll_helper& _coll)
{
    core::iarray* themes = _coll.get_value_as_array("themes");
    if (!themes)
        return;
    
    auto defaultTheme = get_qt_theme_settings()->getDefaultTheme();
    int defaultThemeId = -1;
    if (defaultTheme)
    {
        defaultThemeId = defaultTheme->get_id();
    }
    
    for (int32_t i = 0; i < themes->size(); i++)
    {
        core::coll_helper collSet(themes->get_at(i)->get_as_collection(), false);
        auto themeToInsert = std::make_shared<theme>();
        themeToInsert->unserialize(collSet);

        int themeId = themeToInsert->get_id();
        themes_[themeId] = themeToInsert;
    }
    
    get_qt_theme_settings()->themesDataUnserialized();
}

void cache::setThemeData(core::coll_helper _coll)
{
    const qint32 themeId = _coll.get_value_as_int("theme_id");
    core::istream* imageStream = _coll.get_value_as_stream("image");
    
    auto &theme = themes_[themeId];
    if (theme && imageStream)
    {
        int32_t imageSize = imageStream->size();
        if (imageSize > 0)
        {
            auto p = CopyOnWriteLoadFromData((const uchar*)imageStream->read(imageSize), imageSize);
            theme->setImage(p);
        }
    }
}

void cache::unloadUnusedThemesImages(const std::set<int>& _used)
{
    for (auto it = themes_.begin(); it != themes_.end(); ++it)
    {
        auto theme = it->second;
        int themeId = theme->get_id();
        if (_used.count(themeId) == 0 && theme->isImageLoaded())
        {
            theme->unloadImage();
        }
    }
}

void theme::loadThumb(char* _data, int32_t _size)
{
    thumb_ = CopyOnWriteLoadFromData((const uchar*)_data, _size);
    Utils::check_pixel_ratio(thumb_);
}

void theme::unloadImage()
{
    isImageLoaded_ = false;
    image_ = QPixmap();
}

cache& get_cache()
{
    if (!t_cache)
        t_cache.reset(new cache());
    
    return (*t_cache);
}

void unserialize(core::coll_helper _coll)
{
    get_cache().unserialize(_coll);
}

void unloadUnusedThemesImages(const std::set<int>& _used)
{
    get_cache().unloadUnusedThemesImages(_used);
}

void setThemeData(core::coll_helper _coll)
{
    get_cache().setThemeData(_coll);
}

themesDict loadedThemes()
{
    return get_cache().get_themes();
}

UI_THEMES_NS_END
