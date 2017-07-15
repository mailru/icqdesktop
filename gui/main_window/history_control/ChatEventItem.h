#pragma once

#include "HistoryControlPageItem.h"

namespace HistoryControl
{
    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;
}

namespace Ui
{
    class DpiAwareImage;
    class TextEditEx;

    class ChatEventItem : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        ChatEventItem(const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id);

        ChatEventItem(QWidget* _parent, const HistoryControl::ChatEventInfoSptr& eventInfo, const qint64 _id);

        ~ChatEventItem();

        virtual void clearSelection() override;

        virtual QString formatRecentsText() const override;

        virtual QSize sizeHint() const override;

         virtual bool setLastRead(const bool _isLastRead) override;

         virtual qint64 getId() const override;

		 virtual void setQuoteSelection() override;

    private:

        QRect BubbleRect_;

        std::unique_ptr<DpiAwareImage> Icon_;

        TextEditEx *TextWidget_;

        const ::HistoryControl::ChatEventInfoSptr EventInfo_;

        bool isLastRead_;

        int height_;

        qint64 id_;

        int32_t evaluateFullIconWidth();

        int32_t evaluateTextHeight(const int32_t _textWidth);

        int32_t evaluateTextWidth(const int32_t _widgetWidth);

        void updateTheme();

        virtual void paintEvent(QPaintEvent* _event) override;

        virtual void resizeEvent(QResizeEvent* _event) override;
    };

}