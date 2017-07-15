#pragma once

#include "../../../corelib/collection_helper.h"

#include "../../core_dispatcher.h"
#include "../../namespaces.h"

UI_THEMES_NS_BEGIN

class theme
{
    int id_;
    int position_;
    QColor tint_color_;
    QPixmap thumb_;
    QPixmap image_;
    bool tile_;
    bool isImageLoaded_;
    theme(int);
public:
    theme();
    theme(int _id, QString _tintColor, QByteArray& _imageData, QByteArray& _thumbData, const bool _tile);
    void unserialize(core::coll_helper _coll);
    void loadThumb(char* _data, int32_t _size);

    QPixmap getThumb() const
    {
        return thumb_;
    }
    void setThumb(QPixmap _pixmap)
    {
        thumb_ = _pixmap;
    }

    int get_id() const
    {
        return id_;
    }
    bool is_tile() const
    {
        return tile_;
    }
    QPixmap getImage() const
    {
        QPixmap ret = image_;
        return ret;
    }
    int get_position() const
    {
        return position_;
    }
    QColor get_tint_color();

    static theme getDefaultTheme();

    void setImage(QPixmap &_pixmap)
    {
        image_ = QPixmap(_pixmap);
        if (!image_.isNull())
        {
            isImageLoaded_ = true;
        }
    }
    bool isImageLoaded() const
    {
        return isImageLoaded_;
    }

    void unloadImage();

    static QColor colorFromString(const char* _colorString);

    struct bubble
    {
        QColor bg1_color_;
        QColor bg2_color_;
        QColor time_color_;
        QColor info_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    };
    bubble incoming_bubble_;
    bubble outgoing_bubble_;

    struct date
    {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } date_;

    struct preview_stickers
    {
        QColor time_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } preview_stickers_;

    struct chat_event
    {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } chat_event_;

    struct contact_name
    {
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } contact_name_;

    struct new_messages_plate
    {
        QColor bg_color_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } new_messages_plate_;

    struct typing
    {
        int light_gif_;
        QColor text_color_;
        void unserialize(Ui::gui_coll_helper& _coll);
    } typing_;
};

typedef std::shared_ptr<theme> themePtr;
typedef std::unordered_map<int, themePtr> themesDict;
typedef std::vector<themePtr> themesList;

class cache
{
public:
    cache(){}

    void unserialize(const core::coll_helper& _coll);
    void setThemeData(core::coll_helper _coll);
    themesDict get_themes()
    {
        return themes_;
    }
    void unloadUnusedThemesImages(const std::set<int>& _used);

private:
    themesDict themes_;
};

void unserialize(core::coll_helper _coll);
void setThemeData(core::coll_helper _coll);
void unloadUnusedThemesImages(const std::set<int>& _used);
themesDict loadedThemes();

UI_THEMES_NS_END
