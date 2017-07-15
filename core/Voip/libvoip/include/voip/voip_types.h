/**************************************************************************************************
* voip_types.h
*
* Types definition, used by VoIP API
*
**************************************************************************************************/
#ifndef   _LIBVOIP_TYPES_H
#define   _LIBVOIP_TYPES_H

#ifdef __cplusplus
namespace voip  {

#ifdef  VOIP_EXPORT_DLL
  #define VOIP_EXPORT  _declspec(dllexport)
#elif   VOIP_IMPORT_DLL
  #define VOIP_EXPORT _declspec(dllimport)
#else
  #define VOIP_EXPORT
#endif

#ifndef NULL
  #define NULL 0
#endif

enum {
   kRelayGuidLen = 16
};

/////////////////////////////////////////////////////////////////////////////////////////
// PeerInfo -  platform-dependent parameters required for proper IO operation 
union  SystemObjects
{
  struct 
  {
    void* javaVM;
    void* context;
    bool  useOpenSLES;
  }Android;

  struct
  {
    int*  captureAngle;               // pointer must be valid during voip engine life
    void* IMouseEventsSource;         // ptr to store events sink
    const char *deviceManufacturer;   //  Microsoft.Phone.Info.DeviceStatus.DeviceManufacturer
    const char *deviceName;           //  Microsoft.Phone.Info.DeviceStatus.DeviceName
    const char *osVersion;            //  System.Environment.OSVersion.Version.ToString()
  }WinPhone;

  struct
  {
    void  *ICoreDispatcher;     // use "Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher" from GUI thread to get this ref ptr;
  }WinRT;
  
};

union SystemVibroPattern
{
    struct
    {
        enum { kPatternSize = 16 };
       // class android.os.Vibrator (developer.android.com):
       //
       // Pass in an array of ints that are the durations for which to turn on or off
       // the vibrator in milliseconds.  The first value indicates the number of milliseconds
       // to wait before turning the vibrator on.  The next value indicates the number of milliseconds
       // for which to keep the vibrator on before turning it off.  Subsequent values alternate
       // between durations in milliseconds to turn the vibrator off or to turn the vibrator on.
        long long pattern[kPatternSize];
        bool loop;
    } Android;
};

struct SoundNotificationParams {
    SystemVibroPattern vibro;
    bool loudspeaker;
};

/////////////////////////////////////////////////////////////////////////////////////////
// StreamState -  audio/video incoming stream state
enum StreamState
{
  SS_Disabled       = 0,        // peer has disabled the stream (e.g. switched off camera) or it is not supported by local side
  SS_NotConnected   = 1,        // stream is enabled, waiting for connection to be established
  SS_UDP            = 2,        // receiving stream via UDP
  SS_TCP            = 3,        // receiving stream via TCP
  SS_Relay          = 4,        // receiving stream using intermediate relay server
};
/////////////////////////////////////////////////////////////////////////////////////////
// ProxyType - as name stands, the type of Proxy used
enum  ProxyType
{
  ProxyType_None  = 0,    // Proxy is not used
  ProxyType_HTTP  = 1,
  ProxyType_HTTPS = 2,
  ProxyType_SOCKS4= 3,
  ProxyType_SOCKS5= 4
};

enum ConnectionState {
  CS_NotConnected   = 1,        // stream is enabled, waiting for connection to be established
  CS_UDP            = 2,        // receiving stream via UDP
  CS_TCP            = 3,        // receiving stream via TCP
  CS_Relay          = 4,        // receiving stream using intermediate relay server
};


/////////////////////////////////////////////////////////////////////////////////////////
// List of compatible display orientations
enum OrientationMode{  
    omNormal         = 0, // default no rotation
    omRotateRight    = 1,
    omRotateLeft     = 2,
    omUpsideDown     = 3
};

enum  AudioDeviceType {
    RecordingDevice = 0,
    PlaybackDevice  = 1,
    CommonDevice    = 2
};

enum {
    kMaxDeviceNameLen = 1024
};

enum ConnectionQuality {
    ConnectionQuality_Bad,
    ConnectionQuality_Audio_Ok,
    ConnectionQuality_Audio_Video_Ok
};

/////////////////////////////////////////////////////////////////////////////////////////
// Graphical point type
typedef void* hwnd_t; // platform dependent window handle
typedef void* hbmp_t; // platform dependent image handle

/////////////////////////////////////////////////////////////////////////////////////////
// Available buttons
enum ButtonType {
    ButtonType_Close = 0,
    //~~~~~~~
    ButtonType_Total
};
enum ButtonState {
    ButtonState_Normal = 0,
    ButtonState_Highlited,
    ButtonState_Pressed,
    ButtonState_Disabled,
    ButtonState_Invisible,
    //~~~~~~
    ButtonState_Total
};
enum ButtonPosition {
    ButtonPosition_TopRight = 0
};
struct ButtonContext {
    hbmp_t normal;
    hbmp_t highlighted;
    hbmp_t pressed;
    hbmp_t disabled;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Status messages
struct ChannelStatusContext {
    hbmp_t allocating;       // outgoing call initialization
    hbmp_t calling;          // invite sent, waiting for accept/decline/timeout
    hbmp_t ringing;          // invite sent and recived, waiting for accept/decline/timeout
    hbmp_t inviting;         // incoming call is waiting for local accept
    hbmp_t confInviting;     // incoming conference call is waiting for local accept
    hbmp_t connecting;       // connecting
    hbmp_t connected;
    hbmp_t hold;
    hbmp_t oldVersion;
    hbmp_t videoPaused;
    hbmp_t closedByDecline;
    hbmp_t closedByTimeout;
    hbmp_t closedByError;
    hbmp_t ended;
};

#define ANIMATION_CURVE_SAMPLERATE_HZ 50
enum { kMaxAnimationCurveLen = 500 };

enum VisualEffectTypes {
    kVisualEffectType_Connecting = 0,
    kVisualEffectType_Reconnecting,
    //----------------------------
    kVisualEffectType_Total
};

struct VisualEffectContext {
    unsigned animationPeriodMs; // can be less then animation duration

    unsigned width;
    unsigned height;

    unsigned frameWidth;
    unsigned frameHeight;

    int xOffset; // from viewport center
    int yOffset; // from viewport center

    unsigned curveLength;
    unsigned colorBGRACurve[kMaxAnimationCurveLen];// sampled at 50Hz (20ms step), 5 sec max
    float geometryCurve[kMaxAnimationCurveLen];// sampled at 50Hz (20ms step), 5 sec max
};

struct FocusEffectContext {
    hbmp_t    image;
    unsigned  curveLen;                     // in samples
    float     curve[kMaxAnimationCurveLen]; // sampled at 50Hz (20ms step), 5 sec max
};

//#define WindowSettingsThemesCount 2
typedef enum {
    WindowTheme_One = 0,
    WindowTheme_Two = 1,
    WindowTheme_Three = 2,
    WindowTheme_Four = 3,
    WindowTheme_Total = 4
} WindowThemeType;

#define AUDIOFX_MAX_SLOTS  3
enum e_AudioEffect
{
    AUDIO_EFFECT_NONE = 0,
    AUDIO_EFFECT_CHORUS,
    AUDIO_EFFECT_ECHO,
    AUDIO_EFFECT_ENHANCER,
    AUDIO_EFFECT_EXCITER,
    AUDIO_EFFECT_FLANGER,
    AUDIO_EFFECT_FULLWAVE_RECTIFIER,
    AUDIO_EFFECT_HALFWAVE_RECTIFIER,
    AUDIO_EFFECT_MOORER_REVERB,
    AUDIO_EFFECT_NETWORK_REVERB,
    AUDIO_EFFECT_PHASER,
    AUDIO_EFFECT_SLAPBACK,
    AUDIO_EFFECT_TREMOLO,
    AUDIO_EFFECT_WAHWAH,
    AUDIO_EFFECT_OLD_PHONE,
    AUDIO_EFFECT_DEESSER,
    AUDIO_EFFECT_TROLL_VOICE
};

enum VoipSnapMode {
    SNAP_HIGH_RATE_FOR_REENCODE,
    SNAP_LOW_RATE_FOR_TRANSFER,
    SNAP_SMALL_SIZE_FOR_PTS
};



#if __PLATFORM_WINPHONE || WINDOWS_PHONE
  // See http://developer.nokia.com/Community/Wiki/How_to_create_a_DirectX_texture_with_a_picture
  // to find out how to get pixel data from BitmapImage object
  struct WP8BitmapDesc  // must be passed to CreateBitmap()
  {
    unsigned int   *pixels;
    unsigned       width;
    unsigned       height;
  };
#endif

};  //namespace voip
#endif

#if defined(__cplusplus)
	#define VOIP_EXPORT_C extern "C"
#else
	#define VOIP_EXPORT_C extern
#endif


#ifdef __OBJC__
  #if TARGET_OS_IPHONE || __PLATFORM_IOS
    #define WebrtcRenderView RenderViewIOS
    #import <UIKit/UIKit.h>
    @interface WebrtcRenderView : UIView {}
    @end
    //
    // CallKit integration
    //
    VOIP_EXPORT_C NSString *const VoipCallKitAudioSessionChangedNotification;
    VOIP_EXPORT_C NSString *const VoipCallKitAudioSessionActiveKey;
    VOIP_EXPORT_C NSString *const VoipCallKitWaitForAudioSessionActivationNotification;
    VOIP_EXPORT_C NSString *const VoipCallKitWaitForAudioSessionDeactivationNotification;

  #else
    #define WebrtcRenderView RenderViewOSX
    #import <Cocoa/Cocoa.h>
    @interface WebrtcRenderView : NSView {}
    -(void)startFullScreenAnimation;
    -(void)finishFullScreenAnimation;
    @end
  #endif
#endif

#endif
