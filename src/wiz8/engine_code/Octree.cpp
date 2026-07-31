#include <cstdlib>
#include <cstring>

#include "surrender/srHeap.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/geometry.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <math.h>

#define OCTREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp"

extern W8Object0043A910* g_object_6598bc;

extern unsigned long g_octree_storage_00659770;
extern unsigned long g_octree_state_00659890;
extern void Function46CDC0(void);
extern void Function46CDD0(void);
extern void Function4331F0(void* value);
extern void Function432D60(void* value);
extern void Function459400(int value);
extern void Function439140(void);
extern void Function457B10(void* value);
extern void Function434020(int value);
extern unsigned char g_navigator_link_mode_00659c10;
extern unsigned int Function004669B0(
    W8NavigatorMovement004572C0* movement, float radius, float separation);
extern unsigned int Function00467150(
    W8NavigatorMovement004572C0* movement, float radius, float separation);
/* 0x0045F2D0 consumes the settled pair; its own body is not recovered. */
extern void ApplyPortalTransition0045F2D0(
    const W8Position* destination, const W8Position* source);
extern float g_rate_006068EC;
extern float g_world_scale_005ebc40;
extern const double g_zero_005ebb40;

/* ReadOctFile's own direct callees. Their bodies are not recovered, so they
   keep address-qualified names. */
extern void PrepareOctreeLoad0046CCC0(int value);
extern int CheckLevelAssetSet0042CCC0(const char* level_path);
extern char BuildPreprocessedFiles00492E60(const char* level_path);
extern void ReportStartupMessage004969D0(const char* message);
extern unsigned char BitArrayLoad0043AEC0(BitArray* bits, int handle);
extern void* CreateGameData00449010(int handle, int value);
extern unsigned char ReadLevelName00432E90(const char* name);
extern void ApplyLevelName00432B80(const char* name);
extern void ReadWaypointFile0043A0F0(void);
/* 0x00659888 accumulates every byte the loader reads, and 0x00652DB0 caches the
   game-data block LoadWorld hands back through its out parameter. */
extern unsigned long g_octree_bytes_read_00659888;
extern void* g_octree_game_data_00652db0;

namespace {

void DestroyBitArray(BitArray*& bits)
{
    if (bits != 0) {
        bits->FreeIndex();
        operator delete(bits);
        bits = 0;
    }
}

struct W8OctreeIndex {
    void* buckets;
    void* entries;
    long last_entry;
    unsigned long bucket_count;
};

void DestroyIndex(W8OctreeIndex* index)
{
    if (index != 0) {
        operator delete(index->buckets);
        operator delete(index->entries);
        operator delete(index);
    }
}

W8OctreeIndex* CreateIndex()
{
    W8OctreeIndex* index = new W8OctreeIndex;
    long* entries = new long[12];
    long* buckets = new long[4];
    int position;

    for (position = 0; position < 4; ++position) {
        buckets[position] = -1;
        entries[position * 3] = position + 1;
        entries[position * 3 + 1] = -1;
        entries[position * 3 + 2] = -1;
    }
    entries[9] = -1;
    index->buckets = buckets;
    index->entries = entries;
    index->last_entry = 0;
    index->bucket_count = 4;
    return index;
}

template <class T>
T ReadHeader(const unsigned char* header, unsigned int offset)
{
    T result;
    memcpy(&result, header + offset, sizeof(result));
    return result;
}

template <class T>
void WriteMember(W8Octree* octree, unsigned int offset, T value)
{
    memcpy(reinterpret_cast<unsigned char*>(octree) + offset, &value, sizeof(value));
}

} // namespace

/* Read one .oct file into a fresh octree.

   The original name is the image's own: fourteen assertions in this body spell
   it ReadOctFile. The shape is one block repeated for every table the file
   carries - allocate, read, accumulate the byte count, and on either failure
   copy a message into the local buffer and stop advancing. fSuccess threading
   is what makes the failures cascade rather than each one returning; the flag
   at +0x000 bit 31 is the load error the caller tests.

   A null path builds an empty octree, and a level whose preprocessed files
   cannot be built runs on the LVL file alone with the same error bit set. */
// FUNCTION: WIZ8 0x0042bc10
W8Octree::W8Octree(const char* path, void** game_data)
{
    unsigned char header[0xf5];
    char acMessage[256];
    int hOctFile;
    unsigned int uiRead;
    int uiTerminator;
    unsigned char fSuccess;
    unsigned char fLoaded;
    void* block;
    unsigned int index;
    unsigned int limit;
    unsigned int name_length;
    char* extension;
    void* pGameData = 0;

    PrepareOctreeLoad0046CCC0(0);
    Reset();
    if (path == 0) {
        Initialize(0);
        return;
    }
    if (CheckLevelAssetSet0042CCC0(path) < 0) {
        goto failed;
    }
    if (CheckLevelAssetSet0042CCC0(path) > 0 &&
        BuildPreprocessedFiles00492E60(path) == 0) {
        g_octree_6598a4 = 0;
        m_flags_000 |= 0x80000000;
        ReportStartupMessage004969D0("Cannot find or build current preprocessed files.");
        ReportStartupMessage004969D0(
            "Attempting to run with LVL file only -- SOME FEATURES DISABLED.");
        ReportStartupMessage004969D0(0);
        return;
    }
    hOctFile = FileOpen(const_cast<char*>(path), 1, 0);
    if (hOctFile == 0) {
        srAssertFail("hOctFile", OCTREE_CPP, 0xa3,
                     "ReadOctFile: Couldn't open octree file.");
    }
    fSuccess = ReadVirtualFile(hOctFile, header, 0xf5, &uiRead);
    g_octree_bytes_read_00659888 += uiRead;
    fLoaded = 0;
    if (fSuccess != 0) {
        fSuccess = ReadVirtualFile(hOctFile, &uiTerminator, 4, &uiRead);
        if (fSuccess == 0 || uiTerminator != -1) {
            srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0xac,
                         "ReadOctFile: Header of Oct file longer than expected.");
        }
        fLoaded = 0;
        if (fSuccess != 0) {
            Initialize(header);
            name_length = strlen(path) + 1;
            if (name_length != 1) {
                if (m_owned_0c0 != 0) {
                    free(m_owned_0c0);
                }
                m_owned_0c0 = malloc(name_length);
                if (m_owned_0c0 != 0) {
                    strcpy(static_cast<char*>(m_owned_0c0), path);
                    extension = strrchr(static_cast<char*>(m_owned_0c0), '.');
                    if (extension != 0 &&
                        extension - static_cast<char*>(m_owned_0c0) >
                            (int)(name_length - 7)) {
                        *extension = '\0';
                    }
                }
            }

            block = malloc((ReadHeader<unsigned long>(header, 0x6a) * 9 + 0x12) * 4);
            m_owned_09c = block;
            if (block == 0) {
                fSuccess = 0;
                strcpy(acMessage, "ReadOctFile: Couldn't allocate octree nodes.");
            } else {
                fSuccess = ReadVirtualFile(
                    hOctFile, block, ReadHeader<unsigned long>(header, 0x6a) * 0x24, &uiRead);
                if (fSuccess == 0) {
                    strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
                }
            }
            g_octree_bytes_read_00659888 += uiRead;
            fLoaded = 0;
            if (fSuccess != 0) {
                block = malloc((ReadHeader<unsigned long>(header, 0x6e) * 5 + 10) * 8);
                m_owned_0a0 = block;
                if (block == 0) {
                    fSuccess = 0;
                    strcpy(acMessage, "ReadOctFile: Couldn't allocate octree leaves.");
                } else {
                    fSuccess = ReadVirtualFile(
                        hOctFile, block,
                        ReadHeader<unsigned long>(header, 0x6e) * 0x28, &uiRead);
                    if (fSuccess == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read octree leaves.");
                    }
                }
                g_octree_bytes_read_00659888 += uiRead;
                fLoaded = 0;
                if (fSuccess != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x82) * 4 + 8);
                    m_owned_0d0 = block;
                    if (block == 0) {
                        fLoaded = 0;
                        strcpy(acMessage,
                               "ReadOctFile: Couldn't allocate polygon index list for leaves.");
                    } else {
                        fLoaded = ReadVirtualFile(
                            hOctFile, block,
                            ReadHeader<unsigned long>(header, 0x82) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage,
                                   "ReadOctFile: Couldn't read polygon index list for leaves.");
                        }
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
            }
        }
    }

    limit = m_positional_0a4 * m_positional_0a8 * m_positional_0ac;
    if (fLoaded != 0 && limit < 250000) {
        block = malloc(limit * 4);
        m_owned_0b0 = block;
        if (block == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate polygon index list for regions.");
            goto finish;
        }
        fLoaded = ReadVirtualFile(
            hOctFile, block,
            m_positional_0a4 * m_positional_0a8 * m_positional_0ac * 4, &uiRead);
        if (fLoaded == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
        }
        g_octree_bytes_read_00659888 += uiRead;
    } else {
        m_owned_0b0 = 0;
    }

    fSuccess = 0;
    if (fLoaded != 0) {
        block = malloc(ReadHeader<unsigned long>(header, 0x72) * 4 + 8);
        m_owned_0d4 = block;
        if (block == 0) {
            fSuccess = 0;
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Poly Lookup table.");
        } else {
            fLoaded = ReadVirtualFile(
                hOctFile, block, ReadHeader<unsigned long>(header, 0x72) * 4, &uiRead);
            if (fLoaded == 0) {
                strcpy(acMessage, "ReadOctFile: Couldn't read Poly Lookup table.");
            }
            g_octree_bytes_read_00659888 += uiRead;
            fSuccess = 0;
            if (fLoaded != 0) {
                if (ReadHeader<unsigned long>(header, 0x92) != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x92) * 2 + 4);
                    m_owned_148 = block;
                    if (block == 0) {
                        fSuccess = 0;
                        strcpy(acMessage, "ReadOctFile: Couldn't allocate region list.");
                        goto finish;
                    }
                    fLoaded = ReadVirtualFile(
                        hOctFile, block,
                        ReadHeader<unsigned long>(header, 0x92) * 2, &uiRead);
                    if (fLoaded == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read region list.");
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
                fSuccess = 0;
                if (fLoaded != 0) {
                    if (ReadHeader<unsigned long>(header, 0x86) != 0) {
                        block = malloc(ReadHeader<unsigned long>(header, 0x86) * 4 + 8);
                        m_owned_12c = block;
                        if (block == 0) {
                            fSuccess = 0;
                            strcpy(acMessage, "ReadOctFile: Couldn't allocate GD Poly list.");
                            goto finish;
                        }
                        fLoaded = ReadVirtualFile(
                            hOctFile, block,
                            ReadHeader<unsigned long>(header, 0x86) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage, "ReadOctFile: Couldn't read GD Poly list.");
                        }
                        g_octree_bytes_read_00659888 += uiRead;
                    }
                    fSuccess = 0;
                    if (fLoaded != 0) {
                        if (ReadHeader<unsigned long>(header, 0x8a) != 0) {
                            block = malloc(ReadHeader<unsigned long>(header, 0x8a) * 2 + 4);
                            m_owned_130 = block;
                            if (block == 0) {
                                fSuccess = 0;
                                strcpy(acMessage,
                                       "ReadOctFile: Couldn't allocate Trigger list.");
                                goto finish;
                            }
                            fLoaded = ReadVirtualFile(
                                hOctFile, block,
                                ReadHeader<unsigned long>(header, 0x8a) * 2, &uiRead);
                            if (fLoaded == 0) {
                                strcpy(acMessage, "ReadOctFile: Couldn't read Trigger list.");
                            }
                            g_octree_bytes_read_00659888 += uiRead;
                        }
                        fSuccess = 0;
                        if (fLoaded != 0) {
                            if (ReadHeader<unsigned short>(header, 0x96) > 1) {
                                block = malloc(
                                    (ReadHeader<unsigned short>(header, 0x96) + 2) * 0xe8);
                                m_pRegions_05c = block;
                                if (block == 0) {
                                    fSuccess = 0;
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't allocate region array.");
                                    goto finish;
                                }
                                fLoaded = ReadVirtualFile(
                                    hOctFile, block,
                                    ReadHeader<unsigned short>(header, 0x96) * 0xe8, &uiRead);
                                if (fLoaded == 0) {
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't read region array.");
                                }
                                g_octree_bytes_read_00659888 += uiRead;
                            }
                            fSuccess = 0;
                            if (fLoaded != 0) {
                                fLoaded = ReadVirtualFile(
                                    hOctFile, &uiTerminator, 4, &uiRead);
                                if (fLoaded == 0 || uiTerminator != -1) {
                                    srAssertFail(
                                        "fSuccess && (uiTerminator==0xffffffff)",
                                        OCTREE_CPP, 0x15f,
                                        "ReadOctFile: PreRegions longer than expected.");
                                }
                                fSuccess = 0;
                                if (fLoaded != 0) {
                                    if (ReadHeader<unsigned long>(header, 0x66) != 0) {
                                        block = malloc(
                                            (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10);
                                        m_owned_0d8 = block;
                                        if (block == 0) {
                                            fLoaded = 0;
                                            strcpy(acMessage,
                                                   "ReadOctFile: Couldn't allocate submesh array.");
                                        } else {
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, block,
                                                (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10,
                                                &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read submesh array.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            if (fLoaded != 0) {
                                                unsigned int* scan;
                                                unsigned int remaining;

                                                limit = 0;
                                                scan = reinterpret_cast<unsigned int*>(
                                                    static_cast<unsigned char*>(m_owned_0d8) + 0xc);
                                                remaining =
                                                    ReadHeader<unsigned long>(header, 0x66) + 1;
                                                do {
                                                    if (limit < *scan) {
                                                        limit = *scan;
                                                    }
                                                    scan += 4;
                                                    --remaining;
                                                } while (remaining != 0);
                                                ++limit;
                                                if (limit > 9999) {
                                                    srAssertFail("(i2 < 10000)", OCTREE_CPP,
                                                                 0x179, 0);
                                                }
                                                g_octree_storage_00659770 =
                                                    reinterpret_cast<unsigned long>(
                                                        malloc(limit * 4));
                                                if (g_octree_storage_00659770 == 0) {
                                                    fLoaded = 0;
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't allocate polygon "
                                                           "index list for regions.");
                                                } else {
                                                    for (index = 0; index < limit; ++index) {
                                                        reinterpret_cast<unsigned int*>(
                                                            g_octree_storage_00659770)[index] =
                                                            index;
                                                    }
                                                }
                                            }
                                        }
                                        if (m_positional_1b4 != 0) {
                                            m_pAlphaBits = new BitArray(m_positional_1b4);
                                            if (m_pAlphaBits == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits", OCTREE_CPP, 0x18a,
                                                    "ReadOctFile: Failure allocating Alpha Bits.");
                                            }
                                            if (BitArrayLoad0043AEC0(m_pAlphaBits, hOctFile) == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits->Load(hOctFile)", OCTREE_CPP,
                                                    0x18b,
                                                    "ReadOctFile: Failure reading Alpha Bits.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumParticles != 0) {
                                            m_pusMeshParticleLookup =
                                                static_cast<unsigned short*>(
                                                    malloc(m_positional_1b4 * 2 + 2));
                                            if (m_pusMeshParticleLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticleLookup", OCTREE_CPP, 0x191,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshParticleLookup,
                                                m_positional_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshParticles = static_cast<unsigned short*>(
                                                malloc(m_positional_0e8 * 2));
                                            if (m_pusMeshParticles == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticles", OCTREE_CPP, 0x198,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Link Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshParticles,
                                                m_positional_0e8 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumProps != 0) {
                                            m_pusMeshPropLookup = static_cast<unsigned short*>(
                                                malloc(m_positional_1b4 * 2 + 2));
                                            if (m_pusMeshPropLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshPropLookup", OCTREE_CPP, 0x1a1,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshPropLookup,
                                                m_positional_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshProps = static_cast<unsigned short*>(
                                                malloc(m_positional_0f4 * 2));
                                            if (m_pusMeshProps == 0) {
                                                srAssertFail(
                                                    "m_pusMeshProps", OCTREE_CPP, 0x1a8,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop Link "
                                                    "Table.");
                                            }
                                            fLoaded = ReadVirtualFile(
                                                hOctFile, m_pusMeshProps,
                                                m_positional_0f4 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                    }
                                    fSuccess = 0;
                                    if (fLoaded != 0) {
                                        fLoaded = ReadVirtualFile(
                                            hOctFile, &uiTerminator, 4, &uiRead);
                                        if (fLoaded == 0 || uiTerminator != -1) {
                                            srAssertFail(
                                                "fSuccess && (uiTerminator==0xffffffff)",
                                                OCTREE_CPP, 0x1b2,
                                                "ReadOctFile: Mesh, Prop, and Particle data longer "
                                                "than expected.");
                                        }
                                        fSuccess = 0;
                                        if (fLoaded != 0) {
                                            if (ReadHeader<unsigned long>(header, 0x7e) != 0) {
                                                pathing_180 = new W8Pathing00457CF0();
                                                if (pathing_180 == 0) {
                                                    goto finish;
                                                }
                                                pathing_180->Configure00458A50(
                                                    ReadHeader<unsigned long>(header, 0x7e),
                                                    ReadHeader<unsigned long>(header, 0xac),
                                                    ReadHeader<unsigned long>(header, 0xb4),
                                                    header + 0x0e,
                                                    static_cast<char*>(m_owned_0c0));
                                                fLoaded = pathing_180->Load00458CE0(hOctFile);
                                            }
                                            fSuccess = 0;
                                            if (fLoaded != 0) {
                                                if (m_ulNumProps != 0 &&
                                                    ReadHeader<char>(header, 0xb8) != 0) {
                                                    m_pPropSunBits = new BitArray(m_ulNumProps);
                                                    if (m_pPropSunBits == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits", OCTREE_CPP, 0x1c5,
                                                            "ReadOctFile: Couldn't allocate Prop "
                                                            "Sun Bits.");
                                                    }
                                                    if (BitArrayLoad0043AEC0(
                                                            m_pPropSunBits, hOctFile) == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits->Load(hOctFile)",
                                                            OCTREE_CPP, 0x1c6,
                                                            "ReadOctFile: Failure reading Prop Sun "
                                                            "Bits.");
                                                    }
                                                }
                                                fSuccess = ReadVirtualFile(
                                                    hOctFile, &uiTerminator, 4, &uiRead);
                                                if (fSuccess == 0) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't read octree file "
                                                           "terminator.");
                                                }
                                                if (uiTerminator != -1) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Octree file longer than "
                                                           "expected.");
                                                    fSuccess = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

finish:
    g_octree_game_data_00652db0 = 0;
    *game_data = 0;
    if (fSuccess != 0 && ReadHeader<unsigned long>(header, 0x86) != 0) {
        pGameData = CreateGameData00449010(hOctFile, 0);
        if (pGameData == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate submesh array.");
            fSuccess = 0;
        } else {
            fSuccess = ReadVirtualFile(hOctFile, &uiTerminator, 4, &uiRead);
            if (fSuccess == 0 || uiTerminator != -1) {
                srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0x1e2,
                             "ReadOctFile: GameData portion of Oct file longer than expected.");
            }
        }
    }
    CloseVirtualFile(hOctFile);
    if (fSuccess != 0) {
        g_octree_6598a4 = this;
        *static_cast<W8Octree**>(pGameData) = this;
        *game_data = pGameData;
        g_octree_game_data_00652db0 = pGameData;
        m_positional_169 = ReadLevelName00432E90(static_cast<char*>(m_owned_0c0));
        ApplyLevelName00432B80(static_cast<char*>(m_owned_0c0));
        if (pathing_180 != 0) {
            ReadWaypointFile0043A0F0();
        }
        return;
    }
failed:
    g_octree_6598a4 = 0;
    m_flags_000 |= 0x80000000;
}

/* The raw 0x29c allocation is reset before ReadOctFile applies its header.
   The image spells the clears individually; memset expresses the same POD
   construction invariant while the positional fields are still unnamed. */
// FUNCTION: WIZ8 0x0042d040
void W8Octree::Reset()
{
    g_octree_storage_00659770 = 0;
    g_octree_state_00659890 = 0;
    Function46CDC0();
    memset(this, 0, sizeof(*this));
    current_sector = -1;
}

/* ReadOctFile calls this after Reset. The file header is packed and several
   values are intentionally unaligned, so each read is copied rather than
   reached through an incorrectly aligned C++ record. Positional writes retain
   the proven offsets until the corresponding octree concepts are named. */
// FUNCTION: WIZ8 0x0042d2a0
void W8Octree::Initialize(const void* raw_header)
{
    const unsigned char* header = static_cast<const unsigned char*>(raw_header);
    unsigned int axis;

    if (header != 0) {
        WriteMember(this, 0x04, ReadHeader<unsigned long>(header, 0x02));
        WriteMember(this, 0x08, ReadHeader<unsigned long>(header, 0x06));
        WriteMember(this, 0x70, ReadHeader<unsigned long>(header, 0x0a));
        for (axis = 0; axis < 3; ++axis) {
            WriteMember(this, 0x27c + axis * 4, 0UL);
            WriteMember(this, 0x288 + axis * 4, 0UL);
            WriteMember(this, 0x0c + axis * 4,
                        ReadHeader<unsigned long>(header, 0x0e + axis * 4));
            WriteMember(this, 0x18 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x1a + axis * 4));
            WriteMember(this, 0x24 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x26 + axis * 4));
            WriteMember(this, 0x30 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x32 + axis * 4));
            WriteMember(this, 0x78 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x3e + axis * 4));
            WriteMember(this, 0x84 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x4a + axis * 4));
            WriteMember(this, 0xa4 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x56 + axis * 4));
        }

        WriteMember(this, 0x64,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xa8) *
                        ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x68,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x44, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x58, ReadHeader<unsigned short>(header, 0x64));
        WriteMember(this, 0x46, ReadHeader<unsigned short>(header, 0x60));
        WriteMember(this, 0x52, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x74, ReadHeader<unsigned long>(header, 0x66));
        m_positional_1a8 = ReadHeader<unsigned long>(header, 0x9a);
        m_positional_1ac = ReadHeader<unsigned long>(header, 0xa2);
        m_positional_1b4 = ReadHeader<unsigned long>(header, 0x9e);
        WriteMember(this, 0xb4, ReadHeader<unsigned long>(header, 0x6a));
        WriteMember(this, 0xb8, ReadHeader<unsigned long>(header, 0x6e));
        WriteMember(this, 0x3c, ReadHeader<unsigned long>(header, 0x72));
        WriteMember(this, 0xc8, ReadHeader<unsigned long>(header, 0x76));
        WriteMember(this, 0x40, ReadHeader<unsigned long>(header, 0x7a));
        m_positional_0cc = ReadHeader<unsigned long>(header, 0x82);
        m_positional_124 = ReadHeader<unsigned long>(header, 0x86);
        m_positional_128 = ReadHeader<unsigned long>(header, 0x8a);
        m_positional_138 = ReadHeader<unsigned long>(header, 0x92);
        WriteMember(this, 0x54, ReadHeader<unsigned long>(header, 0xa6));
        m_positional_178 = ReadHeader<unsigned long>(header, 0xac);
        m_positional_17c = ReadHeader<unsigned long>(header, 0xb4);
        WriteMember(this, 0x60, ReadHeader<unsigned long>(header, 0xb9));
        m_ulNumProps = ReadHeader<unsigned long>(header, 0xbd);
        m_positional_0e8 = ReadHeader<unsigned short>(header, 0xc5);
        m_positional_0f4 = ReadHeader<unsigned short>(header, 0xc7);
        m_ulNumParticles = ReadHeader<unsigned long>(header, 0xc1);

        if (m_ulNumParticles != 0) {
            m_owned_0fc = new BitArray(m_ulNumParticles);
            m_owned_100 = new BitArray(m_ulNumParticles);
            m_owned_10c = new BitArray(m_ulNumParticles);
            m_papParticles = static_cast<void**>(malloc(m_ulNumParticles * 4 + 8));
        }
        if (m_ulNumProps != 0) {
            m_owned_104 = new BitArray(m_ulNumProps);
            m_owned_108 = new BitArray(m_ulNumProps);
            m_owned_110 = new BitArray(m_ulNumProps);
            m_papProps = static_cast<void**>(malloc(m_ulNumProps * 4 + 8));
        }

        m_owned_190 = new BitArray(ReadHeader<unsigned long>(header, 0x72));
        m_owned_154 = new BitArray(ReadHeader<unsigned short>(header, 0x64) + 1);
        m_owned_15c = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_160 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_164 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_194 = new BitArray(
            ReadHeader<unsigned long>(header, 0x7a) < 5000
                ? 5000
                : ReadHeader<unsigned long>(header, 0x7a));
        m_owned_198 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_19c = new BitArray(ReadHeader<unsigned long>(header, 0x72));

        W8OctreeIndex** pair = new W8OctreeIndex*[2];
        pair[0] = CreateIndex();
        pair[1] = CreateIndex();
        octree_queue_0bc = reinterpret_cast<W8OctreeQueue00437000*>(pair);
        m_owned_150 = CreateIndex();

        unsigned int visited_size = ReadHeader<unsigned short>(header, 0x64) + 1;
        m_pfRegsVisited = static_cast<unsigned char*>(malloc(visited_size));
        if (m_pfRegsVisited == 0) {
            srAssertFail("m_pfRegsVisited",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                         0x36a, "InitOctree: Couldn't allocate m_pfRegsVisited.");
        }
        memset(m_pfRegsVisited, 0, visited_size);
        m_positional_168 = 1;
    }

    m_aulGDObjs = static_cast<unsigned long*>(malloc(40000));
    if (m_aulGDObjs == 0) {
        srAssertFail("m_aulGDObjs",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                     0x372, "InitOctree: Couldn't allocate m_aulGDObjs.");
    }
    WriteMember(this, 0x6c, static_cast<unsigned short>(3));
    m_fAccumulating = 1;
    Function434020(0);
}

// FUNCTION: WIZ8 0x0042e440
void W8Octree::AddLoadedProp(void* prop)
{
    if (m_fAccumulating) {
        if (m_usNumPropsLoaded >= (unsigned short)m_ulNumProps) {
            srAssertFail(
                "m_usNumPropsLoaded<(UINT16)m_ulNumProps",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x485,
                "Too many props loaded for Octree");
        }
        m_papProps[m_usNumPropsLoaded] = prop;
        m_usNumPropsLoaded++;
        m_papProps[m_usNumPropsLoaded] = 0;
    }
}

// FUNCTION: WIZ8 0x0042e4c0
void W8Octree::AddLoadedParticle(void* particle)
{
    if (m_fAccumulating) {
        if (m_usNumParticlesLoaded >= (unsigned short)m_ulNumParticles) {
            srAssertFail(
                "m_usNumParticlesLoaded<(UINT16)m_ulNumParticles",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x49d,
                "Too many particles loaded for Octree");
        }
        m_papParticles[m_usNumParticlesLoaded] = particle;
        m_usNumParticlesLoaded++;
        m_papParticles[m_usNumParticlesLoaded] = 0;
    }
}

/* Complete non-deleting teardown. LoadWorld and DestroyWorld both perform the
   matching operator delete only after this method returns. */
// FUNCTION: WIZ8 0x0042de60
W8Octree::~W8Octree()
{
    W8OctreeIndex** pair;

    if (m_positional_16c != 0) {
        Function4331F0(m_owned_0c0);
    }
    if (m_positional_16d != 0) {
        Function432D60(m_owned_0c0);
    }
    if (pathing_180 != 0) {
        Function459400(0);
    }

    free(m_aulGDObjs);
    free(m_owned_09c);
    free(m_owned_0a0);
    free(m_owned_0d0);
    free(m_owned_0d4);
    DestroyBitArray(m_pAlphaBits);
    free(m_owned_0b0);
    free(m_owned_12c);
    free(m_owned_148);
    free(m_owned_130);

    if (m_owned_150 != 0) {
        W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_owned_150);
        operator delete(index->buckets);
        operator delete(index->entries);
        index->buckets = 0;
        index->entries = 0;
        index->last_entry = -1;
        index->bucket_count = 0;
        Function439140();
        DestroyIndex(index);
        m_owned_150 = 0;
    }

    free(m_pfRegsVisited);
    DestroyBitArray(m_owned_190);
    DestroyBitArray(m_owned_154);
    DestroyBitArray(m_owned_15c);
    DestroyBitArray(m_owned_160);
    DestroyBitArray(m_owned_164);

    free(m_pusMeshParticleLookup);
    m_pusMeshParticleLookup = 0;
    free(m_pusMeshParticles);
    m_pusMeshParticles = 0;
    free(m_pusMeshPropLookup);
    m_pusMeshPropLookup = 0;
    free(m_pusMeshProps);
    m_pusMeshProps = 0;
    free(m_papProps);
    free(m_papParticles);

    DestroyBitArray(m_owned_0fc);
    DestroyBitArray(m_owned_100);
    DestroyBitArray(m_owned_104);
    DestroyBitArray(m_owned_108);
    DestroyBitArray(m_owned_10c);
    DestroyBitArray(m_owned_110);

    m_positional_170 = 0;
    if (m_sr_owned_174 != 0) {
        srHeap.free(m_sr_owned_174);
    }
    free(m_owned_0c0);
    DestroyBitArray(m_owned_194);
    DestroyBitArray(m_owned_198);
    DestroyBitArray(m_owned_19c);
    DestroyBitArray(m_owned_1a0);
    DestroyBitArray(m_owned_1a4);

    pair = reinterpret_cast<W8OctreeIndex**>(octree_queue_0bc);
    if (pair != 0) {
        DestroyIndex(pair[0]);
        DestroyIndex(pair[1]);
        operator delete(pair);
    }

    free(m_owned_0d8);
    if (pathing_180 != 0) {
        Function457B10(pathing_180);
        operator delete(pathing_180);
        pathing_180 = 0;
    }
    DestroyBitArray(m_pPropSunBits);
    Function46CDD0();
}

/* The node whose 0x1c is non-null is the only kind worth attaching. */
extern int g_shared_mark_006598ac;

extern void OctreeTraverse(
    void* walker, void* arg_2, void* arg_3, int kind, unsigned int limit);   /* 0x0042F280 */

/* Attach a visited set to the walker, but only one that has been built. */
// FUNCTION: WIZ8 0x0042e3e0
void W8Octree::SetVisitedSet0042E3E0(BitArray* visited)
{
    if (visited->puiIndex != 0) {
        m_pPropSunBits = visited;
    }
}

/* Advance or reset the walk's mark. A negative offset takes the shared
   counter's value as this walk's base; anything else advances the shared
   counter and reports whether that mark has already been visited - which with
   no set attached is always no. */
// FUNCTION: WIZ8 0x0042e400
int W8Octree::MarkVisited0042E400(int offset)
{
    if (offset < 0) {
        mark_base_184 = g_shared_mark_006598ac;
        return 0;
    }
    g_shared_mark_006598ac = mark_base_184 + offset;
    if (m_pPropSunBits != 0 && m_pPropSunBits->Test(g_shared_mark_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8Octree::VisitPointCopy0042E620(
    unsigned short location_id, srVector3T<float>* position)
{
    W8Position copy;

    copy.x = position->x;
    copy.y = position->y;
    copy.z = position->z;
    UpdateMonsterLocation0042E540(location_id, &copy);
}

/* Start a traversal of the twelfth kind. A limit of zero means no limit, which
   is what the -1 stands for. */
// FUNCTION: WIZ8 0x0042ef00
void OctreeTraverseKind12(void* walker, void* arg_2, void* arg_3, unsigned short limit)
{
    unsigned int bound = (unsigned int)-1;

    if (limit != 0) {
        bound = limit;
    }
    OctreeTraverse(walker, arg_2, arg_3, 0xc, bound);
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void W8Octree::QueueOctreeKind130042E810(
    int id,
    const srVector3T<float>* position)
{
    int point[3];

    point[0] = (int)((position->x - octree_origin_00c.x) /
                     octree_cell_size_070);
    point[1] = (int)((position->y - octree_origin_00c.y) /
                     octree_cell_size_070);
    point[2] = (int)((position->z - octree_origin_00c.z) /
                     octree_cell_size_070);
    octree_queue_0bc->Queue00437000(0xd, id + 1, point);
}

/* Step one navigator's movement tail towards its target.

   With a pathing service present the work belongs to that service, and which of
   its two entry points runs is the link mode's decision. Without one this walks
   the target itself. The direction is the full vector to the target with its
   height component dropped, so the step stays in the horizontal plane; its
   length is clamped to the distance remaining, and reaching the target is what
   the return value reports. The new position is mirrored into the attachment's
   own three copies. */
// FUNCTION: WIZ8 0x00434620
unsigned int W8Octree::AdvanceNavigator00434620(
    W8NavigatorMovement004572C0* movement, float radius, float separation)
{
    srVector3T<float> vecDir;
    srVector3T<float> vecPos;
    W8NavigatorAttachment* attachment;
    float distance;
    float step;
    unsigned char reached = 1;

    if (pathing_180 != 0) {
        if (g_navigator_link_mode_00659c10 == 0) {
            return Function004669B0(movement, radius, separation);
        }
        return Function00467150(movement, radius, separation);
    }
    if (g_navigator_link_mode_00659c10 != 0) {
        return 1;
    }
    vecDir.x = movement->target_position_04c.x - movement->position_040.x;
    vecDir.y = movement->target_position_04c.y - movement->position_040.y;
    vecDir.z = movement->target_position_04c.z - movement->position_040.z;
    vecDir.y = 0.0f;
    distance = (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
    step = g_object_6598bc->GetValue28() * movement->movement_scale_060 *
        g_rate_006068EC * g_world_scale_005ebc40;
    if (step >= distance) {
        step = distance;
    } else {
        reached = 0;
    }
    if ((double)(vecDir.x * vecDir.x + vecDir.z * vecDir.z) != g_zero_005ebb40) {
        vecDir.y = 0.0f;
        step = step / (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
        vecDir.x = vecDir.x * step;
        vecDir.z = vecDir.z * step;
    }
    vecPos.x = vecDir.x + movement->position_040.x;
    vecPos.y = vecDir.y + movement->position_040.y;
    vecPos.z = vecDir.z + movement->position_040.z;
    movement->position_040 = vecPos;
    attachment = movement->attachment_0ac;
    *attachment->position_4c = vecPos;
    attachment->position_34 = *attachment->position_4c;
    attachment->position_10 = *attachment->position_4c;
    return reached;
}

/* Settle both ends of a portal transition onto the ground before the move is
   applied. Each end is capped at the octree's height ceiling first, and its
   settled height is only taken when the drop actually hit something. The pair
   is then handed on together, which is why one body carries both. */
// FUNCTION: WIZ8 0x00434a30
void W8Octree::AdjustPortalDestination00434A30(
    W8Position* destination, const W8Position* source)
{
    W8Position local_destination;
    W8Position local_source;
    W8Position probe;
    unsigned char hit;

    if (pathing_180 == 0) {
        return;
    }
    if (pathing_180->flag_1c8 == 0 && pathing_180->value_00c != 0) {
        return;
    }
    local_destination = *destination;
    local_source = *source;

    hit = 0;
    if (local_destination.y > height_limit_034) {
        local_destination.y = height_limit_034;
    }
    probe = local_destination;
    SettleToGround00433820(&probe.x, &hit, 1, 500.0f);
    if (hit != 0) {
        local_destination.y = probe.y;
    }

    hit = 0;
    if (local_source.y > height_limit_034) {
        local_source.y = height_limit_034;
    }
    probe = local_source;
    SettleToGround00433820(&probe.x, &hit, 1, 500.0f);
    if (hit != 0) {
        local_source.y = probe.y;
    }
    ApplyPortalTransition0045F2D0(&local_destination, &local_source);
}

/* Engine Code\Trigger.cpp lives in the same interval. Its list of live
   triggers is torn down wholesale rather than one at a time. */

/* 0x006598FC and 0x00659904: the live triggers, count and data. */
extern int g_trigger_count;
extern void** g_trigger_data;
extern int g_trigger_flag_00659994;
extern int g_trigger_flag_006598e4;

/* Release every live trigger and forget the three pieces of state that go with
   the list, which is what groups them. */
// FUNCTION: WIZ8 0x004445b0
void ReleaseAllTriggers(void)
{
    int index;

    for (index = 0; index < g_trigger_count; ++index) {
        operator delete(g_trigger_data[index]);
    }
    g_trigger_flag_00659994 = 0;
    g_trigger_count = 0;
    g_trigger_flag_006598e4 = 0;
}
