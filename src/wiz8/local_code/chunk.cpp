#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/chunk.h"
#include "wiz8/virtual_file.h"

#define CHUNK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\chunk.cpp"

// FUNCTION: WIZ8 0x0055ca20
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

// FUNCTION: WIZ8 0x0055ca80
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
