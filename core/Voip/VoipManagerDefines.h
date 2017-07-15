#ifndef __VOIP_MANAGER_DEFINES_H__
#define __VOIP_MANAGER_DEFINES_H__

#include "libvoip/include/voip/voip2.h"
#include <string>
#include <vector>

namespace voip_manager
{
    enum eNotificationTypes
    {
        kNotificationType_Undefined = 0,

        kNotificationType_CallCreated,
        kNotificationType_CallInvite,
        kNotificationType_CallOutAccepted,
        kNotificationType_CallInAccepted,
        kNotificationType_CallConnected,
        kNotificationType_CallDisconnected,
        kNotificationType_CallDestroyed,
        kNotificationType_CallPeerListChanged,

        kNotificationType_QualityChanged,

        kNotificationType_MediaLocAudioChanged,
        kNotificationType_MediaLocVideoChanged,
        kNotificationType_MediaRemVideoChanged,
        kNotificationType_MediaRemAudioChanged,

        kNotificationType_DeviceListChanged,
        kNotificationType_DeviceStarted,
        kNotificationType_DeviceMuted,
        kNotificationType_DeviceVolChanged,
        kNotificationType_DeviceInterrupt,

        kNotificationType_MouseTap,
        kNotificationType_ButtonTap,
        kNotificationType_LayoutChanged,

        kNotificationType_MissedCall,
        kNotificationType_ShowVideoWindow,
        kNotificationType_FrameSizeChanged,
        kNotificationType_VoipResetComplete,
        kNotificationType_VoipWindowRemoveComplete,
        kNotificationType_VoipWindowAddComplete,
        kNotificationType_CipherStateChanged,

        kNotificationType_MinimalBandwidthChanged,
        kNotificationType_MaskEngineEnable,
        kNotificationType_LoadMask,

        kNotificationType_ConnectionDestroyed,
		kNotificationType_MainVideoLayoutChanged,
    };

    struct VoipProxySettings
    {
        enum eProxyType
        {
            kProxyType_None = 0,
            kProxyType_Http,
            kProxyType_Socks4,
            kProxyType_Socks4a,
            kProxyType_Socks5
        };

        eProxyType   type;
        std::wstring serverUrl;
        std::wstring userName;
        std::wstring userPassword;

        VoipProxySettings() : type(kProxyType_None) { }
    };

    struct DeviceState
    {
        voip2::DeviceType type;
        std::string       uid;
        bool              success;
    };

    struct DeviceMute
    {
        voip2::DeviceType type;
        bool              mute;
    };

    struct CipherState
    {
        enum State
        {
            kCipherStateUnknown,
            kCipherStateEnabled,
            kCipherStateFailed,
            kCipherStateNotSupportedByPeer
        } state;
        std::string secureCode;
    };

    struct DeviceVol
    {
        voip2::DeviceType type;
        float             volume;
    };

    struct DeviceInterrupt
    {
        voip2::DeviceType type;
        bool              interrupt;
    };

    struct MouseTap
    {
        std::string account;
        std::string contact;

        void* hwnd;

        voip2::MouseTap tap;
        voip2::ViewArea area;
    };

    struct ButtonTap
    {
        std::string account;
        std::string contact;

        void* hwnd;

        voip2::ButtonType type;
    };

    struct MissedCall
    {
        std::string account;
        std::string contact;

        unsigned   ts;
    };

    struct LayoutChanged
    {
        bool  tray;
        voip2::LayoutType layout_type;
        void* hwnd;
    };

    struct FrameSize
    {
        intptr_t hwnd;
        float    aspect_ratio;
    };

    struct Contact
    {
        std::string account;
        std::string contact;

        Contact()
        {
        }

        Contact(const std::string& account_, const std::string& contact_) : account(account_), contact(contact_)
        {
        }

        bool operator==(const Contact& otherContact) const
        {
            return otherContact.account == account && otherContact.contact == contact;
        }
    };

    struct ContactEx
    {
        Contact  contact;
        unsigned call_count;
        unsigned connection_count;
        bool     incoming;

        // Affected window for call with this contact.
        std::vector<void*> windows;

        ContactEx() : call_count(0), incoming(false)
        {

        }
    };

    struct device_description
    {
        std::string uid;
        std::string name;
        voip2::DeviceType type;
        bool isActive;
    };

    struct device_list
    {
        voip2::DeviceType type;
        std::vector<voip_manager::device_description> devices;
    };

    struct VideoEnable
    {
        bool enable;
        Contact  contact;
    };

    enum { kAvatarRequestId          = 0xb00b1e               };
    enum { kAvatarDefaultSize        = 140                    };
	enum { kAvatarRequestSize        = 650                    };
    enum { kNickTextW                = kAvatarDefaultSize * 2 };
    enum { kNickTextH                = 36                     };
    enum { kDetachedWndAvatarSize    = 180                    };
    enum { kLogoFromBoundOffset      = 15                     };
    enum { kUseVoipProtocolAsDefault = 1                      };
	enum { kIncomingWndAvatarSize	 = 120					  };

    struct BitmapDescription
    {
        void*    data;
        unsigned size;

        unsigned w;
        unsigned h;
    };

    struct VoipProtoMsg
    {
        voip2::VoipOutgoingMsg msg;
        std::string            request;

        std::string requestId;
        unsigned messageId;
    };

    struct UserBitmap
    {
        voip2::AvatarType type;
        std::string       contact;
        BitmapDescription bitmap;
    };

    struct WindowBitmap
    {
        void* hwnd;
        BitmapDescription bitmap;
    };

    struct WindowParams
    {
        void* hwnd;
        WindowBitmap watermark;
        WindowBitmap cameraStatus;
        WindowBitmap calling;
        bool  isPrimary;
        bool  isSystem;
        float scale;

        BitmapDescription normalButton;
        BitmapDescription highlightedButton;
        BitmapDescription pressedButton;
        BitmapDescription disabledButton;
    };

    struct EnableParams
    {
        bool  enable;
    };

	struct NamedResult
	{
		std::string name;
		bool		result;
	};

    struct WindowState
    {
        void* hwnd;
        bool value;
    };

    struct ContactsList
    {
        std::vector<Contact> contacts;
        std::vector<void*>   windows;
        bool                 isActive;
    };

    enum ConferenceLayout
    {
        ConferenceAllTheSame = 0,
        ConferenceOneIsBig,    
    };

	enum MainVideoLayoutType
	{
		MVL_OUTGOING = 0,
		MVL_CONFERENCE,
		MVL_SIGNLE_CALL	
	};

	struct MainVideoLayout
	{
		void* hwnd;
		MainVideoLayoutType type;	
		MainVideoLayout() : hwnd(nullptr), type(MVL_OUTGOING) {};
		MainVideoLayout(void* hwnd_, MainVideoLayoutType type_) : hwnd(hwnd_), type(type_) {};
	};

    class ICallManager
    {
    public:
        virtual ~ICallManager() { }
    public:
        virtual void        call_set_proxy      (const VoipProxySettings& proxySettings) = 0;
        virtual void        call_create         (const Contact& contact, bool video, bool add)= 0;
        virtual void        call_stop           ()                                  = 0;
        virtual void        call_stop_smart     (const std::function<void()>&)            = 0;
        virtual void        call_accept         (const Contact& contact, bool video)= 0;
        virtual void        call_decline        (const Contact& contact, bool busy) = 0;
        virtual unsigned    call_get_count      ()                                  = 0;
        virtual bool        call_have_established_connection ()                     = 0;
        virtual bool        call_have_call      (const Contact& contact)            = 0;
        virtual void        call_request_calls  ()                                  = 0;
                
        virtual void        mute_incoming_call_sounds(bool mute)                    = 0;

		virtual void        minimal_bandwidth_switch()								= 0;
        virtual bool        has_created_call()                                     = 0;
    };

    class IDeviceManager
    {
    public:
        virtual ~IDeviceManager() { }
    public:
        virtual unsigned get_devices_number(voip2::DeviceType device_type)                                                                     = 0;
        virtual bool     get_device_list   (voip2::DeviceType device_type, std::vector<device_description>& dev_list)                          = 0;

        virtual bool     get_device        (voip2::DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) = 0;
        virtual void     set_device        (voip2::DeviceType device_type, const std::string& device_guid) = 0;

        virtual void     set_device_mute   (voip2::DeviceType deviceType, bool mute)                                                           = 0;
        virtual bool     get_device_mute   (voip2::DeviceType deviceType)                                                                      = 0;

        virtual void     set_device_volume (voip2::DeviceType deviceType, float volume)                                                        = 0;
        virtual float    get_device_volume (voip2::DeviceType deviceType)                                                                      = 0;

        virtual void     update            ()                                                                                                  = 0;
    };

    class IWindowManager
    {
    public:
        virtual ~IWindowManager() { }
    public:
        virtual void window_add          (voip_manager::WindowParams& windowParams) = 0;
        virtual void window_remove       (void* hwnd) = 0;

        virtual void window_set_bitmap   (const WindowBitmap& bmp) = 0;
        virtual void window_set_bitmap   (const UserBitmap& bmp) = 0;

        virtual void window_set_conference_layout(void* hwnd, voip_manager::ConferenceLayout layout) = 0;
        virtual void window_switch_aspect(const std::string& contact, void* hwnd) = 0;

        virtual void window_set_offsets  (void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) = 0;
        virtual void window_add_button   (voip2::ButtonType type, voip2::ButtonPosition position) = 0;

		virtual void window_set_primary(void* hwnd, const std::string& contact) = 0;
    };

    class IConnectionManager
    {
    public:
        virtual ~IConnectionManager() { }
    public:
        virtual void ProcessVoipMsg(const std::string& account_uid, voip2::VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len) = 0;
        virtual void ProcessVoipAck(const std::string& account_uid, const voip_manager::VoipProtoMsg& msg, bool success) = 0;
    };

    class IMediaManager
    {
    public:
        virtual ~IMediaManager() { }
    public:
        virtual void media_video_en      (bool enable)       = 0;
        virtual void media_audio_en      (bool enable)       = 0;

        virtual bool local_video_enabled ()                  = 0;
        virtual bool local_audio_enabled ()                  = 0;

        //virtual bool remote_video_enabled()                  = 0;
        virtual bool remote_video_enabled(const std::string& account, const std::string& contact) = 0;

        virtual bool remote_audio_enabled()                  = 0;
        virtual bool remote_audio_enabled(const std::string& account, const std::string& contact) = 0;
    };

	class IMaskManager
	{
	public:
		virtual ~IMaskManager() { }
	public:
		virtual void load_mask(const std::string& path) = 0;

		virtual unsigned version() = 0;

        virtual void set_model_path(const std::string& path) = 0;

        virtual void init_mask_engine() = 0;
	};


    class IVoipManager
    {
    public:
        virtual ~IVoipManager() { }
    public:
        virtual void reset() = 0;
    };
}

#endif//__VOIP_MANAGER_DEFINES_H__