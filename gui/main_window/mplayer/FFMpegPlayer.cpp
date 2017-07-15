#include "stdafx.h"
#include "FFMpegPlayer.h"
#include "../../utils/utils.h"
#include "../sounds/SoundsManager.h"

#include "../../utils/PainterPath.h"
#include "../history_control/MessageStyle.h"

#include "../sounds/SoundsManager.h"

#ifdef __WIN32__
#include "win32/WindowRenderer.h"
#endif //__WIN32__

namespace Ui
{
    const int32_t maxQueueSize(1024*1024*15);
    const int32_t minFramesCount(25);

    static ffmpeg::AVPacket flush_pkt_data;

    static ffmpeg::AVPacket quit_pkt;

    const int max_video_w = 1280;
    const int max_video_h = 720;

    bool ThreadMessagesQueue::getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _wait_timeout)
    { 
        condition_.tryAcquire(1, _wait_timeout);

        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (_isQuit() || messages_.empty())
        {
            return false;
        }

        _message = messages_.front();

        messages_.pop_front();

        return true;
    }

    void ThreadMessagesQueue::pushMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            if (_clear_others)
            {
                for (std::list<ThreadMessage>::iterator iter = messages_.begin(); iter != messages_.end();)
                {
                    if (iter->videoId_ == _message.videoId_)
                        iter = messages_.erase(iter);
                    else
                        ++iter;
                }
            }

            if (_forward)
                messages_.push_front(_message);
            else
                messages_.push_back(_message);
        }

        condition_.release(1);
    }

    void ThreadMessagesQueue::clear()
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        messages_.clear();
    }


    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    PacketQueue::PacketQueue()
        :   packets_(0),
            size_(0)
    {
    }

    PacketQueue::~PacketQueue()
    {
        free([](ffmpeg::AVPacket& _packet){});
    }

    void PacketQueue::free(std::function<void(ffmpeg::AVPacket& _packet)> _callback)
    {
        QMutexLocker locker(&mutex_);

        for (auto iter = list_.begin(); iter != list_.end(); ++iter)
        {
            _callback(*iter);

            if (iter->data && iter->data != (uint8_t*) &flush_pkt_data && iter->data != (uint8_t*) &quit_pkt)
            {
                ffmpeg::av_packet_unref(&(*iter));
            }
        }

        list_.clear();
        size_ = 0;
        packets_ = 0;
    }

    void PacketQueue::push(ffmpeg::AVPacket* _packet)
    {
        QMutexLocker locker(&mutex_);

        list_.push_back(*_packet);

        ++packets_;

        if (_packet->data != (uint8_t*) &flush_pkt_data)
        {
            size_ += _packet->size;
        }
    }

    bool PacketQueue::get(/*out*/ ffmpeg::AVPacket& _packet)
    {
        QMutexLocker locker(&mutex_);

        if (list_.empty())
        {
            return false;
        }

        _packet = list_.front();

        list_.pop_front();

        --packets_;
        size_ -= _packet.size;

        return true;
    }

    int32_t PacketQueue::getSize() const
    {
        return size_;
    }

    int32_t PacketQueue::getPackets() const
    {
        return packets_;
    }


    MediaData::MediaData()
        : videoStream_(0)
        , audioStream_(0)
        , formatContext_(0)
        , width_(0)
        , height_(0)
        , rotation_(0)
        , duration_(0)
        , frameTimer_(0.0)
        , frameLastPts_(0.0)
        , frameLastDelay_(0.0)
        , seek_position_(-1)
        , videoQueue_(new PacketQueue())
        , audioQueue_(new PacketQueue())
        , videoClock_(0.0)
        , audioClock_(0.0)
        , audioClockTime_(0)
        , startTimeVideo_(0)
        , startTimeAudio_(0)
        , startTimeVideoSet_(false)
        , startTimeAudioSet_(false)
        , frameRGB_(0)
        , swsContext_(0)
        , volume_(100)
        , mute_(true)
        , audioQuitRecv_(false)
        , videoQuitRecv_(false)
        , demuxQuitRecv_(false)
        , streamClosed_(false)
        , codecContext_(0)
        , needUpdateSwsContext_(false)
        , syncWithAudio_(false)
    {
    }


    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    VideoContext::VideoContext()
        : quit_(false)
        , curr_id_(0)
    {
    }

    void VideoContext::init(MediaData& _media)
    {
        ffmpeg::AVPacket flush_pkt_v, flush_pkt_a;
        initFlushPacket(flush_pkt_a);
        initFlushPacket(flush_pkt_v);

        pushVideoPacket(&flush_pkt_v, _media);
        pushAudioPacket(&flush_pkt_a, _media);
    }

    uint32_t VideoContext::addVideo(uint32_t _id)
    {
        {
            std::unique_lock<std::mutex> lock(mediaDataMutex_);
            if (_id == 0)
                mediaData_[++curr_id_] = std::make_shared<MediaData>();
            else
                mediaData_[_id] = std::make_shared<MediaData>();
        }

        {
            std::unique_lock<std::mutex> lock(activeVideosMutex_);
            if (_id == 0)
                activeVideos_[curr_id_] = true;
            else
                activeVideos_[_id] = true;
        }

        return _id == 0 ? curr_id_ : _id;
    }

    uint32_t VideoContext::reserveId()
    {
        return ++curr_id_;
    }

    void VideoContext::deleteVideo(uint32_t _videoId)
    {
        {
            std::unique_lock<std::mutex> lock(mediaDataMutex_);

            if (mediaData_.count(_videoId) > 0)
                mediaData_.erase(_videoId);
        }

        {
            std::unique_lock<std::mutex> lock(activeVideosMutex_);
            activeVideos_.erase(_videoId);
        }

        getMediaContainer()->stopMedia(_videoId);
    }

    ffmpeg::AVStream* VideoContext::openStream(int32_t _type, ffmpeg::AVFormatContext* _context)
    {
        ffmpeg::AVStream* stream = 0;

        assert(_context);

        int32_t streamIndex = ffmpeg::av_find_best_stream(_context, (ffmpeg::AVMediaType) _type, -1, -1, NULL, 0);
        if (streamIndex < 0)
        {
            return 0;
        }

        stream = _context->streams[streamIndex];

        ffmpeg::AVCodecContext* codecContext = stream->codec;

        // Find suitable codec
        ffmpeg::AVCodec* codec = ffmpeg::avcodec_find_decoder(codecContext->codec_id);

        if (!codec)
        {
            // Codec not found
            return 0;
        }

        if (avcodec_open2(codecContext, codec, 0) < 0)
        {
            // Failed to open codec
            return 0;
        }

        return stream;
    }

    void VideoContext::closeStream(ffmpeg::AVStream* _stream)
    {
        if (_stream && _stream->codec)
        {
            avcodec_close(_stream->codec);
        }
    }

    int32_t VideoContext::getVideoStreamIndex(MediaData& _media) const
    {
        return _media.videoStream_->index;
    }

    int32_t VideoContext::getAudioStreamIndex(MediaData& _media) const
    {
        if (_media.audioStream_)
            return _media.audioStream_->index;

        return -1;
    }

    bool VideoContext::isQuit() const
    {
        return quit_;
    }

    bool VideoContext::isVideoQuit(int _videoId) const
    {
        std::unique_lock<std::mutex> lock(activeVideosMutex_);

        if (activeVideos_.count(_videoId) == 0)
        {
            return false;
        }
        return !activeVideos_[_videoId];
    }

    void VideoContext::setVideoQuit(int _videoId)
    {
        std::unique_lock<std::mutex> lock(activeVideosMutex_);

        if (activeVideos_.count(_videoId) == 0)
        {
            return;
        }
        activeVideos_[_videoId] = false;;
    }

    void VideoContext::setQuit(bool _val)
    {
        quit_ = _val;
    }

    int32_t VideoContext::readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        return ffmpeg::av_read_frame(_media.formatContext_, _packet);
    }

    int32_t VideoContext::readAVPacketPause(MediaData& _media)
    {
        return ffmpeg::av_read_pause(_media.formatContext_);
    }

    int32_t VideoContext::readAVPacketPlay(MediaData& _media)
    {
        return ffmpeg::av_read_play(_media.formatContext_);
    }

    bool VideoContext::isEof(int32_t _error, MediaData& _media)
    {
        return (_error == AVERROR_EOF || ffmpeg::avio_feof(_media.formatContext_->pb));
    }

    bool VideoContext::isStreamError(MediaData& _media)
    {
        if (_media.formatContext_->pb && _media.formatContext_->pb->error)
        {
            assert(false);
            return true;
        }

        return false;
    }

    void VideoContext::initFlushPacket(ffmpeg::AVPacket& _packet)
    {
        ffmpeg::av_init_packet(&_packet);

        _packet.data = (uint8_t*) &flush_pkt_data;

        _packet.dts = -1;
        _packet.pts = -1;
        _packet.size = 0;
    }

    bool VideoContext::getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, VideoData& _videoData, MediaData& _media, uint32_t _videoId)
    {
        if (!_media.videoStream_)
            return false;

        ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;

        while (!isVideoQuit(_videoId))
        {
            if (!_videoData.stream_finished_)
            {
                if (!_media.videoQueue_->get(*_packet))
                {
                    continue;
                }
            }
            else
            {

            }

            if (_packet->data == (uint8_t*) &flush_pkt_data)
            {
                if (_packet->dts != -1)
                {
                    _media.seek_position_ = _packet->dts;

                    emit seekedV(_videoId);
                }

                flushVideoBuffers(_media);

                continue;
            }

            if (_packet->data == (uint8_t*) &quit_pkt)
            {
                return false;
            }

            if (!_packet->data)
            {
                _videoData.stream_finished_ = true;
            }
            
            int got_frame = 0;

            int len = ffmpeg::avcodec_decode_video2(videoCodecContext, _frame, &got_frame, _packet);

            if (_packet->data)
            {
                ffmpeg::av_packet_unref(_packet);
            }
            
            if (len < 0)
            {
                return false;
            }

            if (got_frame)
            {
                if (_media.seek_position_ > 0)
                {
                    if (_frame->pkt_dts < _media.seek_position_)
                    {
                        ffmpeg::av_frame_unref(_frame);
                        continue;
                    }
                }
                
                _media.seek_position_ = 0;

                return true;
            }
            else if (_videoData.stream_finished_)
            {
                _videoData.eof_ = true;

                return false;
            }
        }

        return false;
    }

    void pushNullPacket(PacketQueue& _queue, int32_t _streamIndex)
    {
        ffmpeg::AVPacket pkt1, *pkt = &pkt1;
        ffmpeg::av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
        pkt->stream_index = _streamIndex;

        _queue.push(pkt);
    }

    void VideoContext::pushVideoPacket(ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        if (!_packet)
            return pushNullPacket(*(_media.videoQueue_), getVideoStreamIndex(_media));
        
        return _media.videoQueue_->push(_packet);
    }

    int32_t VideoContext::getVideoQueuePackets(MediaData& _media) const
    {
        return _media.videoQueue_->getPackets();
    }

    int32_t VideoContext::getVideoQueueSize(MediaData& _media) const
    {
        return _media.videoQueue_->getSize();
    }

    void VideoContext::pushAudioPacket(ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        if (!_packet)
            return pushNullPacket((*_media.audioQueue_), getAudioStreamIndex(_media));

        return _media.audioQueue_->push(_packet);
    }

    int32_t VideoContext::getAudioQueuePackets(MediaData& _media) const
    {
        return _media.audioQueue_->getPackets();
    }

    int32_t VideoContext::getAudioQueueSize(MediaData& _media) const
    {
        return _media.audioQueue_->getSize();
    }

    bool VideoContext::openStreams(uint32_t _mediaId, const QString& _file, MediaData& _media)
    {
        //qDebug() << "openStreams " << _mediaId;
        int32_t err = 0;
        
        err = ffmpeg::avformat_open_input(&_media.formatContext_, _file.toStdString().c_str(), 0, 0);
        if (err < 0)
        {
            emit streamsOpenFailed(_mediaId);
            return false;
        }

        // Retrieve stream information
        err = ffmpeg::avformat_find_stream_info(_media.formatContext_, 0);
        if (err < 0)
        {
            emit streamsOpenFailed(_mediaId);
            return false;
        }

        // Open video and audio streams
        _media.videoStream_ = openStream(ffmpeg::AVMEDIA_TYPE_VIDEO, _media.formatContext_);

        if (!_media.videoStream_)
        {
            emit streamsOpenFailed(_mediaId);
            return false;
        }

        _media.codecContext_ = _media.videoStream_->codec;

        _media.audioStream_ = openStream(ffmpeg::AVMEDIA_TYPE_AUDIO, _media.formatContext_);

        emit streamsOpened(_mediaId);
        return true;
    }

    bool VideoContext::openFile(MediaData& _media)
    {
        // Create conversion context
        ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;

        ffmpeg::AVDictionary* dictionary = _media.videoStream_->metadata;

        if (dictionary)
        {
            ffmpeg::AVDictionaryEntry* entryRotate = ffmpeg::av_dict_get(dictionary, "rotate", 0, AV_DICT_IGNORE_SUFFIX);

            if (entryRotate && entryRotate->value && entryRotate->value[0] != '\0')
            {
                _media.rotation_ = QString(entryRotate->value).toInt();
            }
        }

        _media.width_ = videoCodecContext->width;
        _media.height_ = videoCodecContext->height;

        _media.duration_ = _media.formatContext_->duration / (AV_TIME_BASE / 1000);

        _media.scaledSize_.setHeight(_media.height_);
        _media.scaledSize_.setWidth(_media.width_);

        resetFrameTimer(_media);

        return true;
    }

    void VideoContext::closeFile(MediaData& _media)
    {
        closeStream(_media.audioStream_);

        closeStream(_media.videoStream_);

        if (_media.formatContext_)
        {
            avformat_close_input(&_media.formatContext_);
        }
    }

    int32_t VideoContext::getWidth(MediaData& _media) const
    {
        return _media.width_;
    }

    int32_t VideoContext::getHeight(MediaData& _media) const
    {
        return _media.height_;
    }

    int32_t VideoContext::getRotation(MediaData& _media) const
    {
        return _media.rotation_;
    }

    int64_t VideoContext::getDuration(MediaData& _media) const
    {
        return _media.duration_;
    }

    QSize VideoContext::getScaledSize(MediaData& _media) const
    {
        return _media.scaledSize_;
    }

    double VideoContext::getVideoTimebase(MediaData& _media)
    {
        return ffmpeg::av_q2d(_media.videoStream_->time_base);
    }

    double VideoContext::getAudioTimebase(MediaData& _media)
    {
        return ffmpeg::av_q2d(_media.audioStream_->time_base);
    }

    double VideoContext::synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts, MediaData& _media)
    {
        ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;
        if (_pts >  std::numeric_limits<double>::epsilon())
        {
            /* if we have pts, set video clock to it */
            _media.videoClock_ = _pts;
        }
        else
        {
            /* if we aren't given a pts, set it to the clock */
            _pts = _media.videoClock_;
        }
        /* update the video clock */
        double frameDelay = ffmpeg::av_q2d(videoCodecContext->time_base);
        /* if we are repeating a frame, adjust clock accordingly */

        frameDelay += _frame->repeat_pict * (frameDelay * 0.5);
        _media.videoClock_ += frameDelay;

        return _pts;
    }

    double VideoContext::computeDelay(double _picturePts, MediaData& _media)
    {
        double delay = _picturePts - _media.frameLastPts_;

        if (delay <= 0.0 || delay >= 10.0 || _media.frameLastPts_ <  std::numeric_limits<double>::epsilon())
        {
            // Delay incorrect - use previous one
            delay = _media.frameLastDelay_;
        }

        // Save for next time
        _media.frameLastPts_ = _picturePts;
        _media.frameLastDelay_ = delay;

        // Update delay to sync to audio
        /*double ref_clock = get_audio_clock(main_context);
        double diff = main_context->pict.pts - ref_clock;
        double sync_threshold = FFMAX(AV_SYNC_THRESHOLD, delay);
        if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }*/

        _media.frameTimer_ += delay;

        double actual_delay = _media.frameTimer_ - (ffmpeg::av_gettime() / 1000000.0);

        int64_t current_time = ffmpeg::av_gettime();

        if (_media.syncWithAudio_)
        {
            int64_t timeDiff = current_time - _media.audioClockTime_;
            double timeDiff_d = ((double) timeDiff) / (double) 1000000.0;
            double pts_audio = _media.audioClock_ + timeDiff_d;

            double diff = _picturePts - pts_audio;

            if (fabs(diff) > 0.04)
            {
                actual_delay += diff;
            }
        }

        if (actual_delay < 0.010)
        {
            actual_delay = 0.010;
        }

        return actual_delay;
    }

    bool VideoContext::initDecodeAudioData(MediaData& _media)
    {
        if (!enableAudio(_media))
            return true;

        _media.audioData_.frame_ = ffmpeg::av_frame_alloc();

        // Generate some AL Buffers for streaming
        openal::alGenBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);

        // Generate a Source to playback the Buffers
        openal::alGenSources(1, &_media.audioData_.uiSource_);
        
        _media.audioData_.audioCodecContext_ = _media.audioStream_->codec;

        _media.audioData_.layout_ = _media.audioData_.audioCodecContext_->channel_layout;
        _media.audioData_.channels_ = _media.audioData_.audioCodecContext_->channels;
        _media.audioData_.freq_ = _media.audioData_.audioCodecContext_->sample_rate;

        if (_media.audioData_.layout_ == 0 && _media.audioData_.channels_ > 0)
        {
            if (_media.audioData_.channels_ == 1)
                _media.audioData_.layout_ = AV_CH_LAYOUT_MONO;
            else
                _media.audioData_.layout_ = AV_CH_LAYOUT_STEREO;
        }

        ffmpeg::AVSampleFormat inputFormat = _media.audioData_.audioCodecContext_->sample_fmt;

        switch (_media.audioData_.layout_)
        {
        case AV_CH_LAYOUT_MONO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
            case ffmpeg::AV_SAMPLE_FMT_U8P:
                _media.audioData_.fmt_ = AL_FORMAT_MONO8;
                _media.audioData_.sampleSize_ = 1;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
            case ffmpeg::AV_SAMPLE_FMT_S16P:
                _media.audioData_.fmt_ = AL_FORMAT_MONO16;
                _media.audioData_.sampleSize_ = sizeof(uint16_t);
                break;

            default:
                _media.audioData_.sampleSize_ = -1;
                break;
            }
            break;

        case AV_CH_LAYOUT_STEREO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
                _media.audioData_.fmt_ = AL_FORMAT_STEREO8;
                _media.audioData_.sampleSize_ = 2;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
                _media.audioData_.fmt_ = AL_FORMAT_STEREO16;
                _media.audioData_.sampleSize_ = 2 * sizeof(uint16_t);
                break;

            default:
                _media.audioData_.sampleSize_ = -1;
                break;
            }
            break;

        default:
            _media.audioData_.sampleSize_ = -1;
            break;
        }

        if (_media.audioData_.freq_ != 44100 && _media.audioData_.freq_ != 48000)
            _media.audioData_.sampleSize_ = -1;

        if (_media.audioData_.sampleSize_ < 0) 
        {
            _media.audioData_.swrContext_ = ffmpeg::swr_alloc();
            if (!_media.audioData_.swrContext_)
                return false;

            int64_t src_ch_layout = _media.audioData_.layout_, dst_ch_layout = audio::outChannelLayout;
            _media.audioData_.srcRate_ = _media.audioData_.freq_;
            ffmpeg::AVSampleFormat src_sample_fmt = inputFormat, dst_sample_fmt = audio::outFormat;
            _media.audioData_.dstRate_ = (_media.audioData_.freq_ != 44100 && _media.audioData_.freq_ != 48000) ? audio::outFrequency : _media.audioData_.freq_;

            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "in_channel_layout", src_ch_layout, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "in_sample_rate", _media.audioData_.srcRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(_media.audioData_.swrContext_, "in_sample_fmt", src_sample_fmt, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "out_channel_layout", dst_ch_layout, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "out_sample_rate", _media.audioData_.dstRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(_media.audioData_.swrContext_, "out_sample_fmt", dst_sample_fmt, 0);

            if (swr_init(_media.audioData_.swrContext_) < 0)
                return false;

            _media.audioData_.sampleSize_ = audio::outChannels * sizeof(uint16_t);
            _media.audioData_.freq_ = _media.audioData_.dstRate_;
            //!!!int64_t len_ = ffmpeg::av_rescale_rnd(Len_, DstRate_, SrcRate_, AV_ROUND_UP);
            _media.audioData_.fmt_ = AL_FORMAT_STEREO16;

            _media.audioData_.maxResampleSamples_ = ffmpeg::av_rescale_rnd(audio::blockSize / _media.audioData_.sampleSize_, _media.audioData_.dstRate_, _media.audioData_.srcRate_, ffmpeg::AV_ROUND_UP);
            if (ffmpeg::av_samples_alloc_array_and_samples(&_media.audioData_.outSamplesData_, 0, audio::outChannels, _media.audioData_.maxResampleSamples_, audio::outFormat, 0) < 0)
                return false;
        }

        return true;
    }

    void VideoContext::freeDecodeAudioData(MediaData& _media)
    {
        if (_media.audioData_.frame_)
            ffmpeg::av_frame_free(&_media.audioData_.frame_);

        if (_media.audioData_.swrContext_)
            ffmpeg::swr_free(&_media.audioData_.swrContext_);

        openal::alDeleteSources(1, &_media.audioData_.uiSource_);
        openal::alDeleteBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);
    }

    bool VideoContext::readFrameAudio(
        ffmpeg::AVPacket* _packet, 
        AudioData& _audioData,
        openal::ALvoid** _frameData,
        openal::ALsizei& _frameDataSize,
        bool& _flush,
        int& _seekCount,
        MediaData& _media,
        uint32_t _videoId)
    {
        int64_t seek_position_audio = -1;

        for (;;)
        {
            if (isVideoQuit(_videoId))
                return false;

            if (!_media.audioQueue_->get(*_packet))
            {
                continue;
            }

            if (!_packet->data)
            {
                _audioData.stream_finished_ = true;
            }
            else if (_packet->data == (uint8_t*) &quit_pkt)
            {
                return false;
            }
            else if (_packet->data == (uint8_t*) &flush_pkt_data)
            {
                _flush = true;

                //qDebug() << "Audio thread: start seek";

                flushAudioBuffers(_media);

                if (_packet->pts != -1)
                {
                    seek_position_audio = _packet->pts;

                    ++_seekCount;
                }

                continue;
            }

            int got_frame = 0;

            if (!_media.audioData_.audioCodecContext_)
                return false;

            int len = avcodec_decode_audio4(_media.audioData_.audioCodecContext_, _media.audioData_.frame_, &got_frame, _packet);

            if (_packet->data)
            {
                ffmpeg::av_packet_unref(_packet);
            }

            if (len < 0)
                return false;

            if (!got_frame)
            {
                if (_audioData.stream_finished_)
                {
                    _audioData.eof_ = true;

                    return false;
                }

                continue;
            }
            else
            {
                setStartTimeAudio(_media.audioData_.frame_->pkt_dts, _media);

                if (seek_position_audio > 0 && _media.audioData_.frame_->pkt_dts < seek_position_audio)
                {
                    ffmpeg::av_frame_unref(_media.audioData_.frame_);
 
                    continue;
                }
 
                 seek_position_audio = 0;
            }

            if (_media.audioData_.outSamplesData_) 
            {
                int64_t delay = ffmpeg::swr_get_delay(_media.audioData_.swrContext_, _media.audioData_.srcRate_);

                int64_t dstSamples = ffmpeg::av_rescale_rnd(delay + _media.audioData_.frame_->nb_samples, _media.audioData_.dstRate_, _media.audioData_.srcRate_, ffmpeg::AV_ROUND_UP);

                if (dstSamples > _media.audioData_.maxResampleSamples_) 
                {
                    _media.audioData_.maxResampleSamples_ = dstSamples;
                    ffmpeg::av_free(_media.audioData_.outSamplesData_[0]);

                    if (ffmpeg::av_samples_alloc(_media.audioData_.outSamplesData_, 0, audio::outChannels, _media.audioData_.maxResampleSamples_, audio::outFormat, 1) < 0) 
                    {
                        _media.audioData_.outSamplesData_[0] = 0;

                        return false;
                    }
                }

                int32_t res = 0;
                if ((res = ffmpeg::swr_convert(_media.audioData_.swrContext_, _media.audioData_.outSamplesData_, dstSamples, (const uint8_t**) _media.audioData_.frame_->extended_data, _media.audioData_.frame_->nb_samples)) < 0) 
                    return false;

                qint32 resultLen = ffmpeg::av_samples_get_buffer_size(0, audio::outChannels, res, audio::outFormat, 1);

                *_frameData = _media.audioData_.outSamplesData_[0];
                _frameDataSize = resultLen;
            } 
            else 
            {
                *_frameData = _media.audioData_.frame_->extended_data[0];
                _frameDataSize = _media.audioData_.frame_->nb_samples * _media.audioData_.sampleSize_;
            }

            return true;
        }
    }

    bool VideoContext::playNextAudioFrame(
        ffmpeg::AVPacket* _packet, 
        /*in out*/ AudioData& _audioData, 
        bool& _flush, 
        int& _seekCount, 
        MediaData& _media, 
        uint32_t _videoId)
    {
        openal::ALint iBuffersProcessed = 0;

        openal::ALvoid* frameData = 0;
        openal::ALsizei frameDataSize = 0;

        // set volume
        openal::ALfloat volume = (_media.mute_ ? (0.0) : (float(_media.volume_) / 100.0));
        openal::alSourcef(_media.audioData_.uiSource_, AL_GAIN, volume);

        openal::ALint iState = 0;
        openal::ALint iQueuedBuffers = 0;

        auto do_flush = [this, &_media]()
        {
            openal::alSourceStop(_media.audioData_.uiSource_);

            openal::alDeleteSources(1, &_media.audioData_.uiSource_);
            openal::alDeleteBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);

            openal::alGenBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);
            openal::alGenSources(1, &_media.audioData_.uiSource_);

            _media.audioData_.queueInited_ = false;

            std::queue<double> q_tmp;
            _media.audio_queue_ptss_.swap(q_tmp);
        };

        if (!_media.audioData_.queueInited_)
        {
            for (int i = 0; i < audio::num_buffers; ++i)
            {
                _flush = false;
                if (!readFrameAudio(_packet, _audioData, &frameData, frameDataSize, _flush, _seekCount, _media, _videoId))
                    return false;

                if (_flush)
                {
                    do_flush();
                    return true;
                }

                openal::alBufferData(_media.audioData_.uiBuffers_[i], _media.audioData_.fmt_, frameData, frameDataSize, _media.audioData_.freq_);
                openal::alSourceQueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffers_[i]);

                _media.audio_queue_ptss_.push(_media.audioData_.frame_->pkt_pts);
            }

            _media.audioData_.queueInited_ = true;
        }

        openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

        while (iBuffersProcessed)
        {
            _media.audioData_.uiBuffer_ = 0;
            openal::alSourceUnqueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            _flush = false;
            if (!readFrameAudio(_packet, _audioData, &frameData, frameDataSize, _flush, _seekCount, _media, _videoId))
                return false;

            if (_flush)
            {
                do_flush();
                return true;
            }

            openal::alBufferData(_media.audioData_.uiBuffer_, _media.audioData_.fmt_, frameData, frameDataSize, _media.audioData_.freq_);
            openal::alSourceQueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            _media.audio_queue_ptss_.push(_media.audioData_.frame_->pkt_pts);

            iBuffersProcessed--;

            _media.audio_queue_ptss_.pop();
        }

        openal::ALfloat offset = 0;
        openal::alGetSourcef(_media.audioData_.uiSource_, AL_SEC_OFFSET, &offset);

        if (!_media.audio_queue_ptss_.empty())
        {
            double pts = _media.audio_queue_ptss_.front();
            if (pts == AV_NOPTS_VALUE)
            {
                pts = 0.0;
            }

            pts *= getAudioTimebase(_media);

            pts += offset;

            _audioData.offset_ = pts;
        }

        openal::alGetSourcei(_media.audioData_.uiSource_, AL_SOURCE_STATE, &iState);

        const auto time_now = std::chrono::system_clock::now();

        static auto lastDelayTime = time_now;

        if ((time_now - lastDelayTime) > std::chrono::milliseconds(1000))
        {
            lastDelayTime = time_now;

            Ui::GetSoundsManager()->delayDeviceTimer();
        }

        if (iState != AL_PLAYING)
        {
            // If there are Buffers in the Source Queue then the Source was starved of audio
            // data, so needs to be restarted (because there is more audio data to play)

            openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_QUEUED, &iQueuedBuffers);
            if (iQueuedBuffers)
            {
                Ui::GetSoundsManager()->sourcePlay(_media.audioData_.uiSource_);
            }
            else
            {
                // Finished playing
                return true;
            }
        }

        return true;
    }

    void VideoContext::cleanupAudioBuffers(MediaData& _media)
    {
        _media.audioData_.buffersProcessed_ = 0;
        openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_PROCESSED, &_media.audioData_.buffersProcessed_);

        while (_media.audioData_.buffersProcessed_)
        {
            _media.audioData_.uiBuffer_ = 0;
            openal::alSourceUnqueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            --_media.audioData_.buffersProcessed_;
        }
    }

    void VideoContext::suspendAudio(MediaData& _media)
    {
        openal::alSourcePause(_media.audioData_.uiSource_);
    }

    void VideoContext::stopAudio(MediaData& _media)
    {
        openal::alSourceStop(_media.audioData_.uiSource_);
    }

    void VideoContext::updateScaledVideoSize(uint32_t _videoId, const QSize& _sz)
    {
        ThreadMessage msg(_videoId, thread_message_type::tmt_update_scaled_size);

        msg.x_ = _sz.width();
        msg.y_ = _sz.height();

        postVideoThreadMessage(msg, false);
    }

    void VideoContext::postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        videoThreadMessagesQueue_.pushMessage(_message, _forward, _clear_others);
    }

    void VideoContext::postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        demuxThreadMessageQueue_.pushMessage(_message, _forward, _clear_others);
    }

    bool VideoContext::getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return demuxThreadMessageQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    void VideoContext::postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        audioThreadMessageQueue_.pushMessage(_message, _forward, _clear_others);
    }

    bool VideoContext::getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return audioThreadMessageQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    void VideoContext::clearMessageQueue()
    {
        videoThreadMessagesQueue_.clear();
        audioThreadMessageQueue_.clear();
        demuxThreadMessageQueue_.clear();
    }

    decode_thread_state VideoContext::getAudioThreadState(MediaData& _media) const
    {
        return _media.audioData_.state_;
    }

    void VideoContext::setAudioThreadState(const decode_thread_state& _state, MediaData& _media)
    {
        _media.audioData_.state_ = _state;
    }

    bool VideoContext::getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return videoThreadMessagesQueue_.getMessage(_message, [this]{return isQuit();}, _waitTimeout);
    }

    bool VideoContext::updateScaleContext(MediaData& _media, const QSize _sz)
    {
        if (_media.scaledSize_ != _sz)
        {
            _media.scaledSize_ = _sz;

            _media.needUpdateSwsContext_ = true;

            emit videoSizeChanged(_media.scaledSize_);
        }

        return true;
    }

    void VideoContext::freeScaleContext(MediaData& _media)
    {
        if (_media.frameRGB_)
        {
            ffmpeg::av_frame_unref(_media.frameRGB_);
            ffmpeg::av_frame_free(&_media.frameRGB_);
        }

        _media.frameRGB_ = 0;
    }

    bool VideoContext::enableAudio(MediaData& _media) const
    {
        return !!_media.audioStream_;
    }

    bool VideoContext::enableVideo(MediaData& _media) const
    {
        return !!_media.videoStream_;
    }

    void VideoContext::setVolume(int32_t _volume, MediaData& _media)
    {
        _media.volume_ = _volume;
    }

    void VideoContext::setMute(bool _mute, MediaData& _media)
    {
        _media.mute_ = _mute;
    }

    void VideoContext::resetFrameTimer(MediaData& _media)
    {
        _media.frameTimer_ = (double) ffmpeg::av_gettime() / 1000000.0;
    }

    void VideoContext::resetLsatFramePts(MediaData& _media)
    {
        _media.frameLastPts_ = 0.0;
    }

    bool VideoContext::seekMs(uint32_t _videoId, int _tsms, MediaData& _media)
    {
        int64_t _ts_video = ffmpeg::av_rescale(_tsms, _media.videoStream_->time_base.den, _media.videoStream_->time_base.num);
        _ts_video /= 1000;

        int64_t _ts_audio = 0;

        if (enableAudio(_media))
        {
            _ts_audio = ffmpeg::av_rescale(_tsms, _media.audioStream_->time_base.den, _media.audioStream_->time_base.num);
            _ts_audio /= 1000;
        }

        return seekFrame(_videoId, _ts_video, _ts_audio, _media);
    }

    bool VideoContext::seekFrame(uint32_t _videoId, int64_t _ts_video, int64_t _ts_audio, MediaData& _media)
    {
        if (ffmpeg::avformat_seek_file(_media.formatContext_, _media.videoStream_->index, INT64_MIN
            , _media.startTimeVideo_ + _ts_video, INT64_MAX, 0) < 0)
        {
            return false;
        }

        _media.videoQueue_->free([_videoId, this](ffmpeg::AVPacket& _packet)
        {
            if (_packet.data == (uint8_t*) &flush_pkt_data)
            {
                qDebug() << "emit seekedV from clear";

                emit seekedV(_videoId);
            }
            
        });

        _media.audioQueue_->free([_videoId, this](ffmpeg::AVPacket& _packet)
        {
            if (_packet.data == (uint8_t*) &flush_pkt_data)
            {
                qDebug() << "emit seekedA from clear";

                emit seekedA(_videoId);
            }
        });

        ffmpeg::AVPacket flush_pkt_v;
        initFlushPacket(flush_pkt_v);
        flush_pkt_v.dts = (_media.startTimeVideo_ + _ts_video);

        pushVideoPacket(&flush_pkt_v, _media);
        
        if (enableAudio(_media))
        {
            ffmpeg::AVPacket flush_pkt_a;
            initFlushPacket(flush_pkt_a);
            flush_pkt_a.pts = (_media.startTimeAudio_ + _ts_audio);

            pushAudioPacket(&flush_pkt_a, _media);
        }
        else
        {
            emit seekedA(_videoId);
        }

        

        return true;
    }

    void VideoContext::flushVideoBuffers(MediaData& _media)
    {
        if (_media.videoStream_)
        {
            ffmpeg::avcodec_flush_buffers(_media.videoStream_->codec);
        }
    }

    void VideoContext::flushAudioBuffers(MediaData& _media)
    {
        if (_media.audioStream_)
        {
            ffmpeg::avcodec_flush_buffers(_media.audioStream_->codec);
        }
    }

    void VideoContext::resetVideoClock(MediaData& _media)
    {
        _media.videoClock_ = 0.0;
    }

    void VideoContext::resetAudioClock(MediaData& _media)
    {
        _media.audioClock_ = 0.0;
    }

    void VideoContext::setStartTimeVideo(const int64_t& _startTime, MediaData& _media)
    {
        if (_media.startTimeVideoSet_)
            return;

        _media.startTimeVideoSet_ = true;
        _media.startTimeVideo_ = _startTime;
    }

    const int64_t& VideoContext::getStartTimeVideo(MediaData& _media) const
    {
        return _media.startTimeVideo_;
    }

    void VideoContext::setStartTimeAudio(const int64_t& _startTime, MediaData& _media)
    {
        if (_media.startTimeAudioSet_)
            return;

        _media.startTimeAudioSet_ = true;
        _media.startTimeAudio_ = _startTime;
    }

    const int64_t& VideoContext::getStartTimeAudio(MediaData& _media) const
    {
        return _media.startTimeAudio_;
    }

    
    std::shared_ptr<MediaData> VideoContext::getMediaData(uint32_t _videoId, bool& _success)
    {
        std::unique_lock<std::mutex> lock(mediaDataMutex_);

        if (mediaData_.count(_videoId) == 0)
        {
            _success = false;

            return nullptr;
        }
            
        _success = true;
        auto result = mediaData_[_videoId];

        return result;
    }

    void VideoContext::audioQuit(uint32_t _videoId)
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId, success);

        if (!success)
            return;

        media_ptr->audioQuitRecv_ = true;

        SendCloseStreams(_videoId);
    }

    void VideoContext::videoQuit(uint32_t _videoId)
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId, success);

        if (!success)
            return;

        media_ptr->videoQuitRecv_ = true;

        SendCloseStreams(_videoId);
    }

    void VideoContext::demuxQuit(uint32_t _videoId)
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId, success);

        if (!success)
            return;

        media_ptr->demuxQuitRecv_ = true;

        SendCloseStreams(_videoId);
    }

    void VideoContext::streamsClosed(uint32_t _videoId)
    {
        deleteVideo(_videoId);
    }

    void VideoContext::SendCloseStreams(uint32_t _videoId)
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId, success);

        if (!success)
            return;

        if (media_ptr->demuxQuitRecv_
            && media_ptr->audioQuitRecv_
            && media_ptr->videoQuitRecv_)
         /*   && (!getMediaContainer()->ctx_.enableVideo(media) || audioQuitRecv_)
            && (!getMediaContainer()->ctx_.enableAudio(media) || videoQuitRecv_))*/
        {
            //qDebug() << "send tmt_close_streams " << _videoId;
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(_videoId, thread_message_type::tmt_close_streams), true, true);
        }
        else
        {
            //qDebug() << "not close stream " << media_ptr->demuxQuitRecv_ << media_ptr->audioQuitRecv_ << media_ptr->videoQuitRecv_;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////

    DemuxThread::DemuxThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    struct DemuxData
    {
        bool inited;

        decode_thread_state state;

        bool eof;

        int64_t seekPosition;

        DemuxData()
            :   inited(false)
                , state(decode_thread_state::dts_playing)
                , eof(false)
                , seekPosition(-1)
        {

        }
    };

    void DemuxThread::run()
    {
        int32_t waitMsgTimeout = 0;
        ffmpeg::AVPacket packet;
        std::unordered_map<uint32_t, DemuxData> demuxData;
        ThreadMessage msg;

        while (!ctx_.isQuit())
        {
            decode_thread_state state = decode_thread_state::dts_none;
            int64_t seekPosition = -1;
            bool eof = false;

            if (ctx_.getDemuxThreadMessage(msg, waitMsgTimeout))
            {
                auto iterData = demuxData.find(msg.videoId_);
                bool videoDataFound = (iterData != demuxData.end());
                
                if (videoDataFound && (msg.message_ == thread_message_type::tmt_init || msg.message_ == thread_message_type::tmt_open_streams))
                {
                    demuxData.erase(msg.videoId_);
                    videoDataFound = false;
                }

                if (!videoDataFound)
                {
                    if (msg.message_ == thread_message_type::tmt_init)
                    {
                        auto emplace_result = demuxData.insert(std::make_pair(msg.videoId_, Ui::DemuxData()));
                        if (!emplace_result.second)
                        {
                            assert(false);
                            continue;
                        }

                        iterData = emplace_result.first;
                        videoDataFound = true;
                    }
                    else if (msg.message_ == thread_message_type::tmt_open_streams)
                    {
                        auto mediaPath = msg.str_;
                        bool success = false;
                        auto media_ptr = ctx_.getMediaData(msg.videoId_, success);

                        if (!success)
                            continue;

                        if (media_ptr->demuxQuitRecv_)
                            continue;

                        getMediaContainer()->ctx_.openStreams(msg.videoId_, mediaPath, *media_ptr);
                        continue;
                    }
                    else if (msg.message_ == thread_message_type::tmt_close_streams)
                    {
                        bool success = false;
                        auto media_ptr = ctx_.getMediaData(msg.videoId_, success);

                        if (!success)
                            continue;

                        if (media_ptr->streamClosed_)
                            continue;

                        media_ptr->streamClosed_ = true;
                        ctx_.pushVideoPacket(&quit_pkt, *media_ptr);
                        ctx_.pushAudioPacket(&quit_pkt, *media_ptr);

                        getMediaContainer()->ctx_.closeFile(*media_ptr);

                        emit ctx_.streamsClosed(msg.videoId_);

                        continue;
                    }
                    else if (msg.message_ == thread_message_type::tmt_quit)
                    {
                        emit ctx_.demuxQuit(msg.videoId_);

                        continue;
                    }
                    else
                    {
                        continue;
                    }
                }

                assert(videoDataFound);
                assert(iterData != demuxData.end());

                state = iterData->second.state;
                seekPosition = iterData->second.seekPosition;
                eof = iterData->second.eof;

                switch (msg.message_)
                {
                    case thread_message_type::tmt_quit:
                    {
                        if (videoDataFound)
                        {
                            demuxData.erase(msg.videoId_);

                            iterData = demuxData.end();
                            videoDataFound = false;
                        }

                        auto mediaPath = msg.str_;
                        bool success = false;
                        auto media_ptr = ctx_.getMediaData(msg.videoId_, success);

                        if (!success)
                        {
                            emit ctx_.demuxQuit(msg.videoId_);
                            continue;
                        }

                        ctx_.pushVideoPacket(&quit_pkt, *media_ptr);
                        ctx_.pushAudioPacket(&quit_pkt, *media_ptr);
                        
                        emit ctx_.demuxQuit(msg.videoId_);
                        break;
                    }
                    case thread_message_type::tmt_play:
                    {
                        state = decode_thread_state::dts_playing;
                        break;
                    }
                    case thread_message_type::tmt_pause:
                    {
                        state = decode_thread_state::dts_paused;
                        break;
                    }
                    case thread_message_type::tmt_seek_position:
                    {
                        if (seekPosition >= 0)
                        {
                            emit ctx_.seekedV(msg.videoId_);
                            emit ctx_.seekedA(msg.videoId_);
                        }

                        seekPosition = msg.x_;

                        break;
                    }
                    case thread_message_type::tmt_init:
                    {
                        break;
                    }
                }

                if (videoDataFound)
                {
                    iterData->second.state = state;
                    iterData->second.seekPosition = seekPosition;
                }
            }

            waitMsgTimeout = 10;

            int playing = 0;
            for (auto& elem : demuxData)
            {
                auto videoId = elem.first;

                state = elem.second.state;
                seekPosition = elem.second.seekPosition;
                eof = elem.second.eof;

                bool success = false;
                auto media_ptr = ctx_.getMediaData(videoId, success);

                if (!success)
                    continue;

                auto& media = *media_ptr;

                if (elem.second.inited && media.formatContext_)
                {
                    if (state == decode_thread_state::dts_paused)
                    {
                        ctx_.readAVPacketPause(media);
                    }
                    else
                    {
                        ctx_.readAVPacketPlay(media);
                        ++playing;
                    }
                }

                if (state == decode_thread_state::dts_paused)
                {
                    continue;
                }

                if (seekPosition >= 0)
                {
                    ctx_.seekMs(videoId, seekPosition, media);

                    seekPosition = -1;
                    elem.second.seekPosition = seekPosition;
                    eof = false;
                    elem.second.eof = eof;
                }

                if (
                        (
                            (ctx_.getAudioQueueSize(media) > maxQueueSize || !ctx_.enableAudio(media))
                            && (ctx_.getVideoQueueSize(media) > maxQueueSize || !ctx_.enableVideo(media))
                        )

                    ||
                        (
                            (ctx_.getAudioQueuePackets(media) > minFramesCount || !ctx_.enableAudio(media))
                            && (ctx_.getVideoQueuePackets(media) > minFramesCount || !ctx_.enableVideo(media))
                        )
                    )
                {
                    continue;
                }

                int32_t readPacketError = ctx_.readAVPacket(&packet, media);

                if (readPacketError < 0)
                {
                    if (ctx_.isEof(readPacketError, media) && !eof) 
                    {
                        if (ctx_.enableAudio(media))
                            ctx_.pushAudioPacket(0, media);

                        ctx_.pushVideoPacket(0, media);

                        eof = true;
                    }
                
                    elem.second.eof = eof;
                    // TODO : do smth with this error
                    if (ctx_.isStreamError(media))
                        continue;

                    continue;
                }
                else
                {
                    eof = false;
                }

                waitMsgTimeout = 1;
                elem.second.eof = eof;

                int32_t videoStreamIndex = ctx_.getVideoStreamIndex(media);
                int32_t audioStreamIndex = ctx_.getAudioStreamIndex(media);

                if (packet.stream_index == videoStreamIndex)
                {
                    ctx_.pushVideoPacket(&packet, media);
                }
                else if (packet.stream_index == audioStreamIndex)
                {
                    ctx_.pushAudioPacket(&packet, media);
                }
                else
                {
                    av_packet_unref(&packet);
                }

                if (demuxData.count(videoId) != 0 && !elem.second.inited)
                {
                    elem.second.inited = true;

                    emit ctx_.dataReady(videoId);
                }
            }
        }
    }



    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    VideoDecodeThread::VideoDecodeThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    void VideoDecodeThread::prepareCtx(MediaData& _media)
    {
        int32_t w = std::max(ctx_.getWidth(_media), ctx_.getHeight(_media));
        int32_t h = std::min(ctx_.getWidth(_media), ctx_.getHeight(_media));


        ctx_.updateScaleContext(_media, QSize(w, h));
    }

    void VideoDecodeThread::run()
    {
        ffmpeg::AVFrame* frame = ffmpeg::av_frame_alloc();

        std::unordered_map<uint32_t, VideoData> videoData;

        ThreadMessage msg;

        int32_t waitMsgTimeout = 60000;

        ffmpeg::AVPacket av_packet;

        while (!ctx_.isQuit())
        {
            if (ctx_.getVideoThreadMessage(msg, waitMsgTimeout))
            {
                auto videoId = msg.videoId_;
                bool success = false;

                if (videoData.count(videoId) == 0)
                {
                    if (msg.message_ == thread_message_type::tmt_init)
                    {
                        VideoData data;
                        data.current_state_ = decode_thread_state::dts_playing;
                        data.stream_finished_ = false;
                        data.eof_ = false;
                        videoData[videoId] = data;
                        auto media = ctx_.getMediaData(videoId, success);

                        if (success)
                            prepareCtx(*media);
                    }
                    else if (msg.message_ == thread_message_type::tmt_quit)
                    {
                        emit ctx_.videoQuit(videoId);

                        continue;
                    }
                    else
                    {
                        continue;
                    }
                }
                auto media_ptr = ctx_.getMediaData(videoId, success);
                auto& media = *media_ptr;

                if (!success)
                {
                    continue;
                }

                switch (msg.message_)
                {
                    case thread_message_type::tmt_quit:
                    {
                        if (videoData.count(msg.videoId_) > 0)
                        {
                            videoData.erase(msg.videoId_);

                             if (!success)
                                continue;

                            ctx_.freeScaleContext(media);
                        }

                        emit ctx_.videoQuit(videoId);

                        break;
                    }
                    case thread_message_type::tmt_update_scaled_size:
                    {
                        const QSize scaledSize = ctx_.getScaledSize(media);

                        QSize newSize(msg.x_, msg.y_);

                        if (scaledSize != newSize)
                        {
                            ctx_.updateScaleContext(media, newSize);
                        }
                        break;
                    }
                    case thread_message_type::tmt_pause:
                    {
                        if (decode_thread_state::dts_failed == videoData[videoId].current_state_)
                        {
                            break;
                        }

                        videoData[videoId].current_state_ = dts_paused;

                        break;
                    }
                    case thread_message_type::tmt_play:
                    {
                        if (decode_thread_state::dts_failed == videoData[videoId].current_state_)
                        {
                            break;
                        }

                        videoData[videoId].current_state_ = dts_playing;

                        break;
                    }
                    case thread_message_type::tmt_seek_position:
                    {
                        if (videoData[videoId].current_state_ == decode_thread_state::dts_end_of_media)
                        {
                            videoData[videoId].current_state_ = dts_playing;
                        }

                        break;
                    }
                    case thread_message_type::tmt_get_next_video_frame:
                    {
                        if (!success)
                            continue;

                        if (videoData[videoId].current_state_ == decode_thread_state::dts_end_of_media || 
                            videoData[videoId].current_state_ == decode_thread_state::dts_failed)
                        {
                            break;
                        }

                        ffmpeg::av_frame_unref(frame);

                        videoData[videoId].eof_ = false;

                        if (ctx_.getNextVideoFrame(frame, &av_packet, videoData[videoId], media, videoId))
                        {
                            ctx_.setStartTimeVideo(frame->pkt_dts, media);

                            // Consider sync
                            double pts = frame->pkt_dts;

                            if (pts == AV_NOPTS_VALUE)
                            {
                                pts = frame->pkt_pts;
                            }
                            if (pts == AV_NOPTS_VALUE)
                            {
                                pts = 0;
                            }

                            pts *= ctx_.getVideoTimebase(media);
                            pts = ctx_.synchronizeVideo(frame, pts, media);

                            if (ctx_.isQuit())
                            {
                                break;
                            }

                            QSize scaledSize(frame->width, frame->height);

                            if (frame->width < frame->height)
                                scaledSize.transpose();

                            if (scaledSize.width() > max_video_w || scaledSize.height() > max_video_h)
                                scaledSize.scale(max_video_w, max_video_h, Qt::KeepAspectRatio);

                            if (frame->width < frame->height)
                                scaledSize.transpose();

                            // update scale context
                            if ((media.needUpdateSwsContext_) || (frame->format != -1 && frame->format != media.codecContext_->pix_fmt) || !media.swsContext_)
                            {
                                if (media.frameRGB_)
                                {
                                    ffmpeg::av_frame_unref(media.frameRGB_);
                                    ffmpeg::av_frame_free(&media.frameRGB_);

                                    media.frameRGB_ = 0;
                                }

                                media.needUpdateSwsContext_ = false;
                                media.swsContext_ = sws_getCachedContext(
                                    media.swsContext_, 
                                    frame->width, 
                                    frame->height, 
                                    ffmpeg::AVPixelFormat(frame->format), scaledSize.width(), scaledSize.height(), ffmpeg::AV_PIX_FMT_RGBA, SWS_POINT, 0, 0, 0);
                                
                                int align = 256;
                                while (1)
                                {
                                    if (frame->linesize[0] % align == 0)
                                        break;
                                    
                                    align = align / 2;
                                }

                                media.frameRGB_ = ffmpeg::av_frame_alloc();
                                int numBytes = ffmpeg::av_image_get_buffer_size(ffmpeg::AV_PIX_FMT_RGBA, scaledSize.width(), scaledSize.height(), align);
                                media.scaledBuffer_.resize(numBytes);
                                av_image_fill_arrays(media.frameRGB_->data, media.frameRGB_->linesize, &media.scaledBuffer_[0], ffmpeg::AV_PIX_FMT_RGBA, scaledSize.width(), scaledSize.height(), align);

                            }

                            ffmpeg::sws_scale(media.swsContext_, frame->data, frame->linesize, 0, frame->height, media.frameRGB_->data, media.frameRGB_->linesize);

                            QImage lastFrame(scaledSize.width(), scaledSize.height(), QImage::Format_RGBA8888);

                            for (int32_t y = 0; y < scaledSize.height(); ++y)
                            {
                                memcpy(lastFrame.scanLine(y), media.frameRGB_->data[0] + y*media.frameRGB_->linesize[0], scaledSize.width()*4);
                            }

                            std::unique_ptr<QTransform> imageTransform;

                            if (ctx_.getRotation(media))
                            {
                                imageTransform.reset(new QTransform());
                                imageTransform->rotate(ctx_.getRotation(media));
                            }

                            if (imageTransform)
                            {
                                lastFrame = lastFrame.transformed(*imageTransform);
                            }

                            emit ctx_.nextframeReady(videoId, lastFrame, pts, false);
                        }
                        else if (videoData[videoId].eof_)
                        {
                            if (!success)
                                continue;

                            videoData[videoId].current_state_ = decode_thread_state::dts_end_of_media;

                            videoData[videoId].stream_finished_ = false;

                            ctx_.resetVideoClock(media);

                            emit ctx_.nextframeReady(videoId, QImage(), 0, true);
                        }
                        else
                        {
                            break;
                        }

                        break;
                    }
                    default:
                        break;
                }
            }
        }

        for (auto& data : videoData)
        {
            bool success = false;
            auto media = ctx_.getMediaData(data.first, success);
            
            if (success)
                ctx_.freeScaleContext(*media);
        }

        ffmpeg::av_frame_unref(frame);
        ffmpeg::av_frame_free(&frame);

        return;
    }



    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    AudioDecodeThread::AudioDecodeThread(VideoContext& _ctx)
        :   ctx_(_ctx)
    {

    }

    void AudioDecodeThread::run()
    {
        qDebug() << "Audio decode thread start";

        std::unordered_map<uint32_t, AudioData> audioData;
        bool flush = false;

        ffmpeg::AVPacket packet;
        
        if (true)//ctx_.initDecodeAudioData(0 /* _videoId*/))
        {
            while (!ctx_.isQuit())
            {
                ThreadMessage msg;

                bool has_playing = false;
                int count_of_playing = 0;
                for (auto& data : audioData)
                {
                    auto success = false;
                    auto media_ptr = ctx_.getMediaData(data.first, success);
                    if (success && media_ptr->audioData_.state_ == decode_thread_state::dts_playing)
                    {
                        has_playing = true;
                        ++count_of_playing;
                        // uncomment after testing
                        // break;
                    }
                }

                if (!flush && ctx_.getAudioThreadMessage(msg, has_playing ?  10 : 60000))
                {
                    auto videoId = msg.videoId_;

                    if (audioData.count(videoId) == 0)
                    {
                        if (msg.message_ == thread_message_type::tmt_init)
                        {
                            bool success = false;
                            auto media = ctx_.getMediaData(videoId, success);

                            if (!success || !ctx_.initDecodeAudioData(*media))
                                continue;

                            AudioData data;
                            data.eof_ = false;
                            data.stream_finished_ = false;
                            audioData[videoId] = data;

                            ctx_.setMute(msg.x_, *media);
                            ctx_.setVolume(msg.y_, *media);
                        }
                        else if (msg.message_ == thread_message_type::tmt_quit)
                        {
                            ctx_.audioQuit(videoId);
                            continue;
                        }
                        else
                            continue;
                    }

                    bool success = false;
                    auto media_ptr = ctx_.getMediaData(videoId, success);
                    auto& media = *media_ptr;

                    if (!success)
                        continue;

                    switch (msg.message_)
                    {
                        case thread_message_type::tmt_set_volume:
                        {
                            ctx_.setVolume(msg.x_, media);

                            break;
                        }
                        case thread_message_type::tmt_pause:
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_paused, media);

                            ctx_.suspendAudio(media);

                            break;
                        }
                        case thread_message_type::tmt_play:
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_playing, media);

                            break;
                        }
                        case thread_message_type::tmt_set_mute:
                        {
                            ctx_.setMute(msg.x_, media);

                            break;
                        }
                        case thread_message_type::tmt_quit:
                        {
                            if (audioData.count(msg.videoId_) > 0)
                            {
                                bool success = false;
                                auto media = ctx_.getMediaData(msg.videoId_, success);
                                if (success)
                                    ctx_.freeDecodeAudioData(*media);
                                
                                audioData.erase(msg.videoId_);
                            }
                            emit ctx_.audioQuit(videoId);
                            break;
                        }
                        case thread_message_type::tmt_set_finished:
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_end_of_media, media);

                            audioData[videoId].eof_ = true;

                            break;
                        }
                    }
                }

                flush = false;

                if (ctx_.isQuit())
                    break;

                bool has_audio_playing = false;
                for (auto data : audioData)
                {
                    auto videoId = data.first;
                    audioData[videoId].eof_ = false;

                    bool success = false;
                    auto media_ptr = ctx_.getMediaData(videoId, success);
                    auto& media = *media_ptr;

                    if (!success || !ctx_.enableAudio(media))
                        continue;

                    if (ctx_.getAudioThreadState(media) == decode_thread_state::dts_playing)
                    {
                        has_audio_playing = true;

                        bool local_flush = false;
                        int seekCount = 0;

                        if (!ctx_.playNextAudioFrame(&packet, audioData[videoId], local_flush, seekCount, media, videoId))
                        {

                        }

                        int64_t current_time = ffmpeg::av_gettime();

                        const int64_t sync_period = 1000 * 200; // 200 milliseconds

                        if ((current_time - audioData[videoId].last_sync_time_) > sync_period)
                        {
                            audioData[videoId].last_sync_time_ = current_time;

                            //qDebug() << "Audio thread: emit audioTime";
                            //qDebug() << "Audio thread: offset = " <<  audioData[videoId].offset_;

                            emit ctx_.audioTime(videoId, current_time, audioData[videoId].offset_);
                        }

                         if (seekCount)
                         {
                             for (int i = 0; i < seekCount; ++i)
                                emit ctx_.seekedA(videoId);
                         }

                        flush |= local_flush;

                        if (audioData[videoId].eof_)
                        {
                            ctx_.setAudioThreadState(decode_thread_state::dts_end_of_media, media);

                            audioData[videoId].stream_finished_ = false;
                        }
                    }
                }
            }

            qDebug() << "Audio decode thread finish";
            for (auto data : audioData)
            {
                auto videoId = data.first;
                bool success = false;
                auto media = ctx_.getMediaData(videoId, success);
                if (success)
                    ctx_.freeDecodeAudioData(*media);
            }
        }

    }

    static int lockmgr(void **mtx, enum ffmpeg::AVLockOp op)
    {
        switch(op) 
        {
            case ffmpeg::AV_LOCK_CREATE:
            {
                *mtx = new std::mutex();
                return 0;
            }
            case ffmpeg::AV_LOCK_OBTAIN:
            {
                ((std::mutex*) *mtx)->lock();
                return 0;
            }
            case ffmpeg::AV_LOCK_RELEASE:
            {
                ((std::mutex*) *mtx)->unlock();
                return 0;
            }
            case ffmpeg::AV_LOCK_DESTROY:
            {
                delete ((std::mutex*) *mtx);
                return 0;
            }
        }

        return 1;
    }


    void FrameRenderer::renderFrame(QPainter& _painter, const QRect& _clientRect)
    {
        if (activeImage_.isNull())
        {
            _painter.fillRect(_clientRect, Qt::black);

            return;
        }

        QSize imageSize = activeImage_.size();

        QRect imageRect(0, 0, imageSize.width(), imageSize.height());

        QSize scaledSize;
        QRect sourceRect;

        if (fillClient_)
        {
            scaledSize = QSize(_clientRect.width(), _clientRect.height());
            QSize size = _clientRect.size();
            size.scale(imageSize, Qt::KeepAspectRatio);
            sourceRect = QRect(0, 0, size.width(), size.height());
            if (imageSize.width() > size.width())
                sourceRect.moveLeft(imageSize.width() / 2 - size.width() / 2);

            if (imageSize.height() > size.height())
                sourceRect.moveTop(imageSize.height() / 2 - size.height() / 2);
        }
        else
        {
            int32_t h = (int32_t) (((double) imageSize.height() / (double) imageSize.width()) * (double) _clientRect.width());

            if (h > _clientRect.height())
            {
                h = _clientRect.height();

                scaledSize.setWidth(((double) imageSize.width() / (double) imageSize.height()) * (double) _clientRect.height());
            }
            else
            {
                scaledSize.setWidth(_clientRect.width());
            }

            scaledSize.setHeight(h);
        }

        int cx = (_clientRect.width() - scaledSize.width()) / 2;
        int cy = (_clientRect.height() - scaledSize.height()) / 2;

        QRect drawRect(cx, cy, scaledSize.width(), scaledSize.height());

        //auto t1 = std::chrono::system_clock::now();

        if (fillColor_.isValid())
            _painter.fillRect(_clientRect, fillColor_);

        QSize sz = activeImage_.size();

        //qDebug() << "image size = " << sz;

        auto t2 = std::chrono::system_clock::now();

        if (fillClient_)
            _painter.drawPixmap(drawRect, activeImage_, sourceRect);
        else
            _painter.drawPixmap(drawRect, activeImage_);

        //auto t3 = std::chrono::system_clock::now();

        //qDebug() << "fill time " << (t2 - t1)/std::chrono::milliseconds(1) << "draw frame time " << (t3 - t2)/std::chrono::milliseconds(1);
    }

    void FrameRenderer::updateFrame(QPixmap _image)
    {
        activeImage_ = _image;
    }

    QPixmap FrameRenderer::getActiveImage() const
    {
        return activeImage_;
    }

    bool FrameRenderer::isActiveImageNull() const
    {
        return activeImage_.isNull();
    }

    void FrameRenderer::setClippingPath(QPainterPath _clippingPath)
    {
        clippingPath_ = _clippingPath;
    }

    void FrameRenderer::setFullScreen(bool _fullScreen)
    {
        fullScreen_ = _fullScreen;
    }

    void FrameRenderer::setFillColor(const QColor& _color)
    {
        fillColor_ = _color;
    }

    void FrameRenderer::setFillClient(bool _fill)
    {
        fillClient_ = _fill;
    }

    void FrameRenderer::setSizeCallback(std::function<void(const QSize)> _callback)
    {
        sizeCallback_ = _callback;
    }

    void FrameRenderer::onSize(const QSize _sz)
    {
        if (sizeCallback_)
            sizeCallback_(_sz);
    }




    GDIRenderer::GDIRenderer(QWidget* _parent)
        : QWidget(_parent)
    {
        setMouseTracking(true);
    }

    QWidget* GDIRenderer::getWidget()
    {
        return this;
    }

    void GDIRenderer::redraw()
    {
        update();
    }

    void GDIRenderer::paintEvent(QPaintEvent* _e)
    {
        QPainter p;
        p.begin(this);

        QRect clientRect = geometry();

        if (!fullScreen_)
        {
            //assert(!clippingPath_.isEmpty());
             if (!clippingPath_.isEmpty())
             {
                 p.setClipPath(clippingPath_);
             }
        }

        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        renderFrame(p, clientRect);

        p.end();

        QWidget::paintEvent(_e);
    }

    void GDIRenderer::resizeEvent(QResizeEvent *_event)
    {
        onSize(_event->size());
    }

    void GDIRenderer::filterEvents(QWidget* _parent)
    {
        installEventFilter(_parent);
    }

    void GDIRenderer::setWidgetVisible(bool _visible)
    {
        setVisible(_visible);
    }

#ifndef __linux__
    OpenGLRenderer::OpenGLRenderer(QWidget* _parent)
        : QOpenGLWidget(_parent)
    {
        //if (platform::is_apple())
            setFillColor(Qt::GlobalColor::black);

        setAutoFillBackground(false);

        setMouseTracking(true);
    }

    QWidget* OpenGLRenderer::getWidget()
    {
        return this;
    }

    void OpenGLRenderer::redraw()
    {
        update();
    }

    void OpenGLRenderer::paint()
    {
        QPainter p;
        p.begin(this);

        QRect clientRect = geometry();

        if (!fullScreen_)
        {
            if (!clippingPath_.isEmpty())
            {
                p.setClipPath(clippingPath_);
            }
        }

        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        renderFrame(p, clientRect);

        p.end();
    }

    void OpenGLRenderer::paintEvent(QPaintEvent* _e)
    {
        paint();

        QOpenGLWidget::paintEvent(_e);
    }

    void OpenGLRenderer::paintGL()
    {
        paint();

        QOpenGLWidget::paintGL();
    }

    void OpenGLRenderer::resizeEvent(QResizeEvent *_event)
    {
        onSize(_event->size());

        QOpenGLWidget::resizeEvent(_event);
    }

    void OpenGLRenderer::filterEvents(QWidget* _parent)
    {
        installEventFilter(_parent);
    }

    void OpenGLRenderer::setWidgetVisible(bool _visible)
    {
        setVisible(_visible);
    }
#endif //__linux__

    const auto mouse_move_rate = std::chrono::milliseconds(200);

    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    FFMpegPlayer::FFMpegPlayer(QWidget* _parent, bool _openGL, bool _continius)
        :   QWidget(_parent),
            state_(decode_thread_state::dts_none),
            isFirstFrame_(true),
            lastVideoPosition_(0),
            lastPostedPosition_(0),
            lastEmitMouseMove_(std::chrono::system_clock::now() - mouse_move_rate),
            started_(false),
            stoped_(false),
            continius_(_continius),
            mediaId_(0),
            imageDuration_(0),
            imageProgress_(0),
            replay_(false),
            active_renderer_(nullptr),
            gdi_renderer_(nullptr),
            opengl_renderer_(nullptr),
            updatePositonRate_(platform::is_apple() ? 1000 : 100),
            mute_(false),
            dataReady_(false),
            pausedByUser_(false),
            seek_request_id_(0),
            volume_(100),
            restoreVolume_(100)
    {
        layout_ = Utils::emptyHLayout();
        setLayout(layout_);

        if (_openGL)
        {
            opengl_renderer_ = CreateRenderer(this, true);
            active_renderer_ = opengl_renderer_;
        }
        else
        {
            gdi_renderer_ = CreateRenderer(this, false);
            active_renderer_ = gdi_renderer_;
        }

        active_renderer_->filterEvents(this);

        layout_->addWidget(active_renderer_->getWidget());

        setAutoFillBackground(false);
        
        static auto is_init = false;

        if (!is_init && ffmpeg::av_lockmgr_register(lockmgr)) // TODO
        {
        }

        is_init = true;

        timer_ = new QTimer(this);
        connect(timer_, SIGNAL(timeout()), this, SLOT(onTimer()));
        setMouseTracking(true);

        connect(&getMediaContainer()->ctx_, &VideoContext::dataReady, this, &FFMpegPlayer::onDataReady, Qt::QueuedConnection);
        connect(&getMediaContainer()->ctx_, &VideoContext::nextframeReady, this, &FFMpegPlayer::onNextFrameReady, Qt::QueuedConnection);
        connect(&getMediaContainer()->ctx_, &VideoContext::streamsOpenFailed, this, &FFMpegPlayer::streamsOpenFailed, Qt::QueuedConnection);
        connect(&getMediaContainer()->ctx_, &VideoContext::audioTime, this, &FFMpegPlayer::onAudioTime, Qt::QueuedConnection);
        connect(&getMediaContainer()->ctx_, &VideoContext::seekedV, this, &FFMpegPlayer::seekedV, Qt::QueuedConnection);
        connect(&getMediaContainer()->ctx_, &VideoContext::seekedA, this, &FFMpegPlayer::seekedA, Qt::QueuedConnection);

        imageProgressAnimation_ = new QPropertyAnimation(this, "imageProgress");
    }


    FFMpegPlayer::~FFMpegPlayer()
    {
        stop();
    }

    void FFMpegPlayer::seeked(uint32_t _videoId, const bool _fromAudio)
    {
        if (_videoId != mediaId_)
            return;

        --seek_request_id_;

        qDebug() << (_fromAudio ? "seekedA" : "seekedV") << " seek_request_id_ = " << seek_request_id_;

        if (seek_request_id_ < 0)
        {
            //assert(false);
            seek_request_id_ = 0;
        }

        if (seek_request_id_ == 0)
        {
            bool success = false;
            auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);
            if (success && media)
            {
                getMediaContainer()->resetLastFramePts(*media);
            }
        }
    }

    void FFMpegPlayer::seekedV(uint32_t _videoId)
    {
        seeked(_videoId, false);
    }

    void FFMpegPlayer::seekedA(uint32_t _videoId)
    {
        seeked(_videoId, true);
   }

    void FFMpegPlayer::onNextFrameReady(uint32_t _videoId, QImage _image, double _pts, bool _eof)
    {
        if (_videoId != mediaId_)
            return;

        if (!_eof)
        {
            QPixmap frame = QPixmap::fromImage(QImage(_image));

            decodedFrames_.emplace_back(frame, _pts);

            if (!firstFrame_)
            {
                firstFrame_.reset(new DecodedFrame(frame, _pts));

                emit firstFrameReady();
            }
        }
        else
        {
            decodedFrames_.emplace_back(_eof);

            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_set_finished), false);
        }
    }

    void FFMpegPlayer::onDataReady(uint32_t _videoId)
    {
        // TODO : use signal mapper
        if (_videoId != mediaId_)
            return;

        timer_->stop();

        emit durationChanged(getDuration());
        dataReady_ = true;

        if (!getMediaContainer()->is_decods_inited_)
        {
            getMediaContainer()->VideoDecodeThreadStart(mediaId_);
            getMediaContainer()->AudioDecodeThreadStart(mediaId_);
            getMediaContainer()->is_decods_inited_ = true;

            getMediaContainer()->updateVideoScaleSize(mediaId_, size());
        }

        timer_->start(100);
    }

    void FFMpegPlayer::onAudioTime(uint32_t _videoId, int64_t _avtime, double _offset)
    {
        if (_videoId != mediaId_)
            return;

        if (seek_request_id_ > 0)
            return;

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);

        if (!success)
            return;

        //qDebug() << "media->syncWithAudio_ = " << media->syncWithAudio_;

        media->syncWithAudio_ = true;
        media->audioClock_ = _offset;
        media->audioClockTime_ = _avtime;
    }

    FrameRenderer* FFMpegPlayer::CreateRenderer(QWidget* _parent, bool _openGL)
    {
        FrameRenderer* renderer = nullptr;
        
#if defined(__linux__)
        renderer = new GDIRenderer(_parent);
#else
        if (_openGL && (platform::is_windows_vista_or_late() || platform::is_apple()))
        {
            renderer = new OpenGLRenderer(_parent);
        }
        else
        {
            renderer = new GDIRenderer(_parent);
        }
#endif //__linux__ || __APPLE__

        renderer->setSizeCallback(std::bind(&FFMpegPlayer::onRendererSize, this, std::placeholders::_1));

        return renderer;
    }


    uint32_t FFMpegPlayer::stop()
    {
        if (stoped_)
            return mediaId_;

        state_ = decode_thread_state::dts_none;
        timer_->stop();

        imageProgressAnimation_->stop();
        setImageProgress(imageDuration_);
        decodedFrames_.clear();

        if (!continius_)
            stoped_ = true;

        getMediaContainer()->ctx_.setVideoQuit(mediaId_);

        getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_quit), true, true);
        getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_quit), true, true);
        getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_quit), true, true);

        auto ret = mediaId_;
        mediaId_ = 0;

        return ret;
    }

    void FFMpegPlayer::updateVideoPosition(const DecodedFrame& _frame)
    {
        if (_frame.eof_)
        {
            bool success = false;
            auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);

            if (!success)
                return;

            emit positionChanged(getMediaContainer()->getDuration(*media));

            lastVideoPosition_ = 0;
            lastPostedPosition_ = 0;

            return;
        }

        qint64 videoClock = (_frame.pts_ * 1000);

        if (seek_request_id_ == 0)
        {
            lastVideoPosition_ = videoClock;

            if ((lastVideoPosition_ - lastPostedPosition_) > updatePositonRate_)
            {
                lastPostedPosition_ = lastVideoPosition_;

                if (seek_request_id_ == 0)
                {
                    emit positionChanged(lastVideoPosition_);
                }
            }
        }
    }

    void FFMpegPlayer::onTimer()
    {
        if (!getStarted())
        {
            active_renderer_->redraw();

            pause();
            return;
        }

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);
        if (!success)
        {
            timer_->stop();
            return;
        }

        if (isFirstFrame_)
        {
            isFirstFrame_ = false;
            getMediaContainer()->resetFrameTimer(*media);
        }

        if (getMediaContainer()->isQuit(mediaId_))
        {
            timer_->stop();

            return;
        }

        if (state_ != decode_thread_state::dts_playing)
        {
            return;
        }

        if (decodedFrames_.empty())
        {
            if (getStarted())
            {
                timer_->start();
                timer_->setInterval((int) 100);
            }


            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);

            return;
        }
        
        if (dataReady_)
        {
            emit dataReady();
            setImageProgress(0);
            dataReady_ = false;
        }

        auto& frame = decodedFrames_.front();

        if (!media->isImage_)
            updateVideoPosition(frame);

        if (frame.eof_)
        {
            if (media->isImage_)
            {
                if (imageProgressAnimation_->state() == QAbstractAnimation::Running)
                {
                    timer_->start(100);
                    return;
                }

                if (getImageProgress() != getDuration())
                {
                    imageProgressAnimation_->stop();
                    imageProgressAnimation_->setStartValue(0);
                    imageProgressAnimation_->setEndValue((qint64)getDuration());
                    imageProgressAnimation_->setDuration((qint64)getDuration());
                    imageProgressAnimation_->start();
                    timer_->start(100);
                    return;
                }
            }

            decodedFrames_.pop_front();
            timer_->stop();

            // emit mediaFinished();

            if (continius_ && !replay_)
            {
                stop();
                loadFromQueue();
            }
            else
            {
                state_ = decode_thread_state::dts_end_of_media;
            }

            if (firstFrame_ && !continius_)
                active_renderer_->updateFrame(firstFrame_->image_);

            active_renderer_->redraw();

            // TODO : use another way
            if (!continius_ || mediaId_ != 0)
                play(true);
            return;
        }
        //qDebug() << "send "<< QTime::currentTime() <<"get next id onTimer2 " << mediaId_;
        getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);

        active_renderer_->updateFrame(frame.image_);

        if (!success)
        {
            timer_->stop();
            return;
        }

        double delay = (seek_request_id_ ? 0.1 : getMediaContainer()->computeDelay(frame.pts_, *media));

        int timeout = (int)(delay * 1000.0 + 0.5);
        if (getStarted())
            timer_->setInterval(timeout);

        decodedFrames_.pop_front();

        active_renderer_->redraw();

    }

    bool FFMpegPlayer::openMedia(const QString& _mediaPath, bool _isImage, uint32_t _id)
    {
        if (!continius_)
            stoped_ = false;

        uint32_t mediaId = getMediaContainer()->init(_id);
        
        openStreamsConnection_ = connect(&getMediaContainer()->ctx_, &VideoContext::streamsOpened, this, &FFMpegPlayer::onStreamsOpened, Qt::QueuedConnection);

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(mediaId, success);
        if (!success)
            return false;

        media->isImage_ = _isImage;

        getMediaContainer()->DemuxThreadStart(mediaId);

        auto msg = ThreadMessage(mediaId, thread_message_type::tmt_open_streams);
        msg.str_ = _mediaPath;

        getMediaContainer()->postDemuxThreadMessage(msg, false);

        if (mediaId_ == 0 || !continius_)
        {
            mediaId_ = mediaId;
            emit mediaChanged(mediaId_);
        }
        else
        {
            if (_id != 0)
                queuedMedia_.push_front(mediaId);
            else
                queuedMedia_.push_back(mediaId);
        }

        getMediaContainer()->updateVideoScaleSize(getMedia(), size());

        return true;// getMediaContainer()->openFile(_mediaPath, *media);
    }

    void FFMpegPlayer::onRendererSize(const QSize _sz)
    {
        getMediaContainer()->updateVideoScaleSize(getMedia(), _sz);
    }

    void FFMpegPlayer::onStreamsOpened(uint32_t _videoId)
    {
        if (_videoId != mediaId_ && !continius_)
            return;

        if (continius_ && _videoId != mediaId_)
        {
            auto queued = std::find(queuedMedia_.begin(), queuedMedia_.end(), _videoId);
            if (queued == queuedMedia_.end())
                return;
        }

        disconnect(openStreamsConnection_);

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(_videoId, success);
        if (!success || !media->videoStream_)
            return;

        getMediaContainer()->openFile(*media);
        emit fileLoaded();
    }

    void FFMpegPlayer::play(bool _init)
    {
        qDebug() << "void FFMpegPlayer::play(bool _init = " << _init << ")";

        if (!_init)
        {
            active_renderer_->redraw();
            return;
        }

        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_, success);

        if (!success)
            return;

        auto& media = *media_ptr;

        setStarted(_init);

        if (state_ == decode_thread_state::dts_none)
        {
            getMediaContainer()->DemuxThreadStart(mediaId_);
            
            // TODO : mb true here
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_init), false);
            ThreadMessage msg(mediaId_, thread_message_type::tmt_init);
            msg.x_ = mute_;
            msg.y_ = volume_;
            getMediaContainer()->postAudioThreadMessage(msg, false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_init), false);

            //qDebug() << "send get next id play " << mediaId_;
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);

            getMediaContainer()->resetFrameTimer(media);
        }
        else if (/*started_ || */state_ == decode_thread_state::dts_paused)
        {

            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);

            getMediaContainer()->resetFrameTimer(media);
        }
        else if (state_ == decode_thread_state::dts_end_of_media)
        {
            setPosition(0);

            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);

            state_ = decode_thread_state::dts_playing;

            getMediaContainer()->resetFrameTimer(media);

            timer_->start(0);
        }

        state_ = decode_thread_state::dts_playing;

        emit played();
    }

    bool FFMpegPlayer::canPause() const
    {
        return !active_renderer_->isActiveImageNull();
    }

    void FFMpegPlayer::pause()
    {
        qDebug() << "void FFMpegPlayer::pause()";

        if (state_ == decode_thread_state::dts_playing && canPause())
        {
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);

            state_ = decode_thread_state::dts_paused;
        }

        emit paused();
    }

    void FFMpegPlayer::setPosition(int64_t _position)
    {
        ThreadMessage msg(mediaId_, thread_message_type::tmt_seek_position);

        msg.x_ = (int32_t) _position;

        ++seek_request_id_; // for video thread
        ++seek_request_id_; // for audio thread

        qDebug() << "seek request id = " << seek_request_id_ << " position = " << _position;

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);

        media->syncWithAudio_ = false;
        media->audioClock_ = 0.0;
        media->audioClockTime_ = 0;

        //qDebug() << "media->syncWithAudio_ = " << media->syncWithAudio_;

        decodedFrames_.clear();

        if (!success)
            return;

        getMediaContainer()->postDemuxThreadMessage(msg, false);
        getMediaContainer()->postVideoThreadMessage(msg, false);
        getMediaContainer()->postAudioThreadMessage(msg, false);

        getMediaContainer()->resetFrameTimer(*media);
        getMediaContainer()->resetLastFramePts(*media);

        lastVideoPosition_ = _position;
        lastPostedPosition_ = _position;
    }

    void FFMpegPlayer::resetPosition()
    {
        lastVideoPosition_ = 0;
        lastPostedPosition_ = 0;
    }

    void FFMpegPlayer::setUpdatePositionRate(int _rate)
    {
        updatePositonRate_ = _rate;
    }

    void FFMpegPlayer::setVolume(int32_t _volume)
    {
        volume_ = _volume;

        ThreadMessage msg(mediaId_, thread_message_type::tmt_set_volume);
        msg.x_ = _volume;

        getMediaContainer()->postAudioThreadMessage(msg, false);
    }

    int32_t FFMpegPlayer::getVolume() const
    {
        return volume_;
    }

    void FFMpegPlayer::setMute(bool _mute)
    {
        ThreadMessage msg(mediaId_, thread_message_type::tmt_set_mute);
        msg.x_ = _mute;

        getMediaContainer()->postAudioThreadMessage(msg, false);

        mute_ = _mute;
    }

    bool FFMpegPlayer::isMute() const
    {
        return mute_;
    }

    QSize FFMpegPlayer::getVideoSize() const
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_, success);

        if (!success)
            return QSize();

        auto& media = *media_ptr;

        if (getMediaContainer()->getRotation(media) == 0 || getMediaContainer()->getRotation(media) == 180)
        {
            return QSize(getMediaContainer()->getWidth(media), getMediaContainer()->getHeight(media));
        }
        else
        {
            return QSize(getMediaContainer()->getHeight(media), getMediaContainer()->getWidth(media));
        }
    }

    int32_t FFMpegPlayer::getVideoRotation() const
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_, success);
        auto& media = *media_ptr;

        if (!success)
            return 0;

        return getMediaContainer()->getRotation(media);
    }

    int64_t FFMpegPlayer::getDuration() const
    {
        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_, success);
        auto& media = *media_ptr;
        if (!success)
            return -1;

        if (media.isImage_)
            return imageDuration_;

        return getMediaContainer()->getDuration(media);
    }

    bool FFMpegPlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        QObject* objectRenderer = qobject_cast<QObject*>(active_renderer_->getWidget());

        if (objectRenderer == _obj)
        {
            if (QEvent::Leave == _event->type())
            {
                emit mouseLeaveEvent();
            }
            else if (QEvent::MouseMove == _event->type())
            {
                const auto currentTime = std::chrono::system_clock::now();
                if (currentTime - lastEmitMouseMove_ > mouse_move_rate)
                {
                    lastEmitMouseMove_ = currentTime;
                    
                    emit mouseMoved();
                }
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    void FFMpegPlayer::setPaused(bool _paused)
    {
        if (_paused)
        {
            pause();
        }
        else
        {
            play(false /* _init */);
        }
    }

    void FFMpegPlayer::setPausedByUser(const bool _paused)
    {
        pausedByUser_ = _paused;
    }

    bool FFMpegPlayer::isPausedByUser() const
    {
        return pausedByUser_;
    }

    QMovie::MovieState FFMpegPlayer::state() const
    {
        if (state_ == decode_thread_state::dts_playing)
            return QMovie::Running;
        else if (state_ == decode_thread_state::dts_paused)
            return QMovie::Paused;
        else
            return QMovie::NotRunning;
    }

    void FFMpegPlayer::setPreview(QPixmap _preview)
    {
        active_renderer_->updateFrame(_preview);

        active_renderer_->redraw();
    }

    QPixmap FFMpegPlayer::getActiveImage() const
    {
        return active_renderer_->getActiveImage();
    }

    bool FFMpegPlayer::getStarted() const
    {
        return started_;
    }
    
    void FFMpegPlayer::setStarted(bool _started)
    {
        started_ = _started;
    }

    void FFMpegPlayer::loadFromQueue()
    {
        if (queuedMedia_.empty())
        {
            emit mediaChanged(-1);
            return;
        }

        mediaId_ = queuedMedia_.front();
        queuedMedia_.pop_front();

        emit mediaChanged(mediaId_);
    }

    void FFMpegPlayer::removeFromQueue(uint32_t _media)
    {
        queuedMedia_.remove(_media);
    }

    void FFMpegPlayer::clearQueue()
    {
        for (auto i : queuedMedia_)
        {
            getMediaContainer()->ctx_.setVideoQuit(i);
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(i, thread_message_type::tmt_quit), true, true);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(i, thread_message_type::tmt_quit), true, true);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(i, thread_message_type::tmt_quit), true, true);
        }
        queuedMedia_.clear();
    }

    bool FFMpegPlayer::queueIsEmpty() const
    {
        return queuedMedia_.empty();
    }

    int FFMpegPlayer::queueSize() const
    {
        return queuedMedia_.size();
    }

    void FFMpegPlayer::setClippingPath(QPainterPath _clippingPath)
    {
        active_renderer_->setClippingPath(_clippingPath);
    }

    void FFMpegPlayer::setFillColor(const QColor& _color)
    {
        active_renderer_->setFillColor(_color);
        update();
    }

    void FFMpegPlayer::setFillClient(bool _fill)
    {
        active_renderer_->setFillClient(_fill);
        update();
    }

    void FFMpegPlayer::setFullScreen(QWidget* _parent, QLayout* _attach_to)
    {
        if (active_renderer_ == gdi_renderer_)
        {
            active_renderer_->setWidgetVisible(false);

            if (!opengl_renderer_)
            {
                opengl_renderer_ = CreateRenderer(_parent, true);

                _attach_to->addWidget(opengl_renderer_->getWidget());

                opengl_renderer_->filterEvents(this);
            }

            opengl_renderer_->updateFrame(active_renderer_->getActiveImage());

            active_renderer_ = opengl_renderer_;
        }

        getMediaContainer()->updateVideoScaleSize(getMedia(), active_renderer_->getWidget()->size());

        active_renderer_->setFullScreen(true);
        active_renderer_->setWidgetVisible(true);
        
    }

    void FFMpegPlayer::setNormal()
    {
        if (active_renderer_ == opengl_renderer_)
        {
            active_renderer_->setWidgetVisible(false);

            if (!gdi_renderer_)
            {
                gdi_renderer_ = CreateRenderer(this, false);

                layout_->addWidget(gdi_renderer_->getWidget());

                gdi_renderer_->filterEvents(this);
            }

            gdi_renderer_->updateFrame(active_renderer_->getActiveImage());

            active_renderer_ = gdi_renderer_;

            opengl_renderer_ = nullptr;
        }

        getMediaContainer()->updateVideoScaleSize(getMedia(), active_renderer_->getWidget()->size());

        active_renderer_->setFullScreen(false);
        active_renderer_->setWidgetVisible(true);
    }

    void FFMpegPlayer::resetRenderer()
    {
        if (active_renderer_ == opengl_renderer_)
        {
            layout_->removeWidget(active_renderer_->getWidget());

            opengl_renderer_ = CreateRenderer(this, true);

            layout_->addWidget(opengl_renderer_->getWidget());

            opengl_renderer_->filterEvents(this);

            active_renderer_ = opengl_renderer_;
        }
    }

    void FFMpegPlayer::setImageDuration(int _duration)
    {
        imageDuration_ = _duration;
    }

    void FFMpegPlayer::setImageProgress(int _val)
    {
        imageProgress_ = _val;

        bool success = false;
        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_, success);
        if (!success)
            return;

        if (media->isImage_)
        {
            emit positionChanged(_val);
        }
    }

    int FFMpegPlayer::getImageProgress() const
    {
        return imageProgress_;
    }

    uint32_t FFMpegPlayer::getLastMedia() const
    {
        return queuedMedia_.empty() ? mediaId_ : queuedMedia_.back();
    }

    uint32_t FFMpegPlayer::getMedia() const
    {
        return mediaId_;
    }

    uint32_t FFMpegPlayer::reserveId()
    {
        return getMediaContainer()->ctx_.reserveId();
    }

    void FFMpegPlayer::setReplay(bool _replay)
    {
        replay_ = _replay;
    }

    void FFMpegPlayer::setRestoreVolume(const int32_t _volume)
    {
        restoreVolume_ = volume_;
    }

    int32_t FFMpegPlayer::getRestoreVolume() const
    {
        return restoreVolume_;
    }

    //////////////////////////////////////////////////////////////////////////
    // MediaContainer
    //////////////////////////////////////////////////////////////////////////

    std::unique_ptr<MediaContainer> g_media_container;
    
    MediaContainer* getMediaContainer()
    {
        if (!g_media_container)
            g_media_container.reset(new MediaContainer());

        return g_media_container.get();
    }

    void ResetMediaContainer()
    {
        if (g_media_container)
            g_media_container.reset();
    }

    MediaContainer::MediaContainer()
        : videoDecodeThread_(ctx_)
        , audioDecodeThread_(ctx_)
        , demuxThread_(ctx_)
        , is_demux_inited_(false)
        , is_decods_inited_(false)
    {}

    MediaContainer::~MediaContainer()
    {
        if (ctx_.isQuit())
            return;
        setQuit();

        postDemuxThreadMessage(ThreadMessage(0, thread_message_type::tmt_wake_up), true);
        postVideoThreadMessage(ThreadMessage(0, thread_message_type::tmt_wake_up), true);
        postAudioThreadMessage(ThreadMessage(0, thread_message_type::tmt_wake_up), true);

        VideoDecodeThreadWait();
        AudioDecodeThreadWait();
        DemuxThreadWait();

        ffmpeg::av_lockmgr_register(NULL);
        ffmpeg::avformat_network_deinit();
    }

    void MediaContainer::VideoDecodeThreadStart(uint32_t _mediaId)
    {
        videoDecodeThread_.start();
    }

    void MediaContainer::AudioDecodeThreadStart(uint32_t _mediaId)
    {
        audioDecodeThread_.start();
    }

    void MediaContainer::DemuxThreadStart(uint32_t _mediaId)
    {
        if (!is_demux_inited_)
        {
            demuxThread_.start();
            is_demux_inited_ = true;
        }
    }

    void MediaContainer::stopMedia(uint32_t _mediaId)
    {
        //qDebug() << "stop media " << _mediaId;

        if (active_video_ids_.count(_mediaId) == 0)
            return;

        active_video_ids_.erase(_mediaId);

        if (active_video_ids_.size() != 0)
            return;

         if (ctx_.isQuit())
            return;

        setQuit();

        postDemuxThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);
        postVideoThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);
        postAudioThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);

        //qDebug() << "before wait " << QTime::currentTime();
        VideoDecodeThreadWait();
        AudioDecodeThreadWait();
        DemuxThreadWait();

        clearMessageQueue();

        //qDebug() << "after wait " << QTime::currentTime();

        is_decods_inited_ = false;
        is_demux_inited_ = false;

        ffmpeg::av_lockmgr_register(NULL);
        ffmpeg::avformat_network_deinit();
    }

    int32_t MediaContainer::getWidth(MediaData& _media) const
    {
        return ctx_.getWidth(_media);
    }

    int32_t MediaContainer::getHeight(MediaData& _media) const
    {
        return ctx_.getHeight(_media);
    }

    int32_t MediaContainer::getRotation(MediaData& _media) const
    {
        return ctx_.getRotation(_media);
    }

    int64_t MediaContainer::getDuration(MediaData& _media) const
    {
        return ctx_.getDuration(_media);
    }

    void MediaContainer::postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postVideoThreadMessage(_message, _forward, _clear_others);
    }
    
    void MediaContainer::updateVideoScaleSize(const uint32_t _mediaId, const QSize _sz)
    {
        ctx_.updateScaledVideoSize(_mediaId, _sz);
    }

    void MediaContainer::postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postDemuxThreadMessage(_message, _forward, _clear_others);
    }

    void MediaContainer::postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postAudioThreadMessage(_message, _forward, _clear_others);
    }

    void MediaContainer::clearMessageQueue()
    {
        ctx_.clearMessageQueue();
    }

    void MediaContainer::resetFrameTimer(MediaData& _media)
    {
        ctx_.resetFrameTimer(_media);
    }

    void MediaContainer::resetLastFramePts(MediaData& _media)
    {
        ctx_.resetLsatFramePts(_media);
    }

    bool MediaContainer::isQuit(uint32_t _mediaId) const
    {
        return ctx_.isQuit();
    }

    void MediaContainer::setQuit(bool _val)
    {
        //qDebug() << "setQuit " << _val << " " << QTime::currentTime();
        ctx_.setQuit(_val);
    }

    bool MediaContainer::openFile(MediaData& _media)
    {
        return ctx_.openFile(_media);
    }

    void MediaContainer::closeFile(MediaData& _media)
    {
        ctx_.closeFile(_media);
    }

    double MediaContainer::computeDelay(double _picturePts, MediaData& _media)
    {
        return ctx_.computeDelay(_picturePts, _media);
    }

    uint32_t MediaContainer::init(uint32_t _id)
    {
        auto id = ctx_.addVideo(_id);

        if (active_video_ids_.empty())
        {
            ffmpeg::av_init_packet(&quit_pkt);
            quit_pkt.data = (uint8_t*) &quit_pkt;

            ffmpeg::av_register_all();
            ffmpeg::avcodec_register_all();
            ffmpeg::avformat_network_init();
        }

        bool success = false;
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(id, success);
        auto& media = *media_ptr;

        ctx_.init(media);

        if (active_video_ids_.empty())
        {
            setQuit(false);
            Ui::GetSoundsManager();
        }
        active_video_ids_.insert(id);

        return id;
    }

    void MediaContainer::DemuxThreadWait()
    {
        demuxThread_.wait();
    }

    void MediaContainer::VideoDecodeThreadWait()
    {
        videoDecodeThread_.wait();
    }

    void MediaContainer::AudioDecodeThreadWait()
    {
        audioDecodeThread_.wait();
    }
}
