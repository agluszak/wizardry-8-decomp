#pragma once

#include <iosfwd>

#include "srHeap.h"

class srStatisticsManager {
public:
    struct Statistics {
        double elapsed_time;
        unsigned long meshes_traversed;
        unsigned long meshes_submitted;
        unsigned long triangles_submitted;
        unsigned long triangles_after_culling;
        unsigned long vertices_submitted;
        unsigned long vertices_after_culling;
        unsigned long material_processing_stalls;
        unsigned long diffuse_operations;
        unsigned long specular_operations;
        unsigned long alpha_operations;
        unsigned long fog_operations;
        unsigned long texture_coordinate_operations;
    };

    SR_DLL_IMPORT void reset();
    SR_DLL_IMPORT void getStatistics(Statistics& statistics) const;
    SR_DLL_IMPORT void dump(
        std::ostream& stream, const Statistics& statistics);

    /* SurRender's submission pipeline updates these counters directly. */
    Statistics statistics;
};

static_assert(sizeof(srStatisticsManager::Statistics) == 0x38,
              "srStatisticsManager_Statistics_must_be_0x38");
static_assert(sizeof(srStatisticsManager) == 0x38,
              "srStatisticsManager_must_be_0x38");
