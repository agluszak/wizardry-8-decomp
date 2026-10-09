#pragma once

#include <iosfwd>

#include "srHeap.h"

class srStatisticsManager {
public:
    struct Statistics {
        double elapsed_time;
        w8_ulong meshes_traversed;
        w8_ulong meshes_submitted;
        w8_ulong triangles_submitted;
        w8_ulong triangles_after_culling;
        w8_ulong vertices_submitted;
        w8_ulong vertices_after_culling;
        w8_ulong material_processing_stalls;
        w8_ulong diffuse_operations;
        w8_ulong specular_operations;
        w8_ulong alpha_operations;
        w8_ulong fog_operations;
        w8_ulong texture_coordinate_operations;
    };

    SR_DLL_IMPORT void reset();
    SR_DLL_IMPORT void getStatistics(Statistics& statistics) const;
    SR_DLL_IMPORT void dump(std::ostream& stream, const Statistics& statistics);

    /* SurRender's submission pipeline updates these counters directly. */
    Statistics statistics;
};

W8_ABI_ASSERT(sizeof(srStatisticsManager::Statistics) == 0x38,
              "srStatisticsManager_Statistics_must_be_0x38");
W8_ABI_ASSERT(sizeof(srStatisticsManager) == 0x38, "srStatisticsManager_must_be_0x38");
