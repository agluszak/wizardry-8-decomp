#ifndef WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H
#define WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H

#include "wiz8/geometry.h"
#include "wiz8/engine_code/IntervalGate.h"

struct W8EncounterTableRuntime;
struct W8Item;
struct W8MonsterRecord;
template <class T> class W8GrowableVector;

#pragma pack(push, 1)
struct W8MonsterGenerator {
    unsigned int flags;    /* 0x00: bit 2 is cleared on teardown */
    unsigned char flag_04; /* 0x04 */
    unsigned char unknown_05;
    unsigned short value_06; /* 0x06 */
    unsigned short value_08; /* 0x08 */
    unsigned char unknown_0a[2];
    /* 0x0c: the generator's world position, saved as three dwords and handed
       to GenerateEncounter and SetLocation as a block. */
    srVector3T<float> state_0c;
    W8Item* node_18; /* 0x18: loaded mongen.itm marker */
    int value_1c;    /* 0x1c */
    /* 0x20: m_pTimer, named by the MonGen.cpp:535 assertion, whose message also
       gives the owning class and method - "MonGen::Reset() out of memory
       allocating m_pTimer". */
    W8IntervalGate* m_pTimer;
    char name[32];         /* 0x24 */
    unsigned char flag_44; /* 0x44: written to the save after the name */

    /* Named by the assertion message above. Rearms the generator's timer,
       creating it on first use, with a delay jittered around the configured
       interval. */
    /* 0x0048A680 */
    W8MonsterGenerator();
    void Reset();
    /* Tests the global encounter gates, range/LOS/occupancy constraints and
       this generator's chance before one random encounter is allowed. */
    unsigned char CanGenerateEncounter(int force); /* 0x0048B200 */
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
    /* Moves the generator, notifying the scene when it is armed. */
    void SetState(const srVector3T<float>* state);
    /* Loads the marker unconditionally, then applies the armed state. */
    void Reload(int unused, unsigned char active);
    /* 0x0048CC30: strncpy into the fixed 32-byte name member. */
    void SetName(const char* name);
    ~W8MonsterGenerator();
};
#pragma pack(pop)

W8MonsterGenerator* FindMonGenByName(const char* name);

#endif