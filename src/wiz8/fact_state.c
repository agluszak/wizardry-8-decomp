#include "gameplay_boundaries.h"

#include <stdlib.h>
#include <string.h>

/* 0x00404FB0, declared as in chunk_io.cpp: the write counterpart of
   ReadVirtualFile. Ghidra carries it as FileWrite(hFile, pDest, uiBytesToWrite,
   puiBytesWritten). */
extern unsigned char WriteVirtualFile(int handle, const void* buffer, unsigned int size,
                                      unsigned int* done);

// FUNCTION: WIZ8 0x00506480
/* The whole 1001-byte fact array minus its last entry goes to the save file in
   one write. The original passes the address of its own parameter as the
   bytes-written out-parameter: the handle has already been copied into a
   register, so the incoming slot is dead and doubles as the scratch the callee
   requires. Reproduced literally, because a separate local would cost a stack
   frame the canonical body does not have. */
void SaveFactState(int save_handle)
{
    WriteVirtualFile(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
}

// FUNCTION: WIZ8 0x00506310
/* Clears every fact, then seeds the ones a fresh party starts with. A party
   imported from Wizardry 7 gets a different set, keyed off the option byte and
   flag mask the importer unpacked.

   The canonical body clears the suppression flag twice, once inside the
   innermost branch ahead of an early return and once at the exit. That
   duplication is VC6's, not the source's: writing both out costs a byte,
   because the compiler then merges the two argument cleanups into one
   add esp,0x10 where the original keeps an add esp,0xc and a pop ecx. One
   trailing call, duplicated by the compiler, is byte-exact. */
void InitializeFactState(void)
{
    memset(g_fact_values, 0, 1000);
    SetFactNotificationsSuppressed(1);
    if (g_import_party_loaded) {
        SetFact(0x75, 1, 0);
        switch (g_import_ending_choice) {
        case 1:
            SetFact(0x4c, 1, 0);
            break;
        case 2:
            SetFact(0x4b, 1, 0);
            break;
        default:
            SetFact(0x4d, 1, 0);
            break;
        }
        if (g_import_flags[0x0b]) {
            SetFact(0x199, 1, 0);
        }
        if (g_import_flags[0x05]) {
            SetFact(0x7b, 1, 0);
        }
    } else {
        SetFact(0x4e, 1, 0);
        SetFact(0x279, 1, 0);
        SetFact(0x27a, 1, 0);
        SetFact(0x27b, 1, 0);
    }
    SetFactNotificationsSuppressed(0);
}
