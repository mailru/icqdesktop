#pragma once

#include "Common.h"
#include "../../types/contact.h"

namespace ContactList
{
	void RenderServiceContact(QPainter& _painter, const bool _isHovered, const bool _isActive, QString _name, Data::ContactType _type, int _leftMargin, const ViewParams& _viewParams);

	void RenderContactItem(QPainter& _painter, VisualDataBase _item, ViewParams _viewParams);

	void RenderGroupItem(QPainter& _painter, const QString &_groupName, const ViewParams& _viewParams);

    void RenderContactsDragOverlay(QPainter& _painter);
}
