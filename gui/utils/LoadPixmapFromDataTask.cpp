#include "stdafx.h"

#include "../../corelib/collection_helper.h"

#include "../utils/utils.h"

#include "LoadPixmapFromDataTask.h"

namespace Utils
{
    LoadPixmapFromDataTask::LoadPixmapFromDataTask(core::istream *stream)
        : Stream_(stream)
    {
        assert(Stream_);

        Stream_->addref();
    }

    LoadPixmapFromDataTask::~LoadPixmapFromDataTask()
    {
        Stream_->release();
    }

    void LoadPixmapFromDataTask::run()
    {
        const auto size = Stream_->size();
        assert(size > 0);

        QByteArray data((const char *)Stream_->read(size), (int)size);

        QPixmap preview;
        Utils::loadPixmap(data, Out preview);

        if (preview.isNull())
        {
            emit loadedSignal(QPixmap());
            return;
        }

        emit loadedSignal(preview);
    }
}
