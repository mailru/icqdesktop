#include "stdafx.h"
#include "VideoFrameWin32.h"
#include <windows.h>
#include "../CommonUI.h"

platform_win32::GraphicsPanelWin32::GraphicsPanelWin32(
    QWidget* _parent,
    std::vector<Ui::BaseVideoPanel*>&/* panels*/, bool /*primaryVideo*/)
    : platform_specific::GraphicsPanel(_parent)
{

    //for (unsigned ix = 0; ix < panels.size(); ix++) {
    //    assert(parent);
    //    if (!parent) { continue; }

    //    assert(panels[ix]);
    //    if (!panels[ix]) { continue; }
    //    
    //    HWND panelView = (HWND)panels[ix]->winId();
    //    assert(panelView);
    //    if (!panelView) { continue; }

    //    HWND parentView = (HWND)parent->winId();
    //    assert(parentView);
    //    if (!parentView) { continue; }
    //    
    //    ::SetParent(panelView, parentView);
    //    DWORD style = ::GetWindowLong(panelView, GWL_STYLE);
    //    style &= ~(WS_POPUP | WS_CAPTION);
    //    style |= WS_CHILD;
    //    SetWindowLong(panelView, GWL_STYLE, style);
    //}
}

platform_win32::GraphicsPanelWin32::~GraphicsPanelWin32()
{

}
