#pragma once

class stLight;

#include "surrender/srMath.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/dice.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/integer_constants.h"

struct W8Item;
class W8Prop;
class Trigger;
struct W8World;
struct W8WorldItem;

/* Timed Trigger actions are ordinary polymorphic objects owned by Trigger.cpp.
   The 0x38-byte event owns its embedded timer, but not m_pCountdown;
   its destructor at 0x004409A0 tears down only timer. */
class W8TriggerEvent {
public:
    W8TriggerEvent();
    virtual ~W8TriggerEvent();
    virtual void Update();

    short action;
    unsigned short unknown_006;
    W8GameTimer timer;
    W8GameTimer* m_pCountdown;
    Trigger* trigger;
    bool repeat;
    bool completed;
};

static_assert(sizeof(W8TriggerEvent) == 0x38, "W8TriggerEvent_must_be_0x38");

void UpdateTimedTriggerEvents(void);

/* Payload tags remain signed bytes in the common prefix and on disk. */
enum W8TriggerPayloadKind {
    W8_TRIGGER_PAYLOAD_NONE = -1,
    W8_TRIGGER_PAYLOAD_ENVIRONMENT = 5,
    W8_TRIGGER_PAYLOAD_STRING = 6,
    W8_TRIGGER_PAYLOAD_DOOR = 10
};

/* Only independently identified door bits are named; the other serialized
   bits retain their original numeric values. */
enum W8DoorTriggerFlag { W8_DOOR_OPEN = 1, W8_DOOR_KEY_REQUIRED = 4 };

/* The common polymorphic prefix of the trigger action payload family. */
class W8TriggerActionData {
public:
    W8TriggerActionData();
    virtual ~W8TriggerActionData();

    signed char type; /* W8TriggerPayloadKind */
};

static_assert(sizeof(W8TriggerActionData) == 0x08, "W8TriggerActionData_must_be_0x08");

/* Type 5 installs its own final table and retains the previous environment value. */
class W8EnvironmentTriggerActionData : public W8TriggerActionData {
public:
    float previous_environment;
};

static_assert(sizeof(W8EnvironmentTriggerActionData) == 0x0c,
              "W8EnvironmentTriggerActionData_must_be_0x0c");

/* The level loader allocates 0x98 bytes for type 10. Its first twelve bytes
   are the common polymorphic payload above; the remaining bytes are the
   linked trigger name and optional world position read from the save. */
class W8DoorTriggerActionData : public W8TriggerActionData {
public:
    unsigned char door_flags;
    unsigned char extra_flags;
    short item;
    char linked_trigger[0x80];
    srVector3T<float> position;
};

static_assert(sizeof(W8DoorTriggerActionData) == 0x98, "W8DoorTriggerActionData_must_be_0x98");

/* Type 6 (built for action 17) owns a copy of the trigger's action string. */
class W8StringTriggerActionData : public W8TriggerActionData {
public:
    virtual ~W8StringTriggerActionData() override;
    char* owned_string;
};

static_assert(sizeof(W8StringTriggerActionData) == 0x0c, "W8StringTriggerActionData_must_be_0x0c");

/* The flags bits whose roles are established by recovered producers and
   consumers:
   - ON arms a trigger plane / state-driven prop; scripts toggle it, mipe uses
     it for the rep item's highlight state.
   - RUNNING marks an in-flight action; FinishAction clears it.
   - FIRE_LINKED enables linked-recipient dispatch from CommitActionResult or
     RunLinkedTriggers; LINK_ON_DEACTIVATE selects the latter path.
   - ENABLED makes the trigger interactable (picking, prop activation, door
     pathing); scripts clear it to retire spent levers and triggers.
   - POSITIONED records that position is live (kind-2 record or
     SetPosition); the proximity scan requires it.
   - EXCLUSIVE lets at most one flagged proximity trigger run per update scan.
   - CAN_RUN_LINKED is the bit CanRunLinkedTriggers reports.
   - REACTIVATE_LINKED re-fires linked recipients when a finished trigger
     reactivates.
   - ALTERNATE_ACTION alternates action_data with
     alternate_action_data, tracked by ALTERNATE_SELECTED.
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
   lock, re-rolled by W8LockState::Reset. */
struct W8TriggerDeviceState {
    unsigned char completed;
    unsigned char pins[8];
};

static_assert(sizeof(W8TriggerDeviceState) == 9, "W8TriggerDeviceState_must_be_9");

/* The locks & traps device record embedded at the tail of Trigger. `lock_type` is the editor "Type" (0 none,
   1 pickable lock, 2 trap, 3 key lock); `difficulty` is the editor "Difficulty"
   grade — it doubles as the pickable lock's pin budget; `device_id` indexes
   the tumbler/trap tables (-1 = roll on first use); `lock_countdown` ticks the
   pick interaction (pins remaining, difficulty * 3); `last_interaction_clock`
   is the world clock of the last attempt (-1 = never). */
struct W8LockState {
    /* Re-rolls the eight pin bytes of a pickable lock (lock_type == 1) and
       resets its difficulty-derived countdown and interaction state. */
    void Reset(); /* 0x00445730 */
    void ReadRuntimeRecord(int handle, int version, int restoring);
    /* Spends one point of the lock countdown and reports whether one
       remained to spend. */
    bool ConsumeCountdown(); /* 0x004457A0 */

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
enum W8TriggerRepresentationKind {
    W8_TRIGGER_REP_NONE = 0,
    W8_TRIGGER_REP_ITEM = 1,
    W8_TRIGGER_REP_PROP = 2,
    W8_TRIGGER_REP_POSITION = 3
};

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

    W8Prop* GetProp() const
    {
        if (m_bRepType != W8_TRIGGER_REP_PROP) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        return m_pProp;
    }

    bool HasActorWithinRadius(float radius, bool include_party);
    bool PlayActionSound(const char* sound_name, int volume);
    void UpdateActionAnimation();
    void CommitActionResult(bool apply_state_changes);
    void CompleteItemInteraction();
    void Activate();
    bool Save(int hFile);
    bool Load(int hFile, char version);
    void RunLinkedTriggers();
    void SetPosition(srVector3T<float>* position);
    void FinishAction();
    void GetPosition(srVector3T<float>* position) const;
    bool CanRunLinkedTriggers();
    /* flags: loaded from the level record's message packed flag; gates
       the m_lData1..3 action message at the end of Run. */
    bool HasActionMessage();
    /* Whether the trigger takes an item: required_item_id >= 0 (the special-item
       notice path) or a type-10 action payload naming item. */
    bool RequiresItem();
    bool SelectAction();
    void GenerateItemGroup();
    W8WorldItem* GetOrCreateItemGroup(bool create);
    /* After a selected-prop Run: while g_trigger_feedback is clear, post either
       the special-item notice (required_item_id != -1) or the nothing-happened notice. */
    void PrintNothingHappenedOrSpecialItemRequired(); /* 0x004456E0 */
    void RunDestination(const char* destination);
    void Run(int source);

    int trigger_kind;
    char name[0x80];
    int trigger_id;
    /* W8TriggerFlag bits with established producer/consumer semantics. The
       rest of the word is record-loaded or unresolved state and stays masked
       by literal. */
    unsigned int flags;
    float range_minimum;
    float range_maximum;
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
    srVector3T<float> representation_vectors[4];
    float angle;
    srVector3T<float> direction;
    unsigned char m_bRepType;
    unsigned char unknown_10d[3];
    W8Prop* m_pProp;
    W8Item* rep_item;
    srVector3T<float> position;
    W8World* m_pWorld;
    char action_data[0x80];
    char alternate_action_data[0x80];
    signed char action_data_mode;
    signed char sound_volume;
    unsigned short initial_action;
    unsigned short alternate_action;
    unsigned short fallback_action;
    unsigned short action;
    unsigned char action_state;
    unsigned char unknown_233;
    W8TriggerActionData* m_pActionData;
    char* m_pacRecipients;
    int required_item_id;
    char* m_pacRequiredStates;
    char* m_pacStateToMod;
    W8TriggerEvent* m_pEvent;
    char inline_action_data[0x100];
    W8WorldItem* world_item_group;
    unsigned char items_generated;
    unsigned char unknown_351[3];
    /* Sampled during construction, persisted in saves and used to reseed
       item-table generation so a trigger's generated loot is repeatable. */
    unsigned int item_group_seed;
    int gold;
    int uses_remaining;
    ActivationCallback activation_callback;
    bool running;
    unsigned char unknown_365[3];
    W8LockState lock_state;
};

void InitializeStateDrivenPropVariables(Trigger* trigger);

static_assert(sizeof(Trigger) == 0x38c, "Trigger_must_be_0x38c");

Trigger* FindTriggerByName(const char* name);

inline void RunNamedTrigger(const char* name, int source)
{
    Trigger* trigger = FindTriggerByName(name);
    if (trigger != 0) {
        trigger->Run(source);
    }
}

W8TriggerActionData* ReadDoorTriggerActionData(int handle);
/* The TRES save chunk: the world's triggers, their runtime states, and their
   action data. */
int ResetNextTriggerId(void);
void SaveWorldTriggers(W8World* world, int handle);
bool LoadWorldTriggers(W8World* world, int handle);
void SaveTriggerRuntimeStates(W8World* world, int handle, bool restoring);
bool LoadTriggerRuntimeStates(int handle);
void SaveTriggerActionData(W8World* world, int handle);
bool LoadTriggerActionData(int handle);

extern bool g_trigger_feedback;
/* Camera position cached by the per-frame trigger walk. */
extern srVector3T<float> g_trigger_camera;
/* Trigger's action camera offset, added to the world scene position while an
   action is active, and the flag that says one is. */
extern bool g_trigger_action_active;
extern srVector3T<float> g_trigger_action_scene_offset;
extern int g_container_event_alt;
extern int g_trap_notice_event;
extern int g_lock_notice_event;
extern int g_container_event;

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
