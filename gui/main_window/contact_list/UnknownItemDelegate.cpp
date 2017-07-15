#include "stdafx.h"
#include "UnknownItemDelegate.h"

#include "ContactList.h"
#include "ContactListModel.h"
#include "UnknownsModel.h"
#include "../history_control/MessagesModel.h"
#include "../../gui_settings.h"
#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/contact_list/RecentsItemRenderer.h"


namespace Logic
{
    UnknownItemDelegate::ItemKey::ItemKey(
        const bool _isSelected,
        const bool _isHovered,
        const int _unreadDigitsNumber)
        : isSelected_(_isSelected)
        , isHovered_(_isHovered)
        , unreadDigitsNumber_(_unreadDigitsNumber)
    {
        assert(_unreadDigitsNumber >= 0);
        assert(_unreadDigitsNumber <= 2);
    }

    bool UnknownItemDelegate::ItemKey::operator < (const ItemKey& _key) const
    {
        if (isSelected_ != _key.isSelected_)
        {
            return (isSelected_ < _key.isSelected_);
        }

        if (isHovered_ != _key.isHovered_)
        {
            return (isHovered_ < _key.isHovered_);
        }

        if (unreadDigitsNumber_ != _key.unreadDigitsNumber_)
        {
            return (unreadDigitsNumber_ < _key.unreadDigitsNumber_);
        }

        return false;
    }

    UnknownItemDelegate::UnknownItemDelegate(QObject* _parent)
        : QItemDelegate(_parent)
        , stateBlocked_(false)
    {
        viewParams_.regim_ = ::Logic::MembersWidgetRegim::UNKNOWN;
    }

    void UnknownItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const Data::DlgState& _dlg, bool _fromAlert, bool _dragOverlay) const
    {
        if (_dlg.AimId_ == "delete_all")
        {
            _painter->save();
            _painter->setRenderHint(QPainter::Antialiasing);
            _painter->setRenderHint(QPainter::TextAntialiasing);
            _painter->setRenderHint(QPainter::SmoothPixmapTransform);
            _painter->translate(_option.rect.topLeft());
            
            const bool hasMouseOver = (platform::is_apple() ? Logic::getUnknownsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
            const bool isSelected_ = (_option.state & QStyle::State_Selected) && !stateBlocked_;
            const bool isHovered_ = (_option.state & QStyle::State_MouseOver) && !stateBlocked_ && !isSelected_ && hasMouseOver;

            ContactList::RenderDeleteAllItem(* _painter, QT_TRANSLATE_NOOP("contact_list", "Close All"), isHovered_, viewParams_);
            
            _painter->restore();
            return;
        }

        const auto isMultichat = Logic::getContactListModel()->isChat(_dlg.AimId_);
        const auto state = isMultichat ? QString() : Logic::getContactListModel()->getState(_dlg.AimId_);

        const auto isFilled = !isMultichat;
        bool isDefault = false;

        const auto avatar = *GetAvatarStorage()->GetRounded(
            _dlg.AimId_,
            QString(),
            Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).avatarSize()),
            state,
            isFilled,
            isDefault,
            false,
            ContactList::GetRecentsParams(viewParams_.regim_).isCL()
        );

        const bool hasMouseOver = (platform::is_apple() ? Logic::getUnknownsModel()->customFlagIsSet(Logic::CustomAbstractListModelFlags::HasMouseOver) : true);
        const bool isSelected_ = (_option.state & QStyle::State_Selected) && !stateBlocked_;
        const bool isHovered_ = (_option.state & QStyle::State_MouseOver) && !stateBlocked_ && !isSelected_ && hasMouseOver;

        const auto displayName = Logic::getContactListModel()->getDisplayName(_dlg.AimId_);

        auto message = _dlg.GetText();

        bool isOfficial = _dlg.Official_ || Logic::getContactListModel()->isOfficial(_dlg.AimId_);

        bool isDrawLastRead = false;

        QPixmap lastReadAvatar;

        bool isOutgoing = _dlg.Outgoing_;
        bool isLastRead = (_dlg.LastMsgId_ >= 0 && _dlg.TheirsLastRead_ > 0 && _dlg.LastMsgId_ <= _dlg.TheirsLastRead_);

        if (!isMultichat && isOutgoing && isLastRead && !Logic::GetMessagesModel()->hasPending(_dlg.AimId_))
        {
            lastReadAvatar = *GetAvatarStorage()->GetRounded(_dlg.AimId_, QString(), Utils::scale_bitmap(ContactList::GetRecentsParams(viewParams_.regim_).getLastReadAvatarSize())
                , QString(), isFilled, isDefault, false, ContactList::GetRecentsParams(viewParams_.regim_).isCL());
            isDrawLastRead = true;
        }

        ContactList::RecentItemVisualData visData(
            _dlg.AimId_,
            avatar,
            state,
            message,
            isHovered_,
            isSelected_,
            displayName.isEmpty() ? _dlg.AimId_ : displayName,
            true /* hasLastSeen */,
            QDateTime::fromTime_t(_dlg.Time_),
            (int)_dlg.UnreadCount_, 
            Logic::getContactListModel()->isMuted(_dlg.AimId_),
            _dlg.senderNick_,
            isOfficial,
            isDrawLastRead,
            lastReadAvatar,
            false /* isTyping */);

        _painter->save();
        _painter->setRenderHint(QPainter::Antialiasing);
        _painter->setRenderHint(QPainter::TextAntialiasing);
        _painter->setRenderHint(QPainter::SmoothPixmapTransform);
        _painter->translate(_option.rect.topLeft());

        ContactList::RenderRecentsItem(*_painter, visData, viewParams_);

        if (_dragOverlay)
            ContactList::RenderRecentsDragOverlay(*_painter, viewParams_);

        _painter->restore();
    }

    void UnknownItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        Data::DlgState dlg = _index.data(Qt::DisplayRole).value<Data::DlgState>();
        return paint(_painter, _option, dlg, false, _index == dragIndex_);
    }

    bool UnknownItemDelegate::isInAddContactFrame(const QPoint& _p) const
    {
        auto f = ContactList::GetRecentsParams(viewParams_.regim_).addContactFrame();
        return f.contains(_p);
    }
    
    bool UnknownItemDelegate::isInRemoveContactFrame(const QPoint& _p) const
    {
        auto f = ContactList::GetRecentsParams(viewParams_.regim_).removeContactFrame();
        return f.contains(_p);
    }

    bool UnknownItemDelegate::isInDeleteAllFrame(const QPoint& _p) const
    {
        auto f = ContactList::DeleteAllFrame();
        return f.contains(_p);
    }

    QSize UnknownItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& _i) const
    {
        auto width = ContactList::GetRecentsParams(viewParams_.regim_).itemWidth();
        if (Logic::getUnknownsModel()->isServiceItem(_i))
            return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).unknownsItemHeight());

        return QSize(width, ContactList::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    QSize UnknownItemDelegate::sizeHintForAlert() const
    {
        return QSize(ContactList::GetRecentsParams(viewParams_.regim_).itemWidth(), ContactList::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    void UnknownItemDelegate::blockState(bool _value)
    {
        stateBlocked_ = _value;
    }
    
    void UnknownItemDelegate::setDragIndex(const QModelIndex& _index)
    {
        dragIndex_ = _index;
    }
    
    void UnknownItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }
    
    bool UnknownItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }
    
    void UnknownItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }
}
