#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEditEx.h"
#include "../../../core_dispatcher.h"
#include "../../../fonts.h"
#include "../../../gui_settings.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/utils.h"
#include "../../contact_list/ContactListModel.h"

#include "../../sounds/SoundsManager.h"

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"
#include "../MessagesModel.h"

#include "ComplexMessageItem.h"
#include "PttBlockLayout.h"
#include "Selection.h"
#include "Style.h"

#include "PttBlock.h"
#include "QuoteBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    QString formatDuration(const int32_t _seconds);
}

enum class PttBlock::PlaybackState
{
    Min,

    Stopped,
    Playing,
    Paused,

    Max
};

PttBlock::PttBlock(
    ComplexMessageItem *_parent,
    const QString &_link,
    const int32_t _durationSec,
    int64_t _id,
    int64_t _prevId)
    : FileSharingBlockBase(_parent, _link, core::file_sharing_content_type::ptt)
    , ctrlButton_(nullptr)
    , pttLayout_(nullptr)
    , textButton_(nullptr)
    , durationSec_(_durationSec)
    , textRequestId_(-1)
    , decodedTextCtrl_(nullptr)
    , isDecodedTextCollapsed_(true)
    , isPlayed_(false)
    , isPlaybackScheduled_(false)
    , playbackState_(PlaybackState::Stopped)
    , playingId_(-1)
    , playbackProgressMsec_(0)
    , playbackProgressAnimation_(nullptr)
    , id_(_id)
    , prevId_(_prevId)
{
    assert(durationSec_ >= 0);

    ctrlButton_ = new ActionButtonWidget(ActionButtonWidget::ResourceSet::PttPlayGreen_, this);

    textButton_ = new ActionButtonWidget(ActionButtonWidget::ResourceSet::PttTextGreen_, this);
    textButton_->setProgressPen(Style::Ptt::getPttProgressColor(), 0.6, Style::Ptt::getPttProgressWidth());

    durationText_ = formatDuration(durationSec_);

    pttLayout_ = new PttBlockLayout();
    setBlockLayout(pttLayout_);
    setLayout(pttLayout_);

    connect(Logic::GetMessagesModel(), SIGNAL(pttPlayed(qint64)), this, SLOT(pttPlayed(qint64)), Qt::QueuedConnection);
}

PttBlock::~PttBlock()
{

}

void PttBlock::clearSelection()
{
    FileSharingBlockBase::clearSelection();

    if (decodedTextCtrl_)
    {
        decodedTextCtrl_->clearSelection();
    }
}

QSize PttBlock::getCtrlButtonSize() const
{
    assert(ctrlButton_);
    if (!ctrlButton_)
    {
        return QSize(0, 0);
    }

    return ctrlButton_->sizeHint();
}

QString PttBlock::getSelectedText(bool isFullSelect) const
{
    QString result;
    result.reserve(512);

    if (isSelected())
    {
        result += getLink();
        result += QChar::LineFeed;
    }

    if (decodedTextCtrl_)
    {
        result += decodedTextCtrl_->selection();
    }

    return result;
}

QSize PttBlock::getTextButtonSize() const
{
    assert(textButton_);
    if (!textButton_)
    {
        return QSize(0, 0);
    }

    return textButton_->sizeHint();
}

bool PttBlock::hasDecodedText() const
{
    return !decodedText_.isEmpty();
}

bool PttBlock::isDecodedTextCollapsed() const
{
    return isDecodedTextCollapsed_;
}

void PttBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    assert(to.y() >= from.y());

    if (!isDecodedTextVisible())
    {
        FileSharingBlockBase::selectByPos(from, to, selection);

        if (decodedTextCtrl_)
        {
            decodedTextCtrl_->clearSelection();
        }

        return;
    }

    if (selection == BlockSelectionType::Full)
    {
        setSelected(true);

        assert(decodedTextCtrl_);
        decodedTextCtrl_->selectWholeText();

        return;
    }

    const auto localFrom = mapFromGlobal(from);
    const auto localTo = mapFromGlobal(to);

    if (selection == BlockSelectionType::FromBeginning)
    {
        const auto isHeaderSelected = (localTo.y() > pttLayout_->getDecodedTextSeparatorY());
        setSelected(isHeaderSelected);

        assert(decodedTextCtrl_);
        decodedTextCtrl_->selectFromBeginning(to);

        return;
    }

    if (selection == BlockSelectionType::TillEnd)
    {
        const auto isHeaderSelected = (localFrom.y() < pttLayout_->getDecodedTextSeparatorY());
        setSelected(isHeaderSelected);

        assert(decodedTextCtrl_);
        decodedTextCtrl_->selectTillEnd(from);

        return;
    }

    assert(selection == BlockSelectionType::PartialInternal);

    const auto isHeaderSelected = (localFrom.y() < pttLayout_->getDecodedTextSeparatorY());
    setSelected(isHeaderSelected);

    decodedTextCtrl_->selectByPos(from, to);

}

void PttBlock::setCtrlButtonGeometry(const QRect &_rect)
{
    assert(!_rect.isEmpty());

    if (!ctrlButton_)
    {
        return;
    }

    ctrlButton_->setGeometry(_rect);
    ctrlButton_->setVisible(true);
}

void PttBlock::setDecodedTextGeometry(const QRect &_rect)
{
    assert(!_rect.isEmpty());

    assert(decodedTextCtrl_);
    if (!decodedTextCtrl_)
    {
        return;
    }

    decodedTextCtrl_->setGeometry(_rect);
    decodedTextCtrl_->setVisible(true);
}

int32_t PttBlock::setDecodedTextWidth(const int32_t _width)
{
    assert(_width > 0);
    assert(decodedTextCtrl_);

    decodedTextCtrl_->setFixedWidth(_width);
    decodedTextCtrl_->document()->setTextWidth(_width);

    return decodedTextCtrl_->getTextHeight();
}

void PttBlock::setTextButtonGeometry(const QRect &_rect)
{
    assert(!_rect.isEmpty());

    if (!textButton_)
    {
        return;
    }

    textButton_->setGeometry(_rect);
    textButton_->setVisible(true);
}

void PttBlock::drawBlock(QPainter &_p, const QRect& _rect, const QColor& quote_color)
{
    const auto &bubbleRect = pttLayout_->getContentRect();

    renderClipPaths(bubbleRect);

    drawBubble(_p, bubbleRect);

    const auto ctrlButtonRect = pttLayout_->getCtrlButtonRect();

    drawDuration(_p, ctrlButtonRect);

    if (hasDecodedText() && !isDecodedTextCollapsed())
    {
        drawDecodedTextSeparator(_p, bubbleRect);
    }

    if (isPlaying() || isPaused())
    {
        const auto durationMsec = (durationSec_ * 1000);
        assert(durationMsec > 0);

        drawPlaybackProgress(_p, playbackProgressMsec_, playbackProgressAnimation_->endValue().toInt(), bubbleRect);
    }

    if (quote_color.isValid())
    {
        _p.fillRect(_rect, QBrush(quote_color));
    }
}

void PttBlock::initializeFileSharingBlock()
{
    connectSignals();

    requestMetainfo(false);
}

void PttBlock::onDownloadingStarted()
{
    assert(ctrlButton_);

    const auto animationTimeout = 300;
    ctrlButton_->startAnimation(animationTimeout);
}

void PttBlock::onDownloadingStopped()
{

}

void PttBlock::onDownloaded()
{
    assert(ctrlButton_);

    ctrlButton_->stopAnimation();

    if (isPlaybackScheduled_)
    {
        isPlaybackScheduled_ = false;

        startPlayback();
    }
}

void PttBlock::onDownloadedAction()
{
    if (isPlaybackScheduled_)
    {
        isPlaybackScheduled_ = false;

        startPlayback();
    }
}


void PttBlock::onDownloading(const int64_t _bytesTransferred, const int64_t _bytesTotal)
{
    assert(ctrlButton_);

    const auto progress = ((double)_bytesTransferred / (double)_bytesTotal);
    assert(progress >= 0);

    ctrlButton_->setProgress(progress);

    notifyBlockContentsChanged();
}

void PttBlock::onDownloadingFailed(const int64_t _seq)
{
    assert(textRequestId_ >= -1);
    assert(_seq >= -1);

    if (textRequestId_ != _seq)
    {
        return;
    }

    getParentComplexMessage()->replaceBlockWithSourceText(this);
}

void PttBlock::onLocalCopyInfoReady(const bool isCopyExists)
{
    const auto isPlayed = (isPlayed_ || isCopyExists);

    const auto isPlayedStatusChanged = (isPlayed != isPlayed_);
    if (!isPlayedStatusChanged)
    {
        return;
    }

    isPlayed_ = isPlayed;

    updateButtonsResourceSets();
}

void PttBlock::onMetainfoDownloaded()
{

}

void PttBlock::onPreviewMetainfoDownloaded(const QString &_miniPreviewUri, const QString &_fullPreviewUri)
{
    Q_UNUSED(_miniPreviewUri);
    Q_UNUSED(_fullPreviewUri);

    assert(!"you're not expected to be here");
}

void PttBlock::connectSignals()
{
    QMetaObject::Connection connection;

    connection = QObject::connect(
        textButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        [this]
        {
            if (hasDecodedText())
            {
                isDecodedTextCollapsed_ = !isDecodedTextCollapsed_;

                if (decodedTextCtrl_)
                {
                    decodedTextCtrl_->setVisible(!isDecodedTextCollapsed_);
                }

                notifyBlockContentsChanged();
                return;
            }

            requestText();
        });
    assert(connection);

    connection = QObject::connect(
        ctrlButton_,
        &ActionButtonWidget::startClickedSignal,
        this,
        &PttBlock::onCtrlButtonClicked);
    assert(connection);

    connection = QObject::connect(
        GetDispatcher(),
        &core_dispatcher::speechToText,
        this,
        &PttBlock::onPttText);
    assert(connection);

    connection = QObject::connect(
        GetSoundsManager(),
        &SoundsManager::pttPaused,
        this,
        &PttBlock::onPttPaused);
    assert(connection);

    connection = QObject::connect(
        GetSoundsManager(),
        &SoundsManager::pttFinished,
        this,
        &PttBlock::onPttFinished);
    assert(connection);
}

void PttBlock::drawBubble(QPainter &_p, const QRect &_bubbleRect)
{
    assert(!_bubbleRect.isEmpty());

    // overall bubble

    _p.save();

    const auto isBodySelected = (isSelected() && !isDecodedTextVisible());

    if (isStandalone())
    {
        assert(!bubbleClipPath_.isEmpty());
        _p.setClipPath(bubbleClipPath_);

        const auto &bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), isBodySelected, getThemeId());
        _p.setBrush(bodyBrush);

        _p.drawRect(_bubbleRect);
    }
    else
    {
        _p.setPen(Style::Files::getFileSharingFramePen());

        if (isBodySelected)
        {
            const auto &bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), isBodySelected, getThemeId());
            _p.setBrush(bodyBrush);
        }

        QRect newRect = _bubbleRect;
        newRect.setRect(
            newRect.x() + Style::Files::getFileSharingFramePen().width(),
            newRect.y() + Style::Files::getFileSharingFramePen().width(),
            newRect.width() - Style::Files::getFileSharingFramePen().width() * 2,
            newRect.height() - Style::Files::getFileSharingFramePen().width() * 2);

        _p.drawRoundedRect(newRect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
    }

    _p.restore();

    if (!isSelected() || !isDecodedTextVisible())
    {
        return;
    }

    // header selection

    _p.save();

    const auto &headerBrush = MessageStyle::getBodyBrush(isOutgoing(), true, getThemeId());
    _p.setBrush(headerBrush);

    assert(!headerClipPath_.isEmpty());
    _p.setClipPath(headerClipPath_);

    _p.drawRect(_bubbleRect);

    _p.restore();
}

void PttBlock::drawDecodedTextSeparator(QPainter &_p, const QRect &_bubbleRect)
{
    assert(!_bubbleRect.isEmpty());

    const auto separatorY = pttLayout_->getDecodedTextSeparatorY();

    const QPoint p0(_bubbleRect.left(), separatorY);
    const QPoint p1(_bubbleRect.right(), separatorY);

    _p.save();

    _p.setPen(Style::Ptt::getDecodedTextSeparatorPen());

    _p.drawLine(p0, p1);

    _p.restore();
}

void PttBlock::drawDuration(QPainter &_p, const QRect &_ctrlButtonRect)
{
    assert(!_ctrlButtonRect.isEmpty());
    assert(!durationText_.isEmpty());

    const auto durationLeftMargin = Utils::scale_value(12);
    const auto durationBaseline = Utils::scale_value(34);

    auto textX = (_ctrlButtonRect.right() + 1);
    textX += durationLeftMargin;

    const auto textY = durationBaseline;

    const auto font = Style::Ptt::getDurationTextFont();
    const auto pen = Style::Ptt::getDurationTextPen();

    _p.save();

    _p.setFont(font);
    _p.setPen(pen);

    const auto playbackProgressSec = (playbackProgressMsec_ / 1000);
    const auto secondsLeft = (durationSec_ - playbackProgressSec);
    const auto &text = ((isPlaying() || isPaused()) ? formatDuration(secondsLeft) : durationText_);

    assert(!text.isEmpty());
    _p.drawText(textX, textY, text);

    _p.restore();
}

void PttBlock::drawPlaybackProgress(QPainter &_p, const int32_t _progressMsec, const int32_t _durationMsec, const QRect &_bubbleRect)
{
    assert(_progressMsec >= 0);
    assert(_durationMsec > 0);
    assert(!_bubbleRect.isEmpty());

    const auto progressWidth = (_durationMsec ? ((_progressMsec * _bubbleRect.width()) / _durationMsec) : 0);

    const QSize progressSize(progressWidth, Style::Ptt::getPttBubbleHeight());

    const QRect progressRect(_bubbleRect.topLeft(), progressSize);

    _p.save();

    if (isStandalone() || !isDecodedTextVisible())
    {
        assert(!bubbleClipPath_.isEmpty());
        _p.setClipPath(bubbleClipPath_);
    }
    else
    {
        assert(!headerClipPath_.isEmpty());
        _p.setClipPath(headerClipPath_);
    }

    _p.setBrush(Style::Ptt::getPlaybackProgressBrush());
    _p.drawRect(progressRect);

    _p.restore();
}

int32_t PttBlock::getPlaybackProgress() const
{
    assert(playbackProgressMsec_ >= 0);
    assert(playbackProgressMsec_ < 300000);

    return playbackProgressMsec_;
}

void PttBlock::setPlaybackProgress(const int32_t _value)
{
    assert(_value >= 0);
    assert(_value < 300000);

    playbackProgressMsec_ = _value;

    update();
}

void PttBlock::initializeDecodedTextCtrl()
{
    assert(!decodedText_.isEmpty());
    assert(!decodedTextCtrl_);

    decodedTextCtrl_ = new TextEditEx(
        this,
        MessageStyle::getTextFont(),
        MessageStyle::getTextColor(),
        false,
        false);

    decodedTextCtrl_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    decodedTextCtrl_->setStyle(QApplication::style());
    decodedTextCtrl_->setFrameStyle(QFrame::NoFrame);
    decodedTextCtrl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    decodedTextCtrl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    decodedTextCtrl_->setOpenLinks(false);
    decodedTextCtrl_->setOpenExternalLinks(false);
    decodedTextCtrl_->setWordWrapMode(QTextOption::WordWrap);
    decodedTextCtrl_->setStyleSheet("background: transparent");
    decodedTextCtrl_->setContextMenuPolicy(Qt::NoContextMenu);
    decodedTextCtrl_->setReadOnly(true);
    decodedTextCtrl_->setUndoRedoEnabled(false);

    const auto selectionStyleSheet = QString("background: transparent; selection-background-color: %1;").arg(Utils::getSelectionColor().name(QColor::HexArgb));
    decodedTextCtrl_->setStyleSheet(selectionStyleSheet);

    auto textColor = MessageStyle::getTextColor();
    QPalette palette;
    palette.setColor(QPalette::Text, textColor);
    decodedTextCtrl_->setPalette(palette);

    Logic::Text4Edit(decodedText_, *decodedTextCtrl_, Logic::Text2DocHtmlMode::Escape, false, true);
}

bool PttBlock::isDecodedTextVisible() const
{
    return (hasDecodedText() && !isDecodedTextCollapsed());
}

bool PttBlock::isPaused() const
{
    return (playbackState_ == PlaybackState::Paused);
}

bool PttBlock::isPlaying() const
{
    return (playbackState_ == PlaybackState::Playing);
}

bool PttBlock::isStopped() const
{
    return (playbackState_ == PlaybackState::Stopped);
}

bool PttBlock::isTextRequested() const
{
    return (textRequestId_ != -1);
}

void PttBlock::renderClipPaths(const QRect &_bubbleRect)
{
    assert(!_bubbleRect.isEmpty());
    if (_bubbleRect.isEmpty())
    {
        return;
    }

    const auto isBubbleRectChanged = (_bubbleRect != bubbleClipRect_);
    if (!isBubbleRectChanged)
    {
        return;
    }

    bubbleClipPath_ = Utils::renderMessageBubble(_bubbleRect, MessageStyle::getBorderRadius(), isOutgoing());

    const QRect headerRect(_bubbleRect.left(), _bubbleRect.top(), _bubbleRect.width(), Style::Ptt::getPttBubbleHeight());
    headerClipPath_ = Utils::renderMessageBubble(headerRect, MessageStyle::getBorderRadius(), isOutgoing(), Utils::RenderBubble_TopRounded);

    bubbleClipRect_ = _bubbleRect;
}

void PttBlock::requestText()
{
    assert(textRequestId_ == -1);

    textRequestId_ = GetDispatcher()->pttToText(
        getLink(),
        Utils::GetTranslator()->getCurrentLang());

    isPlayed_ = true;

    updateButtonsResourceSets();

    startTextRequestProgressAnimation();
}

void PttBlock::startTextRequestProgressAnimation()
{
    textButton_->startAnimation();
    textButton_->setProgress(0.6);
}

void PttBlock::stopTextRequestProgressAnimation()
{
    textButton_->stopAnimation();
}

void PttBlock::startPlayback()
{
    isPlayed_ = true;

    assert(!isPlaying());
    if (isPlaying())
    {
        return;
    }

    playbackState_ = PlaybackState::Playing;

    int duration = 0;
    playingId_ = GetSoundsManager()->playPtt(getFileLocalPath(), playingId_, duration);

    if (!playbackProgressAnimation_)
    {
        assert(duration > 0);

        playbackProgressAnimation_ = new QPropertyAnimation(this, "PlaybackProgress");
        playbackProgressAnimation_->setDuration(duration);
        playbackProgressAnimation_->setLoopCount(1);
        playbackProgressAnimation_->setStartValue(0);
        playbackProgressAnimation_->setEndValue(duration);
    }

    playbackProgressAnimation_->start();

    updateButtonsResourceSets();
}

void PttBlock::pausePlayback()
{
    assert(isPlaying());
    if (!isPlaying())
    {
        return;
    }

    playbackState_ = PlaybackState::Paused;

    assert(playingId_ > 0);
    GetSoundsManager()->pausePtt(playingId_);

    playbackProgressAnimation_->pause();

    updateButtonsResourceSets();
}

void PttBlock::updateButtonsResourceSets()
{
    updateCtrlButtonResourceSets();

    updateTextButtonResourceSets();
}

void PttBlock::updateCtrlButtonResourceSets()
{
    if (!ctrlButton_)
    {
        return;
    }

    if (isPlaying())
    {
        ctrlButton_->setResourceSet(ActionButtonWidget::ResourceSet::PttPause_);
        return;
    }

    if (isPlayed_)
    {
        ctrlButton_->setResourceSet(ActionButtonWidget::ResourceSet::PttPlayGray_);
    }
    else
    {
        ctrlButton_->setResourceSet(ActionButtonWidget::ResourceSet::PttPlayGreen_);
    }
}

void PttBlock::updateTextButtonResourceSets()
{
    if (!textButton_)
    {
        return;
    }

    if (isPlayed_)
    {
        textButton_->setResourceSet(ActionButtonWidget::ResourceSet::PttTextGray_);
    }
    else
    {
        textButton_->setResourceSet(ActionButtonWidget::ResourceSet::PttTextGreen_);
    }
}

void PttBlock::onCtrlButtonClicked()
{
    if (isPlaying())
    {
        pausePlayback();
        return;
    }

    isPlaybackScheduled_ = true;

    if (isFileDownloading())
    {
        return;
    }

    startDownloading(true);
}

void PttBlock::onPttFinished(int _id, bool _byPlay)
{
    assert(_id > 0);

    if (_id != playingId_)
    {
        return;
    }

    assert(playbackProgressAnimation_);
    assert(ctrlButton_);

    playingId_ = -1;

    if (playbackProgressAnimation_)
    {
        playbackProgressAnimation_->stop();
    }

    playbackState_ = PlaybackState::Stopped;

    updateButtonsResourceSets();

    if (_byPlay)
        emit Logic::GetMessagesModel()->pttPlayed(id_);

    update();
}

void PttBlock::onPttPaused(int _id)
{
    assert(_id > 0);

    if (_id != playingId_)
    {
        return;
    }

    if (isPlaying())
    {
        pausePlayback();
    }
}

void PttBlock::onPttText(qint64 _seq, int _error, QString _text, int _comeback)
{
    assert(_seq > 0);

    if (textRequestId_ != _seq)
    {
        return;
    }

    textRequestId_  = -1;

    const auto isError = (_error != 0);
    const auto isComeback = (_comeback > 0);

    if (isComeback)
    {
        const auto retryTimeoutMsec = ((_comeback + 1) * 1000);

        QTimer::singleShot(
            retryTimeoutMsec,
            Qt::VeryCoarseTimer,
            this,
            &PttBlock::requestText);

        return;
    }

    if (isError)
    {
        decodedText_ = QT_TRANSLATE_NOOP("ptt_widget", "unclear message");
    }
    else
    {
        decodedText_ = _text;
    }

    isDecodedTextCollapsed_ = false;

    stopTextRequestProgressAnimation();

    initializeDecodedTextCtrl();

    GetDispatcher()->setUrlPlayed(getLink(), true);

    notifyBlockContentsChanged();
}

void PttBlock::pttPlayed(qint64 id)
{
    if (id != prevId_ || getChatAimid() != Logic::getContactListModel()->selectedContact())
        return;

    if (!isPlayed_ && !isPlaying())
    {
        isPlaybackScheduled_ = true;

        if (isFileDownloading())
        {
            return;
        }

        startDownloading(true);
    }
}

void PttBlock::connectToHover(Ui::ComplexMessage::QuoteBlockHover* hover)
{
    connectButtonToHover(ctrlButton_, hover);
    connectButtonToHover(textButton_, hover);
    GenericBlock::connectToHover(hover);
}

namespace
{
    QString formatDuration(const int32_t _seconds)
    {
        assert(_seconds >= 0);

        const auto minutes = (_seconds / 60);

        const auto seconds = (_seconds % 60);

        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

UI_COMPLEX_MESSAGE_NS_END