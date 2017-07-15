#include "stdafx.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

#include "../../utils/utils.h"

#include "ActionButtonWidgetLayout.h"
#include "MessageStyle.h"

#include "ActionButtonWidget.h"

UI_NS_BEGIN

namespace
{
    const auto PROGRESS_BAR_ANGLE_MAX = 360;

    const auto PROGRESS_BAR_MIN_PERCENTAGE = 0.03;

    int32_t getMaxIconDimension();
}

ActionButtonWidget::ResourceSet::ResourceSet(
    const ResId startButtonImage,
    const ResId startButtonImageActive,
    const ResId startButtonImageHover,
    const ResId stopButtonImage,
    const ResId stopButtonImageActive,
    const ResId stopButtonImageHover)
    : StartButtonImage_(startButtonImage)
    , StartButtonImageHover_(startButtonImageHover)
    , StartButtonImageActive_(startButtonImageActive)
    , StopButtonImage_(stopButtonImage)
    , StopButtonImageHover_(stopButtonImageHover)
    , StopButtonImageActive_(stopButtonImageActive)
{
    assert(StartButtonImage_ > ResId::Min);
    assert(StartButtonImage_ < ResId::Max);
    assert(StartButtonImageHover_ > ResId::Min);
    assert(StartButtonImageHover_ < ResId::Max);
    assert(StartButtonImageActive_ > ResId::Min);
    assert(StartButtonImageActive_ < ResId::Max);
    assert(StopButtonImage_ > ResId::Min);
    assert(StopButtonImage_ < ResId::Max);
    assert(StopButtonImageHover_ > ResId::Min);
    assert(StopButtonImageHover_ < ResId::Max);
    assert(StopButtonImageActive_ > ResId::Min);
    assert(StopButtonImageActive_ < ResId::Max);
}

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::Play_(
    Themes::PixmapResourceId::FileSharingMediaPlay,
    Themes::PixmapResourceId::FileSharingMediaPlayHover,
    Themes::PixmapResourceId::FileSharingMediaPlayHover,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::Gif_(
    Themes::PixmapResourceId::FileSharingGifPlay,
    Themes::PixmapResourceId::FileSharingGifPlay,
    Themes::PixmapResourceId::FileSharingGifPlay,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::DownloadPlainFile_(
    Themes::PixmapResourceId::FileSharingDownload,
    Themes::PixmapResourceId::FileSharingDownload,
    Themes::PixmapResourceId::FileSharingDownload,
    Themes::PixmapResourceId::FileSharingPlainCancel,
    Themes::PixmapResourceId::FileSharingPlainCancel,
    Themes::PixmapResourceId::FileSharingPlainCancel);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::DownloadMediaFile_(
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel,
    Themes::PixmapResourceId::FileSharingMediaCancel);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::ShareContent_(
    Themes::PixmapResourceId::FileSharingShareContent,
    Themes::PixmapResourceId::FileSharingShareContentHover,
    Themes::PixmapResourceId::FileSharingShareContentHover,
    Themes::PixmapResourceId::FileSharingShareContent,
    Themes::PixmapResourceId::FileSharingShareContent,
    Themes::PixmapResourceId::FileSharingShareContent);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::PttPause_(
    Themes::PixmapResourceId::FileSharingPttPause,
    Themes::PixmapResourceId::FileSharingPttPauseActive,
    Themes::PixmapResourceId::FileSharingPttPauseHover,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon48);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::PttPlayGray_(
    Themes::PixmapResourceId::FileSharingPttPlayDone,
    Themes::PixmapResourceId::FileSharingPttPlayDoneActive,
    Themes::PixmapResourceId::FileSharingPttPlayDoneHover,
    Themes::PixmapResourceId::FileSharingPttPause,
    Themes::PixmapResourceId::FileSharingPttPauseActive,
    Themes::PixmapResourceId::FileSharingPttPauseHover);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::PttPlayGreen_(
    Themes::PixmapResourceId::FileSharingPttPlay,
    Themes::PixmapResourceId::FileSharingPttPlayActive,
    Themes::PixmapResourceId::FileSharingPttPlayHover,
    Themes::PixmapResourceId::FileSharingPttPause,
    Themes::PixmapResourceId::FileSharingPttPauseActive,
    Themes::PixmapResourceId::FileSharingPttPauseHover);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::PttTextGray_(
    Themes::PixmapResourceId::FileSharingPttTextDone,
    Themes::PixmapResourceId::FileSharingPttTextDoneActive,
    Themes::PixmapResourceId::FileSharingPttTextDoneHover,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24);

const ActionButtonWidget::ResourceSet ActionButtonWidget::ResourceSet::PttTextGreen_(
    Themes::PixmapResourceId::FileSharingPttText,
    Themes::PixmapResourceId::FileSharingPttTextActive,
    Themes::PixmapResourceId::FileSharingPttTextHover,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24,
    Themes::PixmapResourceId::FileSharingBlankButtonIcon24);

QSize ActionButtonWidget::getMaxIconSize()
{
    const QSize maxSize(
        getMaxIconDimension(),
        getMaxIconDimension());

    return Utils::scale_value(maxSize);
}

ActionButtonWidget::ActionButtonWidget(const ResourceSet &resourceIds, QWidget *parent)
    : QWidget(parent)
    , Progress_(0)
    , ResourcesInitialized_(false)
    , ProgressBarAngle_(0)
    , ProgressBaseAngleAnimation_(nullptr)
    , IsHovered_(false)
    , IsPressed_(false)
    , ResourceIds_(resourceIds)
    , IsAnimating_(false)
    , Layout_(nullptr)
    , AnimationStartTimer_(nullptr)
{
    initResources();

    ProgressTextFont_ = MessageStyle::getRotatingProgressBarTextFont();

    ProgressPen_ = MessageStyle::getRotatingProgressBarPen();

    selectCurrentIcon();

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    Layout_ = new ActionButtonWidgetLayout(this);
    setLayout(Layout_);

    Layout_->setGeometry(QRect());

    setCursor(Qt::PointingHandCursor);
}

ActionButtonWidget::~ActionButtonWidget()
{

}

void ActionButtonWidget::createAnimationStartTimer()
{
    if (AnimationStartTimer_)
    {
        return;
    }

    AnimationStartTimer_ = new QTimer(this);
    AnimationStartTimer_->setSingleShot(true);

    QObject::connect(
        AnimationStartTimer_,
        &QTimer::timeout,
        this,
        &ActionButtonWidget::onAnimationStartTimeout);
}

void ActionButtonWidget::drawIcon(QPainter &p)
{
    assert(Layout_);
    assert(CurrentIcon_);

    CurrentIcon_->Draw(p, Layout_->getIconRect());
}

void ActionButtonWidget::drawProgress(QPainter &p)
{
    assert(IsAnimating_);

    const auto QT_ANGLE_MULT = 16;

    const auto baseAngle = (getProgressBarBaseAngle() * QT_ANGLE_MULT);

    const auto progress = std::max(Progress_, PROGRESS_BAR_MIN_PERCENTAGE);

    const auto progressAngle = (int)std::ceil(progress * PROGRESS_BAR_ANGLE_MAX * QT_ANGLE_MULT);

    p.save();

    p.setPen(ProgressPen_);

    p.drawArc(Layout_->getProgressRect(), -baseAngle, -progressAngle);

    p.restore();
}

void ActionButtonWidget::drawProgressText(QPainter &p)
{
    assert(!ProgressText_.isEmpty());

    const auto &textRect = Layout_->getProgressTextRect();

    p.save();

    p.setFont(ProgressTextFont_);
    p.setPen(MessageStyle::getRotatingProgressBarTextPen());
    p.drawText(textRect, Qt::AlignHCenter, ProgressText_);

    p.restore();
}

int ActionButtonWidget::getProgressBarBaseAngle() const
{
    assert(ProgressBarAngle_ >= 0);
    assert(ProgressBarAngle_ <= PROGRESS_BAR_ANGLE_MAX);

    return ProgressBarAngle_;
}

void ActionButtonWidget::setProgressBarBaseAngle(const int32_t _val)
{
    assert(_val >= 0);
    assert(_val <= PROGRESS_BAR_ANGLE_MAX);

    const auto isAngleChanged = (ProgressBarAngle_ != _val);

    ProgressBarAngle_ = _val;

    if (isAngleChanged)
    {
        update();
    }
}

QPoint ActionButtonWidget::getCenterBias() const
{
    assert(Layout_);

    const QRect geometry(QPoint(), sizeHint());
    const auto geometryCenter = geometry.center();

    const auto iconCenter = Layout_->getIconRect().center();

    const auto direction = (iconCenter - geometryCenter);
    return direction;
}

QSize ActionButtonWidget::getIconSize() const
{
    assert(CurrentIcon_);
    return CurrentIcon_->GetSize();
}

QPoint ActionButtonWidget::getLogicalCenter() const
{
    assert(Layout_);

    return Layout_->getIconRect().center();
}

const QString& ActionButtonWidget::getProgressText() const
{
    return ProgressText_;
}

const QFont& ActionButtonWidget::getProgressTextFont() const
{
    return ProgressTextFont_;
}

const ActionButtonWidget::ResourceSet& ActionButtonWidget::getResourceSet() const
{
    return ResourceIds_;
}

void ActionButtonWidget::setProgress(const double progress)
{
    assert(progress >= 0);
    assert(progress <= 1.01);

    const auto isProgressChanged = (std::abs(Progress_ - progress) >= 0.01);
    if (!isProgressChanged)
    {
        return;
    }

    Progress_ = progress;

    if (IsAnimating_)
    {
        update();
    }
}

void ActionButtonWidget::setProgressPen(const QColor color, const double a, const double width)
{
    assert(a >= 0);
    assert(a <= 1);
    assert(width > 0);
    assert(width < 5);

    QColor progressColor(color);
    progressColor.setAlphaF(a);
    QBrush progressBrush(progressColor);

    ProgressPen_ = QPen(progressBrush, width);
}

void ActionButtonWidget::setProgressText(const QString &progressText)
{
    assert(!progressText.isEmpty());

    if (ProgressText_ == progressText)
    {
        return;
    }

    ProgressText_ = progressText;

    Layout_->setGeometry(QRect());

    updateGeometry();

    update();
}

void ActionButtonWidget::setResourceSet(const ResourceSet &resourceIds)
{
    ResourceIds_ = resourceIds;

    ResourcesInitialized_ = false;

    initResources();

    selectCurrentIcon();

    resetLayoutGeometry();
}

QSize ActionButtonWidget::sizeHint() const
{
    assert(Layout_);

    return Layout_->sizeHint();
}

void ActionButtonWidget::startAnimation(const int32_t _delay)
{
    assert(_delay >= 0);
    
    if (IsAnimating_)
    {
        return;
    }

    createAnimationStartTimer();

    AnimationStartTimer_->stop();
    AnimationStartTimer_->setInterval(_delay);
    AnimationStartTimer_->start();
}

void ActionButtonWidget::stopAnimation()
{
    if (AnimationStartTimer_)
    {
        AnimationStartTimer_->stop();
    }

    if (!IsAnimating_)
    {
        return;
    }

    stopProgressBarAnimation();

    IsAnimating_ = false;

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::enterEvent(QEvent *)
{
    IsHovered_ = true;

    if (selectCurrentIcon())
    {
        resetLayoutGeometry();
    }
}

void ActionButtonWidget::hideEvent(QHideEvent*)
{
    IsHovered_ = false;
    IsPressed_ = false;

    if (selectCurrentIcon())
    {
        resetLayoutGeometry();
    }
}

void ActionButtonWidget::leaveEvent(QEvent *)
{
    IsHovered_ = false;
    IsPressed_ = false;

    if (selectCurrentIcon())
    {
        resetLayoutGeometry();
    }
}

#ifdef __APPLE__
/*
https://jira.mail.ru/browse/IMDESKTOP-3109
normally clicking on a widget causes events like: press -> release. but on sierra (b5-b6) we got: press -> move -> release.
i hope it's a temporary case because before b5 it was ok.
*/
namespace { static auto mousePressStartPoint = QPoint(); }
#endif

void ActionButtonWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (IsPressed_)
    {
#ifdef __APPLE__
        if (!(QSysInfo().macVersion() > QSysInfo().MV_10_11 && (QCursor::pos() - mousePressStartPoint).manhattanLength() <= 2))
#endif
        {
            emit dragSignal();
            IsPressed_ = false;
        }
    }
}

void ActionButtonWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore();

    IsPressed_ = false;

    if (event->button() == Qt::LeftButton)
    {
#ifdef __APPLE__
        mousePressStartPoint = QCursor::pos();
#endif
        event->accept();
        IsPressed_ = true;
    }

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();

    if ((event->button() == Qt::LeftButton) && IsPressed_)
    {
        event->accept();

        if (IsAnimating_)
        {
            emit stopClickedSignal(event->globalPos());
        }
        else
        {
            emit startClickedSignal(event->globalPos());
        }
    }

    IsPressed_ = false;

    selectCurrentIcon();

    resetLayoutGeometry();
}

void ActionButtonWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawIcon(p);

    if (!IsAnimating_)
    {
        return;
    }

    drawProgress(p);

    if (ProgressText_.isEmpty())
    {
        return;
    }

    drawProgressText(p);
}

void ActionButtonWidget::initResources()
{
    if (ResourcesInitialized_)
    {
        return;
    }

    ResourcesInitialized_ = true;

    using namespace Themes;

    StartButtonImage_ = GetPixmap(ResourceIds_.StartButtonImage_);
    StartButtonImageHover_ = GetPixmap(ResourceIds_.StartButtonImageHover_);
    StartButtonImageActive_ = GetPixmap(ResourceIds_.StartButtonImageActive_);

    StopButtonImage_ = GetPixmap(ResourceIds_.StopButtonImage_);
    StopButtonImageHover_ = GetPixmap(ResourceIds_.StopButtonImageHover_);
    StopButtonImageActive_ = GetPixmap(ResourceIds_.StopButtonImageActive_);
}

void ActionButtonWidget::resetLayoutGeometry()
{
    Layout_->setGeometry(geometry());
    resize(sizeHint());
    updateGeometry();

    update();
}

bool ActionButtonWidget::selectCurrentIcon()
{
    auto icon = (IsAnimating_ ? StopButtonImage_ : StartButtonImage_);

    if (IsAnimating_)
    {
        if (IsHovered_)
        {
            icon = StopButtonImageHover_;
        }

        if (IsPressed_)
        {
            icon = StopButtonImageActive_;
        }
    }
    else
    {
        if (IsHovered_)
        {
            icon = StartButtonImageHover_;
        }

        if (IsPressed_)
        {
            icon = StartButtonImageActive_;
        }
    }

    const auto iconChanged = (icon != CurrentIcon_);

    CurrentIcon_ = icon;
    assert(CurrentIcon_);
    assert(CurrentIcon_->GetWidth() <= getMaxIconDimension());
    assert(CurrentIcon_->GetHeight() <= getMaxIconDimension());

    return iconChanged;
}

void ActionButtonWidget::startProgressBarAnimation()
{
    stopProgressBarAnimation();

    if (!ProgressBaseAngleAnimation_)
    {
        ProgressBaseAngleAnimation_ = new QPropertyAnimation(this, "ProgressBarBaseAngle");
        ProgressBaseAngleAnimation_->setDuration(700);
        ProgressBaseAngleAnimation_->setLoopCount(-1);
        ProgressBaseAngleAnimation_->setStartValue(0);
        ProgressBaseAngleAnimation_->setEndValue(PROGRESS_BAR_ANGLE_MAX);
    }

    ProgressBaseAngleAnimation_->start();
}

void ActionButtonWidget::stopProgressBarAnimation()
{
    if (!ProgressBaseAngleAnimation_)
    {
        return;
    }

    ProgressBaseAngleAnimation_->stop();
}

void ActionButtonWidget::onAnimationStartTimeout()
{
    if (IsAnimating_)
    {
        return;
    }

    IsAnimating_ = true;

    startProgressBarAnimation();

    selectCurrentIcon();

    resetLayoutGeometry();
}

namespace
{
    int32_t getMaxIconDimension()
    {
        return Utils::scale_value(48);
    }
}

UI_NS_END