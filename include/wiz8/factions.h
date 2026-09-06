#ifndef WIZ8_FACTIONS_H
#define WIZ8_FACTIONS_H

enum { W8_FACTION_COUNT = 21 };

#pragma pack(push, 1)

typedef struct W8FactionRuntimeRecord {
    signed char disposition_score;
    unsigned char unknown_01[3];
    int value_04;
    unsigned char flag_08;
    unsigned char unknown_09[5];
} W8FactionRuntimeRecord;

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
W8FactionDisposition GetFactionDisposition(signed char faction);

#endif
