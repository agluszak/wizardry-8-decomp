#pragma once

#include "Button System.h"

/* SurRender's button userdata slot is an INT32. Wizardry stores an object
   pointer in that slot, so these two adapters keep the pointer/integer crossing
   here instead of at every dialog call site. SurRender's own interface stays
   integer-typed. */
inline void SetButtonUserDataPointer(INT32 button, void* data)
{
    MSYS_SetBtnUserData(
        button, 0,
        reinterpret_cast<INT32>(data)); // reinterpret-ok: SGP userdata slot carries the pointer
}

template <typename T> inline T* GetButtonUserDataPointer(GUI_BUTTON* button)
{
    return reinterpret_cast<T*>(
        MSYS_GetBtnUserData(button, 0)); // reinterpret-ok: SGP userdata slot carries the pointer
}
