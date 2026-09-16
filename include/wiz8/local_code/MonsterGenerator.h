#ifndef WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H
#define WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H

#include "wiz8/geometry.h"
#include "wiz8/engine_code/IntervalGate.h"

struct W8EncounterTableRuntime;
struct W8Item;
struct W8MonsterRecord;
template <class T> class W8GrowableVector;

/* Retail allocates 0x48 bytes for every MonGen. The unnamed bytes at +0x05,
   +0x0a..+0x0b and +0x45..+0x47 are ordinary alignment padding, not fields. */
struct W8MonsterGenerator {
    unsigned int flags; /* 0x00 */
    union {
        signed char custom_spawn_chance; /* 0x04: MIPE edits 0..100 in steps of ten */
        unsigned char flag_04;            /* compatibility alias for unrevised callers */
    };
    union {
        short custom_interval_seconds; /* 0x06: MIPE's custom "every N s" value */
        unsigned short value_06;       /* compatibility alias for unrevised callers */
    };
    union {
        short unknown_08;          /* 0x08: persisted, initialized to -1; semantics unknown */
        unsigned short value_08;   /* compatibility alias for unrevised callers */
    };
    /* 0x0c: world position, saved as three dwords and handed to GenerateEncounter. */
    srVector3T<float> state_0c;
    union {
        W8Item* marker_item; /* 0x18: loaded Data\Items3D\Bitmaps\mongen.itm marker */
        W8Item* node_18;     /* compatibility alias for unrevised MIPE callers */
    };
    union {
        int encounter_table_index; /* 0x1c: index into g_encounter_tables, -1 means none */
        int value_1c;               /* compatibility alias for unrevised MIPE callers */
    };
    /* 0x20: m_pTimer, named by the MonGen.cpp:535 assertion, whose message also
       gives the owning class and method - "MonGen::Reset() out of memory
       allocating m_pTimer". */
    W8IntervalGate* m_pTimer;
    char name[32]; /* 0x24 */
    union {
        unsigned char generation_enabled; /* 0x44: MIPE "Toggle Active"; gates CanGenerate */
        unsigned char flag_44;             /* compatibility alias for unrevised MIPE callers */
    };

    /* 0x0048A680 */
    W8MonsterGenerator();
    void Reset();
    /* Tests global encounter gates, range/LOS/occupancy constraints and this
       generator's chance. Retail reads the force argument as one byte. */
    unsigned char CanGenerateEncounter(unsigned char force); /* 0x0048B200 */
    /* Selects and spawns the encounter table entry at `position`. */
    unsigned char GenerateEncounter(const srVector3T<float>* position); /* 0x0048AD20 */
    /* Build the candidate-entry list for the current rarity/time/party level. */
    int SelectEncounterCandidates(W8EncounterTableRuntime* table,
                                  W8GrowableVector<int>* candidates); /* 0x0048B9A0 */
    /* Difficulty-aware group-size roll for one monster database record. */
    int RollEncounterGroupSize(W8MonsterRecord* record); /* 0x0048BC30 */
    /* Arms or disarms the generator, loading its marker on the way in. */
    void SetActive(unsigned char active, W8Item* node);
    /* The save pair. Both are __thiscall in the image. */
    void Save(int handle);
    unsigned char Load(int handle);
    /* 0x0048C110: the MONG chunk loader; its own assert spells the original
       name, MonGen::LoadAll. */
    static unsigned char LoadAll(int save_handle);
    /* Moves the generator, notifying the scene when the generator has a marker. */
    void SetState(const srVector3T<float>* state);
    /* Loads the marker unconditionally, then applies the armed state. */
    void Reload(int unused, unsigned char active);
    /* 0x0048CC30: strncpy into the fixed 32-byte name member. */
    void SetName(const char* name);
    /* Select one loaded encounter table and apply its HARASSMENT flag. */
    void SetEncounterTable(int index); /* 0x0048CC50 */
    ~W8MonsterGenerator();
};

static_assert(sizeof(W8MonsterGenerator) == 0x48, "W8MonsterGenerator_size_must_be_0x48");

W8MonsterGenerator* FindMonGenByName(const char* name);

#endif