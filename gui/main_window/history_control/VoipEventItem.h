#pragma once

#include "MessageItemBase.h"

namespace HistoryControl
{
    typedef std::shared_ptr<class VoipEventInfo> VoipEventInfoSptr;
}

namespace Themes
{
    typedef std::shared_ptr<class IThemePixmap> IThemePixmapSptr;
}

namespace Ui
{

    class MessageTimeWidget;

    class VoipEventItem : public MessageItemBase
    {
        Q_OBJECT

    public:
        VoipEventItem(const ::HistoryControl::VoipEventInfoSptr& eventInfo);

        VoipEventItem(QWidget *parent, const ::HistoryControl::VoipEventInfoSptr& eventInfo);

        virtual QString formatRecentsText() const override;

        virtual void setTopMargin(const bool value) override;

        virtual void setHasAvatar(const bool value) override;

        virtual bool setLastRead(const bool _isLastRead) override;

        virtual bool isOutgoing() const override;

        virtual qint64 getId() const override;

        void setId(const qint64 _id);

        void updateStyle();

		virtual void setQuoteSelection() override;

    protected:
        virtual void mouseMoveEvent(QMouseEvent *) override;

        virtual void mouseReleaseEvent(QMouseEvent *) override;

        virtual void leaveEvent(QEvent *) override;

        virtual void paintEvent(QPaintEvent *) override;

        virtual void resizeEvent(QResizeEvent *event) override;
    private:

        std::shared_ptr<const QPixmap> Avatar_;

        QPainterPath Bubble_;

        QRect BubbleRect_;

        ::HistoryControl::VoipEventInfoSptr EventInfo_;

        Themes::IThemePixmapSptr HoverIcon_;

        Themes::IThemePixmapSptr Icon_;

        bool IsAvatarHovered_;

        bool IsBubbleHovered_;

        QString Text_;

        MessageTimeWidget *TimeWidget_;

        QRect TimeWidgetGeometry_;

        bool lastRead_;

        qint64 id_;

        QRect getAvatarRect() const;

        bool isAvatarHovered(const QPoint &mousePos) const;

        bool isBubbleHovered(const QPoint &mousePos) const;

        void updateHeight();

        QColor getTextColor(const bool isHovered);

    };

}