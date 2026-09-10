#pragma once

#include "Button System.h"

/* SurRender's button userdata slot is an integer. These two adapters keep the
   pointer/integer crossing at this boundary instead of at every dialog call
   site. */
inline void SetButtonUserData(INT32 button, void* data)
{
    MSYS_SetBtnUserData(button, 0, reinterpret_cast<INT32>(data));
}

inline void* GetButtonUserData(GUI_BUTTON* button)
{
    return reinterpret_cast<void*>(MSYS_GetBtnUserData(button, 0));
}
