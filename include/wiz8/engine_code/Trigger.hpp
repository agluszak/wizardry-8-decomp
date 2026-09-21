#pragma once

class stLight;

#include "surrender/srMath.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/dice.h"
#include "wiz8/engine_code/game_timer.h"

struct W8Item;
class W8Prop;
class Trigger;
struct W8World;
struct W8WorldItem;

/* Timed Trigger actions are ordinary polymorphic objects owned by Trigger.cpp.
   The 0x38-byte event owns its embedded timer, but not m_pCountdown;
   its destructor at 0x004409A0 tears down only timer_008. */
class W8TriggerEvent {
public:
    W8TriggerEvent();
    virtual ~W8TriggerEvent();
    virtual void Update();

    short action_004;
    unsigned short unknown_006;
    W8GameTimer timer_008;
    W8GameTimer* m_pCountdown;
    Trigger* trigger_030;
    unsigned char repeat_034;
    unsigned char completed_035;
    unsigned char unknown_036[2];
};

static_assert(sizeof(W8TriggerEvent) == 0x38, "W8TriggerEvent_must_be_0x38");

void UpdateTimedTriggerEvents00443D30(void);

/* The common polymorphic prefix of the trigger action payload family. */
class W8TriggerActionData {
public:
    W8TriggerActionData();
    virtual ~W8TriggerActionData();

    signed char type_004;
    unsigned char unknown_005[3];
};

static_assert(sizeof(W8TriggerActionData) == 0x08, "W8TriggerActionData_must_be_0x08");

/* Type 5 installs its own final table and retains the previous environment value. */
class W8EnvironmentTriggerActionData : public W8TriggerActionData {
public:
    float previous_environment_008;
};

static_assert(sizeof(W8EnvironmentTriggerActionData) == 0x0c,
              "W8EnvironmentTriggerActionData_must_be_0x0c");

/* The level loader allocates 0x98 bytes for type 10. Its first twelve bytes
   are the common polymorphic payload above; the remaining bytes are the
   linked trigger name and optional world position read from the save. */
class W8DoorTriggerActionData : public W8TriggerActionData {
public:
    unsigned char flags_008;
    unsigned char flags_009;
    short item_00a;
    char linked_trigger_00c[0x80];
    srVector3T<float> position_08c;
};

static_assert(sizeof(W8DoorTriggerActionData) == 0x98, "W8DoorTriggerActionData_must_be_0x98");

/* Type 6 owns its string at +8. */
class W8TriggerActionData005EC158 : public W8TriggerActionData {
public:
    virtual ~W8TriggerActionData005EC158() override;
    char* owned_string_008;
};

static_assert(sizeof(W8TriggerActionData005EC158) == 0x0c,
              "W8TriggerActionData005EC158_must_be_0x0c");

#pragma pack(push, 1)
struct W8TriggerState370 {
    unsigned char state;
    unsigned char bytes_01[8];
};
#pragma pack(pop)

static_assert(sizeof(W8TriggerState370) == 9, "W8TriggerState370_must_be_9");

/* Engine Code\Trigger.cpp. Trigger is registered directly below srClass. It is
   not an srNode: the temporary table installed while srClassSupport is under
   construction has the same +0 vptr as the final Trigger table, and neither
   table contains any srNode slots. */
class Trigger : public srClassSupport<Trigger, srClass, 1, 0x10008> {
public:
    typedef bool(__cdecl* ActivationCallback)(Trigger* trigger);

    static const char* sGetClassName()
    {
        return "Trigger";
    }

    Trigger();
    virtual ~Trigger() override;
    virtual srClass* vInstance() override;

    static Trigger* CreateAndLoadLevelTrigger(int handle, W8World* world);

    bool HasActorWithinRadius(float radius, bool include_party);
    bool PlayActionSound(const char* sound_name, int volume);
    void UpdateActionAnimation();
    void CommitActionResult(bool apply_state_changes);
    void CompleteItemInteraction004447F0();
    void Activate00444750();
    bool Save0043BE60(int hFile);
    bool Load0043C1B0(int hFile, char version);
    void RunLinkedTriggers00441590();
    void SetPosition004416F0(srVector3T<float>* position);
    void FinishAction();
    void GetPosition(srVector3T<float>* position) const;
    bool CanRunLinkedTriggers();
    /* flag_0a0_17: loaded from the level record's message packed flag; gates
       the m_lData1..3 action message at the end of Run. */
    bool HasActionMessage00441780();
    /* Whether the trigger takes an item: value_23c >= 0 (the special-item
       notice path) or a type-10 action payload naming item_00a. */
    bool RequiresItem00441790();
    bool SelectAction();
    void GenerateItemGroup();
    W8WorldItem* GetOrCreateItemGroup00445670(char create);
    /* After a selected-prop Run: while g_flag_00606994 is clear, post either
       the special-item notice (value_23c != -1) or the nothing-happened notice. */
    void PrintNothingHappenedOrSpecialItemRequired004456E0(); /* 0x004456E0 */
    void RunDestination00440DD0(const char* destination);
    void Run(int source);

    int trigger_kind_018;
    char name_01c[0x80];
    int trigger_id_09c;
    unsigned int flags_0a0;
    float range_minimum_0a4;
    float range_maximum_0a8;
    int value_0ac;
    unsigned char value_0b0;
    unsigned char value_0b1;
    unsigned char value_0b2;
    unsigned char value_0b3;
    unsigned char value_0b4;
    unsigned char unknown_0b5[3];
    int value_0b8;
    int m_lData1;
    int m_lData2;
    int m_lData3;
    unsigned short value_0c8;
    unsigned char unknown_0ca[2];
    srVector3T<float> representation_vectors_0cc[4];
    float angle_0fc;
    srVector3T<float> direction_100;
    unsigned char m_bRepType;
    unsigned char unknown_10d[3];
    W8Prop* m_pProp;
    W8Item* rep_item_114;
    srVector3T<float> position_118;
    W8World* m_pWorld;
    char action_data_128[0x80];
    char alternate_action_data_1a8[0x80];
    signed char action_data_mode_228;
    signed char value_229;
    unsigned short initial_action_22a;
    unsigned short value_22c;
    unsigned short fallback_action_22e;
    unsigned short action_230;
    unsigned char action_state_232;
    unsigned char unknown_233;
    W8TriggerActionData* m_pActionData;
    char* m_pacRecipients;
    int value_23c;
    char* m_pacRequiredStates;
    char* m_pacStateToMod;
    W8TriggerEvent* m_pEvent;
    char inline_action_data_24c[0x100];
    W8WorldItem* world_item_group_34c;
    unsigned char flag_350;
    unsigned char unknown_351[3];
    /* Sampled during construction, persisted in saves and used to reseed
       item-table generation so a trigger's generated loot is repeatable. */
    unsigned int item_group_seed_354;
    int gold_358;
    int value_35c;
    ActivationCallback activation_callback_360;
    unsigned char flag_364;
    unsigned char unknown_365[3];
    int value_368;
    int value_36c;
    W8TriggerState370 state_370;
    unsigned char unknown_379[3];
    int value_37c;
    int value_380;
    int value_384;
    int value_388;
};

void InitializeStateDrivenPropVariables00445200(Trigger* trigger);
/* Re-rolls the eight pin bytes of a pickable lock (lock_state[0] == 1) and
   resets its difficulty-derived seed/state fields. lock_state is
   &Trigger::value_368. */
void __fastcall UpdateTriggerLock00445730(int* lock_state); /* 0x00445730 */
/* Ticks the lock countdown at lock_state[7]; returns 1 while a tick remained. */
unsigned char __fastcall DecrementLockTimer004457A0(int* lock_state); /* 0x004457A0 */

static_assert(sizeof(Trigger) == 0x38c, "Trigger_must_be_0x38c");

Trigger* FindTriggerByName(const char* name);
W8TriggerActionData* LoadTriggerActionData004417C0(int handle);
/* The TRES save chunk: the world's triggers, their runtime states, and their
   action data. */
int ResetNextTriggerId(void);
void SaveWorldTriggers0043C810(W8World* world, int handle);
bool LoadWorldTriggers0043C860(W8World* world, int handle);
void SaveTriggerRuntimeStates0043CB30(W8World* world, int handle, bool restoring);
bool LoadTriggerRuntimeStates0043CCF0(int handle);
void SaveTriggerActionData0043D120(W8World* world, int handle);
bool LoadTriggerActionData0043D1F0(int handle);

extern unsigned char g_flag_00606994;
extern unsigned char g_flag_0068506e;
/* Camera position cached by the per-frame trigger walk. */
extern srVector3T<float> g_trigger_camera_006599a0;
/* Trigger's action camera offset, added to the world scene position while an
   action is active, and the flag that says one is. */
extern unsigned char g_trigger_action_active_006599c8;
extern srVector3T<float> g_trigger_action_scene_offset_006599ac;
extern int g_value_0068c520;
extern int g_value_0068c53c;
extern int g_value_0068c54c;
extern int g_value_0068c548;
extern int g_value_005ee59c;
extern int g_value_005ee5a0;

bool CreateTriggerShakeEvent00444F70(int intensity, float duration, float countdown_duration,
                                     bool reverse);
bool AnyPropTriggerInView00445140(W8World* world);

void ReleaseAllTriggers(void);
void UpdateWorldTriggers00443AE0(W8World* world);
Trigger* FindTriggerForProp00443830(W8World* world, W8Prop* prop);

stLight* FindLightByName00445A10(const char* name, const srRuntimeClass* relative_to);
void DestroyAllWorldTriggers(W8World* world);
/* Walk the world's triggers for a type-0x34 RunDestination trigger whose
   annulus contains the position; answers true when one does. */
bool InsideDestinationTrigger00445940(float x, float y, float z);
