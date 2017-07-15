#pragma once

#include "Common.h"

namespace ContactList
{

	struct RecentItemVisualData : VisualDataBase
	{
		RecentItemVisualData(
			const QString &_aimId,
			const QPixmap &_avatar,
			const QString &_state,
			const QString &_status,
			const bool _isHovered,
			const bool _isSelected,
			const QString &_contactName,
			const bool _hasLastSeen,
			const QDateTime &_lastSeen,
			const int _unreadsCounter,
			const bool _muted,
            const QString &_senderNick,
            const bool _isOfficial,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const bool _isTyping,
            const QString _term = "",
            const bool _hasLastMsg = true,
            const qint64 _msgId = -1);

		const bool Muted_;

        const bool IsTyping_;

        QString senderNick_;

        bool HasLastMsg_;

        qint64 msgId_;

        bool IsMailStatus_;
	};

	void RenderRecentsItem(QPainter &_painter, const RecentItemVisualData &_item, const ViewParams& _viewParams);

    void RenderRecentsDragOverlay(QPainter &_painter, const ViewParams& _viewParams);

    void RenderUnknownsHeader(QPainter &_painter, const QString& _title, const int _count, const ViewParams& _viewParams);

    void RenderServiceItem(QPainter &_painter, const QString& _text, bool _renderState, bool _isFavorites, const ViewParams& _viewParams);

    void RenderDeleteAllItem(QPainter &_painter, const QString& _title, bool _isMouseOver, const ViewParams& _viewParams);

	int RenderContactMessage(QPainter &_painter, const RecentItemVisualData &_visData, const int _rightMargin, const ViewParams& _viewParams, ContactListParams& _recentParams);

	int RenderNotifications(QPainter &_painter, const int _unreads, bool _muted, const ViewParams& viewParams, ContactListParams& _recentParams, bool _isUnknownHeader, bool _isActive, bool _isHover);

    void RenderLastReadAvatar(QPainter &_painter, const QPixmap& _avatar, const int _xOffset, ContactListParams& _recentParams);

    QRect DeleteAllFrame();
}
