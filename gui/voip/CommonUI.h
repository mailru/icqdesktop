
#pragma once

#include "../../core/Voip/VoipManagerDefines.h"
//#include <functional>
#include <memory>
//#include "../main_window/MainWindow.h"

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
	class BaseVideoPanel;
	class ShadowWindow;


	class ResizeEventFilter : public QObject
	{
		Q_OBJECT

	public:
		ResizeEventFilter(std::vector<BaseVideoPanel*>& panels, ShadowWindow* shadow, QObject* _parent);

		void addPanel(BaseVideoPanel* _panel);
		void removePanel(BaseVideoPanel* _panel);

	protected:
		bool eventFilter(QObject* _obj, QEvent* _event);

	private:
		std::vector<BaseVideoPanel*> panels_;
		ShadowWindow* shadow_;
	};

	class ShadowWindowParent
	{
	public:
		ShadowWindowParent(QWidget* parent);
        ~ShadowWindowParent();

		void showShadow();
		void hideShadow();
		ShadowWindow* getShadowWidget();
		void setActive(bool _value);

	protected:
		// shadow
		ShadowWindow* shadow_;
	};

	struct UIEffects
	{
		UIEffects(QWidget& _obj);
		virtual ~UIEffects();

		void fadeOut(unsigned _interval);
		void fadeIn(unsigned _interval);

		void geometryTo(const QRect& _rc, unsigned _interval);
        bool isFadedIn();

	private:
		bool fadedIn_;

		QGraphicsOpacityEffect* fadeEffect_;
		QPropertyAnimation* animation_;
		QPropertyAnimation* resizeAnimation_;

		QWidget& obj_;
	};

	class AspectRatioResizebleWnd : public QWidget
	{
		Q_OBJECT
	public:
		AspectRatioResizebleWnd();
		virtual ~AspectRatioResizebleWnd();

		bool isInFullscreen() const;
		void switchFullscreen();

		void useAspect();
		void unuseAspect();
        void saveMinSize(const QSize& size);

	protected:
		bool nativeEvent(const QByteArray& _eventType, void* _message, long* _result) override;
		virtual quintptr getContentWinId() = 0;
		virtual void escPressed() { }

		private Q_SLOTS:
		void onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs);
		//void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);

	private:
		float aspectRatio_;
		bool useAspect_;
        QSize originMinSize_;

		UIEffects* selfResizeEffect_;
		// Commented because we always constrain size of video window.
		//bool firstTimeUseAspectRatio_;

		void applyFrameAspectRatio(float _wasAr);
		void keyReleaseEvent(QKeyEvent*) override;
#ifdef _WIN32
		bool onWMSizing(RECT& _rc, unsigned _wParam);
#endif
        void fitMinimalSizeToAspect();
	};

    // Inherit from this class to create panel in voip window.
	class BaseVideoPanel : public QWidget	
	{
	public:
		BaseVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

		virtual void updatePosition(const QWidget& parent) = 0;

		virtual void fadeIn(unsigned int duration);
		virtual void fadeOut(unsigned int duration);
        
        bool isGrabMouse();
        virtual bool isFadedIn();

    protected:
        
        bool grabMouse;
        
    private:
        
		std::unique_ptr<UIEffects> effect_;
	};

    // Inherit from this class to create panel on top in voip window.
	class BaseTopVideoPanel : public BaseVideoPanel
	{
	public:
		BaseTopVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

		void updatePosition(const QWidget& parent) override;
	};

    // Inherit from this class to create panel on bottom in voip window.
	class BaseBottomVideoPanel : public BaseVideoPanel
	{
	public:
		BaseBottomVideoPanel(QWidget* parent, Qt::WindowFlags f = Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

		void updatePosition(const QWidget& parent) override;
	};

    // This panel fill all parent window
    class FullVideoWindowPanel : public Ui::BaseVideoPanel
    {
        Q_OBJECT

    public:        

        FullVideoWindowPanel(QWidget* parent);

        virtual void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration)  override {}
        virtual void fadeOut(unsigned int duration) override {}

    signals:

        void onResize();

    protected:

        virtual void	resizeEvent(QResizeEvent *event);
    };

    // Use this class if you want to process
    // mouse event on transparent panels.
    class PanelBackground : public QWidget
    {
    public:
        PanelBackground(QWidget* parent);

        void updateSizeFromParent();
    };
    
    class QSliderEx : public QSlider
    {
        Q_OBJECT
    public:
        QSliderEx(Qt::Orientation _orientation, QWidget* _parent = NULL);
        virtual ~QSliderEx();
        
    protected:
        void mousePressEvent(QMouseEvent* _ev) override;
    };
    
    class QPushButtonEx : public QPushButton
    {
        Q_OBJECT
    Q_SIGNALS:
        void onHover();
        
    public:
        QPushButtonEx(QWidget* _parent);
        virtual ~QPushButtonEx();
        
    protected:
        void enterEvent(QEvent* _e);
    };
    
    class VolumeControl : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void controlActivated(bool);
        void onMuteChanged(bool);
        void clicked();
        
    private Q_SLOTS:
        void onVoipVolumeChanged(const std::string&, int);
        void onVoipMuteChanged(const std::string&, bool);
        
        void onVolumeChanged(int);
        void onMuteOnOffClicked();
        void onCheckMousePos();
        
    public:
        VolumeControl(
                      QWidget* _parent,
                      bool _horizontal,
                      bool _onMainWindow,
                      const QString& _backgroundStyle,
                      const std::function<void(QPushButton&, bool)>& _onChangeStyle
                      );
        virtual ~VolumeControl();
        
        QPoint getAnchorPoint() const;
        virtual void hideSlider();
        
    protected:
        void leaveEvent(QEvent* _e) override;
        void showEvent(QShowEvent *) override;
        void hideEvent(QHideEvent *) override;
        void changeEvent(QEvent*) override;
        
        void updateSlider();

        
    protected:
        bool                                    audioPlaybackDeviceMuted_;
        bool                                    onMainWindow_;
        const QString                           background_;
        const bool                              horizontal_;
        int                                     actualVol_;
        QPushButtonEx*                          btn_;
        QSlider*                                slider_;
        QTimer                                  checkMousePos_;
        QWidget*                                parent_;
        QWidget*                                rootWidget_;
        std::function<void(QPushButton&, bool)> onChangeStyle_;
    };
    
    class VolumeControlHorizontal : public VolumeControl
    {
        Q_OBJECT
        
    public:
        VolumeControlHorizontal (QWidget* _parent, bool _onMainWindow,
                      const QString& _backgroundStyle,
                      const std::function<void(QPushButton&, bool)>& _onChangeStyle);
        
        void showSlider();
        QPoint soundButtonGlobalPosition();
        void hideSlider() override;
        
    Q_SIGNALS:
        void onHoverButton();
    };
    

    /**
     * This widget organizes volume control logic.
     * We have vertical and horizontal volume control depends of parent panel width.
     * But volume controls are not placed to this VolumeGroup, they are placed to parent panel,
     * because volume controls should not affect to panel layout.
     *
     * Organimzation is: parentPanel -> VolumeGroup -> FakeWidget
     *                      |
     *                       ---------> VolumeControlHorizontal
     *                      VolumeControl (vertical) is global, because it is placed outside from parent panel.
     */
    class VolumeGroup : public QWidget
    {
        Q_OBJECT
        
    public:
        
        VolumeGroup (QWidget* parent, bool onMainPage, const std::function<void(QPushButton&, bool)>& _onChangeStyle, int verticalSize);
        
        QWidget* verticalVolumeWidget();
        
        void hideSlider();
        void updateSliderPosition();
    
    Q_SIGNALS:
        
        void controlActivated(bool);
        // Need to show vertical volume control in right time.
        void isVideoWindowActive(bool&);
        void clicked();
        
    private Q_SLOTS:
        
        void showSlider();
        
    private:
        
        void moveEvent(QMoveEvent * event) override;
        
        void forceShowSlider();
        
        VolumeControl * vVolumeControl;
        VolumeControlHorizontal * hVolumeControl;
        int verticalSize;
    };


    /**
     * This widget is used to process mouse event for another panel if it has transparent pixels.
     * Under Windows it is imposible to get mouse event for tranparent part of widgets.
     * We have outgoing panel which implements drag & drop, but it has trasporent parts.
     */
    class TransparentPanel : public BaseVideoPanel
    {
        Q_OBJECT

    public:
        TransparentPanel(QWidget* _parent, BaseVideoPanel* _eventWidget);
        ~TransparentPanel();

        void updatePosition(const QWidget& parent) override;

        virtual void fadeIn(unsigned int duration) override {}
        virtual void fadeOut(unsigned int duration) override {}

    protected:

        void resizeEvent(QResizeEvent * event) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent * event) override;
        void mousePressEvent(QMouseEvent * event) override;

        template <typename E> void resendMouseEventToPanel(E* event_);

    protected:

        // We will resend events to this widget.
        BaseVideoPanel* eventWidget_;

        // Background widget to able process mouse events on transparent parts of panel.
        PanelBackground* backgroundWidget_;
    };

	// QWidget which has clicked signal.
	class PureClickedWidget : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS :
		void clicked();

	public:
		PureClickedWidget(QWidget* parent);
		void setEnabled(bool value);

	protected:
		virtual void mouseReleaseEvent(QMouseEvent *) override;

	private:
		bool Enabled_;
	};

    // Show dialog to add new users to video call.
    void showAddUserToVideoConverenceDialogVideoWindow(QObject* parent, FullVideoWindowPanel* parentWindow);
    void showAddUserToVideoConverenceDialogMainWindow(QObject* parent, QWidget* parentWindow);
}
