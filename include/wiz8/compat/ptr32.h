#pragma once

/* Pointer slots in records that are serialized as raw bytes. The historical
   build retains real four-byte pointers; ports must handle the disk boundary. */
#define W8_PTR32(T) T*
