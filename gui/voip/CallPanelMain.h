#ifndef __CALL_PANEL_H__
#define __CALL_PANEL_H__

#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"
#include "PushButton_t.h"
#include "secureCallWnd.h"
#include "VideoPanel.h"
#include "WindowHeaderFormat.h"

namespace voip_manager
{
    struct CipherState;
    struct ContactEx;
    struct Contact;
}

class QLabel;
namespace Ui
{
    class VolumeGroup;
    
    class SliderEx : public QWidget
    {
        Q_OBJECT

    private Q_SLOTS:
        void onVolumeChanged(int);
        void onVolumeReleased();
        void onIconClicked();

    Q_SIGNALS:
        void onSliderReleased();
        void onIconClick();
        void onSliderValueChanged(int);

    public:
        SliderEx(QWidget* _parent);
        ~SliderEx();

        void setValue(int);
        void setEnabled(bool);

        void setIconForState(const PushButton_t::eButtonState _state, const std::string& _image);
        void setIconSize(const int _w, const int _h);

        void setPropertyForIcon(const char* _name, bool _val);
        void setPropertyForSlider(const char* _name, bool _val);

    private:
        QHBoxLayout  *horizontalLayout_;
        PushButton_t *sliderIcon_;
        QSlider      *slider_;
    };

    class BackToVideoButton : public QPushButton
    {
        Q_OBJECT

    public:

        BackToVideoButton (QWidget * _parent = 0);

        void adjustSize();

	private:
		void updateIconPosition();

		QLabel* icon;
    };

    class QPushButtonEx;

    class CallPanelMainEx : public QWidget
    {
        Q_OBJECT
        public:
            //  |L        topPart       _-x|
            //  |--------------------------|
            //  |       bottomPart         |
            //  |    x x x x x x x x x x   |
            struct CallPanelMainFormat
            {
                eVideoPanelHeaderItems topPartFormat;
                unsigned topPartHeight;
                unsigned bottomPartHeight;
            };

        private Q_SLOTS:
            void _onMinimize();
            void _onMaximize();
            void _onClose();

            void onClickGoChat();
            void onCameraTurn();
            void onStopCall();
            void onMicTurn();
            void onSoundTurn();
            void onSecureCallClicked();
            void onSecureCallWndOpened();
            void onSecureCallWndClosed();
            void addUserToVideoCall();

            void onVoipCallNameChanged(const voip_manager::ContactsList&);
            void onVoipMediaLocalVideo(bool _enabled);
            void onVoipMediaLocalAudio(bool _enabled);
            void onVoipUpdateCipherState(const voip_manager::CipherState& _state);
            void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
            void onVoipCallTimeChanged(unsigned _secElapsed, bool _haveCall);

        Q_SIGNALS:
            void onMinimize();
            void onMaximize();
            void onClose();

            void onClickOpenChat(const std::string& _contact);
            void onBackToVideo();

        protected:
            void hideEvent(QHideEvent* _e) override;
            void moveEvent(QMoveEvent* _e) override;
            void resizeEvent(QResizeEvent* _e) override;
            void enterEvent(QEvent* _e) override;

        public:
            CallPanelMainEx(QWidget* _parent, const CallPanelMainFormat& _panelFormat);
            virtual ~CallPanelMainEx();

            void processOnWindowMaximized();
            void processOnWindowNormalled();

        private:
            template<typename ButtonType>
            ButtonType* addButton(QWidget& _parentWidget, const QString& _propertyName, const char* _slot, bool _bDefaultCursor = false, bool rightAlignment = false);
        
            VolumeGroup* addVolumeGroup(QWidget& _parentWidget, bool rightAlignment, int verticalSize);
        
        private:
            std::string               activeContact_;
            const CallPanelMainFormat format_;
            QWidget*                  rootWidget_;
            PushButton_t*             nameLabel_;
            VolumeControl             vVolControl_;
            VolumeControl             hVolControl_;
            bool                      secureCallEnabled_;
            VolumeGroup*              volumeGroup;
        
        private:
            QPushButton*   buttonMaximize_;
            QPushButton*   buttonLocalCamera_;
            QPushButton*   buttonLocalMic_;
            BackToVideoButton*   backToVideo_;
            SecureCallWnd* secureCallWnd_;
    };
}

#endif//__CALL_PANEL_H__
