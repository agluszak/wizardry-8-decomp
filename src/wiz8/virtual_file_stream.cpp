#include "surrender/srBinIStream.h"
#include "surrender/srCore.h"
#include "surrender/srExtension.h"
#include "surrender/srIStreamOpener.h"
#include "wiz8/virtual_file.h"
#include "wiz8/virtual_file_stream.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

/* The retained FileMan implementation owns the physical/SLF handle split.
   These reviewed first-party entry points keep Wizardry's historical names
   while delegating to that source-backed interface. */
// FUNCTION: WIZ8 0x00404e10
extern "C" void CloseVirtualFile(int handle)
{
    FileClose(handle);
}

// FUNCTION: WIZ8 0x00404ea0
extern "C" unsigned char ReadVirtualFile(
    int handle, void* buffer, unsigned int size, unsigned int* done)
{
    return FileRead(handle, buffer, size, done);
}

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
// FUNCTION: WIZ8 0x0047d5c0
unsigned long W8VirtualFileBinIStream::vread(void* buffer, unsigned long size)
{
    /* SurRender spells its 32-bit count unsigned long; SGP spells the same
       ABI word UINT32 (unsigned int). The canonical body reuses this parameter
       slot, so keep that ownership explicit at the header boundary. */
    if (ReadVirtualFile(
            m_hFile, buffer, size, reinterpret_cast<unsigned int*>(&size))) {
        return size;
    }
    return 0;
}

srBinIStream* W8VirtualFileStreamOpener::open(char* path)
{
    return new W8VirtualFileBinIStream(path);
}

const char* W8VirtualFileStreamOpener::getDescription() const
{
    return "stBinIStream";
}

W8VirtualFileStreamOpener g_virtual_file_stream_opener_65a124;

/* Loads the image importers and routes their JPG/TGA reads through Wizardry's
   SLF-aware virtual file stream, which is the bridge the real menu assets use. */
// FUNCTION: WIZ8 0x0047d5f0
extern "C" void Function47D5F0(void)
{
    srExtension::load("JPEGImporter", NULL);
    srExtension::load("TargaImporter", NULL);
    srCore.getIStreamOpener()->addStreamType(
        &g_virtual_file_stream_opener_65a124, "jpg");
    srCore.getIStreamOpener()->addStreamType(
        &g_virtual_file_stream_opener_65a124, "tga");
}
