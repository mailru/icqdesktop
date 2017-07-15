#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace Style
{

    int32_t getBlocksSeparatorVertMargins();
    int32_t getBlockMaxWidth();
    int getDragDistance();
    const QMargins& getDefaultBlockBubbleMargins();
    bool isBlocksGridEnabled();
    int32_t getMinFileSize4ProgressBar();

    namespace Preview
    {
        int32_t getImageHeightMax();
        int32_t getImageWidthMax();
        QBrush getImageShadeBrush();
        QSize getMinPreviewSize();
        QSizeF getMinPreviewSizeF();
        QSize getImagePlaceholderSize();
        QBrush getImagePlaceholderBrush();
    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize();
        QSize getFaviconSizeUnscaled();
        int32_t getLinkPreviewHeightMax();
        QSize getImagePreloaderSizeDip();
        QColor getSiteNameColor();
        QFont getSiteNameFont();
        QSize getSiteNamePlaceholderSize();
        QSize getTitlePlaceholderSize();
        int32_t getTitlePlaceholderTopOffset();
        int32_t getTitleTopOffset();
        int32_t getFaviconTopPadding();
        int32_t getSiteNameLeftPadding();
        int32_t getSiteNameTopPadding();
        QFont getYoutubeTitleFont();
        QBrush getPreloaderBrush();
    }

    namespace Quote
    {
        const QPen& getQuoteSeparatorPen();
        int32_t getQuoteOffsetLeft();
        int32_t getQuoteAvatarOffset();
        QSize getQuoteAvatarSize();
        QFont getQuoteFont();
        int32_t getQuoteOffsetTop();
        int32_t getQuoteOffsetBottom();
        int32_t getQuoteSpacing();
        int32_t getFirstQuoteOffset();
        int32_t getMaxImageWidthInQuote();
        int32_t getQuoteUsernameOffsetTop();
        int32_t getForwardLabelBottomMargin();
        int32_t getForwardIconOffset();
        int32_t getLineOffset();
        int32_t getQuoteBlockSpacing();
    }

    namespace Files
    {
        QPen getFileSharingFramePen();
        int32_t getFileBubbleHeight();
        QFont getFilenameFont();
        QColor getFileSizeColor();
        QFont getFileSizeFont();
        QFont getShowInDirLinkFont();
        int32_t getCtrlIconLeftMargin();
        int32_t getFilenameBaseline();
        int32_t getFilenameLeftMargin();
        int32_t getFileSizeBaseline();
        int32_t getShowInDirLinkLeftMargin();
    }

    namespace Ptt
    {
        int32_t getPttBubbleHeight();
        int32_t getCtrlButtonMarginLeft();
        int32_t getCtrlButtonMarginTop();
        int32_t getDecodedTextHorPadding();
        int32_t getDecodedTextVertPadding();
        int32_t getTextButtonMarginRight();
        int32_t getTextButtonMarginTop();
        QColor getPttProgressColor();
        int32_t getPttProgressWidth();
        QPen getDecodedTextSeparatorPen();
        QFont getDurationTextFont();
        QPen getDurationTextPen();
        QBrush getPlaybackProgressBrush();
    }

    namespace Snaps
    {
        int32_t getAuthorAvatarBottomMargin();
        int32_t getAuthorAvatarLeft(const bool isStandalone);
        int32_t getAuthorAvatarSize();
        QSize getAuthorAvatarSizeInLayout();
        int32_t getAuthorNickBaseline();
        int32_t getAuthorNickTopMargin(const bool isStandalone);
        QFont getAuthorNickFont();
        int32_t getAuthorNickLeftMargin();
        QSize getFailedSnapSizeMax();
        QBrush getFailedSnapBrush();
        QColor getFailedSnapColor();
        QFont getFailedSnapFont();
    }

}

UI_COMPLEX_MESSAGE_NS_END