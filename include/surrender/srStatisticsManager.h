#pragma once

#include <iostream>

#include "srHeap.h"

class srStatisticsManager {
public:
    struct Statistics {
        double elapsed_time_00;
        unsigned long values_08[12];
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
