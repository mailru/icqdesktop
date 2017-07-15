#include "stdafx.h"
#include "theme_settings.h"

#include "core_dispatcher.h"
#include "main_window/MainWindow.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/history_control/HistoryControlPageThemePanel.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/log/log.h"

namespace Ui
{
    qt_theme_settings::qt_theme_settings() 
        : defaultTheme_(NULL)
        , themesIdToLoad_(std::vector<int>())
        , contactsThemes_(QMap<QString,int>())
        , contactsThemesBackup_(QMap<QString,int>())
        , settingInProcess_(false)
        , isLoaded_(false)
    {
        themeRequest_.reset();
        connect(Ui::GetDispatcher(), SIGNAL(onTheme(int,bool)), this, SLOT(onThemeImageArrived(int,bool)));
    }
    
    qt_theme_settings* get_qt_theme_settings()
    {
        static std::unique_ptr<qt_theme_settings> settings(new qt_theme_settings());
        return settings.get();
    }
    
    qt_theme_settings::~qt_theme_settings()
    {
    }
    
    themes::themePtr qt_theme_settings::initialTheme()
    {
        if (!initialTheme_)
        {
            initialTheme_ = std::make_shared<themes::theme>();
        }
        return initialTheme_;
    }
    
    void qt_theme_settings::postDefaultThemeIdToCore(const int _themeId) const
    {
        Ui::gui_coll_helper clColl(GetDispatcher()->create_collection(), true);
        clColl.set_value_as_int("id", _themeId);
        
        GetDispatcher()->post_message_to_core("themes/default/id", clColl.get());
    }
    
    void qt_theme_settings::unserialize(core::coll_helper _collection)
    {
        core::iarray* valuesArray = _collection.get_value_as_array("values");
        if (!valuesArray)
        {
            assert(false);
            return;
        }
        
        QString tintColor;
        QByteArray imageDataArray;
        QByteArray thumbDataArray;
        QString contactsThemes;
        int32_t themeId = -1;
        bool tile = false;

        for (int i = 0; i < valuesArray->size(); ++i)
        {
            const core::ivalue* val = valuesArray->get_at(i);
            
            gui_coll_helper collVal(val->get_as_collection(), false);
            std::string name = collVal.get_value_as_string("name");
            core::istream* idata = collVal.get_value_as_stream("value");
            int len = idata->size();
            
            const char* data = (char *)idata->read(len);
            
            if (name == "id")
            {
                if (len == sizeof(int32_t))
                {
                    themeId = *((int32_t*) data);
                }
                else if (len == 8)
                {
                    themeId = (int32_t) *((int64_t*) data);
                }
                else
                {
                    assert(false);
                }
            }
            else if (name == "image")
            {
                imageDataArray = QByteArray(data, len);
            }
            else if (name == "thumb")
            {
                thumbDataArray = QByteArray(data, len);
            }
            else if (name == "tint_color")
            {
                tintColor = QString(std::string(data, len).c_str());
            }
            else if (name == "contacts_themes")
            {
                contactsThemes = QString(std::string(data, len).c_str());
            }
            else if (name == "tile")
            {
                if (len == 1)
                {
                    tile = *(data) & 0xFF;
                }
            }
        }
        
        if (themeId != -1)
        {
            defaultTheme_ = std::make_shared<themes::theme>(themeId, tintColor, imageDataArray, thumbDataArray, tile);
        }
        if (contactsThemes.length() > 1)
        {
            QStringList list = contactsThemes.split(",");
            for (int i = 0; i < list.count(); ++i)
            {
                QString item = list[i];
                QStringList contactTheme = item.split(":");
                if (contactTheme.count() == 2)
                {
                    QString aimId = contactTheme[0];
                    int themeId = contactTheme[1].toInt();
                    if (themeId >= 0)
                    {
                        contactsThemes_[aimId] = themeId;
                        themesIdToLoad_.push_back(themeId);
                    }
                }
            }
        }
        setIsLoaded(true);
    }
    
    std::unordered_map<int, int> qt_theme_settings::getThemeCounts()
    {
        std::unordered_map<int, int> result;

        for (const auto& contactTheme : contactsThemes_)
        {
            result[contactTheme] += 1;
        }

        return result;
    }

    void qt_theme_settings::onThemeImageArrived(int _themeId, bool _failed)
    {
        if (!_failed)
        {
            auto theme = themeForId(_themeId);
            if (themeRequest_.requestedId_ == _themeId && themeRequest_.callback_)
            {
                themeRequest_.callback_(theme, true);
                themeRequest_.reset();
            }
        }
        else if (themeRequest_.requestedId_ == _themeId && themeRequest_.callback_)
        {
            themeRequest_.callback_(0, false);
            themeRequest_.reset();
        }
    }
    
    void qt_theme_settings::themesDataUnserialized()
    {
        if (defaultTheme_)
        {
            // replace temporary preloaded theme
            auto theme = Ui::themes::loadedThemes()[defaultTheme_->get_id()];
            if (theme)
            {
                defaultTheme_ = theme;
            }
            else
            {
                defaultTheme_ = initialTheme();
            }
        }
        else
        {
            processNoDefaultThemeCase();
        }
    }
    
    void qt_theme_settings::processNoDefaultThemeCase()
    {
        defaultTheme_ = initialTheme();
//        defaultTheme_ = Ui::themes::loadedThemes().begin()->second;   // consider first as default
//        if (defaultTheme_)
//        {
//            this->postDefaultThemeIdToCore(defaultTheme_->get_id());
//            requestThemeImage(defaultTheme_->get_id(), [this](themes::themePtr theme)
//            {
//                if (theme)
//                {
//                    defaultTheme_ = theme;
//                }
//            });
//        }
    }
    
    // preload used themes images
    void qt_theme_settings::flushThemesToLoad()
    {
        for (int i = 0; i < (int) themesIdToLoad_.size(); ++i)
        {
            int theme_id_to_load = themesIdToLoad_[i];
            Ui::GetDispatcher()->getTheme(theme_id_to_load);
        }
        themesIdToLoad_.clear();
    }
    
    bool qt_theme_settings::setThemeToWindow(std::shared_ptr<themes::theme> _theme)
    {
        if (_theme)
        {
            if (_theme->isImageLoaded())
            {
                QPixmap pixmap = _theme->getImage();
                
                QPainter pixPaint(&pixmap);
                QBrush brush(_theme->get_tint_color());
                pixPaint.fillRect(pixmap.rect(), brush);
                
                Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(pixmap, _theme->is_tile());
            }
            else
            {
                return false;
            }
        }
        else
        {
            QPixmap p;
            Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(p, true);
        }
        return true;
    }
    
    void qt_theme_settings::requestThemeImage(int _themeId, std::function<void(themes::themePtr, bool)> _callback)
    {
        Ui::GetDispatcher()->getTheme(_themeId);
        themeRequest_.requestedId_ = _themeId;
        themeRequest_.callback_ = _callback;
    }
    
    void qt_theme_settings::setOrLoadDefaultTheme()
    {
        if (!setThemeToWindow(defaultTheme_))
        {
            requestThemeImage(defaultTheme_->get_id(), [this](themes::themePtr theme, bool success)
            {
                if (theme && success)
                {
                    this->setThemeToWindow(theme);
                }
            });
        }
    }
    
    int qt_theme_settings::contactOpenned(QString _aimId)
    {
        auto theme = themeForContact(_aimId);
        if (theme && theme->get_id() >= 0)
        {
            if (!setThemeToWindow(theme))
            {
                requestThemeImage(theme->get_id(), [this](themes::themePtr theme, bool success)
                {
                    if (theme && success)
                    {
                        this->setThemeToWindow(theme);
                    }
                    else if (!success)
                    {
                        QPixmap p;
                        Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(p, true);
                    }
                });
            }
            return theme->get_id();
        }
        return -1;
    }
    
    int qt_theme_settings::themeIdForContact(QString _aimId)
    {
        return themeForContact(_aimId)->get_id();
    }
    
    themes::themePtr qt_theme_settings::themeForContact(QString _aimId)
    {
        if (!contactsThemes_.contains(_aimId))
        {
            if (defaultTheme_)
            {
                return defaultTheme_;
            }
        }
        else if (contactsThemes_[_aimId] > 0)
        {
            int themeId = contactsThemes_[_aimId];
            auto theme = Ui::themes::loadedThemes()[themeId];
            if (theme)
            {
                return theme;
            }
        }
        return initialTheme();
    }
    
    std::shared_ptr<themes::theme> qt_theme_settings::themeForId(int _themeId)
    {
        if (_themeId == 0)
        {
            return initialTheme();
        }
        
        auto themes = Ui::themes::loadedThemes();
        auto theme = themes[_themeId];
        if (theme)
        {
            return theme;
        }
        return defaultTheme_;
    }
    
    void qt_theme_settings::setDefaultTheme(std::shared_ptr<themes::theme> _theme)
    {
        defaultTheme_ = _theme;
        if (!setThemeToWindow(defaultTheme_))
        {
            requestThemeImage(_theme->get_id(), [this](themes::themePtr theme, bool success)
            {
                if (theme && success)
                {
                    this->setThemeToWindow(theme);
                }
            });
        }
        int themeId = _theme->get_id();
        contactsThemes_.clear();
        postContactsThemesToCore();
        postDefaultThemeIdToCore(themeId);
    }
    
    void qt_theme_settings::themeSelected(int _themeId, const QString& _targetContact)
    {
        auto theme = themeForId(_themeId);
        QString contactToOpen = _targetContact;
        bool showSetThemeToCurrent = true;
        if (contactToOpen == "")
        {
            contactToOpen = Logic::getContactListModel()->contactToTryOnTheme();
            showSetThemeToCurrent = false;
        }
        
        Logic::getContactListModel()->setCurrent(contactToOpen, -1, true, false, [this, theme, _targetContact, showSetThemeToCurrent, contactToOpen](HistoryControlPage *page)
        {
            // consider peculiar case when themeSelected gets called without ThemePanelChoice-callback from previous call
            if (settingInProcess_)
            {
                restoreThemesMapping();
            }
            settingInProcess_ = true;
            
            page->update(_targetContact);
            saveThemesMapping();
            setThemeIdForContact(theme->get_id(), contactToOpen, false);
            page->updateWidgetsTheme();
            page->showThemesTopPanel(showSetThemeToCurrent, [this, theme, _targetContact, page, contactToOpen](ThemePanelChoice res) 
            {
                if (res == ThemePanelCancel || res == ThemePanelBackToSettings)
                {
                    restoreThemesMapping();
                    setThemeIdForContact(themeIdForContact(contactToOpen), contactToOpen);
                    page->updateWidgetsTheme();
                    unloadUnusedThemesImages();
                    if (_targetContact.isEmpty())
                        emit Utils::InterConnector::instance().themesSettingsOpen();
                }
                else if (res == ThemePanelSet)
                {
                    setThemeIdForContact(theme->get_id(), contactToOpen);
                }
                else if (res == ThemePanelSetToAll)
                {
                    setDefaultTheme(theme);
                }
                settingInProcess_ = false;
            });
        });
    }
    
    void qt_theme_settings::setThemeIdForContact(int _themeId, const QString& _aimId, const bool _saveContactsThemes)
    {
        auto theme = themeForId(_themeId);
        if (_aimId != "")
        {
            if (!setThemeToWindow(theme))
            {
                requestThemeImage(_themeId, [this](themes::themePtr theme, bool success)
                {
                    if (theme && success)
                    {
                        this->setThemeToWindow(theme);
                    }
                    else if (!success)
                    {
                        QPixmap p;
                        Utils::InterConnector::instance().getMainWindow()->setBackgroundPixmap(p, true);
                    }
                });
            }
            contactsThemes_[_aimId] = _themeId;
            if (_saveContactsThemes)
            {
                postContactsThemesToCore();
            }
        }
        else if (theme)
        {
            setDefaultTheme(theme);
        }
        
    }
    
    QString qt_theme_settings::serializedContactsThemes() const
    {
        if (contactsThemes_.empty())
            return "-";

        QString result;

        for (auto iter = contactsThemes_.begin(); iter != contactsThemes_.end(); ++iter)
        {
            if (!result.isEmpty())
                result += ",";

            QString aimId = iter.key();
            int themeId = iter.value();
            result.append(QString("%1:%2").arg(aimId).arg(themeId));
        }
        
        return result;
    }
    
    void qt_theme_settings::postContactsThemesToCore() const
    {
        Ui::gui_coll_helper clColl(GetDispatcher()->create_collection(), true);
        
        core::ifptr<core::istream> data_stream(clColl->create_stream());
        
        QString result = serializedContactsThemes();

        uint size = result.toLatin1().size();
        if (size)
        {
            data_stream->write((const uint8_t*) result.toLatin1().data(), size);
        }
        
        clColl.set_value_as_qstring("name", "contacts_themes");
        clColl.set_value_as_stream("value", data_stream.get());

        GetDispatcher()->post_message_to_core("themes/settings/set", clColl.get());
    }
    
    void qt_theme_settings::saveThemesMapping()
    {
        contactsThemesBackup_ = contactsThemes_;
        if (defaultTheme_)
        {
            contactsThemesBackup_["default"] = defaultTheme_->get_id();
        }
    }
    
    void qt_theme_settings::restoreThemesMapping()
    {
        int defaultThemeId = contactsThemesBackup_["default"];
        
        defaultTheme_ = themeForId(defaultThemeId);
        contactsThemesBackup_.remove("default");
        
        contactsThemes_ = contactsThemesBackup_;
    }
    
    void qt_theme_settings::unloadUnusedThemesImages()
    {
        std::set<int> used(contactsThemes_.begin(), contactsThemes_.end());
        if (defaultTheme_)
        {
            used.insert(defaultTheme_->get_id());
        }
        Ui::themes::unloadUnusedThemesImages(used);
    }
}
