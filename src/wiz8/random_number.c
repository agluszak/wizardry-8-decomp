#include "gameplay_boundaries.h"

#include <stdlib.h>

// FUNCTION: WIZ8 0x0040EFA0
unsigned int GetRandomNumber(unsigned int upper_bound)
{
    if (upper_bound == 0) {
        return 0;
    }
    return ((unsigned int)rand() * upper_bound / RAND_MAX) % upper_bound;
}
