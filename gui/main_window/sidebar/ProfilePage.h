#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"
#include "../../../corelib/enumerations.h"

namespace Ui
{
    class ContactAvatarWidget;
    class CustomButton;
    class TextEditEx;
    class LabelEx;
    class LiveChatMembersControl;
    class TextEmojiWidget;
    class LineWidget;
    class ActionButton;
    class FlatMenu;

    class InfoPlate : public QWidget
    {
        Q_OBJECT
    public:
        InfoPlate(QWidget* parent, int leftMargin);
        void setHeader(const QString& header);
        void setInfo(const QString& info, const QString& prefix = QString());
        void elideText(int width);
        void setAttachPhone(bool value);
        QString getInfoText();

    private Q_SLOTS:
        void menuRequested();
        void menu(QAction*);

    private:
        QLabel* header_;
        TextEmojiWidget* info_;
        QLabel* phoneInfo_;
        QString infoStr_;
        bool attachPhone_;
    };

    class ProfilePage : public SidebarPage
    {
        Q_OBJECT
    public:
        ProfilePage(QWidget* parent);
        virtual void initFor(const QString& aimId);

    protected:
        virtual void paintEvent(QPaintEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
        virtual void updateWidth();

    private Q_SLOTS:
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void removeFromIgnore();
        void contactChanged(QString);
        void contactRemoved(QString);
        void changed();
        void addClicked();
        void chatClicked();
        void callClicked();
        void videoClicked();
        void back();
        void rename();
        void menuStateOnline();
        void menuStateDoNotDisturb();
        void menuStateInvisible();
        void ignoreList();
        void editClicked();
        void attachOld();
        void recvFlags(int);
        void statusClicked();
        void saveClicked();
        void avatarChanged();
        void touchScrollStateChanged(QScroller::State);

    private:
        void init();
        void updateStatus();
        void setState(const core::profile_state _state);

    private:
        QString currentAimId_;
        QWidget* mainWidget_;
        ContactAvatarWidget* avatar_;
        CustomButton* backButton_;
        TextEditEx* name_;
        TextEditEx* nameEdit_;
        TextEditEx* descriptionEdit_;
        QWidget* buttonsMargin_;
        QWidget* ignoreWidget_;
        QWidget* buttonWidget_;
        LineWidget* Line_;
        QWidget* avatarBottomSpace_;
        QWidget* chatEditWidget_;
        LabelEx* ignoreLabel_;
        LabelEx* editLabel_;
        std::shared_ptr<Data::ChatInfo> info_;
        CustomButton* addButton_;
        CustomButton* chatButton_;
        CustomButton* callButton_;
        CustomButton* videoCall_button_;
        ActionButton* renameContact_;
        ActionButton* quiAndDelete;
        ActionButton* ignoreListButton;
        ActionButton* attachOldAcc;
        InfoPlate* uin_;
        InfoPlate* phone_;
        InfoPlate* firstName_;
        InfoPlate* lastName_;
        InfoPlate* nickName_;
        InfoPlate* birthday_;
        InfoPlate* city_;
        InfoPlate* country_;
        QWidget* nameMargin_;
        QWidget* statusWidget_;
        QPushButton* statusButton_;
        QPushButton* saveButton_;
        FlatMenu* statusMenu_;
        LabelEx* statusLabel_;
        QVBoxLayout* subBackButtonLayout_;
        QVBoxLayout* subAvatarLayout_;
        QVBoxLayout* mainBackButtonLayout_;
        QVBoxLayout* mainAvatarLayout_;
        QHBoxLayout* nameLayout_;
        QVBoxLayout* editLayout_;
        QVBoxLayout* subEditLayout_;
        QWidget* saveButtonMargin_;
        QWidget* saveButtonSpace_;
        bool connectOldVisible_;
        bool myInfo_;
    };
}
