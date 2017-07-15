#include "stdafx.h"

#include "strings.h"

#include "url.h"

std::string core::tools::encode_url(const std::string& _url)
{
    std::stringstream result;

    result << std::hex;
    result << std::uppercase;

    for (auto it = std::begin(_url), end = std::end(_url); it != end; ++it)
    {
        const auto c = *it;
        const auto n = utf8_char_size(c);

        if (n == 1)
        {
            result << c;
        }
        else
        {
            for (auto i = 0; i < n; ++i)
            {
                const auto code = reinterpret_cast<const unsigned char&>(*it);
                result << '%' << std::setw(2) << static_cast<unsigned>(code);
                ++it;
            }

            --it;
        }
    }

    return result.str();
}
