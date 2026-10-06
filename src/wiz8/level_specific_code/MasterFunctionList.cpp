#include <stdarg.h>
#include "wiz8/integer_constants.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/level_specific_code/Arnika.h"
#include "wiz8/level_specific_code/Monastery1.h"
#include "wiz8/level_specific_code/Monastery2.h"
#include "wiz8/level_specific_code/Trynnie1.h"
#include "wiz8/level_specific_code/Trynnie2.h"
#include "wiz8/level_specific_code/Swamp.h"
#include "wiz8/level_specific_code/SeaCaves.h"
#include "wiz8/level_specific_code/Rift1.h"
#include "wiz8/level_specific_code/Camp.h"
#include "wiz8/level_specific_code/RapaxMainFloor.h"
#include "wiz8/level_specific_code/RapaxUpperFloor.h"
#include "wiz8/level_specific_code/Ascension.h"
#include "wiz8/level_specific_code/MtGigas1.h"
#include "wiz8/level_specific_code/MtGigas2.h"
#include "wiz8/level_specific_code/MtGigasOuter.h"
#include "wiz8/level_specific_code/MtGigasTop.h"
#include "wiz8/level_specific_code/CosmicCircle.h"
#include "wiz8/level_specific_code/MartensBluff1.h"
#include "wiz8/level_specific_code/MartensBluff2.h"
#include "wiz8/level_specific_code/SavantTower.h"
#include "wiz8/level_specific_code/ConnectiveTissue.h"
#include "wiz8/fact_state.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/layouts/world.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/save_game.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/location_variables.h"
#include "wiz8/character_skills.h"
#include "wiz8/string_database.h"
#include "wiz8/float_constants.h"
#include "wiz8/utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "Debug.h"

/* The world-cursor node the party is standing in, tracked across the
   command-0 sweep so enter/leave commands fire once per crossing. */
// GLOBAL: WIZ8 0x006834d4
static W8WorldCursorNode* g_active_cursor_node;

// GLOBAL: WIZ8 0x006834d8
W8Vector<W8MasterFunction>* g_master_functions;
// GLOBAL: WIZ8 0x006834dc
bool g_remove_current_master_function;

/* SGP full-volume scale: CreateAndPlaySoundNode multiplies its clamped
   loudness fraction by this to get the node's base volume. */
// GLOBAL: WIZ8 0x005EC510
const float g_sound_node_full_volume = 127.0f;

/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
// FUNCTION: WIZ8 0x004D8E40
void RunMasterFunctions(void)
{
    int count = g_master_functions->GetCount();

    for (int index = 0; index < count; ++index) {
        (*g_master_functions->GetAt(index))(0);
        if (g_remove_current_master_function) {
            g_master_functions->RemoveAt(index);
            --count;
            --index;
        }
    }
}

/* Run every registered master function once with argument -1, the persist
   command the level masters answer by writing their live state into the
   location variables. */
// FUNCTION: WIZ8 0x004D8EC0
void SaveMasterFunctions(void)
{
    int count = g_master_functions->GetCount();

    for (int index = 0; index < count; ++index) {
        (*g_master_functions->GetAt(index))(-1);
    }
}

/* The level callbacks' scripted spawn: settle the requested point onto the
   ground when asked, create the group, apply hostility, refresh its centre
   cache and re-run outward sight for the new arrivals. */
// FUNCTION: WIZ8 0x004D8F00
W8MonsterGroup* SpawnMonsters(int monster_id, int count, srVector3T<float>* position, int hostility,
                              bool settle, bool a, bool b)
{
    W8MonsterGroup* group;
    srVector3T<float> position_copy;

    if (settle) {
        position->y = SettlePositionToGround(position, 0);
    }
    position_copy = *position;
    group = CreateGroup(monster_id, count, &position_copy, a, b, settle);
    if (group != 0) {
        SetMonsterGroupHostility(group, hostility, false);
        GetMonsterGroupCentre(group, 0);
        RefreshOutwardSightForAllMonsters();
    }
    return group;
}

/* The level callbacks' one-shot positional sound: reject a negative
   loudness fraction, clamp anything above one, then create the node at the
   requested spot, scale its base volume and falloff and start playback. */
// FUNCTION: WIZ8 0x004D8F80
stSound3D* CreateAndPlaySoundNode(char* sound_name, srVector3T<float> position, float volume,
                                  float scale, bool play_flag)
{
    if (volume > g_float_one) {
        volume = 1.0f;
    } else if (volume < g_float_zero) {
        return 0;
    }

    stSound3D* sound = new stSound3D(sound_name, 0);
    if (sound != 0) {
        srVector3T<double> sound_position;
        sound_position.SetFromFloat(&position);
        sound->setLocation(sound_position);
        sound->volume = static_cast<int>(volume * g_sound_node_full_volume);
        sound->falloff = scale * g_world_scale;
        sound->Play(play_flag, true);
    }
    return sound;
}

/* One row of the cursor-node dispatch table at 0x006109F4, indexed by the
   node's type parameter (numbers[2]). The dispatcher invokes a row as
   (command, node, context); the context slot normally carries the address of
   the command's argument byte, while command 4 smuggles the byte itself
   through it. */
typedef unsigned char (*W8WorldCursorNodeHandler)(int command, W8WorldCursorNode* node,
                                                  int context);

static unsigned char WorldCursorNodeShowMessageOnce(int command, W8WorldCursorNode* node,
                                                    int context);
static unsigned char WorldCursorNodeShowContextMessage(int command, W8WorldCursorNode* node,
                                                       int context);
static unsigned char WorldCursorNodeShowMessage(int command, W8WorldCursorNode* node, int context);
static unsigned char WorldCursorNodeApplyItemEffect(int command, W8WorldCursorNode* node,
                                                    int context);
static unsigned char WorldCursorNodeMaleCharacterEvent(int command, W8WorldCursorNode* node,
                                                       int context);
static unsigned char IsMasterFunctionTypeEight(int command, W8WorldCursorNode* node, int context);
static unsigned char WorldCursorNodeSeenBodies(int command, W8WorldCursorNode* node, int context);
static unsigned char WorldCursorNodeApplyType6ItemEffect(int command, W8WorldCursorNode* node,
                                                         int context);
static unsigned char WorldCursorNodePartyVoice(int command, W8WorldCursorNode* node, int context);

/* The character-event kind constants the cursor-node handlers queue. */
// GLOBAL: WIZ8 0x005EE5F4
const int g_character_event_kind0 = 0x1b;
// GLOBAL: WIZ8 0x005EE63C
const int g_character_event_kind1 = 0x2d;
// GLOBAL: WIZ8 0x005EE688
const int g_character_event_kind3 = 0x40;

// GLOBAL: WIZ8 0x006109F4
static W8WorldCursorNodeHandler g_world_cursor_node_handlers[10] = {
    WorldCursorNodeShowMessageOnce,      /* type 0 */
    WorldCursorNodeShowMessageOnce,      /* type 1 */
    WorldCursorNodeShowContextMessage,   /* type 2 */
    WorldCursorNodeShowMessage,          /* type 3 */
    WorldCursorNodeApplyItemEffect,      /* type 4 */
    WorldCursorNodeSeenBodies,           /* type 5 */
    WorldCursorNodeApplyType6ItemEffect, /* type 6 */
    IsMasterFunctionTypeEight,           /* type 7 */
    WorldCursorNodePartyVoice,           /* type 8 */
    WorldCursorNodeMaleCharacterEvent    /* type 9 */
};

/* Dispatch one world-cursor-node command against the nodes covering `info`'s
   ground-settled position (the camera's when info is null). Command zero is
   the movement sweep: the tracked node's handler gets 2 (leave) and each
   newly entered node's handler gets 1 (enter); commands 3, 4 and 8 deliver
   themselves to nodes of type 3, 2 and 7 respectively. Answers whether any
   handler reported the command handled. */
// FUNCTION: WIZ8 0x004D9080
unsigned char DispatchWorldCursorNodeCommand(W8MonsterInfo* info, int command, ...)
{
    srVector3T<float> position;
    W8WorldCursorNode* node;
    W8WorldCursorNode* previous;
    int context;
    bool handled;
    unsigned char result;

    result = 0;
    handled = false;
    // Retail callers use both two and three arguments. Non-4 commands only
    // forward the optional argument slot's address; handlers do not read it.
    va_list arguments;
    va_start(arguments, command);
    // reinterpret-ok: retail passes the optional argument slot as context.
    context = reinterpret_cast<int>(arguments);
    if (IsMipeActive()) {
        va_end(arguments);
        return handled;
    }
    if (info == 0) {
        GetCameraPosition(&position);
    } else {
        position = info->p3D->GetPosition();
    }
    position.y = SettlePositionToGround(&position, 0) + g_float_one_hundred_twenty_five;
    node = FindWorldCursorNodeAtPoint(0, &position);
    if (command == 4) {
        context = va_arg(arguments, int);
    }
    va_end(arguments);
    if (node == 0) {
        return handled;
    }
    do {
        previous = g_active_cursor_node;
        switch (command) {
        case 0:
            if (previous != 0 && previous != node) {
                result = g_world_cursor_node_handlers[GetWorldCursorNodeParameter(previous, 2)](
                    2, previous, context);
            }
            previous = g_active_cursor_node;
            if (node != 0 && previous != node) {
                result = g_world_cursor_node_handlers[GetWorldCursorNodeParameter(node, 2)](
                    1, node, context);
            }
            g_active_cursor_node = node;
            break;
        case 4:
            if (node != 0 && GetWorldCursorNodeParameter(node, 2) == 2) {
                result = g_world_cursor_node_handlers[2](4, node, context);
            }
            break;
        case 8:
            if (node != 0 && GetWorldCursorNodeParameter(node, 2) == 7) {
                result = g_world_cursor_node_handlers[7](8, node, context);
            }
            break;
        case 3:
            if (node != 0) {
                int type = GetWorldCursorNodeParameter(node, 2);
                if (type == 3) {
                    result = g_world_cursor_node_handlers[3](type, node, context);
                }
            }
            break;
        default:
            result = 0;
            break;
        }
        if (result != 0) {
            handled = true;
        }
        node = FindWorldCursorNodeAtPoint(node, &position);
    } while (node != 0);
    return handled;
}

/* Type-0/1 nodes: on the enter command, show the node's message-database
   string once - the first byte of the node's userdata is the shown flag.
   Parameter 1 additionally gates the message on a living party member with
   trait 0x0c while the party is neither searching nor under effect 0x11. */
// FUNCTION: WIZ8 0x004D9260
static unsigned char WorldCursorNodeShowMessageOnce(int command, W8WorldCursorNode* node,
                                                    int context)
{
    int message_id;
    const char* folder;
    int type;
    unsigned char enabled;
    bool result;
    char* shown;
    int slot;
    char path[512];
    wchar_t text[2048];

    result = false;
    message_id = GetWorldCursorNodeParameter(node, 0);
    folder = GetLevelFolderName(GetLoadedLevelID());
    type = GetWorldCursorNodeParameter(node, 2);
    enabled = static_cast<unsigned char>(GetWorldCursorNodeParameter(node, 1));
    if (enabled != 0 && g_status.search_mode == 0 && g_status.party_modifiers.detect_secrets == 0) {
        for (slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
            if (g_status.buffers.XChar[slot].fOccupied &&
                CharacterHasTrait(g_status.buffers.Char + slot, W8_TRAIT_SEARCH)) {
                goto command_check;
            }
        }
        return 0;
    }
command_check:
    if (command != 1) {
        return 0;
    }
    if (type == 0) {
        result = true;
        GetWorldCursorNodeUserdata(node, &shown, 0);
        if (shown == 0) {
            SetWorldCursorNodeUserdataSize(node, 1);
            GetWorldCursorNodeUserdata(node, &shown, 0);
        }
        if (*shown != 0) {
            return 0;
        }
        *shown = 1;
        if (folder == 0) {
            folder = "Test";
        }
        sprintf(path, "Data\\Messages\\%s.msg", folder);
        if (GetStringFromStringDatabase(path, message_id, text, 0, 0) != 0) {
            ShowString(text);
        }
    }
    return result;
}

/* Type-2 nodes: command 4 with a nonzero context flag shows the node's
   message while the main game screen is up. */
// FUNCTION: WIZ8 0x004D93E0
static unsigned char WorldCursorNodeShowContextMessage(int command, W8WorldCursorNode* node,
                                                       int context)
{
    const char* folder;
    int message_id;
    char path[512];
    wchar_t text[2046];

    folder = GetLevelFolderName(GetLoadedLevelID());
    if (command != 4) {
        return 0;
    }
    if (static_cast<unsigned char>(context) != 0) {
        message_id = GetWorldCursorNodeParameter(node, 0);
        if (folder == 0) {
            folder = "Test";
        }
        sprintf(path, "Data\\Messages\\%s.msg", folder);
        if (GetStringFromStringDatabase(path, message_id, text, 0, 0) != 0) {
            if (wcslen(text) != 0 && g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                ShowString(text);
            }
        }
    }
    return 1;
}

/* Type-3 nodes: command 3 shows the node's message unconditionally. */
// FUNCTION: WIZ8 0x004D94B0
static unsigned char WorldCursorNodeShowMessage(int command, W8WorldCursorNode* node, int context)
{
    const char* folder;
    int message_id;
    char path[512];
    wchar_t text[2048];

    folder = GetLevelFolderName(GetLoadedLevelID());
    if (command != 3) {
        return 0;
    }
    message_id = GetWorldCursorNodeParameter(node, 0);
    if (folder == 0) {
        folder = "Test";
    }
    sprintf(path, "Data\\Messages\\%s.msg", folder);
    if (GetStringFromStringDatabase(path, message_id, text, 0, 0) != 0) {
        if (wcslen(text) != 0) {
            ShowString(text);
        }
    }
    return 1;
}

/* Type-4 nodes: entering applies the node's item effect to a random party
   member. */
// FUNCTION: WIZ8 0x004D9560
static unsigned char WorldCursorNodeApplyItemEffect(int command, W8WorldCursorNode* node,
                                                    int context)
{
    if (command == 1) {
        ApplyItemEffectToRandomCharacter(g_character_event_kind0, -1, 0,
                                         g_character_event_no_flags);
    }
    return 1;
}

/* Type-9 nodes: entering while the fact-0x14c gate holds queues the male
   event for the indexed party member when that member is male. */
// FUNCTION: WIZ8 0x004D9590
static unsigned char WorldCursorNodeMaleCharacterEvent(int command, W8WorldCursorNode* node,
                                                       int context)
{
    W8Character* character;

    if (command == 1 && g_status.rpc_active) {
        character = g_status.buffers.Char + g_status.sedexus_party_slot;
        if (character->gender == W8_GENDER_MALE) {
            QueueCharacterEvent(character, g_character_event_kind1, 0, g_character_event_no_flags,
                                g_character_event_full_volume);
        }
    }
    return 1;
}

/* Predicate installed in the master-function callback table. */
// FUNCTION: WIZ8 0x004D95F0
static unsigned char IsMasterFunctionTypeEight(int command, W8WorldCursorNode* node, int context)
{
    return command == 8;
}

/* Type-5 nodes: the Ascension Peak "seen bodies" check. Once all three relic
   items are on the party and neither blocking fact holds, queue the reaction
   and latch the AP_SeenBodies location variable. Retail runs the fact-0x133
   clear twice. */
// FUNCTION: WIZ8 0x004D9600
static unsigned char WorldCursorNodeSeenBodies(int command, W8WorldCursorNode* node, int context)
{
    if (command == 1) {
        if (GetLocationVarIDByName("AP_SeenBodies") == -1) {
            if (CountAscensionPeakItems() == 3) {
                if (GetFact(W8_FACT_TMISSION_MEET_ZANT_AT_AP) != 0 ||
                    GetFact(W8_FACT_UMISSION_MEET_AP_ASSIGN) != 0) {
                    ApplyItemEffectToRandomCharacter(g_character_event_kind3, -1, 0,
                                                     g_character_event_no_flags);
                    CreateLocationVar("AP_SeenBodies", 1);
                    if (GetFact(W8_FACT_QUEST_PEACE_GOTO_AP) != 0) {
                        SetFact(W8_FACT_QUEST_PEACE_GOTO_AP, 0, false);
                    }
                    if (GetFact(W8_FACT_QUEST_PEACE_GOTO_AP) != 0) {
                        SetFact(W8_FACT_QUEST_PEACE_GOTO_AP, 0, false);
                    }
                }
            }
        }
    }
    return 1;
}

/* Type-6 nodes: entering applies the node's item effect to a random party
   member. */
// FUNCTION: WIZ8 0x004D96C0
static unsigned char WorldCursorNodeApplyType6ItemEffect(int command, W8WorldCursorNode* node,
                                                         int context)
{
    if (command == 1) {
        ApplyItemEffectToRandomCharacter(g_character_event_kind3, -1, 0,
                                         g_character_event_no_flags);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004D96F0
void ClearActiveWorldCursorNode(void)
{
    g_active_cursor_node = 0;
}

/* Master-function values 0x10 and 0x26 select the same path once either of
   the two enabling facts has been set. */
// FUNCTION: WIZ8 0x004D9700
int NormalizeMasterFunctionValue(int value)
{
    if (value == 0x10 || value == 0x26) {
        if (GetFact(W8_FACT_RAPAX_AWAY_CAMP_EXISTS) == 0 && GetFact(W8_FACT_PARTY_AT_RAC) == 0) {
            return 0x26;
        }
        value = 0x10;
    }
    return value;
}

#define MASTER_FUNCTION_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MasterFunctionList.cpp"

// GLOBAL: WIZ8 0x006834DD
bool g_running_trigger_from_script;
// GLOBAL: WIZ8 0x006109F0
bool g_npc_dialogue_closed = true;
// GLOBAL: WIZ8 0x006834E0
static int g_master_function_level;
// GLOBAL: WIZ8 0x00652DA5
bool g_sea_caves_slope_override_enabled;

/* Fill the away camp chest from the level-0x26 item records stored in the
   current save. Records that are not world-persistent move into the chest
   trigger's container item; persistent records are flattened back into the
   load vector (so their own groups are visited too) and then freed. */
// FUNCTION: WIZ8 0x004D9740
void LoadAwayCampChest(void)
{
    W8Vector<W8WorldItem*> items(5);
    W8Prop* pChest;
    Trigger* pTrigger;
    W8WorldItem* pContainer;
    int index;

    pChest = FindPropByName(g_world, "AwayCampChest");
    if (pChest == 0) {
        srAssertFail("pChest", MASTER_FUNCTION_CPP, 0x5a4, 0);
    }
    pTrigger = pChest->GetTrigger();
    if (pTrigger == 0) {
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5a6, 0);
    }
    pContainer = pTrigger->GetOrCreateItemGroup(true);
    if (pContainer == 0) {
        srAssertFail("pContainer", MASTER_FUNCTION_CPP, 0x5a8, 0);
    }
    LoadSavedLevelItems(0x26, &items);
    for (index = 0; index < items.GetCount(); ++index) {
        W8WorldItem* item = *items.GetAt(index);

        if (ItemInfoIsWorldPersistent(item)) {
            if (ItemInfoGetNumInGroup(item) != 0) {
                W8WorldItem* next = ItemInfoGroupGetNext(item);

                if (next != 0) {
                    ItemInfoMakeGroupList(next, &items);
                }
            }
            free(item);
        } else {
            ItemInfoAddToGroup(pContainer, item);
        }
    }
}

/* Type-8 nodes: entering queues a random party member's voice line for the
   node's event type, once - the first byte of the node's userdata is the
   shown flag. */
// FUNCTION: WIZ8 0x004D98C0
static unsigned char WorldCursorNodePartyVoice(int command, W8WorldCursorNode* node, int context)
{
    int event_type;
    char* shown;
    int slot;

    event_type = GetWorldCursorNodeParameter(node, 0);
    if (command == 1) {
        GetWorldCursorNodeUserdata(node, &shown, 0);
        if (shown == 0) {
            SetWorldCursorNodeUserdataSize(node, 1);
            GetWorldCursorNodeUserdata(node, &shown, 0);
        }
        if (*shown == 0) {
            *shown = 1;
            slot = PickRandomPartySpeaker(event_type, 0xff);
            if (slot != -1) {
                QueueCharacterEvent(g_status.buffers.Char + slot, event_type, 0,
                                    g_character_event_no_flags, g_character_event_full_volume);
            }
        }
    }
    return 1;
}

/* Format the current level's message-database path, fetch the indexed string
   and show it. Answers whether the string existed. */
// FUNCTION: WIZ8 0x004D9960
unsigned char ShowLevelMessage(int message_id)
{
    const char* folder;
    char path[512];
    wchar_t text[2000];

    folder = GetLevelFolderName(GetLoadedLevelID());
    if (folder == 0) {
        folder = "Test";
    }
    sprintf(path, "Data\\Messages\\%s.msg", folder);
    if (GetStringFromStringDatabase(path, message_id, text, 0, 0) != 0) {
        ShowString(text);
        return 1;
    }
    return 0;
}

/* The level-specific callbacks and master-function helpers this unit
   installs. Their bodies live in the other level-specific units, so they
   are declared here rather than in the published header. */
void EnsureTrynnie1KilledVar(void);
bool Trynnie2GoodaVineA(Trigger* trigger);
bool Trynnie2GoodaVineB(Trigger* trigger);
bool Trynnie2GiveZulu(Trigger* trigger);
bool Trynnie2MeatMaker(Trigger* trigger);
bool Trynnie2MeatBox(Trigger* trigger);
bool Trynnie2UrnTrigger(Trigger* trigger);
bool Trynnie1FountRandomFX(Trigger* trigger);

/* Activation callbacks that only let the trigger run or stop it. Retail's
   linker folded them into ScreenLifecycleSuccess and IgnoreSpellCastingInput,
   which compile to the same bytes. */
static bool AllowTriggerActivation(Trigger* trigger)
{
    return true;
}

static bool BlockTriggerActivation(Trigger* trigger)
{
    return false;
}

/* Repeated level setup owns the find/assert/bind operation. Diagnostics keep
   their original line and literal spelling, including the inconsistent names. */
static void BindLevelTrigger(const char* name, Trigger::ActivationCallback callback, int line,
                             const char* diagnostic)
{
    Trigger* trigger = FindTriggerByName(name);
    if (trigger == 0) {
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP, line, diagnostic);
    }
    trigger->activation_callback = callback;
}

static void BindLevelTrigger(const char* name, Trigger::ActivationCallback callback, int line)
{
    Trigger* trigger = FindTriggerByName(name);
    if (trigger == 0) {
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP, line,
                     reinterpret_cast<const char*>(
                         String(/* reinterpret-ok: SGP rotating debug buffer */
                                "Missing trigger '%s'! It's not in the LVL file!", name)));
    }
    trigger->activation_callback = callback;
}

/* Install the level's trigger callbacks and master-function helpers. */
// FUNCTION: WIZ8 0x004D6C50
void InitializeLevelMasterFunctions(int level)
{
    Trigger* pTrigger;

    if (g_master_functions == 0) {
        g_master_functions = new W8Vector<W8MasterFunction>(5);
    } else {
        /* Retail caches the count and removes one entry per iteration; the
           clearing loop leaves it at zero. */
        int count = g_master_functions->GetCount();
        for (int index = 0; index < count; ++index) {
            g_master_functions->RemoveAt(0);
        }
    }
    /* Retail still walks the now-empty vector from the top and deletes each
       unlinked entry; the loop cannot run, but its instructions are present,
       so it is preserved rather than dropped. */
    for (int index = g_master_functions->GetCount() - 1; index >= 0; --index) {
        void* entry = reinterpret_cast<void*>(g_master_functions->RemoveAt(
            index)); /* reinterpret-ok: function entry stored as data */
        operator delete(entry);
    }
    g_running_trigger_from_script = false;
    g_npc_dialogue_closed = true;
    g_master_function_level = level;
    g_sea_caves_slope_override_enabled = false;
    switch (level) {
    case 0:
        BindLevelTrigger("ChaosMolori", ArnikaChaosMolori, 0x74,
                         "Missing trigger 'ChaosMolori'! It's not in the LVL file!");
        BindLevelTrigger("Maddmook", ArnikaMaddmook, 0x78,
                         "Missing trigger 'Maddmook'! It's not in the LVL file!");
        BindLevelTrigger("CMbox", ArnikaCMbox, 0x7c,
                         "Missing trigger 'CMbox'! It's not in the LVL file!");
        BindLevelTrigger("AstralDominae", ArnikaAstralDominae, 0x80,
                         "Missing trigger 'AstralDominae'! It's not in the LVL file!");
        BindLevelTrigger("BallSlot", ArnikaBallSlot, 0x84,
                         "Missing trigger 'BallSlot'! It's not in the LVL file!");
        BindLevelTrigger("Flightrecordertrigger", ArnikaFlightRecorder, 0x88,
                         "Missing trigger 'Flightrecordertrigger'! It's not in the LVL file!");
        BindLevelTrigger("ULLspawn", AllowTriggerActivation, 0x8c,
                         "Missing trigger 'ULLspawn'! It's not in the LVL file!");
        BindLevelTrigger("Mookholo", ArnikaMookholo, 0x90,
                         "Missing trigger 'Mookholo'! It's not in the LVL file!");
        BindLevelTrigger("MookFrontDoor", ArnikaMookFrontDoor, 0x9c,
                         "Missing trigger 'MookFrontDoor'! It's not in the LVL file!");
        BindLevelTrigger("YellowButton", ArnikaYellowButton, 0xa0,
                         "Missing trigger 'YellowButton'! It's not in the LVL file!");
        BindLevelTrigger("Vaultalarmdoor", ArnikaVaultAlarmDoor, 0xa4,
                         "Missing trigger 'Vaultalarmdoor'! It's not in the LVL file!");
        BindLevelTrigger("Exitbutton", ArnikaExitButton, 0xa8,
                         "Missing trigger 'Exitbutton'! It's not in the LVL file!");
        BindLevelTrigger("GenVault-2-door", ArnikaGenVaultDoor, 0xac,
                         "Missing trigger 'GenVault-2-door'! It's not in the LVL file!");
        BindLevelTrigger("ARN11", CosmicCircleReturnFalse, 0xb8,
                         "Missing trigger 'ARN11'! It's not in the LVL file!");
        BindLevelTrigger("RedButton", ArnikaRedButton, 0xbd,
                         "Missing trigger 'RedButton'! It's not in the LVL file!");
        BindLevelTrigger("El1-TopButtons", ArnikaEl1TopButtons, 0xc1,
                         "Missing trigger 'El1-TopButtons'! It's not in the LVL file!");
        BindLevelTrigger("El1-BottomButtons", ArnikaEl1BottomButtons, 0xc5,
                         "Missing trigger 'El1-BottomButtons'! It's not in the LVL file!");
        BindLevelTrigger("GreenButton", ArnikaGreenButton, 0xca,
                         "Missing trigger 'GreenButton'! It's not in the LVL file!");
        BindLevelTrigger("Elevator-02", ArnikaElevator02Trigger, 0xce,
                         "Missing trigger 'Elevator-02'! It's not in the LVL file!");
        BindLevelTrigger("LazerScanner", ArnikaLazerScanner, 0xd8,
                         "Missing trigger 'LazerScanner'! It's not in the LVL file!");
        pTrigger = FindTriggerByName("ScannerDoor");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xdc,
                         "Missing trigger 'ScannerDoor'! It's not in the LVL file!");
        }
        pTrigger->m_lData1 = 0;
        pTrigger->activation_callback = ArnikaScannerDoor;
        ArnikaLevelSetup();
        return;
    case 1:
        BindLevelTrigger("RampUp", AscensionRampUp, 0x116,
                         "Missing trigger 'RampUp'! It's not in the LVL file!");
        BindLevelTrigger("ChaosATrigger", AscensionChaosATrigger, 0x11a,
                         "Missing trigger 'ChaosATrigger'! It's not in the LVL file!");
        BindLevelTrigger("ChaosBTrigger", AscensionChaosBTrigger, 0x11e,
                         "Missing trigger 'ChaosBTrigger'! It's not in the LVL file!");
        BindLevelTrigger("LifeATrigger", AscensionLifeATrigger, 0x122,
                         "Missing trigger 'LifeATrigger'! It's not in the LVL file!");
        BindLevelTrigger("LifeBTrigger", AscensionLifeBTrigger, 0x126,
                         "Missing trigger 'LifeBTrigger'! It's not in the LVL file!");
        BindLevelTrigger("KnowATrigger", AscensionKnowATrigger, 0x12a,
                         "Missing trigger 'KnowATrigger'! It's not in the LVL file!");
        BindLevelTrigger("KnowBTrigger", AscensionKnowBTrigger, 0x12e,
                         "Missing trigger 'KnowBTrigger'! It's not in the LVL file!");
        BindLevelTrigger("RampUp", AscensionDarkSavantSpawn, 0x132,
                         "Missing trigger 'RampUp'! It's not in the LVL file!");
        BindLevelTrigger("Path1Camera", AscensionPath1Camera, 0x136,
                         "Missing trigger 'Path1Camera'! It's not in the LVL file!");
        BindLevelTrigger("Shaker", AscensionShaker, 0x5ee);
        AscensionPeakInit();
        return;
    case 4:
        CosmicCircleSetup();
        BindLevelTrigger("CC_TRIGGERPLANE1HEDRA", CosmicCircleTriggerPlane1Hedra, 0x33a,
                         "Missing trigger 'CC_TRIGGERPLANE1HEDRA'! It's not in the LVL file!");
        break;
    case 5:
        MartensBluff1Setup();
        BindLevelTrigger("MR109", MartensBluff1Teleporter, 0x144,
                         "Missing trigger 'MR109'! It's not in the LVL file!");
        pTrigger = FindTriggerByName("MR110");
        if (pTrigger != 0) {
            pTrigger->activation_callback = BlockTriggerActivation;
        }
        pTrigger = FindTriggerByName("MR111");
        if (pTrigger != 0) {
            pTrigger->activation_callback = BlockTriggerActivation;
        }
        pTrigger = FindTriggerByName("MR112");
        if (pTrigger != 0) {
            pTrigger->activation_callback = BlockTriggerActivation;
        }
        BindLevelTrigger("F-Handlock", MartensBluff1FHandlock, 0x154,
                         "Missing trigger 'F-Handlock'! It's not in the LVL file!");
        BindLevelTrigger("ButtonGigas", MartensBluff1ButtonGigas, 0x158,
                         "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        BindLevelTrigger("ButtonTrang", MartensBluff1ButtonTrang, 0x15c,
                         "Missing trigger 'ButtonTrang'! It's not in the LVL file!");
        BindLevelTrigger("ButtonRift", MartensBluff1ButtonRift, 0x160,
                         "Missing trigger 'ButtonRift'! It's not in the LVL file!");
        BindLevelTrigger("ButtonMaten", MartensBluff1ButtonMaten, 0x164,
                         "Missing trigger 'ButtonMaten'! It's not in the LVL file!");
        BindLevelTrigger("WireTrigger", MartensBluff1WireTrigger, 0x168,
                         "Missing trigger 'WireTrigger'! It's not in the LVL file!");
        BindLevelTrigger("ButtonGigas", MartensBluff1ButtonGigas, 0x16c,
                         "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        BindLevelTrigger("Controller", MartensBluff1Controller, 0x170,
                         "Missing trigger 'Controller'! It's not in the LVL file!");
        BindLevelTrigger("Dial-A", MartensBluff1DialA, 0x174,
                         "Missing trigger 'Dial-A'! It's not in the LVL file!");
        BindLevelTrigger("Dial-B", MartensBluff1DialB, 0x178,
                         "Missing trigger 'Dial-B'! It's not in the LVL file!");
        BindLevelTrigger("Dial-C", MartensBluff1DialC, 0x17c,
                         "Missing trigger 'Dial-C'! It's not in the LVL file!");
        BindLevelTrigger("Gas-Switch", MartensBluff1GasSwitch, 0x180,
                         "Missing trigger 'Gas-Switch'! It's not in the LVL file!");
        BindLevelTrigger("J-Doorcontroller", MartensBluff1JDoorController, 0x184,
                         "Missing trigger 'J-Doorcontroller'! It's not in the LVL file!");
        BindLevelTrigger("MartenBook", MartensBluff1MartenBook, 0x188,
                         "Missing trigger 'trigger16254'! It's not in the LVL file!");
        return;
    case 6:
        MartensBluff2Setup();
        BindLevelTrigger("Arrowtraptrigger", TriggerArrowTrap, 0x192,
                         "Missing trigger 'Arrowtraptrigger'! It's not in the LVL file!");
        BindLevelTrigger("Spikeballtrigger", MartensBluff2Spikeballtrigger, 0x196,
                         "Missing trigger 'Spikeballtrigger'! It's not in the LVL file!");
        BindLevelTrigger("DoorBolt", MartensBluff2DoorBolt, 0x19a,
                         "Missing trigger 'DoorBolt'! It's not in the LVL file!");
        BindLevelTrigger("DummyLever", MartensBluff2DummyLever, 0x19e,
                         "Missing trigger 'DummyLever'! It's not in the LVL file!");
        BindLevelTrigger("Dummy", MartensBluff2Dummy, 0x1a2,
                         "Missing trigger 'Dummy'! It's not in the LVL file!");
        BindLevelTrigger("PerfumeBox", MartensBluff2PerfumeBox, 0x1a6,
                         "Missing trigger 'PerfumeBox'! It's not in the LVL file!");
        BindLevelTrigger("StoneIdol", MartensBluff2StoneIdol, 0x1aa,
                         "Missing trigger 'StoneIdol'! It's not in the LVL file!");
        BindLevelTrigger("BlueFlowers", MartensBluff2BlueFlowers, 0x1ae,
                         "Missing trigger 'BlueFlowers'! It's not in the LVL file!");
        BindLevelTrigger("SquisherControls", MartensBluff2SquisherControls, 0x1b2,
                         "Missing trigger 'SquisherControls'! It's not in the LVL file!");
        BindLevelTrigger("DoorControls", MartensBluff2DoorControls, 0x1b6,
                         "Missing trigger 'DoorControls'! It's not in the LVL file!");
        return;
    case 8:
        ClearTextForBarTrigger();
        BindLevelTrigger("roach_trigger", OnRoachTriggerActivated, 0x1c0,
                         "Missing trigger 'roach_trigger'! It's not in the LVL file!");
        BindLevelTrigger("spider_trigger", OnSpiderTriggerActivated, 0x1c4,
                         "Missing trigger 'spider_trigger'! It's not in the LVL file!");
        BindLevelTrigger("Bartrigger", OnBarTriggerActivated, 0x1c8,
                         "Missing trigger 'Bartrigger'! It's not in the LVL file!");
        BindLevelTrigger("Coffinlide", OnCoffinlideActivated, 0x1cc,
                         "Missing trigger 'Coffinlide'! It's not in the LVL file!");
        BindLevelTrigger("Coffinlidg", OnCoffinlidgActivated, 0x1d0,
                         "Missing trigger 'Coffinlidg'! It's not in the LVL file!");
        BindLevelTrigger("wheel_star", OnWheelStarActivated, 0x5ee);
        return;
    case 9:
        BindLevelTrigger("bell_button", Monastery2BellButton, 0x1d8,
                         "Missing trigger 'bell_button'! It's not in the LVL file!");
        BindLevelTrigger("micro_door2", Monastery2MicroDoor2, 0x1dc,
                         "Missing trigger 'micro_door2'! It's not in the LVL file!");
        return;
    case 0xc:
        MtGigas1Setup();
        BindLevelTrigger("_VOC_EWAXXLIFT1", MtGigas1Lift1, 0x219,
                         "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXLIFT2", MtGigas1Lift2, 0x21d,
                         "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        BindLevelTrigger("PRESSUREPLATE", MtGigas1PressurePlate, 0x221,
                         "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        BindLevelTrigger("mudWallTrigger", MtGigas1MudWall, 0x225,
                         "Missing trigger 'mudWallTrigger'! It's not in the LVL file!");
        return;
    case 0xd:
        MtGigas2Setup();
        BindLevelTrigger("_VOC_EWAXXTRAIN", MtGigas2Train, 0x22e,
                         "Missing trigger '_VOC_EWAXXTRAIN'! It's not in the LVL file!");
        BindLevelTrigger("redwire", MtGigas2RedWire, 0x232,
                         "Missing trigger 'redwire'! It's not in the LVL file!");
        BindLevelTrigger("bluewire", MtGigas2BlueWire, 0x236,
                         "Missing trigger 'bluewire'! It's not in the LVL file!");
        BindLevelTrigger("yellowwire", MtGigas2YellowWire, 0x23a,
                         "Missing trigger 'yellowwire'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXLIFT3", MtGigas2Lift3, 0x23e,
                         "Missing trigger '_VOC_EWAXXLIFT3'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXTOPDOOR1", MtGigas2TopDoor1, 0x242,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXOFFICER1", MtGigas2Officer1, 0x246,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXOFFICER2", MtGigas2Officer2, 0x24a,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        BindLevelTrigger("triggerPlaneLaserAlarm", MtGigas2LaserAlarm, 0x24e,
                         "Missing trigger 'triggerPlaneLaserAlarm'! It's not in the LVL file!");
        BindLevelTrigger("triggerPlaneLaserAlarm01", MtGigas2LaserAlarm, 0x252,
                         "Missing trigger 'triggerPlaneLaserAlarm01'! It's not in the LVL file!");
        BindLevelTrigger("accessHatch", MtGigas2AccessHatch, 0x256,
                         "Missing trigger 'accessHatch'! It's not in the LVL file!");
        BindLevelTrigger("wiringMalfunction", MtGigas2WiringMalfunction, 0x25a,
                         "Missing trigger 'wiringMalfunction'! It's not in the LVL file!");
        return;
    case 0xe:
        ProcessFlagPosition();
        BindLevelTrigger("_VOC_EWAXXLIFT1", MtGigas1Lift1, 0x1ff,
                         "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        BindLevelTrigger("crank", OnCrankTriggerActivated, 0x203,
                         "Missing trigger 'crank'! It's not in the LVL file!");
        BindLevelTrigger("Security Button", OnSecurityButtonActivated, 0x207,
                         "Missing trigger 'Security Button'! It's not in the LVL file!");
        BindLevelTrigger("VOC_EWAXXSENTRYtrig", OnSentryTriggerActivated, 0x20b,
                         "Missing trigger 'VOC_EWAXXSENTRYtrig'! It's not in the LVL file!");
        BindLevelTrigger("ewaxxdoortrigger03", OnEwaxxDoor03Activated, 0x20f,
                         "Missing trigger 'ewaxxdoortrigger03'! It's not in the LVL file!");
        BindLevelTrigger("dummytrigger", OnDummyTriggerActivated, 0x5ee);
        return;
    case 0xf:
        BindLevelTrigger("_VOC_EWAXXCANNON1", OnEwaxxCannon1Activated, 0x1e2,
                         "Missing trigger '_VOC_EWAXXCANNON1'! It's not in the LVL file!");
        BindLevelTrigger("EwaxxLanding", OnEwaxxLandingActivated, 0x1ea,
                         "Missing trigger 'EwaxxLanding'! It's not in the LVL file!");
        BindLevelTrigger("catchCord", OnCatchCordActivated, 0x1ee,
                         "Missing trigger 'catchCord'! It's not in the LVL file!");
        BindLevelTrigger("painActivatorTrigger", OnPainActivatorActivated, 0x1f2,
                         "Missing trigger 'painActivatorTrigger'! It's not in the LVL file!");
        BindLevelTrigger("_VOC_EWAXXTOPDOOR2", OnEwaxxTopDoor2Activated, 0x1f6,
                         "Missing trigger '_VOC_EWAXXTOPDOOR2'! It's not in the LVL file!");
        return;
    case 0x10:
        if (GetFact(W8_FACT_PARTY_AT_RAC) == 0) {
            LoadAwayCampChest();
            SetFact(W8_FACT_PARTY_AT_RAC, 0, false);
        }
        BindLevelTrigger("prisondoor06", CampPrisonDoor06, 0x308,
                         "Missing trigger 'prisondoor06'! It's not in the LVL file!");
        BindLevelTrigger("prisondoor04", CampPrisonDoor04, 0x30c,
                         "Missing trigger 'prisondoor04'! It's not in the LVL file!");
        BindLevelTrigger("prisondoor03", CampPrisonDoor03, 0x310,
                         "Missing trigger 'prisondoor03'! It's not in the LVL file!");
        return;
    case 0x12:
        BindLevelTrigger("AltarBox", RapaxMainFloorAltarBox, 0x29c,
                         "Missing trigger 'AltarBox'! It's not in the LVL file!");
        BindLevelTrigger("platformtrigger", RapaxMainFloorPlatform, 0x2a0,
                         "Missing trigger 'platformtrigger'! It's not in the LVL file!");
        BindLevelTrigger("platformtrigger01", RapaxMainFloorPlatform01, 0x2a4,
                         "Missing trigger 'platformtrigger01'! It's not in the LVL file!");
        BindLevelTrigger("platformtrigger02", RapaxMainFloorPlatform02, 0x2a8,
                         "Missing trigger 'platformtrigger02'! It's not in the LVL file!");
        return;
    case 0x13:
        BindLevelTrigger("AirBox", RapaxUpperFloorAirBox, 0x290,
                         "Missing trigger 'AirBox'! It's not in the LVL file!");
        BindLevelTrigger("DoorDone", RapaxUpperFloorDoorDone, 0x294,
                         "Missing trigger 'DoorDone'! It's not in the LVL file!");
        return;
    case 0x15:
        BindLevelTrigger("Fireantspawn", Rift1Fireantspawn, 0x27a,
                         "Missing trigger 'Fireantspawn'! It's not in the LVL file!");
        BindLevelTrigger("Sexspawn", Rift1Sexspawn, 0x27e,
                         "Missing trigger 'Sexspawn'! It's not in the LVL file!");
        BindLevelTrigger("Hotstuff", Rift1Hotstuff, 0x282,
                         "Missing trigger 'Hotstuff'! It's not in the LVL file!");
        BindLevelTrigger("Gate", Rift1Gate, 0x286,
                         "Missing trigger 'Gate'! It's not in the LVL file!");
        BindLevelTrigger("AshLock", Rift1AshLock, 0x5ee);
        BindLevelTrigger("TimeDorado", Rift1TimeDorado, 0x5ee);
        return;
    case 0x16:
        g_sea_caves_slope_override_enabled = true;
        g_party_has_slope_override_item = FindItemOnParty(0x254, 0, 0, 0, 0);
        BindLevelTrigger("HigardiChest01", SeaCavesHigardiChest01, 0x2b1,
                         "Missing trigger 'HigardiChest01'! It's not in the LVL file!");
        BindLevelTrigger("HigardiChest02", SeaCavesHigardiChest02, 0x2b5,
                         "Missing trigger 'HigardiChest02'! It's not in the LVL file!");
        BindLevelTrigger("HigardiChest03", SeaCavesHigardiChest03, 0x2b9,
                         "Missing trigger 'HigardiChest03'! It's not in the LVL file!");
        BindLevelTrigger("HigardiChest04", SeaCavesHigardiChest04, 0x2bd,
                         "Missing trigger 'HigardiChest04'! It's not in the LVL file!");
        BindLevelTrigger("HigardiChest05", SeaCavesHigardiChest05, 0x2c1,
                         "Missing trigger 'HigardiChest05'! It's not in the LVL file!");
        BindLevelTrigger("doortomb", SeaCavesDoorTomb, 0x5ee);
        return;
    case 0x18:
        BindLevelTrigger("gas_trig_plane01", SwampGasPlane, 0x2ca,
                         "Missing trigger 'gas_trig_plane01'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane02", SwampGasPlane, 0x2ce,
                         "Missing trigger 'gas_trig_plane02'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane03", SwampGasPlane, 0x2d2,
                         "Missing trigger 'gas_trig_plane03'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane04", SwampGasPlane, 0x2d6,
                         "Missing trigger 'gas_trig_plane04'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane05", SwampGasPlane, 0x2da,
                         "Missing trigger 'gas_trig_plane05'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane06", SwampGasPlane, 0x2de,
                         "Missing trigger 'gas_trig_plane06'! It's not in the LVL file!");
        BindLevelTrigger("gas_trig_plane07", SwampGasPlane, 0x2e2,
                         "Missing trigger 'gas_trig_plane07'! It's not in the LVL file!");
        BindLevelTrigger("oil_pool", SwampOilPool, 0x2e6,
                         "Missing trigger 'oil_pool'! It's not in the LVL file!");
        BindLevelTrigger("onelid", SwampOnelid, 0x2ea,
                         "Missing trigger 'onelid'! It's not in the LVL file!");
        pTrigger = FindTriggerByName("fire_trig_plane01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane01")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane02")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane03")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane04");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane04")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane05");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane05")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane06");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane06")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane07");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane07")));
        }
        pTrigger->activation_callback = SwampFirePlane;
        return;
    case 0x19:
        BindLevelTrigger("Fount_randomFX", Trynnie1FountRandomFX, 0x2fa,
                         "Missing trigger 'Fount_randomFX'! It's not in the LVL file!");
        EnsureTrynnie1KilledVar();
        return;
    case 0x1a:
        BindLevelTrigger("GoodaVine_A", Trynnie2GoodaVineA, 0x31a,
                         "Missing trigger 'GoodaVine_A'! It's not in the LVL file!");
        BindLevelTrigger("GoodaVine_B", Trynnie2GoodaVineB, 0x31e,
                         "Missing trigger 'GoodaVine_B'! It's not in the LVL file!");
        BindLevelTrigger("Meat_Maker", Trynnie2MeatMaker, 0x322,
                         "Missing trigger 'Meat maker'! It's not in the LVL file!");
        BindLevelTrigger("Meat_Box", Trynnie2MeatBox, 0x326,
                         "Missing trigger 'Meat_Box! It's not in the LVL file!");
        BindLevelTrigger("Give_Zulu", Trynnie2GiveZulu, 0x32a,
                         "Missing trigger 'Give Zulu'! It's not in the LVL file!");
        BindLevelTrigger("URN_Trigger_01", Trynnie2UrnTrigger, 0x5ee);
        BindLevelTrigger("URN_Trigger_02", Trynnie2UrnTrigger, 0x5ee);
        BindLevelTrigger("URN_Trigger_03", Trynnie2UrnTrigger, 0x5ee);
        BindLevelTrigger("URN_Trigger_04", Trynnie2UrnTrigger, 0x5ee);
        EnsureTrynnie2KilledVar();
        return;
    case 0x1b:
        BindLevelTrigger("Liche", ConnectiveTissueLiche, 0xe7,
                         "Missing trigger 'Liche'! It's not in the LVL file!");
        return;
    case 0x24:
        SavantTowerSyncButtonBlocker();
        BindLevelTrigger("Triangle", SavantTowerShape, 0xf2,
                         "Missing trigger 'Triangle'! It's not in the LVL file!");
        BindLevelTrigger("Circle", SavantTowerShape, 0xf6,
                         "Missing trigger 'Circle'! It's not in the LVL file!");
        BindLevelTrigger("Square", SavantTowerShape, 0xfa,
                         "Missing trigger 'Square'! It's not in the LVL file!");
        BindLevelTrigger("Star", SavantTowerShape, 0xfe,
                         "Missing trigger 'Star'! It's not in the LVL file!");
        BindLevelTrigger("ButtonBlocker", SavantTowerButtonBlocker, 0x102,
                         "Missing trigger 'ButtonBlocker'! It's not in the LVL file!");
        return;
    }
    return;
}
