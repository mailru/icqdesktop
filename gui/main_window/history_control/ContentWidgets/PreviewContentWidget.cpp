#include "stdafx.h"

#include "../../../controls/TextEditEx.h"
#include "../../../themes/ThemePixmap.h"
#include "../../../themes/ResourceIds.h"
#include "../../../theme_settings.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/Text.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../complex_message/Style.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"
#include "../ResizePixmapTask.h"

#include "PreviewContentWidget.h"

using namespace Ui::ComplexMessage;

namespace HistoryControl
{
    namespace
    {

        int32_t getBubbleVertPadding();

        const QSizeF& getMaxPreviewSize();

        int32_t getTextBottomMargin();

        bool isLayoutGridEnabled();

    }

    PreviewContentWidget::PreviewContentWidget(QWidget *parent, const bool isOutgoing, const QString &text, const bool previewsEnabled, QString _aimId)
        : MessageContentWidget(parent, isOutgoing, _aimId)
        , Text_(text)
        , TextControl_(nullptr)
        , IsTextVisible_(false)
        , PreviewsEnabled_(previewsEnabled)
        , videoPlayer_(nullptr)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        setMinimumSize(Style::Preview::getMinPreviewSize());

        updateGeometry();
    }

    const QRect& PreviewContentWidget::getLastPreviewGeometry() const
    {
        return LastPreviewGeometry_;
    }

    bool PreviewContentWidget::hasTextBubble() const
    {
        return TextControl_;
    }

    void PreviewContentWidget::select(const bool value)
    {
        MessageContentWidget::select(value);

        if (TextControl_)
        {
            TextControl_->clearSelection();
        }
    }

    bool PreviewContentWidget::selectByPos(const QPoint &pos)
    {
        assert(TextControl_);

        TextControl_->selectByPos(pos);

        return true;
    }

    QSize PreviewContentWidget::sizeHint() const
    {
        return evaluateWidgetSize();
    }

    bool PreviewContentWidget::hasContextMenu(QPoint p) const
    {
        if (!TextControl_)
        {
            return true;
        }

        const auto local = mapFromGlobal(p);
        if (local.y() < getTextBubbleSize().height())
        {
            return false;
        }

        return true;
    }

    const QString& PreviewContentWidget::getText() const
    {
        return Text_;
    }

    bool PreviewContentWidget::isPlaceholderVisible() const
    {
        return true;
    }

    void PreviewContentWidget::renderPreview(QPainter &p, const bool isAnimating, QPainterPath& _path, const QColor& quote_color)
    {
        if (Preview_.isNull())
        {
            renderNoPreview(p, quote_color);
            return;
        }

        const auto imageRect = updateWidgetSize();

        LastPreviewGeometry_ = imageRect;

        p.save();

        if (isLayoutGridEnabled())
        {
            p.setPen(Qt::green);
            p.drawRect(imageRect.adjusted(1, 1, -1, -1));

            p.restore();

            return;
        }

        if (!videoPlayer_)
        {
            p.drawPixmap(imageRect, Preview_);

			if (quote_color.isValid())
			{
				p.setBrush(QBrush(quote_color));
				p.drawRoundedRect(imageRect, Ui::MessageStyle::getBorderRadius(), Ui::MessageStyle::getBorderRadius());
			}

        }
        else
        {
            if (!videoPlayer_->isFullScreen())
            {
                videoPlayer_->setClippingPath(_path);

                if (videoPlayer_->state() == QMovie::MovieState::Paused && videoPlayer_->isGif())
                {
                    p.drawPixmap(imageRect, videoPlayer_->getActiveImage());
                }
            }
        }

        if (isAnimating)
        {
            p.fillRect(imageRect, Style::Preview::getImageShadeBrush());
        }

        if (isSelected())
        {
            const QBrush brush(Utils::getSelectionColor());
            p.fillRect(imageRect, brush);
        }

        p.restore();
    }

    void PreviewContentWidget::renderTextBubble(QPainter &p)
    {
        const auto &bubblePath = getTextBubble();
        if (bubblePath.isEmpty())
        {
            return;
        }

        p.save();

        int theme_id = Ui::get_qt_theme_settings()->themeIdForContact(aimId_);

        QBrush b;
        p.fillPath(
            bubblePath,
            Ui::MessageStyle::getBodyBrush(isOutgoing(), isSelected(), theme_id)
        );

        p.restore();
    }

    QPainterPath PreviewContentWidget::evaluateClippingPath() const
    {
        const auto pathRect = getPreviewScaledRect();
        assert(!pathRect.isEmpty());

        return Utils::renderMessageBubble(
            pathRect,
            Ui::MessageStyle::getBorderRadius(),
            isOutgoing());
    }

    QPainterPath PreviewContentWidget::evaluateRelativeClippingPath() const
    {
        const auto pathRect = getPreviewScaledRect();
        assert(!pathRect.isEmpty());

         auto relativePreviewRect = QRect(0, 0, pathRect.width(), pathRect.height());

        return Utils::renderMessageBubble(
            relativePreviewRect,
            Ui::MessageStyle::getBorderRadius(),
            isOutgoing());
    }

    void PreviewContentWidget::render(QPainter &p, const QColor& qoute_color)
    {
        p.save();

        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);

        applyClippingPath(p);

        renderPreview(p, false, RelativePreviewClippingPath_, qoute_color);

        p.restore();

        renderTextBubble(p);

        if (isLayoutGridEnabled())
        {
            p.save();

            p.setPen(Qt::blue);
            p.drawRect(rect());

            p.restore();
        }
    }

    void PreviewContentWidget::resizeEvent(QResizeEvent *e)
    {
        MessageContentWidget::resizeEvent(e);

        invalidateSizes();
    }

    void PreviewContentWidget::setPreview(const QPixmap &preview)
    {
        assert(!preview.isNull());

        __TRACE(
            "preview",
            "setting preview\n" <<
            toLogString() << "\n"
            "----------------------------\n"
            "    preview_size=<" << preview.size() << ">"
        );

        if (PreviewGenuineSize_.isEmpty())
        {
            setPreviewGenuineSize(preview.size());
        }

        Preview_ = preview;

        emit forcedLayoutUpdatedSignal();

        limitPreviewSize();

        invalidateSizes();

        update();
    }

    QPixmap PreviewContentWidget::getPreview() const
    {
        return Preview_;
    }

    void PreviewContentWidget::setPreviewGenuineSize(const QSize &size)
    {
        assert(!size.isEmpty());
        assert(PreviewGenuineSize_.isEmpty());
        assert(Preview_.isNull());

        PreviewGenuineSize_ = size;

        updateGeometry();
    }

    void PreviewContentWidget::setTextVisible(const bool isVisible)
    {
        IsTextVisible_ = isVisible;

        TextSize_ = QSize();

        if (IsTextVisible_)
        {
            if (TextControl_)
            {
                return;
            }

            createTextControl();
        }
        else
        {
            if(!TextControl_)
            {
                return;
            }

            TextControl_->hide();
            delete TextControl_;
            TextControl_ = nullptr;
        }

        emit forcedLayoutUpdatedSignal();

        updateGeometry();

        update();
    }

    void PreviewContentWidget::onPreviewSizeLimited(QPixmap preview)
    {
        assert(preview);

        Preview_ = preview;
    }

    void PreviewContentWidget::applyClippingPath(QPainter &p)
    {
        if (!PreviewsEnabled_)
        {
            return;
        }

        const auto minExtent = (Ui::MessageStyle::getBorderRadius() * 2);

        const auto isPreviewTooSmall = (
            (PreviewGenuineSize_.width() <= minExtent) ||
            (PreviewGenuineSize_.height() <= minExtent));

        if (isPreviewTooSmall)
        {
            return;
        }

        if (ClippingPath_.isEmpty())
        {
            ClippingPath_ = evaluateClippingPath();
            RelativePreviewClippingPath_ = evaluateRelativeClippingPath();

            assert(!ClippingPath_.isEmpty());
        }

        p.setClipPath(ClippingPath_);
    }

    void PreviewContentWidget::createTextControl()
    {
        assert(IsTextVisible_);
        assert(!Text_.isEmpty());
        assert(!TextControl_);

        blockSignals(true);
        setUpdatesEnabled(false);

        TextControl_ = new Ui::TextEditEx(
            this,
            Ui::MessageStyle::getTextFont(),
            Ui::MessageStyle::getTextColor(),
            false,
            false
        );

        TextControl_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        TextControl_->setStyle(QApplication::style());
        TextControl_->setFrameStyle(QFrame::NoFrame);
        TextControl_->setDocument(TextControl_->document());
        TextControl_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextControl_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextControl_->setOpenLinks(true);
        TextControl_->setOpenExternalLinks(true);
        TextControl_->setWordWrapMode(QTextOption::WordWrap);
        TextControl_->setStyleSheet("background: transparent");
        TextControl_->document()->setDocumentMargin(0);
        TextControl_->setContextMenuPolicy(Qt::NoContextMenu);
        TextControl_->setReadOnly(true);
        TextControl_->setUndoRedoEnabled(false);

        TextControl_->verticalScrollBar()->blockSignals(true);

        Logic::Text4Edit(Text_, *TextControl_, Logic::Text2DocHtmlMode::Escape, true, true);

        TextControl_->verticalScrollBar()->blockSignals(false);

        TextControl_->show();

        setUpdatesEnabled(true);
        blockSignals(false);
    }

    QSizeF PreviewContentWidget::evaluatePreviewScaledSize(const int boundWidth) const
    {
        assert(!PreviewGenuineSize_.isEmpty() || Preview_.isNull());

        const auto &previewSize = (
            PreviewGenuineSize_.isEmpty() ?
                Style::Preview::getImagePlaceholderSize() :
                Utils::scale_value(PreviewGenuineSize_)
        );

        QSizeF fixedSize(boundWidth, 0);

        // prevent small images from being stretched up
        fixedSize.setWidth(
            std::min(fixedSize.width(), previewSize.width())
        );

        const auto verticalAspectRatio = ((double)previewSize.height() / (double)previewSize.width());
        fixedSize.setHeight(fixedSize.width() * verticalAspectRatio);

        const auto shouldScaleDown =
            (fixedSize.width() > getMaxPreviewSize().width()) ||
            (fixedSize.height() > getMaxPreviewSize().height());
        if (shouldScaleDown)
        {
            fixedSize = fixedSize.scaled(getMaxPreviewSize(), Qt::KeepAspectRatio);
        }

        const auto minPreviewSize = Style::Preview::getMinPreviewSize();

        const auto shouldScaleUp =
            (fixedSize.width() < minPreviewSize.width()) &&
            (fixedSize.height() < minPreviewSize.height());
        if (shouldScaleUp)
        {
            fixedSize = fixedSize.scaled(minPreviewSize, Qt::KeepAspectRatio);
        }

        return fixedSize;
    }

    QRect PreviewContentWidget::getPreviewScaledRect() const
    {
        QPoint topLeft(0, 0);

        if (TextControl_)
        {
            topLeft.ry() +=
                getTextBubbleSize().height() +
                getTextBottomMargin();
        }

        QRect result(
            QPoint(0, 0),
            getPreviewScaledSizeF().toSize()
        );

        if (isOutgoing())
        {
            const auto previewX = (
                width() -
                result.width());

            topLeft.rx() = previewX;
        }

        result.moveTopLeft(topLeft);

        return result;
    }

    QSize PreviewContentWidget::evaluateWidgetSize() const
    {
        if (PreviewGenuineSize_.isEmpty())
        {
            const auto minPreviewSize = Style::Preview::getMinPreviewSize();

            auto width = minPreviewSize.width();

            return QSize(width, minPreviewSize.height());
        }

        const auto &previewScaledSize = getPreviewScaledSizeF();

        const auto bubbleSize = getTextBubbleSize();

        auto width = previewScaledSize.width();

        const auto height = (
            bubbleSize.height() +
            getTextBottomMargin() +
            previewScaledSize.height());

        const auto minPreviewSize = Style::Preview::getMinPreviewSizeF();

        QSize widgetSize(
            std::max(width, minPreviewSize.width()),
            std::max(height, minPreviewSize.height()));

        return widgetSize;
    }

    QSizeF PreviewContentWidget::getPreviewScaledSizeF() const
    {
        const auto previewScaledSize = evaluatePreviewScaledSize(width());

        return previewScaledSize;
    }

    const QPainterPath& PreviewContentWidget::getTextBubble() const
    {
        const auto textBubbleEmpty = TextBubble_.isEmpty();
        if (!textBubbleEmpty)
        {
            return TextBubble_;
        }

        const auto &textSize = getTextSize();
        if (textSize.isEmpty())
        {
            return TextBubble_;
        }

        const auto textBubbleSize = getTextBubbleSize();

        TextBubble_ = Utils::renderMessageBubble(
            textBubbleSize,
            Ui::MessageStyle::getBorderRadius(),
            isOutgoing()
        );

        assert(!TextBubble_.isEmpty());

        return TextBubble_;
    }

    QSize PreviewContentWidget::getTextSize() const
    {
        if (!TextControl_)
        {
            TextSize_ = QSize(0, 0);
        }

        if (TextSize_.isValid())
        {
            return TextSize_;
        }

        return TextControl_->getTextSize();
    }

    QSize PreviewContentWidget::getTextBubbleSize() const
    {
        auto bubbleSize = getTextSize();

        if (bubbleSize.isEmpty())
        {
            return QSize(0, 0);
        }

        const auto textHeight = bubbleSize.height();

        bubbleSize.setHeight(
            std::max(
                Ui::MessageStyle::getMinBubbleHeight(),
                textHeight
            )
        );

        bubbleSize.setHeight(
            Utils::applyMultilineTextFix(textHeight, bubbleSize.height())
        );

        bubbleSize.rwidth() += Ui::MessageStyle::getBubbleHorPadding();
        bubbleSize.rwidth() += Ui::MessageStyle::getBubbleHorPadding();
        bubbleSize.rheight() += getBubbleVertPadding();

        return bubbleSize;
    }

    void PreviewContentWidget::limitPreviewSize()
    {
        assert(!Preview_.isNull());
        assert(!PreviewGenuineSize_.isEmpty());

        const auto previewSize = PreviewGenuineSize_;
        const auto shouldScalePreviewDown =
            (previewSize.width() > getMaxPreviewSize().width()) ||
            (previewSize.height() > getMaxPreviewSize().height());
        if (!shouldScalePreviewDown)
        {
            return;
        }

        Utils::check_pixel_ratio(Preview_);

        const auto scaledSize = Utils::scale_bitmap(getMaxPreviewSize().toSize());

        auto task = new ResizePixmapTask(Preview_, scaledSize);

        const auto succeed = QObject::connect(
            task, &ResizePixmapTask::resizedSignal,
            this, &PreviewContentWidget::onPreviewSizeLimited
        );
        assert(succeed);

        QThreadPool::globalInstance()->start(task);
    }

    void PreviewContentWidget::invalidateSizes()
    {
        // invalidate size-dependent children

        ClippingPath_ = QPainterPath();

        TextSize_ = QSize();
        assert(!TextSize_.isValid());

        TextBubble_ = QPainterPath();
        assert(TextBubble_.isEmpty());
    }

    void PreviewContentWidget::prepareTextGeometry()
    {
        if (!TextControl_)
        {
            return;
        }

        const auto widgetSize = evaluateWidgetSize();

        auto documentWidth = (
            widgetSize.width() -
            Ui::MessageStyle::getBubbleHorPadding() -
            Ui::MessageStyle::getBubbleHorPadding()
        );

        const auto textWidthChanged = (TextControl_->width() != documentWidth);
        if (textWidthChanged)
        {
            TextControl_->document()->setTextWidth(documentWidth);
            TextControl_->setFixedWidth(documentWidth);
        }

        TextControl_->move(
            Ui::MessageStyle::getBubbleHorPadding(),
            getBubbleVertPadding()
        );
    }

    void PreviewContentWidget::renderNoPreview(QPainter &p, const QColor& qoute_color)
    {
        const auto nothingToRender = (!isPlaceholderVisible() && !TextControl_);
        if (nothingToRender)
        {
            return;
        }

        if (!isPlaceholderVisible())
        {
            prepareTextGeometry();

            const auto widgetSize = evaluateWidgetSize();

            const QSize newSize(
                widgetSize.width(),
                getTextBubbleSize().height()
            );

            if (newSize.height() != LastSize_.height())
            {
                setFixedHeight(newSize.height());
            }

            LastSize_ = newSize;

            return;
        }

        LastPreviewGeometry_ = updateWidgetSize();

        renderPreloader(p, qoute_color);
    }

    void PreviewContentWidget::renderPreloader(QPainter &p, const QColor& quote_color)
    {
        renderPreloaderBubble(p, quote_color);

        if (!isPreloaderVisible())
        {
            return;
        }
    }

    void PreviewContentWidget::renderPreloaderBubble(QPainter &p, const QColor& quote_color)
    {
        p.save();

        auto bodyBrush = Style::Preview::getImagePlaceholderBrush();

        p.setBrush(bodyBrush);

        p.drawRoundedRect(getPreviewScaledRect(), Ui::MessageStyle::getBorderRadius(), Ui::MessageStyle::getBorderRadius());

		if (quote_color.isValid())
		{
			p.setBrush(QBrush(quote_color));
			p.drawRoundedRect(getPreviewScaledRect(), Ui::MessageStyle::getBorderRadius(), Ui::MessageStyle::getBorderRadius());
		}

        p.restore();
    }

    QRect PreviewContentWidget::updateWidgetSize()
    {
        auto imageRect = getPreviewScaledRect();

        imageRect = QRect(
            imageRect.left(),
            imageRect.top(),
            std::max(imageRect.width(), 2),
            std::max(imageRect.height(), 2)
        );

        auto widgetHeight = imageRect.height();

        if (TextControl_)
        {
            widgetHeight += getTextBubbleSize().height();
            widgetHeight += getTextBottomMargin();
        }

        const auto widgetWidth = evaluateWidgetSize().width();

        const QSize widgetSize(widgetWidth, widgetHeight);

        if (LastSize_ != widgetSize)
        {
            prepareTextGeometry();

            if (widgetSize.height() != LastSize_.height())
            {
                setFixedHeight(widgetSize.height());
            }
        }

        LastSize_ = widgetSize;

        return imageRect;
    }

    namespace
    {

        int32_t getBubbleVertPadding()
        {
            return Utils::scale_value(5);
        }

        const QSizeF& getMaxPreviewSize()
        {
            static const QSizeF size(
                Style::Preview::getImageWidthMax(),
                Style::Preview::getImageHeightMax()
            );

            return size;
        }

        int32_t getTextBottomMargin()
        {
            return Utils::scale_value(2);
        }

        bool isLayoutGridEnabled()
        {
            if (!build::is_debug())
            {
                return false;
            }

            return false;
        }

    }
}