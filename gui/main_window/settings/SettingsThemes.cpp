#include "stdafx.h"
#include "SettingsThemes.h"

#include "themes/ThemesPage.h"
#include "../../utils/utils.h"

namespace Ui
{
    ThemesSettingsWidget::ThemesSettingsWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        init();
    }
    
    void ThemesSettingsWidget::setBackButton(bool _doSet)
    {
        themesPage_->setBackButton(_doSet);
    }
    
    void ThemesSettingsWidget::setTargetContact(QString _aimId)
    {
        themesPage_->setTargetContact(_aimId);
    }
    
    void ThemesSettingsWidget::init()
    {        
        setStyleSheet("border: none; background-color: #ffffff;");
        
        auto mainLayout_ = Utils::emptyVLayout(this);
        
        themesPage_ = new ThemesPage(this, Utils::scale_value(8));
        
        QHBoxLayout *horizontalLayout = Utils::emptyHLayout(this);
        horizontalLayout->addWidget(themesPage_);

        mainLayout_->addLayout(horizontalLayout);
        
        Utils::grabTouchWidget(this);
    }
}