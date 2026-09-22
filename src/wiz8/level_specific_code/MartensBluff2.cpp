#include "wiz8/level_specific_code/MartensBluff2.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/location_variables.h"
#include "wiz8/sr_api.h"
#include "wiz8/string_database.h"
#include "surrender/srMath.h"
#include "random.h"

#include <math.h>

#define MARTENSBLUFF2_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MartensBluff2.cpp"

/* Level Specific Code\MartensBluff2.cpp (level 6).

   Attribution evidence: the spawn lookups at 0x004DCC5F and 0x004DDE4F pass
   this file's path string to MonsterGetIndexByLocationID, and the
   Trigger::m_bRepType assertions quote it. The level-6 block of
   InitializeLevelMasterFunctions004D6C50 registers the surrounding cluster
   (Arrowtraptrigger, Spikeballtrigger, DoorBolt, DummyLever, Dummy,
   PerfumeBox, StoneIdol, BlueFlowers, SquisherControls, DoorControls). */

// GLOBAL: WIZ8 0x00613828
unsigned char g_idol_gas_armed_613828 = 1;
// GLOBAL: WIZ8 0x0068352D
unsigned char g_crusher_excluded_flag_68352d;
// GLOBAL: WIZ8 0x0068352E
unsigned char g_crusher_active_68352e;
// GLOBAL: WIZ8 0x00683530
W8IntervalGate* g_spikeball_gate_683530;
// GLOBAL: WIZ8 0x00683534
int g_spikeball_count_683534;
// GLOBAL: WIZ8 0x00683538
W8Monster* g_crusher_excluded_683538;
// GLOBAL: WIZ8 0x0068353C
W8Prop* g_squisher3_prop_68353c;
// GLOBAL: WIZ8 0x00683540
W8Prop* g_squisher4_prop_683540;
// GLOBAL: WIZ8 0x00683544
W8Prop* g_dummy_prop_683544;
// GLOBAL: WIZ8 0x00683548
W8Prop* g_dummy_rope_prop_683548;
// GLOBAL: WIZ8 0x0068354C
stSound3D* g_crusher_sound_68354c;
// GLOBAL: WIZ8 0x00683550
int g_crusher_state_683550;
// GLOBAL: WIZ8 0x00683554
W8IntervalGate* g_idol_gas_gate_683554;

/* Level init: creates the RavenQuest location variable, fires the Dummy
   trigger when the quest has not started, re-arms the perfume box and dummy
   lever from the saved quest state, respawns the rapax with its move script
   at state 2, re-arms the side-gate text while the bolt is open and restores
   the spikeball/crusher master functions from their saved counts. */
// FUNCTION: WIZ8 0x004DCB50
void MartensBluff2Setup004DCB50(void)
{
    Trigger* pTrigger;
    W8MonsterGroup* group;
    W8MonsterInfo* info;
    srVector3T<float> position;
    int quest_state;
    int location_id;
    int value;

    pTrigger = FindTriggerByName("PerfumeBox");
    quest_state = 0;
    g_crusher_active_68352e = 0;
    if (pTrigger != 0) {
        if (GetLocationVarIDByName("RavenQuest") == -1) {
            CreateLocationVar("RavenQuest", 0);
            g_flag_006834dd = 1;
            FindTriggerByName("Dummy")->Run(-1);
            g_flag_006834dd = 0;
        } else {
            quest_state = GetLocationVarValueByName("RavenQuest");
        }
        pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        if (quest_state != 0) {
            if (quest_state == 1) {
                pTrigger->flags_0a0 |= W8_TRIGGER_ENABLED;
            }
            FindTriggerByName("DummyLever")->flags_0a0 &= ~W8_TRIGGER_ENABLED;
            if (quest_state == 2) {
                if (FindEntityByName("Ravenz", &position, 0, 0)) {
                    group = SpawnMonsters(0x183, 1, &position, 0, 1, 0, 0);
                    location_id = IListGetAt(group->monsters, 0);
                    if (location_id != 0) {
                        info = MonsterGetScriptPartByLocationIndex(
                            MonsterGetIndexByLocationID(0x1d2, MARTENSBLUFF2_CPP, location_id, 1));
                        if (info != 0 && info->monster != 0) {
                            info->monster->SetScript004C7F10("MB_MoveRapax.msf", 1);
                            SetTriggerVariableByName00444030("RavenQuest", 3);
                        }
                    }
                }
            }
        }
    }
    pTrigger = FindTriggerByName("DoorBolt");
    if (pTrigger != 0 && pTrigger->state_index == 1) {
        pTrigger = FindTriggerByName("SideGateText");
        if (pTrigger != 0) {
            pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        }
    }
    if (GetLocationVarIDByName("SpikedBallLauncher") != -1) {
        value = GetLocationVarValueByName("SpikedBallLauncher");
        if (value != 0) {
            MartensBluff2Spikeball004DD3F0(value);
        }
    }
    if (GetLocationVarIDByName("MonsterCrusher") != -1) {
        value = GetLocationVarValueByName("MonsterCrusher");
        if (value != 0) {
            MartensBluff2MonsterCrusher004DDF40(value);
        }
    }
}

/* "Arrowtraptrigger": fires a missile from each named launcher entity along
   its facing direction, rotated by the entity's yaw angle. */
// FUNCTION: WIZ8 0x004DCD40
bool TriggerArrowTrap(Trigger* pTrigger)
{
    W8SpellEffectDefinition effect;
    srVector3T<float> position;
    srVector3T<float> offset;
    srVector3T<float> direction;
    srVector3T<float> row;
    srMatrix3T<float> rotation;
    float angle;
    W8Missile* missile;

    ClearAttackBlock(&effect);
    effect.magnitude.base = 0;
    effect.magnitude.count = 2;
    effect.magnitude.sides = 6;
    if (FindEntityByName("Arrowlauncher1", &position, &angle, &direction)) {
        offset.Set(0.0, 0.0, 15000.0);
        row.Set(1.0, 0.0, 0.0);
        rotation.vectors[0] = row;
        row.Set(0.0, 1.0, 0.0);
        rotation.vectors[1] = row;
        row.Set(0.0, 0.0, 1.0);
        rotation.vectors[2] = row;
        if (angle != 0.0) {
            rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
        }
        offset = position + rotation.Transform(offset);
        missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
        missile->SetEffectDefinition(&effect);
        CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position, 1.0f,
                               15000.0f, 0);
    }
    if (FindEntityByName("Arrowlauncher2", &position, &angle, &direction)) {
        offset.Set(0.0, 0.0, 15000.0);
        row.Set(1.0, 0.0, 0.0);
        rotation.vectors[0] = row;
        row.Set(0.0, 1.0, 0.0);
        rotation.vectors[1] = row;
        row.Set(0.0, 0.0, 1.0);
        rotation.vectors[2] = row;
        if (angle != 0.0) {
            rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
        }
        offset = position + rotation.Transform(offset);
        missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
        missile->SetEffectDefinition(&effect);
        CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position, 1.0f,
                               15000.0f, 0);
    }
    if (FindEntityByName("Arrowlauncher3", &position, &angle, &direction)) {
        offset.Set(0.0, 0.0, 15000.0);
        row.Set(1.0, 0.0, 0.0);
        rotation.vectors[0] = row;
        row.Set(0.0, 1.0, 0.0);
        rotation.vectors[1] = row;
        row.Set(0.0, 0.0, 1.0);
        rotation.vectors[2] = row;
        if (angle != 0.0) {
            rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
        }
        offset = position + rotation.Transform(offset);
        missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
        missile->SetEffectDefinition(&effect);
        CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position, 1.0f,
                               15000.0f, 0);
    }
    if (FindEntityByName("Arrowlauncher4", &position, &angle, &direction)) {
        offset.Set(0.0, 0.0, 15000.0);
        rotation.SetIdentity();
        rotation.RotateAroundAxis(angle, direction);
        offset = rotation.Transform(offset) + position;
        missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
        missile->SetEffectDefinition(&effect);
        CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position, 1.0f,
                               15000.0f, 0);
    }
    return true;
}

/* "Spikeballtrigger": toggles the spikeball master function's launcher
   sequence through its toggle command. */
// FUNCTION: WIZ8 0x004DD3E0
bool MartensBluff2Spikeballtrigger004DD3E0(Trigger* pTrigger)
{
    MartensBluff2Spikeball004DD3F0(static_cast<int>(0xEFFFFFFF));
    return true;
}

/* The registered spikeball master function. A nonzero command arms it: -1
   persists the shot count, 0xEFFFFFFF toggles the sequence off or on, and any
   other small value is the starting shot count. Command 0 is the per-frame
   run; it fires the four Spikeball-launcher entities every two seconds until
   the count reaches sixteen, then unregisters and saves the reset. */
// FUNCTION: WIZ8 0x004DD3F0
void MartensBluff2Spikeball004DD3F0(int command)
{
    W8SpellEffectDefinition effect;
    srVector3T<float> position;
    srVector3T<float> offset;
    srVector3T<float> direction;
    srVector3T<float> row;
    srMatrix3T<float> rotation;
    float angle;
    W8Missile* missile;

    if (command != 0) {
        if (command == -1) {
            if (g_spikeball_count_683534 > 0xf) {
                g_spikeball_count_683534 = 0;
            }
            if (GetLocationVarIDByName("SpikedBallLauncher") == -1) {
                CreateLocationVar("SpikedBallLauncher", g_spikeball_count_683534);
            } else {
                SetTriggerVariableByName00444030("SpikedBallLauncher", g_spikeball_count_683534);
            }
            return;
        }
        if (command == static_cast<int>(0xEFFFFFFF)) {
            if (g_spikeball_count_683534 == 0 || g_spikeball_count_683534 > 0xf) {
                g_spikeball_count_683534 = 0;
            } else {
                g_spikeball_count_683534 = 1;
            }
        } else if (static_cast<unsigned int>(command) <= 0xf) {
            g_spikeball_count_683534 = command;
        }
        if (g_spikeball_gate_683530 == 0) {
            g_spikeball_gate_683530 = new W8IntervalGate(2.0f, 0, 1);
            g_master_functions_006834d8->Add(MartensBluff2Spikeball004DD3F0);
        }
    }
    g_flag_006834dc = 0;
    if (g_spikeball_count_683534 < 0x10) {
        if (g_spikeball_count_683534 == 0 || g_spikeball_gate_683530->IsFinished() ||
            (g_spikeball_gate_683530->PollElapsedIntervals(),
             g_spikeball_gate_683530->IsFinished())) {
            g_spikeball_gate_683530->Arm();
            g_spikeball_count_683534 = g_spikeball_count_683534 + 1;
            ClearAttackBlock(&effect);
            effect.magnitude.base = 0;
            effect.magnitude.count = 2;
            effect.magnitude.sides = 6;
            if (FindEntityByName("Spikeball-launcher1", &position, &angle, &direction)) {
                offset.Set(0.0, 0.0, 15000.0);
                row.Set(1.0, 0.0, 0.0);
                rotation.vectors[0] = row;
                row.Set(0.0, 1.0, 0.0);
                rotation.vectors[1] = row;
                row.Set(0.0, 0.0, 1.0);
                rotation.vectors[2] = row;
                if (angle != 0.0) {
                    rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
                }
                offset = position + rotation.Transform(offset);
                missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
                missile->SetEffectDefinition(&effect);
                CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position,
                                       1.0f, 15000.0f, 0);
            }
            if (FindEntityByName("Spikeball-launcher2", &position, &angle, &direction)) {
                offset.Set(0.0, 0.0, 15000.0);
                row.Set(1.0, 0.0, 0.0);
                rotation.vectors[0] = row;
                row.Set(0.0, 1.0, 0.0);
                rotation.vectors[1] = row;
                row.Set(0.0, 0.0, 1.0);
                rotation.vectors[2] = row;
                if (angle != 0.0) {
                    rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
                }
                offset = position + rotation.Transform(offset);
                missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
                missile->SetEffectDefinition(&effect);
                CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position,
                                       1.0f, 15000.0f, 0);
            }
            if (FindEntityByName("Spikeball-launcher3", &position, &angle, &direction)) {
                offset.Set(0.0, 0.0, 15000.0);
                row.Set(1.0, 0.0, 0.0);
                rotation.vectors[0] = row;
                row.Set(0.0, 1.0, 0.0);
                rotation.vectors[1] = row;
                row.Set(0.0, 0.0, 1.0);
                rotation.vectors[2] = row;
                if (angle != 0.0) {
                    rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
                }
                offset = position + rotation.Transform(offset);
                missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
                missile->SetEffectDefinition(&effect);
                CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position,
                                       1.0f, 15000.0f, 0);
            }
            if (FindEntityByName("Spikeball-launcher4", &position, &angle, &direction)) {
                offset.Set(0.0, 0.0, 15000.0);
                row.Set(1.0, 0.0, 0.0);
                rotation.vectors[0] = row;
                row.Set(0.0, 1.0, 0.0);
                rotation.vectors[1] = row;
                row.Set(0.0, 0.0, 1.0);
                rotation.vectors[2] = row;
                if (angle != 0.0) {
                    rotation.RotateAroundAxis(sin(angle), cos(angle), direction);
                }
                offset = rotation.Transform(offset) + position;
                missile = FireMissile004A2D30(0, &position, &offset, 0, 0, 1, 50000.0f);
                missile->SetEffectDefinition(&effect);
                CreateAndPlaySoundNode("Data\\Sound\\Combat\\Blow_Gun_Attack_01.wav", position,
                                       1.0f, 15000.0f, 0);
            }
        }
        return;
    }
    if (g_spikeball_gate_683530 != 0) {
        delete g_spikeball_gate_683530;
    }
    g_spikeball_gate_683530 = 0;
    g_spikeball_count_683534 = 0;
    if (GetLocationVarIDByName("SpikedBallLauncher") == -1) {
        CreateLocationVar("SpikedBallLauncher", g_spikeball_count_683534);
    } else {
        SetTriggerVariableByName00444030("SpikedBallLauncher", g_spikeball_count_683534);
    }
    g_flag_006834dc = 1;
}

/* "DoorBolt": re-arms the side-gate text trigger when the bolt opens. */
// FUNCTION: WIZ8 0x004DDD30
bool MartensBluff2DoorBolt004DDD30(Trigger* pTrigger)
{
    Trigger* pText = FindTriggerByName("SideGateText");
    if (pText != 0) {
        pText->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    }
    return true;
}

/* "DummyLever": re-arms itself and reports whether the RavenQuest location
   variable is still zero. */
// FUNCTION: WIZ8 0x004DDD50
bool MartensBluff2DummyLever004DDD50(Trigger* pTrigger)
{
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    return !GetLocationVarValueByName("RavenQuest");
}

/* "Dummy": re-arms the PerfumeBox trigger and marks RavenQuest stage 1. The
   shared callback flag suppresses this while the quest state is being driven
   programmatically. */
// FUNCTION: WIZ8 0x004DDD80
bool MartensBluff2Dummy004DDD80(Trigger* pTrigger)
{
    if (g_flag_006834dd == 0) {
        Trigger* pPerfumeBox = FindTriggerByName("PerfumeBox");
        if (pPerfumeBox != 0) {
            pPerfumeBox->flags_0a0 |= W8_TRIGGER_ENABLED;
            SetTriggerVariableByName00444030("RavenQuest", 1);
        }
    }
    return true;
}

/* "PerfumeBox": while item 0x2ea (the perfume) is on the cursor, consume it,
   spawn the rapax on Ravenz with its move script and post the result string.
   Always returns 0. */
// FUNCTION: WIZ8 0x004DDDC0
bool MartensBluff2PerfumeBox004DDDC0(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8MonsterGroup* group;
    W8MonsterInfo* info;
    int location_id;

    int quest_state;

    if (g_status_685170.item_in_cursor == 0) {
        return false;
    }
    if (GetItemInHand() != 0x2ea) {
        return false;
    }
    g_flag_00606994 = 1;
    ClearHeldItemDisplay();
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    quest_state = 2;
    if (FindEntityByName("Ravenz", &position, 0, 0)) {
        group = SpawnMonsters(0x183, 1, &position, 0, 1, 0, 0);
        location_id = IListGetAt(group->monsters, 0);
        if (location_id != 0) {
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x1d2, MARTENSBLUFF2_CPP, location_id, 1));
            if (info != 0 && info->monster != 0) {
                info->monster->SetScript004C7F10("MB_MoveRapax.msf", 1);
                quest_state = 3;
            }
        }
    }
    SetTriggerVariableByName00444030("RavenQuest", quest_state);
    ShowString(gppStringList[0x1c70 / 4]);
    return false;
}

/* "DoorControls": while the control trigger is off, fires the two squisher
   doors whose type-10 action data has flag 1 set. The action-data pointers
   are dereferenced unconditionally - the retail null path is preserved. */
// FUNCTION: WIZ8 0x004DDEB0
bool MartensBluff2DoorControls004DDEB0(Trigger* pTrigger)
{
    if (pTrigger->state_index == 0) {
        Trigger* pDoor = FindTriggerByName("SquisherDoor");
        W8TriggerActionData* action = pDoor->m_pActionData;
        if (action == 0 || action->type_004 != '\n') {
            action = 0;
        }
        if ((static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 1) != 0) {
            pDoor->Run(-1);
        }
        pDoor = FindTriggerByName("SquisherDoor1");
        action = pDoor->m_pActionData;
        if (action == 0 || action->type_004 != '\n') {
            action = 0;
        }
        if ((static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 1) != 0) {
            pDoor->Run(-1);
        }
    }
    return true;
}

/* "SquisherControls": toggles the monster crusher while it is idle. */
// FUNCTION: WIZ8 0x004DDF20
bool MartensBluff2SquisherControls004DDF20(Trigger* pTrigger)
{
    if (g_crusher_active_68352e == 0) {
        MartensBluff2MonsterCrusher004DDF40(static_cast<int>(0xEFFFFFFF));
        return true;
    }
    return false;
}

/* The MonsterCrusher master function. A nonzero command looks up the two
   squisher props and arms the sequence: -1 persists the state, 0xEFFFFFFF is
   the toggle from SquisherControls, and 2 suppresses the hydraulics loop. The
   command-0 run tracks the animation: while the squisher plays it sweeps the
   kill box between the two props' facing planes for monsters, shoving those
   with room aside and crushing the rest (species 0x183 pays out through
   AwardPartyExperience004EEF10); once the squisher finishes it runs the Dummy and DummyRope
   triggers, and once those props finish it unregisters. */
// FUNCTION: WIZ8 0x004DDF40
void MartensBluff2MonsterCrusher004DDF40(int command)
{
    /* The bounds box the octree query below sweeps; function-local statics,
       with the compiler's atexit destructor thunks at 0x004DE500/0x004DE510. */
    static srVector3T<float> crusher_upper;
    static srVector3T<float> crusher_lower;
    Trigger* pTrigger;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> centre;
    srVector3T<float> bounds_min;
    srVector3T<float> bounds_max;
    srVector3T<float> position;
    unsigned long* location_ids;
    unsigned int count;
    unsigned int i;
    unsigned int index;
    W8MonsterInfo* info;
    W8Monster* monster;
    float left;
    float right;
    float radius;

    if (command != 0) {
        if (command == -1) {
            if (GetLocationVarIDByName("MonsterCrusher") == -1) {
                CreateLocationVar("MonsterCrusher", g_crusher_state_683550);
            } else {
                SetTriggerVariableByName00444030("MonsterCrusher", g_crusher_state_683550);
            }
            return;
        }
        g_squisher3_prop_68353c = 0;
        g_squisher4_prop_683540 = 0;
        g_dummy_prop_683544 = 0;
        g_dummy_rope_prop_683548 = 0;
        pTrigger = FindTriggerByName("Squisher-3");
        if (pTrigger != 0) {
            if (pTrigger->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            g_squisher3_prop_68353c = pTrigger->m_pProp;
        }
        pTrigger = FindTriggerByName("Squisher-4");
        if (pTrigger != 0) {
            if (pTrigger->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            g_squisher4_prop_683540 = pTrigger->m_pProp;
        }
        if (command != static_cast<int>(0xEFFFFFFF)) {
            g_crusher_state_683550 = command;
        }
        if (g_squisher3_prop_68353c != 0 && g_squisher4_prop_683540 != 0) {
            g_squisher3_prop_68353c->PlayRepAnimation(&crusher_lower, &crusher_upper);
            g_squisher4_prop_683540->PlayRepAnimation(&lower, &upper);
            crusher_lower.y -= g_world_scale_005ebc40;
            crusher_upper.x = upper.x;
            g_crusher_excluded_683538 = 0;
            g_crusher_excluded_flag_68352d = 0;
            centre.x = (crusher_lower.x + crusher_upper.x) * g_double_005ebe80;
            centre.y = (crusher_lower.y + crusher_upper.y) * g_double_005ebe80;
            centre.z = (crusher_lower.z + crusher_upper.z) * g_double_005ebe80;
            if (command != 2) {
                g_crusher_sound_68354c = CreateAndPlaySoundNode(
                    "Data\\Sound\\Ambients\\Hydraulics Squisher Loop.wav", centre, 0.7f, 30.0f, 1);
            }
            g_crusher_active_68352e = 1;
            g_crusher_state_683550 = 1;
            g_master_functions_006834d8->Add(MartensBluff2MonsterCrusher004DDF40);
        }
        if (command != 1 && GetLocationVarValueByName("RavenQuest") != 0) {
            g_flag_006834dd = 1;
            FindTriggerByName("Dummy")->Run(-1);
            FindTriggerByName("DummyRope")->Run(-1);
            g_flag_006834dd = 0;
        }
        return;
    }
    g_flag_006834dc = 0;
    if (g_squisher3_prop_68353c == 0) {
        return;
    }
    if (g_squisher4_prop_683540 == 0) {
        return;
    }
    if (g_dummy_prop_683544 != 0) {
        if (g_dummy_prop_683544->Rep()->animation_playing_06d != 0) {
            return;
        }
        if (g_dummy_rope_prop_683548->Rep()->animation_playing_06d != 0) {
            return;
        }
        g_flag_006834dc = 1;
        g_crusher_active_68352e = 0;
        g_crusher_state_683550 = 0;
        return;
    }
    if (g_squisher3_prop_68353c->Rep()->animation_playing_06d == 0) {
        if (g_crusher_sound_68354c != 0) {
            g_crusher_sound_68354c->Stop();
        }
        g_crusher_sound_68354c = 0;
        pTrigger = FindTriggerByName("Dummy");
        if (GetLocationVarValueByName("RavenQuest") == 0) {
            g_flag_006834dc = 1;
            g_crusher_active_68352e = 0;
            g_crusher_state_683550 = 0;
            return;
        }
        g_flag_006834dd = 1;
        pTrigger->Run(-1);
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        g_dummy_prop_683544 = pTrigger->m_pProp;
        pTrigger = FindTriggerByName("DummyRope");
        pTrigger->Run(-1);
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        g_dummy_rope_prop_683548 = pTrigger->m_pProp;
        g_flag_006834dd = 0;
        g_crusher_state_683550 = 2;
        return;
    }
    g_squisher3_prop_68353c->m_gd_prop->ComputeBounds004B7500(&bounds_min, &bounds_max);
    left = bounds_max.x;
    g_squisher4_prop_683540->m_gd_prop->ComputeBounds004B7500(&bounds_min, &bounds_max);
    right = bounds_min.x;
    location_ids = 0;
    count = g_octree_6598a4->QueryLocationsInBox(&location_ids, &crusher_lower, &crusher_upper, 0);
    if (count == 0) {
        return;
    }
    for (i = 0; i < count; i++) {
        index = MonsterGetIndexByLocationID(0x28d, MARTENSBLUFF2_CPP, location_ids[i], 1);
        info = MonsterGetScriptPartByLocationIndex(index);
        if (info != 0 && info->monster != 0 &&
            (g_crusher_excluded_flag_68352d == 0 || info->monster != g_crusher_excluded_683538) &&
            info->monster->flags_00c != 0x200000) {
            monster = info->monster;
            position = monster->GetPosition();
            radius = monster->movement_0c0.alternate_radius_0b4;
            if (left <= position.x - radius) {
                if (right < position.x + radius) {
                    position.x = right - radius;
                    monster->SetPositionInternal00453590(&position);
                }
            } else if (position.x + radius <= right || Random(100) < 0x24) {
                position.x = left + radius;
                monster->SetPositionInternal00453590(&position);
            } else {
                MonsterStartsDying(info, 1);
                if (info->monster_species == 0x183) {
                    AwardPartyExperience004EEF10(10000, 0);
                }
            }
        }
    }
}

// SYNTHETIC: WIZ8 0x004DE500
// `dynamic atexit destructor for 'crusher_lower''
// SYNTHETIC: WIZ8 0x004DE510
// `dynamic atexit destructor for 'crusher_upper''

/* "StoneIdol": while the cursor is free, puts item 0x291 in hand, posts the
   text, activates the IdolGas particle and arms the IdolGas master function. */
// FUNCTION: WIZ8 0x004DE520
bool MartensBluff2StoneIdol004DE520(Trigger* pTrigger)
{
    stParticle* particle;

    if (g_status_685170.item_in_cursor != 0) {
        return false;
    }
    ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, 0x291, 0, 0, 0);
    SetItemCursor(0);
    ShowString(gppStringList[0x1c74 / 4]);
    particle = FindRegisteredParticle0049ADB0("IdolGas");
    if (particle != 0) {
        particle->SetActive(1);
    }
    BeginSurprise005025F0();
    MartensBluff2IdolGas004DE660(1);
    g_master_functions_006834d8->Add(MartensBluff2IdolGas004DE660);
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    g_flag_006834dd = 1;
    SetFact(0x323, 1, 0);
    return true;
}

/* "BlueFlowers": while the cursor is free, puts item 0x2eb in hand. The
   shared callback flag is cleared on every run. */
// FUNCTION: WIZ8 0x004DE620
bool MartensBluff2BlueFlowers004DE620(Trigger* pTrigger)
{
    if (g_flag_006834dd == 0) {
        if (g_status_685170.item_in_cursor != 0) {
            return false;
        }
        ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, 0x2eb, 0, 0, 0);
        SetItemCursor(0);
    }
    g_flag_006834dd = 0;
    return true;
}

/* The IdolGas master function. A nonzero command arms it: fades the world
   lighting out and starts the four-second gate. The command-0 run waits on
   the gate, then fades back in, picks a party member to suffer the gas when
   no kind-0x1e NPC is around and stops the particle; the following run
   unregisters. */
// FUNCTION: WIZ8 0x004DE660
void MartensBluff2IdolGas004DE660(int command)
{
    stParticle* particle;

    if (command != 0) {
        g_idol_gas_armed_613828 = 1;
        BeginWorldLightingFade(-3000.0f);
        if (g_idol_gas_gate_683554 != 0) {
            g_idol_gas_gate_683554->Arm();
            return;
        }
        g_idol_gas_gate_683554 = new W8IntervalGate(4.0f, 0, 1);
        return;
    }
    g_flag_006834dc = 0;
    if (g_idol_gas_armed_613828 == 0) {
        g_flag_006834dc = 1;
        if (g_idol_gas_gate_683554 != 0) {
            delete g_idol_gas_gate_683554;
        }
        g_idol_gas_gate_683554 = 0;
        g_idol_gas_armed_613828 = 1;
        ResolveSurpriseWake005029E0();
        return;
    }
    if (!g_idol_gas_gate_683554->IsFinished()) {
        g_idol_gas_gate_683554->PollElapsedIntervals();
        if (!g_idol_gas_gate_683554->IsFinished()) {
            return;
        }
    }
    g_idol_gas_gate_683554->Arm();
    g_idol_gas_armed_613828 = 0;
    if (FindNpcOfKind(0x1e) == 0) {
        MartensBluff2IdolGasVictim004DE7D0();
    }
    BeginWorldLightingFade(3000.0f);
    particle = FindRegisteredParticle0049ADB0("IdolGas");
    if (particle != 0) {
        particle->SetActive(0);
    }
}

/* The IdolGas victim picker: scores the eight party slots by profession
   (fighters 2, rogues 3, priests 5, mages 4, all others 1; empty rows and the
   first two slots sit out at 10), picks a random least-affected member, and
   falls back to the first dead member when nothing scored - a retail
   unreachable path, since a nonzero lowest score implies a candidate. The
   winner gets condition 0x13 indefinitely and fact 0x33 records the event. */
// FUNCTION: WIZ8 0x004DE7D0
void MartensBluff2IdolGasVictim004DE7D0(void)
{
    unsigned int severities[8];
    unsigned int lowest;
    unsigned int slot;
    unsigned int count;
    unsigned int pick;
    int i;

    lowest = 10;
    for (slot = 0; slot < 8; slot++) {
        if (g_status_685170.buffers.XChar[slot].fOccupied == 0 || slot < 2) {
            severities[slot] = 10;
        } else {
            switch (g_status_685170.buffers.Char[slot].iProfession) {
            case W8_PROFESSION_FIGHTER:
                severities[slot] = 2;
                break;
            case W8_PROFESSION_ROGUE:
                severities[slot] = 3;
                break;
            case W8_PROFESSION_PRIEST:
                severities[slot] = 5;
                break;
            case W8_PROFESSION_MAGE:
                severities[slot] = 4;
                break;
            default:
                severities[slot] = 1;
                break;
            }
        }
        if (severities[slot] < lowest) {
            lowest = severities[slot];
        }
    }
    if (lowest != 10) {
        count = 0;
        for (i = 0; i < 8; i++) {
            if (severities[i] == lowest) {
                count++;
            }
        }
        if (count == 0) {
            slot = 0;
            while (g_status_685170.buffers.XChar[slot].fOccupied == 0 ||
                   g_status_685170.buffers.Char[slot].highest_condition != W8_CONDITION_DEAD) {
                slot++;
                if (slot > 7) {
                    return;
                }
            }
        } else {
            pick = Random(count) + 1;
            for (slot = 0; slot < 8; slot++) {
                if (severities[slot] == lowest) {
                    pick--;
                    if (pick == 0) {
                        break;
                    }
                }
            }
        }
        g_status_685170.party_slot_249c = slot;
        SetCharacterCondition(slot, 0x13, W8_CONDITION_INDEFINITE, 0, 0, 1);
        SetFact(0x33, 1, 0);
    }
}

/* srMatrix3T<float>::RotateAroundAxis(double, ...) emitted out-of-line for the
   arrow trap's rotation math; the primary is in srMath.h. */
// TEMPLATE: WIZ8 0x004DE940
// srMatrix3T<float>::RotateAroundAxis(double, const srVector3T<float>&) (MartensBluff2.cpp emission)
