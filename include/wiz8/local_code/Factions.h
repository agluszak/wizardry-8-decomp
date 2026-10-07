#ifndef WIZ8_LOCAL_CODE_FACTIONS_H
#define WIZ8_LOCAL_CODE_FACTIONS_H

/* The factions. The name table, the runtime score array and the relationship
   matrix keep one row per id. The last two ids name no faction. */
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
    /* Witnessed offenses against the faction; RecordFactionOffense
       increments it (cap 0xfa) and reads it for the penalty tiers. */
    int offense_count;
    unsigned char unknown_05;
    /* G_status.world_clock at the last band change. */
    int band_changed_clock;
    /* Raised by the sight pass the first time the party sees one of
       this faction's monsters; the journal lists encountered factions. */
    bool encountered;
    unsigned char unknown_0b[3];
};

#pragma pack(pop)

typedef unsigned char W8FactionDisposition;

enum { W8_FACTION_HOSTILE = 0, W8_FACTION_NEUTRAL = 1, W8_FACTION_FRIENDLY = 2 };

typedef unsigned char W8Disposition;

enum { W8_DISPOSITION_NEUTRAL = 0, W8_DISPOSITION_HOSTILE = 1, W8_DISPOSITION_FRIENDLY = 2 };

extern W8FactionRuntimeRecord g_factions[W8_FACTION_COUNT];
/* The 21x21 byte relation matrix, one row per faction, saved and
   loaded whole by the FATA state handlers and reset with g_factions. */
extern unsigned char g_faction_relations[W8_FACTION_COUNT][W8_FACTION_COUNT];
W8FactionDisposition GetFactionDisposition(signed char faction);

/* `other` as the party reads the live score band; any other
   target is answered by the static relation matrix. */
W8FactionDisposition GetFactionDispositionToward(signed char faction, signed char other);

signed char GetFactionDispositionScore(signed char faction);

/* Zero both faction tables and seed the starting dispositions and
   relations a fresh game begins with. */
void ResetFactions(void);

void SetFactionDispositionBand(signed char faction, signed char band);

/* Faction change entry point - op 1 with a faction outside
   unaligned/party routes mode 0 to the witnessed-offense path and modes
   1..3 to a direct disposition delta. */
void ApplyFactionChange(char mode, char op, signed char faction, int value);
/* Bump the faction's offense count and apply the penalty the
   victim's record and the running count select. */
void RecordFactionOffense(signed char faction, unsigned int victim_location_index);
/* Clamp disposition_score + delta to 0..99, stamp the world clock
   when the band moved, and post the worsened/improved notice. */
void AdjustFactionDisposition(signed char faction, char delta);
/* FATA section save and load. */
void SaveFactionState(int file);
void LoadFactionState(int file);
#endif
