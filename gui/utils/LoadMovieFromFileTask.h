#pragma once

#include "../namespaces.h"
#include "../main_window/mplayer/VideoPlayer.h"

UTILS_NS_BEGIN

class LoadMovieFromFileTask
    : public QObject
    , public QRunnable
{
    Q_OBJECT

Q_SIGNALS:
    void loadedSignal(QSharedPointer<QMovie> movie);

public:
    explicit LoadMovieFromFileTask(const QString& path);

    virtual ~LoadMovieFromFileTask();

    void run();

private:
    const QString Path_;

};

class LoadMovieToFFMpegPlayerFromFileTask
    : public QObject
    , public QRunnable
{
    Q_OBJECT

Q_SIGNALS:
    void loadedSignal(QSharedPointer<Ui::DialogPlayer> _player);

public Q_SLOTS:
    void onLoaded();

public:
    explicit LoadMovieToFFMpegPlayerFromFileTask(const QString& path, bool _isGif, QWidget* _parent);

    virtual ~LoadMovieToFFMpegPlayerFromFileTask();

    void run();

private:
    const QString Path_;
    QSharedPointer<Ui::DialogPlayer> mplayer_;
    bool isGif_;
    QWidget* parent_;

};

UTILS_NS_END