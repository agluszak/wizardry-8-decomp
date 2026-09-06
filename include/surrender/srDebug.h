#pragma once

#include <ostream>
#include "srHeap.h"

// Export-proven declarations; this grouping's original header name is unknown.
SR_DLL_IMPORT long __cdecl srDebugPrintf(unsigned long level, const char* format, ...);
SR_DLL_IMPORT long __cdecl srStreamPrintf(std::ostream& stream, const char* format, ...);
