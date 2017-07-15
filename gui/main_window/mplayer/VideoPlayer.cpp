#include "stdafx.h"
#include "VideoPlayer.h"

#include "../../utils/utils.h"
#include "../../gui_settings.h"
#include "../../controls/CommonStyle.h"

#include "../../utils/InterConnector.h"
#include "../MainWindow.h"

#include "FFMpegPlayer.h"
#include "../../fonts.h"

namespace Ui
{
    int getMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 16 : 8);
    }

    int getVolumeSliderHeight(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 80 : 48);
    }

    int getControlPanelSoundHeight(bool _isFullScreen)
    {
        return (int) getVolumeSliderHeight(_isFullScreen) + getMargin(_isFullScreen);
    }

    int getControlPanelButtonSize(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 32 : 24);
    }

    int getSoundButtonVolumeSpacerHeight(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    int getSoundsWidgetHeight(bool _isFullScreen)
    {
        return (int) getControlPanelButtonSize(_isFullScreen) + getControlPanelSoundHeight(_isFullScreen);
    }

    int getControlPanelMaxHeight(bool _isFullScreen)
    {
        return (int) getSoundsWidgetHeight(_isFullScreen) + 2 * getMargin(_isFullScreen) + getSoundButtonVolumeSpacerHeight(_isFullScreen);
    }

    int getVolumeSliderWidth(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 28 : 24);
    }

    int getTimeRightMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    int getTimeBottomMargin(bool _isFullScreen)
    {
        return (int) Utils::scale_value(_isFullScreen ? 8 : 4);
    }

    const uint32_t hideTimeout = 2000;
    const uint32_t hideTimeoutShort = 100;


    //////////////////////////////////////////////////////////////////////////
    // VideoProgressSlider
    //////////////////////////////////////////////////////////////////////////
    InstChangedSlider::InstChangedSlider(Qt::Orientation _orientation, QWidget* _parent = 0)
        : QSlider(_orientation, _parent)
    {

    }

    void InstChangedSlider::mousePressEvent(QMouseEvent* _event)
    {
        int32_t sliderWidth = width();

        if (_event->button() == Qt::LeftButton)
        {
            int32_t value = 0;

            if (orientation() == Qt::Vertical)
            {
                value = minimum() + ((maximum() - minimum()) * (height() - _event->y())) / height();
            }
            else
            {
                value = minimum() + ((maximum() - minimum()) * _event->x()) / sliderWidth;
            }

            setValue(value);

            _event->accept();

            emit sliderMoved(value);
            emit sliderReleased();
        }

        QSlider::mousePressEvent(_event);
    }

    void InstChangedSlider::wheelEvent(QWheelEvent* _event)
    {
        QSlider::wheelEvent(_event);

        emit sliderMoved(value());
        emit sliderReleased();
    }

    //////////////////////////////////////////////////////////////////////////
    // VideoPlayerControlPanel
    //////////////////////////////////////////////////////////////////////////
    VideoPlayerControlPanel::VideoPlayerControlPanel(
        VideoPlayerControlPanel* _copyFrom, 
        QWidget* _parent, 
        FFMpegPlayer* _player, 
        const QString& _mode)

        :   QWidget(_parent),
            player_(_player),
            duration_(_copyFrom ? _copyFrom->duration_ : 0),
            position_(_copyFrom ? _copyFrom->position_ : 0),
            positionSliderTimer_(new QTimer(this)),
            fullscreen_(_mode == "full_screen"),
            lastSoundButtonMode_(_copyFrom ? _copyFrom->lastSoundButtonMode_ : "0")
    {
        setProperty("mode", _mode);

        setStyleSheet(Utils::LoadStyle(":/main_window/mplayer/mstyles.qss"));

        int32_t volume = get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

        QVBoxLayout* rootLayout = new QVBoxLayout();
        rootLayout->setContentsMargins(0, 0, 0, 0);

        gradient_ = new QWidget(this);
        gradient_->setObjectName("control_panel_gradient");
        gradient_->setProperty("mode", _mode);
        gradient_->show();

        baseLayout_ = new QHBoxLayout();
        baseLayout_->setContentsMargins(getMargin(isFullScreen()), getMargin(isFullScreen()), getMargin(isFullScreen()), getMargin(isFullScreen()));
        baseLayout_->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

        {
            playLayout_ = new QVBoxLayout();
            playLayout_->setContentsMargins(0, getControlPanelSoundHeight(isFullScreen()), 0, 0);
            playLayout_->setSpacing(0);
            playLayout_->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

            playButton_ = new QPushButton(this);
            playButton_->setObjectName("VideoPlayerPlayButton");
            playButton_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
            playButton_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));
            playButton_->setCursor(Qt::PointingHandCursor);
            playButton_->setCheckable(true);

            playLayout_->addWidget(playButton_);
            baseLayout_->addLayout(playLayout_);
        }

        {
            progressSliderLayout_ = new QVBoxLayout();
            progressSliderLayout_->setContentsMargins(0, getControlPanelSoundHeight(false), 0, 0);
            progressSliderLayout_->setSpacing(0);
            progressSliderLayout_->setAlignment(Qt::AlignBottom);

            progressSlider_ = new InstChangedSlider(Qt::Orientation::Horizontal, this);
            progressSlider_->setObjectName("VideoProgressSlider");
            progressSlider_->setProperty("mode", _mode);
            progressSlider_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
            progressSlider_->setCursor(Qt::PointingHandCursor);
            progressSliderLayout_->addWidget(progressSlider_);
            baseLayout_->addLayout(progressSliderLayout_);
        }

        {
            timeLayout_ = new QVBoxLayout();
            timeLayout_->setContentsMargins(
                getTimeRightMargin(isFullScreen()), 
                getControlPanelSoundHeight(isFullScreen()), 
                getTimeRightMargin(isFullScreen()), 
                getTimeBottomMargin(isFullScreen()));

            timeLayout_->setSpacing(0);
            timeLayout_->setAlignment(Qt::AlignBottom);

            timeRight_ = new QLabel(this);
            timeRight_->setObjectName("VideoTimeProgress");
            timeRight_->setProperty("mode", _mode);
            timeRight_->setText("0:00");
            
            timeLayout_->addWidget(timeRight_);
            baseLayout_->addLayout(timeLayout_);
        }
        
        {
            soundWidget_ = new QWidget(this);
            soundWidget_->setFixedHeight(getSoundsWidgetHeight(isFullScreen()));
            soundWidget_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));

            QVBoxLayout* soundLayout = new QVBoxLayout();
            soundLayout->setContentsMargins(0, 0, 0, 0);
            soundLayout->setSpacing(0);
            soundLayout->setAlignment(Qt::AlignBottom);

            soundWidget_->setLayout(soundLayout);
            soundWidget_->installEventFilter(this);
            {
                soundButton_ = new QPushButton(this);
                soundButton_->setObjectName("VideoPlayerSoundButton");
                soundButton_->setFixedHeight(getControlPanelButtonSize(isFullScreen()));
                soundButton_->setFixedWidth(getControlPanelButtonSize(isFullScreen()));
                soundButton_->setCursor(Qt::PointingHandCursor);
                soundButton_->setProperty("mode", lastSoundButtonMode_);
                soundButton_->installEventFilter(this);

                volumeContainer_ = new QWidget(this);
                volumeContainer_->setFixedWidth(getVolumeSliderWidth(isFullScreen()));
                soundLayout->addWidget(volumeContainer_);
                {
                    QHBoxLayout* volumeLayout = new QHBoxLayout();
                    volumeLayout->setContentsMargins(0, 0, 0, 0);
                    volumeLayout->setSpacing(0);
                    volumeContainer_->setLayout(volumeLayout);

                    volumeSlider_ = new InstChangedSlider(Qt::Orientation::Vertical, this);
                    volumeSlider_->setOrientation(Qt::Orientation::Vertical);
                    volumeSlider_->setObjectName("VideoVolumeSlider");
                    volumeSlider_->setProperty("mode", _mode);
                    volumeSlider_->setMinimum(0);
                    volumeSlider_->setMaximum(100);
                    volumeSlider_->setPageStep(10);
                    volumeSlider_->setFixedHeight(getVolumeSliderHeight(isFullScreen()));
                    volumeSlider_->setCursor(Qt::PointingHandCursor);
                    volumeSlider_->setSliderPosition(volume);
                    volumeSlider_->setFixedWidth(getVolumeSliderWidth(isFullScreen()));
                    volumeSlider_->hide();
                    volumeLayout->addWidget(volumeSlider_);
                }
                
                soundButtonVolumeSpacer_ = new QWidget();
                soundButtonVolumeSpacer_->setFixedHeight(getSoundButtonVolumeSpacerHeight(isFullScreen()));
                soundLayout->addWidget(soundButtonVolumeSpacer_);
                soundLayout->addWidget(soundButton_);
            }
            baseLayout_->addWidget(soundWidget_);
        }

        {
            fullScreenLayout_ = new QVBoxLayout();
            fullScreenLayout_->setSpacing(0);
            fullScreenLayout_->setContentsMargins(0, getControlPanelSoundHeight(false), 0, 0);
            fullScreenLayout_->setAlignment(Qt::AlignBottom);

            fullscreenButton_ = new QPushButton(this);
            fullscreenButton_->setObjectName("VideoPlayerFullscreenButton");
            fullscreenButton_->setFixedHeight(getControlPanelButtonSize(false));
            fullscreenButton_->setFixedWidth(getControlPanelButtonSize(false));
            fullscreenButton_->setCursor(Qt::PointingHandCursor);
            fullscreenButton_->setVisible(!isFullScreen());

            normalscreenButton_ = new QPushButton(this);
            normalscreenButton_->setObjectName("VideoPlayerNormalscreenButton");
            normalscreenButton_->setFixedHeight(getControlPanelButtonSize(false));
            normalscreenButton_->setFixedWidth(getControlPanelButtonSize(false));
            normalscreenButton_->setCursor(Qt::PointingHandCursor);
            normalscreenButton_->setVisible(isFullScreen());

            fullScreenLayout_->addWidget(fullscreenButton_);
            fullScreenLayout_->addWidget(normalscreenButton_);

            baseLayout_->addLayout(fullScreenLayout_);
        }

        rootLayout->addLayout(baseLayout_);

        setLayout(rootLayout);

        connectSignals(_player);

        setMouseTracking(true);

        if (_copyFrom)
        {
            videoDurationChanged(duration_);
            videoPositionChanged(_copyFrom->progressSlider_->value());
            playButton_->setChecked(_copyFrom->playButton_->isChecked());
            volumeSlider_->setSliderPosition(_copyFrom->volumeSlider_->sliderPosition());

            connect(progressSlider_, &QSlider::sliderMoved, _copyFrom->progressSlider_, [_copyFrom](int _value)
            {
                _copyFrom->videoPositionChanged(_value);
            });
        }
    }
    
    VideoPlayerControlPanel::~VideoPlayerControlPanel()
    {
    }

    void VideoPlayerControlPanel::showVolumeControl(const bool _isShow)
    {
        if (!_isShow)
        {
            volumeSlider_->hide();
        }
        else
        {
            volumeSlider_->show();
        }
    }

    void VideoPlayerControlPanel::paintEvent(QPaintEvent* _e)
    {
        QStyleOption style_option;
        style_option.init(this);

        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &style_option, &p, this);

        return QWidget::paintEvent(_e);
    }

    bool VideoPlayerControlPanel::eventFilter(QObject* _obj, QEvent* _event)
    {
        QObject* objectSoundButton = qobject_cast<QObject*>(soundButton_);
        QObject* objectSoundWidget = qobject_cast<QObject*>(soundWidget_);

        if (objectSoundButton == _obj && QEvent::Enter == _event->type())
        {
            showVolumeControl(true);
        }
        else if (objectSoundWidget == _obj && QEvent::Leave == _event->type())
        {
            showVolumeControl(false);
        }

        return QObject::eventFilter(_obj, _event);
    }

    void VideoPlayerControlPanel::mousePressEvent(QMouseEvent* _event)
    {
        QWidget::mousePressEvent(_event);
    }

    QString getDurationString(const qint64& _duration, const qint64& _init_duration)
    {
        const qint64 one_hour = (1000 * 60 * 60);
        const qint64 one_minute = (1000 * 60);
        const qint64 one_sec = (1000);

        qint64 hours = _duration / one_hour;
        qint64 minutes = (_duration - hours * one_hour) / one_minute;
        qint64 seconds = (_duration - hours * one_hour - minutes * one_minute) / one_sec;

        qint64 duration_hours = _init_duration / one_hour;
        qint64 duration_minutes = (_init_duration - duration_hours * one_hour) / one_minute;

        QString sout;

        QString hourString;
        if (duration_hours != 0)
            hourString.sprintf("%02d:", (int) hours);

        QString minutesString;
        if (duration_minutes >= 10)
            minutesString.sprintf("%02d:", (int) minutes);
        else
            minutesString.sprintf("%01d:", (int) minutes);

        QString secondsString;
        secondsString.sprintf("%02d", (int) seconds);

        sout = hourString + minutesString + secondsString;

        return sout;
    }

    QString getZeroTime(const qint64& _init_duration)
    {
        const qint64 one_hour = (1000 * 60 * 60);
        const qint64 one_minute = (1000 * 60);

        qint64 duration_hours = _init_duration / one_hour;
        qint64 duration_minutes = (_init_duration - duration_hours * one_hour) / one_minute;

        QString sout;

        QString hourString;
        if (duration_hours != 0)
            hourString.sprintf("%02d:", (int) 0);

        QString minutesString;
        if (duration_minutes >= 10)
            minutesString.sprintf("%02d:", (int) 0);
        else
            minutesString.sprintf("%01d:", (int) 0);

        QString secondsString;
        secondsString.sprintf("%02d", (int) 0);

        sout = hourString + minutesString + secondsString;

        return sout;
    }

    void VideoPlayerControlPanel::resizeEvent(QResizeEvent* _event)
    {
        const auto button_size = getControlPanelButtonSize(isFullscreen());

        progressSlider_->setFixedHeight(getControlPanelButtonSize(isFullscreen()));

        volumeContainer_->setFixedWidth(getVolumeSliderWidth(isFullscreen()));

        volumeSlider_->setFixedHeight(getVolumeSliderHeight(isFullscreen()));
        volumeSlider_->setFixedWidth(getVolumeSliderWidth(isFullscreen()));

        auto font = Fonts::appFontScaled(isFullscreen() ? 18 : 14, Fonts::FontWeight::Medium);
        QFontMetrics m(font);

        timeRight_->setVisible(_event->size().width() >= Utils::scale_value(200));
        progressSlider_->setVisible(_event->size().width() >= Utils::scale_value(96));

        if (_event->size().width() < Utils::scale_value(68))
        {
            fullscreenButton_->setFixedWidth(0);
            normalscreenButton_->setFixedWidth(0);
        }
        else
        {
            fullscreenButton_->setFixedSize(button_size, button_size);
            normalscreenButton_->setFixedSize(button_size, button_size);
        }

        if (_event->size().width() < Utils::scale_value(48))
        {
            soundWidget_->setFixedWidth(0);
        }
        else
        {
            soundWidget_->setFixedHeight(getSoundsWidgetHeight(isFullscreen()));
            soundWidget_->setFixedWidth(getControlPanelButtonSize(isFullscreen()));

            soundButton_->setFixedHeight(getControlPanelButtonSize(isFullscreen()));
            soundButton_->setFixedWidth(getControlPanelButtonSize(isFullscreen()));
        }

        playButton_->setFixedSize(button_size, button_size);

        const char* mode = isFullscreen() ? "full_screen" : "dialog";

        timeRight_->setProperty("mode", mode);
        progressSlider_->setProperty("mode", mode);
        volumeSlider_->setProperty("mode", mode);

        const int gradientHeight = isFullscreen() ? Utils::scale_value(64) : Utils::scale_value(40);
        const int gradientWidth = _event->size().width();
        gradient_->setFixedSize(gradientWidth, gradientHeight);
        gradient_->move(QPoint(0, _event->size().height() - gradientHeight));
    }

    void VideoPlayerControlPanel::videoDurationChanged(const qint64 _duration)
    {
        duration_ = _duration;;

        progressSlider_->setMinimum(0);
        progressSlider_->setMaximum((int) _duration);
        progressSlider_->setPageStep(_duration/10);

        timeRight_->setText(getDurationString(_duration, duration_));

        auto font = Fonts::appFontScaled((isFullscreen() ? 18 : 14), Fonts::FontWeight::Medium);
        QFontMetrics m(font);
        timeRight_->setFixedWidth(m.width(getZeroTime(duration_)));
    }

    void VideoPlayerControlPanel::videoPositionChanged(const qint64 _position)
    {
        position_ = _position;

        if (!progressSlider_->isSliderDown())
            progressSlider_->setSliderPosition((int) _position);

        timeRight_->setText(getDurationString(duration_ - position_, duration_));
    }

    void VideoPlayerControlPanel::connectSignals(FFMpegPlayer* _player)
    {
        connect(_player, &FFMpegPlayer::durationChanged, this, &VideoPlayerControlPanel::videoDurationChanged);
        connect(_player, &FFMpegPlayer::positionChanged, this, &VideoPlayerControlPanel::videoPositionChanged);
        connect(_player, &FFMpegPlayer::paused, this, &VideoPlayerControlPanel::playerPaused, Qt::QueuedConnection);
        connect(_player, &FFMpegPlayer::played, this, &VideoPlayerControlPanel::playerPlayed, Qt::QueuedConnection);

        connect(progressSlider_, &QSlider::sliderMoved, this, [this](int /*_value*/)
        {
            player_->setPosition(progressSlider_->value());

            positionSliderTimer_->stop();
        });


        connect(playButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            if (_checked)
            {
                player_->pause();
                player_->setPausedByUser(true);
            }
            else
            {
                player_->play(true /* _init */);
                player_->setPausedByUser(false);
            }
        });

        connect(volumeSlider_, &QSlider::sliderMoved, this, [this](int _value)
        {
            setVolume(_value, false);
        });

        connect(volumeSlider_, &QSlider::sliderReleased, this, [this]()
        {
            auto pos = volumeSlider_->sliderPosition();

            if (pos != 0)
            {
                player_->setRestoreVolume(pos);

                Ui::get_gui_settings()->set_value<int32_t>(setting_mplayer_volume, pos);
            }
        });


        connect(soundButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            if (volumeSlider_->sliderPosition() == 0)
            {
                restoreVolume();
            }
            else
            {
                setVolume(0, false);
            }
        });

        connect(fullscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            QString mode = property("mode").toString();

            if (mode != "dialog")
            {
                fullscreenButton_->setVisible(false);
                normalscreenButton_->setVisible(true);
            }

            emit signalFullscreen(true);
        });

        connect(normalscreenButton_, &QPushButton::clicked, this, [this](bool _checked)
        {
            QString mode = property("mode").toString();

            if (mode != "dialog")
            {
                normalscreenButton_->setVisible(false);
                fullscreenButton_->setVisible(true);
            }

            emit signalFullscreen(false);
        });

        connect(_player, &FFMpegPlayer::mediaFinished, this, [this]()
        {
            playButton_->setChecked(true);
            progressSlider_->setValue(0);
        });
    }

    void VideoPlayerControlPanel::playerPaused()
    {
        if (playButton_->isChecked())
            return;

        playButton_->setChecked(true);
    }

    void VideoPlayerControlPanel::playerPlayed()
    {
        if (!playButton_->isChecked())
            return;

        playButton_->setChecked(false);
    }

    bool VideoPlayerControlPanel::isFullscreen() const
    {
        return fullscreen_;
    }

    bool VideoPlayerControlPanel::isPause() const
    {
        return playButton_->isChecked();
    }

    void VideoPlayerControlPanel::setPause(const bool& _pause)
    {
        playButton_->setChecked(_pause);
    }

    QString VideoPlayerControlPanel::getSoundButtonMode(const int32_t _volume)
    {
        if (_volume == 0)
        {
            return "0";
        }
        else if (_volume < 50)
        {
            return "1";
        }
        else
        {
            return "2";
        }
    }

    void VideoPlayerControlPanel::setVolume(const int32_t _value, const bool _toRestore)
    {
        player_->setVolume(_value);

        player_->setMute(_value == 0);

        volumeSlider_->setValue(_value);

        if (_toRestore)
        {
            player_->setRestoreVolume(_value);

            Ui::get_gui_settings()->set_value<int32_t>(setting_mplayer_volume, _value);
        }

        updateSoundButtonState();
    }

    int32_t VideoPlayerControlPanel::getVolume() const
    {
        return player_->getVolume();
    }

    void VideoPlayerControlPanel::restoreVolume()
    {
        volumeSlider_->setValue(player_->getRestoreVolume());

        player_->setVolume(player_->getRestoreVolume());

        player_->setMute(false);

        updateSoundButtonState();
    }

    void VideoPlayerControlPanel::updateSoundButtonState()
    {
        auto newSoundButtonMode = getSoundButtonMode(player_->isMute() ? 0 : player_->getVolume());

        if (lastSoundButtonMode_ != newSoundButtonMode)
        {
            lastSoundButtonMode_ = newSoundButtonMode;
            soundButton_->setProperty("mode", newSoundButtonMode);
            soundButton_->style()->unpolish(soundButton_);
            soundButton_->style()->polish(soundButton_);
            soundButton_->update();
        }
    }

    


    void VideoPlayerControlPanel::mouseMoveEvent(QMouseEvent * _event)
    {
        QWidget::mouseMoveEvent(_event);
    }




    //////////////////////////////////////////////////////////////////////////
    // DialogPlayer
    //////////////////////////////////////////////////////////////////////////
    DialogPlayer::DialogPlayer(QWidget* _parent, const uint32_t _flags)
        :   QWidget(_parent)
            , attachedPlayer_(nullptr)
            , showControlPanel_(_flags & DialogPlayer::Flags::enable_control_panel)
    {
        const QString mode = ((_flags & DialogPlayer::Flags::as_window) ? "window" : "dialog");

        if (DialogPlayer::Flags::as_window & _flags)
        {
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        }

        ffplayer_ = new FFMpegPlayer(this, (_flags & DialogPlayer::Flags::use_opengl));

        init(_parent, (_flags &DialogPlayer::Flags::is_gif));

        isFullScreen_ = false;

        rootLayout_->addWidget(ffplayer_);

        controlPanel_.reset(new VideoPlayerControlPanel(nullptr, this, ffplayer_, mode));
        controlPanel_->installEventFilter(this);

        if (showControlPanel_)
        {
            //controlPanel_->raise();
        }
        else
        {
            controlPanel_->hide();
        }

        connect(controlPanel_.get(), &VideoPlayerControlPanel::signalFullscreen, this, &DialogPlayer::fullScreen);
    }

    DialogPlayer::~DialogPlayer()
    {
        if (attachedPlayer_)
        {
            attachedPlayer_->setVolume(getVolume(), false);
            attachedPlayer_->setAttachedPlayer(nullptr);
        }
    }

    void DialogPlayer::init(QWidget* _parent, const bool _isGif)
    {
        controlsShowed_ = false;

        timerHide_ = new QTimer(this);
        timerMousePress_ = new QTimer(this);

        parent_ = _parent;
        isLoad_ = false;
        isGif_ = _isGif;

        rootLayout_ = new QVBoxLayout();
        rootLayout_->setContentsMargins(0, 0, 0, 0);
        rootLayout_->setSpacing(0);

        setLayout(rootLayout_);

        connect(timerHide_, &QTimer::timeout, this, &DialogPlayer::timerHide, Qt::QueuedConnection);
        connect(timerMousePress_, &QTimer::timeout, this, &DialogPlayer::timerMousePress, Qt::QueuedConnection);
        connect(ffplayer_, &FFMpegPlayer::mouseMoved, this, &DialogPlayer::playerMouseMoved);
        connect(ffplayer_, &FFMpegPlayer::mouseLeaveEvent, this, &DialogPlayer::playerMouseLeaved);
        connect(ffplayer_, &FFMpegPlayer::fileLoaded, this, &DialogPlayer::onLoaded);
        connect(ffplayer_, &FFMpegPlayer::firstFrameReady, this, &DialogPlayer::onFirstFrameReady);
        setMouseTracking(true);

        installEventFilter(this);
    }

    void DialogPlayer::moveToScreen()
    {
        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();

        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        setGeometry(screenGeometry);
    }

    void DialogPlayer::paintEvent(QPaintEvent* _e)
    {
        QStyleOption style_option;
        style_option.init(this);

        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &style_option, &p, this);

        return QWidget::paintEvent(_e);
    }

    bool DialogPlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        switch (_event->type())
        {
            case QEvent::Enter:
            {
                QObject* objectcontrolPanel = qobject_cast<QObject*>(controlPanel_.get());
                if (_obj == objectcontrolPanel)
                {
                    timerHide_->stop();
                }
                break;
            }
            case QEvent::Leave:
            {
                QObject* objectcontrolPanel = qobject_cast<QObject*>(controlPanel_.get());
                if (_obj == objectcontrolPanel)
                {
                    {
                        timerHide_->start(hideTimeoutShort);
                    }
                }
                break;
            }

            case QEvent::KeyPress:
            {
                QKeyEvent *ke = static_cast<QKeyEvent *>(_event);

                if (ke->key() == Qt::Key_Escape)
                {
                    changeFullScreen();
                }

                return true;
            }
            default:
                break;
        }

        return QObject::eventFilter(_obj, _event);
    }

    void DialogPlayer::timerHide()
    {
        showControlPanel(false);
        timerHide_->stop();
        controlsShowed_ = false;
    }

    void DialogPlayer::showControlPanel(const bool _isShow)
    {
        if (!showControlPanel_)
            return;

        const QRect rcParent = rect();

        auto panelGeom = rcParent;

        panelGeom.setTop(panelGeom.bottom() - getControlPanelMaxHeight(controlPanel_->isFullscreen()));

        controlPanel_->setGeometry(panelGeom);

        if (_isShow)
        {
            controlPanel_->show();
            controlPanel_->raise();
        }
        else
        {
            controlPanel_->hide();
        }
    }

    void DialogPlayer::playerMouseMoved()
    {
        showControlPanel();
    }

    void DialogPlayer::showControlPanel()
    {
        if (isGif_)
        {
            return;
        }

        if (controlsShowed_)
            return;

        timerHide_->stop();
        timerHide_->start(hideTimeout);
        
        controlsShowed_ = true;
        showControlPanel(true);
    }

    void DialogPlayer::playerMouseLeaved()
    {
        timerHide_->stop();
        timerHide_->start(hideTimeoutShort);
    }

    bool DialogPlayer::openMedia(const QString& _mediaPath)
    {
        isLoad_ = true;
        mediaPath_ = _mediaPath;
        
        return ffplayer_->openMedia(_mediaPath);
    }

    void DialogPlayer::setPaused(const bool _paused, const bool _byUser)
    {
        if (!_paused)
        {
            start(true);
            setPausedByUser(false);
        }
        else
        {
            ffplayer_->pause();
            controlPanel_->setPause(_paused);

            if (ffplayer_->getStarted())
                showControlPanel();

            setPausedByUser(_byUser);

            emit paused();
        }
    }

    void DialogPlayer::setPausedByUser(const bool _paused)
    {
        ffplayer_->setPausedByUser(_paused);
    }

    bool DialogPlayer::isPausedByUser() const
    {
        return ffplayer_->isPausedByUser();
    }

    QMovie::MovieState DialogPlayer::state() const
    {
        return ffplayer_->state();
    }

    void DialogPlayer::setVolume(const int32_t _volume, bool _toRestore)
    {
        controlPanel_->setVolume((double) _volume, _toRestore);
    }

    int32_t DialogPlayer::getVolume() const
    {
        return (int32_t) controlPanel_->getVolume();
    }

    void DialogPlayer::setMute(bool _mute)
    {
        if (_mute)
        {
            controlPanel_->setVolume(0, false);
        }
        else
        {
            controlPanel_->restoreVolume();
        }
    }

    void DialogPlayer::start(bool _start)
    {
        ffplayer_->play(_start);
        if (_start)
        {
            controlPanel_->setPause(false);
            ffplayer_->setPausedByUser(false);
        }
    }

    void DialogPlayer::updateSize(const QRect& _sz)
    {
        setGeometry(_sz);

        showControlPanel(controlsShowed_);
    }

    void DialogPlayer::changeFullScreen()
    {
        bool isFullScreen = controlPanel_->isFullscreen();

        fullScreen(!isFullScreen);

    }

    void DialogPlayer::timerMousePress()
    {
        moveToTop();

        setPaused(!controlPanel_->isPause(), true);

        timerMousePress_->stop();
    }

    void DialogPlayer::mousePressEvent(QMouseEvent* _event)
    {
        QWidget::mousePressEvent(_event);
    }

    void DialogPlayer::mouseReleaseEvent(QMouseEvent* _event)
    {
        if(_event->button() == Qt::RightButton)
        {
            QWidget::mousePressEvent(_event);
        }
        else if(_event->button() == Qt::LeftButton)
        {
            if (rect().contains(_event->pos()))
            {
                if (!isGif())
                {
                    if (isFullScreen())
                    {
                        setPaused(!controlPanel_->isPause(), true);
                    }

                    emit mouseClicked();
                }
            }

            return QWidget::mouseReleaseEvent(_event);
        }
    }

    void DialogPlayer::showAsFullscreen()
    {
        normalModePosition_ = geometry();

        const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();

        const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);

        hide();

        updateSize(screenGeometry);

        if (platform::is_windows())
            showMaximized();
        else
            showFullScreen();

        showControlPanel(controlsShowed_);

        show();
    }

    void DialogPlayer::showAs()
    {
        show();

        showControlPanel(controlsShowed_);
    }

    void DialogPlayer::showAsNormal()
    {
        showNormal();

        setGeometry(normalModePosition_);

        showControlPanel(controlsShowed_);
    }

    void DialogPlayer::fullScreen(const bool _checked)
    {
        emit fullScreenClicked();
    }

    void DialogPlayer::moveToTop()
    {
        raise();
        ffplayer_->raise();
        controlPanel_->raise();
    }

    bool DialogPlayer::isFullScreen() const
    {
        return isFullScreen_;
    }

    void DialogPlayer::setIsFullScreen(bool _isFullScreen)
    {
        isFullScreen_ = _isFullScreen;
    }

    bool DialogPlayer::inited()
    {
        return ffplayer_->getStarted();
    }

    void DialogPlayer::setPreview(QPixmap _preview)
    {
        ffplayer_->setPreview(_preview);
    }

    QPixmap DialogPlayer::getActiveImage() const
    {
        return ffplayer_->getActiveImage();
    }

    void DialogPlayer::onLoaded()
    {
        emit loaded();
    }

    void DialogPlayer::onFirstFrameReady()
    {
        emit firstFrameReady();
    }

    void DialogPlayer::setLoadingState(bool _isLoad)
    {
        //qDebug() << "setLoadingState " << _isLoad << ", old: " << isLoad_ << ", " << mediaPath_;

        if (_isLoad == isLoad_)
            return;
        isLoad_ = _isLoad;

        if (isLoad_)
        {
            //qDebug() << "reload media " << mediaPath_;
            ffplayer_->openMedia(mediaPath_);
        }
        else
        {
            //qDebug() << "unload media " << mediaPath_;
            ffplayer_->stop();
        }
    }

    void DialogPlayer::setHost(QWidget* _host)
    {
        parent_ = _host;
    }

    bool DialogPlayer::isGif() const
    {
        return isGif_;
    }

    void DialogPlayer::setClippingPath(QPainterPath _clippingPath)
    {
        ffplayer_->setClippingPath(_clippingPath);
    }

    void DialogPlayer::setAttachedPlayer(DialogPlayer* _player)
    {
        attachedPlayer_ = _player;
    }

    DialogPlayer* DialogPlayer::getAttachedPlayer() const
    {
        return attachedPlayer_;
    }

    QSize DialogPlayer::getVideoSize() const
    {
        if (!ffplayer_)
            return QSize();

        return ffplayer_->getVideoSize();
    }

    void DialogPlayer::wheelEvent(QWheelEvent* _event)
    {
        emit mouseWheelEvent(_event->delta());

        QWidget::wheelEvent(_event);
    }

    void DialogPlayer::keyPressEvent(QKeyEvent* _event)
    {
        QWidget::keyPressEvent(_event);
    }
}
