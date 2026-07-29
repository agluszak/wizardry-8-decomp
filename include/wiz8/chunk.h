#pragma once

#include "wiz8/vector.h"

struct W8ChunkHead {
    unsigned int chunk_id;                   /* 0x00 */
    unsigned char unknown_04;
    unsigned char at_end;                    /* 0x05 */
    unsigned char unknown_06[2];
    int extent_08;                           /* 0x08 */
};

static_assert(sizeof(W8ChunkHead) == 0x0c, "W8ChunkHead_size_must_be_0x0c");

/* This specialization has a distinct vtable from W8GrowableVector<int> and
   owns heap-allocated chunk heads. Its destructor is not recovered yet, so
   preserve the established vector layout without claiming an element-template
   identity. */
struct W8ChunkHeadVectorStorage {
    void* vptr;
    int count;
    int capacity;
    W8ChunkHead** data;
};

/* Local Code\chunk.cpp. The constructor at 0x0055BCE0 initializes this complete
   0x48-byte object: the asserted direction state, one owning head vector and
   three W8GrowableVector<int> navigation stacks. */
struct W8Chunk {
    int m_hFile;                            /* 0x00 */
    unsigned char m_fWriting;               /* 0x04 */
    unsigned char padding_05[3];            /* 0x05 */
    W8ChunkHeadVectorStorage m_heads;        /* 0x08 */
    W8GrowableVector<int> m_group_counts;   /* 0x18 */
    W8GrowableVector<int> m_offsets;        /* 0x28 */
    W8GrowableVector<int> m_group_progress; /* 0x38 */

    W8Chunk();
    ~W8Chunk();

    unsigned char OpenRead(char* path);
    unsigned char OpenReadWrite(char* path);
    void Close();
    unsigned char CopyCurrentChunkFrom(W8Chunk* source);
    unsigned char SkipCurrentChunk();
    unsigned char OpenChunkGroup();
    unsigned char CloseChunkGroup();
    unsigned int CurrentChunkId();
    int CurrentChunkExtent();
    int ChunkCount();
    unsigned char ReadChunkHeader(unsigned int chunk_id, int value);
    unsigned char Read(void* buffer, unsigned int size, unsigned int* transferred);
    unsigned char Write(const void* buffer, unsigned int size, unsigned int* transferred);
    void RewindCurrentChunk();
    unsigned char ReleaseCurrentChunk();
    unsigned char CurrentChunkAtEnd();
};

static_assert(sizeof(W8Chunk) == 0x48, "W8Chunk_size_must_be_0x48");
