#ifndef __VIDEO_PANEL_H__
#define __VIDEO_PANEL_H__

#include "CommonUI.h"

extern const QString vertSoundBg;
extern const QString horSoundBg;

namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Ui
{

    class ToolTipEx;
    struct UIEffects;
    
    bool onUnderMouse(QWidget& _widg);

    class VideoPanel : public BaseBottomVideoPanel
    {
        Q_OBJECT

        Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onFullscreenClicked();
        void onkeyEscPressed();
        void showPanel();
        void autoHideToolTip(bool& _autoHide);
        void companionName(QString& _autoHide);
        void showToolTip(bool& _show);
        // Need to show vertical volume control in right time.
        void isVideoWindowActive(bool&);
        // Update conferention layout.
        void updateConferenceMode(voip_manager::ConferenceLayout layout);
        void addUserToConference();

    private Q_SLOTS:
        void onHangUpButtonClicked();
        void onAudioOnOffClicked();
        void onVideoOnOffClicked();
        void _onFullscreenClicked();
        void controlActivated(bool);

        void onClickGoChat();
        void onClickAddChat();
        void onClickSettings();

        void onMinimalBandwithMode();

        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);        
        void onVoipMinimalBandwidthChanged (bool _bEnable);
        void hideBandwidthTooltip();
        void activateVideoWindow();

        void onChangeConferenceMode();
        void onAddUserClicked();

    public:
        VideoPanel(QWidget* _parent, QWidget* _container);
        ~VideoPanel();

        void setFullscreenMode(bool _en);
        void fadeIn(unsigned int kAnimationDefDuration) override;
        void fadeOut(unsigned int  kAnimationDefDuration) override;
        bool isFadedIn() override;


        void talkCreated();
        // Calls when your companion accept the call.
        void talkStarted();
        void talkFinished();
        
        void startToolTipHideTimer();
        
        bool isActiveWindow();
        void setContacts(const std::vector<voip_manager::Contact>&);
        void changeConferenceMode(voip_manager::ConferenceLayout layout);

    private:
        void resetHangupText();
        void updateToolTipsPosition();
        bool isNormalPanelMode();
		bool isFitSpacersPanelMode();
        void updateConferenceModeButton();
        void updateBandwidthButtonState();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;
        void moveEvent(QMoveEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void hideEvent(QHideEvent *) override;
        void showEvent(QShowEvent *)  override;

    private:
        bool mouseUnderPanel_;

        QWidget* container_;
        QWidget* parent_;
        QWidget* rootWidget_;

        std::vector<voip_manager::Contact> activeContact_;
        QPushButton* addChatButton_;
        QPushButton* fullScreenButton_;
        QPushButton* micButton_;
        QPushButton* minimalBandwidthMode_;
        QPushButton* settingsButton_;
        QPushButton* stopCallButton_;
        QPushButton* videoButton_;
        QPushButton* conferenceModeButton_;
        QPushButton* addUsers_;

        QTimer*    hideBandwidthTooltipTimer;
        QVector<QWidget* > hideButtonList;

        ToolTipEx* minimalBandwidthTooltip_;

        VolumeGroup* volumeGroup;

        UIEffects* videoPanelEffect_;
        UIEffects* minimalBandwidthTooltipEffect_;

		QSpacerItem* leftSpacer_;
		QSpacerItem* rightSpacer_;
        //Are we talking in this moment?
        bool isTakling;

        bool isFadedVisible;
    };
}

#endif//__VIDEO_PANEL_H__
