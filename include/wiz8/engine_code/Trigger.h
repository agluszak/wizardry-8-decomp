#pragma once

#include "surrender/srMath.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/layouts/gameplay_databases.h"

class W8Prop;
class Trigger;
struct W8World;
struct W8WorldItem;

/* Timed Trigger actions are ordinary polymorphic objects owned by Trigger.cpp.
   The 0x38-byte event owns its embedded timer, but not auxiliary_timer_02c;
   its destructor at 0x004409A0 tears down only timer_008. */
class W8TriggerEvent {
public:
    W8TriggerEvent();
    virtual ~W8TriggerEvent();
    virtual void Update();

    short action_004;
    unsigned short unknown_006;
    W8GameTimer timer_008;
    W8GameTimer* auxiliary_timer_02c;
    Trigger* trigger_030;
    unsigned char repeat_034;
    unsigned char completed_035;
    unsigned char unknown_036[2];
};

static_assert(sizeof(W8TriggerEvent) == 0x38,
              "W8TriggerEvent_must_be_0x38");

/* Trigger's m_pActionData points at this polymorphic payload. The concrete
   object created by Trigger::Run has a distinct final vtable at 0x005EC148
   and returns to the base table at 0x005EC138 during destruction. The payload
   is shared by multiple actions: type 5 stores the previous environment value
   as a float, while type 10 uses the flag/item view. */
class W8TriggerActionData {
public:
    W8TriggerActionData();
    virtual ~W8TriggerActionData();

    signed char type_004;
    unsigned char unknown_005[3];
    union {
        W8Dice value_008;
        float float_value_008;
        char* owned_string_008;
        struct {
            unsigned char flags_008;
            unsigned char flags_009;
            short item_00a;
        };
    };
};

static_assert(sizeof(W8TriggerActionData) == 0x0c,
              "W8TriggerActionData_must_be_0x0c");

class W8TriggerActionData005EC148 : public W8TriggerActionData {
};

static_assert(sizeof(W8TriggerActionData005EC148) == 0x0c,
              "W8TriggerActionData005EC148_must_be_0x0c");

/* The level loader allocates 0x98 bytes for type 10. Its first twelve bytes
   are the common polymorphic payload above; the remaining bytes are the
   linked trigger name and optional world position read from the save. */
class W8TriggerActionData005EC134 : public W8TriggerActionData {
public:
    char linked_trigger_00c[0x80];
    srVector3T<float> position_08c;
};

static_assert(sizeof(W8TriggerActionData005EC134) == 0x98,
              "W8TriggerActionData005EC134_must_be_0x98");

/* Type 6 owns the string stored in the common payload's +8 union. */
class W8TriggerActionData005EC158 : public W8TriggerActionData {
public:
    virtual ~W8TriggerActionData005EC158() override;
};

static_assert(sizeof(W8TriggerActionData005EC158) == 0x0c,
              "W8TriggerActionData005EC158_must_be_0x0c");

#pragma pack(push, 1)
struct W8TriggerState370 {
    unsigned char state;
    int value_01;
    int value_05;
};
#pragma pack(pop)

static_assert(sizeof(W8TriggerState370) == 9,
              "W8TriggerState370_must_be_9");

/* Engine Code\Trigger.cpp. Trigger is registered directly below srClass. It is
   not an srNode: the temporary table installed while srClassSupport is under
   construction has the same +0 vptr as the final Trigger table, and neither
   table contains any srNode slots. */
class Trigger : public srClassSupport<Trigger, srClass, 1, 0x10008> {
public:
    typedef unsigned char (__cdecl *ActivationCallback)(Trigger* trigger);

    static const char* sGetClassName() { return "Trigger"; }

    Trigger();
    virtual ~Trigger() override;
    virtual srClass* vInstance() override;

    static Trigger* CreateAndLoadLevelTrigger(int handle, W8World* world);

    unsigned char HasActorWithinRadius(float radius, unsigned char include_party);
    unsigned char PlayActionSound(const char* sound_name, int volume);
    void UpdateActionAnimation();
    void CommitActionResult(unsigned char apply_state_changes);
    void CompleteItemInteraction004447F0();
    void FinishAction();
    void GetPosition(srVector3T<float>* position) const;
    unsigned char CanRunLinkedTriggers();
    unsigned char SelectAction();
    void GenerateItemGroup();
    void RunDestination00440DD0(const char* destination);
    void Run(int source);

    int trigger_kind_018;
    unsigned char state_01c;
    unsigned char unknown_01d[0x7f];
    int trigger_id_09c;
    union {
        unsigned int flags_0a0;
        struct {
            unsigned int flag_0a0_00 : 1;
            unsigned int flag_0a0_01 : 1;
            unsigned int flag_0a0_02 : 1;
            unsigned int flag_0a0_03 : 1;
            unsigned int flag_0a0_04 : 1;
            unsigned int flag_0a0_05 : 1;
            unsigned int flag_0a0_06 : 1;
            unsigned int flag_0a0_07 : 1;
            unsigned int flag_0a0_08 : 1;
            unsigned int flag_0a0_09 : 1;
            unsigned int flag_0a0_10 : 1;
            unsigned int flag_0a0_11 : 1;
            unsigned int flag_0a0_12 : 1;
            unsigned int flag_0a0_13 : 1;
            unsigned int flag_0a0_14 : 1;
            unsigned int flag_0a0_15 : 1;
            unsigned int flag_0a0_16 : 1;
            unsigned int flag_0a0_17 : 1;
            unsigned int flag_0a0_18 : 1;
            unsigned int flag_0a0_19 : 1;
            unsigned int flag_0a0_20 : 1;
            unsigned int flag_0a0_21 : 1;
            unsigned int flag_0a0_22 : 1;
            unsigned int flag_0a0_23 : 1;
            unsigned int flag_0a0_24 : 1;
            unsigned int flag_0a0_25 : 1;
            unsigned int flag_0a0_26 : 1;
            unsigned int flag_0a0_27 : 1;
            unsigned int flag_0a0_28 : 1;
            unsigned int flag_0a0_29 : 1;
            unsigned int flag_0a0_30 : 1;
            unsigned int flag_0a0_31 : 1;
        };
    };
    int value_0a4;
    int value_0a8;
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
    unsigned char unknown_0ca[0x32];
    float angle_0fc;
    float value_100;
    float value_104;
    float value_108;
    unsigned char m_bRepType;
    unsigned char unknown_10d[3];
    W8Prop* m_pProp;
    void* value_114;
    float position_118;
    float position_11c;
    float position_120;
    W8World* m_pWorld;
    unsigned char action_data_128[0x80];
    unsigned char alternate_action_data_1a8[0x80];
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
    unsigned int next_activation_time_354;
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

static_assert(sizeof(Trigger) == 0x38c, "Trigger_must_be_0x38c");

Trigger* FindTriggerByName(const char* name);
