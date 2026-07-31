#include "wiz8/engine_code/Octree.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"

#include <string.h>

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

extern unsigned char g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
extern W8Pathing00457CF0* g_pathing_00659c60;
extern void* CreatePathState004CAE40(void);

/* Build the pathing service.

   Everything starts cleared except three hundred-bit sets, a reserve table
   sized from the shared bound, and one state object. The service registers
   itself in the global slot as it is built, which is what lets the rest of the
   engine reach it without the octree handing it over. */
// FUNCTION: WIZ8 0x004578e0
W8Pathing00457CF0::W8Pathing00457CF0()
{
    int index;

    value_01c = 0;
    span_020 = 0;
    for (index = 0; index < 6; ++index) {
        bounds_02c[index] = 0;
    }
    m_positional_044 = 0;
    size_004 = 0;
    m_positional_008 = 0;
    value_00c = 0;
    m_positional_010 = 0;
    m_positional_014 = 0;
    m_positional_018 = 0;
    m_positional_048 = 0;
    m_positional_04c = 0;
    m_positional_050 = 0;
    m_positional_054 = 0;
    m_owned_058 = new BitArray(100);
    m_owned_05c = new BitArray(100);
    m_owned_060 = new BitArray(100);
    m_positional_064 = 0;
    m_positional_074 = 0;
    name_068 = 0;
    flag_08c = 0;
    m_positional_06c = 0;
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
    m_owned_214 = CreatePathState004CAE40();
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
void W8Pathing00457CF0::Configure00458A50(
    int size, int value_1c, int value_28, const float* bounds, const char* name)
{
    size_004 = size;
    value_01c = value_1c;
    value_028 = value_28;
    bounds_02c[0] = bounds[0];
    bounds_02c[1] = bounds[1];
    bounds_02c[2] = bounds[2];
    bounds_02c[3] = bounds[3];
    bounds_02c[4] = bounds[4];
    bounds_02c[5] = bounds[5];
    span_020 = (bounds_02c[4] - bounds_02c[1]) * g_path_span_scale_005ec344;
    cell_count_024 = (short)(int)span_020 + 1;
    name_068 = name;
}

/* Look one named path up, and report the region and height range it spans.

   The table holds fixed 0x44-byte entries whose name is the entry itself and
   whose handle sits at +0x40. A match walks the path's two chained tables to
   take the extent of every node it touches: the region ids straight out of the
   low and high halves of each entry, and the height from the entry's low half
   scaled by the service's own span and lifted by the bounds floor. */
// FUNCTION: WIZ8 0x00457cf0
unsigned int W8Pathing00457CF0::FindPathHandle(
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
                            span_020 + bounds_02c[1];
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

