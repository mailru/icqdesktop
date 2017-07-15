#ifndef __VOIP2_H__
#define __VOIP2_H__

#include "voip_types.h"
#include "std_connectors.hpp"

namespace voip2 {

using namespace voip;

#define PREVIEW_RENDER_NAME     "@preview"
#define MASKARAD_USER           "@maskarad"

enum LayoutType {       // -= remote =-         -= camera =-
    LayoutType_One,     // equally spaced       detached
    LayoutType_Two,     // equally spaced       as remote
    LayoutType_Three,   // primary+tray         detached
    LayoutType_Four     // primary+tray         as remote
};
enum PreviewMode {
    PreviewMode_Detached,
    PreviewMode_AsRemote,
};
enum MouseTap {
    MouseTap_Single,
    MouseTap_Double,
    MouseTap_Long,
    MouseTap_Over, // reported once per second
};

enum ViewArea {
    ViewArea_Primary,
    ViewArea_Detached,
    ViewArea_Default,
    ViewArea_Background,
};

enum AvatarType {
    AvatarType_UserMain,
    AvatarType_UserNoVideo,
    AvatarType_UserText,
    AvatarType_Header,
    AvatarType_Camera,
    AvatarType_CameraCrossed,
    AvatarType_Background,
    AvatarType_Foreground,
};
enum DeviceType {
    AudioRecording = 0,
    AudioPlayback,
    VideoCapturing,
};

enum Position {
    Position_Left    = 0x01,
    Position_HCenter = 0x02,
    Position_Right   = 0x04,

    Position_Top     = 0x08,
    Position_VCenter = 0x10,
    Position_Bottom  = 0x20
};

enum SoundEvent {
    SoundEvent_OutgoingStarted,
    SoundEvent_WaitingForAccept,            // loop
    SoundEvent_WaitingForAccept_Confirmed,  // loop (we have ringing received)
    SoundEvent_IncomingInvite,              // loop
    SoundEvent_Connected,
    SoundEvent_Connecting,                  // loop
    SoundEvent_Reconnecting,                // loop
    SoundEvent_Hold,                        // loop
    SoundEvent_HangupLocal,
    SoundEvent_HangupRemote,
    SoundEvent_HangupRemoteBusy,
    SoundEvent_HangupHandledByAnotherInstance, // silent
    SoundEvent_HangupByError,

    SoundEvent_Max
};

enum VoipIncomingMsg {
    WIM_Incoming_fetch_url           = 0,
    WIM_Incoming_allocated,

    MRIM_Incoming_SessionAllocated   = 10,
    MRIM_Incoming_UdpMedia,
    MRIM_Incoming_UdpMediaAck,

    OSCAR_Incoming_Allocated         = 20,
    OSCAR_Incoming_WebRtc
};

enum VoipOutgoingMsg {
    WIM_Outgoing_allocate            = 0,
    WIM_Outgoing_invite,
    WIM_Outgoing_accept,
    WIM_Outgoing_decline,
    WIM_Outgoing_json,
    WIM_Outgoing_keepalive,

    MRIM_Outgoing_SessionAllocate    = 10,
    MRIM_Outgoing_UdpMedia,
    MRIM_Outgoing_UdpMediaAck,

    OSCAR_Outgoing_SessionAllocate   = 20,
    OSCAR_Outgoing_SessionAllocatePstn,
    OSCAR_Outgoing_WebRtc
};

/*
    | Event | Code   |
    +-------+--------+  <- DTMF codes
    | 0--9  | 0--9   |
    | *     | 10     |
    | #     | 11     |
    | A--D  | 12--15 |
*/

struct WindowSettings {
    
    struct {
        // picture size
        unsigned width;
        unsigned height;
        // picture size at connecting time zero if unused
        // width  * connectingSizeRatio
        // height * connectingSizeRatio
        float connectingSizeRatio;
        
        // text size
        unsigned textWidth;
        unsigned textHeight;

        unsigned position;
        // Avatar position adjustment:
        // for (Position_HCenter | Position_VCenter)
        // signed (up/down) offset in percents (-100..100) of the (HEIGHT/2) of the draw rect
        signed offsetVertical;

        // Avatar offset from the viewport frame
        unsigned offsetTop;
        unsigned offsetBottom;
        unsigned offsetLeft;
        unsigned offsetRight;

        ChannelStatusContext status;
        
        // Avatar color rings
        VisualEffectContext visualEffect[kVisualEffectType_Total];
        
        hbmp_t   logoImage;
        unsigned logoPosition;// combination of values from Position enum, if (hwnd == NULL) sets position for all windows
        int      logoOffsetL;
        int      logoOffsetT;
        int      logoOffsetR;
        int      logoOffsetB;
    } avatarMain[WindowTheme_Total];

    // Preview:
    bool previewSelfieMode;         // on Calling or Invite mode show preview fullscreen
                                    // In Snap mode this flag required for th focusEffect and Camera Zoom
    bool previewIsButton;           // false - show actual camera state, true - operate as a player button
    bool previewDisable;            // disable preview for this window
    bool previewSolo;               // show only preview in this window
    bool previewRounded;            // show rounded deatached preview
    bool previewSelfieIsRounded;    // and rounded also in selfie mode

    unsigned detachedStreamMaxArea; // maximum area = (width * heigth) for preview
    unsigned detachedStreamMinArea; // minimum area

    unsigned      previewBorderWidth;        // width in pixels
    unsigned char previewBorderColorBGRA[4]; // IOS: Quartz supports premultiplied alpha only for images


    bool disable_mouse_events_handler;

    unsigned      highlight_border_pix;
    unsigned char highlight_color_bgra[4];
    unsigned char normal_color_bgra[4];


    // Conferencing settings
    // Avatar secondary peer names text size (conference)
    unsigned channelTextDisplayW;
    unsigned channelTextDisplayH;
    // Avatar main peer status text size
    unsigned channelStatusDisplayW; // if 0 then appropriate size will be extracted
    unsigned channelStatusDisplayH; // from ChannelStatusContext

    struct Conference {
        bool     forcePrimaryVideoCrop;
        bool     alignPrimaryVideoTop;
        
        unsigned trayHeight;         // in pixels
        float    trayMaxHeight;      // in % of view height
        unsigned trayChannelsGap;    // gap between channel in LayoutType_Three and LayoutType_Four
        unsigned blocksChannelsGap;  // gap between channel in LayoutType_One and LayoutType_Two
        unsigned blocksGap;          // horizontal gap from frame

        float    aspectRatio;
        bool     useGridAdvance;
        
        bool     useHeaders;        // used headers for remote streams otherwise use log in under avatar.
    } conference;
    
    
    ButtonContext        buttonContext;

    unsigned headerOffset;
    unsigned header_height_pix;

    FocusEffectContext focusEffect; // Required previewSelfieMode = true

    // Avatar bounce animation
    // TODO: make context
    unsigned  avatarAnimationCurveLen;   // in samples
    float     avatarAnimationCurve[kMaxAnimationCurveLen];     // sampled at 50Hz (20ms step), 5 sec max

    // Glow animation
    // TODO: make context
    unsigned      statusGlowRadius;           // 0 - disable glow effect, otherwise set radius
    double        glowAttenuation;
    unsigned char connectedGlowColor[4];
    unsigned char disconnectedGlowColor[4];

    //
    hbmp_t oldverTextLarge;     // joint peer
    hbmp_t oldverTextLarge2;    // direct
    hbmp_t oldverTextSmall;
    unsigned char oldverBackround_bgra[4];   
    unsigned oldverBackroundHeightPer; // 0 - 100 %
    unsigned oldverTextLargeDelayMs; // if 0 - TextLarge will be shown on hover only
    
    unsigned animationTimeMs;

	bool     usePreviewBackground; // use background for preview.
};
/*
    Integration design:

    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Application <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<        >>>>>>>>>>> Voip <<<<<<<<<<<   

     ------------------------------------                        --------------------------------------                   ----------------
    |                                    |                      |                                      |                 |                |
    |      -= MainWnd =- -- MAIN_THREAD ------ user action ---->| -= Controller =-  *--- MAIN_THREAD ---- user action -->|   -= Voip =-   |
    |                                    |                      |                                      |                 |                |
    |                                    |<--- read info -------|       implements VoipObserver  <------- signaling -----|                |
    |                                    |                      |                       |              |                 |                |
    |   implements ControllerObserver <------- state changed -----*----- MAIN_THREAD ---*              |                 |                |
    |                                    |                      | |                                    |                 |                |
     ------------------------------------                       | |                                    |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | *--|         -= CallMgr =-         | |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |            \/     /\               |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | *--|     -= ContactListMgr =-      | |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |            \/     /\               |                 |                |
                                                                | |   -------------------------------  |                 |                |
                                                                | |  |     -= IMConnectionMgr =-     | |                 |                |
                                                                | *--|   implements VoipConnection <----- signaling -----|                |
                                                                |     -------------------------------  |                  ---------------
                                                                 --------------------------------------
    Outgoing call:

                ButtonPressed          ---------------------------------------------------------------------------------> CallStart(user_id)
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_OUTGOING_STARTED
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_OUTGOING_ACCEPTED_VIDEO
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_CONNECTED
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---
                ...
                ...

                ButtonPressed          ---------------------------------------------------------------------------------> CallDecline(user_id)
                                                                                                       <----------------- SendVoipMsg
                                                                                                       -----------------> ReadVoipMsg
                ContactUpdated         <-------------------------------- Update CallMgr --------------------------------- SE_CLOSED_BY_LOCAL_HANGUP
                                       ---- get user_id info -->
                RedrawContactItem      <------- user_id info ---

*/

class VoipConnection {
public:
    // msg_idx == 0 means this message is not required to store for resending
    virtual void SendVoipMsg   (const char* from, VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx = 0) = 0;
};

enum SessionEvent { // alive session events
    SE_OPEN_FIRST = 0,
    SE_OUTGOING_STARTED_AUDIO = SE_OPEN_FIRST,  // Feedback on local CallStart(): there is no session with given user_id and we're going to create one
    SE_OUTGOING_STARTED_VIDEO,                  //      [ audio/video flag duplicates local camera state off/on ]
    SE_INCOMING_INVITE_AUDIO,                   // Invite from remote peer. Application may ask for attached list of conference participants using ... .
    SE_INCOMING_INVITE_VIDEO,
    SE_OUTGOING_ACCEPTED_AUDIO,                 // Remote peer has accepted our invite
    SE_OUTGOING_ACCEPTED_VIDEO,
    SE_INCOMING_ACCEPTED_AUDIO,                 // Feedback on local CallAccept()
    SE_INCOMING_ACCEPTED_VIDEO,                 //      [ audio/video flag duplicates local camera state off/on ]
    SE_JOINED_AUDIO,                            // Notification about verified peer joined existing conference
    SE_JOINED_VIDEO,
    SE_OPEN_LAST = SE_JOINED_VIDEO,

    SE_CONNECTION_FIRST = 20,
    SE_DISCONNECTED = SE_CONNECTION_FIRST,
    SE_CONNECTED,
    SE_CONNECTED_EXT_AUDIO_NONE,    // Detailed connection info, application must not rely on those states to manage call flow
    SE_CONNECTED_EXT_AUDIO_UDP,
    SE_CONNECTED_EXT_AUDIO_TCP,
    SE_CONNECTED_EXT_AUDIO_RELAY,
    SE_CONNECTED_EXT_VIDEO_NONE,
    SE_CONNECTED_EXT_VIDEO_UDP,
    SE_CONNECTED_EXT_VIDEO_TCP,
    SE_CONNECTED_EXT_VIDEO_RELAY,
    SE_CIPHER_ENABLED,               // app should call GetCipherSAS() and render it to user
    SE_CIPHER_NOT_SUPPORTED_BY_PEER,
    SE_CIPHER_FAILED,
    SE_CONNECTION_LAST = SE_CIPHER_FAILED,

    SE_REMOTE_FIRST = 40,
    SE_REMOTE_MIC_ON = SE_REMOTE_FIRST,
    SE_REMOTE_MIC_OFF,
    SE_REMOTE_CAM_ON,
    SE_REMOTE_CAM_OFF,
    SE_QUALITY_BAD,
    SE_QUALITY_AUDIO_OK,
    SE_QUALITY_AUDIO_VIDEO_OK,
    SE_INCOMING_CONF_PEERS_UPDATED,             // new conference participant for existing invite, application must ask voip for details and update invite dialog
    SE_NO_CONF_SUPPORTED,                       // remote peer does not support conference mode 
    SE_OUTGOING_VIDEO_DISABLED_LOW_BANDWIDTH,
    SE_REMOTE_LAST = SE_OUTGOING_VIDEO_DISABLED_LOW_BANDWIDTH,

    // closed session events:   (Note: if you change this part (e.g. by adding new events), don't forget to update 
    //                           stat mapping in call_stat/call_record.cc:SessionEvent2TerminateReason() )
    SE_CLOSED_BY_REMOTE_DECLINE = 128,
    SE_CLOSED_BY_REMOTE_HANDLED_BY_ANOTHER_INSTANCE,
    SE_CLOSED_BY_REMOTE_BUSY,
    SE_CLOSED_BY_REMOTE_ERROR,
    SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_REMOTE,
    SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_LOCAL,
    SE_CLOSED_BY_TIMEOUT_INACTIVE,      // this peer invited by conference participant and we get no additional info on timeout exhausted
    SE_CLOSED_BY_TIMEOUT_CONNECT_INIT,  // connection has not been started
    SE_CLOSED_BY_TIMEOUT_CONNECTION,
    SE_CLOSED_BY_TIMEOUT_RECONNECT,
    SE_CLOSED_BY_ERROR_CREATE,
    SE_CLOSED_BY_ERROR_START,
    SE_CLOSED_BY_ERROR_INTERNAL,
    SE_CLOSED_BY_LOCAL_BUSY,
    SE_CLOSED_BY_LOCAL_HANGUP
};

struct CallInfo {
    enum { kMaxUsers = 16 };

    bool     local_audio_enabled;
    bool     local_video_enabled;
    unsigned numUsers;

    struct UserInfo {
        enum { kMaxUserIdSize = 256 };

        char user_id[kMaxUserIdSize];    

        ConnectionState connstate_audio;
        ConnectionState connstate_video;  

        bool remote_audio_enabled;
        bool remote_video_enabled;

    } users[kMaxUsers];
};

enum DeviceStatus {
    DeviceStatus_Started,
    DeviceStatus_Resumed,
    DeviceStatus_Paused,
    DeviceStatus_Stopped,
    DeviceStatus_Stopped_ByVoip,
    DeviceStatus_Stopped_StartFail,
};

struct VideoDeviceCapability {
    bool canFlash;
    struct {
        bool On;
        bool Auto;
    } FlashModes;

    bool canTorch;
    struct {
        bool On;
        bool Auto;
    } TorchModes;
};

enum VideoDeviceFlashFlags {
    VideoDevice_FlashOff = 0,
    VideoDevice_FlashOn,
    VideoDevice_FlashAuto,
};
enum VideoDeviceTorchFlags {
    VideoDevice_TorchOff = 0,
    VideoDevice_TorchOn,
    VideoDevice_TorchAuto
};
enum SnapRecordingStatus {      // Started -> Started_Reencoding -> Ready
    SnapRecording_DELIM_STARTED,
        SnapRecording_Started = SnapRecording_DELIM_STARTED,
    SnapRecording_DELIM_INFO,
//      SnapRecording_Started_Reencoding = SnapRecording_DELIM_INFO, // stop offline recording w/o delete_output flag set
//      SnapRecording_Competion_Forced,
//      SnapRecording_Progress,
        SnapRecording_ChunkReady = SnapRecording_DELIM_INFO,  // <- empty buffer signals end-of-chunk
    SnapRecording_DELIM_COMPLETE,
        SnapRecording_Ready = SnapRecording_DELIM_COMPLETE,
    SnapRecording_DELIM_COMPLETE_NOFILE,
        SnapRecording_Destroyed_ByUser = SnapRecording_DELIM_COMPLETE_NOFILE,
    SnapRecording_DELIM_COMPLETE_WITH_ERROR,
        SnapRecording_Destroyed_NoSession = SnapRecording_DELIM_COMPLETE_WITH_ERROR, // start failed since no maskarad session exist
        SnapRecording_Destroyed_SessionClosed,
        SnapRecording_Destroyed_CreateError,
        SnapRecording_Destroyed_CameraFailed,
        SnapRecording_Destroyed_Error,
};

enum MaskLoadStatus {
    MaskLoad_Success,
    MaskLoad_Canceled,
    MaskLoad_Failed,
};

class VoipObserver {
public:
    virtual void DeviceListChanged          (DeviceType deviceType) = 0;
    virtual void DeviceStatusChanged        (DeviceType deviceType, const char *uid, DeviceStatus deviceStatus) = 0;

    virtual void AudioDeviceVolumeChanged   (DeviceType deviceType, float volume) = 0;
    virtual void AudioDeviceMuteChanged     (DeviceType deviceType, bool mute) = 0;
    virtual void AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) = 0;
    virtual void VideoDeviceCapabilityChanged(const char* camera_uid, VideoDeviceCapability caps) {}

    virtual void RenderMouseTap     (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) = 0;
    virtual void ButtonPressed      (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) = 0;
    virtual void MissedCall         (const char* account_uid, const char* user_id, unsigned timestamp) = 0;
    virtual void SessionEvent       (const char* account_uid, const char* user_id, voip2::SessionEvent sessionEvent) = 0;
    virtual void LayoutTypeChanged  (voip::hwnd_t hwnd, voip2::LayoutType layoutType) = 0;
    virtual void FrameSizeChanged   (voip::hwnd_t hwnd, float aspectRatio) = 0;
    virtual void VideoStreamChanged (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) = 0;

    virtual void InterruptByGsmCall(bool gsmCallStarted) = 0;

    virtual void MinimalBandwidthMode_StateChanged(bool mbmEnabled) = 0;
    
    // TODO: For now it is used only for Maskarad.
    virtual void StillImageReady(const char *data, unsigned len, unsigned width, unsigned height) = 0;
    // end-of-chunk <=> no_data <=> data = NULL, size == 0
    virtual void SnapRecordingStatusChanged(const char* filename, voip2::SnapRecordingStatus snapRecordingStatus, unsigned width, unsigned height, const char *data, unsigned size) = 0;
    virtual void FirstFramePreviewForSnapReady(const char *rgb565, unsigned len, unsigned width, unsigned height) = 0;

    // @return res = true on success.
    virtual void MaskModelInitStatusChanged(bool success) {}  // Model status always reported as first
    virtual void MaskRenderInitStatusChanged(bool success) {} // MaskEngineInitialized = ModelOk & RenderOk
    virtual void MaskLoadStatusChanged(const char* mask_path, MaskLoadStatus maskLoadStatus) {}
};

enum {
    MAX_DEVICE_NAME_LEN = 512,
    MAX_DEVICE_GUID_LEN = 512,
    MAX_SAS_LENGTH      = 128
};

struct MaskInfo
{
    String     name;                       // todo: how to localize names?
    String     mask_path;                  // should be passed to LoadMask()
    String     preview_path;               // path to preview image file
    int        version;
    Vector<String>      categories;        // which tabs to put this mask into
};

struct CreatePrms; // hidden from application

struct VOIP_EXPORT ConferenceParticipants {
    ConferenceParticipants();
    ~ConferenceParticipants();
    const char *user_uids; // separated with '\0', ends with '\0\0'
};

class VOIP_EXPORT Voip2 { // singletone
public:
    static Voip2* CreateVoip2(
        VoipConnection& voipConnection, 
        VoipObserver& voipObserver, 
        const char* client_name_and_version,        // Short client identification string, e.g. "ICQ 1.2.1234", "MyPhone 1.2.111", etc.
        const char* app_data_folder_utf8,           // Path to folder where call_stat will store backup file
        const char* stat_servers_override = NULL,   // Format: "host1:port1;host2:port2;host3" (80 assumed for host3)
        bool useOscarAsIcqProtocol = false,
        voip::SystemObjects *platformSystemObject = NULL, 
        const char *paramDB_json = NULL, 
        CreatePrms* createPrms = NULL               // internal use only
    );
    static void  DestroyVoip2(Voip2*);

    virtual bool Init(bool iosCallKitSupport = false, CreatePrms* createPrms = NULL) = 0;
    virtual void EnableMsgQueue() = 0;
    virtual void StartSignaling() = 0;

    virtual void EnableMinimalBandwithMode(bool enable) = 0;
    virtual void EnableRtpDump(bool enable) = 0;

    virtual unsigned GetDevicesNumber(DeviceType deviceType) = 0;
    virtual bool     GetDevice       (DeviceType deviceType, unsigned index, char deviceName[MAX_DEVICE_NAME_LEN], char deviceUid[MAX_DEVICE_GUID_LEN]) = 0;
    virtual void     SetDevice       (DeviceType deviceType, const char *deviceUid) = 0;
    virtual void     SetDeviceMute   (DeviceType deviceType, bool mute) = 0;
    virtual bool     GetDeviceMute   (DeviceType deviceType) = 0;
    virtual void     SetDeviceVolume (DeviceType deviceType, float volume) = 0;
    virtual float    GetDeviceVolume (DeviceType deviceType) = 0;

    virtual void SetProxyPrms        (voip::ProxyType proxyType, const char *serverUrl, const char *userName, const char *password) = 0;
    virtual void SetSound            (SoundEvent soundEvent, const void* data, unsigned size, const unsigned* vibroPatternMs = NULL, unsigned vibroPatternLen = 0, unsigned samplingRateHz = 0) = 0;
    virtual void SetSound            (SoundEvent soundEvent, const char* fileName, const unsigned* vibroPatternMs = NULL, unsigned vibroPatternLen = 0) = 0;
    virtual void MuteIncomingSoundNotifications   (const char* user_id, bool mute) = 0;
    virtual void MuteAllIncomingSoundNotifications(bool mute) = 0;

    virtual void SetLoudspeakerMode  (bool enable)  = 0;

    virtual void ReadVoipMsg (const char *account_uid, VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len, const char *phonenum = NULL) = 0;
    virtual void ReadVoipPush(const char *account_uid, const char *data, unsigned len, const char *phonenum = NULL) = 0;
    virtual void ReadVoipAck (const char *account_uid, unsigned msg_idx, bool success) = 0;

    virtual void EnableOutgoingAudio(bool enable) = 0;
    virtual void EnableOutgoingVideo(bool enable) = 0;

    virtual void CallStart  (const char* account_uid, const char* user_id) = 0;
    virtual void CallAccept (const char* account_uid, const char* user_id) = 0;
    virtual void CallDecline(const char* user_id, bool busy = false) = 0;
    virtual void CallStop   () = 0; // Does not affect incoming calls

    // -- Camera interface
    virtual void SetMaskaradAspectRatio(unsigned aspect_num, unsigned aspect_denum) = 0;
    virtual void StartSnapRecording(const char* filename, bool previewFirstFrame = false, VoipSnapMode voipSnapMode = SNAP_HIGH_RATE_FOR_REENCODE, int chunkLimitSec = 0) = 0;
    virtual void StopSnapRecording(const char* filename = NULL, bool deleteRecordedFile = false) = 0; // empty names means "current" file

    virtual void CaptureStillImage() = 0;
    virtual void SetVideoDeviceParams(VideoDeviceFlashFlags flash, VideoDeviceTorchFlags torch, bool enableManualFocus, bool enableManualExposure) = 0;

    virtual void ShowIncomingConferenceParticipants(const char* user_id, ConferenceParticipants& conferenceParticipants) = 0;
    virtual void SendAndPlayOobDTMF(const char* user_id, int code, int play_ms=200, int play_Db=10) = 0;

    virtual bool GetCipherSAS(const char* account_uid, const char* user_id, char SAS_utf8[MAX_SAS_LENGTH]) = 0;  // valid after receiving SE_CIPHER_ENABLED

    // Layout controls
    virtual void WindowAdd                  (hwnd_t wnd, const WindowSettings& windowSettings) = 0;
    virtual void WindowRemove               (hwnd_t wnd) = 0;
    virtual void WindowSetBackground        (hwnd_t wnd, hbmp_t hbmp) = 0;
    virtual void WindowSetAvatar            (const char* user_id, hbmp_t hbmp,  AvatarType avatarType, WindowThemeType theme = WindowTheme_One,
                                                int radiusPix = 0, // negative value set radius to min(width/2, height/2)
                                                unsigned smoothingBorderPx = 0, unsigned attenuation_from_0_to_100 = 0) = 0;
    virtual void WindowChangeOrientation    (OrientationMode mode) = 0;
    virtual void WindowSetLayoutType        (hwnd_t wnd, LayoutType layoutType, bool animated=true) = 0;
    virtual void WindowSwitchAspectMode     (hwnd_t wnd, const char* user_id, bool animated=true) = 0;
    virtual void WindowSetPrimary           (hwnd_t wnd, const char* user_id, bool animated=true) = 0;
    virtual void WindowSetControlsStatus    (hwnd_t wnd, bool visible, unsigned off_left, unsigned off_top, unsigned off_right, unsigned off_bottom, bool animated, bool enableOverlap) = 0;
    virtual void WindowAddButton            (ButtonType type, ButtonPosition position) = 0;
	virtual void WindowShowButton			(ButtonType type, bool visible) = 0;
    virtual void WindowSetTheme             (hwnd_t wnd, voip2::WindowThemeType theme) = 0;

    virtual void WindowSetPostRenderEffectParams (hwnd_t wnd, const char *param_str) = 0; // see video_render_device_opengl_postfilter.h for params description

    virtual void WindowExternalMouseClick   (hwnd_t wnd, int x, int y) = 0;
    virtual void WindowExternalMouseZoom    (hwnd_t wnd, int x, int y, float scale) = 0;

    // Mask API
    static unsigned GetMaskEngineVersion();                                 // Downloaded data must match engine's version.
    virtual void InitMaskEngine(const char *data_folder_path = 0) = 0;      // Use NULL to destroy. On error you can catch signal.
    virtual void EnumerateMasks(const char *mask_base_folder, Vector<MaskInfo> &masks) = 0;
    virtual void LoadMask(const char *mask_path) = 0;                       // Use NULL to unload mask.
    
    virtual void SetEffect(int slot, e_AudioEffect effect) = 0;
    virtual void SetPitch(float pitch_factor) = 0;

#if (__PLATFORM_WINPHONE || WINDOWS_PHONE) && defined(__cplusplus_winrt)
    virtual void GetVoipInfo(voip2::CallInfo& callInfo) = 0;
#endif

    virtual void UserRateLastCall(const char* account_uid, const char* user_id, int score) = 0; // use after call complete (SE_CLOSED_BY_* received)
    static void DisableStatistics();// internal use only
};
}

#if (__PLATFORM_WINPHONE || WINDOWS_PHONE) && defined(__cplusplus_winrt)
namespace webrtc {
    class MouseEventHandler;
}

namespace VoipSvc
{
    class IMouseEventsSource
    {
    public:
        // invokes to store sink in UI every time mouse and windows event listener created.
        virtual void OnMouseEventsSinkCreated(voip::hwnd_t wnd, webrtc::MouseEventHandler& mouseEventHandler) = 0;

        // invokes to reset sink in UI.
        virtual void OnMouseEventsSinkDestroyed(voip::hwnd_t wnd, webrtc::MouseEventHandler& mouseEventHandler) = 0;
    };
}

#endif

#endif
