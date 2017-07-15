#pragma once

#include "../../../types/link_metadata.h"

#include "GenericBlock.h"
#include "../../mplayer/VideoPlayer.h"
#include "../../../utils/LoadMovieFromFileTask.h"

UI_NS_BEGIN
class ActionButtonWidget;
class TextEditEx;
UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class ImagePreviewBlockLayout;

class ImagePreviewBlock final : public GenericBlock
{
    friend class ImagePreviewBlockLayout;

    Q_OBJECT

public:
    ImagePreviewBlock(ComplexMessageItem *parent, const QString& aimId, const QString &imageUri, const QString &imageType);

    virtual ~ImagePreviewBlock() override;

    virtual void clearSelection() override;

    QPoint getActionButtonLogicalCenter() const;

    QSize getActionButtonSize() const;

    virtual IItemBlockLayout* getBlockLayout() const override;

    QSize getPreviewSize() const;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    bool hasActionButton() const;

    void hideActionButton();

    bool hasPreview() const;

    virtual bool isBubbleRequired() const override;

    virtual bool isSelected() const override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    virtual void setMaxPreviewWidth(int width) override;

    virtual int getMaxPreviewWidth() const override;

    void showActionButton(const QRect &pos);

    virtual ContentType getContentType() const { return IItemBlock::Link; }

    virtual void connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover) override;

    virtual void setQuoteSelection() override;

protected:
    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color) override;

    void drawEmptyBubble(QPainter &p, const QRect &bubbleRect);

    virtual void initialize() override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void onMenuCopyFile() override;

    virtual void onMenuCopyLink() override;

    virtual void onMenuSaveFileAs() override;

    virtual void onMenuOpenInBrowser() override;

    virtual void onRestoreResources() override;

    virtual void onUnloadResources() override;

    virtual void showEvent(QShowEvent*) override;

    virtual bool drag() override;

    virtual void resizeEvent(QResizeEvent * event) override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

private:

    TextEditEx* createTextEditControl(const QString& _text);

    void setTextEditTheme(TextEditEx *textControl);

    void connectSignals();

    void downloadFullImage(const QString &destination);

    QPainterPath evaluateClippingPath(const QRect &imageRect) const;

    void initializeActionButton();

    bool isFullImageDownloaded() const;

    bool isFullImageDownloading() const;

    bool isGifPlaying() const;

    void setGifPlaying(const bool _playing);

    bool isVideoPlaying() const;

    bool isGifPreview() const;

    bool isOverImage(const QPoint &pos) const;

    bool isVideoPreview() const;

    void onFullImageDownloaded(QPixmap image, const QString &localPath);

    void onGifLeftMouseClick();

    void onGifPlaybackStatusChanged(const bool isPlaying);

    void onGifVisibilityChanged(const bool isVisible);

    void onLeftMouseClick(const QPoint &globalPos);

    void onPreviewImageDownloaded(QPixmap image, const QString &localPath);

    void openPreviewer(QPixmap image, const QString &localPath);

    void playGif(const QString &localPath, const bool _byUser);

    void preloadFullImageIfNeeded();

    void requestSnapMetainfo();

    void schedulePreviewerOpening(const QPoint &globalPos);

    bool shouldDisplayProgressAnimation() const;

    bool isSingle() const;

    void stopDownloadingAnimation();

    void pauseGif(const bool _byUser);

    void onChangeLoadState(const bool isVisible);

    bool isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

    ImagePreviewBlockLayout *Layout_;

    QString AimId_;

    QString ImageUri_;

    QString ImageType_;

    QString FullImageLocalPath_;

    int64_t PreviewDownloadSeq_;

    QPixmap Preview_;

    Ui::TextEditEx* link_;

    QPainterPath PreviewClippingPath_;

    QPainterPath RelativePreviewClippingPath_;

    QRect PreviewClippingPathRect_;

    QString PreviewLocalPath_;

    bool PressedOverPreview_;

    int64_t FullImageDownloadSeq_;

    QSharedPointer<Ui::DialogPlayer> videoPlayer_;

    bool IsGifPlaying_;

    bool IsSelected_;

    BlockSelectionType Selection_;

    ActionButtonWidget *ActionButton_;

    QString SaveAs_;

    QPoint OpenPreviewer_;

    bool CopyFile_;

    QString DownloadUri_;

    int64_t FileSize_;

    uint64_t SnapId_;

    int64_t SnapMetainfoRequestId_;

    int MaxPreviewWidth_;

    std::unique_ptr<Utils::LoadMovieToFFMpegPlayerFromFileTask> load_task_;

    bool IsVisible_;

    bool IsInPreloadDistance_;

    std::shared_ptr<bool> ref_;

    double TextOpacity_;

    int TextFontSize_;

private Q_SLOTS:
    void onGifFrameUpdated(int frameNumber);

    void onImageDownloadError(qint64 seq, QString rawUri);

    void onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath);

    void onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred);

    void onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta);

    void onSnapMetainfoDownloaded(int64_t _seq, bool _success, uint64_t _snap_id);

    void onPaused();
};

UI_COMPLEX_MESSAGE_NS_END