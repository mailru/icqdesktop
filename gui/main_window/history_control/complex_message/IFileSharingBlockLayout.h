#pragma once

#include "../../../namespaces.h"

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout : public GenericBlockLayout
{
public:
    IFileSharingBlockLayout();

    virtual ~IFileSharingBlockLayout() override;

    virtual QRect getAuthorAvatarRect() const = 0;

    virtual QRect getAuthorNickRect() const = 0;

    virtual const QRect& getContentRect() const = 0;

    virtual const QRect& getFilenameRect() const = 0;

    virtual QRect getFileSizeRect() const = 0;

    virtual QRect getShowInDirLinkRect() const = 0;

};

UI_COMPLEX_MESSAGE_NS_END