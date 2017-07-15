#pragma once

#include "IFileSharingBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class FileSharingBlock;

class FileSharingPlainBlockLayout final : public IFileSharingBlockLayout
{
public:
    FileSharingPlainBlockLayout();

    virtual ~FileSharingPlainBlockLayout() override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual QRect getAuthorAvatarRect() const override;

    virtual QRect getAuthorNickRect() const override;

    virtual const QRect& getContentRect() const override;

    const QRect& getFilenameRect() const override;

    virtual QRect getFileSizeRect() const override;

    virtual QRect getShowInDirLinkRect() const override;

    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

private:
    QRect setCtrlButtonGeometry(FileSharingBlock &block, const QRect &contentRect);

    QRect ContentRect_;

    QRect FilenameRect_;

    QRect FileSizeRect_;

    QRect ShowInDirLinkRect_;

};

UI_COMPLEX_MESSAGE_NS_END