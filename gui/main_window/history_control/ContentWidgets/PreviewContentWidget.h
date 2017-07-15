#pragma once

#include "MessageContentWidget.h"
#include "../../mplayer/VideoPlayer.h"

namespace Ui
{
    class TextEditEx;
}

namespace HistoryControl
{

    class PreviewContentWidget : public MessageContentWidget
    {
        Q_OBJECT

    public:
        PreviewContentWidget(QWidget *parent, const bool isOutgoing, const QString &text, const bool previewsEnabled, QString _aimId);

        const QRect& getLastPreviewGeometry() const;

        virtual bool hasTextBubble() const override;

        virtual void render(QPainter &p, const QColor& quate_color) override final;

        virtual void select(const bool value) override;

        virtual bool selectByPos(const QPoint &pos) override;

        virtual QSize sizeHint() const override;

        virtual bool hasContextMenu(QPoint) const override;

        virtual void onActivityChanged(const bool isActive) {}

        virtual void onVisibilityChanged(const bool isVisible) {}

        virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) {}

        QSharedPointer<Ui::DialogPlayer> videoPlayer_;

        QPixmap Preview_;

    protected:
        const bool PreviewsEnabled_;

        QRect getPreviewScaledRect() const;

        const QString& getText() const;

        virtual bool isPlaceholderVisible() const;

        virtual bool isPreloaderVisible() const = 0;

        virtual void renderPreview(QPainter &p, const bool isAnimating, QPainterPath& _path, const QColor& qoute_color);

        void invalidateSizes();

        void renderTextBubble(QPainter &p);

        QPainterPath evaluateClippingPath() const;

        QPainterPath evaluateRelativeClippingPath() const;

        virtual void resizeEvent(QResizeEvent *event) override;

        void setPreview(const QPixmap &preview);

        QPixmap getPreview() const;

        void setPreviewGenuineSize(const QSize &size);

        void setTextVisible(const bool isVisible);

        QRect updateWidgetSize();

    private Q_SLOTS:
        void onPreviewSizeLimited(QPixmap preview);

    private:

        QSizeF PreviewGenuineSize_;


        QPainterPath ClippingPath_;

        QPainterPath RelativePreviewClippingPath_;

        const QString Text_;

        // you should not access the variable directly,
        // use getTextSize() instead
        mutable QSize TextSize_;

        bool IsTextVisible_;

        Ui::TextEditEx *TextControl_;

        mutable QPainterPath TextBubble_;

        QSize LastSize_;

        QRect LastPreviewGeometry_;

        void applyClippingPath(QPainter &p);

        void createTextControl();

        QSizeF evaluatePreviewScaledSize(const int boundWidth) const;

        QSize evaluateWidgetSize() const;

        QSizeF getPreviewScaledSizeF() const;

        const QPainterPath& getTextBubble() const;

        QSize getTextSize() const;

        QSize getTextBubbleSize() const;

        void limitPreviewSize();

        void prepareTextGeometry();

        void renderNoPreview(QPainter &p, const QColor& quote_color);

        void renderPreloader(QPainter &p, const QColor& quote_color);

        void renderPreloaderBubble(QPainter &p, const QColor& quote_color);


    };

}
