#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/*
 * Minimal product PCH surface required by Compression.c.
 *
 * The release build has no _DEBUG, so MemAlloc and MemFree reduce to the CRT
 * allocator. Assert likewise compiles away. ZLIB.H is the pinned SGP copy of
 * the zlib 1.0.4 public header used by this translation unit.
 */
#include "types.h"
#include "MemMan.h"
#include "DEBUG.H"
#include "ZLIB.H"
#include "Compression.h"

#endif
