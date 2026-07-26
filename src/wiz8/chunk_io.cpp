#include "gameplay_boundaries.h"
#include "sr_api.h"

extern "C" unsigned char ReadVirtualFile(int handle, void* buffer, unsigned int size,
                                         unsigned int* done);
/* 0x00404FB0: the write counterpart of ReadVirtualFile, sitting between it and
   SeekVirtualFile in the same virtual-file family. Descriptive name. */
extern "C" unsigned char WriteVirtualFile(int handle, const void* buffer, unsigned int size,
                                          unsigned int* done);

#define CHUNK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\chunk.cpp"

/* Local Code\chunk.cpp. m_fWriting is named by the canonical assertions at
   lines 640 and 669, which guard the two directions against each other; the
   handle at +0 is whatever ReadVirtualFile/WriteVirtualFile take. */
struct W8Chunk {
    int m_hFile;                            /* 0x00 */
    unsigned char m_fWriting;               /* 0x04 */

    unsigned char Read(void* buffer, unsigned int size, unsigned int* transferred);
    unsigned char Write(const void* buffer, unsigned int size, unsigned int* transferred);
};

// FUNCTION: WIZ8 0x0055CA20
unsigned char W8Chunk::Read(void* buffer, unsigned int size, unsigned int* transferred)
{
    unsigned int done;
    unsigned char result;

    if (m_fWriting) {
        srAssertFail("!m_fWriting", CHUNK_CPP, 0x280, 0);
    }
    result = ReadVirtualFile(m_hFile, buffer, size, &done);
    if (transferred) {
        *transferred = done;
    }
    return result;
}

// FUNCTION: WIZ8 0x0055CA80
unsigned char W8Chunk::Write(const void* buffer, unsigned int size, unsigned int* transferred)
{
    unsigned int done;
    unsigned char result;

    if (!m_fWriting) {
        srAssertFail("m_fWriting", CHUNK_CPP, 0x29d, 0);
    }
    result = WriteVirtualFile(m_hFile, buffer, size, &done);
    if (transferred) {
        *transferred = done;
    }
    return result;
}
