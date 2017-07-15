#include "stdafx.h"
#include "ThemesModel.h"

#include "ThemeWidget.h"
#include "ThemesPage.h"
#include "../../../theme_settings.h"
#include "../../../cache/themes/themes.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../../utils/utils.h"
#include "../../../utils/log/log.h"

#include "../.././../core_dispatcher.h"
#include "../../../../common.shared/themes_constants.h"

namespace Ui
{
    ThemesModel::ThemesModel(ThemesPage* _themesPage):
        QObject(_themesPage),
        themesPage_(_themesPage),
        targetContact_("")
    {
        connect(GetDispatcher(), SIGNAL(onThemesMeta()), this, SLOT(onThemesMeta()));
        connect(GetDispatcher(), SIGNAL(onThemesMetaError()), this, SLOT(onThemesMetaError()));
        connect(Ui::GetDispatcher(), SIGNAL(im_created()), this, SLOT(im_created()), Qt::QueuedConnection);

        if (Ui::GetDispatcher()->isImCreated())
        {
            im_created();
        }
    }
    
    void ThemesModel::im_created()
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        int themesScale = (int)ThemesScale100;
        if (Utils::is_mac_retina())
        {
            themesScale = ThemesScaleRetina;
        }
        else
        {
            double scaleCoefficient = Utils::getScaleCoefficient();
            if (scaleCoefficient == 1.0)
            {
                themesScale = ThemesScale100;
            }
            else if (scaleCoefficient == 1.25)
            {
                themesScale = ThemesScale125;
            }
            else if (scaleCoefficient == 1.5)
            {
                themesScale = ThemesScale150;
            }
            else if (scaleCoefficient == 2.0)
            {
                themesScale = ThemesScale200;
            }
        }
        collection.set_value_as_int("themes_scale", themesScale);
        GetDispatcher()->post_message_to_core("themes/meta/get", collection.get());
    }
    
    void ThemesModel::onThemesMeta()
    {
        auto themesDict = themes::loadedThemes();
        
        std::vector<themes::themePtr> themesVector;
    
        for (auto it = themesDict.begin(); it != themesDict.end(); ++it)
        {
            auto theme = it->second;
            if (theme)
            {
                themesVector.push_back(theme);
            }
        }
        std::sort(themesVector.begin(), themesVector.end(),[](themes::themePtr a, themes::themePtr b)
        {
            return a->get_position() < b->get_position();
        });
        
        for (auto it = themesVector.begin(); it != themesVector.end(); ++it)
        {
            auto theme = *it;
            themesPage_->onThemeGot(theme);
        }
    }

    void ThemesModel::onThemesMetaError()
    {
        static int triesCount = 0;
        if ((triesCount++) < 5)
            QTimer::singleShot(100, [this]()
        {
            im_created();
        });
    }
    
    void ThemesModel::themeSelected(int _themeId)
    {
        get_qt_theme_settings()->themeSelected(_themeId, targetContact_);
    }
    
    void ThemesModel::setTargetContact(QString _aimId)
    {
        targetContact_ = _aimId;
    }
}
