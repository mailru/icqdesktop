#pragma once

namespace Ui
{
    class CustomButton;
    class FFMpegPlayer;


    class InstChangedSlider : public QSlider
    {
        Q_OBJECT

    protected:

        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void wheelEvent(QWheelEvent* _event) override;
    
    public:

        InstChangedSlider(Qt::Orientation _orientation, QWidget* _parent);
    };


    class VideoPlayerControlPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void signalFullscreen(bool _checked);
        void mouseMoved();

    public Q_SLOTS:

        void videoDurationChanged(const qint64 _duration);
        void videoPositionChanged(const qint64 _position);

        void playerPaused();
        void playerPlayed();

    private:

        InstChangedSlider* progressSlider_;
        
        QLabel* timeLeft_;
        QLabel* timeRight_;

        qint64 duration_;
        qint64 position_;

        FFMpegPlayer* player_;

        QPushButton* playButton_;
        
        QWidget* soundWidget_;
        QPushButton* soundButton_;
        QString lastSoundButtonMode_;
        QWidget* gradient_;

        InstChangedSlider* volumeSlider_;

        QPushButton* fullscreenButton_;
        QPushButton* normalscreenButton_;

        QTimer* positionSliderTimer_;

        QWidget* volumeContainer_;
        QVBoxLayout* fullScreenLayout_;
        QHBoxLayout* baseLayout_;
        QVBoxLayout* playLayout_;
        QVBoxLayout* progressSliderLayout_;
        QVBoxLayout* soundLayout_;
        QWidget* soundButtonVolumeSpacer_;
        QVBoxLayout* timeLayout_;

        const bool fullscreen_;

    private:
        
        void connectSignals(FFMpegPlayer* _player);
        QString getSoundButtonMode(const int32_t _volume);
        void showVolumeControl(const bool _isShow);
        void updateSoundButtonState();

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void resizeEvent(QResizeEvent * event) override;
        virtual void mouseMoveEvent(QMouseEvent * _event) override;

    public:
        
        VideoPlayerControlPanel(
            VideoPlayerControlPanel* _copyFrom, 
            QWidget* _parent, 
            FFMpegPlayer* _player, 
            const QString& _mode = "dialog");

        virtual ~VideoPlayerControlPanel();

        bool isFullscreen() const;

        bool isPause() const;
        void setPause(const bool& _pause);

        void setVolume(const int32_t _volume, const bool _toRestore);
        int32_t getVolume() const;
        void restoreVolume();
    };



    class DialogPlayer : public QWidget
    {
        Q_OBJECT

    private:

        FFMpegPlayer* ffplayer_;

        std::unique_ptr<VideoPlayerControlPanel> controlPanel_;

        DialogPlayer* attachedPlayer_;

        QString mediaPath_;
        QWidget* rootWidget_;

        QTimer* timerHide_;
        QTimer* timerMousePress_;
        bool controlsShowed_;
        QWidget* parent_;
        QVBoxLayout* rootLayout_;

        static const uint32_t hideTimeout = 2000;
        static const uint32_t hideTimeoutShort = 100;

        bool isFullScreen_;
        bool isLoad_;
        bool isGif_;
        const bool showControlPanel_;

        QRect normalModePosition_;

        void init(QWidget* _parent, const bool _isGif);
        void moveToScreen();

    public:

        enum Flags
        {
            is_gif = 1 << 0,
            enable_control_panel = 1 << 1,
            use_opengl = 1 << 2,
            as_window = 1 << 3
        };

        void showAs();
        void showAsFullscreen();
        void showAsNormal();
        
        void start(bool _start);

        DialogPlayer(QWidget* _parent, const uint32_t _flags = 0);
        virtual ~DialogPlayer();

        bool openMedia(const QString& _mediaPath);

        void setPaused(const bool _paused, const bool _byUser);
        QMovie::MovieState state() const;

        void setPausedByUser(const bool _paused);
        bool isPausedByUser() const;
        
        void setVolume(const int32_t _volume, bool _toRestore = true);
        int32_t getVolume() const;
        void setMute(bool _mute);

        void updateSize(const QRect& _sz);

        void moveToTop();

        void setHost(QWidget* _host);

        bool isFullScreen() const;

        void setIsFullScreen(bool _is_full_screen);

        bool inited();

        void setPreview(QPixmap _preview);
        QPixmap getActiveImage() const;

        void setLoadingState(bool _isLoad);

        bool isGif() const;

        void setClippingPath(QPainterPath _clippingPath);

        void setAttachedPlayer(DialogPlayer* _player);
        DialogPlayer* getAttachedPlayer() const;

        QSize getVideoSize() const;

    public Q_SLOTS:

        void fullScreen(const bool _checked);

        void timerHide();
        void timerMousePress();
        void showControlPanel(const bool _isShow);
        void playerMouseMoved();
        void playerMouseLeaved();
        void onLoaded();
        void onFirstFrameReady();

    Q_SIGNALS:
        void loaded();
        void paused();
        void closed();
        void firstFrameReady();
        void mouseClicked();
        void fullScreenClicked();
        void mouseWheelEvent(const int _delta);

    protected:

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent * event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void paintEvent(QPaintEvent* _event) override;
        virtual void wheelEvent(QWheelEvent* _event) override;
        virtual void keyPressEvent(QKeyEvent* _event) override;
    private:
        void showControlPanel();
        void changeFullScreen();
    };
}
