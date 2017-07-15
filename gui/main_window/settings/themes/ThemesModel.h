#ifndef ThemesModel_h
#define ThemesModel_h

namespace Ui
{
    class ThemesPage;
    class ThemesModel : public QObject
    {
        Q_OBJECT
        ThemesPage* themesPage_;
        QString targetContact_;
    private Q_SLOTS:
        void onThemesMeta();
        void onThemesMetaError();
        void im_created();

    public:
        ThemesModel(ThemesPage* _flowLayout);
        void themeSelected(int _themeId);
        void setTargetContact(QString _aimId);
    };
}

#endif /* ThemesModel_h */
