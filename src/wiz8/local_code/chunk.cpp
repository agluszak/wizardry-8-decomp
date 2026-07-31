#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/chunk.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#define CHUNK_CPP "C:\\Projects\\Wizardry 8\\Local Code\\chunk.cpp"

enum { W8_RIFF_CHUNK_ID = 0x46464952 };

/* The two scalars are initialised, not assigned: retail sets them before any of
   the four vectors is built, and they are declared first, so they can only come
   from a member initialiser list. The vectors are ordinary members and build
   themselves in declaration order after them. */
// FUNCTION: WIZ8 0x0055bce0
W8Chunk::W8Chunk()
    : m_hFile(0), m_fWriting(0)
{
}

/* Empty. The four member vectors release their own backing storage in reverse
   declaration order and nothing else happens - in particular the heads are not
   deleted here, because W8Chunk removes and deletes each one as it releases it
   rather than at teardown. */
// FUNCTION: WIZ8 0x0055bde0
W8Chunk::~W8Chunk()
{
}

// FUNCTION: WIZ8 0x0055ca20
__forceinline unsigned char W8Chunk::Read(void* buffer, unsigned int size,
                                          unsigned int* transferred)
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
__forceinline unsigned char W8Chunk::Write(const void* buffer, unsigned int size,
                                           unsigned int* transferred)
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
    OpenChunk(0, 0);
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
    OpenChunk(0, 0);
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

/* Close the root chunk and then its file. In write mode the root's stored
   child count is patched before the ordinary current-chunk finalizer writes
   the root extent. */
// FUNCTION: WIZ8 0x0055c100
void W8Chunk::Close()
{
    if (m_hFile != 0) {
        if (m_fWriting) {
            int position = FileGetPos(m_hFile);
            int count;

            FileSeek(m_hFile, m_offsets.data[m_offsets.count - 1], FILE_SEEK_FROM_START);
            count = m_group_progress.RemoveAt(m_group_progress.count - 1);
            Write(&count, sizeof(count), 0);
            FileSeek(m_hFile, position, FILE_SEEK_FROM_START);
        }
        else {
            m_group_counts.RemoveAt(m_group_counts.count - 1);
        }
        ReleaseCurrentChunk();
        if (m_hFile != 0) {
            FileClose(m_hFile);
            m_hFile = 0;
        }
        m_fWriting = 0;
    }
}

/* Materialize the source's active payload, reproduce its tag and grouping bit
   in this write stream, and finalize the copy as one complete chunk. */
// FUNCTION: WIZ8 0x0055c1e0
unsigned char W8Chunk::CopyCurrentChunkFrom(W8Chunk* source)
{
    W8ChunkHead* source_head = source->m_heads.data[source->m_heads.count - 1];
    unsigned int transferred;
    unsigned int extent;
    unsigned char* contents;

    if (source_head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x204, 0);
    }
    extent = source_head->extent_08;
    if (!m_fWriting) {
        return 0;
    }
    contents = static_cast<unsigned char*>(::operator new(extent));
    if (contents == 0) {
        return 0;
    }
    source->Read(contents, extent, &transferred);
    if (extent != transferred) {
        ::operator delete(contents);
        return 0;
    }
    source_head = source->m_heads.data[source->m_heads.count - 1];
    if (source_head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x166, 0);
    }
    OpenChunk(source->CurrentChunkId(), source_head->unknown_04);
    Write(contents, extent, &transferred);
    if (extent != transferred) {
        ::operator delete(contents);
        ReleaseCurrentChunk();
        return 0;
    }
    ReleaseCurrentChunk();
    ::operator delete(contents);
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

/* The on-disk header deliberately writes its four established fields
   separately: bytes 6 and 7 are object padding, not file data. A zero tag in
   read mode asks the stream to populate the header; write mode requires the
   caller to supply the tag. */
// FUNCTION: WIZ8 0x0055c6d0
unsigned char W8Chunk::OpenChunk(unsigned int chunk_id, unsigned char grouped)
{
    W8ChunkHead* head = new W8ChunkHead;
    unsigned int transferred;

    if (head == 0) {
        srAssertFail("pHead", CHUNK_CPP, 0x229, 0);
    }
    else {
        head->chunk_id = 0;
        head->unknown_04 = 0;
        head->at_end = 0;
        head->extent_08 = 0;
    }
    if (m_fWriting) {
        if (chunk_id == 0) {
            srAssertFail("chunkID!=0", CHUNK_CPP, 0x22d, 0);
        }
        head->chunk_id = chunk_id;
        head->extent_08 = 0;
        head->unknown_04 = grouped;
        Write(&head->chunk_id, 4, &transferred);
        Write(&head->unknown_04, 1, &transferred);
        Write(&head->at_end, 1, &transferred);
        Write(&head->extent_08, 4, &transferred);
    }
    else {
        Read(&head->chunk_id, 4, &transferred);
        Read(&head->unknown_04, 1, &transferred);
        Read(&head->at_end, 1, &transferred);
        Read(&head->extent_08, 4, &transferred);
    }
    m_heads.Add(head);
    m_offsets.Add(FileGetPos(m_hFile));
    return 1;
}

/* Remove the active header. Writers patch its extent in place; readers only
   discard the saved payload offset. A nested group advances its parent's
   completed-child count. */
// FUNCTION: WIZ8 0x0055c930
unsigned char W8Chunk::ReleaseCurrentChunk()
{
    delete m_heads.RemoveAt(m_heads.count - 1);
    if (m_fWriting) {
        int end = FileGetPos(m_hFile);
        int payload = m_offsets.RemoveAt(m_offsets.count - 1);
        int extent = end - payload;

        FileSeek(m_hFile, payload - 4, FILE_SEEK_FROM_START);
        Write(&extent, sizeof(extent), 0);
        FileSeek(m_hFile, end, FILE_SEEK_FROM_START);
    }
    else {
        m_offsets.RemoveAt(m_offsets.count - 1);
    }
    if (m_group_progress.count != 0) {
        m_group_progress.data[m_group_progress.count - 1] =
            m_group_progress.data[m_group_progress.count - 1] + 1;
    }
    return 1;
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
