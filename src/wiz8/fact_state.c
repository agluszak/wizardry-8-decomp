#include "gameplay_boundaries.h"

#include <stdlib.h>

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
