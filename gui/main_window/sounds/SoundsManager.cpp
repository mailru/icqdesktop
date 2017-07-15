#include "stdafx.h"
#include "SoundsManager.h"

#include "../../gui_settings.h"
#include "../contact_list/ContactListModel.h"
#include "../../utils/InterConnector.h"

#include "MpegLoader.h"

namespace openal
{
    #define AL_ALEXT_PROTOTYPES
    #include <AL/alext.h>
}

const int IncomingMessageInterval = 3000;
const int PttCheckInterval = 100;
const int DeviceCheckInterval = 60 * 1000;

namespace Ui
{
    void PlayingData::init()
    {
        openal::alGenSources(1, &Source_);
        openal::alSourcef(Source_, AL_PITCH, 1.f);
        openal::alSourcef(Source_, AL_GAIN, 1.f);
        openal::alSource3f(Source_, AL_POSITION, 0, 0, 0);
        openal::alSource3f(Source_, AL_VELOCITY, 0, 0, 0);
        openal::alSourcei(Source_, AL_LOOPING, 0);
        openal::alGenBuffers(1, &Buffer_);
    }

    void PlayingData::setBuffer(const QByteArray& data, qint64 freq, qint64 fmt)
    {
        openal::alBufferData(Buffer_, fmt, data.constData(), data.size(), freq);
        openal::alSourcei(Source_, AL_BUFFER, Buffer_);
    }

    int PlayingData::play()
    {
        if (isEmpty())
            return 0;

        auto duration = calcDuration();
        GetSoundsManager()->sourcePlay(Source_);
        return duration;
    }

    void PlayingData::pause()
    {
        if (isEmpty())
            return;

        openal::alSourcePause(Source_);
    }

    void PlayingData::stop()
    {
        if (isEmpty())
            return;

        openal::alSourceStop(Source_);
        if (openal::alIsBuffer(Buffer_)) 
        {
            openal::alSourcei(Source_, AL_BUFFER, 0);
            openal::alDeleteBuffers(1, &Buffer_);
        }
    }

    void PlayingData::clear()
    {
        Buffer_ = 0;
        Source_ = 0;
        Id_ = -1;
    }
    
    void PlayingData::free()
    {
        if (!isEmpty())
        {
            stop();
            openal::alDeleteSources(1, &Source_);
            clear();
        }
    }

    void PlayingData::clearData()
    {
        openal::alSourceStop(Source_);
        openal::ALuint buffer = 0;
        openal::alSourceUnqueueBuffers(Source_, 1, &buffer);
    }

    bool PlayingData::isEmpty() const
    {
        return Id_ == -1;
    }

    int PlayingData::calcDuration()
    {
        openal::ALint sizeInBytes;
        openal::ALint channels;
        openal::ALint bits;

        openal::alGetBufferi(Buffer_, AL_SIZE, &sizeInBytes);
        openal::alGetBufferi(Buffer_, AL_CHANNELS, &channels);
        openal::alGetBufferi(Buffer_, AL_BITS, &bits);

        auto lengthInSamples = sizeInBytes * 8 / (channels * bits);
        openal::ALint frequency;
        openal::alGetBufferi(Buffer_, AL_FREQUENCY, &frequency);
        return ((float)lengthInSamples / (float)frequency) * 1000;
    }

    openal::ALenum PlayingData::state() const
    {
        openal::ALenum state = AL_NONE;
        if (!isEmpty())
        {
            openal::alGetSourcei(Source_, AL_SOURCE_STATE, &state);
        }
        return state;
    }

	SoundsManager::SoundsManager()
		: QObject(0)
		, CallInProgress_(false)
		, CanPlayIncoming_(true)
		, Timer_(new QTimer(this))
        , PttTimer_(new QTimer(this))
        , DeviceTimer_(new QTimer(this))
        , AlId(-1)
        , AlAudioDevice_(0)
        , AlAudioContext_(0)
        , AlInited_(false)
	{
        initIncomig();
        initOutgoing();
        initMail();
		Timer_->setInterval(IncomingMessageInterval);
		Timer_->setSingleShot(true);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(timedOut()), Qt::QueuedConnection);

        PttTimer_->setInterval(PttCheckInterval);
        PttTimer_->setSingleShot(true);
        connect(PttTimer_, SIGNAL(timeout()), this, SLOT(checkPttState()), Qt::QueuedConnection);
        
        DeviceTimer_->setInterval(DeviceCheckInterval);
        DeviceTimer_->setSingleShot(true);
        connect(DeviceTimer_, SIGNAL(timeout()), this, SLOT(deviceTimeOut()), Qt::QueuedConnection);

        connect(this, SIGNAL(needUpdateDeviceTimer()), this, SLOT(updateDeviceTimer()), Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyControlReady, this, &SoundsManager::contactChanged, Qt::QueuedConnection);
	}

	SoundsManager::~SoundsManager()
	{
        CurPlay_.free();
        PrevPlay_.free();
        Incoming_.free();
		Mail_.free();
        Outgoing_.free();
        if (AlInited_)
            shutdownOpenAl();
	}

	void SoundsManager::timedOut()
	{
		CanPlayIncoming_ = true;
        Incoming_.clearData();
        Mail_.clearData();
        Outgoing_.clearData();
	}

    void SoundsManager::updateDeviceTimer()
    {
        if (!DeviceTimer_->isActive() || DeviceTimer_->remainingTime() < DeviceTimer_->interval() / 2)
            DeviceTimer_->start();
    }

    void SoundsManager::checkPttState()
    {
        if (CurPlay_.Source_ != 0)
        {
            openal::ALenum state;
            openal::alGetSourcei(CurPlay_.Source_, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING || state == AL_INITIAL)
            {
                PttTimer_->start();
            }
            else if (state == AL_PAUSED)
            {
                emit pttPaused(CurPlay_.Id_);
            }
            else if (state == AL_STOPPED)
            {
                emit pttFinished(CurPlay_.Id_, true);
                if (!PrevPlay_.isEmpty())
                {
                    CurPlay_.stop();
                    CurPlay_.clear();
                    CurPlay_ = PrevPlay_;
                    PrevPlay_.clear();
                }
            }
        }
    }

    void SoundsManager::contactChanged(QString)
    {
        if (CurPlay_.state() == AL_PLAYING)
        {
            CurPlay_.stop();
            emit pttPaused(CurPlay_.Id_);
        }
    }

    void SoundsManager::deviceTimeOut()
    {
        openal::alcDevicePauseSOFT(AlAudioDevice_);
    }

	void SoundsManager::playIncomingMessage()
	{
        if (CurPlay_.state() == AL_PLAYING)
            return;

		if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) && CanPlayIncoming_ && !CallInProgress_)
		{
			CanPlayIncoming_ = false;
            Incoming_.play();
			Timer_->start();
		}
	}

    void SoundsManager::playIncomingMail()
    {
        if (CurPlay_.state() == AL_PLAYING)
            return;

        if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) && CanPlayIncoming_ && !CallInProgress_)
        {
            CanPlayIncoming_ = false;
            Mail_.play();
            Timer_->start();
        }
    }

	void SoundsManager::playOutgoingMessage()
	{
        if (CurPlay_.state() == AL_PLAYING)
            return;

		if (get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false) && !CallInProgress_)
        {
            Outgoing_.play();
            Timer_->start();
        }
	}

    int SoundsManager::playPtt(const QString& file, int id, int& duration)
    {
        if (!AlInited_)
            initOpenAl();

        if (!CurPlay_.isEmpty())
        {
            if (!PrevPlay_.isEmpty())
            {
                if (PrevPlay_.state() == AL_PAUSED && PrevPlay_.Id_ == id)
                {
                    if (CurPlay_.state() == AL_PLAYING)
                    {
                        CurPlay_.pause();
                        emit pttPaused(CurPlay_.Id_);
                    }

                    PlayingData exchange;
                    exchange = PrevPlay_;
                    PrevPlay_ = CurPlay_;
                    CurPlay_ = exchange;

                    duration = CurPlay_.play();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }
            }
            if (CurPlay_.state() == AL_PLAYING)
            {
                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.stop();
                    emit pttFinished(PrevPlay_.Id_, false);
                    PrevPlay_.clear();
                }
                
                CurPlay_.pause();
                emit pttPaused(CurPlay_.Id_);
                PrevPlay_ = CurPlay_;
            }
            else if (CurPlay_.state() == AL_PAUSED)
            {
                if (CurPlay_.Id_ == id)
                {
                    duration = CurPlay_.play();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }
                
                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.stop();
                    emit pttFinished(PrevPlay_.Id_, false);
                    PrevPlay_.clear();
                }

                PrevPlay_ = CurPlay_;
            }
            else if (CurPlay_.state() == AL_STOPPED)
            {
                emit pttFinished(CurPlay_.Id_, false);
            }
            CurPlay_.clear();
        }

        CurPlay_.init();

        MpegLoader l(file, false);
        if (!l.open())
            return -1;

        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1) 
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }     

        CurPlay_.setBuffer(result, frequency, format);
        
        CurPlay_.Id_ = ++AlId;
        duration = CurPlay_.play();
        PttTimer_->start();
        return CurPlay_.Id_;
    }

    void SoundsManager::pausePtt(int id)
    {
        if (CurPlay_.Id_ == id)
            CurPlay_.pause();
    }

    void SoundsManager::delayDeviceTimer()
    {
        emit needUpdateDeviceTimer();
    }

    void SoundsManager::sourcePlay(unsigned source)
    {
        openal::alcDeviceResumeSOFT(AlAudioDevice_);
        openal::alSourcePlay(source);

        delayDeviceTimer();
    }

	void SoundsManager::callInProgress(bool value)
	{
		CallInProgress_ = value;
	}

    void SoundsManager::reinit()
    {
        if (AlInited_)
            shutdownOpenAl();
        initIncomig();
        initOutgoing();
		initMail();
    }

    void SoundsManager::initOpenAl()
    {
        AlAudioDevice_ = openal::alcOpenDevice(NULL);
        AlAudioContext_ = openal::alcCreateContext(AlAudioDevice_, NULL);
        openal::alcMakeContextCurrent(AlAudioContext_);

        openal::ALfloat v[] = { 0.f, 0.f, -1.f, 0.f, 1.f, 0.f };
        openal::alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
        openal::alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
        openal::alListenerfv(AL_ORIENTATION, v);

        openal::alDistanceModel(AL_NONE);

        if (openal::alGetError() == AL_NO_ERROR)
        {
            AlInited_ = true;
        }
    }
    
    void SoundsManager::initIncomig()
    {
        if (!AlInited_)
            initOpenAl();

        if (!Incoming_.isEmpty())
            return;
        
        Incoming_.init();
        
        MpegLoader l(build::is_icq() ? ":/sounds/incoming" : ":/sounds/incoming_agent", true);
        if (!l.open())
            return;
        
        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }
        
        Incoming_.setBuffer(result, frequency, format);
        
        int err;
        if ((err = openal::alGetError()) == AL_NO_ERROR)
        {
            Incoming_.Id_ = ++AlId;
        }
    }

    void SoundsManager::initMail()
    {
        if (!AlInited_)
            initOpenAl();

        if (!Mail_.isEmpty())
            return;

        Mail_.init();

        MpegLoader l(":/sounds/mail", true);
        if (!l.open())
            return;

        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }

        Mail_.setBuffer(result, frequency, format);

        int err;
        if ((err = openal::alGetError()) == AL_NO_ERROR)
        {
            Mail_.Id_ = ++AlId;
        }
    }
    
    void SoundsManager::initOutgoing()
    {
        if (!AlInited_)
            initOpenAl();

        if (!Outgoing_.isEmpty())
            return;
        
        Outgoing_.init();
        
        MpegLoader l(":/sounds/outgoing", true);
        if (!l.open())
            return;
        
        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }
        
        Outgoing_.setBuffer(result, frequency, format);
        
        int err;
        if ((err = openal::alGetError()) == AL_NO_ERROR)
        {
            Outgoing_.Id_ = ++AlId;
        }
    }

    void SoundsManager::shutdownOpenAl()
    {
        emit pttPaused(CurPlay_.Id_);
        emit pttFinished(CurPlay_.Id_, false);
        emit pttPaused(PrevPlay_.Id_);
        emit pttFinished(PrevPlay_.Id_, false);

        PrevPlay_.stop();
        PrevPlay_.free();
        PrevPlay_.clear();

        CurPlay_.stop();
        CurPlay_.free();
        CurPlay_.clear();

        Incoming_.stop();
        Incoming_.free();
        Incoming_.clear();

        Outgoing_.stop();
        Outgoing_.free();
        Outgoing_.clear();

		Mail_.stop();
		Mail_.free();
		Mail_.clear();

        if (AlAudioContext_)
        {
            openal::alcMakeContextCurrent(NULL);
            openal::alcDestroyContext(AlAudioContext_);
            AlAudioContext_ = 0;
        }

        if (AlAudioDevice_)
        {
            openal::alcCloseDevice(AlAudioDevice_);
            AlAudioDevice_ = 0;
        }

        AlInited_ = false;
    }

    std::unique_ptr<SoundsManager> g_sounds_manager;

    SoundsManager* GetSoundsManager()
    {
        if (!g_sounds_manager)
            g_sounds_manager.reset(new SoundsManager());

        return g_sounds_manager.get();
    }

    void ResetSoundsManager()
    {
        if (g_sounds_manager)
            g_sounds_manager.reset();
    }
}
