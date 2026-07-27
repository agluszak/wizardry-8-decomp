#include "surrender/srBinIStream.h"

/* Returns a byte-sized boolean, not an int: the canonical body tests it with
   `neg al`, which an int-returning declaration cannot produce. */
extern "C" unsigned char ReadVirtualFile(int handle, void* buffer, unsigned long size,
                                         unsigned long* done);

/* The SurRender-facing stream adapter that carries the SLF virtual file system
   into the SR stream hierarchy. It is declared as what it is rather than as an
   opaque prefix: the two vtables the constructor at 0x0047CBD0 installs are the
   two this declaration produces, and every slot in them is accounted for by
   evidence/snapshots/polymorphism/slots.csv.

   Primary 0x005EC6A0, at offset 0, has vget imported from SR.DLL in slot 0 and
   Read in slot 1 - the slot srBinIStream leaves pure. Secondary 0x005EC68C, the
   virtual srBinStream base at +0x10, inherits getSize from SR.DLL in slot 1 and
   overrides the destructor and the three seek/tell slots locally.

   The 0x20-byte size the sole caller of the constructor allocates is what the
   assertion below checks, and it holds only if the srBinIStream base really is
   vptr, vbptr and a virtual srBinStream subobject placed last. */
class W8VirtualFileBinIStream : public srBinIStream {
public:
    /* Primary vtable slot 1. */
    virtual unsigned long vread(void* buffer, unsigned long size);

    int m_hFile;                            /* 0x08 */
    /* The vbtable places the virtual base at +0x10 and the sole caller of the
       constructor allocates 0x20, so these four bytes belong to this class.
       Nothing recovered so far reads or writes them. */
    unsigned char unknown_0c[4];            /* 0x0c */
};

typedef char W8VirtualFileBinIStream_size_must_be_0x20[
    sizeof(W8VirtualFileBinIStream) == 0x20 ? 1 : -1];

// Note the original reuses the `size` parameter slot as the completed-count
// out-parameter, and returns it branchlessly.
// FUNCTION: WIZ8 0x0047D5C0
unsigned long W8VirtualFileBinIStream::vread(void* buffer, unsigned long size)
{
    if (ReadVirtualFile(m_hFile, buffer, size, &size)) {
        return size;
    }
    return 0;
}
