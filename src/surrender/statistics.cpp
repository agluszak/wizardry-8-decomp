#include <string.h>

#include "surrender/srStatisticsManager.h"
#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srHeap.h"
#include "surrender/srPixelConvert.h"
#include "surrender/srTimer.h"

// FUNCTION: SURRENDER 0x100148A0
void srStatisticsManager::reset()
{
    memset(&statistics_00, 0, sizeof(Statistics));
    statistics_00.elapsed_time_00 = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x10014920
void srStatisticsManager::getStatistics(Statistics& statistics) const
{
    double now = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
    statistics = statistics_00;
    statistics.elapsed_time_00 = now - statistics.elapsed_time_00;
}

// FUNCTION: SURRENDER 0x10014950
void srStatisticsManager::dump(std::ostream& stream, const Statistics& statistics)
{
    stream << '\n';
    stream << "Pipeline statistics/second:" << '\n';
    stream << "Time since reset:               " << statistics.elapsed_time_00 << '\n' << '\n';
    if (statistics.elapsed_time_00 > 0.01) {
        stream << "Meshes traversed:               "
               << statistics.meshes_traversed_08 / statistics.elapsed_time_00 << '\n';
        stream << "Meshes submitted:               "
               << statistics.meshes_submitted_0c / statistics.elapsed_time_00 << '\n';
        stream << "Triangles submitted:            "
               << statistics.triangles_submitted_10 / statistics.elapsed_time_00 << '\n';
        stream << "Triangles after culling:        "
               << statistics.triangles_after_culling_14 / statistics.elapsed_time_00
               << '\n';
        if (statistics.triangles_submitted_10 != 0) {
            stream << "Triangle cull ratio:            "
                   << 100.0 - statistics.triangles_after_culling_14 * 100.0 /
                                 statistics.triangles_submitted_10
                   << "%" << '\n';
        }
        stream << "Vertices submitted:             "
               << statistics.vertices_submitted_18 / statistics.elapsed_time_00 << '\n';
        stream << "Vertices after culling:         "
               << statistics.vertices_after_culling_1c / statistics.elapsed_time_00 << '\n';
        if (statistics.vertices_submitted_18 != 0) {
            stream << "Vertex cull ratio:              "
                   << 100.0 - statistics.vertices_after_culling_1c * 100.0 /
                                 statistics.vertices_submitted_18
                   << "%" << '\n';
        }
        stream << "Material processing stalls:     "
               << statistics.material_processing_stalls_20 / statistics.elapsed_time_00
               << '\n';
        if (statistics.meshes_submitted_0c != 0) {
            stream << "Avg. triangles per mesh:        "
                   << statistics.triangles_submitted_10 /
                          (double)statistics.meshes_submitted_0c
                   << '\n';
            stream << "Avg. vertices per mesh:         "
                   << statistics.vertices_submitted_18 /
                          (double)statistics.meshes_submitted_0c
                   << '\n';
        }
        if (statistics.vertices_after_culling_1c != 0) {
            double inv_vertices = 1.0 / statistics.vertices_after_culling_1c;
            stream << "Avg. diffuse ops per vertex:    "
                   << statistics.diffuse_operations_24 * inv_vertices << '\n';
            stream << "Avg. alpha ops per vertex:      "
                   << statistics.alpha_operations_2c * inv_vertices << '\n';
            stream << "Avg. specular ops per vertex:   "
                   << statistics.specular_operations_28 * inv_vertices << '\n';
            stream << "Avg. fog ops per vertex:        "
                   << statistics.fog_operations_30 * inv_vertices << '\n';
            stream << "Avg. texcoord ops per vertex:   "
                   << statistics.texture_coordinate_operations_34 * inv_vertices << '\n';
        }
    }
    stream << std::endl;
}

/* Per-TU CRT stream-init sentinels: retail emits an ios_base::Init ctor call
   and atexit registrar (0x10014E20/0x10014E30) plus the _Winit pair
   (0x10014E60/0x10014E70) for this unit's <iostream> include. */
// SYNTHETIC: SURRENDER 0x10014E20
// ios_base::Init static-init call

// SYNTHETIC: SURRENDER 0x10014E30
// ios_base::Init atexit registrar

// SYNTHETIC: SURRENDER 0x10014E60
// _Winit static-init call

// SYNTHETIC: SURRENDER 0x10014E70
// _Winit atexit registrar

// FUNCTION: SURRENDER 0x10014FE0
void __cdecl _srLibraryInit(void)
{
    srAssertSetFunc(srDefaultAssertFailFunc);
    initPixelTables();
}

// FUNCTION: SURRENDER 0x10015000
void __cdecl _srLibraryExit(void)
{
    srHeap.freeAll();
}
