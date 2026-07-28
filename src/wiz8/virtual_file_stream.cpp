#include "surrender/srBinIStream.h"
#include "surrender/srCore.h"
#include "surrender/srIStreamOpener.h"

#include <stdlib.h>
#include <string.h>

/* Returns a byte-sized boolean, not an int: the canonical body tests it with
   `neg al`, which an int-returning declaration cannot produce. */
extern "C" unsigned char ReadVirtualFile(int handle, void* buffer, unsigned long size,
                                         unsigned long* done);
extern "C" int FileOpen(const char* path, int mode, int flags);
extern "C" unsigned char FileSeek(int handle, int offset, int origin);
extern "C" int FileGetPos(int handle);
extern "C" unsigned char FileRead(
    int handle, void* buffer, unsigned long size, unsigned long* done);
extern "C" void FileClose(int handle);

/* The retained FileMan implementation owns the physical/SLF handle split.
   These reviewed first-party entry points keep Wizardry's historical names
   while delegating to that source-backed interface. */
// FUNCTION: WIZ8 0x00404E10
extern "C" void CloseVirtualFile(int handle)
{
    FileClose(handle);
}

// FUNCTION: WIZ8 0x00404EA0
extern "C" unsigned char ReadVirtualFile(
    int handle, void* buffer, unsigned long size, unsigned long* done)
{
    return FileRead(handle, buffer, size, done);
}

class __declspec(dllimport) srExtension {
public:
    static srExtension* load(const char* name, const char* path);
};

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
#pragma vtordisp(off)
class W8VirtualFileBinIStream : public srBinIStream {
public:
    W8VirtualFileBinIStream(char* path);
    virtual ~W8VirtualFileBinIStream();

    /* Primary vtable slot 1. */
    virtual unsigned long vread(void* buffer, unsigned long size);
    virtual srBinStream& seek(unsigned long position, e_seekDir direction);
    virtual srBinStream& seek(unsigned long position);
    virtual unsigned long tell();

    int m_hFile;                            /* 0x08 */
    /* The vbtable places the virtual base at +0x10 and the sole caller of the
       constructor allocates 0x20, so these four bytes belong to this class.
       Nothing recovered so far reads or writes them. */
    unsigned char unknown_0c[4];            /* 0x0c */
};
#pragma vtordisp(on)

typedef char W8VirtualFileBinIStream_size_must_be_0x20[
    sizeof(W8VirtualFileBinIStream) == 0x20 ? 1 : -1];

W8VirtualFileBinIStream::W8VirtualFileBinIStream(char* path)
    : m_hFile(0)
{
    char* normalized;
    unsigned long index;

    normalized = (char*)malloc(strlen(path) + 1);
    if (!normalized) {
        setState(SR_STREAM_ERROR);
        return;
    }
    strcpy(normalized, path);
    for (index = 0; normalized[index] != '\0'; ++index) {
        if (normalized[index] == '/') {
            normalized[index] = '\\';
        }
    }
    m_hFile = FileOpen(normalized, 0x41, 0);
    free(normalized);
    setState(m_hFile == 0 ? SR_STREAM_ERROR : SR_STREAM_OK);
}

W8VirtualFileBinIStream::~W8VirtualFileBinIStream()
{
    if (m_hFile) {
        CloseVirtualFile(m_hFile);
    }
}

srBinStream& W8VirtualFileBinIStream::seek(
    unsigned long position, e_seekDir direction)
{
    static const int origins[] = {1, 4, 2};
    if (!FileSeek(m_hFile, position, origins[direction])) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

srBinStream& W8VirtualFileBinIStream::seek(unsigned long position)
{
    if (!FileSeek(m_hFile, position, 1)) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

unsigned long W8VirtualFileBinIStream::tell()
{
    return FileGetPos(m_hFile);
}

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

class W8VirtualFileStreamOpener : public srIStreamOpener::Opener {
public:
    virtual srBinIStream* open(char* path)
    {
        return new W8VirtualFileBinIStream(path);
    }

    virtual const char* getDescription() const
    {
        return "stBinIStream";
    }
};

W8VirtualFileStreamOpener g_virtual_file_stream_opener_65a124;

/* Loads the image importers and routes their JPG/TGA reads through Wizardry's
   SLF-aware virtual file stream, which is the bridge the real menu assets use. */
// FUNCTION: WIZ8 0x0047D5F0
extern "C" void Function47D5F0(void)
{
    srExtension::load("JPEGImporter", NULL);
    srExtension::load("TargaImporter", NULL);
    srCore.getIStreamOpener()->addStreamType(
        &g_virtual_file_stream_opener_65a124, "jpg");
    srCore.getIStreamOpener()->addStreamType(
        &g_virtual_file_stream_opener_65a124, "tga");
}
