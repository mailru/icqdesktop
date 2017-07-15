#include "stdafx.h"

#include "../../../cache/themes/themes.h"

#include "../../../utils/utils.h"

#include "ComplexMessageItem.h"
#include "IItemBlockLayout.h"
#include "Style.h"

#include "GenericBlock.h"
#include "../ActionButtonWidget.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int32_t getResourcesUnloadDelayMsec();
}

GenericBlock::GenericBlock(
    ComplexMessageItem *parent,
    const QString &sourceText,
    const MenuFlags menuFlags,
    const bool isResourcesUnloadingEnabled)
    : QWidget(parent)
    , QuoteAnimation_(parent)
    , Initialized_(false)
    , Parent_(parent)
    , SourceText_(sourceText)
    , MenuFlags_(menuFlags)
    , ResourcesUnloadingTimer_(nullptr)
    , IsResourcesUnloadingEnabled_(isResourcesUnloadingEnabled)
    , IsBubbleRequired_(true)
    , ClickHandled_(false)
{
    assert(Parent_);
    connect(this, SIGNAL(setQuoteAnimation()), parent, SLOT(setQuoteAnimation()));
}

GenericBlock::GenericBlock()
    : QuoteAnimation_(this)
{
    assert(!"program isn't supposed to reach this point");
}

GenericBlock::~GenericBlock()
{
}

QSize GenericBlock::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth > 0);

    return getBlockLayout()->blockSizeForMaxWidth(maxWidth);
}


void GenericBlock::deleteLater()
{
    QWidget::deleteLater();
}

QString GenericBlock::formatRecentsText() const
{
    return getSourceText();
}

ComplexMessageItem* GenericBlock::getParentComplexMessage() const
{
    assert(Parent_);
    return Parent_;
}

qint64 GenericBlock::getId() const
{
    return Parent_->getId();
}

QString GenericBlock::getSenderAimid() const
{
    return getParentComplexMessage()->getSenderAimid();
}

QString GenericBlock::getSenderFriendly() const
{
    return getParentComplexMessage()->getSenderFriendly();
}

const QString& GenericBlock::getChatAimid() const
{
    return getParentComplexMessage()->getChatAimid();
}

QString GenericBlock::getSourceText() const
{
    assert(!SourceText_.isEmpty());
    return SourceText_;
}

QString GenericBlock::getTextForCopy() const
{
    return getSourceText();
}

std::shared_ptr<const themes::theme> GenericBlock::getTheme() const
{
    return getParentComplexMessage()->getTheme();
}

int GenericBlock::getThemeId() const
{
    return getTheme()->get_id();
}

bool GenericBlock::isBubbleRequired() const
{
    return IsBubbleRequired_;
}

void GenericBlock::setBubbleRequired(bool required)
{
    IsBubbleRequired_ = required;
}

bool GenericBlock::isOutgoing() const
{
    return getParentComplexMessage()->isOutgoing();
}

bool GenericBlock::isDraggable() const
{
    return true;
}

bool GenericBlock::isSharingEnabled() const
{
    return true;
}

bool GenericBlock::isStandalone() const
{
    return getParentComplexMessage()->isSimple();
}

IItemBlock::MenuFlags GenericBlock::getMenuFlags() const
{
    return MenuFlags_;
}

bool GenericBlock::onMenuItemTriggered(const QString &command)
{
    const auto isCopyLink = (command == "copy_link");
    if (isCopyLink)
    {
        onMenuCopyLink();
        return true;
    }

    const auto isCopyFile = (command == "copy_file");
    if (isCopyFile)
    {
        onMenuCopyFile();
        return true;
    }

    const auto isSaveAs = (command == "save_as");
    if (isSaveAs)
    {
        onMenuSaveFileAs();
        return true;
    }

    const auto isOpenInBrowser = (command == "open_in_browser");
    if (isOpenInBrowser)
    {
        onMenuOpenInBrowser();
    }

    return false;
}

void GenericBlock::onActivityChanged(const bool isActive)
{
    if (!isActive)
    {
        startResourcesUnloadTimer();
        return;
    }

    stopResourcesUnloadTimer();

    onRestoreResources();

    if (Initialized_)
    {
        return;
    }

    Initialized_ = true;

    initialize();
}

void GenericBlock::onVisibilityChanged(const bool /*isVisible*/)
{
}

void GenericBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{}

QRect GenericBlock::setBlockGeometry(const QRect &ltr)
{
    auto blockLayout = getBlockLayout();
    if (!blockLayout)
    {
        return ltr;
    }

    const auto blockGeometry = blockLayout->setBlockGeometry(ltr);

    setGeometry(blockGeometry);

    return blockGeometry;
}

QSize GenericBlock::sizeHint() const
{
    const auto blockLayout = getBlockLayout();

    if (blockLayout)
    {
        return blockLayout->blockSizeHint();
    }

    return QSize(-1, 0);
}

void GenericBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quate_color)
{
    (void)p;

    if (Style::isBlocksGridEnabled())
    {
        p.setPen(Qt::red);
        p.drawRect(rect());
    }
}

void GenericBlock::initialize()
{
    setMouseTracking(true);
}

bool GenericBlock::drag()
{
    return false;
}

void GenericBlock::enterEvent(QEvent *)
{
    getParentComplexMessage()->onHoveredBlockChanged(this);
}

bool GenericBlock::isInitialized() const
{
    return Initialized_;
}

void GenericBlock::notifyBlockContentsChanged()
{
    auto blockLayout = getBlockLayout();

    if (!blockLayout)
    {
        assert(!"block layout is missing");
        return;
    }

    blockLayout->onBlockContentsChanged();

    getParentComplexMessage()->onBlockSizeChanged();

    updateGeometry();

    update();
}

void GenericBlock::onMenuCopyLink()
{
    assert(!"should be overriden in child class");
}

void GenericBlock::onMenuCopyFile()
{
    assert(!"should be overriden in child class");
}

void GenericBlock::onMenuSaveFileAs()
{
    assert(!"should be overriden in child class");
}

void GenericBlock::onMenuOpenInBrowser()
{

}

void GenericBlock::onRestoreResources()
{

}

void GenericBlock::onUnloadResources()
{

}

void GenericBlock::paintEvent(QPaintEvent *e)
{
    if (!isInitialized())
    {
        return;
    }

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::NoBrush);

    drawBlock(p, e->rect(), QuoteAnimation_.quoteColor());

    if (Style::isBlocksGridEnabled())
    {
        p.setPen(Qt::red);
        p.drawRect(rect());
    }

    QWidget::paintEvent(e);
}

void GenericBlock::mouseMoveEvent(QMouseEvent *e)
{
    const auto isLeftButtonPressed = ((e->buttons() & Qt::LeftButton) != 0);
    const auto beginDrag = (isLeftButtonPressed && isDraggable());
    if (beginDrag)
    {
        auto pos = e->pos();
        if (mousePos_.isNull())
        {
            mousePos_ = pos;
        }
        else if (!mousePos_.isNull() && (abs(mousePos_.x() - pos.x()) > Style::getDragDistance() || abs(mousePos_.y() - pos.y()) > Style::getDragDistance()))
        {
            mousePos_ = QPoint();
            drag();
            return;
        }
    }
    else
    {
        mousePos_ = QPoint();
    }
    return QWidget::mouseMoveEvent(e);
}

void GenericBlock::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        mouseClickPos_ = e->pos();

    return QWidget::mousePressEvent(e);
}

void GenericBlock::mouseReleaseEvent(QMouseEvent *e)
{
    if (!ClickHandled_ && mouseClickPos_.x() == e->pos().x() && mouseClickPos_.y() == e->pos().y())
        emit clicked();

    ClickHandled_ = false;
    return QWidget::mouseReleaseEvent(e);
}

void Ui::ComplexMessage::GenericBlock::connectButtonToHover(ActionButtonWidget* btn, Ui::ComplexMessage::QuoteBlockHover* hover)
{
    if (btn && hover)
    {
        btn->installEventFilter(hover);
        btn->setMouseTracking(true);
    }
}

void GenericBlock::startResourcesUnloadTimer()
{
    if (!IsResourcesUnloadingEnabled_)
    {
        return;
    }

    if (ResourcesUnloadingTimer_)
    {
        ResourcesUnloadingTimer_->start();
        return;
    }

    ResourcesUnloadingTimer_ = new QTimer(this);
    ResourcesUnloadingTimer_->setSingleShot(true);
    ResourcesUnloadingTimer_->setInterval(getResourcesUnloadDelayMsec());

    auto success = QObject::connect(
        ResourcesUnloadingTimer_,
        &QTimer::timeout,
        this,
        &GenericBlock::onResourceUnloadingTimeout);
    assert(success);

    ResourcesUnloadingTimer_->start();
}

void GenericBlock::stopResourcesUnloadTimer()
{
    if (!ResourcesUnloadingTimer_)
    {
        return;
    }

    ResourcesUnloadingTimer_->stop();
}

void GenericBlock::onResourceUnloadingTimeout()
{
    onUnloadResources();
}

void GenericBlock::setQuoteSelection()
{
	QuoteAnimation_.startQuoteAnimation();
}

void GenericBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    if (hover)
    {
        installEventFilter(hover);
        raise();
    }
}

void GenericBlock::hideBlock()
{
    hide();
}

bool GenericBlock::isHasLinkInMessage() const
{
    return false;
}

namespace
{
    int32_t getResourcesUnloadDelayMsec()
    {
        return (build::is_debug() ? 1000 : 5000);
    }
}

UI_COMPLEX_MESSAGE_NS_END