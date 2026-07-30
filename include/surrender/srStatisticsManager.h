#pragma once

#include <iostream>

#include "srHeap.h"

class srStatisticsManager {
public:
    struct Statistics {
        double elapsed_time_00;
        unsigned long meshes_traversed_08;
        unsigned long meshes_submitted_0c;
        unsigned long triangles_submitted_10;
        unsigned long triangles_after_culling_14;
        unsigned long vertices_submitted_18;
        unsigned long vertices_after_culling_1c;
        unsigned long material_processing_stalls_20;
        unsigned long diffuse_operations_24;
        unsigned long specular_operations_28;
        unsigned long alpha_operations_2c;
        unsigned long fog_operations_30;
        unsigned long texture_coordinate_operations_34;
    };

    SR_DLL_IMPORT void reset();
    SR_DLL_IMPORT void getStatistics(Statistics& statistics) const;
    SR_DLL_IMPORT void dump(
        std::ostream& stream, const Statistics& statistics);

private:
    Statistics statistics_00;
};

static_assert(sizeof(srStatisticsManager::Statistics) == 0x38,
              "srStatisticsManager_Statistics_must_be_0x38");
static_assert(sizeof(srStatisticsManager) == 0x38,
              "srStatisticsManager_must_be_0x38");
