#pragma once

namespace Ui
{
    struct PlayingData
    {
        PlayingData()
            : Source_(0)
            , Buffer_(0)
            , Id_(-1)
        {
        }

        void init();
        void setBuffer(const QByteArray& data, qint64 freq, qint64 fmt);
        int play();
        void pause();
        void stop();
        void clear();
        void free();
        void clearData();
        bool isEmpty() const;
        int calcDuration();
        openal::ALenum state() const;

        openal::ALuint Source_;
        openal::ALuint Buffer_;
        int Id_;
    };

	class SoundsManager : public QObject
	{
		Q_OBJECT
Q_SIGNALS:
        void pttPaused(int);
        void pttFinished(int, bool);
        void needUpdateDeviceTimer();

	public:
		SoundsManager();
		~SoundsManager();

		void playIncomingMessage();
		void playOutgoingMessage();
        void playIncomingMail();

        int playPtt(const QString& file, int id, int& duration);
        void pausePtt(int id);

        void delayDeviceTimer();
        void sourcePlay(unsigned source);

		void callInProgress(bool value);

        void reinit();

	private Q_SLOTS:
		void timedOut();
        void checkPttState();
        void contactChanged(QString);
        void deviceTimeOut();

        void initOpenAl();
        void shutdownOpenAl();
        void initIncomig();
        void initMail();
        void initOutgoing();
        void updateDeviceTimer();

	private:
		bool CallInProgress_;
		bool CanPlayIncoming_;

        PlayingData Incoming_;
        PlayingData Outgoing_;
        PlayingData Mail_;

        int AlId;
        openal::ALCdevice *AlAudioDevice_;
        openal::ALCcontext *AlAudioContext_;
        PlayingData CurPlay_;
        PlayingData PrevPlay_;
        bool AlInited_;

        QTimer* Timer_;
        QTimer* PttTimer_;
        QTimer* DeviceTimer_;
	};

	SoundsManager* GetSoundsManager();
    void ResetSoundsManager();
}
