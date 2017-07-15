#pragma once

#include "../../../namespaces.h"

#include "IFileSharingBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class PttBlock;

class PttBlockLayout : public IFileSharingBlockLayout
{
public:
    PttBlockLayout();

    virtual ~PttBlockLayout() override;

    virtual QRect getAuthorAvatarRect() const override;

    virtual QRect getAuthorNickRect() const override;

    virtual const QRect& getContentRect() const override;

    const QRect& getCtrlButtonRect() const;

    const QRect& getTextButtonRect() const;

    int32_t getDecodedTextSeparatorY() const;

    virtual const QRect& getFilenameRect() const override;

    virtual QRect getFileSizeRect() const override;

    virtual QRect getShowInDirLinkRect() const override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

    QRect setCtrlButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry);

    QSize setDecodedTextHorGeometry(PttBlock &pttBlock, const int32_t bubbleWidth);

    void setDecodedTextGeometry(PttBlock &_pttBlock, const QRect &_contentRect, const QSize &_textSize);

    QRect setTextButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry);

private:
    QRect contentRect_;

    QRect ctrlButtonRect_;

    QRect textButtonRect_;

};

UI_COMPLEX_MESSAGE_NS_END