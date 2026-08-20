#include "wiz8/gameplay_boundaries.h"
#include "wiz8/factions.h"
#include "wiz8/sr_api.h"

/* Local Code\Factions.cpp. The unit is named by the two assertions this body
   embeds, and the 0..20 domain they bound is the same one the contiguous
   21-name faction table establishes. */

// FUNCTION: WIZ8 0x00535ad0
W8FactionDisposition GetFactionDisposition(signed char faction)
{
    signed char disposition_score;

    if (faction < 0) {
        srAssertFail(
            "bFaction >= 0",
            "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp",
            0xaf,
            0);
    }
    if (faction >= 21) {
        srAssertFail(
            "bFaction < FACTION_COUNT",
            "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp",
            0xb0,
            0);
    }

    disposition_score = g_factions[faction].disposition_score;
    if (disposition_score < 34) {
        return W8_FACTION_HOSTILE;
    }
    if (disposition_score < 67) {
        return W8_FACTION_NEUTRAL;
    }
    return W8_FACTION_FRIENDLY;
}

/* The raw score the band above is derived from. Unlike its neighbour this one
   asserts nothing, which is what makes the checked accessor the one callers
   outside the file are meant to use. The index arithmetic is the 14-byte
   record stride the reviewed table already establishes. */
// FUNCTION: WIZ8 0x00535d30
signed char GetFactionDispositionScore(signed char faction)
{
    return g_factions[faction].disposition_score;
}
