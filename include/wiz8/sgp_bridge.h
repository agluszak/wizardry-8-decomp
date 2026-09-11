#pragma once

/* The complete C-language build boundary: the Wizardry symbols the original
   SGP C translation units reference directly. Everything else under
   include/wiz8 and src/wiz8 is ordinary C++ linkage, including globals with
   fixed original addresses and free functions. Keep this header to symbols
   with an actual src/sgp caller; do not use it to fix a linker name. */

#include "Types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { TIMER_SUSPEND = 1, TIMER_RESUME = 8 };

/* src/sgp/sgp.c calls MoveTimer(TIMER_SUSPEND) and MoveTimer(TIMER_RESUME)
   while it suspends and resumes the shared game timers. */
float MoveTimer(INT32 action);

/* src/sgp/mousesystem.c measures and draws fast-help text with the handle
   startup_subsystems.cpp loads from TinyMonoFont.sti. */
extern int ghTinyMonoFont;

#ifdef __cplusplus
}
#endif
