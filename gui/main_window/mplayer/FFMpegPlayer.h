#pragma once

#include "ffmpeg.h"

namespace Ui
{
    namespace audio
    {
        const int32_t num_buffers = 4;
        const int32_t outFrequency = 48000;
        const int32_t blockSize = 4096;
        const ffmpeg::AVSampleFormat outFormat = ffmpeg::AV_SAMPLE_FMT_S16;
        const int64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
        const qint32 outChannels = 2;
    }

    enum decode_thread_state
    {
        dts_none = 0,
        dts_playing = 1,
        dts_paused = 2,
        dts_end_of_media = 3,
        dts_seeking = 4,
        dts_failed = 5
    };

    //////////////////////////////////////////////////////////////////////////
    // ThreadMessage
    //////////////////////////////////////////////////////////////////////////
    enum thread_message_type
    {
        tmt_unknown = 0,
        tmt_update_scaled_size = 1,
        tmt_quit = 3,
        tmt_set_volume = 4,
        tmt_set_mute = 5,
        tmt_pause = 6,
        tmt_play = 7,
        tmt_get_next_video_frame = 8,
        tmt_seek_position = 9,
        tmt_stream_seeked = 10,
        tmt_set_finished = 11,
        tmt_init = 12,
        tmt_open_streams = 13,
        tmt_close_streams = 14,
        tmt_wake_up = 15

    };

    struct ThreadMessage
    {
        thread_message_type message_;

        int32_t x_;
        int32_t y_;
        uint32_t videoId_;
        QString str_;

        ThreadMessage(uint32_t _videoId = UINT32_MAX, const thread_message_type _message = thread_message_type::tmt_unknown)
            : message_(_message)
            , videoId_(_videoId)
            , x_(0)
            , y_(0)
        {
        }
    };


    //////////////////////////////////////////////////////////////////////////
    // ThreadMessagesQueue
    //////////////////////////////////////////////////////////////////////////
    class ThreadMessagesQueue
    {
        std::mutex queue_mutex_;

        QSemaphore condition_;

        std::list<ThreadMessage> messages_;

    public:

        bool getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _wait_timeout);
        void pushMessage(const ThreadMessage& _message, bool _forward, bool _clear_others);
        void clear();
    };


    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    class PacketQueue
    {
        QMutex mutex_;

        std::list<ffmpeg::AVPacket> list_;

        std::atomic<int32_t> packets_;
        std::atomic<int32_t> size_;

    public:

        PacketQueue();
        ~PacketQueue();

        void free(std::function<void(ffmpeg::AVPacket& _packet)> _callback);

        void push(ffmpeg::AVPacket* _packet);
        bool get(ffmpeg::AVPacket& _packet);

        int32_t getSize() const;
        int32_t getPackets() const;
    };


    //////////////////////////////////////////////////////////////////////////
    // DecodeAudioData
    //////////////////////////////////////////////////////////////////////////
    struct DecodeAudioData
    {
        ffmpeg::AVFrame* frame_;

        uint8_t** outSamplesData_;

        ffmpeg::SwrContext* swrContext_;

        openal::ALuint uiBuffers_[audio::num_buffers];
        openal::ALuint uiSource_;
        openal::ALuint uiBuffer_;
        openal::ALint buffersProcessed_;
        openal::ALint iState_;
        openal::ALint iQueuedBuffers_;

        int iloop_;

        uint64_t layout_;
        int32_t channels_;
        int32_t fmt_;
        int32_t sampleSize_;
        int32_t freq_;
        int32_t maxResampleSamples_;
        int32_t srcRate_;
        int32_t dstRate_;
        //ffmpeg::AVPacket packet_;

        bool queueInited_;
                
        ffmpeg::AVCodecContext* audioCodecContext_;
        decode_thread_state state_;

        DecodeAudioData()
            :   frame_(0),
                outSamplesData_(0),
                swrContext_(0),
                uiSource_(0),
                uiBuffer_(0),
                buffersProcessed_(0),
                iState_(0),
                iQueuedBuffers_(0),
                layout_(0),
                channels_(0),
                fmt_(AL_FORMAT_STEREO16),
                sampleSize_(2 * sizeof(uint16_t)),
                freq_(0),
                maxResampleSamples_(1024),
                srcRate_(audio::outFrequency),
                dstRate_(audio::outFrequency),
                audioCodecContext_(0),
                state_(decode_thread_state::dts_playing),
                queueInited_(false),
                iloop_(0)
        {

        }
    };

    struct MediaData 
    {
        bool syncWithAudio_;

        ffmpeg::AVStream* videoStream_;
        ffmpeg::AVStream* audioStream_;
        ffmpeg::AVFormatContext* formatContext_;
        ffmpeg::AVCodecContext* codecContext_;

        QSharedPointer<PacketQueue> videoQueue_;
        QSharedPointer<PacketQueue> audioQueue_;

        bool needUpdateSwsContext_;
        ffmpeg::SwsContext* swsContext_;
        std::vector<uint8_t> scaledBuffer_;
        ffmpeg::AVFrame* frameRGB_;
        DecodeAudioData audioData_;

        std::map<int32_t, QImage> frames_;

        int32_t width_;
        int32_t height_;
        int32_t rotation_;
        int64_t duration_;
        int64_t seek_position_;

        QSize scaledSize_;

        double frameTimer_;
        double frameLastPts_;
        double frameLastDelay_;

        bool startTimeVideoSet_;
        bool startTimeAudioSet_;
        int64_t startTimeVideo_;
        int64_t startTimeAudio_;

        double videoClock_;

        double audioClock_;
        int64_t audioClockTime_;

        bool mute_;
        int32_t volume_;

        bool audioQuitRecv_;
        bool videoQuitRecv_;
        bool demuxQuitRecv_;
        bool streamClosed_;
        bool isImage_;

        std::queue<double> audio_queue_ptss_;

        MediaData();
    };

    struct VideoData
    {
        decode_thread_state current_state_;
        bool eof_;
        bool stream_finished_;

        VideoData() : eof_(false), stream_finished_(false), current_state_(dts_none) {}
    };

    struct AudioData
    {
        bool eof_;
        bool stream_finished_;
        double offset_;
        int64_t last_sync_time_;

        AudioData() : eof_(false), stream_finished_(false), offset_(0.0), last_sync_time_(0) {}
    };


    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    class VideoContext : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:

        void dataReady(uint32_t _videoId);

        void nextframeReady(uint32_t _videoId, QImage _frame, double _pts, bool _eof);
        void videoSizeChanged(QSize _sz);
        void streamsOpened(uint32_t _videoId);
        void streamsOpenFailed(uint32_t _videoId);
        void audioTime(uint32_t _videoId, int64_t _avtime, double _offset);

        void seekedV(uint32_t _videoId);
        void seekedA(uint32_t _videoId);

    public Q_SLOTS:
            
        void audioQuit(uint32_t _videoId);
        void videoQuit(uint32_t _videoId);
        void demuxQuit(uint32_t _videoId);
        void streamsClosed(uint32_t _videoId);

    private:

        bool quit_;
        uint32_t curr_id_;

        mutable std::unordered_map<uint32_t, std::shared_ptr<MediaData>> mediaData_;
        mutable std::mutex mediaDataMutex_;

        mutable std::unordered_map<uint32_t, bool> activeVideos_;
        mutable std::mutex activeVideosMutex_;

        ThreadMessagesQueue videoThreadMessagesQueue_;
        ThreadMessagesQueue demuxThreadMessageQueue_;
        ThreadMessagesQueue audioThreadMessageQueue_;

    private:

        ffmpeg::AVStream* openStream(int32_t _type, ffmpeg::AVFormatContext* _context);
        void closeStream(ffmpeg::AVStream* _stream);
        void SendCloseStreams(uint32_t _videoId);

    public:

        VideoContext();

        void init(MediaData& _media);
        uint32_t addVideo(uint32_t id = 0);
        void deleteVideo(uint32_t _videoId);
        uint32_t reserveId();

        int32_t getVideoStreamIndex(MediaData& _media) const;
        int32_t getAudioStreamIndex(MediaData& _media) const;

        int32_t readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t readAVPacketPause(MediaData& media);

        int32_t readAVPacketPlay(MediaData& _media);
        bool isEof(int32_t _error, MediaData& _media);
        bool isStreamError(MediaData& _media);

        bool getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, VideoData& _videoData, MediaData& _media, uint32_t _videoId);

        void pushVideoPacket(ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t getVideoQueuePackets(MediaData& _media) const;
        int32_t getVideoQueueSize(MediaData& _media) const;
        void pushAudioPacket(ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t getAudioQueuePackets(MediaData& _media) const;
        int32_t getAudioQueueSize(MediaData& _media) const;

        bool isQuit() const;
        void setQuit(bool _val = true);

        bool isVideoQuit(int _videoId) const;
        void setVideoQuit(int _videoId);

        bool openStreams(uint32_t _mediaId, const QString& _file, MediaData& _media);
        bool openFile(MediaData& _media);
        void closeFile(MediaData& _media);

        int32_t getWidth(MediaData& _media) const;
        int32_t getHeight(MediaData& _media) const;
        int32_t getRotation(MediaData& _media) const;
        int64_t getDuration(MediaData& _media) const;

        QSize getScaledSize(MediaData& _media) const;
        bool enableAudio(MediaData& _media) const;
        bool enableVideo(MediaData& _media) const;
        
        double getVideoTimebase(MediaData& _media);
        double getAudioTimebase(MediaData& _media);
        double synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts, MediaData& _media);
        double computeDelay(double _picturePts, MediaData& _media);

        bool initDecodeAudioData(MediaData& _media);
        void freeDecodeAudioData(MediaData& _media);

        bool readFrameAudio(
            ffmpeg::AVPacket* _packet, 
            AudioData& _audioData, 
            openal::ALvoid** _frameData, 
            openal::ALsizei& _frameDataSize,
            bool& _flush,
            int& _seekCount,
            MediaData& _media, 
            uint32_t _videoId);
        
        bool playNextAudioFrame(
            ffmpeg::AVPacket* _packet, 
            /*in out*/ AudioData& _audioData, 
            bool& _flush, 
            int& _seekCount,
            MediaData& _media, 
            uint32_t _videoId);

        void cleanupAudioBuffers(MediaData& _media);
        void suspendAudio(MediaData& _media);
        void stopAudio(MediaData& _media);

        void updateScaledVideoSize(uint32_t _videoId, const QSize& _sz);

        void postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);
        bool getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);
        bool getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);
        bool getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void clearMessageQueue();

        decode_thread_state getAudioThreadState(MediaData& _media) const;
        void setAudioThreadState(const decode_thread_state& _state, MediaData& _media);

        bool updateScaleContext(MediaData& _media, const QSize _sz);
        void freeScaleContext(MediaData& _media);

        void setVolume(int32_t _volume, MediaData& _media);
        void setMute(bool _mute, MediaData& _media);

        void resetFrameTimer(MediaData& _media);
        void resetLsatFramePts(MediaData& _media);
        bool seekMs(uint32_t _videoId, int _tsms, MediaData& _media);
        bool seekFrame(uint32_t _videoId, int64_t _ts_video, int64_t _ts_audio, MediaData& _media);

        void flushVideoBuffers(MediaData& _media);
        void flushAudioBuffers(MediaData& _media);

        void resetVideoClock(MediaData& _media);
        void resetAudioClock(MediaData& _media);

        void setStartTimeVideo(const int64_t& _startTime, MediaData& _media);
        const int64_t& getStartTimeVideo(MediaData& _media) const;

        void setStartTimeAudio(const int64_t& _startTime, MediaData& _media);
        
        const int64_t& getStartTimeAudio(MediaData& _media) const;

        std::shared_ptr<MediaData> getMediaData(uint32_t _videoId, bool& _success);

        void initFlushPacket(ffmpeg::AVPacket& _packet);
    };


    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////
    class DemuxThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        DemuxThread(VideoContext& _ctx);
    };


    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class VideoDecodeThread : public QThread
    {

        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        VideoDecodeThread(VideoContext& _ctx);

        void prepareCtx(MediaData& _media);
    };



    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class AudioDecodeThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        AudioDecodeThread(VideoContext& _ctx);
    };

    class FrameRenderer
    {
        QPixmap activeImage_;
        QColor fillColor_;

        std::function<void(const QSize _sz)> sizeCallback_;

    protected:

        QPainterPath clippingPath_;
        bool fullScreen_;
        bool fillClient_;

    protected:

        void renderFrame(QPainter& _painter, const QRect& _clientRect);

        void onSize(const QSize _sz);

    public:

        void updateFrame(QPixmap _image);
        QPixmap getActiveImage() const;

        bool isActiveImageNull() const;

        virtual QWidget* getWidget() = 0;

        virtual void redraw() = 0;

        virtual void filterEvents(QWidget* _parent) = 0;

        void setClippingPath(QPainterPath _clippingPath);

        void setFullScreen(bool _fullScreen);

        void setFillColor(const QColor& _color);

        void setFillClient(bool _fill);

        virtual void setWidgetVisible(bool _visible) = 0;

        void setSizeCallback(std::function<void(const QSize)> _callback);

        FrameRenderer()
            : fullScreen_(false)
            , fillClient_(false)
        {
        }
    };

    class GDIRenderer : public QWidget, public FrameRenderer
    {
        virtual QWidget* getWidget() override;

        virtual void redraw() override;

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual void filterEvents(QWidget* _parent) override;

        virtual void setWidgetVisible(bool _visible) override;

        virtual void resizeEvent(QResizeEvent *_event) override;

    public:

        GDIRenderer(QWidget* _parent);
    };

#ifndef __linux__
    class OpenGLRenderer : public QOpenGLWidget, public FrameRenderer
    {
        virtual QWidget* getWidget() override;

        virtual void redraw() override;

        void paint();

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual void paintGL() override;

        virtual void filterEvents(QWidget* _parent) override;

        virtual void setWidgetVisible(bool _visible) override;

        virtual void resizeEvent(QResizeEvent *_event) override;

    public:
        OpenGLRenderer(QWidget* _parent);

    };
#endif //__linux__

    class MediaContainer
    {
    public:
        MediaContainer();
        ~MediaContainer();

        void VideoDecodeThreadStart(uint32_t _mediaId);
        void AudioDecodeThreadStart(uint32_t _mediaId);
        void DemuxThreadStart(uint32_t _mediaId);

        int32_t getWidth(MediaData& _media) const;
        int32_t getHeight(MediaData& _media) const;
        int32_t getRotation(MediaData& _media) const;
        int64_t getDuration(MediaData& _media) const;

        void postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);
        void postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);
        void postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others = false);

        void clearMessageQueue();

        void resetFrameTimer(MediaData& _media);
        void resetLastFramePts(MediaData& _media);

        bool isQuit(uint32_t _mediaId) const;
        void setQuit(bool _val = true);

        bool openFile(MediaData& _media);
        void closeFile(MediaData& _media);

        double computeDelay(double _picturePts, MediaData& _media);

        uint32_t init(uint32_t _id = 0);
        
        void stopMedia(uint32_t _mediaId);
        void updateVideoScaleSize(const uint32_t _mediaId, const QSize _sz);

        VideoContext ctx_;

        bool is_decods_inited_;

    private:

        bool is_demux_inited_;
        std::unordered_set<uint32_t> active_video_ids_;

        DemuxThread demuxThread_;
        VideoDecodeThread videoDecodeThread_;
        AudioDecodeThread audioDecodeThread_;

        void DemuxThreadWait();
        void VideoDecodeThreadWait();
        void AudioDecodeThreadWait();
    };

    MediaContainer* getMediaContainer();
    
    void ResetMediaContainer();

    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    class FFMpegPlayer : public QWidget
    {
        Q_OBJECT

        uint32_t mediaId_;

        int32_t restoreVolume_;
        int32_t volume_;

        std::list<uint32_t> queuedMedia_;

        QTimer* timer_;

        FrameRenderer* active_renderer_;
        FrameRenderer* gdi_renderer_;
        FrameRenderer* opengl_renderer_;

        QHBoxLayout* layout_;

        bool isFirstFrame_;

        int updatePositonRate_;

        struct DecodedFrame
        {
            QPixmap image_;

            double pts_;

            bool eof_;

            DecodedFrame(const QPixmap& _image, const double _pts) : image_(_image), pts_(_pts), eof_(false) {}
            DecodedFrame(const bool& _eof) : eof_(_eof) {}
        };

        std::unique_ptr<DecodedFrame> firstFrame_;

        std::list<DecodedFrame> decodedFrames_;

        double computeDelay();

        decode_thread_state state_;

        qint64 lastVideoPosition_;
        qint64 lastPostedPosition_;

        void makeObject();

        std::chrono::system_clock::time_point lastEmitMouseMove_;

        QMetaObject::Connection openStreamsConnection_;

        bool stoped_;
        bool started_;
        bool continius_;
        bool replay_;
        bool mute_;
        bool dataReady_;
        bool pausedByUser_;

        int imageDuration_;
        int imageProgress_;

        int seek_request_id_;

        QPropertyAnimation* imageProgressAnimation_;

    private:

        void updateVideoPosition(const DecodedFrame& _frame);
        bool canPause() const;

        FrameRenderer* CreateRenderer(QWidget* _parent, bool _openGL);

    Q_SIGNALS:

        void durationChanged(qint64 _duration);
        void positionChanged(qint64 _position);
        void mediaFinished();
        void mouseMoved();
        void mouseLeaveEvent();
        void fileLoaded();
        void mediaChanged(qint32);
        void dataReady();
        void streamsOpenFailed(uint32_t _videoId);
        void paused();
        void played();
        void firstFrameReady();

    private Q_SLOTS:

        void onTimer();


        void onStreamsOpened(uint32_t _videoId);
        void onRendererSize(const QSize _sz);
        void onAudioTime(uint32_t _videoId, int64_t _avtime, double _offset);
        void onDataReady(uint32_t _videoId);
        void onNextFrameReady(uint32_t _videoId, QImage _image, double _pts, bool _eof);
        void seekedV(uint32_t _videoId);
        void seekedA(uint32_t _videoId);

    public Q_SLOTS:

        uint32_t stop();

    public:

        FFMpegPlayer(QWidget* _parent, bool _openGL, bool _continius = false);
        virtual ~FFMpegPlayer();

        bool openMedia(const QString& _mediaPath, bool isImage = false, uint32_t id = 0);

        void play(bool _init);
        void pause();

        void setPaused(const bool _paused);
        void setPausedByUser(const bool _paused);
        bool isPausedByUser() const;

        void setPosition(int64_t _position);
        void resetPosition();
        void setUpdatePositionRate(int _rate);

        void setVolume(int32_t _volume);
        int32_t getVolume() const;

        void setMute(bool _mute);
        bool isMute() const;

        QSize getVideoSize() const;
        int32_t getVideoRotation() const;
        int64_t getDuration() const;

        QMovie::MovieState state() const;

        void setClippingPath(QPainterPath _clippingPath);

        void setFillColor(const QColor& _color);

        void setFillClient(bool _fill);

        void setFullScreen(QWidget* _parent, QLayout* _attach_to);
        void setNormal();

        void setPreview(QPixmap _preview);
        QPixmap getActiveImage() const;

        bool getStarted() const;
        void setStarted(bool _started);

        void loadFromQueue();
        void removeFromQueue(uint32_t _media);
        void clearQueue();
        bool queueIsEmpty() const;
        int queueSize() const;

        void setImageDuration(int duration);

        Q_PROPERTY(int imageProgress READ getImageProgress WRITE setImageProgress)

        void setImageProgress(int _val);
        int getImageProgress() const;

        uint32_t getLastMedia() const;
        uint32_t getMedia() const;

        uint32_t reserveId();

        void setReplay(bool _replay);

        void setRestoreVolume(const int32_t _volume);
        int32_t getRestoreVolume() const;

        void resetRenderer();

    protected:

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;

        void seeked(uint32_t _videoId, const bool _fromAudio);
    };

}
