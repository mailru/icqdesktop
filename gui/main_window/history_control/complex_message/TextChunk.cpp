#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../gui_settings.h"

#include "FileSharingUtils.h"

#include "TextChunk.h"

const Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::Empty(Ui::ComplexMessage::TextChunk::Type::Undefined, QString(), QString(), -1);

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString& _text)
    : tokenizer_(_text.toUtf8().constData())
{
}

bool Ui::ComplexMessage::ChunkIterator::hasNext() const
{
    return tokenizer_.has_token();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::ChunkIterator::current() const
{
    const auto& token = tokenizer_.current();

    if (token.type_ == common::tools::message_token::type::text)
    {
        const auto& text = boost::get<std::string>(token.data_);
        return TextChunk(TextChunk::Type::Text, QString::fromUtf8(text.data(), text.length()), QString(), -1);
    }

    assert(token.type_ == common::tools::message_token::type::url);

    const auto& url = boost::get<common::tools::url>(token.data_);
    const auto text = QString::fromUtf8(url.original_.data(), url.original_.length());

    const bool previewsEnabled = Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
    if (url.type_ != common::tools::url::type::email && !previewsEnabled)
    {
        return TextChunk(TextChunk::Type::Text, text, QString(), -1);
    }

    switch (url.type_)
    {
    case common::tools::url::type::image:
    case common::tools::url::type::video:
        return TextChunk(TextChunk::Type::ImageLink, text, to_string(url.extension_), -1);
    case common::tools::url::type::filesharing:
    {
        const QString& id = extractIdFromFileSharingUri(text);
        const auto content_type = extractContentTypeFromFileSharingId(id);

        auto Type = TextChunk::Type::FileSharingGeneral;
        switch (content_type)
        {
        case core::file_sharing_content_type::image:
            Type = TextChunk::Type::FileSharingImage;
            break;
        case core::file_sharing_content_type::gif:
            Type = TextChunk::Type::FileSharingGif;
            break;
        case core::file_sharing_content_type::video:
            Type = TextChunk::Type::FileSharingVideo;
            break;
        case core::file_sharing_content_type::snap_image:
            Type = TextChunk::Type::FileSharingImageSnap;
            break;
        case core::file_sharing_content_type::snap_gif:
            Type = TextChunk::Type::FileSharingGifSnap;
            break;
        case core::file_sharing_content_type::snap_video:
            Type = TextChunk::Type::FileSharingVideoSnap;
            break;
        case core::file_sharing_content_type::ptt:
            Type = TextChunk::Type::FileSharingPtt;
            break;
        }

        const auto durationSec = extractDurationFromFileSharingId(id);

        return TextChunk(Type, text, QString(), durationSec);
    }
    case common::tools::url::type::site:
        {
            if (url.has_prtocol())
                return TextChunk(TextChunk::Type::GenericLink, text, QString(), -1);
            else
                return TextChunk(TextChunk::Type::Text, text, QString(), -1);
        }
    case common::tools::url::type::email:
        return TextChunk(TextChunk::Type::Text, text, QString(), -1);
    case common::tools::url::type::ftp:
        return TextChunk(TextChunk::Type::GenericLink, text, QString(), -1);
    }

    assert(!"invalid url type");
    return TextChunk(TextChunk::Type::Text, text, QString(), -1);
}

void Ui::ComplexMessage::ChunkIterator::next()
{
    tokenizer_.next();
}

Ui::ComplexMessage::TextChunk::TextChunk(
    const Type type,
    const QString& _text,
    const QString &imageType,
    const int32_t durationSec)
    : Type_(type)
    , text_(_text)
    , ImageType_(imageType)
    , DurationSec_(durationSec)
{
    assert(Type_ > Type::Min);
    assert(Type_ < Type::Max);
    assert((Type_ != Type::ImageLink) || !ImageType_.isEmpty());
    assert(DurationSec_ >= -1);
}

int32_t Ui::ComplexMessage::TextChunk::length() const
{
    return text_.length();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::mergeWith(const TextChunk &chunk) const
{
    if (Type_ != Type::Text && Type_ != TextChunk::Type::GenericLink)
    {
        return TextChunk::Empty;
    }

    if (chunk.Type_ != Type::Text && chunk.Type_ != TextChunk::Type::GenericLink)
    {
        return TextChunk::Empty;
    }

    return TextChunk(
        Type::Text,
        text_ + chunk.text_,
        QString(),
        -1);
}
