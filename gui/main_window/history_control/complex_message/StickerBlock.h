#pragma once

#include "GenericBlock.h"
#include "StickerBlockLayout.h"
#include "../StickerInfo.h"
#include "../../../themes/ThemePixmap.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class StickerBlock final : public GenericBlock
{
    friend class StickerBlockLayout;

    Q_OBJECT

public:
    StickerBlock(ComplexMessageItem *parent, const HistoryControl::StickerInfoSptr& _info);

    virtual ~StickerBlock() override;

    virtual void clearSelection() override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    virtual bool isSelected() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual QRect setBlockGeometry(const QRect &ltr) override;

    virtual QString getSourceText() const override;

    virtual QString formatRecentsText() const override;

    virtual bool isBubbleRequired() const override { return false; }

    virtual bool isSharingEnabled() const override { return false; }

    virtual HistoryControl::StickerInfoSptr getStickerInfo() const override { return Info_; };

protected:
    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color) override;

    virtual void initialize() override;

private Q_SLOTS:
    void onSticker(qint32 setId, qint32 stickerId);

    void onStickers();

private:
    void connectStickerSignal(const bool _isConnected);

    void loadSticker();

    void renderSelected(QPainter& _p);

    void requestSticker();

    void updateStickerSize();

    const HistoryControl::StickerInfoSptr Info_;

    QImage Sticker_;

    Themes::IThemePixmapSptr Placeholder_;

    StickerBlockLayout *Layout_;

    ComplexMessageItem* Parent_;

    QSize LastSize_;

    bool IsSelected_;

    QRect Geometry_;
};

UI_COMPLEX_MESSAGE_NS_END
