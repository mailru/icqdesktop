#ifndef __VIDEO_FRAME_WIN32_H__
#define __VIDEO_FRAME_WIN32_H__

#include <QWidget>
#include "../VideoFrame.h"

namespace Ui
{
	class BaseVideoPanel;
}

namespace platform_win32
{
    
class GraphicsPanelWin32 : public platform_specific::GraphicsPanel
{
    Q_OBJECT
public:
    GraphicsPanelWin32(QWidget* _parent, std::vector<Ui::BaseVideoPanel*>& _panels, bool primaryVideo);
    virtual ~GraphicsPanelWin32();
};

}

#endif//__VIDEO_FRAME_WIN32_H__
