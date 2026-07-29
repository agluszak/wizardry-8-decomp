#pragma once

struct W8ChunkHead {
    unsigned int chunk_id;                   /* 0x00 */
    unsigned int unknown_04;
    int extent_08;                           /* 0x08 */
};

static_assert(sizeof(W8ChunkHead) == 0x0c, "W8ChunkHead_size_must_be_0x0c");

/* Local Code\chunk.cpp. m_fWriting is named by the canonical assertions at
   lines 640 and 669, which guard the two directions against each other; the
   handle at +0 is whatever ReadVirtualFile/WriteVirtualFile take.  The two
   head stacks and their indices are established by the five current-head
   consumers at 0x0055C000..0x0055C690. */
struct W8Chunk {
    int m_hFile;                            /* 0x00 */
    unsigned char m_fWriting;               /* 0x04 */
    unsigned char unknown_05[7];
    int head_count_0c;                      /* 0x0c */
    unsigned int unknown_10;
    W8ChunkHead** heads_14;                 /* 0x14 */
    unsigned char unknown_18[0x14];
    int secondary_head_count_2c;            /* 0x2c */
    unsigned int unknown_30;
    W8ChunkHead** secondary_heads_34;       /* 0x34 */

    unsigned char OpenRead0055C000(char* path);
    unsigned char OpenReadWrite0055C080(char* path);
    unsigned char FinishCurrentHead0055C390();
    unsigned int CurrentChunkID0055C660();
    int CurrentChunkExtent0055C690();
    unsigned char Read(void* buffer, unsigned int size, unsigned int* transferred);
    unsigned char Write(const void* buffer, unsigned int size, unsigned int* transferred);

    void Function55C6D0(unsigned int chunk_id, int value);
};

static_assert(sizeof(W8Chunk) == 0x38, "W8Chunk_size_must_be_0x38");
