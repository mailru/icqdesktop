#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"

namespace Logic
{
    class ChatMembersModel;
    class ContactListItemDelegate;
}

namespace Ui
{
    class CustomButton;
    class ContactAvatarWidget;
    class TextEditEx;
    class BackButton;
    class ContactList;
    class SearchWidget;
    class LabelEx;
    class LineWidget;
    class ClickedWidget;
    class ActionButton;
    class FocusableListView;

    class MenuPage : public SidebarPage
    {
        Q_OBJECT
    Q_SIGNALS:
        void updateMembers();

    public:
        MenuPage(QWidget* parent);
        virtual void initFor(const QString& aimId);

    public Q_SLOTS:
        void allMemebersClicked();

    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent *e);
        virtual void updateWidth();

    private Q_SLOTS:
        void contactChanged(QString);
        void favoritesClicked();
        void copyLinkClicked();
        void themesClicked();
        void privacyClicked();
        void eraseHistoryClicked();
        void ignoreClicked();
        void quitClicked();
        void notificationsChecked(int);
        void addToChatClicked();
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatBlocked(QList<Data::ChatMemberInfo>);
        void chatPending(QList<Data::ChatMemberInfo>);
        void spamClicked();
        void addContactClicked();
        void contactClicked(QString);
        void searchBegin();
        void searchEnd();
        void backButtonClicked();
        void moreClicked();
        void adminsClicked();
        void blockedClicked();
        void pendingClicked();
        void avatarClicked();
        void chatEvent(QString);
        void menu(QAction*);
        void actionResult(int);
        void approveAllClicked();
        void publicChanged(int);
        void approvedChanged(int);
        void linkToChatClicked(int);
        void ageClicked(int);
        void readOnlyClicked(int);
        void removeClicked();
        void touchScrollStateChanged(QScroller::State);
        void searchClicked();
        void chatRoleChanged(QString);

    private:
        void init();
        void initAvatarAndName();
        void initAddContactAndSpam();
        void initFavoriteNotificationsSearchTheme();
        void initChatMembers();
        void initEraseIgnoreDelete();
        void initListWidget();
        void connectSignals();
        void initDescription(const QString& description, bool full = false);
        void blockUser(const QString& aimId, bool block);
        void readOnly(const QString& aimId, bool block);
        void changeRole(const QString& aimId, bool moder);
        void approve(const QString& aimId, bool approve);
        void changeTab(int tab);

    private:
        QString currentAimId_;
        QScrollArea* area_;
        ContactAvatarWidget* avatar_;
        TextEditEx* name_;
        TextEditEx* description_;
        QWidget* notMemberTopSpacer_;
        QWidget* notMemberBottomSpacer_;
        TextEditEx* youAreNotAMember_;
        LineWidget* firstLine_;
        LineWidget* secondLine_;
        LineWidget* thirdLine_;
        LineWidget* approveAllLine_;
        QWidget* adminsSpacer_;
        QWidget* blockSpacer_;
        QWidget* pendingSpacer_;
        QWidget* addContactSpacerTop_;
        QWidget* addContactSpacer_;
        QWidget* labelsSpacer_;
        QWidget* mainWidget_;
        QWidget* listWidget_;
        QWidget* textTopSpace_;
        CustomButton* notificationsButton_;
        QCheckBox* notificationsCheckbox_;
        CustomButton* publicButton_;
        QCheckBox* publicCheckBox_;
        CustomButton* approvedButton_;
        QCheckBox* approvedCheckBox_;
        CustomButton* linkToChat_;
        QCheckBox* linkToChatCheckBox_;
        CustomButton* readOnly_;
        QCheckBox* readOnlyCheckBox_;
        CustomButton* ageRestrictions_;
        QCheckBox* ageCheckBox_;
        ActionButton* addToChat_;
        ActionButton* favoriteButton_;
        ActionButton* copyLink_;
        ActionButton* themesButton_;
        ActionButton* privacyButton_;
        ActionButton* eraseHistoryButton_;
        ActionButton* ignoreButton_;
        ActionButton* quitAndDeleteButton_;
        CustomButton* addContact_;
        ActionButton* spamButton_;
        ActionButton* spamButtonAuth_;
        ActionButton* deleteButton_;
        CustomButton* backButton_;
        ClickedWidget* allMembers_;
        ClickedWidget* admins_;
        ClickedWidget* blockList_;
        ClickedWidget* avatarName_;
        ClickedWidget* pendingList_;
        ActionButton* searchInChat_;

        QLabel* allMembersCount_;
        QLabel* blockCount_;
        QLabel* pendingCount_;
        QLabel* blockLabel_;
        QLabel* pendingLabel_;
        QLabel* listLabel_;
        QLabel* allMembersLabel_;
        Logic::ChatMembersModel* chatMembersModel_;
        Logic::ContactListItemDelegate* delegate_;
        std::shared_ptr<Data::ChatInfo> info_;
        LabelEx* moreLabel_;
        LabelEx* approveAll_;
        LabelEx* publicAbout_;
        LabelEx* readOnlyAbout_;
        LabelEx* linkToChatAbout_;
        LabelEx* approvalAbout_;
        LabelEx* ageAbout_;
        QWidget* approveAllWidget_;
        QWidget* contactListWidget_;
        QWidget* privacyWidget_;
        QWidget* publicBottomSpace_;
        QWidget* approvedBottomSpace_;
        QWidget* linkBottomSpace_;
        QWidget* readOnlyBottomSpace_;
        ContactList* cl_;
        SearchWidget* searchWidget_;
        QStackedWidget* stackedWidget_;
        QVBoxLayout* rootLayout_;
        QVBoxLayout* nameLayout_;
        int currentTab_;
    };
}
