#include "../../../cache/themes/themes.h"

namespace Ui
{
    class FlowLayout;
    class ThemesModel;
    class BackButton;
    
    class ThemesPage : public QWidget
    {
        Q_OBJECT
        FlowLayout *flowLayout_;
        ThemesModel *themesModel_;
        themes::themesList themesList_;
        QMap<int,bool> loadedThemes_;
        BackButton* backButton_;
        QWidget *backButtonAndCaptionSpacer_;
        
        bool firstThemeAdded_;
        void checkFirstTheme_(themes::themePtr _theme);

    public:
        ThemesPage(QWidget* _parent, int _spacing);
        ~ThemesPage();

        void onThemeGot(themes::themePtr theme);
        void setTargetContact(QString _aimId);
        void setBackButton(bool _doSet);

    protected:
        virtual void resizeEvent(QResizeEvent* _e) override;

    private Q_SLOTS:
        void touchScrollStateChanged(QScroller::State);
        void backPressed();
    };

}