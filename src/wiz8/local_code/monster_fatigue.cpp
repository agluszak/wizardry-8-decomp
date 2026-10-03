#include "wiz8/local_code/HealthStaminaMana.h"

// FUNCTION: WIZ8 0x0052A780
int CalculateMonsterFatigueBand(int current, int maximum)
{
    int percentage_lost = 100 - current * 100 / static_cast<unsigned int>(maximum);
    if (percentage_lost < 50)
        return 0;
    if (percentage_lost < 70)
        return 1;
    if (percentage_lost < 85)
        return 2;
    if (percentage_lost < 95)
        return 3;
    return 4;
}
