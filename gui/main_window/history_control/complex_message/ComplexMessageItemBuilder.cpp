#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"

#include "../KnownFileTypes.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlock.h"
#include "LinkPreviewBlock.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "QuoteBlock.h"
#include "StickerBlock.h"
#include "TextChunk.h"

#include "ComplexMessageItemBuilder.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

    namespace ComplexMessageItemBuilder
{
    ComplexMessageItem* makeComplexItem(
        QWidget *parent,
        const int64_t id,
        const QDate date,
        const int64_t prev,
        const QString &text,
        const QString &chatAimid,
        const QString &senderAimid,
        const QString &senderFriendly,
        const QList<Data::Quote>& quotes,
        HistoryControl::StickerInfoSptr sticker,
        const bool isOutgoing,
        const bool isNotAuth)
    {
        assert(id >= -1);
        assert(!senderAimid.isEmpty());
        assert(!senderFriendly.isEmpty());
        assert(date.isValid());

        std::unique_ptr<ComplexMessage::ComplexMessageItem> complexItem(
            new ComplexMessage::ComplexMessageItem(
            parent,
            id,
            date,
            chatAimid,
            senderAimid,
            senderFriendly,
            sticker ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : text,
            isOutgoing));

        std::list<TextChunk> chunks;

        std::unique_ptr<TextChunk> snippet;
        int snippetsCount = 0;
        bool removeSnippetOnFail = false;

        int i = 0;
        for (auto quote : quotes)
        {
            ++i;
            quote.id_ = i;
            auto text = quote.text_;

            if (quote.isSticker())
            {
                TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
                chunk.Sticker_ = HistoryControl::StickerInfo::Make(quote.setId_, quote.stickerId_);
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            if (text.startsWith(">"))
            {
                TextChunk chunk(TextChunk::Type::Text, text, QString(), -1);
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            ChunkIterator it(text);
            while (it.hasNext())
            {
                auto chunk = it.current();
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                it.next();
            }
        }


        if (sticker)
        {
            TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
            chunk.Sticker_ = sticker;
            chunks.emplace_back(std::move(chunk));
        }
        else
        {
            auto t = text.toStdString();
            ChunkIterator it(text);
            while (it.hasNext())
            {
                auto chunk = it.current();

                if (chunk.Type_ == TextChunk::Type::GenericLink)
                {
                    ++snippetsCount;
                    if (snippetsCount == 1)
                        snippet.reset(new TextChunk(chunk));

                    // if snippet is single in message or if it last, don't append link
                    it.next();
                    if (it.hasNext() || snippetsCount > 1)
                    {
                        removeSnippetOnFail = true;
                        chunks.emplace_back(std::move(chunk));
                    }

                    continue;
                }
                else
                {
                    chunks.emplace_back(std::move(chunk));
                }

                it.next();
            }
        }

        for (auto iter = chunks.begin(); iter != chunks.end(); )
        {
            auto next = iter;
            ++next;

            if (next == chunks.end())
            {
                break;
            }

            if (iter->Quote_.isEmpty())
            {
                auto merged = iter->mergeWith(*next);

                if (merged.Type_ != TextChunk::Type::Undefined)
                {
                    chunks.erase(iter);
                    *next = std::move(merged);
                }
            }
            iter = next;
        }

        IItemBlocksVec items;
        items.reserve(chunks.size());

        QuoteBlock* prevQuote = 0;
        std::vector<QuoteBlock*> quoteBlocks;

        const bool hide_links = (isNotAuth && !isOutgoing);

        int count = 0;
        for (auto iterChunk = chunks.begin(); iterChunk != chunks.end(); ++iterChunk)
        {
            const auto& chunk = *iterChunk;
            ++count;

            const auto& chunkText = chunk.text_;

            GenericBlock *block = nullptr;
            switch(chunk.Type_)
            {
            case TextChunk::Type::Text:
                block = new TextBlock(
                    complexItem.get(),
                    chunkText);
                break;

            case TextChunk::Type::GenericLink:
                block = new TextBlock(
                    complexItem.get(),
                    chunkText,
                    hide_links);
                break;

            case TextChunk::Type::ImageLink:
                if (hide_links)
                {
                    block = new TextBlock(
                        complexItem.get(),
                        chunkText,
                        true);
                }
                else
                {
                    block =  new ImagePreviewBlock(
                        complexItem.get(), 
                        chatAimid, 
                        chunkText, 
                        chunk.ImageType_);
                }
                break;

            case TextChunk::Type::FileSharingImage:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::image);
                break;

            case TextChunk::Type::FileSharingGif:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::gif);
                break;

            case TextChunk::Type::FileSharingVideo:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::video);
                break;

            case TextChunk::Type::FileSharingImageSnap:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::snap_image);
                break;

            case TextChunk::Type::FileSharingGifSnap:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::snap_gif);
                break;

            case TextChunk::Type::FileSharingVideoSnap:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::snap_video);
                break;

            case TextChunk::Type::FileSharingPtt:
                block = new PttBlock(complexItem.get(), chunkText, chunk.DurationSec_, id, prev);
                break;

            case TextChunk::Type::FileSharingGeneral:
                block = new FileSharingBlock(
                    complexItem.get(), chunkText, core::file_sharing_content_type::undefined);
                break;

            case  TextChunk::Type::Sticker:
                block = new StickerBlock(complexItem.get(), chunk.Sticker_);
                break;

            case TextChunk::Type::Junk:
                break;

            default:
                assert(!"unexpected chunk type");
                break;
            }

            if (block)
            {
                if (!chunk.Quote_.isEmpty())
                {
                    if (prevQuote && prevQuote->getQuote().id_ == chunk.Quote_.id_)
                    {
                        prevQuote->addBlock(block);
                    }
                    else
                    {
                        auto quoteBlock = new QuoteBlock(complexItem.get(), chunk.Quote_);
                        quoteBlock->addBlock(block);
                        items.emplace_back(quoteBlock);
                        quoteBlocks.push_back(quoteBlock);
                        prevQuote = quoteBlock;

                        if (!chunk.Quote_.isForward_)
                            quoteBlock->createQuoteHover(complexItem.get());
                    }
                }
                else
                {
                    for (auto q : quoteBlocks)
                    {
                        q->setReplyBlock(block);
                    }
                    items.emplace_back(block);
                }
            }
        }

        // create snippet
        if (snippetsCount == 1)
        {
            items.emplace_back(
                new LinkPreviewBlock(
                complexItem.get(), 
                snippet->text_,
                removeSnippetOnFail));
        }


        if (!quoteBlocks.empty())
        {
            QString sourceText;
            for (auto i : items)
            {
                sourceText += i->getSourceText();
            }
            complexItem->setSourceText(sourceText);
        }

        int index = 0;
        for (auto& val : quoteBlocks)
        {
            val->setMessagesCountAndIndex(count, index++);
        }

        complexItem->setItems(std::move(items));

        return complexItem.release();
    }
}

UI_COMPLEX_MESSAGE_NS_END
