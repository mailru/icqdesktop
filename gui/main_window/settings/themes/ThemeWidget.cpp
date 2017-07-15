#include "stdafx.h"
#include "ThemeWidget.h"
#include "ThemesModel.h"
#include "../../../controls/CustomButton.h"
#include "../../../utils/utils.h"

namespace Ui
{
    ThemeWidget::ThemeWidget(QWidget* _parent, QPixmap& _pixmap, ThemesModel* _themesModel, int _themeId)
        : QWidget(_parent)
        , pixmap_(QPixmap(_pixmap))
        , themesModel_(_themesModel)
        , themeId_(_themeId)
    {
        CustomButton *themeButton = new CustomButton(this, pixmap_);
        
        borderWidget_ = new QWidget(this);
        int w = _pixmap.width() / (platform::is_apple() ? 2 : 1);// / Utils::scale_bitmap(1);
        int h = _pixmap.height() / (platform::is_apple() ? 2 : 1);// / Utils::scale_bitmap(1);
        setFixedSize(w, h);
        themeButton->setFixedSize(w, h);
        themeButton->setCursor(Qt::PointingHandCursor);
        borderWidget_->setFixedSize(w, h);
        
        connect(themeButton, SIGNAL(clicked()), this, SLOT(onThemePressed()));
        Utils::ApplyStyle(borderWidget_, "background-color: transparent; border-style: solid; border-color: #579e1c; border-width: 4dip;");
        
        CustomButton *mark = new CustomButton(this, ":/resources/contr_wallpaper_select_100.png");
        Utils::ApplyStyle(mark, "background-color: transparent; border-color: transparent;");
        mark->setFixedSize(Utils::scale_value(32), Utils::scale_value(32));
        QVBoxLayout* vlayout = Utils::emptyVLayout();
        QHBoxLayout* hlayout = Utils::emptyHLayout();
        vlayout->setAlignment(Qt::AlignTop);
        hlayout->setAlignment(Qt::AlignRight);
        vlayout->addLayout(hlayout);
        hlayout->addWidget(mark);
        borderWidget_->setLayout(vlayout);
        borderWidget_->setAttribute( Qt::WA_TransparentForMouseEvents);

        Utils::grabTouchWidget(themeButton);
        Utils::grabTouchWidget(mark);
        
        setBorder(false);
    }
    
    void ThemeWidget::onThemePressed()
    {
        themesModel_->themeSelected(themeId_);
    }
    
    void ThemeWidget::setBorder(const bool _visible)
    {
        borderWidget_->setVisible(_visible);
    }
}
