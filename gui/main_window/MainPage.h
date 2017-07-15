#pragma once

#include "../utils/InterConnector.h"

class QStandardItemModel;

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace Utils
{
    enum class CommonSettingsType;
    class SignalsDisconnector;
}

namespace Ui
{
    class main_page;
    class WidgetsNavigator;
    class ContactList;
    class SearchWidget;
    class CountrySearchCombobox;
    class VideoWindow;
    class VideoSettings;
    class IncomingCallWindow;
    class CallPanelMain;
    class ContactDialog;
    class SearchContactsWidget;
    class GeneralSettingsWidget;
    class SelectContactsWidget;
    class HistoryControlPage;
    class ThemesSettingsWidget;
    class ContextMenu;
    class FlatMenu;
    class IntroduceYourself;
    class LiveChatHome;
    class LiveChats;
    class TextEmojiWidget;
    class TopPanelWidget;
    class SnapsPage;
    class MainMenu;
    class SemitransparentWindowAnimated;
    class HorScrollableView;
    class CustomButton;

    class UnknownsHeader: public QWidget
    {
        Q_OBJECT

    public:
        explicit UnknownsHeader(QWidget* _parent = nullptr);
        ~UnknownsHeader();

    };

    class UnreadsCounter : public QWidget
    {
        Q_OBJECT

    public:
        UnreadsCounter(QWidget* _parent);

    protected:
        virtual void paintEvent(QPaintEvent* e);
    };

    class HeaderBack : public QPushButton
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized();
        
    public:
        HeaderBack(QWidget* _parent);

    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
    };

    class BackButton;

    enum class LeftPanelState
    {
        min,

        normal,
        picture_only,
        spreaded,

        max
    };

    class MainPage : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void startSearhInDialog(QString _aimid);
        void setSearchFocus();
        void snapsClose();
        void onAddContactClicked();
        void settingsClicked();

    private Q_SLOTS:
        void searchBegin();
        void searchEnd();
        void searchInputClear();
        void onContactSelected(QString _contact);
        void contactsClicked();
        void storiesClicked();
        void discoverClicked();
        void createGroupChat();
        void myProfileClicked();
        void aboutClicked();
        void contactUsClicked();
        // settings
        void onProfileSettingsShow(QString _uin);
        void onGeneralSettingsShow(int _type);
        void onThemesSettingsShow(bool, QString);
//        void onLiveChatsShow();
        //voip
        void onVoipShowVideoWindow(bool);
        void onVoipCallIncoming(const std::string&, const std::string&);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contacEx);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);

        void showPlaceholder(Utils::PlaceholdersType _placeholdersType);

        void post_stats_with_settings();
        void myInfo();
        void popPagesToRoot();
        void liveChatSelected();

        void spreadCL();
        void hideRecentsPopup();
        void searchActivityChanged(bool _isActive);

        void changeCLHeadToSearchSlot();
        void changeCLHeadToUnknownSlot();

        void openRecents();
        void showMainMenu();
        void compactModeChanged();
        void tabChanged(int);
        void themesSettingsOpen();
        void animFinished();
        void snapsChanged();
        void showSnapsChanged();
        void snapClicked(const QModelIndex& index);
        void headerBack();
        void showHeader(QString);

    private:
        MainPage(QWidget* _parent);
        static MainPage* _instance;

    public:
        static MainPage* instance(QWidget* _parent = 0);
        static void reset();
        ~MainPage();
        void selectRecentChat(QString _aimId);
        void recentsTabActivate(bool _selectUnread = false);
        void settingsTabActivate(Utils::CommonSettingsType _item = Utils::CommonSettingsType::CommonSettingsType_None);
        void hideInput();
        void cancelSelection();
        void clearSearchMembers();
        void openCreatedGroupChat();

        void raiseVideoWindow();

        void nextChat();
        void prevChat();
        void leftTab();
        void rightTab();
        void nextSnap();
        void nextUserSnap();
        void prevUserSnap();

        ContactDialog* getContactDialog() const;
        HistoryControlPage* getHistoryPage(const QString& _aimId) const;

        void insertTopWidget(const QString& _aimId, QWidget* _widget);
        void removeTopWidget(const QString& _aimId);

        void showSidebar(const QString& _aimId, int _page);
        bool isSidebarVisible() const;
        void setSidebarVisible(bool _show);
        void restoreSidebar();

        bool isContactDialog() const;
        bool isSnapsPageVisible() const;

        static int getContactDialogWidth(int _mainPageWidth);

        Q_PROPERTY(int anim READ getAnim WRITE setAnim)

        void setAnim(int _val);
        int getAnim() const;

        Q_PROPERTY(int clWidth READ getCLWidth WRITE setCLWidth)

        void setCLWidth(int _val);
        int getCLWidth() const;

        void showVideoWindow();

        void notifyApplicationWindowActive(const bool isActive);
        bool isVideoWindowActive();

        void setFocusOnInput();
        void clearSearchFocus();

        void onSendMessage(const QString& contact);

        void hideMenu();
        bool isMenuVisible() const;
        bool isMenuVisibleOrOpening() const;

        static QString getMainWindowQss();

    protected:
        virtual void resizeEvent(QResizeEvent* _event);

    private:

        QWidget* showNoContactsYetSuggestions(QWidget* _parent, std::function<void()> _addNewContactsRoutine);
        QWidget* showIntroduceYourselfSuggestions(QWidget* _parent);
        void animateVisibilityCL(int _newWidth, bool _withAnimation);
        void setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search = false, bool _force = false);
        void changeCLHead(bool _showUnknownHeader);

    private:
        UnknownsHeader                  *unknownsHeader_;

        ContactList*                    contactListWidget_;
        SearchWidget*                   searchWidget_;
        VideoWindow*                    videoWindow_;
        VideoSettings*                  videoSettings_;
        WidgetsNavigator*               pages_;
        ContactDialog*                  contactDialog_;
        QVBoxLayout*                    pagesLayout_;
        SearchContactsWidget*           searchContacts_;
        GeneralSettingsWidget*          generalSettings_;
        ThemesSettingsWidget*           themesSettings_;
        LiveChatHome*                   liveChatsPage_;
        SnapsPage*                      snapsPage_;
        QHBoxLayout*                    horizontalLayout_;
        QWidget*                        noContactsYetSuggestions_;
        QWidget*                        introduceYourselfSuggestions_;
        bool                            needShowIntroduceYourself_;
        QTimer*                         settingsTimer_;
        bool                            recvMyInfo_;
        QPropertyAnimation*             animCLWidth_;
        QPropertyAnimation*             animTopPanelHeight;
        QPropertyAnimation*             animBurger_;
        QWidget*                        clSpacer_;
        QVBoxLayout*                    contactsLayout;
        QHBoxLayout*                    originalLayout;
        QWidget*                        contactsWidget_;
        QHBoxLayout*                    clHostLayout_;
        LeftPanelState                  leftPanelState_;
        TopPanelWidget*                 myTopWidget_;
        MainMenu*                       mainMenu_;
        SemitransparentWindowAnimated*  semiWindow_;
        HorScrollableView*              snapsView_;
        CustomButton*                   searchButton_;
        QWidget*                        headerWidget_;
        HeaderBack*                    headerBack_;
        UnreadsCounter*                 counter_;
        QLabel*                         headerLabel_;
        bool                            NeedShowUnknownsHeader_;
        bool                            menuVisible_;
        int                             currentTab_;
        int                             anim_;

        std::map<std::string, std::shared_ptr<IncomingCallWindow> > incomingCallWindows_;
        std::unique_ptr<LiveChats> liveChats_;
        void destroyIncomingCallWindow(const std::string& _account, const std::string& _contact);
    };
}
