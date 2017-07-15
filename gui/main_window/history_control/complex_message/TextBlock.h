#pragma once

#include "GenericBlock.h"

UI_NS_BEGIN

class TextEditEx;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class TextBlockLayout;

class TextBlock : public GenericBlock
{
    friend class TextBlockLayout;

    Q_OBJECT

Q_SIGNALS:
    void selectionChanged();
    void setTextEditEx(TextEditEx*);

public:
    TextBlock(ComplexMessageItem *parent, const QString &text, const bool _hideLinks = false);

    virtual ~TextBlock() override;

    virtual void clearSelection() override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    virtual bool isDraggable() const override;

    virtual bool isSelected() const override;

    virtual bool isSharingEnabled() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual void setFontSize(int size) override;

    virtual void setTextOpacity(double opacity) override;

    virtual ContentType getContentType() const { return IItemBlock::Text; }

    virtual QString getTrimmedText() const { return TrimmedText_; }

    virtual void connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover);

    virtual bool isBubbleRequired() const override;

protected:
    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color) override;

    virtual void initialize() override;

private:
    TextEditEx* createTextEditControl(const QString &text);

    void setTextEditTheme(TextEditEx *textControl);

    QString Text_;

    QString TrailingSpaces_;

    QString TrimmedText_;

    TextBlockLayout *Layout_;

    TextEditEx *TextCtrl_;

    BlockSelectionType Selection_;

    int TextFontSize_;

    double TextOpacity_;

    const bool hideLinks_;

private Q_SLOTS:
    void onAnchorClicked(const QUrl &_url);
};

UI_COMPLEX_MESSAGE_NS_END
