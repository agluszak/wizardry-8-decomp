#pragma once

/* The runtime-only identity of one generated fallback for an unrecovered
   retail callable. The generated trap records carry the exact decorated linker
   symbol, so the debugger maps a stop back to the retail entity without any
   stack reconstruction. */

struct Wiz8UnrecoveredFunction {
    unsigned long retail_address;
    const char* linker_symbol;
    const char* name;
};

/* Announce the stub on stderr and stop with the caller's stack intact. The
   generated thunks never return. */
void Wiz8UnrecoveredFunctionTrap(const Wiz8UnrecoveredFunction* function);
