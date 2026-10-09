#ifndef WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H
#define WIZ8_LOCAL_CODE_MONSTER_GENERATOR_H

#include "wiz8/geometry.h"
#include "wiz8/engine_code/IntervalGate.h"

struct W8EncounterTableRuntime;
struct W8Item;
struct W8MonsterRecord;
template <class T> class W8GrowableVector;

struct MonGen {
    unsigned int flags;
    signed char custom_spawn_chance; /* MIPE edits 0..100 in steps of ten */
    short custom_interval_seconds;   /* MIPE's custom "every N s" value */
    short unknown_08;                /* persisted, initialized to -1 */
    /* World position, saved as three dwords and handed to GenerateEncounter. */
    srVector3T<float> spawn_position;
    W8Item* marker_item;       /* loaded Data\Items3D\Bitmaps\mongen.itm marker */
    int encounter_table_index; /* index into g_encounter_tables, -1 means none */
    W8IntervalGate* m_pTimer;
    char name[32];
    unsigned char generation_enabled; /* MIPE "Toggle Active"; gates CanGenerate */

    MonGen();
    void Reset();
    /* Tests global encounter gates, range/LOS/occupancy constraints and this
       generator's chance. */
    bool CanGenerateEncounter(bool force);
    /* Selects and spawns the encounter table entry at `position`. */
    unsigned char GenerateEncounter(const srVector3T<float>* position);
    /* Build the candidate-entry list for the current rarity/time/party level. */
    int SelectEncounterCandidates(W8EncounterTableRuntime* table,
                                  W8GrowableVector<int>* candidates);
    /* Difficulty-aware group-size roll for one monster database record. */
    int RollEncounterGroupSize(W8MonsterRecord* record);
    /* Arms or disarms the generator, loading its marker on the way in. */
    void SetActive(unsigned char active, W8Item* node);
    /* The save pair. Both are __thiscall in the image. */
    void Save(int handle);
    unsigned char Load(int handle);
    /* The MONG chunk loader. */
    static unsigned char LoadAll(int save_handle);
    /* Moves the generator, notifying the scene when the generator has a marker. */
    void SetState(const srVector3T<float>* state);
    /* Loads the marker unconditionally, then applies the armed state. */
    void Reload(int, bool active);
    /* Strncpy into the fixed 32-byte name member. */
    void SetName(const char* name);
    /* Select one loaded encounter table and apply its HARASSMENT flag. */
    void SetEncounterTable(int index);
    ~MonGen();
};

W8_ABI_ASSERT(sizeof(MonGen) == 0x48, "MonGen_size_must_be_0x48");

MonGen* FindMonGenByName(const char* name);

#endif
