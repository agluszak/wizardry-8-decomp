#include "wiz8/local_code/Factions.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/sr_api.h"

#include <stdio.h>
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

/* 0x0061EACC: the gppStringList name id for each faction row; the two filler
   factions reuse the unaligned name. */
// GLOBAL: WIZ8 0x0061EACC
const unsigned short g_faction_name_ids_61eacc[W8_FACTION_COUNT] = {
    0x5a1, 0x5a2, 0x5a3, 0x5a4, 0x5a5, 0x5a6, 0x5a7, 0x5a8, 0x5a9, 0x5aa, 0x5ab,
    0x5ac, 0x5ad, 0x5ae, 0x5af, 0x5b0, 0x5b1, 0x5b2, 0x5b3, 0x5a1, 0x5a1,
};

/* Snap a faction's score onto the requested band's anchor (25/55/75). When the
   band actually moved, stamp the world clock and post the worsened/improved
   notice naming the faction. */
// FUNCTION: WIZ8 0x00535B40
void SetFactionDispositionBand(signed char faction, char band)
{
    wchar_t notice[20];
    char old_band;
    char palette;

    if (faction < 0) {
        srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xaf,
                     0);
    }
    if (faction >= W8_FACTION_COUNT) {
        srAssertFail("bFaction < FACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
    }
    if (g_factions[faction].disposition_score < 34) {
        old_band = 0;
    } else {
        old_band = (g_factions[faction].disposition_score >= 67) + 1;
    }
    switch (band) {
    case 0:
        g_factions[faction].disposition_score = 0x19;
        break;
    case 1:
        g_factions[faction].disposition_score = 0x37;
        break;
    case 2:
        g_factions[faction].disposition_score = 0x4b;
        break;
    }
    if (band != old_band) {
        g_factions[faction].band_changed_clock_06 = g_status_685170.world_clock;
        if (band < old_band) {
            swprintf(notice, gppStringList[0x245]);
            palette = 0;
        } else {
            swprintf(notice, gppStringList[0x246]);
            palette = 5;
        }
        ShowNoticef(palette, gppStringList[0x247],
                    gppStringList[g_faction_name_ids_61eacc[faction]], notice);
    }
}

/* The faction-effect dispatcher every caller funnels through: op 1 is the
   apply operation and skips the unaligned/party rows; mode 0 records a
   witnessed offense (value is the victim's monster location index) while
   modes 1..3 apply value as a direct disposition delta. */
// FUNCTION: WIZ8 0x00535CF0
void ApplyFactionChange(char mode, char op, signed char faction, int value)
{
    if (op == 1 && faction != 0 && faction != 1) {
        if (mode != 0) {
            if (mode > 0 && mode <= 3) {
                AdjustFactionDisposition(faction, value);
            }
        } else {
            RecordFactionOffense(faction, value);
        }
    }
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

/* A witnessed offense against the faction: bump the running count, then weigh
   the victim. Factions that flip hostile outright do so once they were not
   already hostile; everyone else loses a record-scaled amount that grows as
   the offense count climbs. */
// FUNCTION: WIZ8 0x00535D50
void RecordFactionOffense(signed char faction, unsigned int victim_location_index)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;

    if (g_factions[faction].offense_count_01 < 0xfa) {
        g_factions[faction].offense_count_01++;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(victim_location_index);
    record = GetMonsterDataForInfo(monster_info);
    switch (faction) {
    case W8_FACTION_MOOK:
    case W8_FACTION_RATTKIN_MAFIA:
    case W8_FACTION_HIGARDI_BANK:
    case W8_FACTION_HIGARDI_HLL:
    case W8_FACTION_HIGARDI_COMMON:
        if (faction < 0) {
            srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp",
                         0xaf, 0);
        }
        if (faction >= W8_FACTION_COUNT) {
            srAssertFail("bFaction < FACTION_COUNT",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
        }
        if (g_factions[faction].disposition_score >= 34) {
            SetFactionDispositionBand(faction, 0);
            return;
        }
    }
    if (record->record_id_187 == 0x146) {
        AdjustFactionDisposition(faction, -0x28);
        return;
    }
    if ((record->flags_0d0 & 1) != 0) {
        AdjustFactionDisposition(faction, -0x14);
        return;
    }
    if (g_factions[faction].offense_count_01 < 0x15) {
        AdjustFactionDisposition(faction, -2);
        return;
    }
    AdjustFactionDisposition(faction, g_factions[faction].offense_count_01 > 0x28 ? -10 : -6);
}

/* Shift a faction's disposition by delta, clamped to 0..99. A band crossing
   stamps the world clock; any score move posts the worsened/improved notice
   naming the faction. The bounds asserts genuinely run twice - before the
   read and again before the band check. */
// FUNCTION: WIZ8 0x00535EA0
void AdjustFactionDisposition(signed char faction, char delta)
{
    wchar_t notice[20];
    signed char score;
    signed char old_score;
    signed char old_band;
    signed char new_band;
    char palette;

    if (faction < 0) {
        srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xaf,
                     0);
    }
    if (faction >= W8_FACTION_COUNT) {
        srAssertFail("bFaction < FACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
    }
    score = g_factions[faction].disposition_score;
    old_score = g_factions[faction].disposition_score;
    if (old_score < 34) {
        old_band = 0;
    } else {
        old_band = (old_score >= 67) + 1;
    }
    if (score + delta > 99) {
        score = 99;
    } else if (score + delta < 0) {
        score = 0;
    } else {
        score += delta;
    }
    g_factions[faction].disposition_score = score;
    if (faction < 0) {
        srAssertFail("bFaction >= 0", "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xaf,
                     0);
    }
    if (faction >= W8_FACTION_COUNT) {
        srAssertFail("bFaction < FACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp", 0xb0, 0);
    }
    if (g_factions[faction].disposition_score < 34) {
        new_band = 0;
    } else {
        new_band = (g_factions[faction].disposition_score >= 67) + 1;
    }
    if (new_band != old_band) {
        g_factions[faction].band_changed_clock_06 = g_status_685170.world_clock;
    }
    if (g_factions[faction].disposition_score < old_score) {
        swprintf(notice, gppStringList[0x245]);
        palette = 0;
    } else if (g_factions[faction].disposition_score > old_score) {
        swprintf(notice, gppStringList[0x246]);
        palette = 5;
    } else {
        return;
    }
    ShowNoticef(palette, gppStringList[0x247], gppStringList[g_faction_name_ids_61eacc[faction]],
                notice);
}
