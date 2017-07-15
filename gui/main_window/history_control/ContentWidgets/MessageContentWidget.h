#pragma once

#include "../QuoteColorAnimation.h"

namespace HistoryControl
{

	class MessageContentWidget : public QWidget
	{
		Q_OBJECT

	Q_SIGNALS:
        void stateChanged();
		void removeMe();

        void forcedLayoutUpdatedSignal() const;

    public:
        virtual bool isBlockElement() const = 0;

        virtual bool canUnload() const = 0;

        virtual void render(QPainter &p, const QColor& quote_color) = 0;

        virtual QString toLogString() const = 0;

        virtual QString toString() const = 0;

        virtual void copyFile() = 0;

        virtual void saveAs() = 0;

        virtual bool hasContextMenu(QPoint) const = 0;

        virtual QString toLink() const = 0;

        virtual bool hasOpenInBrowserMenu() { return false; }

		void startQuoteAnimation();

	public:
		MessageContentWidget(QWidget *parent, const bool isOutgoing, QString _aimId);

		virtual ~MessageContentWidget();

        virtual bool canReplace() const;

        virtual bool hasTextBubble() const;

        bool isSelected() const;

        virtual void select(const bool value);

        virtual bool selectByPos(const QPoint &pos);

        virtual void clearSelection();

        virtual void onVisibilityChanged(const bool isVisible);

        virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

        virtual QString selectedText() const;

        virtual QString toRecentsString() const;

        virtual int maxWidgetWidth() const;

        const QString& getContact() { return aimId_; }
        void setContact(QString _aimId) { aimId_ = _aimId; }

	protected:
		bool isOutgoing() const;

        int64_t getCurrentProcessId() const;

        bool isCurrentProcessId(const int64_t id) const;

        void resetCurrentProcessId();

        void setCurrentProcessId(const int64_t id);

		void setFixedSize(const QSize &size);

		void setFixedSize(const int32_t w, const int32_t h);

        virtual bool drag() { return false; }

        virtual void mouseMoveEvent(QMouseEvent *) override;

        QString aimId_;

    protected:
        bool Selected_;

        virtual void initialize() = 0;

	private:
        int64_t CurrentProcessId_;

        bool Initialized_;

		const bool IsOutgoing_;

        QPoint mousePos_;

        virtual void paintEvent(QPaintEvent*) override final;

		QuoteColorAnimation QuoteAnimation_;
	};
}