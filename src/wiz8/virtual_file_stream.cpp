#include "wiz8/gameplay_boundaries.h"

/* Returns a byte-sized boolean, not an int: the canonical body tests it with
   `neg al`, which an int-returning declaration cannot produce. */
extern "C" unsigned char ReadVirtualFile(int handle, void* buffer, unsigned int size,
                                         unsigned int* done);

/* The SurRender-facing stream adapter whose constructor at 0x0047CBD0 installs
   the imported srBinIStream vftables and virtually inherits srBinStream at
   +0x10. Only the handle at +8 is established here; +0 is the primary vptr and
   +4 the vbptr, so they stay opaque. */
struct W8VirtualFileBinIStream {
    unsigned char unknown_00[8];
    int m_hFile;                            /* 0x08 */

    unsigned int Read(void* buffer, unsigned int size);
};

// Primary vtable slot 1. Note the original reuses the `size` parameter slot as
// the completed-count out-parameter, and returns it branchlessly.
// FUNCTION: WIZ8 0x0047D5C0
unsigned int W8VirtualFileBinIStream::Read(void* buffer, unsigned int size)
{
    if (ReadVirtualFile(m_hFile, buffer, size, &size)) {
        return size;
    }
    return 0;
}
