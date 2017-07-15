#include "stdafx.h"

#include "ComplexMessageUtils.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QSize limitSize(const QSize &size, const QSize &maxSize)
{
    assert(!size.isEmpty());
    assert((maxSize.width() > 0) || (maxSize.height() > 0));

    auto result(size);

    const auto maxWidth = maxSize.width();
    assert(maxWidth >= 0);

    if (maxWidth > 0)
    {
        const auto needScaleDownWidth = (result.width() > maxWidth);
        if (needScaleDownWidth)
        {
            const auto scaleDownK = ((double)maxWidth / (double)(result.width()));
            result *= scaleDownK;
        }
    }

    const auto maxHeight = maxSize.height();
    assert(maxHeight >= 0);

    if (maxHeight > 0)
    {
        const auto needScaleDownHeight = (result.height() > maxHeight);
        if (needScaleDownHeight)
        {
            const auto scaleDownK = ((double)maxHeight / (double)(result.height()));
            result *= scaleDownK;
        }
    }

    return result;
}

UI_COMPLEX_MESSAGE_NS_END