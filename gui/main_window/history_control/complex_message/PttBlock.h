#pragma once

#include "../../../namespaces.h"

#include "FileSharingBlockBase.h"

UI_NS_BEGIN

class TextEditEx;

UI_NS_END

THEMES_NS_BEGIN

class IThemePixmap;

typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;

THEMES_NS_END

UI_NS_BEGIN

class ActionButtonWidget;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class PttBlockLayout;

class PttBlock final : public FileSharingBlockBase
{
    Q_OBJECT

public:
    PttBlock(
        ComplexMessageItem *_parent,
        const QString &_link,
        const int32_t _durationSec,
        int64_t _id,
        int64_t _prevId);

    virtual ~PttBlock() override;

    virtual void clearSelection() override;

    QSize getCtrlButtonSize() const;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    QSize getTextButtonSize() const;

    bool hasDecodedText() const;

    bool isDecodedTextCollapsed() const;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    void setCtrlButtonGeometry(const QRect &rect);

    void setDecodedTextGeometry(const QRect &_rect);

    int32_t setDecodedTextWidth(const int32_t _width);

    void setTextButtonGeometry(const QRect &rect);

    virtual void connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover) override;

protected:
    virtual void drawBlock(QPainter &_p, const QRect& _rect, const QColor& quate_color) override;

    virtual void initializeFileSharingBlock() override;

    virtual void onDownloadingStarted() override;

    virtual void onDownloadingStopped() override;

    virtual void onDownloaded() override;

    virtual void onDownloadedAction() override;

    virtual void onDownloading(const int64_t _bytesTransferred, const int64_t _bytesTotal) override;

    virtual void onDownloadingFailed(const int64_t requestId) override;

    virtual void onLocalCopyInfoReady(const bool isCopyExists) override;

    virtual void onMetainfoDownloaded() override;

    virtual void onPreviewMetainfoDownloaded(const QString &_miniPreviewUri, const QString &_fullPreviewUri) override;

private:
    Q_PROPERTY(int32_t PlaybackProgress READ getPlaybackProgress WRITE setPlaybackProgress);

    enum class PlaybackState;

    void connectSignals();

    void drawBubble(QPainter &_p, const QRect &_bubbleRect);

    void drawDecodedTextSeparator(QPainter &_p, const QRect &_bubbleRect);

    void drawDuration(QPainter &_p, const QRect &_ctrlButtonRect);

    void drawPlaybackProgress(QPainter &_p, const int32_t _progressMsec, const int32_t _durationMsec, const QRect &_bubbleRect);

    int32_t getPlaybackProgress() const;

    void setPlaybackProgress(const int32_t _value);

    void initializeDecodedTextCtrl();

    bool isDecodedTextVisible() const;

    bool isPaused() const;

    bool isPlaying() const;

    bool isStopped() const;

    bool isTextRequested() const;

    void renderClipPaths(const QRect &_bubbleRect);

    void requestText();

    void startTextRequestProgressAnimation();

    void stopTextRequestProgressAnimation();

    void startPlayback();

    void pausePlayback();

    void updateButtonsResourceSets();

    void updateCtrlButtonResourceSets();

    void updateTextButtonResourceSets();

    QPainterPath bubbleClipPath_;

    QRect bubbleClipRect_;

    ActionButtonWidget *ctrlButton_;

    QString decodedText_;

    TextEditEx *decodedTextCtrl_;

    int32_t durationSec_;

    QString durationText_;

    QPainterPath headerClipPath_;

    bool isDecodedTextCollapsed_;

    bool isPlayed_;

    bool isPlaybackScheduled_;

    PlaybackState playbackState_;

    int32_t playbackProgressMsec_;

    QPropertyAnimation *playbackProgressAnimation_;

    int playingId_;

    PttBlockLayout *pttLayout_;

    ActionButtonWidget *textButton_;

    int64_t textRequestId_;

    qint64 id_;

    qint64 prevId_;

private Q_SLOTS:
    void onCtrlButtonClicked();

    void onPttFinished(int _id, bool _byPlay);

    void onPttPaused(int _id);

    void onPttText(qint64 _seq, int _error, QString _text, int _comeback);

    void pttPlayed(qint64);

};

UI_COMPLEX_MESSAGE_NS_END