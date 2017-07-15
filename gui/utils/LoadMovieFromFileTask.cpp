#include "stdafx.h"

#include "LoadMovieFromFileTask.h"

UTILS_NS_BEGIN

LoadMovieFromFileTask::LoadMovieFromFileTask(const QString& path)
    : Path_(path)
{
    assert(!Path_.isEmpty());
    assert(QFile::exists(Path_));
}

LoadMovieFromFileTask::~LoadMovieFromFileTask()
{
}

void LoadMovieFromFileTask::run()
{
    QSharedPointer<QMovie> movie(new QMovie(Path_));

    emit loadedSignal(movie);
}

// LoadFFMpegPlayerFromFileTask

LoadMovieToFFMpegPlayerFromFileTask::LoadMovieToFFMpegPlayerFromFileTask(const QString& path, bool _isGif, QWidget* _parent)
    : Path_(path)
    , isGif_(_isGif)
    , parent_(_parent)
{
    assert(!Path_.isEmpty());
    assert(QFile::exists(Path_));
}

LoadMovieToFFMpegPlayerFromFileTask::~LoadMovieToFFMpegPlayerFromFileTask()
{
}

void LoadMovieToFFMpegPlayerFromFileTask::run()
{
    mplayer_ = QSharedPointer<Ui::DialogPlayer>(new Ui::DialogPlayer(parent_, (isGif_ ? Ui::DialogPlayer::Flags::is_gif : 0)));

    connect(mplayer_.data(), &Ui::DialogPlayer::loaded, this, &LoadMovieToFFMpegPlayerFromFileTask::onLoaded, (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

    mplayer_->openMedia(Path_);
}

void LoadMovieToFFMpegPlayerFromFileTask::onLoaded()
{
    emit loadedSignal(mplayer_);
}

UTILS_NS_END