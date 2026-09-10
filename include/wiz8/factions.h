#ifndef WIZ8_FACTIONS_H
#define WIZ8_FACTIONS_H

enum { W8_FACTION_COUNT = 21 };

#pragma pack(push, 1)

struct W8FactionRuntimeRecord {
    signed char disposition_score;
    unsigned char unknown_01[5];
    int value_06;
    unsigned char flag_0a;
    unsigned char unknown_0b[3];
};

#pragma pack(pop)

typedef unsigned char W8FactionDisposition;

enum {
    W8_FACTION_HOSTILE = 0,
    W8_FACTION_NEUTRAL = 1,
    W8_FACTION_FRIENDLY = 2
};

enum {
    W8_FACTION_UNALIGNED = 0,
    W8_FACTION_PARTY = 1
};

enum {
    W8_DISPOSITION_NEUTRAL = 0,
    W8_DISPOSITION_HOSTILE = 1,
    W8_DISPOSITION_FRIENDLY = 2
};

extern W8FactionRuntimeRecord g_factions[W8_FACTION_COUNT];
/* 0x0068D528: the 21x21 byte relation matrix, one row per faction, saved and
   loaded whole by the FATA state handlers and reset with g_factions. */
extern unsigned char g_faction_relations[W8_FACTION_COUNT][W8_FACTION_COUNT];
W8FactionDisposition GetFactionDisposition(signed char faction);

signed char GetFactionDispositionScore(signed char faction);

/* 0x00535920: zero both faction tables and seed the starting dispositions and
   relations a fresh game begins with. */
void ResetFactions(void);

#endif
