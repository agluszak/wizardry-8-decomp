#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"

#include <stdlib.h>
#include <string.h>

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

extern unsigned char g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
extern W8PathingService* g_pathing_00659c60;
extern void ConstructPathState004CCCB0(void* state);
extern void* CreateOctPathIndex();
extern void* g_path_scratch_00659c64;
extern void RegisterPathSurface004B7730(unsigned int index, const int* point);
extern void RegisterPathVertex004B7830(
    unsigned int index, const int* point, const int* second);

#define OCTPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPath.cpp"

/* Read the path graph out of the octree file.

   Two parts. The hash array pairs a key with a value for every node the service
   was sized for and goes straight into the first index. Then a four-dword block
   gives the three conditional-table counts and one loose value, and a graph with
   fewer than two lookup or key entries is treated as having none at all rather
   than allocated. Every conditional table's allocation failure asserts against
   m_pCondPaths rather than against itself, which is the original's own
   shorthand and is reproduced. */
// FUNCTION: WIZ8 0x00458ce0
unsigned char W8PathingService::Load00458CE0(int handle)
{
    char acMessage[256];
    unsigned int block[4];
    unsigned int uiRead;
    unsigned int* buffer;
    unsigned int* scan;
    unsigned char fSuccess = 0;
    unsigned int index;

    if (size_004 != 0) {
        m_pIndex_064 = CreateOctPathIndex();
        m_pIndex_074 = CreateOctPathIndex();
        buffer = static_cast<unsigned int*>(malloc(size_004 * 8));
        if (m_pIndex_064 == 0 || buffer == 0) {
            strcpy(acMessage, "ReadPathNodes: Couldn't allocate path hash array.");
        } else {
            fSuccess = ReadVirtualFile(handle, buffer, size_004 * 8, &uiRead);
            if (fSuccess == 0) {
                strcpy(acMessage, "ReadPathNodes: Couldn't read path hash array.");
                free(buffer);
            } else {
                scan = buffer;
                for (index = 0; index < (unsigned int)size_004; ++index) {
                    InsertEntry0055DBB0(
                        static_cast<W8OctreeIndex*>(m_pIndex_064),
                        &scan[0], reinterpret_cast<const int*>(&scan[1]));
                    scan += 2;
                }
                free(buffer);
            }
        }
    }
    fSuccess = ReadVirtualFile(handle, block, 0x10, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8fa,
                     "ReadPathNodes: Couldn't write Conditional Counts.\n");
    }
    m_ulNumCondPaths = block[0];
    m_ulNumCondLookup = block[1];
    m_ulNumCondKeys = block[2];
    m_positional_000 = block[3];
    if (m_ulNumCondLookup < 2 || m_ulNumCondKeys < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondLookup = 0;
        m_ulNumCondKeys = 0;
        return fSuccess;
    }
    m_pCondPaths = static_cast<unsigned char*>(malloc(block[0] * 0x44));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x903,
                     "ReadPathNodes: Couldn't allocate Conditional Prop array.\n");
    }
    m_pCondLookup = static_cast<int*>(malloc(m_ulNumCondLookup << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x905,
                     "ReadPathNodes: Couldn't allocate Conditional Lookup array.\n");
    }
    m_pCondFrames = static_cast<short*>(malloc(m_ulNumCondLookup << 1));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x907,
                     "ReadPathNodes: Couldn't allocate Conditional Frame array.\n");
    }
    m_pCondKeys = static_cast<int*>(malloc(m_ulNumCondKeys << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x909,
                     "ReadPathNodes: Couldn't allocate Conditional Key array.\n");
    }
    m_pCondValues = static_cast<int*>(malloc(m_ulNumCondKeys << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x90b,
                     "ReadPathNodes: Couldn't allocate Conditional Value array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondPaths, m_ulNumCondPaths * 0x44, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x90e,
                     "ReadPathNodes: Couldn't write Conditional Prop array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondLookup, m_ulNumCondLookup << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x910,
                     "ReadPathNodes: Couldn't write Conditional Lookup array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondFrames, m_ulNumCondLookup << 1, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x912,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondKeys, m_ulNumCondKeys << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x914,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondValues, m_ulNumCondKeys << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x916,
                     "ReadPathNodes: Couldn't write Conditional Value array.\n");
    }
    return fSuccess;
}

/* Offer every flagged surface to the path builder.

   Surfaces are 0x28 bytes apart and the walk starts at index one, so entry zero
   is never a real surface. The point handed on is the surface's own position
   converted to the graph's integer grid - x from the bounds floor, z from the
   third bound - and the two conversions happen in the order the point's fields
   do not. */
// FUNCTION: WIZ8 0x00460020
void W8PathingService::LinkSurfaces00460020()
{
    unsigned int index = 1;
    int offset = 0x28;
    int point[2];
    unsigned char* surface;
    int converted;

    if (m_ulNumSurfaces <= index) {
        return;
    }
    do {
        surface = m_pSurfaces_048 + offset;
        if ((*surface & 0x40) != 0) {
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            point[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                             grid_scale_01c);
            point[1] = converted;
            RegisterPathSurface004B7730(index, point);
        }
        offset += 0x28;
        ++index;
    } while (index < m_ulNumSurfaces);
}

/* The same for the edges, which are 0xe bytes apart, gated by a different flag,
   and which name two surfaces by index in their shorts at +4 and +6. Each of
   those surfaces contributes one converted point, so the builder receives the
   edge as a pair. */
// FUNCTION: WIZ8 0x004600b0
void W8PathingService::LinkEdges004600B0()
{
    unsigned int index = 1;
    int offset = 0xe;
    int first[2];
    int second[2];
    unsigned char* edge;
    unsigned char* surface;
    unsigned int surface_index;
    int converted;

    if (m_ulNumEdges <= index) {
        return;
    }
    do {
        edge = m_pEdges_04c + offset;
        if ((*reinterpret_cast<unsigned int*>(edge) & 0x20000000) != 0) {
            surface_index = *reinterpret_cast<unsigned short*>(edge + 4);
            surface = m_pSurfaces_048 + surface_index * 0x28;
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            first[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                             grid_scale_01c);
            first[1] = converted;

            surface_index = *reinterpret_cast<unsigned short*>(edge + 6);
            surface = m_pSurfaces_048 + surface_index * 0x28;
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            second[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                              grid_scale_01c);
            second[1] = converted;
            RegisterPathVertex004B7830(index, first, second);
        }
        offset += 0xe;
        ++index;
    } while (index < m_ulNumEdges);
}

/* Give everything the service owns back.

   Not a destructor: nothing restores a vtable and the object is left holding
   dangling pointers, which is the same shape BitArray::FreeIndex has. The four
   malloc'd tables and the conditional path tables go back through free, the bit
   sets and the two hash indexes through their own teardown, and the global slot
   the constructor claimed is cleared last. */
// FUNCTION: WIZ8 0x00457b10
void W8PathingService::Release00457B10()
{
    void** index;

    if (m_owned_044 != 0) {
        free(m_owned_044);
    }
    if (m_pSurfaces_048 != 0) {
        free(m_pSurfaces_048);
    }
    if (m_pEdges_04c != 0) {
        free(m_pEdges_04c);
    }
    if (m_owned_050 != 0) {
        free(m_owned_050);
    }
    if (m_owned_054 != 0) {
        delete m_owned_054;
    }
    if (m_owned_058 != 0) {
        m_owned_058->FreeIndex();
        ::operator delete(m_owned_058);
    }
    if (m_owned_05c != 0) {
        m_owned_05c->FreeIndex();
        ::operator delete(m_owned_05c);
    }
    if (m_owned_060 != 0) {
        m_owned_060->FreeIndex();
        ::operator delete(m_owned_060);
    }
    index = static_cast<void**>(m_pIndex_064);
    if (index != 0) {
        if (index[0] != 0) {
            ::operator delete(index[0]);
        }
        if (index[1] != 0) {
            ::operator delete(index[1]);
        }
        ::operator delete(index);
    }
    index = static_cast<void**>(m_pIndex_074);
    if (index != 0) {
        if (index[0] != 0) {
            ::operator delete(index[0]);
        }
        if (index[1] != 0) {
            ::operator delete(index[1]);
        }
        ::operator delete(index);
    }
    index = static_cast<void**>(m_owned_06c);
    if (index != 0) {
        void** held = static_cast<void**>(index[0]);

        if (held != 0) {
            if (held[1] == 0) {
                ::operator delete(held[0]);
            }
            ::operator delete(held);
        }
        ::operator delete(index);
    }
    if (m_owned_0c8 != 0) {
        ::operator delete(m_owned_0c8);
    }
    if (m_owned_214 != 0) {
        ::operator delete(m_owned_214);
    }
    if (g_path_scratch_00659c64 != 0) {
        free(g_path_scratch_00659c64);
    }
    g_path_scratch_00659c64 = 0;
    if (m_pCondPaths != 0) {
        free(m_pCondPaths);
    }
    if (m_pCondLookup != 0) {
        free(m_pCondLookup);
    }
    if (m_pCondFrames != 0) {
        free(m_pCondFrames);
    }
    if (m_pCondKeys != 0) {
        free(m_pCondKeys);
    }
    if (m_pCondValues != 0) {
        free(m_pCondValues);
    }
    g_pathing_00659c60 = 0;
}

/* Build the pathing service.

   Everything starts cleared except three hundred-bit sets, a reserve table
   sized from the shared bound, and one state object. The service registers
   itself in the global slot as it is built, which is what lets the rest of the
   engine reach it without the octree handing it over. */
// FUNCTION: WIZ8 0x004578e0
W8PathingService::W8PathingService()
{
    int index;

    grid_scale_01c = 0;
    span_020 = 0;
    for (index = 0; index < 6; ++index) {
        level_bounds[index] = 0;
    }
    m_owned_044 = 0;
    size_004 = 0;
    m_positional_008 = 0;
    m_ulNumSurfaces = 0;
    m_ulNumEdges = 0;
    m_positional_014 = 0;
    m_positional_018 = 0;
    m_pSurfaces_048 = 0;
    m_pEdges_04c = 0;
    m_owned_050 = 0;
    m_owned_054 = 0;
    m_owned_058 = new BitArray(100);
    m_owned_05c = new BitArray(100);
    m_owned_060 = new BitArray(100);
    m_pIndex_064 = 0;
    m_pIndex_074 = 0;
    level_name = 0;
    flag_08c = 0;
    m_owned_06c = 0;
    m_positional_070 = 0x501502f9;
    flag_1c8 = 0;
    flag_1c9 = 0;
    flag_1ca = 0;
    flag_1cb = 0;
    flag_1cc = 0;
    value_1ce = 4;
    m_positional_1d0 = 0;
    value_1d4 = 0;
    value_1d6 = 0;
    value_1d8 = 0;
    m_owned_0c8 = ::operator new((g_path_reserve_0060827a + 0x14) * 0x2c);
    m_positional_0cc = 0;
    m_positional_0d0 = 0;
    flag_09c = 0;
    flag_0a4 = 0;
    m_positional_0c0 = 0;
    m_positional_0a8 = 0;
    m_positional_0ac = 0;
    m_positional_0b0 = 0;
    m_positional_0b4 = 0;
    m_positional_0b8 = 0;
    m_positional_0bc = 0;
    m_positional_0c4 = 0;
    m_owned_214 = new W8PathState004CAE40();
    m_positional_218 = 0;
    m_pCondPaths = 0;
    m_ulNumCondPaths = 0;
    m_ulNumCondLookup = 0;
    m_ulNumCondKeys = 0;
    m_pCondLookup = 0;
    m_pCondFrames = 0;
    m_pCondKeys = 0;
    m_pCondValues = 0;
    g_pathing_00659c60 = this;
    g_path_limit_006081e8 = 500.0f;
}

/* Take the octree's own bounds and level name. The span is the vertical extent
   of that box scaled, and the cell count is that span plus one. */
// FUNCTION: WIZ8 0x00458a50
void W8PathingService::ConfigureForLevel(
    int size, float grid_scale, int value_28, const float* bounds, const char* name)
{
    size_004 = size;
    grid_scale_01c = grid_scale;
    value_028 = value_28;
    level_bounds[0] = bounds[0];
    level_bounds[1] = bounds[1];
    level_bounds[2] = bounds[2];
    level_bounds[3] = bounds[3];
    level_bounds[4] = bounds[4];
    level_bounds[5] = bounds[5];
    span_020 = (level_bounds[4] - level_bounds[1]) * g_path_span_scale_005ec344;
    cell_count_024 = (short)(int)span_020 + 1;
    level_name = name;
}

/* Look one named path up, and report the region and height range it spans.

   The table holds fixed 0x44-byte entries whose name is the entry itself and
   whose handle sits at +0x40. A match walks the path's two chained tables to
   take the extent of every node it touches: the region ids straight out of the
   low and high halves of each entry, and the height from the entry's low half
   scaled by the service's own span and lifted by the bounds floor. */
// FUNCTION: WIZ8 0x00457cf0
unsigned int W8PathingService::FindPathHandle(
    const unsigned char* path_name,
    unsigned short* path_bounds,
    float* path_range)
{
    const unsigned char* entry;
    unsigned int index;
    unsigned short value;
    float height;
    int outer;
    int inner;
    int outer_table;
    int inner_table;

    if (path_name == 0 || *path_name == 0 || m_pCondPaths == 0 || m_ulNumCondPaths == 0) {
        return 0;
    }
    index = 0;
    entry = m_pCondPaths;
    do {
        if (strcmp(reinterpret_cast<const char*>(path_name),
                   reinterpret_cast<const char*>(entry)) == 0) {
            path_bounds[2] = 0xffff;
            path_bounds[0] = 0xffff;
            path_bounds[3] = 0;
            path_bounds[1] = 0;
            path_range[0] = 1e+08f;
            path_range[2] = -1e+08f;
            outer = *reinterpret_cast<int*>(m_pCondPaths + index * 0x44 + 0x40) * 4;
            outer_table = reinterpret_cast<int>(m_pCondLookup);
            for (inner = *reinterpret_cast<int*>(outer_table + outer); inner != 0;
                 inner = *reinterpret_cast<int*>(inner + outer_table)) {
                inner = *reinterpret_cast<int*>(outer + outer_table) * 4;
                inner_table = reinterpret_cast<int>(m_pCondKeys);
                for (int node = *reinterpret_cast<int*>(inner_table + inner); node != 0;
                     node = *reinterpret_cast<int*>(node + inner_table)) {
                    value = *reinterpret_cast<unsigned short*>(inner + inner_table);
                    if (value < path_bounds[0]) {
                        path_bounds[0] = value;
                    }
                    if (path_bounds[1] < value) {
                        path_bounds[1] = value;
                    }
                    value = (unsigned short)
                        (*reinterpret_cast<unsigned int*>(inner + inner_table) >> 0x10);
                    if (value < path_bounds[2]) {
                        path_bounds[2] = value;
                    }
                    if (path_bounds[3] < value) {
                        path_bounds[3] = value;
                    }
                    height = (float)(*reinterpret_cast<unsigned int*>(
                                         inner + reinterpret_cast<int>(m_pCondValues)) & 0xffff) *
                            span_020 + level_bounds[1];
                    if (height < path_range[0]) {
                        path_range[0] = height;
                    }
                    if (path_range[2] < height) {
                        path_range[2] = height;
                    }
                    inner_table = reinterpret_cast<int>(m_pCondKeys);
                    inner += 4;
                }
                outer_table = reinterpret_cast<int>(m_pCondLookup);
                outer += 4;
            }
            return *reinterpret_cast<unsigned int*>(m_pCondPaths + index * 0x44 + 0x40);
        }
        ++index;
        entry += 0x44;
    } while (index < (unsigned int)m_ulNumCondPaths);
    return 0;
}


/* The state object's whole constructor: it defers to the one at 0x004CCCB0 and
   adds nothing of its own, which is why it has no members to clear here. */
// FUNCTION: WIZ8 0x004cae40
W8PathState004CAE40::W8PathState004CAE40()
{
    ConstructPathState004CCCB0(this);
}
