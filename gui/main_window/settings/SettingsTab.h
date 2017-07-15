#pragma once

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
    
    class SettingsTab : public QWidget
    {
        class UI;

        Q_OBJECT

    public Q_SLOTS:
        void settingsProfileClicked();
        void settingsGeneralClicked();
        void settingsVoiceVideoClicked();
        void settingsNotificationsClicked();
        void settingsThemesClicked();

    private Q_SLOTS:
        void compactModeChanged();

    private:
        std::unique_ptr< UI > ui_;
        Utils::CommonSettingsType currentSettingsItem_;
        bool isCompact_;

    public:
        SettingsTab(QWidget* _parent);
        ~SettingsTab() throw();
        
        void cleanSelection();
        void setCompactMode(bool isCompact, bool force = false);
        
    private:
        void updateSettingsState();
    };
}