/* Local Code\ThingEditorShared.cpp - code shared with the thing editor.

   Retail retains no path string for this unit; the official demo carries two
   "E:\Wizardry 8\Local Code\ThingEditorShared.cpp" anchors at demo 0x0055AC00
   (line 444) and 0x0055AE80 (line 540). Retail 0x00556000 uniquely matches
   demo 0x0055AE30, which sits inside that demo hull. The unit's retail span
   lies between Formation & Facing.cpp (0x00555F30) and GroupAttacks.cpp
   (0x00556050); the other bodies in that interval stay unproven. */

#include "wiz8/local_code/ThingEditorShared.h"

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
