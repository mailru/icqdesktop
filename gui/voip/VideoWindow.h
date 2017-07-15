#ifndef __VIDEO_WINDOW_H__
#define __VIDEO_WINDOW_H__

#include "VideoFrame.h"
#include "DetachedVideoWnd.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#endif

namespace voip_manager
{
    struct CipherState;
    struct FrameSize;
}

namespace Ui
{
	class DetachedVideoWindow;
	class ShadowWindow;
    class VoipSysPanelHeader;
    class VideoPanel;
    class MaskPanel;
    class SecureCallWnd;

    class VideoWindow : public AspectRatioResizebleWnd
    {
        Q_OBJECT
        
    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void resizeEvent(QResizeEvent* _e) override;
        void closeEvent(QCloseEvent *) override;
        void paintEvent(QPaintEvent *_e) override;
        void changeEvent(QEvent *) override;
        void escPressed() override;

        void executeCommandList();

        
#ifndef _WIN32
#ifndef __APPLE__
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
#endif
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent * event) override;
        void mousePressEvent(QMouseEvent * event) override;
        void wheelEvent(QWheelEvent * event) override;
        void moveEvent(QMoveEvent * event) override;
        
        // @return true if we resend message to any transparent panel
        template <typename E> bool resendMouseEventToPanel(E* event_);
#endif

    private:
        quintptr getContentWinId() override;
        void updatePanels() const;
        void _switchFullscreen();
        void checkPanelsVisibility(bool _forceHide = false);
        inline void updateWindowTitle();
        bool isOverlapped() const;

        // @return true, if we were able to load size from settings
        bool getWindowSizeFromSettings(int& _nWidth, int& _nHeight);
        
        void offsetWindow(int _bottom, int _top);
        void updateUserName();

		// We use this proxy to catch call methods during fullscreen animation
		// we will save commands and call it later, after fullscreen animation
		void callMethodProxy(const QString& _method);

		void showPanel(QWidget* widget);
		void hidePanel(QWidget* widget);

        void fadeInPanels(int kAnimationDefDuration);
		void fadeOutPanels(int kAnimationDefDuration);
		void hidePanels();

        // call this method in all cases, when you need to hide panels
        void tryRunPanelsHideTimer();
        void hideSecurityDialog();

        bool hasRemoteVideoInCall(); // @return true for calls (not for conference), if remote camera is turned on.
        bool hasRemoteVideoForConference(); // @return true for conference, if any remote camera is turned on.

        void removeUnneededRemoteVideo(); // Remove unneeded elements from hasRemoteVideo_
        void checkCurrentAspectState();

		void updateOutgoingState(const voip_manager::ContactEx& _contactEx);

    private Q_SLOTS:
        void checkOverlap();
        void checkPanelsVis();
        void onVoipMouseTapped(quintptr, const std::string& _tapType);
        void onVoipCallNameChanged(const voip_manager::ContactsList&);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);
        void onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onVoipChangeWindowLayout(intptr_t hwnd, bool bTray, const std::string& layout);
		void onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout& mainLayout);

        void onEscPressed();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onPanelMouseEnter();
        void onPanelMouseLeave();
        void onPanelFullscreenClicked();

        void onVoipUpdateCipherState(const voip_manager::CipherState& _state);
        void onSecureCallClicked(const QRect&);
        void onSecureCallWndOpened();
        void onSecureCallWndClosed();
        
        // These methods are used under macos to close video window correctly from the fullscreen mode
        void fullscreenAnimationStart();
        void fullscreenAnimationFinish();
        void activeSpaceDidChange();
        
        void showVideoPanel();
        void autoHideToolTip(bool& autoHide);
        void companionName(QString& name);
        void showToolTip(bool &show);

		void onSetPreviewPrimary();
		void onSetContactPrimary();

        void onShowMaskList();
        void onHideMaskList();

        void getCallStatus(bool& isAccepted);

        void onMaskListAnimationFinished(bool out);

        void updateConferenceMode(voip_manager::ConferenceLayout layout);
        void onAddUserClicked();

    public:
        VideoWindow();
        ~VideoWindow();

        void hideFrame();
        void showFrame();
        bool isActiveWindow();

    private:
        FrameControl_t* rootWidget_;

        // Video window panels list
		std::vector<BaseVideoPanel*> panels_;
        
#ifndef _WIN32
        // Transparent widgets. We resend mouse events to these widgets under macos,
        // because we did not find any better way to catch mouse events for tranparent panels
		std::vector<BaseVideoPanel*> transparentPanels_;
#endif
        
		std::unique_ptr<VideoPanelHeader>   topPanelSimple_;
		std::unique_ptr<VoipSysPanelHeader> topPanelOutgoing_;
        std::unique_ptr<VideoPanel>          videoPanel_;
		std::unique_ptr<MaskPanel>           maskPanel_;
        std::unique_ptr<TransparentPanel>   transparentPanelOutgoingWidget_;

        std::unique_ptr<DetachedVideoWindow> detachedWnd_;
        ResizeEventFilter*     eventFilter_;

        QTimer checkOverlappedTimer_;
        QTimer showPanelTimer_;

        // Remote video by users.
        QHash <QString, bool> hasRemoteVideo_;
        bool outgoingNotAccepted_;

        // Current contacts
        std::vector<voip_manager::Contact> currentContacts_;

        //UIEffects* video_panel_header_effect_;
        //UIEffects* video_panel_header_effect_with_avatars_;

        QWidget *widget_;

        SecureCallWnd* secureCallWnd_;

        struct
        {
            std::string name;
            unsigned time;
        } callDescription;
        
		ShadowWindowParent shadow_;

        // Last applied offsets.
        int lastBottomOffset_;
        int lastTopOffset_;

        // All is connected and talking now.
        bool startTalking;
        
        // Load size from settings only one time per call. Actual for Mac.
        bool isLoadSizeFromSettings_;

#ifdef __APPLE__
        // We use fullscreen notification to fix a problem on macos
        platform_macos::FullScreenNotificaton _fullscreenNotification;
        // Commands after fullscreen mode
        QList<QString> commandList_;
        bool isFullscreenAnimation_;
        bool changeSpaceAnimation_;
        
        // We have own double click processing,
        // Because Qt has bug with double click and fullscreen
        QTime doubleClickTime_;
        QPoint doubleClickMousePos_;
#endif
    };
}

#endif//__VIDEO_WINDOW_H__
