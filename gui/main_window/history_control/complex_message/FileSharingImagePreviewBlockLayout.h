#pragma once

#include "../../../namespaces.h"

#include "IFileSharingBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class FileSharingBlock;

class FileSharingImagePreviewBlockLayout final : public IFileSharingBlockLayout
{
public:
    FileSharingImagePreviewBlockLayout();

    virtual ~FileSharingImagePreviewBlockLayout() override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual QRect getAuthorAvatarRect() const override;

    virtual QRect getAuthorNickRect() const override;

    virtual const QRect& getContentRect() const override;

    const IItemBlockLayout::IBoxModel& getBlockBoxModel() const override;

    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

    virtual const QRect& getFilenameRect() const override;

    virtual QRect getFileSizeRect() const override;

    virtual QRect getShowInDirLinkRect() const override;

private:
    QRect PreviewRect_;

    QRect AuthorAvatarRect_;

    QRect AuthorNickRect_;

    QRect evaluateAuthorAvatarRect(const bool isStandalone) const;

    QRect evaluateAuthorNickRect(const bool isStandalone, const QRect &authorAvatarRect, const int32_t blockWidth) const;

    QRect evaluatePreviewRect(const FileSharingBlock &block, const int32_t blockWidth) const;

    void setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect);

};

UI_COMPLEX_MESSAGE_NS_END