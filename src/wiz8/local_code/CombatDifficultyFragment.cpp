#include "wiz8/local_code/CombatDifficulty.h"

#include <math.h>

/* Scale total surviving hostile experience to the level used by the combat
   difficulty evaluator. The signed byte clamp is visible in retail. */
// FUNCTION: WIZ8 0x00556000
unsigned char EstimateCombatThreatLevel(unsigned int experience)
{
    signed char level =
        static_cast<signed char>((pow(static_cast<double>(experience), 0.29f) - 3.5) * 0.5);
    if (level < 0) {
        return 0;
    }
    if (level > 50) {
        return 50;
    }
    return level;
}
