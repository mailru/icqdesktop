#pragma once

#include "../../../namespaces.h"
#include "../../../types/message.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class ComplexMessageItem;
class IItemBlockLayout;

class IItemBlock
{
public:
    enum MenuFlags
    {
        MenuFlagNone            = 0,

        MenuFlagLinkCopyable    = (1 << 0),
        MenuFlagFileCopyable    = (1 << 1),
        MenuFlagOpenInBrowser   = (1 << 2),
    };

    enum ContentType
    {
        Other = 0,
        Text = 1,
        FileSharing = 2,
        Link = 3,
        Quote = 4,
    };

    virtual ~IItemBlock() = 0;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) = 0;

    virtual void clearSelection() = 0;

    virtual void deleteLater() = 0;

    virtual bool drag() = 0;

    virtual QString formatRecentsText() const = 0;

    virtual bool isDraggable() const = 0;

    virtual bool isSharingEnabled() const = 0;

    virtual bool containSharingBlock() const { return false; }
    
    virtual bool standaloneText() const = 0;

    virtual void onActivityChanged(const bool isActive) = 0;

    virtual void onVisibilityChanged(const bool isVisible) = 0;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) = 0;

    virtual QRect setBlockGeometry(const QRect &ltr) = 0;

    virtual IItemBlockLayout* getBlockLayout() const = 0;

    virtual MenuFlags getMenuFlags() const = 0;

    virtual ComplexMessageItem* getParentComplexMessage() const = 0;

    virtual QString getSelectedText(bool isFullSelect = false) const = 0;

    virtual QString getSourceText() const = 0;

    virtual QString getTextForCopy() const = 0;

    virtual bool isBubbleRequired() const = 0;

    virtual bool isSelected() const = 0;

    virtual bool onMenuItemTriggered(const QString &command) = 0;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) = 0;
    
    virtual bool replaceBlockWithSourceText(IItemBlock* /*block*/) { return false; }

    virtual bool isSimple() const { return true; }

    virtual Data::Quote getQuote() const;

    virtual HistoryControl::StickerInfoSptr getStickerInfo() const;

    virtual bool needFormatQuote() const { return true; }

    virtual void setFontSize(int size) { };

    virtual void setTextOpacity(double opacity) { };

    virtual IItemBlock* findBlockUnder(const QPoint &pos) const { return nullptr; }

    virtual ContentType getContentType() const { return Other; }

    virtual QString getTrimmedText() const { return QString(); }

	virtual void setQuoteSelection() = 0;

    virtual void hideBlock() = 0;

    virtual bool isHasLinkInMessage() const = 0;

    virtual int getMaxWidth() const { return -1; }

};

typedef std::vector<IItemBlock*> IItemBlocksVec;

UI_COMPLEX_MESSAGE_NS_END