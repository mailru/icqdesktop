#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/utils.h"

#include "ImageLoader.h"

Previewer::ImageLoader::ImageLoader(const QString& _aimId, const Data::Image& _image)
    : aimId_(_aimId)
    , image_(_image)
    , seq_(0)
    , state_(State::ReadyToLoad)
{
}

Previewer::ImageLoader::ImageLoader(const QString& _aimId, const Data::Image& _image, const QString& _localPath)
    : aimId_(_aimId)
    , image_(_image)
    , seq_(0)
    , state_(State::Success)
    , localFileName_(_localPath)
{
    if (!Utils::loadPixmap(_localPath, pixmap_))
    {
        load();
    }
}

void Previewer::ImageLoader::load()
{
    assert(getState() != State::Loading);

    if (image_.is_filesharing_)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded,
            this, &ImageLoader::onSharedFileDownloaded, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingError,
            this, &ImageLoader::onSharedFileDownloadError, Qt::QueuedConnection);
    }
    else
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded,
            this, &ImageLoader::onImageDownloaded, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloadError,
            this, &ImageLoader::onImageDownloadError, Qt::QueuedConnection);
    }

    start();
}

void Previewer::ImageLoader::cancelLoading() const
{
    if (getState() != State::Loading)
        return;

    core::coll_helper helper(Ui::GetDispatcher()->create_collection(), true);
    helper.set_value_as_string("contact", aimId_.toStdString());
    helper.set_value_as_string("url", image_.url_.toStdString());
    helper.set_value_as_int64("process_seq", seq_);
    Ui::GetDispatcher()->post_message_to_core("files/download/abort", helper.get());
}

Previewer::ImageLoader::State Previewer::ImageLoader::getState() const
{
    return state_;
}

QString Previewer::ImageLoader::getFileName()
{
    if (image_.is_filesharing_)
    {
        QEventLoop wait;
        connect(this, &Previewer::ImageLoader::metainfoLoaded, &wait, &QEventLoop::quit);

        const auto seq = Ui::GetDispatcher()->downloadFileSharingMetainfo(image_.url_);

        QString fileName;
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded, this,
            [this, seq, &fileName](qint64 s, QString name, QString /*downloadUri*/, qint64 /*size*/)
            {
                if (seq == s)
                {
                    fileName = name;
                    emit metainfoLoaded();
                }
            });

        wait.exec();

        return fileName;
    }

    QUrl urlParser(image_.url_);
    return urlParser.fileName();
}

const QPixmap& Previewer::ImageLoader::getPixmap() const
{
    return pixmap_;
}

const QString& Previewer::ImageLoader::getLocalFileName() const
{
    return localFileName_;
}

void Previewer::ImageLoader::start()
{
    setState(State::Loading);

    if (image_.is_filesharing_)
    {
        seq_ = Ui::GetDispatcher()->downloadSharedFile(aimId_, image_.url_, false, QString(), true);
    }
    else
    {
        seq_ = Ui::GetDispatcher()->downloadImage(image_.url_, aimId_, QString(), false, 0, 0);
    }
}

void Previewer::ImageLoader::setState(State _newState)
{
    state_ = _newState;
}

void Previewer::ImageLoader::onCancelLoading()
{
    cancelLoading();
}

void Previewer::ImageLoader::onTryAgain()
{
    start();
}

void Previewer::ImageLoader::onSharedFileDownloaded(qint64 _seq, QString /*_url*/, QString _local)
{
    if (seq_ == _seq)
    {
        setState(State::Success);
        localFileName_ = _local;
        emit imageLoaded();
    }
}

void Previewer::ImageLoader::onSharedFileDownloadError(qint64 _seq, QString /*_url*/, qint32 /*_errorCode*/)
{
    if (seq_ == _seq)
    {
        setState(State::Error);
        emit imageLoadingError();
    }
}

void Previewer::ImageLoader::onImageDownloaded(qint64 _seq, QString _url, QPixmap _image, QString _local)
{
    if (seq_ == _seq)
    {
        setState(State::Success);
        pixmap_ = _image;
        localFileName_ = _local;
        emit imageLoaded();
    }
}

void Previewer::ImageLoader::onImageDownloadError(qint64 _seq, QString /*_url*/)
{
    if (seq_ == _seq)
    {
        setState(State::Error);
        emit imageLoadingError();
    }
}
