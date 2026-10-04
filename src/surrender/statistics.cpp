#include <ostream>
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
    memset(&statistics, 0, sizeof(Statistics));
    statistics.elapsed_time = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x10014920
void srStatisticsManager::getStatistics(Statistics& statistics) const
{
    double now = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
    statistics = this->statistics;
    statistics.elapsed_time = now - statistics.elapsed_time;
}

// FUNCTION: SURRENDER 0x10014950
void srStatisticsManager::dump(std::ostream& stream, const Statistics& statistics)
{
    stream << '\n';
    stream << "Pipeline statistics/second:" << '\n';
    stream << "Time since reset:               " << statistics.elapsed_time << '\n' << '\n';
    if (statistics.elapsed_time > 0.01) {
        stream << "Meshes traversed:               "
               << statistics.meshes_traversed / statistics.elapsed_time << '\n';
        stream << "Meshes submitted:               "
               << statistics.meshes_submitted / statistics.elapsed_time << '\n';
        stream << "Triangles submitted:            "
               << statistics.triangles_submitted / statistics.elapsed_time << '\n';
        stream << "Triangles after culling:        "
               << statistics.triangles_after_culling / statistics.elapsed_time
               << '\n';
        if (statistics.triangles_submitted != 0) {
            stream << "Triangle cull ratio:            "
                   << 100.0 - statistics.triangles_after_culling * 100.0 /
                                 statistics.triangles_submitted
                   << "%" << '\n';
        }
        stream << "Vertices submitted:             "
               << statistics.vertices_submitted / statistics.elapsed_time << '\n';
        stream << "Vertices after culling:         "
               << statistics.vertices_after_culling / statistics.elapsed_time << '\n';
        if (statistics.vertices_submitted != 0) {
            stream << "Vertex cull ratio:              "
                   << 100.0 - statistics.vertices_after_culling * 100.0 /
                                 statistics.vertices_submitted
                   << "%" << '\n';
        }
        stream << "Material processing stalls:     "
               << statistics.material_processing_stalls / statistics.elapsed_time
               << '\n';
        if (statistics.meshes_submitted != 0) {
            stream << "Avg. triangles per mesh:        "
                   << statistics.triangles_submitted /
                          (double)statistics.meshes_submitted
                   << '\n';
            stream << "Avg. vertices per mesh:         "
                   << statistics.vertices_submitted /
                          (double)statistics.meshes_submitted
                   << '\n';
        }
        if (statistics.vertices_after_culling != 0) {
            double inv_vertices = 1.0 / statistics.vertices_after_culling;
            stream << "Avg. diffuse ops per vertex:    "
                   << statistics.diffuse_operations * inv_vertices << '\n';
            stream << "Avg. alpha ops per vertex:      "
                   << statistics.alpha_operations * inv_vertices << '\n';
            stream << "Avg. specular ops per vertex:   "
                   << statistics.specular_operations * inv_vertices << '\n';
            stream << "Avg. fog ops per vertex:        "
                   << statistics.fog_operations * inv_vertices << '\n';
            stream << "Avg. texcoord ops per vertex:   "
                   << statistics.texture_coordinate_operations * inv_vertices << '\n';
        }
    }
    stream << std::endl;
}

/* Per-TU CRT stream-init sentinels: retail emits an ios_base::Init ctor call
   and atexit registrar (0x10014E20/0x10014E30) plus the _Winit pair
   (0x10014E60/0x10014E70) for this unit's <iostream> include. */

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
