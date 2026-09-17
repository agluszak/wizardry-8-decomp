#include "wiz8/local_code/Factions.h"
#include "wiz8/sr_api.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"

#include <string.h>
#include <stdio.h>

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
        srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xaf,
                     0);
    }
    if (faction >= W8_FACTION_COUNT) {
        srAssertFail("bFaction < FACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
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

/* gppStringList index of each faction's display name, in faction-id order.
   The two filler factions reuse the unaligned name. */
// GLOBAL: WIZ8 0x0061EACC
const unsigned short g_faction_name_string_ids_61eacc[W8_FACTION_COUNT] = {
    0x5a1, 0x5a2, 0x5a3, 0x5a4, 0x5a5, 0x5a6, 0x5a7, 0x5a8, 0x5a9, 0x5aa, 0x5ab,
    0x5ac, 0x5ad, 0x5ae, 0x5af, 0x5b0, 0x5b1, 0x5b2, 0x5b3, 0x5a1, 0x5a1,
};

/* Move a faction's disposition band (0 hostile, 1 neutral, 2 friendly): the
   score is set to the band's anchor value, the change is timestamped and a
   banner notice posts when the band actually moved. */
// FUNCTION: WIZ8 0x00535B40
void SetFactionDispositionBand(signed char faction, signed char band)
{
    wchar_t message[20];
    char old_band;
    char icon;

    if (faction < 0) {
        srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xaf,
                     0);
    }
    if (faction >= W8_FACTION_COUNT) {
        srAssertFail("bFaction < FACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
    }

    if (g_factions[faction].disposition_score < 34) {
        old_band = W8_FACTION_HOSTILE;
    } else {
        old_band = (g_factions[faction].disposition_score >= 67) + 1;
    }
    switch (band) {
    case W8_FACTION_HOSTILE:
        g_factions[faction].disposition_score = 0x19;
        break;
    case W8_FACTION_NEUTRAL:
        g_factions[faction].disposition_score = 0x37;
        break;
    case W8_FACTION_FRIENDLY:
        g_factions[faction].disposition_score = 0x4b;
        break;
    }
    if (band == old_band) {
        return;
    }
    g_factions[faction].value_06 = g_status_685170.world_clock;
    if (band < old_band) {
        swprintf(message, gppStringList[0x914 / 4]);
        icon = 0;
    } else {
        swprintf(message, gppStringList[0x918 / 4]);
        icon = 5;
    }
    Function58AAD0(icon, gppStringList[0x91c / 4],
                   gppStringList[g_faction_name_string_ids_61eacc[faction]], message);
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
