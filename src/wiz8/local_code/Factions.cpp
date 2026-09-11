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

    g_faction_relations[W8_FACTION_BROTHERHOOD][W8_FACTION_RAPAX_COMMON] = 1;
    g_faction_relations[W8_FACTION_BROTHERHOOD][W8_FACTION_RAPAX_TEMPLAR] = 1;
    g_faction_relations[W8_FACTION_BROTHERHOOD][W8_FACTION_RAPAX_ARMY] = 1;
    g_faction_relations[W8_FACTION_BROTHERHOOD][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_BROTHERHOOD][W8_FACTION_KINGS_ASSASINS] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_BANK][W8_FACTION_RAPAX_COMMON] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_BANK][W8_FACTION_RAPAX_TEMPLAR] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_BANK][W8_FACTION_RAPAX_ARMY] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_BANK][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_BANK][W8_FACTION_KINGS_ASSASINS] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_HLL][W8_FACTION_RAPAX_COMMON] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_HLL][W8_FACTION_RAPAX_TEMPLAR] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_HLL][W8_FACTION_RAPAX_ARMY] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_HLL][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_HLL][W8_FACTION_KINGS_ASSASINS] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_COMMON][W8_FACTION_RAPAX_COMMON] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_COMMON][W8_FACTION_RAPAX_TEMPLAR] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_COMMON][W8_FACTION_RAPAX_ARMY] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_COMMON][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_HIGARDI_COMMON][W8_FACTION_KINGS_ASSASINS] = 1;
    g_faction_relations[W8_FACTION_TRYNNIE][W8_FACTION_RAPAX_COMMON] = 1;
    g_faction_relations[W8_FACTION_TRYNNIE][W8_FACTION_RATTKIN_COMMON] = 1;
    g_faction_relations[W8_FACTION_TRYNNIE][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_UMPANI][W8_FACTION_TRANG] = 1;
    g_faction_relations[W8_FACTION_UMPANI][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_TRANG][W8_FACTION_UMPANI] = 1;
    g_faction_relations[W8_FACTION_TRANG][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_RATTKIN_COMMON][W8_FACTION_TRYNNIE] = 1;
    g_faction_relations[W8_FACTION_RATTKIN_MAFIA][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_RAPAX_COMMON][W8_FACTION_BROTHERHOOD] = 1;
    g_faction_relations[W8_FACTION_RAPAX_COMMON][W8_FACTION_HIGARDI_BANK] = 1;
    g_faction_relations[W8_FACTION_RAPAX_COMMON][W8_FACTION_HIGARDI_HLL] = 1;
    g_faction_relations[W8_FACTION_RAPAX_COMMON][W8_FACTION_HIGARDI_COMMON] = 1;
    g_faction_relations[W8_FACTION_RAPAX_COMMON][W8_FACTION_TRYNNIE] = 1;
    g_faction_relations[W8_FACTION_RAPAX_TEMPLAR][W8_FACTION_BROTHERHOOD] = 1;
    g_faction_relations[W8_FACTION_RAPAX_TEMPLAR][W8_FACTION_HIGARDI_BANK] = 1;
    g_faction_relations[W8_FACTION_RAPAX_TEMPLAR][W8_FACTION_HIGARDI_HLL] = 1;
    g_faction_relations[W8_FACTION_RAPAX_TEMPLAR][W8_FACTION_HIGARDI_COMMON] = 1;
    g_faction_relations[W8_FACTION_RAPAX_TEMPLAR][W8_FACTION_TRYNNIE] = 1;
    g_faction_relations[W8_FACTION_RAPAX_ARMY][W8_FACTION_BROTHERHOOD] = 1;
    g_faction_relations[W8_FACTION_RAPAX_ARMY][W8_FACTION_HIGARDI_BANK] = 1;
    g_faction_relations[W8_FACTION_RAPAX_ARMY][W8_FACTION_HIGARDI_HLL] = 1;
    g_faction_relations[W8_FACTION_RAPAX_ARMY][W8_FACTION_HIGARDI_COMMON] = 1;
    g_faction_relations[W8_FACTION_RAPAX_ARMY][W8_FACTION_TRYNNIE] = 1;
    g_faction_relations[W8_FACTION_MAD_MARTEN][W8_FACTION_DARK_SAVANT] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_BROTHERHOOD] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_HIGARDI_BANK] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_HIGARDI_HLL] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_HIGARDI_COMMON] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_TRYNNIE] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_UMPANI] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_TRANG] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_RATTKIN_COMMON] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_RATTKIN_MAFIA] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_MOOK] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_MAD_MARTEN] = 1;
    g_faction_relations[W8_FACTION_DARK_SAVANT][W8_FACTION_COSMIC_LORDS] = 1;

    g_factions[W8_FACTION_PARTY].disposition_score = 100;
    g_factions[W8_FACTION_COSMIC_LORDS].disposition_score = 100;
    g_factions[W8_FACTION_DARK_SAVANT].disposition_score = 0;
    g_factions[W8_FACTION_UMPANI].disposition_score = 0x50;
    g_factions[W8_FACTION_TRANG].disposition_score = 0x50;
    g_factions[W8_FACTION_MOOK].disposition_score = 0x50;
    g_factions[W8_FACTION_RATTKIN_COMMON].disposition_score = 0x32;
    g_factions[W8_FACTION_RATTKIN_MAFIA].disposition_score = 0x50;
    g_factions[W8_FACTION_BROTHERHOOD].disposition_score = 0x50;
    g_factions[W8_FACTION_HIGARDI_BANK].disposition_score = 0x50;
    g_factions[W8_FACTION_HIGARDI_HLL].disposition_score = 0x50;
    g_factions[W8_FACTION_HIGARDI_COMMON].disposition_score = 0x50;
    g_factions[W8_FACTION_TRYNNIE].disposition_score = 0x50;
    g_factions[W8_FACTION_MAD_MARTEN].disposition_score = 0x46;
    g_factions[W8_FACTION_RAPAX_COMMON].disposition_score = 0x19;
    g_factions[W8_FACTION_RAPAX_TEMPLAR].disposition_score = 0x32;
    g_factions[W8_FACTION_RAPAX_ARMY].disposition_score = 0x32;
    g_factions[W8_FACTION_KINGS_ASSASINS].disposition_score = 0x50;
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
    if (faction >= W8_FACTION_COUNT) {
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
