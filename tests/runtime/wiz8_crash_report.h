#ifndef WIZ8_CRASH_REPORT_H
#define WIZ8_CRASH_REPORT_H

#include <stdio.h>

/* Optional extra context printed by the shared crash filter. The runtime-test
   harness uses it for its screen-state counters; the product leaves it unset. */
typedef void(__cdecl* W8CrashContextWriter)(FILE* stream);

void W8SetCrashContextWriter(W8CrashContextWriter writer);

#endif
