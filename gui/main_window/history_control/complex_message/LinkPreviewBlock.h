#pragma once

#include "GenericBlock.h"

#include "../../../types/link_metadata.h"

UI_NS_BEGIN

class ActionButtonWidget;
class TextEditEx;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class EmbeddedPreviewWidgetBase;
class ILinkPreviewBlockLayout;

class LinkPreviewBlock final : public GenericBlock
{
    friend class LinkPreviewBlockLayout;

    Q_OBJECT

public:
    LinkPreviewBlock(ComplexMessageItem *parent, const QString &uri, const bool _hasLinkInMessage);

    virtual ~LinkPreviewBlock() override;

    virtual void clearSelection() override;

    QPoint getActionButtonLogicalCenter() const;

    QSize getActionButtonSize() const;

    virtual IItemBlockLayout* getBlockLayout() const override;

    const QString& getDescription() const;

    QSize getFaviconSizeUnscaled() const;

    QSize getPreviewImageSize() const;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    virtual QString getTextForCopy() const override;

    const QString& getSiteName() const;

    const QFont& getSiteNameFont() const;

    int32_t getTitleTextHeight() const;

    bool hasActionButton() const;

    bool hasTitle() const;

    void hideActionButton();

    bool isInPreloadingState() const;

    virtual bool isSelected() const override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    void setTitleGeometry(const QRect &geometry);

    void setTitleWidth(const int32_t width);

    void showActionButton(const QRect &pos);

    virtual void setMaxPreviewWidth(int width) override;

    virtual int getMaxPreviewWidth() const override;

    virtual void setFontSize(int size) override;

    virtual void setTextOpacity(double opacity) override;

    virtual ContentType getContentType() const { return IItemBlock::Link; }

    virtual void connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover) override;

    virtual void setQuoteSelection() override;

    virtual bool isHasLinkInMessage() const override;

    virtual int getMaxWidth() const override;

protected:
    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color) override;

    virtual void initialize() override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void showEvent(QShowEvent *event) override;

    virtual void onMenuCopyLink() override;

    virtual bool drag() override;

private Q_SLOTS:
    void onLinkMetainfoMetaDownloaded(int64_t seq, bool success, Data::LinkMetadata meta);

    void onLinkMetainfoFaviconDownloaded(int64_t seq, bool success, QPixmap image);

    void onLinkMetainfoImageDownloaded(int64_t seq, bool success, QPixmap image);

    void onSnapMetainfoDownloaded(int64_t seq, bool success, uint64_t snap_id);

private:
    Q_PROPERTY(int PreloadingTicker READ getPreloadingTickerValue WRITE setPreloadingTickerValue);

    bool createDescriptionControl(const QString &description);

    void createTextControls(const QRect &blockGeometry);

    bool createTitleControl(const QString &title);

    int getPreloadingTickerValue() const;

    void requestSnapMetainfo();

    void setPreloadingTickerValue(const int32_t _val);

    Ui::TextEditEx *Title_;

    Ui::TextEditEx *Annotation_;

    QPixmap FavIcon_;

    QString SiteName_;

    QString Uri_;

    int64_t RequestId_;

    std::unique_ptr<ILinkPreviewBlockLayout> Layout_;

    QPixmap PreviewImage_;

    QSize PreviewSize_;

    QRect previewClippingPathRect_;

    QPainterPath previewClippingPath_;

    QPropertyAnimation *PreloadingTickerAnimation_;

    int32_t PreloadingTickerValue_;

    QPainterPath Bubble_;

    int32_t Time_;

    QFont SiteNameFont_;

    int64_t SnapMetainfoRequestId_;

    bool PressedOverSiteLink_;

    bool MetaDownloaded_;

    bool ImageDownloaded_;

    bool FaviconDownloaded_;

    bool IsSelected_;

    ActionButtonWidget *ActionButton_;

    QString ContentType_;

    Data::LinkMetadata Meta_;

    uint64_t SnapId_;

    int TextFontSize_;

    double TextOpacity_;

    int MaxPreviewWidth_;

    const bool hasLinkInMessage_;

    void connectSignals(const bool isConnected);

    enum class TextOptions
    {
        PlainText,
        ClickableLinks
    };

    Ui::TextEditEx* createTextEditControl(const QString &text, const QFont &font, TextOptions options);

    void drawFavicon(QPainter &p);

    void drawPreloader(QPainter &p);

    void drawPreview(QPainter &p);

    void drawSiteName(QPainter &p);

    QPainterPath evaluateClippingPath(const QRect &imageRect) const;

    void initializeActionButton();

    bool isOverSiteLink(const QPoint p) const;

    bool isTitleClickable() const;

    void scalePreview(QPixmap &image);

    QSize scalePreviewSize(const QSize &size) const;

    void updateRequestId();
};

UI_COMPLEX_MESSAGE_NS_END