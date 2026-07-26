#pragma once

/* Local Code\chunk.cpp. m_fWriting is named by the canonical assertions at
   lines 640 and 669, which guard the two directions against each other; the
   handle at +0 is whatever ReadVirtualFile/WriteVirtualFile take. */
struct W8Chunk {
    int m_hFile;                            /* 0x00 */
    unsigned char m_fWriting;               /* 0x04 */

    unsigned char Read(void* buffer, unsigned int size, unsigned int* transferred);
    unsigned char Write(const void* buffer, unsigned int size, unsigned int* transferred);
};
