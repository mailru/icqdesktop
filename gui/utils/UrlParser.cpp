#include "stdafx.h"

#include "UrlParser.h"

namespace
{
    static int32_t utf8_char_size(char _s)
    {
        const unsigned char b = *reinterpret_cast<unsigned char*>(&_s);

        if ((b & 0xE0) == 0xC0) return 2;
        if ((b & 0xF0) == 0xE0) return 3;
        if ((b & 0xF8) == 0xF0) return 4;
        if ((b & 0xFC) == 0xF8) return 5;
        if ((b & 0xFE) == 0xFC) return 6;

        return 1;
    }
}

Utils::UrlParser::UrlParser()
    : charsProcessed_(0)
{
}

void Utils::UrlParser::process(const QStringRef& _text)
{
    auto utf8text = _text.toUtf8();
    int size = 0;

    int utf8counter = 0;

    for (auto& c : utf8text)
    {
        ++size;

        if (utf8counter == 0)
        {
            utf8counter = utf8_char_size(c);
        }

        parser_.process(c);

        --utf8counter;

        if (utf8counter == 0)
        {
            if (parser_.skipping_chars())
                goto finish;

            if (parser_.has_url())
                goto finish;
        }
    }

    parser_.finish();

finish:
    if (hasUrl())
    {
        const auto& url = parser_.get_url();
        charsProcessed_ = QString::fromUtf8(url.original_.c_str(), url.original_.size()).length();
    }
    else
    {
        charsProcessed_ = QString::fromUtf8(utf8text, size).length();
    }
}

bool Utils::UrlParser::hasUrl() const
{
    return parser_.has_url();
}

const common::tools::url& Utils::UrlParser::getUrl() const
{
    assert(parser_.has_url());
    return parser_.get_url();
}

QString::size_type Utils::UrlParser::charsProcessed() const
{
    return charsProcessed_;
}

void Utils::UrlParser::reset()
{
    charsProcessed_ = 0;
    parser_.reset();
}
