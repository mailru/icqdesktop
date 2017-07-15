#pragma once

#include "GenericBlock.h"
#include "QuoteBlockLayout.h"
#include "../../../types/message.h"

UI_NS_BEGIN

class TextEmojiWidget;
class PictureWidget;
class ContactAvatarWidget;
class TextEditEx;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class QuoteBlock;

class QuoteBlockHoverPainter : public QWidget
{
	Q_OBJECT

public:
	QuoteBlockHoverPainter(QWidget* parent);

	void startAnimation(qreal begin, qreal end);
	void startAnimation(qreal end);

protected:
	virtual void paintEvent(QPaintEvent * e) override;

private:
	qreal Opacity_;

private slots:
	void onOpacityChanged(qreal o);
};

/////////////////////////////////////////////////////////////////////
class QuoteBlockHover : public QWidget
{
	Q_OBJECT

public:
	QuoteBlockHover(QuoteBlockHoverPainter* painter, QWidget* parent, QuoteBlock* block);
    virtual bool eventFilter(QObject * obj, QEvent * e) override;

protected:
    virtual void mouseMoveEvent(QMouseEvent * e) override;
	virtual void mousePressEvent(QMouseEvent * e) override;
	virtual void mouseReleaseEvent(QMouseEvent * e) override;

	QuoteBlockHoverPainter* Painter_;
	QuoteBlock*				Block_;
    Ui::TextEditEx*         Text_;
    bool                    bAnchorClicked_;

signals:
	void openMessage();
	void openAvatar();
	void contextMenu(const QPoint &globalPos);

public slots:
    void onEventFilterRequest(QWidget*);
    void onSetTextEditEx(Ui::TextEditEx*);
    void onLeave();

private slots:
    void onAnchorClicked(const QUrl&);
};

/////////////////////////////////////////////////////////////////////
class QuoteBlock final : public GenericBlock
{
    friend class QuoteBlockLayout;

    Q_OBJECT

public:
    QuoteBlock(ComplexMessageItem *parent, const Data::Quote & quote);

    virtual ~QuoteBlock() override;

    virtual void clearSelection() override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    virtual bool isSelected() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    virtual QRect setBlockGeometry(const QRect &ltr) override;
    
    virtual void onActivityChanged(const bool isActive) override;
    
    virtual bool replaceBlockWithSourceText(IItemBlock *block) override;
    
    virtual bool isSharingEnabled() const override;

    virtual bool containSharingBlock() const override;
    
    virtual QString getSourceText() const override;
    
    virtual QString formatRecentsText() const override;
    
    virtual bool standaloneText() const override;

    virtual bool isSimple() const override { return false; }

    virtual Data::Quote getQuote() const override;

    virtual bool needFormatQuote() const override { return false; }

    virtual IItemBlock* findBlockUnder(const QPoint &pos) const;

    void addBlock(GenericBlock* block);

    bool needForwardBlock() const;

    void setReplyBlock(GenericBlock* block);

    virtual ContentType getContentType() const { return IItemBlock::Quote; }

	void createQuoteHover(ComplexMessage::ComplexMessageItem* complex_item);

	void setMessagesCountAndIndex(int count, int index);

protected:
    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color) override;

    virtual void initialize() override;

private:
    bool quoteOnly() const;

private Q_SLOTS:
    void blockClicked();

Q_SIGNALS:
    void observeToSize();

private:

    Data::Quote Quote_;

    QuoteBlockLayout *Layout_;

    std::vector<GenericBlock*> Blocks_;

    QLabel* ForwardLabel_;

    TextEmojiWidget *TextCtrl_;

    ContactAvatarWidget* Avatar_;
    
    QRect Geometry_;

    BlockSelectionType Selection_;
    
    ComplexMessageItem* Parent_;

    GenericBlock* ReplyBlock_;

    PictureWidget* ForwardIcon_;

	QuoteBlockHoverPainter* QuoteHoverPainter_;

	QuoteBlockHover* QuoteHover_;

	int MessagesCount_;

	int MessageIndex_;
};

UI_COMPLEX_MESSAGE_NS_END
