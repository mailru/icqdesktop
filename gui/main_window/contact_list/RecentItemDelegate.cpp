#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModelDLG.h"

#include "ContactList.h"
#include "../history_control/MessagesModel.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "RecentsItemRenderer.h"

#include "../../gui_settings.h"

namespace Logic
{
	RecentItemDelegate::RecentItemDelegate(QObject* parent)
		: AbstractItemDelegateWithRegim(parent)
		, StateBlocked_(false)
	{
	}

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const Data::DlgState& dlg, bool dragOverlay) const
	{
        if (dlg.AimId_ == "favorites" || dlg.AimId_ == "recents")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());

            ContactList::RenderServiceItem(*painter, dlg.GetText(), dlg.AimId_ == "favorites", dlg.AimId_ == "favorites" /* drawLine */, viewParams_);

            painter->restore();
            return;
        }
        else if (dlg.AimId_ == "unknowns")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());
            
            ContactList::RenderUnknownsHeader(*painter, QT_TRANSLATE_NOOP("contact_list", "Unknown contacts"), Logic::getUnknownsModel()->totalUnreads(), viewParams_);
            
            painter->restore();
            return;
        }
        else if (dlg.AimId_ == "contacts" || dlg.AimId_ == "all messages")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::TextAntialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->translate(option.rect.topLeft());

            auto text = dlg.GetText();
            ContactList::RenderServiceItem(*painter, text, dlg.AimId_ == "favorites" /* renderState */, dlg.AimId_ == "favorites" /* drawLine */, viewParams_);

            painter->restore();
            return;
        }

		const auto isMultichat = Logic::getContactListModel()->isChat(dlg.AimId_);
		auto state = isMultichat ? QString() : Logic::getContactListModel()->getState(dlg.AimId_);

        if ((state == "online" || state == "mobile") && (option.state & QStyle::State_Selected))
            state += "_active";

		const auto isFilled = !isMultichat;
		
        bool isDefaultAvatar = false;
        QString aimId = dlg.AimId_;
        if (aimId == "mail")
        {
            auto i1 = dlg.Friendly_.indexOf('<');
            auto i2 = dlg.Friendly_.indexOf('>');
            if (i1 != -1 && i2 != -1)
                aimId = dlg.Friendly_.mid(i1 + 1, dlg.Friendly_.length() - i1 - (dlg.Friendly_.length() - i2 + 1));
        }
		auto avatar = GetAvatarStorage()->GetRounded(
            aimId,
            QString(),
            Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).avatarSize()),
            state,
            isFilled,
            isDefaultAvatar,
            false,
            ContactList::GetRecentsParams(viewParams_.regim_).isCL()
        );

        const auto hasMouseOver = (platform::is_apple() ? Logic::getRecentsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
		const auto isSelected = (option.state & QStyle::State_Selected) && !StateBlocked_;
        const auto isHovered = (option.state & QStyle::State_MouseOver) && !StateBlocked_ && !isSelected && hasMouseOver;

		auto displayName = dlg.AimId_ == "mail" ? dlg.Friendly_ : Logic::getContactListModel()->getDisplayName(dlg.AimId_);

        auto message = dlg.GetText();

        bool isTyping = false;

        if (viewParams_.regim_ != ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
            for (auto iter_typing = typings_.rbegin(); iter_typing != typings_.rend(); ++iter_typing)
            {
                if (iter_typing->aimId_ == dlg.AimId_)
                {
                    isTyping = true;

                    message.clear();

                    if (isMultichat)
                    {
                        message += iter_typing->getChatterName() + " ";
                    }

                    message += QT_TRANSLATE_NOOP("contact_list", "typing...");

                    break;
                }
            }

        const auto isOfficial = dlg.Official_ || Logic::getContactListModel()->isOfficial(dlg.AimId_);
        auto isDrawLastRead = false;

        QPixmap lastReadAvatar;

        bool isOutgoing = dlg.Outgoing_;
        bool isLastRead = (dlg.LastMsgId_ >= 0 && dlg.TheirsLastRead_ > 0 && dlg.LastMsgId_ <= dlg.TheirsLastRead_);

        if (!isMultichat && isOutgoing && isLastRead && !Logic::GetMessagesModel()->hasPending(dlg.AimId_))
        {
            lastReadAvatar = *GetAvatarStorage()->GetRounded(dlg.AimId_, QString(), Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).getLastReadAvatarSize())
                , QString(), isFilled, isDefaultAvatar, false, ContactList::GetRecentsParams(viewParams_.regim_).isCL());
            isDrawLastRead = true;
        }

        if (dlg.AimId_ == "mail" && dlg.MailId_.isEmpty())
            displayName = message;

		ContactList::RecentItemVisualData visData(
			dlg.AimId_,
			*avatar,
			state,
            message,
			isHovered,
			isSelected,
            displayName.isEmpty() ? dlg.AimId_ : displayName,
			true /* hasLastSeen */,
			QDateTime::fromTime_t(dlg.Time_),
			(int)dlg.UnreadCount_, 
            Logic::getContactListModel()->isMuted(dlg.AimId_),
            dlg.senderNick_,
            isOfficial,
            isDrawLastRead,
            lastReadAvatar,
            isTyping,
            dlg.SearchTerm_,
            dlg.HasLastMsgId(),
            dlg.SearchedMsgId_);

        if (dlg.AimId_ == "mail" && dlg.MailId_.isEmpty())
        {
            visData.IsMailStatus_ = true;
        }

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		painter->setRenderHint(QPainter::TextAntialiasing);
		painter->setRenderHint(QPainter::SmoothPixmapTransform);
		painter->translate(option.rect.topLeft());

		ContactList::RenderRecentsItem(*painter, visData, viewParams_);

        if (dragOverlay)
            ContactList::RenderRecentsDragOverlay(*painter, viewParams_);

		painter->restore();
	}

	void RecentItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		Data::DlgState dlg = index.data(Qt::DisplayRole).value<Data::DlgState>();
        if (dlg.AimId_ == "snaps")
            return;

		return paint(painter, option, dlg, index == DragIndex_);
	}

	QSize RecentItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& i) const
	{
        auto width = ContactList::GetRecentsParams(viewParams_.regim_).itemWidth();

        if (viewParams_.regim_ == ::Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            if (Logic::getSearchModelDLG()->isServiceItem(i.row()))
            {
                return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).serviceItemHeight());
            }
        }
        else
        {
            if (Logic::getRecentsModel()->isSnapsVisible() && i.row() == 0)
                return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).snapItemHeight());

            if (Logic::getRecentsModel()->isServiceItem(i))
            {
                if (Logic::getRecentsModel()->isUnknownsButton(i))
                    return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).unknownsItemHeight());
                else
                    return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).serviceItemHeight());
            }
        }
		return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).itemHeight());
	}

	QSize RecentItemDelegate::sizeHintForAlert() const
	{
		return QSize(ContactList::GetRecentsParams(viewParams_.regim_).itemWidth(), ContactList::GetRecentsParams(viewParams_.regim_).itemHeight());
	}

	void RecentItemDelegate::blockState(bool value)
	{
		StateBlocked_= value;
	}
    
    void RecentItemDelegate::addTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter == typings_.end())
        {
            typings_.push_back(_typing);
        }
    }

    void RecentItemDelegate::removeTyping(const TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter != typings_.end())
        {
            typings_.erase(iter);
        }
    }

    void RecentItemDelegate::setDragIndex(const QModelIndex& index)
    {
        DragIndex_ = index;
    }

    void RecentItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool RecentItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void RecentItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }

    void RecentItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

	RecentItemDelegate::ItemKey::ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber)
		: IsSelected(isSelected)
		, IsHovered(isHovered)
		, UnreadDigitsNumber(unreadDigitsNumber)
	{
		assert(unreadDigitsNumber >= 0);
		assert(unreadDigitsNumber <= 2);
	}

	bool RecentItemDelegate::ItemKey::operator < (const ItemKey &_key) const
	{
		if (IsSelected != _key.IsSelected)
		{
			return (IsSelected < _key.IsSelected);
		}

		if (IsHovered != _key.IsHovered)
		{
			return (IsHovered < _key.IsHovered);
		}

		if (UnreadDigitsNumber != _key.UnreadDigitsNumber)
		{
			return (UnreadDigitsNumber < _key.UnreadDigitsNumber);
		}

		return false;
	}
}
