#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/chunk.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#define CHUNK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\chunk.cpp"

enum { W8_RIFF_CHUNK_ID = 0x46464952 };

// FUNCTION: WIZ8 0x0055c000
unsigned char W8Chunk::OpenRead(char* path)
{
    W8ChunkHead* head;

    if (m_hFile != 0) {
        return 0;
    }
    m_hFile = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (m_hFile == 0) {
        return 0;
    }
    m_fWriting = 0;
    ReadChunkHeader(0, 0);
    head = m_heads.data[m_heads.count - 1];
    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x1f0, 0);
    }
    if (head->chunk_id != W8_RIFF_CHUNK_ID) {
        return 0;
    }
    SkipCurrentChunk();
    return 1;
}

// FUNCTION: WIZ8 0x0055c080
unsigned char W8Chunk::OpenReadWrite(char* path)
{
    W8ChunkHead* head;

    if (m_hFile != 0) {
        return 0;
    }
    m_hFile = FileOpen(path, FILE_ACCESS_READWRITE | FILE_OPEN_EXISTING, 0);
    if (m_hFile == 0) {
        return 0;
    }
    m_fWriting = 0;
    ReadChunkHeader(0, 0);
    head = m_heads.data[m_heads.count - 1];
    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x1f0, 0);
    }
    if (head->chunk_id != W8_RIFF_CHUNK_ID) {
        return 0;
    }
    SkipCurrentChunk();
    return 1;
}

/* Move past the unread remainder of the current head. Its absolute extent is
   combined with the active group's base offset and the current file position. */
// FUNCTION: WIZ8 0x0055c390
unsigned char W8Chunk::SkipCurrentChunk()
{
    W8ChunkHead* head = m_heads.data[m_heads.count - 1];
    int position;
    int distance;

    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x136, 0);
    }
    position = FileGetPos(m_hFile);
    distance = m_offsets.data[m_offsets.count - 1] + (head->extent_08 - position);
    if (distance != 0) {
        FileSeek(m_hFile, distance, FILE_SEEK_FROM_CURRENT);
    }
    return 1;
}

// FUNCTION: WIZ8 0x0055c660
unsigned int W8Chunk::CurrentChunkId()
{
    W8ChunkHead* head = m_heads.data[m_heads.count - 1];

    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x1f0, 0);
    }
    return head->chunk_id;
}

// FUNCTION: WIZ8 0x0055c690
int W8Chunk::CurrentChunkExtent()
{
    W8ChunkHead* head = m_heads.data[m_heads.count - 1];

    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x204, 0);
    }
    return head->extent_08;
}

/* The active group count is the top of the first integer navigation stack. */
// FUNCTION: WIZ8 0x0055c6c0
int W8Chunk::ChunkCount()
{
    return m_group_counts.data[m_group_counts.count - 1];
}

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

// FUNCTION: WIZ8 0x0055cae0
void W8Chunk::RewindCurrentChunk()
{
    FileSeek(m_hFile, m_offsets.data[m_offsets.count - 1], FILE_SEEK_FROM_START);
}

// FUNCTION: WIZ8 0x0055cb60
unsigned char W8Chunk::CurrentChunkAtEnd()
{
    W8ChunkHead* head = m_heads.data[m_heads.count - 1];

    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x303, 0);
    }
    return head->at_end;
}
