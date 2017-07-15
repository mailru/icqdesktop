#pragma once

#include "../MessagesModel.h"

#include "../../../common.shared/message_processing/message_tokenizer.h"

namespace Ui
{
    namespace ComplexMessage
    {
        struct TextChunk
        {
            enum class Type
            {
                Invalid,

                Min,

                Undefined,
                Text,
                GenericLink,
                ImageLink,
                Junk,
                FileSharingImage,
                FileSharingGif,
                FileSharingVideo,
                FileSharingImageSnap,
                FileSharingGifSnap,
                FileSharingVideoSnap,
                FileSharingPtt,
                FileSharingGeneral,
                Sticker,

                Max
            };

            static const TextChunk Empty;

            TextChunk(
                const Type type, 
                const QString& _text, 
                const QString &imageType, 
                const int32_t durationSec);

            int32_t length() const;

            TextChunk mergeWith(const TextChunk &chunk) const;

            Type Type_;

            QString text_;

            QString ImageType_;

            int32_t DurationSec_;

            Data::Quote Quote_;

            HistoryControl::StickerInfoSptr Sticker_;
        };

        class ChunkIterator final
        {
        public:
            explicit ChunkIterator(const QString& _text);

            bool hasNext() const;
            TextChunk current() const;
            void next();

        private:
            common::tools::message_tokenizer tokenizer_;
        };
    }
}
