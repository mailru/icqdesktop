#include "stdafx.h"
#include "MessageItem.h"

#include "MessageItemLayout.h"
#include "MessagesModel.h"
#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "ContentWidgets/FileSharingWidget.h"
#include "../contact_list/ContactListModel.h"

#include "../../app_config.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../cache/themes/themes.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/ContextMenu.h"
#include "../../my_info.h"
#include "../../theme_settings.h"
#include "../../utils/InterConnector.h"
#include "../../utils/PainterPath.h"
#include "../../utils/Text.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"


namespace Ui
{

    namespace
    {
        int32_t getMessageTopPadding();
        int32_t getMessageBottomPadding();

        QMap<QString, QVariant> makeData(const QString& command);
    }

    MessageData::MessageData()
        : AvatarVisible_(false)
        , SenderVisible_(false)
        , IndentBefore_(false)
        , deliveredToServer_(false)
        , Id_(-1)
        , AvatarSize_(-1)
        , Time_(0)
        , Chat_(false)
    {
        Outgoing_.Outgoing_ = false;
        Outgoing_.Set_ = false;
    }

    bool MessageData::isOutgoing() const
    {
        assert(Outgoing_.Set_);

        return Outgoing_.Outgoing_;
    }

    MessageItemsAvatars::MessageItemsAvatars()
    {
        QObject::connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, [this](QString _aimId)
        {
            if (MessageItemsAvatars::instance().data_.find(_aimId) != MessageItemsAvatars::instance().data_.end())
            {
                bool isDefault = false;
                auto &info = MessageItemsAvatars::instance().data_[_aimId];
                info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(_aimId, info.friendlyName_, info.size_, QString(), true, isDefault, false, false).get();
                if (info.callback_)
                {
                    info.callback_();
                }
            }
        });
    }

    MessageItemsAvatars &MessageItemsAvatars::instance()
    {
        static MessageItemsAvatars instance_;
        return instance_;
    }

    MessageItemsAvatars::~MessageItemsAvatars()
    {
    }

    QPixmap &MessageItemsAvatars::get(const QString& _aimId, const QString& _friendlyName, int _size, const std::function<void()>& _callback)
    {
        if (MessageItemsAvatars::instance().data_.find(_aimId) == MessageItemsAvatars::instance().data_.end())
        {
            bool isDefault = false;
            MessageItemsAvatars::Info info;
            info.avatar_ = *Logic::GetAvatarStorage()->GetRounded(_aimId, _friendlyName, _size, QString(), true, isDefault, false, false).get();
            info.aimId_ = _aimId;
            info.friendlyName_ = _friendlyName;
            info.size_ = _size;
            info.callback_ = _callback;
            MessageItemsAvatars::instance().data_[_aimId] = info;
        }
        return MessageItemsAvatars::instance().data_[_aimId].avatar_;
    }

    void MessageItemsAvatars::reset(const QString& _aimId)
    {
        if (MessageItemsAvatars::instance().data_.find(_aimId) != MessageItemsAvatars::instance().data_.end())
        {
            MessageItemsAvatars::instance().data_.erase(_aimId);
        }
    }

    MessageItem::MessageItem()
        : MessageItemBase(0)
        , Data_(new MessageData())
        , MessageBody_(nullptr)
        , Sender_(nullptr)
        , ContentWidget_(nullptr)
        , Direction_(SelectDirection::NONE)
        , Menu_(0)
        , ContentMenu_(0)
        , ClickedOnAvatar_(false)
        , Layout_(nullptr)
        , TimeWidget_(nullptr)
        , startSelectY_(0)
        , isSelection_(false)
        , isNotAuth_(false)
    {
    }

	MessageItem::MessageItem(QWidget* _parent)
		: MessageItemBase(_parent)
		, MessageBody_(nullptr)
		, Sender_(nullptr)
		, ContentWidget_(nullptr)
		, Data_(new MessageData())
        , Direction_(SelectDirection::NONE)
		, Menu_(0)
        , ContentMenu_(0)
        , ClickedOnAvatar_(false)
        , Layout_(new MessageItemLayout(this))
        , TimeWidget_(new MessageTimeWidget(this))
        , startSelectY_(0)
        , isSelection_(false)
        , LastRead_(false)
        , isNotAuth_(false)
    {
        QMetaObject::connectSlotsByName(this);

		setAttribute(Qt::WA_AcceptTouchEvents);

        setMouseTracking(true);

		Utils::grabTouchWidget(this);
		setFocusPolicy(Qt::NoFocus);

        assert(Layout_);
        setLayout(Layout_);

        connect(parent(), SIGNAL(recreateAvatarRect()), this, SLOT(onRecreateAvatarRect()));
	}

	MessageItem::~MessageItem()
    {
        if (Data_)
            MessageItemsAvatars::reset(Data_->Sender_);
	}

    void MessageItem::setContact(const QString& _aimId)
    {
        HistoryControlPageItem::setContact(_aimId);
        if (TimeWidget_)
        {
            TimeWidget_->setContact(_aimId);
        }
    }

    void MessageItem::setSender(const QString& _sender)
    {
        Data_->Sender_ = _sender;
    }

	QString MessageItem::formatRecentsText() const
	{
		if (ContentWidget_)
		{
			return ContentWidget_->toRecentsString();
		}

		return Data_->Text_;
	}

	void MessageItem::setId(qint64 _id, const QString& _aimId)
	{
		Data_->Id_ = _id;
		Data_->AimId_ = _aimId;
	}

    void MessageItem::loadAvatar(const int _size)
    {
        assert(_size > 0);

        if (Data_->AvatarSize_ != _size)
        {
            MessageItemsAvatars::reset(Data_->Sender_);
        }

        Data_->AvatarSize_ = _size;

        setAvatarVisible(true);

        update();
    }

    QSize MessageItem::sizeHint() const
    {
        auto height = evaluateTopContentMargin();

        height += evaluateDesiredContentHeight();

        if (LastRead_)
        {
            height += MessageStyle::getLastReadAvatarSize() + 2 * MessageStyle::getLastReadAvatarMargin();
        }

        const QSize result(
            0,
            height
        );

        return result;
    }


    void MessageItem::onVisibilityChanged(const bool _isVisible)
    {
        if (ContentWidget_)
        {
            ContentWidget_->onVisibilityChanged(_isVisible);
        }
    }

    void MessageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        if (ContentWidget_)
        {
            ContentWidget_->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
        }
    }

    Data::Quote MessageItem::getQuote(bool force) const
    {
        Data::Quote quote;
        if (selection(true).isEmpty() && !force)
            return quote;

        if (force)
        {
            if (MessageBody_)
            {
                quote.text_ = MessageBody_->getPlainText();
                quote.type_ = Data::Quote::Type::text;
            }
            else if (ContentWidget_)
            {
                quote.text_ = ContentWidget_->toString();
                quote.type_ = Data::Quote::Type::file_sharing;
            }
        }
        else
        {
            quote.text_ = selection(true);
        }

        if (Data_)
        {
            quote.senderId_ = Data_->isOutgoing() ? MyInfo()->aimId() : Data_->Sender_;
            quote.chatId_ = Data_->AimId_;
            quote.time_ = Data_->Time_;
            quote.msgId_ = Data_->Id_;
            QString senderFriendly = Data_->SenderFriendly_.isEmpty() ? Logic::getContactListModel()->getDisplayName(Data_->AimId_) : Data_->SenderFriendly_;
            if (Data_->isOutgoing())
                senderFriendly = MyInfo()->friendlyName();
            quote.senderFriendly_ = senderFriendly;
        }
        return quote;
    }

    void MessageItem::setHasAvatar(const bool _value)
    {
        setAvatarVisible(_value);
    }

    void MessageItem::setAvatarVisible(const bool _visible)
	{
        const auto visibilityChanged = (Data_->AvatarVisible_ != _visible);

        Data_->AvatarVisible_ = _visible;
        Data_->SenderVisible_ = (!Data_->Sender_.isEmpty() && Data_->AvatarVisible_);

        if (visibilityChanged)
        {
            if (Data_->AvatarVisible_ && Data_->AvatarSize_ <= 0)
            {
                loadAvatar(Utils::scale_bitmap(MessageStyle::getAvatarSize()));
            }

            update();
        }
	}

    void MessageItem::leaveEvent(QEvent* _e)
    {
        ClickedOnAvatar_ = false;

        QWidget::leaveEvent(_e);
    }

    void MessageItem::mouseMoveEvent(QMouseEvent* _e)
    {
        if (isOverAvatar(_e->pos()) && Data_->AvatarSize_ > 0)
        {
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }

        QWidget::mouseMoveEvent(_e);
    }

    void MessageItem::mousePressEvent(QMouseEvent* _e)
    {
        const auto isLeftButtonFlagSet = ((_e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (_e->button() == Qt::LeftButton)
        );

        if (isLeftButton && isOverAvatar(_e->pos()) && Data_->AvatarSize_ > 0)
        {
            ClickedOnAvatar_ = true;
        }

        QWidget::mousePressEvent(_e);
    }

    void MessageItem::trackContentMenu(const QPoint& _pos)
    {
        if (ContentMenu_)
        {
            delete ContentMenu_;
            ContentMenu_ = nullptr;
        }

        ContentMenu_ = new ContextMenu(this);

        if (ContentWidget_->hasOpenInBrowserMenu())
            ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/openbrowser_100.png")), QT_TRANSLATE_NOOP("context_menu", "Open in browser"), makeData("open_in_browser"));

        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/link_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData("copy_link"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/attach_100.png")), QT_TRANSLATE_NOOP("context_menu", "Copy file"), makeData("copy_file"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/download_100.png")), QT_TRANSLATE_NOOP("context_menu", "Save as..."), makeData("save_as"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/quote_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/forwardmsg_100.png")), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData("forward"));
        ContentMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/context_menu/closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData("delete"));

        connect(ContentMenu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        const auto isChatAdmin = Logic::getContactListModel()->isYouAdmin(Data_->AimId_);
        if (isOutgoing || isChatAdmin)
        {
            ContentMenu_->addActionWithIcon(QIcon(
                Utils::parse_image_name(":/resources/context_menu/closechat_all_100.png")),
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData("delete_all"));
        }

        ContentMenu_->popup(_pos);
    }

    void MessageItem::trackMenu(const QPoint& _pos)
    {
        if (Menu_)
            delete Menu_;

        Menu_ = new ContextMenu(this);
        Menu_->addActionWithIcon(":/resources/context_menu/copy_100.png", QT_TRANSLATE_NOOP("context_menu", "Copy"), makeData("copy"));
        Menu_->addActionWithIcon(":/resources/context_menu/quote_100.png", QT_TRANSLATE_NOOP("context_menu", "Quote"), makeData("quote"));
        Menu_->addActionWithIcon(":/resources/context_menu/forwardmsg_100.png", QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData("forward"));

        Menu_->addActionWithIcon(":/resources/context_menu/closechat_100.png", QT_TRANSLATE_NOOP("context_menu", "Delete for me"), makeData("delete"));

        connect(Menu_, &ContextMenu::triggered, this, &MessageItem::menu, Qt::QueuedConnection);

        const auto isOutgoing = (Data_->Outgoing_.Set_ && Data_->Outgoing_.Outgoing_);
        const auto isChatAdmin = Logic::getContactListModel()->isYouAdmin(Data_->AimId_);
        if (isOutgoing || isChatAdmin)
        {
            Menu_->addActionWithIcon(
                ":/resources/context_menu/closechat_all_100.png",
                QT_TRANSLATE_NOOP("context_menu", "Delete for all"),
                makeData("delete_all"));
        }

        if (GetAppConfig().IsContextMenuFeaturesUnlocked())
        {
            Menu_->addActionWithIcon(":/resources/copy_100.png", "Copy Message ID", makeData("dev:copy_message_id"));
        }

        Menu_->popup(_pos);
    }

    void MessageItem::mouseReleaseEvent(QMouseEvent* _e)
    {
        const auto isRightButtonFlagSet = ((_e->buttons() & Qt::RightButton) != 0);
        const auto isRightButton = (
            isRightButtonFlagSet ||
            (_e->button() == Qt::RightButton)
        );

        if (isRightButton)
        {
            if (isOverAvatar(_e->pos()))
            {
                emit adminMenuRequest(Data_->Sender_);
                return;
            }
            QPoint globalPos = mapToGlobal(_e->pos());
            if (MessageBody_)
            {
                trackMenu(globalPos);
            }
            else if (ContentWidget_)
            {
                if (ContentWidget_->hasContextMenu(globalPos))
                    trackContentMenu(globalPos);
                else
                    trackMenu(globalPos);
            }
        }

        const auto isLeftButtonFlagSet = ((_e->buttons() & Qt::LeftButton) != 0);
        const auto isLeftButton = (
            isLeftButtonFlagSet ||
            (_e->button() == Qt::LeftButton)
        );

        if (isLeftButton &&
            ClickedOnAvatar_ &&
            isOverAvatar(_e->pos()))
        {
            avatarClicked();
            _e->accept();
            return;
        }

        QWidget::mouseReleaseEvent(_e);
    }

    void MessageItem::paintEvent(QPaintEvent*)
    {
        assert(parent());

        QPainter p(this);

		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::TextAntialiasing);

        drawAvatar(p);

        drawMessageBubble(p);

        if (LastRead_)
        {
            drawLastReadAvatar(
                p,
                Data_->Sender_,
                Data_->SenderFriendly_,
                MessageStyle::getRightMargin(isOutgoing()),
                MessageStyle::getLastReadAvatarMargin());
        }
    }

    void MessageItem::updateMessageBodyColor()
    {
        if (!MessageBody_)
        {
            return;
        }

        auto textColor = MessageStyle::getTextColor();

        QPalette palette;
        palette.setColor(QPalette::Text, textColor);
        MessageBody_->setPalette(palette);
    }

    void MessageItem::resizeEvent(QResizeEvent* _e)
    {
        HistoryControlPageItem::resizeEvent(_e);
    }

    void MessageItem::createMessageBody()
    {
        if (MessageBody_)
        {
            return;
        }

        QPalette palette;
        MessageBody_ = new TextEditEx(
            this,
            MessageStyle::getTextFont(),
            palette,
            false,
            false
        );
        updateMessageBodyColor();

        QString styleSheet = QString("background: transparent; selection-background-color: %1;")
            .arg(Utils::rgbaStringFromColor(Utils::getSelectionColor()));

        MessageBody_->document()->setDefaultStyleSheet(MessageStyle::getMessageStyle());
        MessageBody_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        MessageBody_->setFrameStyle(QFrame::NoFrame);
        MessageBody_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->horizontalScrollBar()->setEnabled(false);
        MessageBody_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        MessageBody_->setOpenLinks(true);
        MessageBody_->setOpenExternalLinks(true);
        MessageBody_->setWordWrapMode(QTextOption::WordWrap);
        Utils::ApplyStyle(MessageBody_, styleSheet);
        MessageBody_->setFocusPolicy(Qt::NoFocus);
        MessageBody_->document()->setDocumentMargin(0);
        MessageBody_->setContextMenuPolicy(Qt::NoContextMenu);
        MessageBody_->setReadOnly(true);
        MessageBody_->setUndoRedoEnabled(false);

        connect(MessageBody_, &QTextBrowser::selectionChanged, [this]() { emit selectionChanged(); });
    }

    void MessageItem::updateSenderControlColor()
    {
        QColor color = theme() ? theme()->contact_name_.text_color_ : MessageStyle::getSenderColor();
        Sender_->setColor(color);
    }

    void MessageItem::createSenderControl()
    {
        if (Sender_)
        {
            return;
        }

        QColor color;
        Sender_ = new TextEmojiWidget(
            this,
            Fonts::appFont(Ui::MessageStyle::getSenderFont().pixelSize()),
            color
        );
        updateSenderControlColor();
    }

    void MessageItem::drawAvatar(QPainter& _p)
    {
        if (!Data_ || Data_->Sender_.isEmpty() || Data_->AvatarSize_ <= 0)
        {
            return;
        }
        auto avatar = MessageItemsAvatars::get(Data_->Sender_, Data_->SenderFriendly_, Data_->AvatarSize_, [this](){ parentWidget()->update(); });
        if (avatar.isNull() || !Data_->AvatarVisible_)
        {
            return;
        }

        const auto &rect = getAvatarRect();
        if (rect.isValid())
        {
            _p.drawPixmap(rect, avatar);
        }
    }

    void MessageItem::drawMessageBubble(QPainter& _p)
    {
        if (ContentWidget_ && !ContentWidget_->isBlockElement())
        {
            return;
        }

        if (Bubble_.isEmpty())
        {
            return;
        }

        assert(!Bubble_.isEmpty());

        _p.fillPath(Bubble_, MessageStyle::getBodyBrush(Data_->isOutgoing(), isSelected(), theme()->get_id()));

		QColor qoute_color = QuoteAnimation_.quoteColor();
		if (qoute_color.isValid())
		{
			_p.fillPath(Bubble_, (QBrush(qoute_color)));
		}
    }

    QRect MessageItem::evaluateAvatarRect() const
    {
        assert(!AvatarRect_.isValid());

        QRect result(
            MessageStyle::getLeftMargin(isOutgoing()),
            evaluateTopContentMargin(),
            MessageStyle::getAvatarSize(),
            MessageStyle::getAvatarSize()
        );

        return result;
    }

    QRect MessageItem::evaluateBubbleGeometry(const QRect &_contentGeometry) const
    {
        assert(!_contentGeometry.isEmpty());

        auto bubbleGeometry(_contentGeometry);

        bubbleGeometry.setWidth(
            bubbleGeometry.width());

        return bubbleGeometry;
    }

    QRect MessageItem::evaluateContentHorGeometry(const int32_t contentWidth) const
    {
        assert(contentWidth > 0);

        auto width = contentWidth;

        const QRect result(
            evaluateLeftContentMargin(),
            -1,
            width,
            0);

        return result;
    }

    QRect MessageItem::evaluateContentVertGeometry() const
    {
        const auto contentHeight = evaluateDesiredContentHeight();

        const QRect result(
            -1,
            evaluateTopContentMargin(),
            0,
            contentHeight
        );

        return result;
    }

    int32_t MessageItem::evaluateContentWidth(const int32_t _widgetWidth) const
    {
        assert(_widgetWidth > 0);

        auto contentWidth = _widgetWidth;

        contentWidth -= evaluateLeftContentMargin();
        contentWidth -= evaluateRightContentMargin();

        return contentWidth;
    }

    int32_t MessageItem::evaluateDesiredContentHeight() const
    {
        auto height = MessageStyle::getMinBubbleHeight();

        if (MessageBody_)
        {
            assert(!ContentWidget_);

            const auto textHeight = MessageBody_->getTextSize().height();

            if (textHeight > 0)
            {
                if (platform::is_apple())
                {
                    height = std::max(getMessageTopPadding() + textHeight + getMessageBottomPadding(), MessageStyle::getMinBubbleHeight());
                }
                else
                {
                    height = std::max<int32_t>(
                        height,
                        textHeight
                    );

                    height += getMessageTopPadding();

                    height = std::max<int32_t>(Utils::applyMultilineTextFix(textHeight, height), MessageStyle::getMinBubbleHeight());
                }
            }
        }

        if (ContentWidget_)
        {
            assert(!MessageBody_);

            height = std::max(height, ContentWidget_->height());
        }

        return height;
    }

    int32_t MessageItem::evaluateLeftContentMargin() const
    {
        auto leftContentMargin = MessageStyle::getLeftMargin(isOutgoing());

        if (isAvatarVisible())
        {
            leftContentMargin += MessageStyle::getAvatarSize();
            leftContentMargin += MessageStyle::getAvatarRightMargin();
        }

        return leftContentMargin;
    }

    int32_t MessageItem::evaluateRightContentMargin() const
    {
        auto rightContentMargin =
            MessageStyle::getRightMargin(isOutgoing()) +
            MessageStyle::getTimeMaxWidth();

        return rightContentMargin;
    }

    int32_t MessageItem::evaluateTopContentMargin() const
    {
        auto topContentMargin = MessageStyle::getTopMargin(hasTopMargin());

        if (Data_->SenderVisible_)
        {
            topContentMargin += MessageStyle::getSenderHeight();
            topContentMargin += MessageStyle::getSenderBottomMargin();
        }

        return topContentMargin;
    }

    const QRect& MessageItem::getAvatarRect() const
    {
        assert(isAvatarVisible());

        if (!AvatarRect_.isValid())
        {
            AvatarRect_ = evaluateAvatarRect();
            assert(AvatarRect_.isValid());
        }

        return AvatarRect_;
    }

    bool MessageItem::isAvatarVisible() const
    {
        return !isOutgoing();
    }

    bool MessageItem::isBlockItem() const
    {
        assert((bool)MessageBody_ ^ (bool)ContentWidget_);

        return (MessageBody_ || ContentWidget_->isBlockElement());
    }

    bool MessageItem::isOutgoing() const
    {
        if (!Data_)
            return false;

        return Data_->isOutgoing();
    }

    bool MessageItem::isFileSharing() const
    {
        if (!ContentWidget_)
        {
            return false;
        }

        return qobject_cast<HistoryControl::FileSharingWidget*>(ContentWidget_);
    }

    bool MessageItem::isOverAvatar(const QPoint& _p) const
    {
        return (
            isAvatarVisible() &&
            getAvatarRect().contains(_p)
        );
    }

    void MessageItem::manualUpdateGeometry(const int32_t _widgetWidth)
    {
        assert(_widgetWidth > 0);

        const auto contentWidth = evaluateContentWidth(_widgetWidth);

        const auto enoughSpace = (contentWidth > 0);
        if (!enoughSpace)
        {
            return;
        }

        setUpdatesEnabled(false);

        const auto contentHorGeometry = evaluateContentHorGeometry(contentWidth);

        updateMessageBodyHorGeometry(contentHorGeometry);

        updateContentWidgetHorGeometry(contentHorGeometry);

        const auto contentVertGeometry = evaluateContentVertGeometry();
        const QRect contentGeometry(
            contentHorGeometry.left(),
            contentVertGeometry.top(),
            contentHorGeometry.width(),
            contentVertGeometry.height());

        assert(!contentGeometry.isEmpty());

        updateMessageBodyFullGeometry(contentGeometry);

        const auto bubbleGeometry = evaluateBubbleGeometry(contentGeometry);

        updateBubbleGeometry(bubbleGeometry);

        updateSenderGeometry();

        updateTimeGeometry(contentGeometry);

        setUpdatesEnabled(true);
    }


    QString MessageItem::contentClass() const
    {
        if (MessageBody_)
        {
            static const auto N_OF_LEFTMOST_CHARS = 5;

            const auto &text = Data_->Text_;

            QString result;
            result.reserve(64);

            result.append("text(");
            result.append(text.left(N_OF_LEFTMOST_CHARS));

            if (text.length() > N_OF_LEFTMOST_CHARS)
            {
                result.append("...");
            }

            result.append(")");

            return result;
        }

        return ContentWidget_->metaObject()->className();
    }

    void MessageItem::updateBubbleGeometry(const QRect &_bubbleGeometry)
    {
        assert(!_bubbleGeometry.isEmpty());

        Bubble_ = Utils::renderMessageBubble(_bubbleGeometry, MessageStyle::getBorderRadius(), Data_->isOutgoing());
        assert(!Bubble_.isEmpty());
    }

    void MessageItem::updateContentWidgetHorGeometry(const QRect &_contenHorGeometry)
    {
        assert(_contenHorGeometry.width() > 0);

        if (!ContentWidget_)
        {
            return;
        }

        auto left = _contenHorGeometry.left();

        if (ContentWidget_->isBlockElement())
        {
            left += MessageStyle::getBubbleHorPadding();
        }

        ContentWidget_->move(
            left,
            evaluateTopContentMargin()
        );

        if (ContentWidget_->isBlockElement())
        {
            auto blockWidth = _contenHorGeometry.width();
            blockWidth -= (MessageStyle::getBubbleHorPadding() * 2);

            ContentWidget_->setFixedWidth(blockWidth);

            return;
        }

        auto width = _contenHorGeometry.width();

        const auto maxWidgetWidth = ContentWidget_->maxWidgetWidth();

        assert(width > 0);

        if (maxWidgetWidth == -1)
        {
            ContentWidget_->setFixedWidth(width);
            return;
        }

        if (width > maxWidgetWidth)
            width = maxWidgetWidth;

        ContentWidget_->setFixedWidth(width);
    }

    void MessageItem::updateMessageBodyHorGeometry(const QRect &_bubbleHorGeometry)
    {
        assert(_bubbleHorGeometry.width() > 0);

        if (!MessageBody_)
        {
            return;
        }

        auto messageBodyWidth = _bubbleHorGeometry.width();
        messageBodyWidth -= MessageStyle::getBubbleHorPadding();
        messageBodyWidth -= MessageStyle::getBubbleHorPadding();

        const auto widthChanged = (messageBodyWidth != MessageBody_->getTextSize().width());
        if (!widthChanged)
        {
            return;
        }

        MessageBody_->setFixedWidth(messageBodyWidth);
        MessageBody_->document()->setTextWidth(messageBodyWidth);
    }

    void MessageItem::updateMessageBodyFullGeometry(const QRect &bubbleRect)
    {
        if (!MessageBody_)
        {
            return;
        }

        const QRect messageBodyGeometry(
            bubbleRect.left() + MessageStyle::getBubbleHorPadding(),
            bubbleRect.top() + getMessageTopPadding(),
            MessageBody_->getTextSize().width(),
            MessageBody_->getTextSize().height()
        );
        assert(!messageBodyGeometry.isEmpty());

        MessageBody_->setGeometry(messageBodyGeometry);
    }

    void MessageItem::updateSenderGeometry()
    {
        if (!Sender_)
        {
            return;
        }

        if (!Data_->SenderVisible_)
        {
            Sender_->setVisible(false);
            return;
        }

        Sender_->move(
            MessageStyle::getLeftMargin(isOutgoing()),
            MessageStyle::getTopMargin(hasTopMargin())
        );

        Sender_->setVisible(true);
    }

    void MessageItem::updateTimeGeometry(const QRect &_contentGeometry)
    {
        const auto timeWidgetSize = TimeWidget_->sizeHint();

        QPoint posHint;

        assert(posHint.x() >= 0);
        assert(posHint.y() >= 0);

        auto timeX = 0;

        if (posHint.x() > 0)
        {
            timeX = posHint.x();
            timeX += evaluateLeftContentMargin();
            timeX += MessageStyle::getTimeMarginX();
        }
        else
        {
            timeX = _contentGeometry.right();
            timeX += MessageStyle::getTimeMarginX();
        }

        auto timeY = 0;

        if (posHint.y() > 0)
        {
            timeY = posHint.y();
            timeY += evaluateTopContentMargin();
        }
        else
        {
            timeY = _contentGeometry.bottom();
        }

        timeY -= MessageStyle::getTimeMarginY();
        timeY -= timeWidgetSize.height();

        const QRect timeGeometry(
            timeX,
            timeY,
            MessageStyle::getTimeMaxWidth(),
            timeWidgetSize.height()
        );

        TimeWidget_->setGeometry(timeGeometry);
    }

	void MessageItem::selectByPos(const QPoint& _pos, bool _doNotSelectImage)
	{
        if (!isSelection_)
        {
			isSelection_ = true;
			startSelectY_ = _pos.y();
        }

		if (MessageBody_)
		{
			MessageBody_->selectByPos(_pos);
            return;
		}

        assert(ContentWidget_);
        if (!ContentWidget_)
        {
            return;
        }

        QRect widgetRect;
        bool selected = false;
        if (ContentWidget_->hasTextBubble())
        {
            selected = ContentWidget_->selectByPos(_pos);
            widgetRect = QRect(mapToGlobal(ContentWidget_->rect().topLeft()), mapToGlobal(ContentWidget_->rect().bottomRight()));
        }
        else
        {
            widgetRect = QRect(mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
        }

        const auto isCursorOverWidget = (
            (widgetRect.top() <= _pos.y()) &&
            (widgetRect.bottom() >= _pos.y())
        );
		if (isCursorOverWidget && !selected && !_doNotSelectImage)
		{
			if (!isSelected())
			{
				select();
			}

            return;
		}

        const auto distanceToWidgetTop = std::abs(_pos.y() - widgetRect.top());
        const auto distanceToWidgetBottom = std::abs(_pos.y() - widgetRect.bottom());
        const auto isCursorCloserToTop = (distanceToWidgetTop < distanceToWidgetBottom);

        if (Direction_ == SelectDirection::NONE)
        {
            Direction_ = (isCursorCloserToTop ? SelectDirection::DOWN : SelectDirection::UP);
        }

        if (selected)
            return;

        const auto isDirectionDown = (Direction_ == SelectDirection::DOWN);
        const auto isDirectionUp = (Direction_ == SelectDirection::UP);

		if (isSelected())
		{
            const auto needToClear = (
                (isCursorCloserToTop && isDirectionDown) ||
                (!isCursorCloserToTop && isDirectionUp)
            );

			if (needToClear && (startSelectY_ < widgetRect.top() || startSelectY_ > widgetRect.bottom()))
            {
				clearSelection();
				isSelection_ = true;
            }

            return;
		}

        const auto needToSelect = (
            (isCursorCloserToTop && isDirectionUp) ||
            (!isCursorCloserToTop && isDirectionDown)
        );

		if (needToSelect)
        {
            select();
        }
	}

	void MessageItem::select()
	{
		HistoryControlPageItem::select();

		if (ContentWidget_)
        {
			ContentWidget_->select(true);
        }
	}

	void MessageItem::clearSelection()
	{
		isSelection_ = false;
        HistoryControlPageItem::clearSelection();

		if (MessageBody_)
        {
			MessageBody_->clearSelection();
        }

		if (ContentWidget_)
		{
			Direction_ = SelectDirection::NONE;

            if (ContentWidget_->hasTextBubble())
            {
                ContentWidget_->clearSelection();
            }

			if (isMessageBubbleVisible())
			{
			}
			else
			{
				ContentWidget_->select(false);
			}
		}
	}

	QString MessageItem::selection(bool _textonly/* = false*/) const
	{
        QString selectedText;
        if (ContentWidget_)
            selectedText = ContentWidget_->selectedText();

        if (!MessageBody_ && !isSelected() && selectedText.isEmpty())
        {
            return QString();
        }

		if (_textonly && MessageBody_ && !MessageBody_->isAllSelected())
        {
			return MessageBody_->selection();
        }

		QString displayName;

		if (isOutgoing())
        {
			displayName = MyInfo()->friendlyName();
        }
		else
        {
			displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::getContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        QString format = (
            _textonly ?
                "" :
                QString("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm")
                )
        );

        if (MessageBody_)
		{
            if (!MessageBody_->selection().isEmpty())
            {
			    format += MessageBody_->selection();
                return format;
            }
            return QString();
		}

        assert(isSelected() || !selectedText.isEmpty());

		if (!Data_->StickerText_.isEmpty())
        {
			format += Data_->StickerText_;
        }
		else
        {
            format += ContentWidget_->toString();
        }

        if (!_textonly && !format.isEmpty())
        {
            format += "\n";
        }

        return format;
	}

    bool MessageItem::isSelected() const
    {
        return HistoryControlPageItem::isSelected();
    }

    bool MessageItem::isTextSelected() const
    {
        return MessageBody_
            && !MessageBody_->selection().isEmpty();
    }

    void MessageItem::avatarClicked()
    {
        emit Utils::InterConnector::instance().profileSettingsShow(Data_->Sender_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_avatar);
    }

    void MessageItem::forwardRoutine()
    {
        if (isFileSharing())
        {
            HistoryControl::FileSharingWidget* p = qobject_cast<HistoryControl::FileSharingWidget*>(ContentWidget_);
            p->shareRoutine();
        }
        else
        {
            QList<Data::Quote> q;
            q.push_back(getQuote(true));
            emit forward(q);
        }
    }

    void MessageItem::setNotAuth(const bool _isNotAuth)
    {
        isNotAuth_ = _isNotAuth;
    }

    bool MessageItem::isNotAuth()
    {
        return isNotAuth_;
    }

	void MessageItem::setQuoteSelection()
	{
		QuoteAnimation_.startQuoteAnimation();
		if (ContentWidget_)
			ContentWidget_->startQuoteAnimation();
	}

	void MessageItem::menu(QAction* _action)
	{
		const auto params = _action->data().toMap();
		const auto command = params["command"].toString();

        if (command == "dev:copy_message_id")
        {
            const auto idStr = QString::number(getId());

            QApplication::clipboard()->setText(idStr);
        }

        QString displayName;

        if (isOutgoing())
        {
            displayName = MyInfo()->friendlyName();
        }
        else
        {
            displayName = (
                Data_->Chat_ ?
                    Sender_->text() :
                    Logic::getContactListModel()->getDisplayName(Data_->AimId_)
            );
        }

        auto format = (
            (command == "copy") ?
                QString() :
                QString("%1 (%2):\n").arg(
                    displayName,
                    QDateTime::fromTime_t(Data_->Time_).toString("dd.MM.yyyy hh:mm")
                )
        );

        if (MessageBody_)
        {
            format += MessageBody_->getPlainText();
        }
        else
        {
            if (!Data_->StickerText_.isEmpty())
            {
                format += Data_->StickerText_;
            }
            else
            {
                format += ContentWidget_->toString();
            }
        }

		if (command == "copy")
		{
			emit copy(format);
		}
		else if (command == "quote")
		{
            QList<Data::Quote> q;
            q.push_back(getQuote(true));
            emit quote(q);
		}
        else if (command == "forward")
        {
            forwardRoutine();
        }
        else if (command == "copy_link")
        {
            emit copy(ContentWidget_->toLink());
        }
        else if (command == "open_in_browser")
        {
            QDesktopServices::openUrl(ContentWidget_->toLink());
        }
        else if (command == "copy_file")
        {
            ContentWidget_->copyFile();
        }
        else if (command == "save_as")
        {
            ContentWidget_->saveAs();
        }
        else if ((command == "delete_all") || (command == "delete"))
        {
            const auto isPendingMessage = (Data_->Id_ <= -1);
            if (isPendingMessage)
            {
                return;
            }

            const auto is_shared = (command == "delete_all");
            std::vector<int64_t> ids;
            ids.push_back(Data_->Id_);

            QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Delete message"),
                NULL
            );

            if (confirm && Data_)
            {
                GetDispatcher()->deleteMessages(ids, Data_->AimId_, is_shared);
            }
        }
	}

    void MessageItem::onRecreateAvatarRect()
    {
        AvatarRect_ = QRect();
    }

    bool MessageItem::isMessageBubbleVisible() const
	{
		if (!ContentWidget_)
		{
			return true;
		}

		return ContentWidget_->isBlockElement();
	}

	void MessageItem::setMessage(const QString& _message)
	{
        assert(!ContentWidget_);

		Data_->Text_ = _message;

        if (!parent())
        {
            return;
        }

        setUpdatesEnabled(false);

        createMessageBody();

		MessageBody_->verticalScrollBar()->blockSignals(true);

        MessageBody_->document()->clear();

        const bool showLinks = (!isNotAuth() || isOutgoing());
		Logic::Text4Edit(_message, *MessageBody_, Logic::Text2DocHtmlMode::Escape, showLinks, true);

        MessageBody_->verticalScrollBar()->blockSignals(false);

        MessageBody_->setVisible(true);

        Layout_->setDirty();

        setUpdatesEnabled(true);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateGeometry();
        update();
	}

    bool MessageItem::setLastRead(const bool _isLastRead)
    {
        HistoryControlPageItem::setLastRead(_isLastRead);

        if (_isLastRead == LastRead_)
        {
            return false;
        }

        LastRead_ = _isLastRead;

        Layout_->setDirty();

        updateGeometry();

        return true;
    }

    void MessageItem::setDeliveredToServer(const bool _delivered)
    {
        setOutgoing(Data_->isOutgoing(), true, Data_->Chat_);
        setTime(Data_->Time_);
    }

	void MessageItem::setOutgoing(const bool _isOutgoing, const bool _isDeliveredToServer, const bool _isMChat, const bool _isInit)
	{
        Data_->Outgoing_.Outgoing_ = _isOutgoing;
        Data_->Outgoing_.Set_ = true;
        Data_->Chat_ = _isMChat;

        if (_isOutgoing && (Data_->deliveredToServer_ != _isDeliveredToServer || _isInit))
        {
            Data_->deliveredToServer_ = _isDeliveredToServer;

            if (!_isDeliveredToServer)
            {
                QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
                opacityEffect->setOpacity(0.5);
                setGraphicsEffect(opacityEffect);

                if (!_isInit)
                    update();
            }
            else
            {
                auto effect = graphicsEffect();
                if (effect)
                {
                    setGraphicsEffect(nullptr);
                    if (!_isInit)
                        update();
                }
            }
        }
	}

    void MessageItem::setMchatSenderAimId(const QString& _senderAimId)
    {
        MessageSenderAimId_ = _senderAimId;
    }

	void MessageItem::setMchatSender(const QString& _sender)
	{
        createSenderControl();

		Sender_->setText(_sender);
        Data_->SenderFriendly_ = _sender;

        auto senderName = _sender;
        Utils::removeLineBreaks(InOut senderName);

		Data_->SenderVisible_ = (!senderName.isEmpty() && Data_->AvatarVisible_);

        Sender_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

        updateGeometry();
	}

	void MessageItem::setTime(const int32_t _time)
	{
		Data_->Time_ = _time;

        TimeWidget_->setTime(_time);

        TimeWidget_->show();
	}

	void MessageItem::setTopMargin(const bool _value)
	{
        const auto topMarginUpdated = (hasTopMargin() != _value);
        if (topMarginUpdated)
        {
            AvatarRect_ = QRect();
        }

        Data_->IndentBefore_ = _value;

        HistoryControlPageItem::setTopMargin(_value);
	}

    themes::themePtr MessageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(Data_.get() ? Data_->AimId_ : getAimid());
    }

	void MessageItem::setContentWidget(HistoryControl::MessageContentWidget* _widget)
	{
		assert(_widget);
		assert(!ContentWidget_);
        assert(!MessageBody_);

		ContentWidget_ = _widget;

        connect(ContentWidget_, SIGNAL(stateChanged()), this, SLOT(updateData()), Qt::UniqueConnection);

        if (!parent())
        {
            return;
        }

        auto success = QObject::connect(
            ContentWidget_,
            &HistoryControl::MessageContentWidget::forcedLayoutUpdatedSignal,
            this,
            [this]
            {
                Layout_->setDirty();
            },
            Qt::DirectConnection
        );
        assert(success);

        ContentWidget_->setParent(this);
        ContentWidget_->show();

        TimeWidget_->raise();

		Utils::grabTouchWidget(ContentWidget_);

        updateGeometry();

        update();
	}

	void MessageItem::setStickerText(const QString& _text)
	{
		Data_->StickerText_ = _text;
	}

	void MessageItem::setDate(const QDate& _date)
	{
		Data_->Date_ = _date;
	}

	bool MessageItem::selected()
	{
		if (!MessageBody_)
		{
			return false;
		}

		return !MessageBody_->textCursor().selectedText().isEmpty();
	}

	QDate MessageItem::date() const
	{
		return Data_->Date_;
	}

	qint64 MessageItem::getId() const
	{
		return Data_->Id_;
	}

    bool MessageItem::isRemovable() const
	{
		return (
            ContentWidget_ ?
                ContentWidget_->canUnload() :
                true
        );
	}

    bool MessageItem::isUpdateable() const
    {
        return (
            ContentWidget_ ?
                ContentWidget_->canReplace() :
                true
        );
    }

	QString MessageItem::toLogString() const
	{
		QString result;
		result.reserve(512);

		QTextStream fmt(&result);

		if (ContentWidget_)
		{
			fmt << ContentWidget_->toLogString();
		}

		if (MessageBody_)
		{
			auto text = MessageBody_->getPlainText();
			text.replace("\n", "\\n");
			fmt << "	text=<" << text << ">";
		}

		return result;
	}

    bool MessageItem::updateData()
    {
        updateMessageBodyColor();

        updateSenderControlColor();

        update();

        return true;
    }

	void MessageItem::updateWith(MessageItem& _messageItem)
	{
        auto &data = _messageItem.Data_;
        assert(data);
        assert(Data_ != data);

		Data_ = std::move(data);
        setAimid(Data_->AimId_);

        if (_messageItem.MessageBody_)
        {
            assert(!Data_->Text_.isEmpty());
            assert(!_messageItem.ContentWidget_);

            if (ContentWidget_)
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;
            }

            setMessage(Data_->Text_);
        }

        if (_messageItem.ContentWidget_)
        {
            assert(Data_->Text_.isEmpty());
            assert(!_messageItem.MessageBody_);

            if (MessageBody_)
            {
                MessageBody_->deleteLater();
                MessageBody_ = nullptr;
            }

            if (ContentWidget_ && ContentWidget_->canReplace())
            {
                ContentWidget_->deleteLater();
                ContentWidget_ = nullptr;

                setContentWidget(_messageItem.ContentWidget_);
                _messageItem.ContentWidget_ = nullptr;
            }
        }

		setTime(Data_->Time_);
		setOutgoing(Data_->isOutgoing(), Data_->deliveredToServer_, Data_->Chat_);
		setTopMargin(Data_->IndentBefore_);

		if (Data_->AvatarVisible_)
        {
            loadAvatar(Utils::scale_bitmap(MessageStyle::getAvatarSize()));
        }
		else
        {
			setAvatarVisible(false);
        }

        setMchatSenderAimId(Data_->Sender_);
        setMchatSender(Data_->SenderFriendly_);
	}

	std::shared_ptr<MessageData> MessageItem::getData()
	{
		return Data_;
	}

    namespace
    {

        int32_t getMessageTopPadding()
        {
#ifdef __linux__
            return Utils::scale_value(7);
#else
            return Utils::scale_value(4);
#endif //__linux__
        }

        int32_t getMessageBottomPadding()
        {
            return Utils::scale_value(8);
        }

        QMap<QString, QVariant> makeData(const QString& _command)
        {
            QMap<QString, QVariant> result;
            result["command"] = _command;
            return result;
        }
    }
}


