#include "stdafx.h"

#include "HistoryControlPageItem.h"
#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "MessageStyle.h"

namespace Ui
{
	HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
		: QWidget(parent)
        , QuoteAnimation_(parent)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , HasAvatarSet_(false)
        , Selected_(false)
        , isDeleted_(false)
	{
	}

    void HistoryControlPageItem::clearSelection()
    {
        if (Selected_)
        {
            update();
        }

        Selected_ = false;
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        assert(HasAvatarSet_);
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::select()
    {
        if (!Selected_)
        {
            update();
        }

        Selected_ = true;
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
        {
            return;
        }

        HasTopMargin_ = value;

        updateGeometry();
    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::onActivityChanged(const bool /*isActive*/)
    {
    }

    void HistoryControlPageItem::onVisibilityChanged(const bool /*isVisible*/)
    {
    }

    void HistoryControlPageItem::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {}

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;
        HasAvatarSet_ = true;

        updateGeometry();
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {

    }

    themes::themePtr HistoryControlPageItem::theme() const
    {
        return get_qt_theme_settings()->themeForContact(aimId_);
    }

    bool HistoryControlPageItem::setLastRead(const bool /*_isLastRead*/)
    {
        return false;
    }

    void HistoryControlPageItem::setDeliveredToServer(const bool _delivered)
    {

    }

    void HistoryControlPageItem::drawLastReadAvatar(QPainter& _p, const QString& _aimid, const QString& _friendly, const int _rightPadding, const int _bottomPadding)
    {
        const QRect rc = rect();

        const int size = Utils::scale_bitmap(MessageStyle::getLastReadAvatarSize());

        bool isDefault = false;
        QPixmap avatar = *Logic::GetAvatarStorage()->GetRounded(_aimid, _friendly, size, QString(), true, isDefault, false, false);

        const int avatarSize = MessageStyle::getLastReadAvatarSize();

        if (!avatar.isNull())
        {
            _p.drawPixmap(
                rc.right() - avatarSize - _rightPadding,
                rc.bottom() - avatarSize - _bottomPadding,
                avatarSize,
                avatarSize,
                avatar);
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }


    void HistoryControlPageItem::setDeleted(const bool _isDeleted)
    {
        isDeleted_ = _isDeleted;
    }

    bool HistoryControlPageItem::isDeleted() const
    {
        return isDeleted_;
    }

    void HistoryControlPageItem::setAimid(const QString &aimId)
    {
        assert(!aimId.isEmpty());

        if (aimId == aimId_)
        {
            return;
        }

        assert(aimId_.isEmpty());
        aimId_ = aimId;
    }
}