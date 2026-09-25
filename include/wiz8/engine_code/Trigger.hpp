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
    bool completed_035;
    unsigned char unknown_036[2];
};

static_assert(sizeof(W8TriggerEvent) == 0x38, "W8TriggerEvent_must_be_0x38");

void UpdateTimedTriggerEvents(void);

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

/* The flags_0a0 bits whose roles are established by recovered producers and
   consumers:
   - ON arms a trigger plane / state-driven prop; scripts toggle it, mipe uses
     it for the rep item's highlight state.
   - RUNNING marks an in-flight action; FinishAction clears it.
   - FIRE_LINKED enables linked-recipient dispatch from CommitActionResult or
     RunLinkedTriggers; LINK_ON_DEACTIVATE selects the latter path.
   - ENABLED makes the trigger interactable (picking, prop activation, door
     pathing); scripts clear it to retire spent levers and triggers.
   - POSITIONED records that position_118 is live (kind-2 record or
     SetPosition); the proximity scan requires it.
   - EXCLUSIVE lets at most one flagged proximity trigger run per update scan.
   - CAN_RUN_LINKED is the bit CanRunLinkedTriggers reports.
   - REACTIVATE_LINKED re-fires linked recipients when a finished trigger
     reactivates.
   - ALTERNATE_ACTION alternates action_data_128 with
     alternate_action_data_1a8, tracked by ALTERNATE_SELECTED.
   - ITEM_PICKER marks the item-picker dialog open for this trigger.
   - SEARCHED marks an already-searched trigger; loading unregisters it.
   - ANIMATE_STATES animates the prop through its states as the state index
     cycles; ANIMATE_ACTION lets UpdateActionAnimation run.
   - PLANE marks a trigger registered with AddTriggerPlane; the proximity scan
     skips it.
   - KEEP_ON_FINISH stops FinishAction undoing the action on the recipients.
   - HAS_ALTERNATE is set when the record has an alternate action; SelectAction
     then raises USE_ALTERNATE after the initial action, and ALTERNATE_TOGGLES
     drops it again after the alternate one.
   - CONSUME_ITEM removes the required item from the party when it is used.
   - ONCE triggers refuse SelectAction once the action has set FIRED. */
enum W8TriggerFlag {
    W8_TRIGGER_ANIMATE_STATES = 0x1,
    W8_TRIGGER_ANIMATE_ACTION = 0x2,
    W8_TRIGGER_PLANE = 0x4,
    W8_TRIGGER_KEEP_ON_FINISH = 0x8,
    W8_TRIGGER_ON = 0x10,
    W8_TRIGGER_RUNNING = 0x40,
    W8_TRIGGER_FIRE_LINKED = 0x80,
    W8_TRIGGER_ENABLED = 0x100,
    W8_TRIGGER_LINK_ON_DEACTIVATE = 0x200,
    W8_TRIGGER_POSITIONED = 0x800,
    W8_TRIGGER_HAS_ALTERNATE = 0x2000,
    W8_TRIGGER_USE_ALTERNATE = 0x4000,
    W8_TRIGGER_ALTERNATE_TOGGLES = 0x8000,
    W8_TRIGGER_CONSUME_ITEM = 0x10000,
    W8_TRIGGER_CAN_RUN_LINKED = 0x20000,
    W8_TRIGGER_ONCE = 0x40000,
    W8_TRIGGER_FIRED = 0x80000,
    W8_TRIGGER_EXCLUSIVE = 0x100000,
    W8_TRIGGER_REACTIVATE_LINKED = 0x200000,
    W8_TRIGGER_ALTERNATE_ACTION = 0x800000,
    W8_TRIGGER_ALTERNATE_SELECTED = 0x1000000,
    W8_TRIGGER_ITEM_PICKER = 0x2000000,
    W8_TRIGGER_SEARCHED = 0x4000000,
};

/* Persisted lock/trap device state: `completed` latches once the pick/disarm
   interaction finishes; `pins` holds the eight tumbler bytes of a pickable
   lock, re-rolled by UpdateTriggerLock00445730. */
struct W8TriggerDeviceState {
    unsigned char completed;
    unsigned char pins[8];
};

static_assert(sizeof(W8TriggerDeviceState) == 9, "W8TriggerDeviceState_must_be_9");

/* The locks & traps device record embedded at the tail of Trigger. This is
   the block `UpdateTriggerLock00445730`/`ConsumeLockQuality004457A0` take by
   pointer (`&trigger->lock_state`). `lock_type` is the editor "Type" (0 none,
   1 pickable lock, 2 trap, 3 key lock); `difficulty` is the editor "Difficulty"
   grade — it doubles as the pickable lock's pin budget; `device_id` indexes
   the tumbler/trap tables (-1 = roll on first use); `lock_countdown` ticks the
   pick interaction (pins remaining, difficulty * 3); `last_interaction_clock`
   is the world clock of the last attempt (-1 = never). */
struct W8LockState {
    int lock_type;
    int difficulty;
    W8TriggerDeviceState device_state;
    unsigned char unknown_011[3];
    int device_id;
    int key_id;
    int lock_countdown;
    int last_interaction_clock;
};

static_assert(sizeof(W8LockState) == 0x24, "W8LockState_must_be_0x24");

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
    void CompleteItemInteraction();
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
    bool HasActionMessage();
    /* Whether the trigger takes an item: required_item_id >= 0 (the special-item
       notice path) or a type-10 action payload naming item_00a. */
    bool RequiresItem();
    bool SelectAction();
    void GenerateItemGroup();
    W8WorldItem* GetOrCreateItemGroup(char create);
    /* After a selected-prop Run: while g_trigger_feedback_00606994 is clear, post either
       the special-item notice (required_item_id != -1) or the nothing-happened notice. */
    void PrintNothingHappenedOrSpecialItemRequired(); /* 0x004456E0 */
    void RunDestination00440DD0(const char* destination);
    void Run(int source);

    int trigger_kind_018;
    char name_01c[0x80];
    int trigger_id_09c;
    /* W8TriggerFlag bits with established producer/consumer semantics. The
       rest of the word is record-loaded or unresolved state and stays masked
       by literal. */
    unsigned int flags_0a0;
    float range_minimum_0a4;
    float range_maximum_0a8;
    int action_value;
    unsigned char state_count;
    unsigned char state_index;
    unsigned char state_direction;
    unsigned char cycle_bounce;
    unsigned char state_mod_mode;
    unsigned char unknown_0b5[3];
    int surface_id;
    int m_lData1;
    int m_lData2;
    int m_lData3;
    unsigned short searchable;
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
    signed char sound_volume;
    unsigned short initial_action_22a;
    unsigned short alternate_action;
    unsigned short fallback_action_22e;
    unsigned short action_230;
    unsigned char action_state_232;
    unsigned char unknown_233;
    W8TriggerActionData* m_pActionData;
    char* m_pacRecipients;
    int required_item_id;
    char* m_pacRequiredStates;
    char* m_pacStateToMod;
    W8TriggerEvent* m_pEvent;
    char inline_action_data_24c[0x100];
    W8WorldItem* world_item_group_34c;
    unsigned char items_generated;
    unsigned char unknown_351[3];
    /* Sampled during construction, persisted in saves and used to reseed
       item-table generation so a trigger's generated loot is repeatable. */
    unsigned int item_group_seed_354;
    int gold_358;
    int uses_remaining;
    ActivationCallback activation_callback_360;
    bool running;
    unsigned char unknown_365[3];
    /* Locks & traps device block — the record UpdateTriggerLock00445730/
       ConsumeLockQuality004457A0 take by pointer. */
    W8LockState lock_state;
};

void InitializeStateDrivenPropVariables(Trigger* trigger);
/* Re-rolls the eight pin bytes of a pickable lock (lock_type == 1) and
   resets its difficulty-derived seed/state fields. */
void __fastcall UpdateTriggerLock00445730(W8LockState* lock_state); /* 0x00445730 */
/* Spends one point of the lock's quality budget (lock_countdown) and reports
   whether one remained to spend. */
unsigned char __fastcall ConsumeLockQuality004457A0(W8LockState* lock_state); /* 0x004457A0 */

static_assert(sizeof(Trigger) == 0x38c, "Trigger_must_be_0x38c");

Trigger* FindTriggerByName(const char* name);
W8TriggerActionData* LoadTriggerActionData004417C0(int handle);
/* The TRES save chunk: the world's triggers, their runtime states, and their
   action data. */
int ResetNextTriggerId(void);
void SaveWorldTriggers(W8World* world, int handle);
bool LoadWorldTriggers(W8World* world, int handle);
void SaveTriggerRuntimeStates(W8World* world, int handle, bool restoring);
bool LoadTriggerRuntimeStates(int handle);
void SaveTriggerActionData(W8World* world, int handle);
bool LoadTriggerActionData0043D1F0(int handle);

extern unsigned char g_trigger_feedback_00606994;
extern unsigned char g_flag_0068506e;
/* Camera position cached by the per-frame trigger walk. */
extern srVector3T<float> g_trigger_camera_006599a0;
/* Trigger's action camera offset, added to the world scene position while an
   action is active, and the flag that says one is. */
extern bool g_trigger_action_active_006599c8;
extern srVector3T<float> g_trigger_action_scene_offset_006599ac;
extern int g_container_event_alt_0068c520;
extern int g_trap_notice_event_0068c53c;
extern int g_lock_notice_event_0068c54c;
extern int g_container_event_0068c548;
extern int g_condition_reaction_005ee59c;
extern int g_condition_reaction_alt_005ee5a0;

bool CreateTriggerShakeEvent(int intensity, float duration, float countdown_duration, bool reverse);
bool AnyPropTriggerInView(W8World* world);

void ReleaseAllTriggers(void);
void UpdateWorldTriggers(W8World* world);
Trigger* FindTriggerForProp(W8World* world, W8Prop* prop);

stLight* FindLightByName(const char* name, const srRuntimeClass* relative_to);
void DestroyAllWorldTriggers(W8World* world);
/* Walk the world's triggers for a type-0x34 RunDestination trigger whose
   annulus contains the position; answers true when one does. */
bool InsideDestinationTrigger(float x, float y, float z);
