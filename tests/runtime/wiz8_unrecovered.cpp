/* Runtime-only support for the generated unrecovered-call traps. This object
   is linked into the runnable products only, never into the matching image,
   and is compiled without optimization or frame-pointer omission so the
   debugger sees an ordinary stack. */

#include "wiz8/runtime_unrecovered.h"

#include <stdio.h>
#include <windows.h>

/* The VC6 CRT startup objects are not part of these links. The float marker is
   an ordinary dword; the SEH chain head is the absolute zero symbol the CRT's
   dllsupp/exsup objects define, which the generated alias object supplies. */
extern "C" {
int _fltused = 0;
}

void Wiz8UnrecoveredFunctionTrap(const Wiz8UnrecoveredFunction* function)
{
    if (function->retail_address != 0) {
        fprintf(stderr, "WIZ8_RUNTIME_STUB address=%08lx symbol=%s name=%s\n",
                function->retail_address, function->linker_symbol, function->name);
    }
    else {
        fprintf(stderr, "WIZ8_RUNTIME_STUB address=unmapped symbol=%s name=%s\n",
                function->linker_symbol, function->name);
    }
    fflush(stderr);
    DebugBreak();
    for (;;) {
    }
}
