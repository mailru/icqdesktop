#include "stdafx.h"
#include "contextMenuEvent.h"

QEvent::Type ContextMenuCreateEvent::EventType_ = QEvent::None;
QEvent::Type ContextMenuDestroyEvent::EventType_ = QEvent::None;

ContextMenuCreateEvent::ContextMenuCreateEvent() :
    QEvent(ContextMenuCreateEvent::type())
{
}

QEvent::Type ContextMenuCreateEvent::type()
{
    if (EventType_ == QEvent::None)
        EventType_ = (QEvent::Type)QEvent::registerEventType();

    return EventType_;
}

////////////////////////////////////////////////////////////////////////////
ContextMenuDestroyEvent::ContextMenuDestroyEvent() :
    QEvent(ContextMenuDestroyEvent::type())
{
}

QEvent::Type ContextMenuDestroyEvent::type()
{
    if (EventType_ == QEvent::None)
        EventType_ = (QEvent::Type)QEvent::registerEventType();

    return EventType_;
}
