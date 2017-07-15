#pragma once

namespace Ui
{
    class ThemesPage;
    class ThemesSettingsWidget: public QWidget
    {
        Q_OBJECT

        ThemesPage *themesPage_;

        void init();
        
    public:
        ThemesSettingsWidget(QWidget* _parent);

        void setBackButton(bool _doSet);
        void setTargetContact(QString _aimId);
    };
    
    
}

