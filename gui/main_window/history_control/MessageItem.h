#pragma once

#include "../../types/message.h"

#include "MessageItemBase.h"

namespace HistoryControl
{
	class MessageContentWidget;
}

namespace Themes
{
    enum class PixmapResourceId;
}

namespace Ui
{
	class TextEditEx;
	class TextEmojiWidget;
	class PictureWidget;
	class ContextMenu;
    class MessageTimeWidget;

	class MessageData
	{
	public:
		MessageData();

        bool isOutgoing() const;

		bool AvatarVisible_;
		bool SenderVisible_;
		bool IndentBefore_;
		QDate Date_;

		bool deliveredToServer_;

		bool Chat_;
		qint64 Id_;
		QStringList NotificationsKeys_;
		QString AimId_;
		QString Sender_;
		QString Text_;
		QString StickerText_;
        QString SenderFriendly_;
		int AvatarSize_;
		qint32 Time_;

        struct IsOutgoingField
        {
            bool Outgoing_;
            bool Set_;
        } Outgoing_;
	};

	enum class SelectDirection
	{
		NONE = 0,
		DOWN,
		UP,
	};

    class MessageItemLayout;

    class MessageItemsAvatars
    {
    private:
        struct Info
        {
            QString aimId_;
            QString friendlyName_;
            int size_;
            QPixmap avatar_;
            std::function<void()> callback_;
        };
        std::map<QString, MessageItemsAvatars::Info> data_;

    private:
        MessageItemsAvatars();
        static MessageItemsAvatars &instance();

    public:
        ~MessageItemsAvatars();

        static QPixmap &get(const QString& _aimId, const QString& _friendlyName, int _size, const std::function<void()> & _callback);
        static void reset(const QString& _aimId);
    };

	class MessageItem : public MessageItemBase
	{
        friend class MessageItemLayout;

		Q_OBJECT

	Q_SIGNALS:
		void copy(QString);
        void quote(QList<Data::Quote>);
        void forward(QList<Data::Quote>);
        void adminMenuRequest(QString);
        void selectionChanged();

	public:
        MessageItem();
		MessageItem(QWidget* _parent);
		~MessageItem();

		virtual QString formatRecentsText() const override;

        virtual void setContact(const QString& _aimId) override;
        virtual void setSender(const QString& _sender) override;
        virtual void setHasAvatar(const bool _value) override;

		void selectByPos(const QPoint& _pos, bool _doNotSelectImage = false);
		QString selection(bool textonly = false) const;

        bool isSelected() const override;
        bool isTextSelected() const;

		void setId(qint64 id, const QString& _aimId);
        qint64 getId() const override;

		void waitForAvatar(bool _wait);
		void setAvatarVisible(const bool);
		void setMessage(const QString& _message);
        void setMchatSenderAimId(const QString& _senderAimId);
        inline const QString& getMchatSenderAimId() const { return MessageSenderAimId_; };
		void setMchatSender(const QString& _sender);

		void setOutgoing(
            const bool _isOutgoing,
            const bool _isDeliveredToServer,
            const bool _isMChat,
            const bool _isInit = false);

        virtual bool setLastRead(const bool _isLastRead) override;

        virtual void setDeliveredToServer(const bool _delivered) override;

        virtual bool isOutgoing() const override;

		void setTime(const int32_t _time);
		void setContentWidget(::HistoryControl::MessageContentWidget* _widget);
		void setStickerText(const QString& _text);
		virtual void setTopMargin(const bool _value) override;
        virtual themes::themePtr theme() const override;
		void setDate(const QDate& _date);
		bool selected();
		QDate date() const;
		bool isRemovable() const;
        bool isUpdateable() const;
		QString toLogString() const;
        virtual void select() override;
        virtual void clearSelection() override;
        void manualUpdateGeometry(const int32_t _widgetWidth);
        void updateMenus();
        QString contentClass() const;

		void updateWith(MessageItem& _messageItem);
		std::shared_ptr<MessageData> getData();

        void loadAvatar(const int size);

        virtual QSize sizeHint() const override;

        virtual void onVisibilityChanged(const bool _isVisible) override;
        virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;
        
        Data::Quote getQuote(bool force = false) const;
        
        void forwardRoutine();

        void setNotAuth(const bool _isNotAuth);
        bool isNotAuth();

		virtual void setQuoteSelection() override;

    public Q_SLOTS:
        bool updateData();

	private Q_SLOTS:
        void avatarClicked();
		void menu(QAction*);
        void onRecreateAvatarRect();

	protected:
        virtual void leaveEvent(QEvent*) override;

        virtual void mouseMoveEvent(QMouseEvent*) override;

        virtual void mousePressEvent(QMouseEvent*) override;

        virtual void mouseReleaseEvent(QMouseEvent*) override;

        virtual void paintEvent(QPaintEvent*) override;

        virtual void resizeEvent(QResizeEvent*) override;

	private:

        void createMessageBody();

        void updateMessageBodyColor();

        void updateSenderControlColor();

        void createSenderControl();

        void drawAvatar(QPainter& _p);

        void drawMessageBubble(QPainter& _p);

        QRect evaluateAvatarRect() const;

        QRect evaluateBubbleGeometry(const QRect &_contentGeometry) const;

        QRect evaluateContentHorGeometry(const int32_t _contentWidth) const;

        QRect evaluateContentVertGeometry() const;

        int32_t evaluateContentWidth(const int32_t _widgetWidth) const;

        int32_t evaluateDesiredContentHeight() const;

        int32_t evaluateLeftContentMargin() const;

        int32_t evaluateRightContentMargin() const;

        int32_t evaluateTopContentMargin() const;

        const QRect& getAvatarRect() const;

        bool isAvatarVisible() const;

        bool isBlockItem() const;

        bool isFileSharing() const;

        bool isOverAvatar(const QPoint& _p) const;

        void updateBubbleGeometry(const QRect& _bubbleGeometry);

        void updateContentWidgetHorGeometry(const QRect& _bubbleHorGeometry);

        void updateMessageBodyHorGeometry(const QRect& _bubbleHorGeometry);

        void updateMessageBodyFullGeometry(const QRect& _bubbleRect);

        void updateSenderGeometry();

        void updateTimeGeometry(const QRect& _contentGeometry);

		bool isMessageBubbleVisible() const;

        void trackContentMenu(const QPoint& _pos);
        void trackMenu(const QPoint& _pos);

		TextEditEx *MessageBody_;
		TextEmojiWidget *Sender_;
        QString MessageSenderAimId_;
		::HistoryControl::MessageContentWidget *ContentWidget_;

        bool LastRead_;

		SelectDirection Direction_;
		ContextMenu* Menu_;
        ContextMenu* ContentMenu_;

		std::shared_ptr<MessageData> Data_;

        QPainterPath Bubble_;

        mutable QRect AvatarRect_;

        bool ClickedOnAvatar_;

        MessageTimeWidget *TimeWidget_;

        MessageItemLayout *Layout_;
        int startSelectY_;
        bool isSelection_;
        bool isNotAuth_;
	};
}
