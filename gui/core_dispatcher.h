#pragma once

#include "voip/VoipProxy.h"

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"
#include "../corelib/enumerations.h"

#include "types/chat.h"
#include "types/common_phone.h"
#include "types/contact.h"
#include "types/images.h"
#include "types/link_metadata.h"
#include "types/message.h"
#include "types/typing.h"
#include "types/snap.h"

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace voip_proxy
{
    struct device_desc;
}

namespace core
{
    enum class file_sharing_function;
    enum class sticker_size;
    enum class group_chat_info_errors;
    enum class typing_status;
}

namespace launch
{
    int main(int argc, char *argv[]);
}

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    typedef std::function<void(int64_t, core::coll_helper&)> message_function;

    #define REGISTER_IM_MESSAGE(_message_string, _callback) \
        messages_map_.emplace( \
        _message_string, \
        std::bind(&core_dispatcher::_callback, this, std::placeholders::_1, std::placeholders::_2));

    namespace stickers
    {
        enum class sticker_size;
    }

    class gui_signal : public QObject
    {
        Q_OBJECT
    public:

Q_SIGNALS:
        void received(const QString, const qint64, core::icollection*);

    };

    class gui_connector : public gui_signal, public core::iconnector
    {
        std::atomic<int>	refCount_;

        // ibase interface
        virtual int addref() override;
        virtual int release() override;

        // iconnector interface
        virtual void link(iconnector*, const common::core_gui_settings&) override;
        virtual void unlink() override;
        virtual void receive(const char *, int64_t, core::icollection*) override;
    public:
        gui_connector() : refCount_(1) {}
    };

    enum class MessagesBuddiesOpt
    {
        Min,

        Requested,
        FromServer,
        DlgState,
        Pending,
        Init,
        MessageStatus,

        Max
    };

    typedef std::function<void(core::icollection*)> message_processed_callback;

    class core_dispatcher : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void needLogin(const bool _is_auth_error);
        void contactList(std::shared_ptr<Data::ContactList>, QString);
        void im_created();
        void loginComplete();
        void getImagesResult(Data::ImageListPtr images);
        void messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64, int64_t last_msg_id);
        void getSmsResult(int64_t, int _errCode, int _codeLength);
        void loginResult(int64_t, int code);
        void loginResultAttachUin(int64_t, int _code);
        void loginResultAttachPhone(int64_t, int _code);
        void avatarLoaded(const QString&, QPixmap*, int);
        void avatarUpdated(const QString &);
        
        void presense(Data::Buddy*);
        void searchResult(QStringList);
        void dlgStates(std::shared_ptr<QList<Data::DlgState>>);
        void searchedMessage(Data::DlgState);
        void searchedContacts(QList<Data::DlgState>, qint64);
        void emptySearchResults(qint64);
        void activeDialogHide(QString);
        void guiSettings();
        void coreLogins(const bool _has_valid_login);
        void themeSettings();
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatBlocked(QList<Data::ChatMemberInfo>);
        void chatPending(QList<Data::ChatMemberInfo>);
        void chatInfoFailed(qint64, core::group_chat_info_errors);
        void myInfo();
        void login_new_user();
        void messagesDeleted(QString, QList<int64_t>);
        void messagesDeletedUpTo(QString, int64_t);
        void messagesModified(QString, std::shared_ptr<Data::MessageBuddies>);

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);

        void messagesReceived(QString, QVector< QString >);

        void contactRemoved(QString);
        void openChat(QString _aimid);

        void feedbackSent(bool);

        // sticker signals
        void onStickers();
        void onSticker(qint32 _setId, qint32 _stickerId);

        void onThemesMeta();
        void onThemesMetaError();
        void onTheme(int,bool);

        // remote files signals
        void imageDownloaded(qint64 _seq, QString _rawUri, QPixmap _image, QString _localPath);
        void imageMetaDownloaded(qint64 _seq, Data::LinkMetadata _meta);
        void imageDownloadingProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred);
        void imageDownloadError(int64_t _seq, QString _rawUri);

        void fileSharingError(qint64 _seq, QString _rawUri, qint32 _errorCode);

        void fileSharingFileDownloaded(qint64 _seq, QString _rawUri, QString _localPath);
        void fileSharingFileDownloading(qint64 _seq, QString _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);

        void fileSharingFileMetainfoDownloaded(qint64 _seq, QString _fileName, QString _downloadUri, qint64 _size);
        void fileSharingPreviewMetainfoDownloaded(qint64 _seq, QString _miniPreviewUri, QString _fullPreviewUri);

        void fileSharingLocalCopyCheckCompleted(qint64 _seq, bool _success, QString _localPath);

        void fileSharingUploadingProgress(QString _uploadingId, qint64 _bytesTransferred);
        void fileSharingUploadingResult(QString _seq, bool _success, QString localPath, QString _uri, int _contentType, bool _isFileTooBig);

        void linkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta);
        void linkMetainfoImageDownloaded(int64_t _seq, bool _success, QPixmap _image);
        void linkMetainfoFaviconDownloaded(int64_t _seq, bool _success, QPixmap _image);

        void signedUrl(QString);
        void speechToText(qint64 _seq, int _error, QString _text, int _comeback);

        void chatsHome(QList<Data::ChatInfo>, QString, bool, bool);
        void chatsHomeError(int);

        void recvPermitDeny(bool);

        void recvFlags(int);
        void updateProfile(int);
        void getUserProxy();

        void setChatRoleResult(int);
        void blockMemberResult(int);
        void pendingListResult(int);

        void phoneInfoResult(qint64, Data::PhoneInfo);
        void snapMetainfoDownloaded(int64_t _seq, bool _success, uint64_t _snapId, int64_t _expireUtc, QString _authorUin, QString _authorName);
        void snapPreviewInfoDownloaded(qint64 _snapId, QString _mini_preview, QString _ttl_id, bool _found);

        // masks
        void maskListLoaded(QList<QString> maskList);
        void maskPreviewLoaded(int64_t _seq, const QString& _localPath);
        void maskModelLoaded();
        void maskLoaded(int64_t _seq, const QString& _localPath);
        void maskLoadingProgress(int64_t _seq, unsigned _percent);
        void maskRetryUpdate();

        void appConfig();

        void mailStatus(QString, unsigned, bool);
        void newMail(QString, QString, QString, QString);
        void mrimKey(qint64, QString);

        void historyUpdate(QString, qint64);
        void userSnaps(Logic::UserSnapsInfo, bool);
        void userSnapsState(Logic::SnapState);
        void userSnapsStorage(QList<Logic::UserSnapsInfo>, bool);

    public Q_SLOTS:
        void received(const QString, const qint64, core::icollection*);

    public:
        core_dispatcher();
        virtual ~core_dispatcher();

        core::icollection* create_collection() const;
        qint64 post_message_to_core(const QString& _message, core::icollection* _collection, const QObject* _object = nullptr, const message_processed_callback _callback = nullptr);

        qint64 post_stats_to_core(core::stats::stats_event_names _eventName);
        qint64 post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props);
        void set_enabled_stats_post(bool _isStatsEnabled);
        bool get_enabled_stats_post() const;

        voip_proxy::VoipController& getVoipController();

        qint64 getFileSharingPreviewSize(const QString& _url);
        qint64 downloadFileSharingMetainfo(const QString& _url);
        qint64 downloadSharedFile(const QString& _contactAimid, const QString& _url, bool _forceRequestMetainfo, const QString& _fileName = QString(), bool _raise_priority = false);

        qint64 abortSharedFileDownloading(const QString& _url);

        qint64 uploadSharedFile(const QString &contact, const QString& _localPath);
        qint64 uploadSharedFile(const QString &contact, const QByteArray& _array, const QString& ext);
        qint64 abortSharedFileUploading(const QString &contact, const QString& _localPath, const QString& _uploadingProcessId);

        qint64 getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size);
        qint64 getTheme(const qint32 _themeId);

        int64_t downloadImage(
            const QUrl& _uri,
            const QString& _contactAimid,
            const QString& _destination,
            const bool _isPreview,
            const int32_t _maxPreviewWidth,
            const int32_t _maxPreviewHeight,
            bool _raisePriority = false);

        void cancelImageDownloading(const QString& _url);

        int64_t downloadLinkMetainfo(
            const QString& _contactAimid,
            const QString& _uri,
            const int32_t _previewWidth,
            const int32_t _previewHeight);

        qint64 deleteMessages(const std::vector<int64_t>& _messageIds, const QString& _contactAimId, const bool _forAll);
        qint64 deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId);

        bool isImCreated() const;

        qint64 raiseDownloadPriority(const QString &_contactAimid, int64_t _procId);

        qint64 contactSwitched(const QString &_contactAimid);

        void sendMessageToContact(const QString& _contact, const QString& _text);

        void read_snap(const QString& _contact, const uint64_t _snapId, const bool _markPrevSnapsRead, bool _refreshStorage = false);

        int64_t download_snap_metainfo(const QString& _contact, const QString& _ttlId, bool _raise = false);

        int64_t pttToText(const QString& _pttLink, const QString& _locale);

        int64_t setUrlPlayed(const QString& _url, const bool _isPlayed);

        void setUserState(const core::profile_state state);
        void invokeStateAway();
        void invokePreviousState();

        void onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params);

        // messages
        void onNeedLogin(const int64_t _seq, core::coll_helper _params);
        void onImCreated(const int64_t _seq, core::coll_helper _params);
        void onLoginComplete(const int64_t _seq, core::coll_helper _params);
        void onContactList(const int64_t _seq, core::coll_helper _params);
        void onLoginGetSmsCodeResult(const int64_t _seq, core::coll_helper _params);
        void onLoginResult(const int64_t _seq, core::coll_helper _params);
        void onAvatarsGetResult(const int64_t _seq, core::coll_helper _params);
        void onAvatarsPresenceUpdated(const int64_t _seq, core::coll_helper _params);
        void onContactPresence(const int64_t _seq, core::coll_helper _params);
        void onGuiSettings(const int64_t _seq, core::coll_helper _params);
        void onCoreLogins(const int64_t _seq, core::coll_helper _params);
        void onThemeSettings(const int64_t _seq, core::coll_helper _params);
        void onArchiveImagesGetResult(const int64_t _seq, core::coll_helper _params);
        void onArchiveMessagesGetResult(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedDlgState(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params);
        void onArchiveMessagesPending(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedInit(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedMessageStatus(const int64_t _seq, core::coll_helper _params);
        void onMessagesDelUpTo(const int64_t _seq, core::coll_helper _params);
        void onDlgStates(const int64_t _seq, core::coll_helper _params);
        void onHistorySearchResultMsg(const int64_t _seq, core::coll_helper _params);
        void onHistorySearchResultContacts(const int64_t _seq, core::coll_helper _params);
        void onEmptySearchResults(const int64_t _seq, core::coll_helper _params);
        void onSearchNeedUpdate(const int64_t _seq, core::coll_helper _params);
        void onHistoryUpdate(const int64_t _seq, core::coll_helper _params);

        void onVoipSignal(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsHide(const int64_t _seq, core::coll_helper _params);
        void onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params);
        void onThemesMetaGetResult(const int64_t _seq, core::coll_helper _params);
        void onThemesMetaGetError(const int64_t _seq, core::coll_helper _params);
        void onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params);
        void onThemesThemeGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsBlockedResult(const int64_t _seq, core::coll_helper _params);
        void onChatsPendingResult(const int64_t _seq, core::coll_helper _params);
        void onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params);

        void fileSharingErrorResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void fileSharingGetPreviewSizeResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingMetainfoResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingCheckExistsResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params);

        void imageDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResult(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void fileUploadingProgress(const int64_t _seq, core::coll_helper _params);
        void fileUploadingResult(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params);
        void onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params);
        void onContactsRemoveResult(const int64_t _seq, core::coll_helper _params);
        void onAppConfig(const int64_t _seq, core::coll_helper _params);
        void onMyInfo(const int64_t _seq, core::coll_helper _params);
        void onSignedUrl(const int64_t _seq, core::coll_helper _params);
        void onFeedbackSent(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params);
        void onTyping(const int64_t _seq, core::coll_helper _params);
        void onTypingStop(const int64_t _seq, core::coll_helper _params);
        void onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params);
        void onLoginResultAttachUin(const int64_t _seq, core::coll_helper _params);
        void onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params);
        void onRecvFlags(const int64_t _seq, core::coll_helper _params);
        void onUpdateProfileResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params);
        void onUserProxyResult(const int64_t _seq, core::coll_helper _params);
        void onOpenCreatedChat(const int64_t _seq, core::coll_helper _params);
        void onLoginNewUser(const int64_t _seq, core::coll_helper _params);
        void onSetAvatarResult(const int64_t _seq, core::coll_helper _params);
        void onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsBlockResult(const int64_t _seq, core::coll_helper _params);
        void onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params);
        void onPhoneinfoResult(const int64_t _seq, core::coll_helper _params);
        void onSnapGetMetainfoResult(const int64_t _seq, const core::coll_helper & _params);
        void onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params);
        void onMasksGetIdListResult(const int64_t _seq, core::coll_helper _params);
        void onMasksPreviewResult(const int64_t _seq, core::coll_helper _params);
        void onMasksModelResult(const int64_t _seq, core::coll_helper _params);
        void onMasksGetResult(const int64_t _seq, core::coll_helper _params);
        void onMasksProgress(const int64_t _seq, core::coll_helper _params);
        void onMasksRetryUpdate(const int64_t _seq, core::coll_helper _params);
        void onMailStatus(const int64_t _seq, core::coll_helper _params);
        void onMailNew(const int64_t _seq, core::coll_helper _params);
        void getMrimKeyResult(const int64_t _seq, core::coll_helper _params);
        void onNeedShowPromoLoaded(const int64_t _seq, core::coll_helper _params);

        void onUserSnaps(const int64_t _seq, core::coll_helper _params);
        void onUserSnapsState(const int64_t _seq, core::coll_helper _params);
        void onUserSnapsStorage(const int64_t _seq, core::coll_helper _params);


    private:

        struct callback_info
        {
            message_processed_callback callback_;

            QDateTime date_;

            const QObject* object_;

            callback_info(const message_processed_callback& _callback, const QDateTime& _date, const QObject* _object)
                :   callback_(_callback), date_(_date), object_(_object)
            {
            }
        };

    private:

        bool init();
        void initMessageMap();
        void uninit();

        void cleanupCallbacks();
        void executeCallback(const int64_t _seq, core::icollection* _params);

        void onEventTyping(core::coll_helper _params, bool _isTyping);

    private:

        std::unordered_map<std::string, message_function> messages_map_;

        core::iconnector* coreConnector_;
        core::icore_interface* coreFace_;
        voip_proxy::VoipController voipController_;
        core::iconnector* guiConnector_;

        std::unordered_map<int64_t, callback_info> callbacks_;

        QDateTime lastTimeCallbacksCleanedUp_;

        bool isStatsEnabled_;
        bool isImCreated_;

        bool userStateGoneAway_;
    };

    core_dispatcher* GetDispatcher();
    void createDispatcher();
    void destroyDispatcher();
}
