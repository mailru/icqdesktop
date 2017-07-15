#include "stdafx.h"

#include "VoipManager.h"
#include "VoipSerialization.h"
#include "../connections/base_im.h"
#include "../core.h"

#include "call_busy.h"
#include "call_connected.h"
#include "call_dial.h"
#include "call_end.h"
#include "call_hold.h"
#include "call_incoming.h"
#include "call_start.h"

#include "../utils.h"
#include "../../common.shared/version_info.h"
#include <mutex>
#include "VoipProtocol.h"
#include <time.h>
#include "../async_task.h"

#ifdef _WIN32
    #include <windows.h>
    #include <assert.h>
    #include <hash_map>
#endif

#if __APPLE__
    #include <CoreFoundation/CoreFoundation.h>
    #include <ImageIO/ImageIO.h>
#endif

#define ANIMATION_R 172//89
#define ANIMATION_G 172
#define ANIMATION_B 172


#if defined(_DEBUG) || defined(DEBUG)
    #ifdef _WIN32
        #define VOIP_ASSERT(exp) \
            if (!(exp)) { \
                if (::IsDebuggerPresent()) { \
                    __debugbreak(); \
                } else { \
                    const_cast<VoipLogger*>(&_voipLogger)->logOut(__FILE__, __LINE__, __FUNCSIG__, ""); \
                    const_cast<VoipLogger*>(&_voipLogger)->logFlush(); \
                } \
            }
    #elif __APPLE__
        #define VOIP_ASSERT(exp) \
            if (!(exp)) { \
                const_cast<VoipLogger*>(&_voipLogger)->logOut(__FILE__, __LINE__, __PRETTY_FUNCTION__, ""); \
                const_cast<VoipLogger*>(&_voipLogger)->logFlush(); \
            }
    #else
        #define VOIP_ASSERT(exp) (void)0
    #endif
#else//DEBUG
    #define VOIP_ASSERT(exp) (void)0
#endif//DEBUG

#define VOIP_ASSERT_RETURN(exp) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        return; \
    }

#define VOIP_ASSERT_RETURN_VAL(exp,val) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        return (val); \
    }

#define VOIP_ASSERT_ACTION(exp,action) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        action; \
    }

#define VOIP_ASSERT_BREAK(exp) \
    if (!(exp)) { \
        VOIP_ASSERT(false); \
        break; \
    }

namespace voip_manager {
    typedef size_t VoipKey;
}

namespace {
    void* bitmapHandleFromRGBData(unsigned width, unsigned height, unsigned bps, const void *data, bool invert = false);

    inline std::string from_unicode_to_utf8(const std::wstring& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        return cv.to_bytes(str);
    }

    inline std::wstring from_utf8_to_unicode(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        return cv.from_bytes(str.c_str());
    }

    inline double calcDecipateWave(double t, double w, double A, double b) {
        return sin(w * t) * A * exp(-1 * b * t);
    };

    void getPriWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

		// Single call
        ws.avatarMain[voip2::WindowTheme_One].height = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].width  = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].textHeight = voip_manager::kNickTextH * scale;
        ws.avatarMain[voip2::WindowTheme_One].textWidth  = voip_manager::kNickTextW * scale;

		// Conference
        ws.avatarMain[voip2::WindowTheme_Two].height = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_Two].width = voip_manager::kAvatarDefaultSize * scale;
        ws.avatarMain[voip2::WindowTheme_Two].textHeight = voip_manager::kNickTextH * scale;
        ws.avatarMain[voip2::WindowTheme_Two].textWidth = voip_manager::kNickTextW * scale;

		// Outgoing call
		ws.avatarMain[voip2::WindowTheme_Three].height = voip_manager::kAvatarDefaultSize * scale;
		ws.avatarMain[voip2::WindowTheme_Three].width = voip_manager::kAvatarDefaultSize * scale;
		ws.avatarMain[voip2::WindowTheme_Three].textHeight = voip_manager::kNickTextH * scale;
		ws.avatarMain[voip2::WindowTheme_Three].textWidth = voip_manager::kNickTextW * scale;


        ws.previewIsButton = true;

        ws.avatarAnimationCurveLen = 0;
        ws.previewDisable      = false;
        ws.previewSelfieMode   = true;

        ws.avatarMain[voip2::WindowTheme_One].offsetLeft = ws.avatarMain[voip2::WindowTheme_One].offsetRight  = 12 * scale;
        ws.avatarMain[voip2::WindowTheme_One].offsetTop  = ws.avatarMain[voip2::WindowTheme_One].offsetBottom = 12 * scale;
        ws.avatarMain[voip2::WindowTheme_One].offsetVertical = 0;// voip2::Position_HCenter | voip2::Position_VCenter;

        ws.avatarMain[voip2::WindowTheme_Two].offsetLeft = ws.avatarMain[voip2::WindowTheme_Two].offsetRight = 12 * scale;
        ws.avatarMain[voip2::WindowTheme_Two].offsetTop  = ws.avatarMain[voip2::WindowTheme_Two].offsetBottom = 12 * scale;
        ws.avatarMain[voip2::WindowTheme_Two].offsetVertical = 0;// voip2::Position_HCenter | voip2::Position_VCenter;

		ws.avatarMain[voip2::WindowTheme_Two].offsetLeft = ws.avatarMain[voip2::WindowTheme_Two].offsetRight = 12 * scale;
		ws.avatarMain[voip2::WindowTheme_Two].offsetTop = ws.avatarMain[voip2::WindowTheme_Two].offsetBottom = 12 * scale;
		ws.avatarMain[voip2::WindowTheme_Two].offsetVertical = 0;// voip2::Position_HCenter | voip2::Position_VCenter;


        //ws.tray_height_pix    = 200 * scale;//m_fullscreenPanel.GetHeight() + int(BORD_CONTROLS - BORD_UPPER);
        ws.previewBorderWidth = 2 * scale;

        // gray
        ws.previewBorderColorBGRA[0] = 0;
        ws.previewBorderColorBGRA[1] = 0;
        ws.previewBorderColorBGRA[2] = 0;
        ws.previewBorderColorBGRA[3] = 100;

        ws.highlight_border_pix = 2;
        // yellow
        ws.highlight_color_bgra[0] = 225;
        ws.highlight_color_bgra[1] = 158;
        ws.highlight_color_bgra[2] = 47;
        ws.highlight_color_bgra[3] = 0;

        ws.normal_color_bgra[0] = 0;
        ws.normal_color_bgra[1] = 0;
        ws.normal_color_bgra[2] = 0;
        ws.normal_color_bgra[3] = 0;

        ws.header_height_pix = 32 * scale;
        //ws.gap_width_pix = 30 * scale;
        ws.conference.blocksGap = 30 * scale;
        ws.conference.trayHeight = 200 * scale;//m_fullscreenPanel.GetHeight() + int(BORD_CONTROLS - BORD_UPPER);
        ws.conference.trayMaxHeight = 0.4f; // 40% of view height.
        //ws.desired_aspect_ration_in_videoconf = 4.f/3;
        ws.conference.aspectRatio = 4.f / 3;
        //ws.lentaBetweenChannelOffset = 20 * scale;
        //ws.blocksBetweenChannelOffset = 60 * scale;
        ws.conference.useGridAdvance    = false;
        ws.conference.trayChannelsGap   = 8 * scale;
        ws.conference.blocksChannelsGap = 8 * scale;
        ws.conference.forcePrimaryVideoCrop = false;
        ws.conference.alignPrimaryVideoTop  = false;
        ws.conference.useHeaders = true;
        ws.usePreviewBackground = true;

        ws.oldverBackroundHeightPer = 50 * scale;
        ws.oldverBackround_bgra[0] = 0;
        ws.oldverBackround_bgra[1] = 0;
        ws.oldverBackround_bgra[2] = 0;
        ws.oldverBackround_bgra[3] = 120;
        ws.animationTimeMs = 350;


        const float animationPeriodSec = 2.5f;
        // Reconnecting animation.
        {
            ws.avatarAnimationCurveLen = 0;
            for (float t = 0; t < animationPeriodSec; t += 1.f / ANIMATION_CURVE_SAMPLERATE_HZ) {
                double w, A, b, x;
                if (t <= 0.25f*2.f) {
                    w = 3.14 * 12; A = 0.15; b = 1.5f; x = t / animationPeriodSec;
                }
                else {
                    w = 3.14 * 8, A = 0.13; b = 5.0f; x = t / animationPeriodSec - 0.25;
                }
                double ampl = sin(w * x) * A * exp(-1 * b * t);
                ws.avatarAnimationCurve[ws.avatarAnimationCurveLen++] = float(1.f + ampl);
            }
        }

        // Connecting animation for conference.
        {
            voip2::VisualEffectContext& visualEffectContext = ws.avatarMain[voip2::WindowTheme_Two].visualEffect[voip2::kVisualEffectType_Connecting];
            visualEffectContext.animationPeriodMs = 700;
            visualEffectContext.height = 300 * scale;
            visualEffectContext.width = 300 * scale;
            visualEffectContext.frameWidth = 300 * scale;
            visualEffectContext.frameHeight = 300 * scale;
            for (float t = 0; t < animationPeriodSec; t += 1.f / ANIMATION_CURVE_SAMPLERATE_HZ) {
                visualEffectContext.geometryCurve[visualEffectContext.curveLength] = t / animationPeriodSec;
                visualEffectContext.colorBGRACurve[visualEffectContext.curveLength] = (unsigned((1.f - t / animationPeriodSec) * 0xff) << 24) |
#if __PLATFORM_WINDOWS
                (ANIMATION_R << 16) | (ANIMATION_G << 8) | (ANIMATION_B << 0);
#else
                    (ANIMATION_B << 16) | (ANIMATION_G << 8) | (ANIMATION_R << 0);
#endif
                visualEffectContext.curveLength++;
            }
        }
    }

    void getSecWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

        ws.avatarMain[voip2::WindowTheme_One].height = voip_manager::kDetachedWndAvatarSize * scale;
        ws.avatarMain[voip2::WindowTheme_One].width  = voip_manager::kDetachedWndAvatarSize * scale;

        ws.avatarMain[voip2::WindowTheme_Two].height = voip_manager::kDetachedWndAvatarSize * scale;
        ws.avatarMain[voip2::WindowTheme_Two].width  = voip_manager::kDetachedWndAvatarSize * scale;

        ws.avatarMain[voip2::WindowTheme_One].offsetVertical = voip2::Position_HCenter | voip2::Position_VCenter;
        ws.avatarMain[voip2::WindowTheme_Two].offsetVertical = voip2::Position_HCenter | voip2::Position_VCenter;

        ws.previewIsButton  = false;
        ws.avatarAnimationCurveLen = 0;
        ws.previewDisable     = true;
        ws.disable_mouse_events_handler = true;

        ws.conference.aspectRatio = 4.f/3;
        ws.animationTimeMs = 350;
    }

    void getSysWindowSettings(voip2::WindowSettings& ws, const float scale) {
        ::memset(&ws, 0, sizeof(ws));

		ws.avatarMain[voip2::WindowTheme_One].height = voip_manager::kIncomingWndAvatarSize * scale;
		ws.avatarMain[voip2::WindowTheme_One].width  = voip_manager::kIncomingWndAvatarSize * scale;

        ws.avatarMain[voip2::WindowTheme_Two].height = voip_manager::kIncomingWndAvatarSize * scale;
        ws.avatarMain[voip2::WindowTheme_Two].width  = voip_manager::kIncomingWndAvatarSize * scale;

        ws.avatarMain[voip2::WindowTheme_One].offsetVertical = voip2::Position_HCenter | voip2::Position_VCenter;
        ws.avatarMain[voip2::WindowTheme_Two].offsetVertical = voip2::Position_HCenter | voip2::Position_VCenter;

		ws.avatarAnimationCurveLen = 0;

        ws.disable_mouse_events_handler = true;
        ws.previewSolo = true;
		ws.previewSelfieMode = true;
        ws.conference.aspectRatio = 4.f/3;
        ws.animationTimeMs = 350;
    }
}

namespace {
    template<typename __Param>
    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype, const __Param& vtype) {
        core::coll_helper coll(dispatcher.create_collection(), true); 
        ntype >> coll;
        vtype >> coll;
        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }
    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype) {
        core::coll_helper coll(dispatcher.create_collection(), true); 
        ntype >> coll;

        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }
}

#define SIGNAL_NOTIFICATOION(type,key) { \
    if (key) { \
        __signal_to_ui(_dispatcher, type, *key); \
    } else {\
        __signal_to_ui(_dispatcher, type); \
    } \
}

#define PSTN_POSTFIX "@pstn"
#define LOCAL_MAX_PATH 300

#define DB_LEFT    0x01
#define DB_CENTER  0x02
#define DB_RIGHT   0x04
#define DB_TOP     0x08
#define DB_VCENTER 0x10
#define DB_BOTTOM  0x20

namespace {
    struct recti {
        int left;
        int top;
        int right;
        int bottom;
    };

    void* bitmapHandleFromRGBData(unsigned width, unsigned height, unsigned bps, const void *data, bool invert) {
#ifdef _WIN32
        BITMAPINFO bmi              = {0};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biBitCount    = (WORD)bps; // RGB32, RGB24, 16-RGB555
        bmi.bmiHeader.biWidth       = width;
        bmi.bmiHeader.biHeight      = (invert ? -1 : 1)*height;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC hdc = ::GetDC(NULL);
        char *bits;
        HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
        if(data) {
            unsigned bytesPerRow = (bps>>3)*width;
            unsigned off = (0-bytesPerRow)&0x3;
            if(off == 0) {
                memcpy(bits, data, bytesPerRow*height);
            } else {
                const char* src = (const char*)data;
                for(unsigned i = 0; i < height; i++) {
                    memcpy(bits, src, bytesPerRow);
                    src += bytesPerRow;
                    bits += bytesPerRow + off;
                }
            }
        }
        ReleaseDC(NULL, hdc);
        return (void*)hBitmap;
#elif defined (__APPLE__)
        
        // We need to premultiply alpha and swap red and green channel, becuase apple does not support our format.
        unsigned bytesPerRow = (bps>>3)*width;
        std::shared_ptr<unsigned char> pMacCorrectImageDataPtr (new unsigned char[bytesPerRow * height], [=](unsigned char* pbuffer)
            {
                delete[] pbuffer;
            });
        
        memcpy(pMacCorrectImageDataPtr.get(), data, height * bytesPerRow);
        
        // We make premultiplay and swap channels only for RGBA8
        if (bps == 32)
        {
            unsigned char* pMacCorrectImageData = pMacCorrectImageDataPtr.get();
            
            for (int i = 0; i < height; i++)
            {
                unsigned char* pRow = pMacCorrectImageData;
                
                for (int j = 0; j < width; j++)
                {
                    unsigned char red   = pRow[2];
                    unsigned char blue  = pRow[0];
                    
                    pRow[0] = ((unsigned)red * pRow[3]) / 255; // Red
                    pRow[1] = ((unsigned)pRow[1] * pRow[3]) / 255; // Green
                    pRow[2] = ((unsigned)blue * pRow[3]) / 255; // Blue
                    pRow[3] = pRow[3]; // Alpha
                    
                    pRow += 4;
                }
                
                pMacCorrectImageData += bytesPerRow;
            }
        }

        CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
        
        CGContextRef bitmapContext = CGBitmapContextCreate((void *)pMacCorrectImageDataPtr.get(), width, height, 8, bytesPerRow, colorspace, (uint32_t)kCGImageAlphaPremultipliedLast);
        CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
        
        CFRelease(bitmapContext);
        CGColorSpaceRelease(colorspace);
        
        return cgImage;
#endif//WIN32
    }

    void bitmapReleaseHandle(void* hbmp) {
#ifdef _WIN32
        DeleteObject((HBITMAP)hbmp);
#elif defined (__APPLE__)
        CFRelease(hbmp);
#endif//WIN32
    }
}


namespace {
    const std::string protoNameICQ   = "ICQ";
    const std::string protoNameAgent = "Agent";
    const std::string protoNamePstn  = "pstn";

    inline voip_manager::VoipKey getHash(const std::string& account_id, const std::string& user_id) {
        if (account_id.empty()) { return 0; }
        if (user_id.empty()) { return 0; }

        std::string name;
        name.reserve(account_id.size() + sizeof(char) + user_id.size());

        name += account_id;
        name += '#';
        name += user_id;

        assert(!name.empty());
        if (name.empty()) { return 0; }

        std::hash<std::string> __hash;
        return __hash(name);
    }

    const std::string getProtoName(const std::string& account_uid) {
        if (account_uid.empty()) { return ""; }

		return protoNameICQ;

        //if(account_uid.find('@') == std::string::npos || account_uid.find("@uin.icq") != std::string::npos) {
        //    return protoNameICQ;
        //} else if(account_uid.find(PSTN_POSTFIX) != std::string::npos) {
        //    return protoNamePstn;
        //} else {
        //    return protoNameAgent;
        //}
    }

    std::string normalizeUid(const std::string& uid) {
        std::string _uid = uid;
        auto position = _uid.find("@uin.icq");
        if (std::string::npos == position) {
            position = _uid.find(PSTN_POSTFIX);
        }
        if (std::string::npos != position) {
            _uid = _uid.substr(0, position);
        }
        return _uid;
    }

    std::wstring getFilePath() {
#ifdef _WIN32
        wchar_t path[MAX_PATH * 2] = { 0 };
        const auto chars_write = ::GetModuleFileNameW(NULL, path, sizeof(path) / sizeof(path[0]) - 1);
        path[chars_write] = L'\0';

        std::wstring path_str = path;
        const int slash_pos = path_str.rfind(L'\\');
        if (slash_pos >= 0) {
            path_str = path_str.substr(0, slash_pos) + L"\\";
            return path_str;
        }
        return L"";
#else
        char dir[LOCAL_MAX_PATH];
        if (!getcwd(dir, LOCAL_MAX_PATH)) {
            return L"";
        }
        
        std::wstring path = from_utf8_to_unicode(dir);
        if (path.empty()) {
            return L"";
        }
        
        if (path.at(path.size() - 1) != L'/') {
            path += L"/";
        }

        return path;
#endif//WIN32
    }

    std::string getPhoneNumberFromUid(const std::string& uid) {
        const auto pos = uid.find(PSTN_POSTFIX);
        if (std::string::npos != pos) {
            return uid.substr(0, pos);
        }
        
        return "";
    }

    struct uncopyable {
    protected:
        uncopyable() { }
    private:
        uncopyable(const uncopyable&);
        uncopyable& operator=(const uncopyable&);
    };

#ifdef _WIN32
    enum { kMaxPath = MAX_PATH + 1 };
#else 
    enum { kMaxPath = 300 };
#endif
}

namespace voip_manager {

    class VoipLogger : private uncopyable {
        std::ofstream _logFile;
        std::string   _fileName;

    public:
        VoipLogger();
        ~VoipLogger();

        bool logOpen (const std::string& filename);
        void logOut  (const char* file, int line, const char* func, const char* msg);
        void logFlush();
    };

    VoipLogger::VoipLogger() {
    }

    VoipLogger::~VoipLogger() {
        if (_logFile.is_open()) {
            _logFile.flush();
            _logFile.close();
        }
    }

    bool VoipLogger::logOpen(const std::string& filename) {
        if (_logFile.is_open()) {
            _logFile.flush();
            _logFile.close();
        }
        _fileName = filename;
        return true;
    }

    void VoipLogger::logOut(const char* file, int line, const char* func, const char* msg) {
        if (!_logFile.is_open()) {
            if (!_fileName.empty()) {
                _logFile.open(_fileName, std::ios_base::out | std::ios_base::app);
                if (!_logFile.is_open()) {
                    return;
                }
            } else {
                return;
            }   
        }

        time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        _logFile << std::put_time(std::localtime(&now), "%Y-%m-%d %X");;
        _logFile << " <" << file << ", " << line << ">";
        _logFile << " " << func;

#ifdef _WIN32
         _logFile << " [thread:" << ::GetCurrentThreadId() << "] " << msg << std::endl;
#else
         _logFile << " [unknown thread] " << msg << std::endl;
#endif
    }

    void VoipLogger::logFlush() {
        if (_logFile.is_open()) {
            _logFile.flush();
        }
    }


    class VoipManagerImpl 
        : private uncopyable
        , public ICallManager
        , public IDeviceManager
        , public IWindowManager
        , public IVoipManager
        , public IMediaManager
		, public IMaskManager
        , public voip2::VoipObserver
        , public voip2::VoipConnection
        , public IConnectionManager {

        enum eCallState {
            kCallState_Unknown = 0,
            kCallState_Created,

            kCallState_Initiated,
            kCallState_Accepted,

            kCallState_Connected,
            kCallState_Disconnected,
        };

        enum
        {
            kVolumePos_Pre = 0,
            kVolumePos_Cur = 1,
            ///////////////////
            kVolumePos_Tot
        };

        struct VoipSettings {
            bool enableRtpDump;
            bool enableVoipLogs;
            VoipSettings() : enableRtpDump(false), enableVoipLogs(false) { }
        };

        /**
         * ConnectionDesc is description for P2P connection.
         * Video conferension contains list of ConnectionDesc.
         *
         */
        struct ConnectionDesc {
            VoipKey connectionKey;

            std::string account_id; // You account.
            std::string user_id;    // Your companion.


            bool       remote_cam_en;
            bool       remote_mic_en;

            eCallState call_state;
            voip2::SessionEvent close_reason;
        };
        typedef std::shared_ptr<ConnectionDesc> ConnectionDescPtr;
        typedef std::list<ConnectionDescPtr> ConnectionDescList;

        /**
         * CallDesc is description for one talk.
         * Video conferension is one CallDesc with list of ConnectionDesc.
         */
        struct CallDesc
        {
            VoipKey callKey;
            ConnectionDescList connections;

            bool       outgoing;
            unsigned   started; // GetTickCount()
            unsigned   quality; // 0 - 100% 

            bool hasWindow; // Does this call has window or not.

            // Is this call current
            bool current()
            {
                return started > 0;
            }
        };

        typedef std::shared_ptr<CallDesc> CallDescPtr;
        typedef std::list<CallDescPtr> CallDescList;


        struct VoipDesc {
            bool  local_cam_en;
            bool  local_aud_en;
            bool  mute_en;
            bool  incomingSoundsMuted;
            float volume[kVolumePos_Tot];

            std::string aPlaybackDevice;
            std::string aCaptureDevice;
            std::string vCaptureDevice;

            std::string aPlaybackDefDevice;
            std::string aCaptureDefDevice;
            std::string vCaptureDefDevice;

			bool		minimalBandwidth;

            VoipDesc() : local_aud_en(true), local_cam_en(false), mute_en(false), incomingSoundsMuted(false), minimalBandwidth(false)
            {
                volume[kVolumePos_Pre] = 0.0f; 
                volume[kVolumePos_Cur] = 0.0f;
            }
        };

        struct WindowDesc {
            void* handle;
            voip2::LayoutType layout_type;
            float aspectRatio;
            float scaleCoeffitient;
            VoipKey talkKey;
            ConferenceLayout confLayout;
        };

        struct RequestInfo {
            unsigned messageId;
            std::string requestId;
            std::chrono::milliseconds ts;
        };

        std::recursive_mutex    _windowsMx;
        std::vector<WindowDesc> _windows;

        std::recursive_mutex _callsMx;
        CallDescList         _calls;

        bool                           sendViaIm_;
        std::function<void()>          callback_;
        std::shared_ptr<VoipProtocol> _voipProtocol;

        std::recursive_mutex _voipDescMx;
        VoipDesc   _voip_desc;
        bool _defaultDevicesStored;

        std::map<unsigned, RequestInfo> _requestIds;
        std::shared_ptr<voip2::Voip2>  _engine;
        VoipLogger _voipLogger;

        core::core_dispatcher& _dispatcher;
        bool _voipDestroyed;

		// temp value of mute fix, we load it from settings once and reuse.
		bool _voipMuteFix;
            
        std::shared_ptr<core::async_executer> _async_tasks;

		bool _masksEngineInited;

        std::string _maskModelPath;
        
        // Does user try to use mask early?
        bool _needToRunMask;

        // True if PC has local camera;
        bool _hasCamera;

        // Use for call key;
        static uint32_t _autoIncrementKey;

    private:
        std::shared_ptr<voip2::Voip2> _get_engine(bool skipCreation = false);

        bool _init_signaling(std::shared_ptr<voip2::Voip2> engine);
        bool _init_media    (std::shared_ptr<voip2::Voip2> engine);
        bool _init_sounds_sc(std::shared_ptr<voip2::Voip2> engine);
        void _storeDefaultDevices(std::shared_ptr<voip2::Voip2> engine);

        void _update_layout(CallDescPtr callDesc);
        void _update_video_window_state(CallDescPtr call);
        void _set_layout   (void* handle, const voip2::LayoutType& lt);

        bool _call_create (const std::string& account_id, const std::string& user_id, CallDescPtr& desc);
        bool _call_destroy(const VoipKey& callKey);
        bool _call_exists (const VoipKey& callKey);
        bool _call_get    (const VoipKey& callKey, CallDescPtr& desc);
        bool _call_get    (const std::string& account_uid, const std::string& user_uid, CallDescPtr& desc);
        bool _connection_destroy(const std::string& account_uid, const std::string& user_uid);

        bool _connection_get (const std::string& account_uid, const std::string& user_uid, ConnectionDescPtr& connectionDesc);
        unsigned _call_get_connection_count(const VoipKey& talkKey);
        void _call_request_connections(CallDescPtr call);

        bool _call_have_outgoing_connection();

        inline bool _getVoipSettingsPath(std::wstring& path) const;
        inline bool _getVoipLogPath(std::wstring& path) const;
        inline const VoipSettings _getVoipSettings() const;
        inline voip2::ProxyType _toVoipProxyType(const VoipProxySettings::eProxyType& type);

        __inline WindowDesc* _find_window(void* handle);

        void _load_avatars       (const std::string& account_uid, const std::string& user_uid, bool request_if_not_exists = true);
        void _update_peer_list   (CallDescPtr call, const std::string& account_uid, const std::string& user_uid);
        void _update_device_list (voip2::DeviceType dev_type);
        void _notify_remote_video_state_changed(ConnectionDescPtr connection);

        void _protocolSendAlloc  (const char* data, unsigned size); //executes in VOIP SIGNALING THREAD context
        void _protocolSendMessage(const VoipProtoMsg& data); //executes in VOIP SIGNALING THREAD context

        void _window_set_bg(void* hwnd, void* hbmp);
        void _window_set_av(const std::string& contact, voip2::AvatarType type, void* hbmp);

		void check_mute_compatibility(); // Make mute compatible with prev versions.

		void set_speaker_volume_from_settings(); // Set mute or not, depends of sound enable settings.

		void start_video();

		void setupAvatarForeground(); // Setup fade for video.

		void initMaskEngine(const std::string& modelDir); // Init mask engine.
		void loadMask(const std::string& maskPath);		  // Load mask.
            
        void update_media_video_en(bool enable); // Update local camera state.

        void _cleanupCalls(); // Remove empty calls from list.

        void walkThroughCurrentCall(std::function<bool(ConnectionDescPtr)> func);    // Enum all connection of current call.
        void walkThroughAllConnections(std::function<bool(ConnectionDescPtr)> func); // Enum all connections.
        void walkThroughCalls(std::function<bool(CallDescPtr)> func);                // Enum all call.
        bool walkThroughCallConnections(CallDescPtr call, std::function<bool(ConnectionDescPtr)> func);                // Enum all connections of call.

        // @return current active call.
        CallDescPtr get_current_call();

        // @return current outgoing call.
        CallDescPtr get_outgoing_call();

        bool _remote_video_enabled(ConnectionDescPtr connection);

        // @return list of window related with this call.
        std::vector<void*> _get_call_windows(VoipKey callKey);

        CallDescPtr _get_incomming_call_without_window();

        // create new connection.
        ConnectionDescPtr _create_connection(const std::string& account_id, const std::string& user_id);

        // Remove connection with created state.
        void _connection_clear_not_inited(CallDescPtr call);

        void _cleanup_windows_for_call(VoipKey callKey);

        typedef std::unique_ptr<std::vector<voip::hbmp_t>, std::function<void(std::vector<voip::hbmp_t>*)>> HBMPVectorPtr;
        HBMPVectorPtr setupButtonsBitmaps(voip2::WindowSettings& ws, voip_manager::WindowParams& windowParams);

        // Update call states on signal event.
        void _update_call_states(CallDescPtr call);
            
    public:
        //=========================== ICallManager API ===========================
        void call_set_proxy               (const VoipProxySettings& proxySettings) override;
        void call_create                  (const Contact& contact, bool video, bool add) override;
        void call_stop                    () override;
        void call_accept                  (const Contact& contact, bool video) override;
        void call_decline                 (const Contact& contact, bool busy) override;
        unsigned call_get_count           () override;
        bool call_have_established_connection () override;
        bool call_have_call               (const Contact& contact) override;
        void call_request_calls           () override;
        void call_stop_smart              (const std::function<void()>&) override;

        void mute_incoming_call_sounds    (bool mute) override;
		void minimal_bandwidth_switch() override;
        bool has_created_call() override;        

        //=========================== IWindowManager API ===========================
        void window_add           (voip_manager::WindowParams& windowParams) override;
        void window_remove        (void* hwnd) override;
        void window_set_bitmap    (const WindowBitmap& bmp) override;
        void window_set_bitmap    (const UserBitmap& bmp) override;
        void window_set_conference_layout(void* hwnd, voip_manager::ConferenceLayout layout) override;
        void window_switch_aspect (const std::string& contact, void* hwnd) override;
        void window_set_offsets   (void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) override;
        void window_add_button    (voip2::ButtonType type, voip2::ButtonPosition position) override;
		void window_set_primary   (void* hwnd, const std::string& contact) override;

        //=========================== IMediaManager API ===========================
        void media_video_en       (bool enable) override;
        void media_audio_en       (bool enable) override;
        bool local_video_enabled  () override;
        bool local_audio_enabled  () override;
        bool remote_video_enabled (const std::string& account, const std::string& contact) override;
        bool remote_audio_enabled () override;
        bool remote_audio_enabled (const std::string& account, const std::string& contact) override;

		//=========================== IMaskManager API ===========================
		void load_mask(const std::string& path) override;
		unsigned int version() override;
        void set_model_path(const std::string& path) override;
        void init_mask_engine() override;

        //=========================== IVoipManager API ===========================
        void reset() override;

        //=========================== VoipObserver ===========================
        void DeviceListChanged        (voip2::DeviceType deviceType) override;
        void DeviceStatusChanged      (voip2::DeviceType deviceType, const char *uid, voip2::DeviceStatus deviceStatus) override;
        void AudioDeviceVolumeChanged (voip2::DeviceType deviceType, float volume) override;
        void AudioDeviceMuteChanged   (voip2::DeviceType deviceType, bool mute) override;
        void AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) override;
		void VideoDeviceCapabilityChanged(const char* camera_uid, voip2::VideoDeviceCapability caps) override { ; }
        //void AudioDeviceInterrupt     (voip2::DeviceType deviceType, bool is_in_interrupt) override;
        void RenderMouseTap           (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) override;
        void ButtonPressed            (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) override;
        void LayoutTypeChanged        (voip::hwnd_t hwnd, voip2::LayoutType layoutType) override;
        void MissedCall               (const char* account_uid, const char* user_id, unsigned timestamp) override;
        void SessionEvent             (const char* account_uid, const char* user_id, voip2::SessionEvent sessionEvent) override;
        void FrameSizeChanged         (voip::hwnd_t hwnd, float aspectRatio) override;
        void InterruptByGsmCall       (bool gsmCallStarted) override;
        void VideoStreamChanged       (const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) override;
        void MinimalBandwidthMode_StateChanged(bool mbmEnabled) override;
		void MaskModelInitStatusChanged(bool res) override;

		void StillImageReady(const char *data, unsigned len, unsigned width, unsigned height) override {}
		void SnapRecordingStatusChanged(const char* filename, voip2::SnapRecordingStatus snapRecordingStatus, unsigned width, unsigned height, const char *data, unsigned size) override {}
		void FirstFramePreviewForSnapReady(const char *rgb565, unsigned len, unsigned width, unsigned height) override {}


        //=========================== VoipConnection ===========================
        void SendVoipMsg(const char* from, voip2::VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx) override;

        //=========================== IConnectionManager API ===========================
        void ProcessVoipMsg(const std::string& account_uid, voip2::VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len) override;
        void ProcessVoipAck(const std::string& account_uid, const voip_manager::VoipProtoMsg& msg, bool success) override;

        //=========================== IDeviceManager API ===========================
        unsigned get_devices_number(voip2::DeviceType device_type) override;
        bool     get_device_list   (voip2::DeviceType device_type, std::vector<device_description>& dev_list) override;
        bool     get_device        (voip2::DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) override;
        void     set_device        (voip2::DeviceType device_type, const std::string& device_guid) override;
        void     set_device_mute   (voip2::DeviceType deviceType, bool mute) override;
        bool     get_device_mute   (voip2::DeviceType deviceType) override;
        void     set_device_volume (voip2::DeviceType deviceType, float volume) override;
        float    get_device_volume (voip2::DeviceType deviceType) override;
        void     update            () override;

    public:
        VoipManagerImpl (core::core_dispatcher& dispatcher);
        ~VoipManagerImpl();
    };

    uint32_t VoipManagerImpl::_autoIncrementKey = 0;

    inline bool _initFirstDevice(std::shared_ptr<voip2::Voip2> engine, voip2::DeviceType type, std::string& uid) {
        if (!engine) { return false; }
        if (engine->GetDevicesNumber(type) == 0) { return true; }

        unsigned defaultDeviceIx = 0;
        if (voip2::AudioRecording == type || voip2::AudioPlayback == type) {
            defaultDeviceIx = (unsigned)-1;
        }

        char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
        char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
        if (!!engine->GetDevice(type, defaultDeviceIx, name, guid)) {
            engine->SetDevice(type, guid);
            uid = guid;
        }

        return true;
    }

    VoipManagerImpl::VoipManagerImpl(core::core_dispatcher& dispatcher) 
        : _engine(NULL)
        , _voipDestroyed(false)
        , sendViaIm_(false)
        , _defaultDevicesStored(false)
        , _dispatcher(dispatcher)
		, _voipMuteFix(false)
        , _async_tasks(new core::async_executer())
		, _masksEngineInited(false)
        , _needToRunMask(false)
        , _hasCamera(false)
	{

        srand(time(NULL));

        const VoipSettings voipSettings = _getVoipSettings();
        if (voipSettings.enableVoipLogs) {
            std::wstring filePath;
            _getVoipLogPath(filePath);
            assert(!filePath.empty());

            if (!filePath.empty()) {
                const bool openResult = _voipLogger.logOpen(from_unicode_to_utf8(filePath));
                assert(openResult);
            }
        }
    }

    VoipManagerImpl::~VoipManagerImpl() {
        _engine.reset();

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT(_calls.empty());
    }

    bool VoipManagerImpl::_init_signaling(std::shared_ptr<voip2::Voip2> engine) {
        VOIP_ASSERT_RETURN_VAL(engine, false);
        engine->StartSignaling();

        return true;
    }

    bool VoipManagerImpl::_init_media(std::shared_ptr<voip2::Voip2> engine) {
        using namespace voip2;
        VOIP_ASSERT_RETURN_VAL(engine, false);

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        if (!_voip_desc.aPlaybackDevice.empty()) {
            engine->SetDevice(AudioPlayback, _voip_desc.aPlaybackDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, AudioPlayback, _voip_desc.aPlaybackDevice),  false);
        }

        if (!_voip_desc.aCaptureDevice.empty()) {
            engine->SetDevice(AudioRecording, _voip_desc.aCaptureDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, AudioRecording, _voip_desc.aCaptureDevice), false);
        }

        if (!_voip_desc.vCaptureDevice.empty()) {
            engine->SetDevice(VideoCapturing, _voip_desc.vCaptureDevice.c_str());
        } else {
            VOIP_ASSERT_RETURN_VAL(_initFirstDevice(engine, VideoCapturing, _voip_desc.vCaptureDevice), false);
        }

        return true;
    }

    void VoipManagerImpl::_set_layout(void* handle, const voip2::LayoutType& lt) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(engine);

        engine->WindowSetLayoutType(handle, lt);
    }

    void VoipManagerImpl::_update_peer_list(CallDescPtr call, const std::string& account_uid, const std::string& user_uid) {
        VOIP_ASSERT_RETURN(!account_uid.empty());
        VOIP_ASSERT_RETURN(!user_uid.empty());

        using namespace voip2;
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

#ifndef STRIP_VOIP
        ConferenceParticipants cp;
        engine->ShowIncomingConferenceParticipants(user_uid.c_str(), cp);

        if (cp.user_uids) {
            int str_offset = 0;
            while (true) {
                std::string uid = &cp.user_uids[str_offset];
                const unsigned name_len = (unsigned)uid.size();
                if (!name_len) { break; }

                uid = normalizeUid(uid);
                VOIP_ASSERT_BREAK(!uid.empty());

                ConnectionDescPtr newConnection = _create_connection(normalizeUid(account_uid), normalizeUid(uid));
                if (newConnection)
                {
                    call->connections.push_back(newConnection);
                }
                str_offset += name_len + 1;
            }
        }
        _call_request_connections(call);
#endif //STRIP_VOIP
    }

    bool VoipManagerImpl::_getVoipSettingsPath(std::wstring& path) const {
        path.clear();
        path.reserve(kMaxPath);

        path += core::utils::get_product_data_path();
        VOIP_ASSERT_RETURN_VAL(!path.empty(), false);

#ifdef _WIN32
        path += L"\\settings\\voip_config.txt";
#else 
        path += L"/settings/voip_config.txt";
#endif
        return true;
    }

    bool VoipManagerImpl::_getVoipLogPath(std::wstring& path) const {
        path.clear();
        path.reserve(kMaxPath);

        path += core::utils::get_product_data_path();
        VOIP_ASSERT_RETURN_VAL(!path.empty(), false);

#ifdef _WIN32
        path += L"\\voip_log.txt";
#else
        path += L"/voip_log.txt";
#endif
        return true;
    }

    const VoipManagerImpl::VoipSettings VoipManagerImpl::_getVoipSettings() const {
        VoipSettings settings;
        
        std::wstring filePath;
        _getVoipSettingsPath(filePath);
        VOIP_ASSERT_RETURN_VAL(!filePath.empty(), settings);

        std::ifstream fileStream;
        fileStream.open(from_unicode_to_utf8(filePath));
        if (!fileStream.is_open()) { return settings; }

        auto __removeSpaces = [] (std::string& str) {
            str.erase (std::remove(str.begin(), str.end(), ' '), str.end());
        };

        while (fileStream.good()) {
            std::string line;
            std::getline(fileStream, line);

            const std::string::size_type position = line.find('=');
            VOIP_ASSERT_ACTION(position != std::string::npos, continue);

            std::string param = line.substr(0, position);
            VOIP_ASSERT_ACTION(!param.empty(), continue);

            std::string value = line.substr(position + 1);
            VOIP_ASSERT_ACTION(!value.empty(), continue);

            __removeSpaces(param);
            __removeSpaces(value);
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (std::string::npos != param.find("enablertpdump")) {
                settings.enableRtpDump = value[0] != '0';
            } else if (std::string::npos != param.find("enablevoiplogs")) {
                settings.enableVoipLogs = value[0] != '0';
            }
        }

        return settings;
    }

    void VoipManagerImpl::_update_layout(CallDescPtr callDesc) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        int counter = 0;
		int activeCounter = 0;

        for (auto it = callDesc->connections.begin(); it != callDesc->connections.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (!callDesc->outgoing && call->call_state != kCallState_Accepted && 
                call->call_state != kCallState_Connected && call->call_state != kCallState_Disconnected) {
                continue;
            } else {
				if (call->call_state >= kCallState_Accepted)
				{
					activeCounter++;
				}				
                ++counter;
            }
        }

        if (!counter) {
            return;
        }

        const bool active_video_conf = activeCounter > 0 && counter > 1;
		const bool outgoing          = activeCounter == 0;

        std::for_each(_windows.begin(), _windows.end(), [active_video_conf, this, callDesc, engine, outgoing](WindowDesc& window_desc) {
                if (window_desc.talkKey == callDesc->callKey)
                {
                    using namespace voip2;
                    LayoutType lt;

					MainVideoLayout mainLayout;
					mainLayout.hwnd = window_desc.handle;
					if (outgoing)
					{
						lt = LayoutType_One;
						engine->WindowSetTheme(window_desc.handle, voip::WindowTheme_Three);
						mainLayout.type = MVL_OUTGOING;
						SIGNAL_NOTIFICATOION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
                        engine->WindowShowButton(voip2::ButtonType_Close, false);
					}
					else if (active_video_conf)
                    {
                        lt = (window_desc.confLayout == ConferenceAllTheSame ? LayoutType_Two : LayoutType_Four);
                        engine->WindowSetTheme(window_desc.handle, voip::WindowTheme_Two);
						mainLayout.type = MVL_CONFERENCE;
						SIGNAL_NOTIFICATOION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
                        engine->WindowShowButton(voip2::ButtonType_Close, true);
                    }
                    else
                    {
                        lt = LayoutType_One;
                        engine->WindowSetTheme(window_desc.handle, voip::WindowTheme_One);
						mainLayout.type = MVL_SIGNLE_CALL;
						SIGNAL_NOTIFICATOION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
                        engine->WindowShowButton(voip2::ButtonType_Close, false);
                    }

                    _set_layout(window_desc.handle, lt);
                }
            });

        if (active_video_conf)
        {
            window_add_button(voip2::ButtonType_Close, voip2::ButtonPosition_TopRight);
        }
    }

    bool VoipManagerImpl::_init_sounds_sc(std::shared_ptr<voip2::Voip2> engine) {
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        engine->SetSound(voip2::SoundEvent_HangupRemoteBusy, call_busy_data, sizeof(call_busy_data));
        engine->SetSound(voip2::SoundEvent_Connected, call_connected_data, sizeof(call_connected_data));
        engine->SetSound(voip2::SoundEvent_WaitingForAccept_Confirmed, call_dial_data, sizeof(call_dial_data));
        engine->SetSound(voip2::SoundEvent_HangupLocal, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupRemote, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupHandledByAnotherInstance, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_HangupByError, call_end_data, sizeof(call_end_data));
        engine->SetSound(voip2::SoundEvent_Hold, call_hold_data, sizeof(call_hold_data));
        engine->SetSound(voip2::SoundEvent_IncomingInvite, call_incoming_data, sizeof(call_incoming_data));
        engine->SetSound(voip2::SoundEvent_WaitingForAccept, call_start_data, sizeof(call_start_data));

        return true;
    }

    std::shared_ptr<voip2::Voip2> VoipManagerImpl::_get_engine(bool skipCreation/* = false*/) {
        if (!_engine) {

            if (_voipDestroyed) {
                return NULL;
            }
            
            if (skipCreation) {
                return NULL;
            }

            using namespace voip2;
            std::string stat_file_path = from_unicode_to_utf8(core::utils::get_product_data_path());
            VOIP_ASSERT_RETURN_VAL(!stat_file_path.empty(), NULL);

            std::string app_name;
            app_name += "icq.desktop ";
            app_name += core::tools::version_info().get_version();

#ifndef STRIP_VOIP
            _engine.reset(Voip2::CreateVoip2(*this, *this, app_name.c_str(), stat_file_path.c_str()), [] (voip2::Voip2* obj) {
                if (obj) {
                    voip2::Voip2::DestroyVoip2(obj);
                }
            });
#endif //STRIP_VOIP
            VOIP_ASSERT_RETURN_VAL(!!_engine, NULL);
            VOIP_ASSERT_RETURN_VAL(!!_engine->Init(), NULL);

            _engine->EnableMsgQueue();
            _init_signaling(_engine);
            _init_media    (_engine);
            _init_sounds_sc(_engine);
            

            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _engine->MuteAllIncomingSoundNotifications(_voip_desc.incomingSoundsMuted);
            _storeDefaultDevices(_engine);
            _defaultDevicesStored = true;

            const VoipSettings voipSettings = _getVoipSettings();
            _engine->EnableRtpDump(voipSettings.enableRtpDump);

            _update_device_list(AudioPlayback);
            _update_device_list(AudioRecording);
            _update_device_list(VideoCapturing);            
        }

        return _engine;
    }

    void VoipManagerImpl::_storeDefaultDevices(std::shared_ptr<voip2::Voip2> engine) {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        const voip2::DeviceType devices [] = { voip2::AudioRecording, voip2::AudioPlayback };

        for (unsigned ix = 0; ix < sizeof(devices) / sizeof(devices[0]); ++ix) {
            const voip2::DeviceType device = devices[ix];
            if (engine->GetDevicesNumber(device) == 0) { continue; }

            unsigned defaultDeviceIx = 0;
            if (voip2::AudioRecording == device || voip2::AudioPlayback == device) {
                defaultDeviceIx = (unsigned)-1;
            }

            std::string& defaultDevice = voip2::AudioPlayback == device ? _voip_desc.aPlaybackDefDevice :
                                         voip2::AudioRecording == device ? _voip_desc.aCaptureDefDevice :
                                         _voip_desc.vCaptureDefDevice;


            char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
            char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
            if (!!engine->GetDevice(device, defaultDeviceIx, name, guid)) {
                defaultDevice = guid;
            }
        }
    }

    bool VoipManagerImpl::_call_exists(const VoipKey& callKey) {
        CallDescPtr desc;
        return _call_get(callKey, desc);
    }

    bool VoipManagerImpl::_call_create(const std::string& account_id, const std::string& user_id, CallDescPtr& desc) {
        VOIP_ASSERT_RETURN_VAL(!account_id.empty(), false);
        VOIP_ASSERT_RETURN_VAL(!user_id.empty(), false);
        
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        ConnectionDescPtr connection = _create_connection(account_id, user_id);
        VOIP_ASSERT_RETURN_VAL(!!connection, false);

        desc.reset(new(std::nothrow) CallDesc());
        VOIP_ASSERT_RETURN_VAL(!!desc, false);

        desc->callKey  = ++_autoIncrementKey;
        desc->started  = 0;
        desc->outgoing = false;
        desc->quality  = 0;
        desc->hasWindow = false;        

        desc->connections.push_back(connection);

        _calls.push_back(desc);

        return true;
    }

    void VoipManagerImpl::_notify_remote_video_state_changed(ConnectionDescPtr connection) {
        const bool remote_video_en = _remote_video_enabled(connection);

        VideoEnable videoEnable = { remote_video_en , Contact(connection->account_id, connection->user_id)};

        SIGNAL_NOTIFICATOION(kNotificationType_MediaRemVideoChanged, &videoEnable);
    }

    bool VoipManagerImpl::_call_destroy(const VoipKey& key) {
        VOIP_ASSERT_RETURN_VAL(!!key, false);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        bool found = false;

        walkThroughCalls([key, this](CallDescPtr call) {
                if (call->callKey == key)
                {
                    VOIP_ASSERT_RETURN_VAL(call->connections.empty(), true);

                    _cleanup_windows_for_call(call->callKey);

                    call->connections.clear();
                    return true;
                }

                return false;
            });


        _cleanupCalls();

        // Reset flag if last call was removed.
        if (_calls.empty())
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.local_cam_en = false;
#ifdef __APPLE__
            // Under mac microfone state is reseted on new call.
            _voip_desc.local_aud_en = true;
#endif
            _needToRunMask = false;
            loadMask(""); // reset mask
        }

        return found;
    }

    void VoipManagerImpl::media_video_en(bool enable) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        // We enable camera only if we have camera.
        bool videoEnable = _hasCamera && enable;
        engine->EnableOutgoingVideo(videoEnable);

        update_media_video_en(videoEnable);
    }

	void VoipManagerImpl::start_video()
	{
		auto engine = _get_engine();
		VOIP_ASSERT_RETURN(!!engine);

		engine->EnableOutgoingVideo(true);
	}
    
    void VoipManagerImpl::update_media_video_en(bool enable) {
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.local_cam_en = enable;
        }
        
        SIGNAL_NOTIFICATOION(kNotificationType_MediaLocVideoChanged, &enable);

        // Update all windows.
        walkThroughCalls([this](VoipManagerImpl::CallDescPtr call) {
            _update_video_window_state(call);
            return false;
        });        
    }

    void VoipManagerImpl::media_audio_en(bool enable) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetDeviceMute(voip2::AudioRecording, !enable);

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        _voip_desc.local_aud_en = enable;

        SIGNAL_NOTIFICATOION(kNotificationType_MediaLocAudioChanged, &enable);
    }

    bool VoipManagerImpl::local_video_enabled() {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_cam_en;
    }

    bool VoipManagerImpl::local_audio_enabled() {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_aud_en;
    }

    bool VoipManagerImpl::_remote_video_enabled(ConnectionDescPtr connection) {
        return connection->remote_cam_en;
        //std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        //bool res = false;
        //walkThroughCurrentCall([&res](ConnectionDescPtr connection) -> bool
        //    {   
        //    res = connection->remote_cam_en;
        //        return true;
        //    });

        //auto call = get_current_call();
        //if (call)
        //{
            //for (auto connection = call->connections.begin(); connection != call->connections.end(); ++connection) {
            //    VOIP_ASSERT_ACTION(!!(*connection), continue);

            //    if ((*connection)->remote_cam_en) {
            //        return true;
            //    }
            //}
        //}
        //return res;
    }

    bool VoipManagerImpl::remote_video_enabled(const std::string& account, const std::string& contact) {
        ConnectionDescPtr connectionDesc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT_RETURN_VAL(_connection_get(account, contact, connectionDesc), false);

        return connectionDesc->remote_cam_en;
    }

    void VoipManagerImpl::VideoStreamChanged(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, bool havePicture) {
        
    }

    void VoipManagerImpl::MinimalBandwidthMode_StateChanged(bool mbmEnabled)
    {
		{
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
			_voip_desc.minimalBandwidth = mbmEnabled;
        }

		EnableParams minimalBandwidth;
		minimalBandwidth.enable = mbmEnabled;
        SIGNAL_NOTIFICATOION(kNotificationType_MinimalBandwidthChanged, &minimalBandwidth);
    }

    bool VoipManagerImpl::remote_audio_enabled() {
        bool res = false;
        walkThroughCurrentCall([&res](VoipManagerImpl::ConnectionDescPtr connection) -> bool {
                res = connection->remote_mic_en;
                return res;
            });
        return res;
        //std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        //for (unsigned ix = 0; ix < _calls.size(); ++ix) {
        //    auto call = _calls[ix];
        //    VOIP_ASSERT_ACTION(!!call, continue);

        //    if (call->remote_mic_en) {
        //        return true;
        //    }
        //}
        //return false;
    }

    bool VoipManagerImpl::remote_audio_enabled(const std::string& account, const std::string& contact) {
        ConnectionDescPtr desc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT_RETURN_VAL(_connection_get(account, contact, desc), false);

        return desc->remote_mic_en;
    }

    bool VoipManagerImpl::_call_get(const VoipKey& callKey, CallDescPtr& desc) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        bool res = false;

        walkThroughCalls([&callKey, &res, &desc](CallDescPtr call) -> bool
            {
                if (call->callKey == callKey)
                {
                    desc = call;
                    res = true;
                }

                return res;
            });

        //auto foundCall = std::find_if(_calls.begin(), _calls.end(), [&callKey, this](CallDescPtr call)
        //{
        //    VOIP_ASSERT_ACTION(!!call, return false);
        //    return call->callKey == callKey;
        //});

        //if (foundCall != _calls.end())
        //{
        //    desc = *foundCall;
        //    res = true;
        //}

        return res;
    }

    bool VoipManagerImpl::_call_get(const std::string& account_uid, const std::string& user_uid, CallDescPtr& desc)
    {
        bool res = false;
        walkThroughCalls([&account_uid, &user_uid, &res, this, &desc](CallDescPtr call) {
            res = walkThroughCallConnections(call, [&res, &account_uid, &user_uid](VoipManagerImpl::ConnectionDescPtr connection) -> bool {
                return connection->account_id == account_uid && connection->user_id == user_uid;                
            });

            if (res)
            {
                desc = call;
            }

            return res;
        });

        return res;
    }

    voip2::ProxyType VoipManagerImpl::_toVoipProxyType(const VoipProxySettings::eProxyType& type) {
        switch (type) {
        case VoipProxySettings::kProxyType_None:    return voip2::ProxyType_None;
        case VoipProxySettings::kProxyType_Http:    return voip2::ProxyType_HTTP;
        case VoipProxySettings::kProxyType_Socks4:  return voip2::ProxyType_SOCKS4;
        case VoipProxySettings::kProxyType_Socks4a: return voip2::ProxyType_SOCKS4;
        case VoipProxySettings::kProxyType_Socks5:  return voip2::ProxyType_SOCKS5;
        }

        VOIP_ASSERT(false);
        return voip2::ProxyType_None;
    }

    void VoipManagerImpl::call_set_proxy(const VoipProxySettings& proxySettings) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetProxyPrms(
            _toVoipProxyType(proxySettings.type), 
            from_unicode_to_utf8(proxySettings.serverUrl).c_str(), 
            from_unicode_to_utf8(proxySettings.userName).c_str(), 
            from_unicode_to_utf8(proxySettings.userPassword).c_str()
            );
    }

    void VoipManagerImpl::call_create(const Contact& contact, bool video, bool add) {
        VOIP_ASSERT_RETURN(!contact.account.empty());
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        bool haveCall = get_outgoing_call() != nullptr || get_current_call() != nullptr;// !_calls.empty();

        if (haveCall && !add) {
            bool localVideoEnabled;
            {
                std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
                localVideoEnabled = _voip_desc.local_cam_en;
            }
            if (video && !localVideoEnabled) {
                media_video_en(true);
            } else if (!video && localVideoEnabled) {
                media_video_en(false);
            }
            return;
        }

        // Append connection to current call.
        if (add)
        {
            CallDescPtr currentCall = get_current_call();
            if (!currentCall)
            {
                currentCall = get_outgoing_call();
            }
            VOIP_ASSERT_RETURN(!!currentCall);
            ConnectionDescPtr connection = _create_connection(contact.account, contact.contact);
            VOIP_ASSERT_RETURN(!!connection);
            {
                std::lock_guard<std::recursive_mutex> __lock(_callsMx);
                currentCall->connections.push_back(connection);
            }
        }
		else
		{
			// Disable for add user to conference.
			media_video_en(video);
			check_mute_compatibility();
		}
        engine->CallStart(contact.account.c_str(), contact.contact.c_str());
    }

    void VoipManagerImpl::call_stop() {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->CallStop();
    }

    void VoipManagerImpl::call_stop_smart(const std::function<void()>& callback) {
        auto engine = _get_engine(true);
        if(!engine) {
            callback();
            return;
        }

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        const bool haveCalls = !_calls.empty();
        callback_ = callback;

        // Stop incomming/outgoing calls.
        for (auto it = _calls.begin(); it != _calls.end(); ++it) {
            for (auto itconnection = (*it)->connections.begin(); itconnection != (*it)->connections.end(); ++itconnection) {
                auto call = *itconnection;
                VOIP_ASSERT_ACTION(!!call, continue);
                if (!(*it)->outgoing && call->call_state != kCallState_Accepted) {
                    engine->CallDecline(call->user_id.c_str());
                }
            }
        }

        // Stop active call.
        engine->CallStop();

        if (haveCalls) {
            sendViaIm_ = true;
        } else {
            callback();
            callback_ = std::function<void()>();
        }
    }

    void VoipManagerImpl::call_accept(const Contact& contact, bool video) {
        VOIP_ASSERT_RETURN(!contact.account.empty());
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        media_video_en(video);
		check_mute_compatibility();
        engine->CallAccept(contact.account.c_str(), contact.contact.c_str());
    }

    void VoipManagerImpl::call_decline(const Contact& contact, bool busy) {
        VOIP_ASSERT_RETURN(!contact.contact.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->CallDecline(contact.contact.c_str(), busy);
    }

    unsigned VoipManagerImpl::call_get_count() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        return _calls.size();
    }

    unsigned VoipManagerImpl::_call_get_connection_count(const VoipKey& talkKey) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        unsigned res = 0;
        CallDescPtr call;
        _call_get(talkKey, call);
        if (call)
        {
            res = call->connections.size();
        }
        return res;
    }

    void VoipManagerImpl::_call_request_connections(CallDescPtr call) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        ContactsList contacts;
        //std::vector<Contact> connections;

        if (call)
        {
            contacts.contacts.reserve(call->connections.size());

            walkThroughCallConnections(call, [&contacts](ConnectionDescPtr connection) -> bool {
                contacts.contacts.push_back(Contact(normalizeUid(connection->account_id), normalizeUid(connection->user_id)));
                return false;
            });

            contacts.windows = _get_call_windows(call->callKey);
            contacts.isActive = call->current() || call->outgoing;
        }
        
        SIGNAL_NOTIFICATOION(kNotificationType_CallPeerListChanged, &contacts);
    }

    void VoipManagerImpl::call_request_calls() {    

        bool found = false;
        walkThroughCalls([this, &found](VoipManagerImpl::CallDescPtr call) {
            found = true;
            _call_request_connections(call);
            return false;
        });
        if (!found)
        {
            _call_request_connections(nullptr);
        }
    }

    bool VoipManagerImpl::call_have_call(const Contact& contact) {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        
        for (auto it = _calls.begin(); it != _calls.end(); ++it) {
            for (auto it_conn = (*it)->connections.begin(); it_conn != (*it)->connections.end(); ++it_conn) {
                auto call = *it_conn;
                VOIP_ASSERT_ACTION(!!call, continue);

                if (normalizeUid(call->account_id) == contact.account && normalizeUid(call->user_id) == contact.contact) {
                    return true;
                }
            }
        }
        return false;       
    }

    bool VoipManagerImpl::call_have_established_connection() {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        for (auto it = _calls.begin(); it != _calls.end(); ++it) {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);

            if (call->current()) {
                return true;
            }
        }
        return false;       
    }

    bool VoipManagerImpl::_call_have_outgoing_connection() {

        bool res = false;
        walkThroughCalls([&res](CallDescPtr call) {
            res = call->outgoing;
            return res;
        });

        return res;

        //std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        //for (auto it = _calls.begin(); it != _calls.end(); ++it) {
        //    for (auto it_connection = (*it)->connections.begin(); it_connection != (*it)->connections.end(); ++it_connection) {
        //        auto call = *it_connection;
        //        VOIP_ASSERT_ACTION(!!call, continue);

        //        if (call->outgoing) {
        //            return true;
        //        }
        //    }
        //}
        //return false;       
    }

    VoipManagerImpl::WindowDesc* VoipManagerImpl::_find_window(void* handle) {
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
        for (unsigned ix = 0; ix < _windows.size(); ++ix) {
            WindowDesc& window_description = _windows[ix];
            if (window_description.handle == handle) {
                return &window_description;
            }
        }
        return NULL;
    }

    void VoipManagerImpl::window_add(voip_manager::WindowParams& windowParams) {
        VOIP_ASSERT_RETURN(!!windowParams.hwnd);

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        
        WindowDesc window_description;
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            if (NULL != _find_window(windowParams.hwnd)) {
                return;
            }

            window_description.handle           = windowParams.hwnd;
            window_description.aspectRatio      = 0.0f;
            window_description.scaleCoeffitient = windowParams.scale >= 0.01 ? windowParams.scale : 1.0f;
            window_description.layout_type      = voip2::LayoutType_One;            
            window_description.confLayout       = ConferenceAllTheSame;

            CallDescPtr callForWindow;
            if (windowParams.isPrimary && !windowParams.isSystem) 
            {
                auto currentCall = get_current_call();
                auto outgoingCall = get_outgoing_call();

                if (currentCall || outgoingCall)
                {
                    callForWindow = currentCall ? currentCall : outgoingCall;

                    window_description.talkKey = callForWindow->callKey;
                    callForWindow->hasWindow = true;
                }
            }
            else
            {
                callForWindow = _get_incomming_call_without_window();
                if (callForWindow)
                {
                    window_description.talkKey = callForWindow->callKey;
                    callForWindow->hasWindow = true;
                }
            }

            _windows.push_back(window_description);

            // Send contects for new window.
            if (callForWindow)
            {
                _call_request_connections(callForWindow);
            }
        }

        voip2::WindowSettings ws = {};
        if (windowParams.isPrimary) {
            getPriWindowSettings(ws, window_description.scaleCoeffitient);
        } else if (windowParams.isSystem) {
            getSysWindowSettings(ws, window_description.scaleCoeffitient);
        } else {
            getSecWindowSettings(ws, window_description.scaleCoeffitient);
        }

        void* cameraStatusHBMP = NULL;
        void* callingHBMP = NULL;
        if (windowParams.cameraStatus.bitmap.data && windowParams.cameraStatus.bitmap.size && windowParams.cameraStatus.bitmap.w && windowParams.cameraStatus.bitmap.h) {
            cameraStatusHBMP = bitmapHandleFromRGBData(windowParams.cameraStatus.bitmap.w, windowParams.cameraStatus.bitmap.h, sizeof(unsigned) * 8, windowParams.cameraStatus.bitmap.data, true);
            callingHBMP = bitmapHandleFromRGBData(windowParams.calling.bitmap.w, windowParams.calling.bitmap.h, sizeof(unsigned) * 8, windowParams.calling.bitmap.data, true);

            if (cameraStatusHBMP) {
                for (int i = voip2::WindowTheme_One; i <= voip2::WindowTheme_Two; i ++)
                {
                    ws.avatarMain[i].status.videoPaused = cameraStatusHBMP;
                }
                ws.avatarMain[voip2::WindowTheme_Two].status.calling = callingHBMP;
            }
        }

        void* hbmp = NULL;
        if (windowParams.watermark.bitmap.data && windowParams.watermark.bitmap.size && windowParams.watermark.bitmap.w && windowParams.watermark.bitmap.h) {
            hbmp = bitmapHandleFromRGBData(windowParams.watermark.bitmap.w, windowParams.watermark.bitmap.h, sizeof(unsigned) * 8, windowParams.watermark.bitmap.data, true);

            if (hbmp) {
                for (int i = voip2::WindowTheme_One; i <= voip2::WindowTheme_Two; i++)
                {
                    ws.avatarMain[i].logoImage = hbmp;
                    ws.avatarMain[i].logoOffsetL = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                    ws.avatarMain[i].logoOffsetT = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                    ws.avatarMain[i].logoOffsetR = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                    ws.avatarMain[i].logoOffsetB = kLogoFromBoundOffset * window_description.scaleCoeffitient;
                    ws.avatarMain[i].logoPosition = voip2::Position_Top | voip2::Position_Right;
                }
            }
        }

        auto objectForRelease = setupButtonsBitmaps(ws, windowParams);

		setupAvatarForeground();

        engine->WindowAdd(windowParams.hwnd, ws);
        const intptr_t winId = (intptr_t)windowParams.hwnd;
        SIGNAL_NOTIFICATOION(kNotificationType_VoipWindowAddComplete, &winId)

        if (windowParams.isSystem) {
			//engine->WindowSetAvatarPosition(windowParams.hwnd, voip2::Position_HCenter | voip2::Position_VCenter);
            _set_layout(windowParams.hwnd, voip2::LayoutType_Two);
        }
        else if (windowParams.isPrimary) {
            //engine->WindowSetAvatarPosition(windowParams.hwnd, voip2::Position_HCenter | voip2::Position_VCenter);
            auto currentCall = get_current_call();
            auto outgoingCall = get_outgoing_call();
            if (currentCall || outgoingCall)
            {
                _update_layout(currentCall ? currentCall : outgoingCall);
            }
        } else {
            _set_layout(windowParams.hwnd, voip2::LayoutType_Two);
        }

        if (hbmp) {
            bitmapReleaseHandle(hbmp);
        }
        if (cameraStatusHBMP) {
            bitmapReleaseHandle(cameraStatusHBMP);
        }
        if (callingHBMP)
        {
            bitmapReleaseHandle(callingHBMP);
        }
    }

    void VoipManagerImpl::window_remove(void* hwnd) {
        auto engine = _get_engine(true);
        if (!engine) {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            _windows.clear();
            return;
        }
        
		// Call WindowRemove async for Mac.
        _async_tasks->run_async_function([engine, hwnd]{
            engine->WindowRemove(hwnd);
            return 0;
            }
        )->on_result_ = [this, hwnd] (int32_t error)
        {
            const intptr_t winId = (intptr_t)hwnd;
            SIGNAL_NOTIFICATOION(kNotificationType_VoipWindowRemoveComplete, &winId)
            
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            WindowDesc* desc = _find_window(hwnd);
            if (NULL == desc) {
                return;
            }
            
            for (auto it = _windows.begin(); it != _windows.end(); ++it) {
                if (&*it == desc) {
                    _windows.erase(it);
                    return;
                }
            }
        };
    }

    void VoipManagerImpl::_window_set_bg(void* hwnd, void* hbmp) {
        VOIP_ASSERT_RETURN(hbmp);
        VOIP_ASSERT_RETURN(hwnd);

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowSetBackground(hwnd, hbmp);
    }

    void VoipManagerImpl::window_set_bitmap(const WindowBitmap& bmp) {
        VOIP_ASSERT_RETURN(bmp.hwnd);
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);

        void* hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, sizeof(unsigned) * 8, bmp.bitmap.data, true);
        VOIP_ASSERT_RETURN(hbmp);

        _window_set_bg(bmp.hwnd, hbmp);
        bitmapReleaseHandle(hbmp);
    }

    void VoipManagerImpl::_window_set_av(const std::string& contact, voip2::AvatarType type, void* hbmp) {
        VOIP_ASSERT_RETURN(hbmp);
        VOIP_ASSERT_RETURN(!contact.empty());

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        if (voip2::AvatarType_Camera == type || voip2::AvatarType_CameraCrossed == type || voip2::AvatarType_Foreground == type) {
            engine->WindowSetAvatar(PREVIEW_RENDER_NAME, hbmp, type);
        } else {
            if (contact != PREVIEW_RENDER_NAME)
            {
                if (voip2::AvatarType_Background != type)
                {
                    engine->WindowSetAvatar(contact.c_str(), hbmp, type, voip2::WindowTheme_One);
                }
                engine->WindowSetAvatar(contact.c_str(), hbmp, type, voip2::WindowTheme_Two);
            }
            else
            {
				engine->WindowSetAvatar(contact.c_str(), hbmp, type, voip2::WindowTheme_Two);
            }
        }
    }

    void VoipManagerImpl::window_set_bitmap(const UserBitmap& bmp) {
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        bool loadAvatar = (voip2::AvatarType_Camera == bmp.type) || (voip2::AvatarType_CameraCrossed == bmp.type) ||
            (voip2::AvatarType_UserMain == bmp.type) || (voip2::AvatarType_Foreground == bmp.type) ||
            (voip2::AvatarType_Background == bmp.type);

        loadAvatar = loadAvatar || get_current_call();

        if (loadAvatar) {
            void* hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, sizeof(unsigned) * 8, bmp.bitmap.data, true);
            VOIP_ASSERT_RETURN(hbmp);

            _window_set_av(bmp.contact, bmp.type, hbmp);
            bitmapReleaseHandle(hbmp);
        }
    }

    void VoipManagerImpl::window_set_conference_layout(void* hwnd, voip_manager::ConferenceLayout layout)
    {
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
        auto window_description = _find_window(hwnd);
        VOIP_ASSERT_RETURN(NULL != window_description);

        window_description->confLayout = layout;

        CallDescPtr desc;
        _call_get(window_description->talkKey, desc);

        VOIP_ASSERT_RETURN(desc);


        _update_layout(desc);
    }

    void VoipManagerImpl::window_switch_aspect(const std::string& contact, void* hwnd) {
        VOIP_ASSERT_RETURN(!contact.empty());

        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowSwitchAspectMode(hwnd, contact.c_str());
    }

    void VoipManagerImpl::window_set_offsets(void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        VoipManagerImpl::WindowDesc* winDesc = _find_window(hwnd);

        if (!!winDesc)
        {
            bool bOverlaped = voip2::LayoutType_One == winDesc->layout_type;

            engine->WindowSetControlsStatus(hwnd, true, l, t, r, b, true, bOverlaped);
        }
    }

    void VoipManagerImpl::window_add_button(voip2::ButtonType type, voip2::ButtonPosition position) {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        engine->WindowAddButton(type, position);
    }

	void VoipManagerImpl::window_set_primary(void* hwnd, const std::string& contact)
	{
		auto engine = _get_engine(true);
		VOIP_ASSERT_RETURN(!!engine);

		engine->WindowSetPrimary(hwnd, contact.c_str());
	}

    bool VoipManagerImpl::get_device_list(voip2::DeviceType device_type, std::vector<device_description>& dev_list) {
        dev_list.clear();

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        const std::string& activeDevice = voip2::AudioPlayback == device_type ? _voip_desc.aPlaybackDevice :
                                          voip2::AudioRecording == device_type ? _voip_desc.aCaptureDevice :
                                          _voip_desc.vCaptureDevice;

        const auto dev_count = engine->GetDevicesNumber(device_type);
        if (!dev_count) {
            return true;
        }

        dev_list.reserve(dev_count);
        char _device_name[voip2::MAX_DEVICE_NAME_LEN];
        char _device_guid[voip2::MAX_DEVICE_GUID_LEN];

        for (unsigned ix = 0; ix < dev_count; ix++) {
            if (!engine->GetDevice(device_type, ix, _device_name, _device_guid)) {
                continue;
            }
            
            device_description desc;
            desc.name = _device_name;
            desc.uid  = _device_guid;
            desc.type = device_type;
            desc.isActive = activeDevice == _device_guid;

            dev_list.push_back(desc);

            // We found at least one camera.
            if (voip2::VideoCapturing == device_type)
            {
                _hasCamera = true;
            }
        }

        assert((dev_count > 0 && !dev_list.empty()) || (!dev_count && dev_list.empty()));
        return true;
    }

    void VoipManagerImpl::_update_device_list(voip2::DeviceType dev_type) {
        device_list dev_list;
        dev_list.type = dev_type;
        VOIP_ASSERT_RETURN(get_device_list(dev_type, dev_list.devices));

        SIGNAL_NOTIFICATOION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::DeviceListChanged(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        if (!engine) {
            _update_device_list(deviceType);
            VOIP_ASSERT(false);
            return;
        }

        if (engine->GetDevicesNumber(deviceType) == 0) {
            _update_device_list(deviceType);
            return;
        }

        unsigned defaultDeviceIx = 0;
        if (voip2::AudioRecording == deviceType || voip2::AudioPlayback == deviceType) {
            defaultDeviceIx = (unsigned)-1;
        }

        if (voip2::VideoCapturing != deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            if (!_defaultDevicesStored) {
                return;
            }

            std::string& defaultDevice = voip2::AudioPlayback == deviceType ? _voip_desc.aPlaybackDefDevice :
                voip2::AudioRecording == deviceType ? _voip_desc.aCaptureDefDevice :
                _voip_desc.vCaptureDefDevice;

            char name[voip2::MAX_DEVICE_NAME_LEN]  = { 0 };
            char guid[voip2::MAX_DEVICE_GUID_LEN]  = { 0 };
            if (!!engine->GetDevice(deviceType, defaultDeviceIx, name, guid)) {
                if (defaultDevice != guid) {
                    set_device(deviceType, guid);
                    defaultDevice = guid;
                }
            }
        }

        device_list dev_list;
        dev_list.type = deviceType;
        VOIP_ASSERT_RETURN(get_device_list(deviceType, dev_list.devices));

        for (int ix = dev_list.devices.size() - 1; ix >= 0; --ix) {
            device_description& dd = dev_list.devices[ix];
            if (ix == 0 && !dd.isActive) {
                set_device(deviceType, dd.uid);
                dd.isActive = true;
            } else if (dd.isActive) {
                break;
            }
        }
        
        SIGNAL_NOTIFICATOION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::DeviceStatusChanged(voip2::DeviceType deviceType, const char *uid, voip2::DeviceStatus deviceStatus) {

		if (deviceType == voip2::VideoCapturing)
		{
			update_media_video_en(deviceStatus == voip2::DeviceStatus_Started || deviceStatus == voip2::DeviceStatus_Resumed);
		}
    }
    
    void VoipManagerImpl::AudioDeviceVolumeChanged(voip2::DeviceType deviceType, float volume) {
        if (voip2::AudioPlayback == deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.volume[kVolumePos_Pre] = _voip_desc.volume[1];
            _voip_desc.volume[kVolumePos_Cur] = volume;
        }

        DeviceVol vol;
        vol.type   = deviceType;
        vol.volume = volume;
        SIGNAL_NOTIFICATOION(kNotificationType_DeviceVolChanged, &vol);
    }
    
    void VoipManagerImpl::AudioDeviceMuteChanged(voip2::DeviceType deviceType, bool mute) {
        if (voip2::AudioPlayback == deviceType) {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.mute_en = mute;
        }

        DeviceMute dm;
        dm.mute = mute;
        dm.type = deviceType;
        SIGNAL_NOTIFICATOION(kNotificationType_DeviceMuted, &dm);
    }
    
    void VoipManagerImpl::FrameSizeChanged(voip::hwnd_t hwnd, float aspectRatio) {
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            auto window_description = _find_window(hwnd);
            VOIP_ASSERT_RETURN(NULL != window_description);
            window_description->aspectRatio = aspectRatio;
        }

        FrameSize fs = { 0 };
        fs.hwnd = (intptr_t)hwnd;
        fs.aspect_ratio = aspectRatio;

        SIGNAL_NOTIFICATOION(kNotificationType_FrameSizeChanged, &fs);
    }

    void VoipManagerImpl::LayoutTypeChanged(voip::hwnd_t hwnd, voip2::LayoutType layoutType) {
        {
            std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);
            auto window_description = _find_window(hwnd);
            VOIP_ASSERT_RETURN(NULL != window_description);
            window_description->layout_type = layoutType;
        }

        struct LayoutChanged lc = { 0 };
        lc.tray        = voip2::LayoutType_Three == layoutType || voip2::LayoutType_Four == layoutType;
        lc.layout_type = layoutType; 
        lc.hwnd = hwnd;

        SIGNAL_NOTIFICATOION(kNotificationType_LayoutChanged, &lc);
    }
    
    void VoipManagerImpl::RenderMouseTap(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip2::MouseTap mouseTap, voip2::ViewArea viewArea) {
        MouseTap mt;
        mt.area    = viewArea;
        mt.account = normalizeUid(account_uid);
        mt.contact = normalizeUid(user_id);
        mt.hwnd    = hwnd;
        mt.tap     = mouseTap;

        SIGNAL_NOTIFICATOION(kNotificationType_MouseTap, &mt);
    }

    void VoipManagerImpl::ButtonPressed(const char* account_uid, const char* user_id, voip::hwnd_t hwnd, voip::ButtonType type) {
        VOIP_ASSERT_RETURN(account_uid);
        VOIP_ASSERT_RETURN(user_id);

        ButtonTap bt;
        bt.account = normalizeUid(account_uid);
        bt.contact = normalizeUid(user_id);
        bt.hwnd    = hwnd;
        bt.type    = type;

        SIGNAL_NOTIFICATOION(kNotificationType_ButtonTap, &bt);
    }

    void VoipManagerImpl::MissedCall(const char* account_uid, const char* user_id, unsigned timestamp) {
        VOIP_ASSERT_RETURN(!!account_uid);
        VOIP_ASSERT_RETURN(!!user_id);

        struct MissedCall mc;
        mc.account = normalizeUid(account_uid);
        mc.contact = normalizeUid(user_id);
        mc.ts      = timestamp;

        SIGNAL_NOTIFICATOION(kNotificationType_MissedCall, &mc);
    }

    void VoipManagerImpl::SessionEvent(const char* _account_uid, const char* _user_id, voip2::SessionEvent sessionEvent) {
        VOIP_ASSERT_RETURN(!!_account_uid);
        VOIP_ASSERT_RETURN(!!_user_id);

        const std::string account_uid = _account_uid;
        const std::string user_id = _user_id;

        using namespace voip2;
        const voip2::SessionEvent startSessionEvents[] = {
            SE_OUTGOING_STARTED_AUDIO, SE_OUTGOING_STARTED_VIDEO,
            SE_INCOMING_INVITE_AUDIO, SE_INCOMING_INVITE_VIDEO
        };

        const voip2::SessionEvent joinedSessionEvents[] = {
            SE_JOINED_AUDIO,  SE_JOINED_VIDEO            
        };

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        CallDescPtr desc;
        eNotificationTypes notification = kNotificationType_Undefined;

        // Search sessionEvent in startSessionEvents.
        bool start_new_call_event = (std::end(startSessionEvents) != std::find(std::begin(startSessionEvents), std::end(startSessionEvents), sessionEvent));
        // Search sessionEvent in joinedSessionEvents.
        bool joined_call_event = (std::end(joinedSessionEvents) != std::find(std::begin(joinedSessionEvents), std::end(joinedSessionEvents), sessionEvent));

        if (start_new_call_event) {
            // We have conferentions and call already created.
            if (!_call_get(account_uid, user_id, desc))
            {
                VOIP_ASSERT_RETURN(!!_call_create(account_uid, user_id, desc));
            }
        }

        if (joined_call_event)
        {
            desc = get_current_call();
            VOIP_ASSERT_RETURN(desc);
        }

        if (!desc || desc->connections.empty()) {
            VOIP_ASSERT_RETURN(!!_call_get(account_uid, user_id, desc));
        }

        const VoipKey key = desc ? desc->callKey : 0;
        VOIP_ASSERT_RETURN(!!key);

        ContactEx contact_ex;
        contact_ex.contact.account = normalizeUid(account_uid);
        contact_ex.contact.contact = normalizeUid(user_id);
        contact_ex.connection_count = _call_get_connection_count(desc->callKey);
        contact_ex.call_count      = _calls.size();
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(key);

        auto notification_sender = [this, &contact_ex, &user_id] (eNotificationTypes nt) {
            if (kNotificationType_Undefined != nt) {
                SIGNAL_NOTIFICATOION(nt, &contact_ex);
            }
        };

        switch (sessionEvent) {
            case SE_OUTGOING_STARTED_AUDIO:
            case SE_OUTGOING_STARTED_VIDEO:
                {
                    std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);

                    ConnectionDescPtr connection;                    
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Initiated;
                    }
                    desc->outgoing = true;

                    contact_ex.incoming     = !desc->outgoing;
                    _voip_desc.local_cam_en = SE_OUTGOING_STARTED_VIDEO == sessionEvent;

					notification_sender(kNotificationType_CallCreated);
                    //_call_request_connections(desc);
					_update_peer_list(desc, account_uid, user_id);
                    _update_layout(desc);
                }
                break;

            case SE_OUTGOING_ACCEPTED_AUDIO:
            case SE_OUTGOING_ACCEPTED_VIDEO:
                {
                    ConnectionDescPtr connection;                    
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Accepted;
                        connection->remote_cam_en = SE_OUTGOING_ACCEPTED_VIDEO == sessionEvent;
                    }

                    //desc->outgoing      = false;
                    notification        = kNotificationType_CallOutAccepted;
                    _notify_remote_video_state_changed(connection);
                    _update_video_window_state(desc);

                    _load_avatars(account_uid, user_id);
					_update_layout(desc);
                }
                break;


            case SE_INCOMING_INVITE_AUDIO:
            case SE_INCOMING_INVITE_VIDEO:
                {
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Initiated;
                        connection->remote_cam_en = SE_INCOMING_INVITE_VIDEO == sessionEvent;
                    }

                    desc->outgoing      = false;
                    contact_ex.incoming = !desc->outgoing;

                    notification_sender(kNotificationType_CallCreated);
                    notification_sender(kNotificationType_CallInvite);
                    _notify_remote_video_state_changed(connection);
                    _update_peer_list(desc, account_uid, user_id);

					start_video();
                }
                break;

            case SE_INCOMING_ACCEPTED_AUDIO:
            case SE_INCOMING_ACCEPTED_VIDEO:
                {
                    std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);

                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Accepted;
                    }

                    _voip_desc.local_cam_en = SE_INCOMING_ACCEPTED_VIDEO == sessionEvent;
                    notification            = kNotificationType_CallInAccepted;
                    _update_video_window_state(desc);

                    _load_avatars(account_uid, user_id);
                }
                break;

            case SE_JOINED_AUDIO:
            case SE_JOINED_VIDEO:
                {
                    ConnectionDescPtr connection;

                    _connection_get(normalizeUid(account_uid), normalizeUid(user_id), connection);

                    if (!connection)
                    {
                        connection = _create_connection(normalizeUid(account_uid), normalizeUid(user_id));
                        if (connection)
                        {
                            std::lock_guard<std::recursive_mutex> __lock(_callsMx);
                            desc->connections.push_back(connection);
                        }
                    }

                    if (connection)
                    {
                        connection->call_state = kCallState_Connected;
                        connection->remote_cam_en = SE_JOINED_VIDEO == sessionEvent;

                        //desc->outgoing      = false;
                        contact_ex.incoming = !desc->outgoing;
                        // TODO: Check the time.
                        if (!desc->started) {
                            desc->started = (unsigned)clock();
                        }
                        //notification_sender(kNotificationType_CallCreated);
                        _notify_remote_video_state_changed(connection);
                        notification = kNotificationType_CallConnected;

                        _call_request_connections(desc);
                        _update_layout(desc);
                    }
                }
                break;

            case SE_QUALITY_BAD:
            case SE_QUALITY_AUDIO_OK:
            case SE_QUALITY_AUDIO_VIDEO_OK:
                {
                    desc->quality = SE_QUALITY_BAD == sessionEvent ? 30 : 
                                    SE_QUALITY_AUDIO_OK == sessionEvent ? 60 : 100;

                    notification_sender(kNotificationType_QualityChanged);
                }
                break;

            case SE_CONNECTED:
                {
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Connected;
                    }                    
                    if (!desc->started) {
                        desc->started   = (unsigned)clock();
                    }
                    notification        = kNotificationType_CallConnected;
                }
                break;

            case SE_DISCONNECTED:
                {
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->call_state = kCallState_Disconnected;
                    }

                    notification        = kNotificationType_CallDisconnected;
                }
                break;

            case SE_INCOMING_CONF_PEERS_UPDATED:
                {
                    _update_peer_list(desc, account_uid, user_id);
                    _update_video_window_state(desc);
                }
                break;

            case SE_REMOTE_MIC_ON:
            case SE_REMOTE_MIC_OFF:
                {
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->remote_mic_en = SE_REMOTE_MIC_ON == sessionEvent;
                    }
                    notification_sender(kNotificationType_MediaRemAudioChanged);
                }
                break;

            case SE_REMOTE_CAM_OFF:
            case SE_REMOTE_CAM_ON:
                {
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->remote_cam_en = SE_REMOTE_CAM_ON == sessionEvent;
                    }
                    _notify_remote_video_state_changed(connection);
                    _update_video_window_state(desc);
                }
                break;

            case SE_CIPHER_ENABLED:
                {
                    auto e = _get_engine(true);
                    VOIP_ASSERT(!!e);
                    if (!!e) {
                        char sas[MAX_SAS_LENGTH] = { 0 };

                        CipherState state;
                        state.state = CipherState::kCipherStateUnknown;
                        if (e->GetCipherSAS(account_uid.c_str(), user_id.c_str(), sas)) {
                            state.state      = CipherState::kCipherStateEnabled;
                            state.secureCode = sas;
                            SIGNAL_NOTIFICATOION(kNotificationType_CipherStateChanged, &state);
                        }
                    }
                }
                break;

            case SE_CIPHER_NOT_SUPPORTED_BY_PEER:
                {
                    CipherState state;
                    state.state = CipherState::kCipherStateNotSupportedByPeer;

                    SIGNAL_NOTIFICATOION(kNotificationType_CipherStateChanged, &state);
                }
                break;

            case SE_CIPHER_FAILED:
                {
                    CipherState state;
                    state.state = CipherState::kCipherStateFailed;

                    SIGNAL_NOTIFICATOION(kNotificationType_CipherStateChanged, &state);
                }
                break;

            case SE_CLOSED_BY_REMOTE_DECLINE:
            case SE_CLOSED_BY_REMOTE_HANDLED_BY_ANOTHER_INSTANCE:
            case SE_CLOSED_BY_REMOTE_BUSY:
            case SE_CLOSED_BY_REMOTE_ERROR:
            case SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_REMOTE:
            case SE_CLOSED_BY_TIMEOUT_NO_ACCEPT_FROM_LOCAL:
            case SE_CLOSED_BY_TIMEOUT_INACTIVE:
            case SE_CLOSED_BY_TIMEOUT_CONNECT_INIT:
            case SE_CLOSED_BY_TIMEOUT_CONNECTION:
            case SE_CLOSED_BY_TIMEOUT_RECONNECT:
            case SE_CLOSED_BY_ERROR_CREATE:
            case SE_CLOSED_BY_ERROR_START:
            case SE_CLOSED_BY_ERROR_INTERNAL:
            case SE_CLOSED_BY_LOCAL_BUSY:
            case SE_CLOSED_BY_LOCAL_HANGUP:
                {                    
                    ConnectionDescPtr connection;
                    if (_connection_get(account_uid, user_id, connection))
                    {
                        connection->close_reason = sessionEvent;
                    }

                    // For incomming conferention, we can have sever not inited connections.
                    _connection_clear_not_inited(desc);

                    {
                        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
                        notification = (desc->connections.size() <= 1) ? kNotificationType_CallDestroyed : kNotificationType_ConnectionDestroyed;
                    }
                }
                break;

            default:
                break;
        }

        if (start_new_call_event) {
            _update_layout(desc);
            _update_video_window_state(desc);

            if (_call_get_connection_count(desc->callKey) == 1) {
				DeviceVol vol;
				DeviceMute dm;
				{
					std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
					auto engine = _get_engine();
					if (!!engine)
					{
						_voip_desc.mute_en = engine->GetDeviceMute(voip2::AudioPlayback);
						_voip_desc.volume[kVolumePos_Pre] = _voip_desc.volume[1];
						_voip_desc.volume[kVolumePos_Cur] = engine->GetDeviceVolume(voip2::AudioPlayback);
					}

					vol.type   = voip2::AudioPlayback;
					vol.volume = _voip_desc.volume[kVolumePos_Cur];

					dm.mute = _voip_desc.mute_en;
					dm.type = voip2::AudioPlayback;

                    // turn on microfon on call start.
                    media_audio_en(true);

					SIGNAL_NOTIFICATOION(kNotificationType_DeviceMuted, &dm);
					SIGNAL_NOTIFICATOION(kNotificationType_DeviceVolChanged, &vol);
				}
            }

            const bool audio_enabled = local_audio_enabled();
            const bool video_enabled = local_video_enabled();
            SIGNAL_NOTIFICATOION(kNotificationType_MediaLocAudioChanged, &audio_enabled);
            SIGNAL_NOTIFICATOION(kNotificationType_MediaLocVideoChanged, &video_enabled);
        }


        // TODO implement support.
        if (notification != kNotificationType_ConnectionDestroyed)
        {
            notification_sender(notification);
        }

        if (SE_CLOSED_BY_REMOTE_DECLINE <= sessionEvent) {
            std::lock_guard<std::recursive_mutex> __lock(_callsMx);

            _connection_destroy(account_uid, user_id);
            if (desc->connections.empty())
            {
                _call_destroy(key);
            }
            _update_layout(desc);
            desc = NULL;
            _update_video_window_state(desc);
            call_request_calls();
        }

        _update_call_states(desc);

        _dispatcher.execute_core_context([this] () {
            std::lock_guard<std::recursive_mutex> __lockCalls(_callsMx);
            if (_calls.empty() && sendViaIm_) {
                sendViaIm_ = false;
                if (callback_ != NULL) {
                    callback_();
                }
                callback_ = std::function<void()>();
            }
        });
    }

    void VoipManagerImpl::mute_incoming_call_sounds(bool mute) {
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.incomingSoundsMuted = mute;
        }

        auto engine = _get_engine(true);
        if (!!engine) {
            engine->MuteAllIncomingSoundNotifications(mute);
        }
    }

	void VoipManagerImpl::minimal_bandwidth_switch()
	{
		bool mode = false;
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.minimalBandwidth = !_voip_desc.minimalBandwidth;
			mode = _voip_desc.minimalBandwidth;
        }

		auto engine = _get_engine(true);
        if (!!engine) {
            engine->EnableMinimalBandwithMode(mode);
        }
	}

    bool VoipManagerImpl::has_created_call()
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (auto it = _calls.begin(); it != _calls.end(); ++it) {
            for (auto itconnection = (*it)->connections.begin(); itconnection != (*it)->connections.end(); ++itconnection) {
                auto call = *it;
                VOIP_ASSERT_ACTION(!!call, continue);

                if (call->callKey >= kCallState_Created) {
                    return true;
                }
            }
        }
        return false;
    }

    void VoipManagerImpl::_protocolSendAlloc(const char* data, unsigned size) {
        core::core_dispatcher& dispatcher = _dispatcher;
        std::string dataBuf(data, size);

        _dispatcher.execute_core_context([dataBuf, &dispatcher, this] () {
            if (!_voipProtocol) {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);

            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);
            
            auto packet = im->prepare_voip_msg(dataBuf);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, this] (int32_t _error) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                auto response = packet->getRawData();
                if (!!response && response->available()) {
                    uint32_t responseSize = response->available();
                    ptrIm->on_voip_proto_msg(true, (const char*)response->read(responseSize), responseSize, std::make_shared<core::auto_callback>([](int32_t){}));
                }
            };
        });
    }

    void VoipManagerImpl::_protocolSendMessage(const VoipProtoMsg& data) {
        core::core_dispatcher& dispatcher = _dispatcher;
        _dispatcher.execute_core_context([data, &dispatcher, this] () {
            if (!_voipProtocol) {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);

            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);

            auto packet = im->prepare_voip_pac(data);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, data, this] (int32_t _error) {
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);

                auto response = packet->getRawData();
                bool success  = _error == 0 && !!response && response->available();
                ptrIm->on_voip_proto_ack(data, success);
            };
        });
    }

    void VoipManagerImpl::SendVoipMsg(const char* from, voip2::VoipOutgoingMsg voipOutgoingMsg, const char *data, unsigned len, unsigned msg_idx) {
        VOIP_ASSERT_RETURN(data && len);
        VOIP_ASSERT_RETURN(from);

        using namespace voip2;
        switch(voipOutgoingMsg) {
        case WIM_Outgoing_allocate:
            if (sendViaIm_) {
                _dispatcher.post_voip_alloc(0, data, len);
            } else {
                _protocolSendAlloc(data, len);
            }
            break;

        case WIM_Outgoing_invite:
        case WIM_Outgoing_accept:
        case WIM_Outgoing_decline:
        case WIM_Outgoing_json:
        case WIM_Outgoing_keepalive:
            {
                VoipProtoMsg msg;
                msg.msg       = voipOutgoingMsg;
                msg.request.assign(data, len);
                msg.messageId = msg_idx;

                using namespace std::chrono;
                const milliseconds msNow = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
                for (auto it = _requestIds.begin(); it != _requestIds.end();) {
                    RequestInfo& requestInfo = it->second;
                    if (msNow.count() - requestInfo.ts.count() >= 3 * 60 * 1000) { // since last sequense number reuse elapsed 3 min... like in voip
                        it = _requestIds.erase(it);
                    } else {
                        ++it;
                    }
                }

                const char delimiter = '_';
                if (msg_idx) {
                    auto it = _requestIds.find(msg_idx);
                    if (it == _requestIds.end()) {
                        std::stringstream reqIdStr;
                        reqIdStr << msNow.count() << delimiter << rand();
                        msg.requestId = reqIdStr.str();

                        RequestInfo requestInfo;
                        requestInfo.requestId = msg.requestId;
                        requestInfo.messageId = msg_idx;
                        requestInfo.ts        = msNow;

                        _requestIds[msg_idx] = requestInfo;
                    } else {
                        RequestInfo& requestInfo = it->second;
                        requestInfo.ts = msNow;
                        msg.requestId  = requestInfo.requestId;
                    }
                } else {
                    std::stringstream reqIdStr;
                    reqIdStr << msNow.count() << delimiter << rand();
                    msg.requestId = reqIdStr.str();
                }

                VOIP_ASSERT(!msg.requestId.empty());
                if (kUseVoipProtocolAsDefault) {
                    if (sendViaIm_) {
                        _dispatcher.post_voip_message(0, msg);
                    } else {
                        _protocolSendMessage(msg);
                    }
                } else {
                    _dispatcher.post_voip_message(0, msg);
                }
            }
            break;

        default:
            VOIP_ASSERT(false);
            break;
        }
    }

    void VoipManagerImpl::ProcessVoipAck(const std::string& account_uid, const voip_manager::VoipProtoMsg& msg, bool success) {
        VOIP_ASSERT_RETURN(!account_uid.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->ReadVoipAck(account_uid.c_str(), msg.messageId, success);
    }

    void VoipManagerImpl::ProcessVoipMsg(const std::string& account_uid, voip2::VoipIncomingMsg voipIncomingMsg, const char *data, unsigned len) {
        VOIP_ASSERT_RETURN(!account_uid.empty());

        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->ReadVoipMsg(account_uid.c_str(), voipIncomingMsg, data, len);
    }

    void VoipManagerImpl::_load_avatars(const std::string& account_uid, const std::string& user_uid, bool request_if_not_exists/* = true*/) {   
        _dispatcher.execute_core_context([account_uid, user_uid, this] () {
            auto im = _dispatcher.find_im_by_id(0);
            assert(!!im);

            im->get_contact_avatar(0, user_uid, kAvatarRequestSize, false);
        });
    }

    unsigned VoipManagerImpl::get_devices_number(voip2::DeviceType device_type) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, 0);

        return engine->GetDevicesNumber(device_type);
    }

    bool VoipManagerImpl::get_device(voip2::DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        char _device_name[voip2::MAX_DEVICE_NAME_LEN];
        char _device_guid[voip2::MAX_DEVICE_GUID_LEN];
        if (engine->GetDevice(device_type, index, _device_name, _device_guid)) {
            device_name = _device_name;
            device_guid = _device_guid;
            return true;
        }
        return false;
    }

    void VoipManagerImpl::AudioDeviceSpeakerphoneChanged(bool speakerphoneOn) {
        
    }

    void VoipManagerImpl::InterruptByGsmCall(bool gsmCallStarted) {
        
    }

    void VoipManagerImpl::_update_video_window_state(CallDescPtr call) {
        unsigned call_count = 0;

        {
            std::lock_guard<std::recursive_mutex> __lock(_callsMx);
            call_count = call ? _call_get_connection_count(call->callKey) : (_calls.size() > 0 ? 1 : 0);
        }
        
        const bool have_established_connection = call_have_established_connection();
        const bool have_outgoing               = _call_have_outgoing_connection();
        const bool have_video                  = true;//local_video_enabled() || remote_video_enabled(); // not remove video window on off camera both

        // TODO: Test bug with unneeded video window.
        const bool enable_video = /*call_count > 1 ||*/ (have_video && call_count && (have_established_connection || have_outgoing));
        SIGNAL_NOTIFICATOION(kNotificationType_ShowVideoWindow, &enable_video);
    }

    void VoipManagerImpl::set_device(voip2::DeviceType device_type, const std::string& device_guid) {
        VOIP_ASSERT_RETURN(voip2::MAX_DEVICE_GUID_LEN >= device_guid.length());
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            switch(device_type) {
            case voip2::AudioRecording: { _voip_desc.aCaptureDevice  = device_guid; } break;
            case voip2::AudioPlayback:  { _voip_desc.aPlaybackDevice = device_guid; } break;
            case voip2::VideoCapturing: { 
                _hasCamera = true;
                _voip_desc.vCaptureDevice = device_guid;
            } break;
            default: { VOIP_ASSERT(false); return; }
            }
        }

        auto engine = _engine;
        if (!!engine) {
            engine->SetDevice(device_type, device_guid.c_str());
        }

    }

    void VoipManagerImpl::set_device_mute(voip2::DeviceType deviceType, bool mute) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        VOIP_ASSERT_RETURN(voip2::AudioPlayback == deviceType);

        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        if (mute) {
            engine->SetDeviceVolume(deviceType, 0.0f);
        } else {
            float volume = _voip_desc.volume[kVolumePos_Pre] <= 0.0001f ? 0.5f : _voip_desc.volume[kVolumePos_Pre];
            engine->SetDeviceVolume(deviceType, volume);
        }
    }

    bool VoipManagerImpl::get_device_mute(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);

        VOIP_ASSERT_RETURN_VAL(voip2::AudioPlayback == deviceType, false);
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.volume[kVolumePos_Cur] <= 0.0001f;
    }

    void VoipManagerImpl::set_device_volume(voip2::DeviceType deviceType, float volume) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        engine->SetDeviceVolume(deviceType, volume);
    }

    void VoipManagerImpl::update() {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
    }

    float VoipManagerImpl::get_device_volume(voip2::DeviceType deviceType) {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, 0.0f);

        return engine->GetDeviceVolume(deviceType);
    }

    void VoipManagerImpl::reset() {
        _voipDestroyed = true;
        
        // Made reset voip async to fix quit problem during call.
        _async_tasks->run_async_function([=] 
        {
            // Use this temp shared ptr to wait, while other threads finish to use engine.
            // It fixed crash, when we try to destry voip from voip signal thread.
            std::shared_ptr<voip2::Voip2> tempVoipPtr = _engine;
            _engine.reset();

            // Wait until other threads release engine.
            while (tempVoipPtr && !tempVoipPtr.unique())
            {
                std::this_thread::yield();
            }

            return 0;
        }
        )->on_result_ = [=](int32_t error) 
        {
            SIGNAL_NOTIFICATOION(kNotificationType_VoipResetComplete, &_voipDestroyed);
        };
    }

	void VoipManagerImpl::check_mute_compatibility()
	{
#ifdef _WIN32
		// In prev version in mute we set SetDeviceMute to true, but
		// now we change volume of sounde to 0, to make mute. 
		// This code make mute comportable with prev versions.
		if (!_voipMuteFix)
		{
			auto engine = _get_engine();
			VOIP_ASSERT_RETURN(!!engine);

			_dispatcher.execute_core_context([this, engine] () 
			{
				_voipMuteFix = _dispatcher.get_voip_mute_fix_flag();
				if (!_voipMuteFix)
				{
					// Get prev mute value.
					bool bPrevMute = engine->GetDeviceMute(voip2::AudioPlayback);

					if (bPrevMute)
					{
						// Set mute to false and use current mute logic.
						engine->SetDeviceMute(voip2::AudioPlayback, false);
						set_device_mute(voip2::AudioPlayback, true);
					}

					// Ok. We made mute fix. Save it in settings.
					_dispatcher.set_voip_mute_fix_flag(true);
				}
			});
		}
#endif
	}

	void VoipManagerImpl::load_mask(const std::string& path)
	{
		loadMask(path);
	}

	unsigned int VoipManagerImpl::version()
	{
		auto engine = _get_engine();
		VOIP_ASSERT_RETURN_VAL(!!engine, 0);
        unsigned int res = 0;
#ifndef STRIP_VOIP
        res = engine->GetMaskEngineVersion();
#endif //STRIP_VOIP
		return res;
	}

    void VoipManagerImpl::set_model_path(const std::string& path)
    {
        // In case, when we already have model, but we need to reload old and load new.
        _masksEngineInited = false;
        _maskModelPath = path;
        if (_needToRunMask)
        {
            initMaskEngine(_maskModelPath);
        }
    }

	void VoipManagerImpl::setupAvatarForeground()
	{
		unsigned char foreground[] = { 0, 0, 0, 255 / 3 };
		voip_manager::UserBitmap bmp;
		bmp.bitmap.data = (void*)foreground;
		bmp.bitmap.size = 4;
		bmp.bitmap.w = 1;
		bmp.bitmap.h = 1;
		bmp.contact = PREVIEW_RENDER_NAME;
		bmp.type = voip2::AvatarType_Foreground;

		window_set_bitmap(bmp);
	}

	void VoipManagerImpl::initMaskEngine(const std::string& modelDir)
	{
		auto engine = _get_engine();
		VOIP_ASSERT_RETURN(!!engine);

		if (!_masksEngineInited)
		{
			_async_tasks->run_async_function([engine, modelDir] {
				engine->InitMaskEngine(modelDir.c_str());
				return 0;
			}
			)->on_result_ = [](int32_t error) {};
		}
	}

	void VoipManagerImpl::MaskModelInitStatusChanged(bool res)
	{
		_masksEngineInited = res;
		
		EnableParams maskEnable;
		maskEnable.enable = _masksEngineInited;

		SIGNAL_NOTIFICATOION(kNotificationType_MaskEngineEnable, &maskEnable);
	}

	void VoipManagerImpl::loadMask(const std::string& maskPath)
	{
		auto engine = _get_engine();
		VOIP_ASSERT_RETURN(!!engine);

		NamedResult loadMaskRes;
		loadMaskRes.name = maskPath;
		loadMaskRes.result = false;

		if (_masksEngineInited)
		{
			engine->LoadMask(maskPath.c_str());
		}

		SIGNAL_NOTIFICATOION(kNotificationType_LoadMask, &loadMaskRes);
	}

    void VoipManagerImpl::init_mask_engine()
    {
        _needToRunMask = true;
        if (!_maskModelPath.empty())
        {
            initMaskEngine(_maskModelPath);
        }
    }

    void VoipManagerImpl::walkThroughCurrentCall(std::function<bool(VoipManagerImpl::ConnectionDescPtr)> func)
    {
        auto currentCall = get_current_call();

        VOIP_ASSERT_ACTION(!!currentCall, return);

        if (currentCall)
        {
            walkThroughCallConnections(currentCall, func);
        }
    }

    void VoipManagerImpl::walkThroughAllConnections(std::function<bool(VoipManagerImpl::ConnectionDescPtr)> func)
    {
        walkThroughCalls([func, this](CallDescPtr call) {
            return walkThroughCallConnections(call, func);
        });
    }

    void VoipManagerImpl::walkThroughCalls(std::function<bool(VoipManagerImpl::CallDescPtr)> func)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        for (auto it = _calls.begin(); it != _calls.end(); ++it) {
            CallDescPtr callDesc = *it;

            VOIP_ASSERT_ACTION(!!callDesc, continue);

            if (func(callDesc)) {
                break;
            }
        }
    }

    bool VoipManagerImpl::walkThroughCallConnections(VoipManagerImpl::CallDescPtr call, std::function<bool(VoipManagerImpl::ConnectionDescPtr)> func)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        for (auto it = call->connections.begin(); it != call->connections.end(); ++it) {
            ConnectionDescPtr connectionDesc = *it;

            VOIP_ASSERT_ACTION(!!connectionDesc, continue);

            if (func(connectionDesc)) {
                return true;
            }
        }

        return false;
    }

    bool VoipManagerImpl::_connection_get(const std::string& account_uid, const std::string& user_uid, ConnectionDescPtr& connectionDesc)
    {
        bool res = false;
        walkThroughAllConnections([&res, &account_uid, &user_uid, &connectionDesc](VoipManagerImpl::ConnectionDescPtr connection) -> bool {
            res = connection->account_id == account_uid && connection->user_id == user_uid;
            if (res)
            {
                connectionDesc = connection;
            }
            return res;
        });

        return res;
    }

    void VoipManagerImpl::_cleanupCalls()
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        _calls.erase(
            std::remove_if(_calls.begin(), _calls.end(), [](CallDescPtr call) 
                {                    
                    return call->connections.empty();         
                }
            ), _calls.end());
    }

    VoipManagerImpl::CallDescPtr VoipManagerImpl::get_current_call()
    {
        CallDescPtr res;
        walkThroughCalls([this, &res](CallDescPtr call)
        {
            bool found = walkThroughCallConnections(call, [](ConnectionDescPtr connection) {
                return (connection->call_state >= kCallState_Accepted);
            });
            if (found)
            {
                res = call;
            }

            return found;
        });

        return res;
    }

    VoipManagerImpl::CallDescPtr VoipManagerImpl::get_outgoing_call()
    {
        CallDescPtr res;
        walkThroughCalls([this, &res](CallDescPtr call)
        {
            bool found = call->outgoing;
            if (found)
            {
                res = call;
            }

            return found;
        });

        return res;
    }

    std::vector<void*> VoipManagerImpl::_get_call_windows(VoipKey callKey)
    {
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);

        std::vector<void*> res;

        std::for_each(_windows.begin(), _windows.end(), [&res, callKey](WindowDesc& window)
        {
            if (callKey == window.talkKey)
            {
                res.push_back(window.handle);
            }
        });
    
        return res;
    }

    VoipManagerImpl::CallDescPtr VoipManagerImpl::_get_incomming_call_without_window()
    {
        CallDescPtr res;
        walkThroughCalls([this, &res](CallDescPtr call)
        {
            bool found = false;
            if (!call->outgoing)
            {
                found = walkThroughCallConnections(call, [](ConnectionDescPtr connection) {
                    return (connection->call_state >= kCallState_Accepted);
                });

                if (!found)
                {
                    res = call;
                }                
            }

            return found;
        });

        return res;
    }

    VoipManagerImpl::ConnectionDescPtr VoipManagerImpl::_create_connection(const std::string& account_id, const std::string& user_id)
    {
        const VoipKey key = getHash(account_id, user_id);
        VOIP_ASSERT_RETURN_VAL(!!key, nullptr);

        ConnectionDescPtr temp;
        VOIP_ASSERT_RETURN_VAL(!_connection_get(account_id, user_id, temp), nullptr);

        ConnectionDescPtr res = ConnectionDescPtr(new(std::nothrow) ConnectionDesc());

        res->account_id = account_id;
        res->user_id = user_id;
        res->remote_cam_en = false;
        res->remote_mic_en = false;

        res->call_state = kCallState_Created;
        res->connectionKey = key;
        res->close_reason = voip2::SE_OPEN_FIRST;

        return res;
    }

    bool VoipManagerImpl::_connection_destroy(const std::string& account_uid, const std::string& user_uid)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        CallDescPtr desc;
        _call_get(account_uid, user_uid, desc);

        VOIP_ASSERT_RETURN_VAL(!!desc, false)

        desc->connections.erase(std::remove_if(desc->connections.begin(), desc->connections.end(), 
            [&account_uid, &user_uid](ConnectionDescPtr connections)
            {
                return connections->account_id == account_uid && connections->user_id == user_uid;
            }
            ), desc->connections.end());

        return true;
    }

    void VoipManagerImpl::_cleanup_windows_for_call(VoipKey callKey)
    {
        std::lock_guard<std::recursive_mutex> __wlock(_windowsMx);

        std::for_each(_windows.begin(), _windows.end(), [callKey](WindowDesc& window)
        {
            if (callKey == window.talkKey)
            {
                window.talkKey = 0;
            }
        });
    }

    void VoipManagerImpl::_connection_clear_not_inited(CallDescPtr call)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);

        VOIP_ASSERT_RETURN(!!call)

        call->connections.erase(std::remove_if(call->connections.begin(), call->connections.end(),
                [](ConnectionDescPtr connections)
        {
            return connections->call_state <= kCallState_Created;
        }
        ), call->connections.end());
    }

    VoipManagerImpl::HBMPVectorPtr VoipManagerImpl::setupButtonsBitmaps(voip2::WindowSettings& ws, voip_manager::WindowParams& windowParams)
    {
        // Object wich automaticle release bmp in destructor.
        HBMPVectorPtr res =
            HBMPVectorPtr(new std::vector<voip::hbmp_t>(), 
                [](std::vector<voip::hbmp_t>* vector) 
                { 
                    for (auto hbmp : *vector)
                    {
                        bitmapReleaseHandle(hbmp);
                    }
                    delete vector;
                });

        BitmapDescription* bmpDesc[] = {&windowParams.normalButton, &windowParams.highlightedButton,
            &windowParams.pressedButton, &windowParams.disabledButton};

        voip::hbmp_t* hbmps[] = { &ws.buttonContext.normal, &ws.buttonContext.highlighted,
            &ws.buttonContext.pressed, &ws.buttonContext.disabled};

        for (int i = 0; i < sizeof (bmpDesc) / sizeof(bmpDesc[0]); i++)
        {
            if (bmpDesc[i]->data != nullptr)
            {
                *hbmps[i] = bitmapHandleFromRGBData(bmpDesc[i]->w,
                    bmpDesc[i]->h, sizeof(unsigned) * 8, bmpDesc[i]->data, true);

                res->push_back(*hbmps[i]);
            }
        }

        return res;
    }

    void VoipManagerImpl::_update_call_states(CallDescPtr call)
    {
        // Disable minimalBandwidth for conference.    
        bool minimalBandwidth = false;
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            minimalBandwidth = _voip_desc.minimalBandwidth;
        }
        auto engine = _get_engine(true);
        if (!!engine && minimalBandwidth) {
            engine->EnableMinimalBandwithMode(false);
        }
    }


    VoipManager::VoipManager(core::core_dispatcher& dispatcher) {
        _impl.reset(new(std::nothrow) VoipManagerImpl(dispatcher));
    }

    std::shared_ptr<ICallManager> VoipManager::get_call_manager() {
        return _impl;
    }

    std::shared_ptr<IWindowManager> VoipManager::get_window_manager() {
        return _impl;
    }

    std::shared_ptr<IMediaManager> VoipManager::get_media_manager() {
        return _impl;
    }

    std::shared_ptr<IDeviceManager> VoipManager::get_device_manager() {
        return _impl;
    }

    std::shared_ptr<IConnectionManager> VoipManager::get_connection_manager() {
        return _impl;
    }

    std::shared_ptr<IVoipManager> VoipManager::get_voip_manager() {
        return _impl;
    }

	std::shared_ptr<IMaskManager> VoipManager::get_mask_manager()
	{
		return _impl;
	}
}
