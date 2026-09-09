#pragma once

#include "input.h"
#include "wiz8/wiz8_windows.h"

// Storage owned by released SGP input.c and sgp.c and shared with Wizardry.
// Public input state remains declared by the dependency's input.h.
extern "C" {
extern UINT16 gusQueueCount;
extern UINT16 gusHeadIndex;
extern UINT16 gusTailIndex;
extern UINT16 gfShiftState;
extern UINT16 gfCtrlState;
extern UINT16 gfAltState;
extern UINT16 gusRecordedKeyState;
extern BOOLEAN gfTrackMousePos;
extern BOOLEAN gfTrackDblClick;
extern BOOLEAN gfRecordedLeftButtonUp;
extern UINT32 guiDoubleClkDelay;
extern UINT32 guiSingleClickTimer;
extern UINT32 guiLeftButtonRepeatTimer;
extern UINT32 guiRightButtonRepeatTimer;
extern HHOOK ghKeyboardHook;
extern HHOOK ghMouseHook;
extern BOOLEAN gfCurrentStringInputState;
extern StringInput* gpCurrentStringDescriptor;
extern BOOLEAN gfApplicationActive;
}
