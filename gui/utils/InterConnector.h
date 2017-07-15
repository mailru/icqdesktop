#pragma once

namespace Ui
{
    class MainWindow;
    class HistoryControlPage;
    class ContactDialog;
}

namespace Utils
{
    enum class CommonSettingsType
    {
        min = -1,

        CommonSettingsType_None,
        CommonSettingsType_Profile,
        CommonSettingsType_General,
        CommonSettingsType_VoiceVideo,
        CommonSettingsType_Notifications,
        CommonSettingsType_Themes,
        CommonSettingsType_About,
        CommonSettingsType_ContactUs,
        CommonSettingsType_AttachPhone,
        CommonSettingsType_AttachUin,

        max
    };

    enum class PlaceholdersType
    {
        min = -1,

        PlaceholdersType_FindFriend,
        PlaceholdersType_HideFindFriend,

        PlaceholdersType_IntroduceYourself,
        PlaceholdersType_SetExistanseOnIntroduceYourself,
        PlaceholdersType_SetExistanseOffIntroduceYourself,

        max
    };

    class InterConnector : public QObject
    {
        Q_OBJECT

    public:

Q_SIGNALS:
        void profileSettingsShow(QString uin);
        void profileSettingsBack();

        void themesSettingsOpen();

        void generalSettingsShow(int type);
        void generalSettingsBack();
        void themesSettingsShow(bool, QString);
        void themesSettingsBack();
        void profileSettingsUpdateInterface();
        
        void generalSettingsContactUsShown();

        void attachPhoneBack();
        void attachUinBack();

        void makeSearchWidgetVisible(bool);
        void showIconInTaskbar(bool _show);

        void popPagesToRoot();

        void showPlaceholder(PlaceholdersType);
        void showNoContactsYet();
        void hideNoContactsYet();

        void showNoRecentsYet();
        void hideNoRecentsYet();

        void showNoSearchResults();
        void hideNoSearchResults();

        void showSearchSpinner();
        void hideSearchSpinner();
        void disableSearchInDialog();
        void repeatSearch();
        void dialogClosed(QString _aimid);

        void resetSearchResults();

        void onThemes();
        void cancelTheme(QString);
        void setToAllTheme(QString);
        void setTheme(QString);

        void closeAnyPopupWindow();
        void closeAnyPopupMenu();
        void closeAnySemitransparentWindow();

        void forceRefreshList(QAbstractItemModel *, bool);
        void updateFocus();
        void liveChatsShow();
        
        void schemeUrlClicked(QString);

        void setAvatar(qint64 _seq, int error);
        void setAvatarId(QString);

        void historyControlPageFocusIn(QString);
        
        void unknownsGoSeeThem();
        void unknownsGoBack();
        void unknownsDeleteThemAll();
        
        void liveChatSelected();

        void activateNextUnread();

        void historyControlReady(QString, qint64 _message_id, qint64 _last_read_msg);

        void imageCropDialogIsShown(QWidget *);
        void imageCropDialogIsHidden(QWidget *);
        void imageCropDialogMoved(QWidget *);
        void imageCropDialogResized(QWidget *);

        void startSearchInDialog(QString);
        void setSearchFocus();

        void searchEnd();
        void myProfileBack();

        void compactModeChanged();
        void showSnapsChanged();
        void mailBoxOpened();

        void logout();
        void authError(const int _error);
        void contacts();
        void showHeader(QString);

    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        Ui::MainWindow* getMainWindow() const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;

        void insertTopWidget(const QString& aimId, QWidget* widget);
        void removeTopWidget(const QString& aimId);

        void showSidebar(const QString& aimId, int page);
        void setSidebarVisible(bool show);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void setDragOverlay(bool enable);
        bool isDragOverlay() const;

        void setUrlHandler();
        void unsetUrlHandler();

        void setFocusOnInput();
        void onSendMessage(const QString&);

        int getSemiwindowsCount() const;
        void incSemiwindowsCount();
        void decSemiwindowsCount();

        bool isSemiWindowsTouchSwallowed() const;
        void setSemiwindowsTouchSwallowed(bool _val);

    public Q_SLOTS:
        void open_url(const QUrl& url);

    private:
        bool parseLocalUrl(const QString& _urlString);

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;

        int semiWindowsCount_;
        bool semiWindowsTouchSwallowed_;
    };
}
