#include "wiz8/factions.h"
#include "wiz8/sr_api.h"

#include <string.h>

/* Local Code\Factions.cpp. The unit is named by the two assertions this body
   embeds, and the 0..20 domain they bound is the same one the contiguous
   21-name faction table establishes. */

// GLOBAL: WIZ8 0x0068D6E8
W8FactionRuntimeRecord g_factions[W8_FACTION_COUNT];

// GLOBAL: WIZ8 0x0068D528
unsigned char g_faction_relations[W8_FACTION_COUNT][W8_FACTION_COUNT];

/* Zero both tables, then write the starting relation cells and the per-faction
   disposition scores. The two low scores are the hostile bands the faction
   accessors cut at 34 and 67. */
// FUNCTION: WIZ8 0x00535920
void ResetFactions(void)
{
    memset(g_factions, 0, sizeof(g_factions));
    memset(g_faction_relations, 0, sizeof(g_faction_relations));

    g_faction_relations[9][15] = 1;
    g_faction_relations[9][16] = 1;
    g_faction_relations[9][17] = 1;
    g_faction_relations[9][2] = 1;
    g_faction_relations[9][18] = 1;
    g_faction_relations[10][15] = 1;
    g_faction_relations[10][16] = 1;
    g_faction_relations[10][17] = 1;
    g_faction_relations[10][2] = 1;
    g_faction_relations[10][18] = 1;
    g_faction_relations[11][15] = 1;
    g_faction_relations[11][16] = 1;
    g_faction_relations[11][17] = 1;
    g_faction_relations[11][2] = 1;
    g_faction_relations[11][18] = 1;
    g_faction_relations[12][15] = 1;
    g_faction_relations[12][16] = 1;
    g_faction_relations[12][17] = 1;
    g_faction_relations[12][2] = 1;
    g_faction_relations[12][18] = 1;
    g_faction_relations[13][15] = 1;
    g_faction_relations[13][7] = 1;
    g_faction_relations[13][2] = 1;
    g_faction_relations[4][5] = 1;
    g_faction_relations[4][2] = 1;
    g_faction_relations[5][4] = 1;
    g_faction_relations[5][2] = 1;
    g_faction_relations[7][13] = 1;
    g_faction_relations[8][2] = 1;
    g_faction_relations[15][9] = 1;
    g_faction_relations[15][10] = 1;
    g_faction_relations[15][11] = 1;
    g_faction_relations[15][12] = 1;
    g_faction_relations[15][13] = 1;
    g_faction_relations[16][9] = 1;
    g_faction_relations[16][10] = 1;
    g_faction_relations[16][11] = 1;
    g_faction_relations[16][12] = 1;
    g_faction_relations[16][13] = 1;
    g_faction_relations[17][9] = 1;
    g_faction_relations[17][10] = 1;
    g_faction_relations[17][11] = 1;
    g_faction_relations[17][12] = 1;
    g_faction_relations[17][13] = 1;
    g_faction_relations[14][2] = 1;
    g_faction_relations[2][9] = 1;
    g_faction_relations[2][10] = 1;
    g_faction_relations[2][11] = 1;
    g_faction_relations[2][12] = 1;
    g_faction_relations[2][13] = 1;
    g_faction_relations[2][4] = 1;
    g_faction_relations[2][5] = 1;
    g_faction_relations[2][7] = 1;
    g_faction_relations[2][8] = 1;
    g_faction_relations[2][6] = 1;
    g_faction_relations[2][14] = 1;
    g_faction_relations[2][3] = 1;

    g_factions[1].disposition_score = 100;
    g_factions[3].disposition_score = 100;
    g_factions[2].disposition_score = 0;
    g_factions[4].disposition_score = 0x50;
    g_factions[5].disposition_score = 0x50;
    g_factions[6].disposition_score = 0x50;
    g_factions[7].disposition_score = 0x32;
    g_factions[8].disposition_score = 0x50;
    g_factions[9].disposition_score = 0x50;
    g_factions[10].disposition_score = 0x50;
    g_factions[11].disposition_score = 0x50;
    g_factions[12].disposition_score = 0x50;
    g_factions[13].disposition_score = 0x50;
    g_factions[14].disposition_score = 0x46;
    g_factions[15].disposition_score = 0x19;
    g_factions[16].disposition_score = 0x32;
    g_factions[17].disposition_score = 0x32;
    g_factions[18].disposition_score = 0x50;
}

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
