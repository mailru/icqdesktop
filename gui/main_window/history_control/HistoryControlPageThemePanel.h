#ifndef HistoryControlPageThemePanel_h
#define HistoryControlPageThemePanel_h

namespace Ui
{
    class BackButton;
    class CustomButton;
    class HistoryControlPage;
    class LabelEx;

    enum ThemePanelChoice: int
    {
        ThemePanelCancel,
        ThemePanelSet,
        ThemePanelSetToAll,
        ThemePanelBackToSettings
    };
    
    typedef std::function<void(ThemePanelChoice)> ThemePanelCallback;
    
    class HistoryControlPageThemePanel : public QWidget
    {
        Q_OBJECT
        
        virtual void paintEvent(QPaintEvent*) override;
        
        LabelEx *cancelButton_;
        QPushButton *setToAllButton_;
        QPushButton *setButton_;
        BackButton *backFromThemeButton_;
        
        HistoryControlPage *historyControlPage_;
        ThemePanelCallback callback_;
        
        bool selectionThemeFromSettings_;
        
    public:
        HistoryControlPageThemePanel(HistoryControlPage* _parent);
        ~HistoryControlPageThemePanel();
        
        void setShowSetThemeButton(const bool _show);
        
        void setCallback(ThemePanelCallback);
        bool settingThemeToAll_;
        void setSelectionToAll(bool _fromSettings);

    public Q_SLOTS:
        void cancelThemePressed();

    private Q_SLOTS:
        void backFromThemePressed();
        void setToAllThemePressed();
        void setThemePressed();
    };
}

#endif /* HistoryControlPageThemePanel_h */
