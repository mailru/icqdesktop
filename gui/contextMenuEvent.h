#pragma once

class ContextMenuCreateEvent : public QEvent
{
public:
    /// create or destroy context menu
    ContextMenuCreateEvent();
    static QEvent::Type type();

private:
    static QEvent::Type EventType_;
};

class ContextMenuDestroyEvent : public QEvent
{
public:
    /// create or destroy context menu
    ContextMenuDestroyEvent();
    static QEvent::Type type();

private:
    static QEvent::Type EventType_;
};
