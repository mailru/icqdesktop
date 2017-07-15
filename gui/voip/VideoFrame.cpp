#include "stdafx.h"
#include "VideoFrame.h"
#include "CommonUI.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#elif _WIN32
    #include "win32/VideoFrameWin32.h"
#else
#ifndef STRIP_VOIP
    #error "video frame need to create"
#endif //STRIP_VOIP
#endif

platform_specific::GraphicsPanel* platform_specific::GraphicsPanel::create(
    QWidget* _parent,
    std::vector<Ui::BaseVideoPanel*>& _panels, bool primaryVideo)
{
#ifdef __APPLE__
    return  platform_macos::GraphicsPanelMacos::create(_parent, _panels, primaryVideo);
#elif _WIN32
    return  new platform_win32::GraphicsPanelWin32(_parent, _panels, primaryVideo);
#endif
    return nullptr;
}

WId platform_specific::GraphicsPanel::frameId() const
{
    return QWidget::winId();
}

void platform_specific::GraphicsPanel::addPanels(std::vector<Ui::BaseVideoPanel*>& /*panels*/)
{

}

void platform_specific::GraphicsPanel::clearPanels()
{
    
}

void platform_specific::GraphicsPanel::fullscreenModeChanged(bool /*fullscreen*/)
{
    
}

void platform_specific::GraphicsPanel::fullscreenAnimationStart() {}

void platform_specific::GraphicsPanel::fullscreenAnimationFinish() {}

