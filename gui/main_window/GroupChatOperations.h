#pragma once

namespace Logic
{
    class ChatMembersModel;
}

namespace Data
{
    struct Quote;
}

namespace Ui
{
    class GeneralDialog;
    class TextEditEx;
    class ContactAvatarWidget;
}

namespace GroupChatOperations
{
    struct ChatData
    {
        QString name;
        bool publicChat;
        bool approvedJoin;
        bool joiningByLink;
        bool readOnly;
        bool ageGate;
        ChatData(): name(), publicChat(false), approvedJoin(false), joiningByLink(true), readOnly(false), ageGate(false)
        {
        }
    };
}

namespace Ui
{
    class GroupChatSettings final: public QWidget
    {
    private:
        std::unique_ptr<GeneralDialog> dialog_;
        QWidget *content_;
        TextEditEx *chatName_;
        ContactAvatarWidget *photo_;
        QPixmap lastCroppedImage_;
        
        GroupChatOperations::ChatData &chatData_;
        
        bool editorIsShown_;
        
    public:
        GroupChatSettings(QWidget *parent, const QString &buttonText, const QString &headerText, GroupChatOperations::ChatData &chatData);
        ~GroupChatSettings();
        
        bool show();
        inline const QPixmap &lastCroppedImage() const { return lastCroppedImage_; }
        inline ContactAvatarWidget *photo() const { return photo_; }
        
    private:
        void showImageCropDialog();
    };
    
    void createGroupChat(QStringList _members_aimIds);
    bool callChatNameEditor(QWidget* _parent, GroupChatOperations::ChatData &chatData, Out std::shared_ptr<GroupChatSettings> &groupChatSettings);

    void postCreateChatInfoToCore(const QString &_aimId, const GroupChatOperations::ChatData &chatData, QString avatarId = QString());
    void postAddChatMembersFromCLModelToCore(QString _aimId);

    void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim, QWidget* _parent);
    
    void forwardMessage(QList<Data::Quote> quotes, bool fromMenu);
    void forwardMessage(const QString& _message, bool fromMenu);
}
