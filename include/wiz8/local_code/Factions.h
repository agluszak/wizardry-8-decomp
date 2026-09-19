#ifndef WIZ8_LOCAL_CODE_FACTIONS_H
#define WIZ8_LOCAL_CODE_FACTIONS_H

/* The factions, in the order the contiguous 21-name table at 0x0061CE74
   carries them. FindFactionByName indexes that table with these ids and both
   the runtime score array and the relationship matrix keep one row per id. The
   last two ids name no faction and exist only to hold the table's extent. */
enum W8Faction {
    W8_FACTION_UNALIGNED = 0,
    W8_FACTION_PARTY = 1,
    W8_FACTION_DARK_SAVANT = 2,
    W8_FACTION_COSMIC_LORDS = 3,
    W8_FACTION_UMPANI = 4,
    W8_FACTION_TRANG = 5,
    W8_FACTION_MOOK = 6,
    W8_FACTION_RATTKIN_COMMON = 7,
    W8_FACTION_RATTKIN_MAFIA = 8,
    W8_FACTION_BROTHERHOOD = 9,
    W8_FACTION_HIGARDI_BANK = 10,
    W8_FACTION_HIGARDI_HLL = 11,
    W8_FACTION_HIGARDI_COMMON = 12,
    W8_FACTION_TRYNNIE = 13,
    W8_FACTION_MAD_MARTEN = 14,
    W8_FACTION_RAPAX_COMMON = 15,
    W8_FACTION_RAPAX_TEMPLAR = 16,
    W8_FACTION_RAPAX_ARMY = 17,
    W8_FACTION_KINGS_ASSASINS = 18,
    W8_FACTION_FILLER_19 = 19,
    W8_FACTION_FILLER_20 = 20,
    W8_FACTION_COUNT = 21,
    /* FindFactionByName answers this for a name no faction carries. */
    W8_FACTION_INVALID = -1
};

#pragma pack(push, 1)

struct W8FactionRuntimeRecord {
    signed char disposition_score;
    /* 0x01: witnessed offenses against the faction; RecordFactionOffense
       increments it (cap 0xfa) and reads it for the penalty tiers. */
    int offense_count_01;
    unsigned char unknown_05;
    /* 0x06: g_status_685170.world_clock at the last band change. */
    int band_changed_clock_06;
    unsigned char flag_0a;
    unsigned char unknown_0b[3];
};

#pragma pack(pop)

typedef unsigned char W8FactionDisposition;

enum { W8_FACTION_HOSTILE = 0, W8_FACTION_NEUTRAL = 1, W8_FACTION_FRIENDLY = 2 };

typedef unsigned char W8Disposition;

enum { W8_DISPOSITION_NEUTRAL = 0, W8_DISPOSITION_HOSTILE = 1, W8_DISPOSITION_FRIENDLY = 2 };

extern W8FactionRuntimeRecord g_factions[W8_FACTION_COUNT];
/* 0x0068D528: the 21x21 byte relation matrix, one row per faction, saved and
   loaded whole by the FATA state handlers and reset with g_factions. */
extern unsigned char g_faction_relations[W8_FACTION_COUNT][W8_FACTION_COUNT];
W8FactionDisposition GetFactionDisposition(signed char faction);

/* 0x00535C50: `other` as the party reads the live score band; any other
   target is answered by the static relation matrix. */
W8FactionDisposition GetFactionDispositionToward(signed char faction, signed char other);

signed char GetFactionDispositionScore(signed char faction);

/* 0x00535920: zero both faction tables and seed the starting dispositions and
   relations a fresh game begins with. */
void ResetFactions(void);

void SetFactionDispositionBand(signed char faction, signed char band); /* 0x00535B40 */

/* 0x00535CF0: faction change entry point - op 1 with a faction outside
   unaligned/party routes mode 0 to the witnessed-offense path and modes
   1..3 to a direct disposition delta. */
void ApplyFactionChange(char mode, char op, signed char faction, int value);
/* 0x00535D50: bump the faction's offense count and apply the penalty the
   victim's record and the running count select. */
void RecordFactionOffense(signed char faction, unsigned int victim_location_index);
/* 0x00535EA0: clamp disposition_score + delta to 0..99, stamp the world clock
   when the band moved, and post the worsened/improved notice. */
void AdjustFactionDisposition(signed char faction, char delta);
#endif
