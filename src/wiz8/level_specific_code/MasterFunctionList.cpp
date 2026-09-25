#include <stdarg.h>
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
W8WorldCursorNode* g_active_cursor_node;

// GLOBAL: WIZ8 0x006834d8
W8Vector<W8MasterFunction>* g_master_functions;
// GLOBAL: WIZ8 0x006834dc
bool g_flag_006834dc;

/* SGP full-volume scale: CreateAndPlaySoundNode multiplies its clamped
   loudness fraction by this to get the node's base volume. */
// GLOBAL: WIZ8 0x005EC510
const float g_float_005ec510 = 127.0f;

/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
// FUNCTION: WIZ8 0x004D8E40
void RunMasterFunctions(void)
{
    int count = g_master_functions->GetCount();

    for (int index = 0; index < count; ++index) {
        (*g_master_functions->GetAt(index))(0);
        if (g_flag_006834dc != 0) {
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
                              unsigned char settle, unsigned char a, unsigned char b)
{
    W8MonsterGroup* group;
    srVector3T<float> position_copy;

    if (settle != 0) {
        position->y = SettlePositionToGround00420BD0(position, 0);
    }
    position_copy = *position;
    group = CreateGroup(monster_id, count, &position_copy, a, b, settle);
    if (group != 0) {
        SetMonsterGroupHostility(group, hostility, 0);
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
                                  float scale, unsigned char play_flag)
{
    if (volume > g_float_005ebb38) {
        volume = 1.0f;
    } else if (volume < g_float_005ebb34) {
        return 0;
    }

    stSound3D* sound = new stSound3D(sound_name, 0);
    if (sound != 0) {
        srVector3T<double> sound_position;
        sound_position.Set(position.x, position.y, position.z);
        sound->setLocation(sound_position);
        sound->volume = static_cast<int>(volume * g_float_005ec510);
        sound->falloff = scale * g_world_scale;
        sound->Play(play_flag, 1);
    }
    return sound;
}

/* One row of the cursor-node dispatch table at 0x006109F4, indexed by the
   node's type parameter (numbers_0c[2]). The dispatcher invokes a row as
   (command, node, context); the context slot normally carries the address of
   the command's argument byte, while command 4 smuggles the byte itself
   through it. */
typedef unsigned char (*W8WorldCursorNodeHandler)(int command, W8WorldCursorNode* node,
                                                  int context);

unsigned char WorldCursorNodeShowMessageOnce(int command, W8WorldCursorNode* node, int context);
unsigned char WorldCursorNodeShowContextMessage(int command, W8WorldCursorNode* node, int context);
unsigned char WorldCursorNodeShowMessage(int command, W8WorldCursorNode* node, int context);
unsigned char WorldCursorNodeApplyItemEffect004D9560(int command, W8WorldCursorNode* node,
                                                     int context);
unsigned char WorldCursorNodeMaleCharacterEvent(int command, W8WorldCursorNode* node, int context);
unsigned char IsMasterFunctionTypeEight(int command, W8WorldCursorNode* node, int context);
unsigned char WorldCursorNodeSeenBodies(int command, W8WorldCursorNode* node, int context);
unsigned char WorldCursorNodeApplyItemEffect004D96C0(int command, W8WorldCursorNode* node,
                                                     int context);
unsigned char WorldCursorNodePartyVoice(int command, W8WorldCursorNode* node, int context);

/* The character-event kind constants the cursor-node handlers queue. */
// GLOBAL: WIZ8 0x005EE5F4
const int g_character_event_kind_005ee5f4 = 0x1b;
// GLOBAL: WIZ8 0x005EE63C
const int g_character_event_kind_005ee63c = 0x2d;
// GLOBAL: WIZ8 0x005EE688
const int g_character_event_kind_005ee688 = 0x40;

// GLOBAL: WIZ8 0x006109F4
static W8WorldCursorNodeHandler const g_world_cursor_node_handlers[10] = {
    WorldCursorNodeShowMessageOnce,         /* type 0 */
    WorldCursorNodeShowMessageOnce,         /* type 1 */
    WorldCursorNodeShowContextMessage,      /* type 2 */
    WorldCursorNodeShowMessage,             /* type 3 */
    WorldCursorNodeApplyItemEffect004D9560, /* type 4 */
    WorldCursorNodeSeenBodies,              /* type 5 */
    WorldCursorNodeApplyItemEffect004D96C0, /* type 6 */
    IsMasterFunctionTypeEight,              /* type 7 */
    WorldCursorNodePartyVoice,              /* type 8 */
    WorldCursorNodeMaleCharacterEvent       /* type 9 */
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
    handled = 0;
    // Retail callers use both two and three arguments. Non-4 commands only
    // forward the optional argument slot's address; handlers do not read it.
    va_list arguments;
    va_start(arguments, command);
    // reinterpret-ok: retail passes the optional argument slot as context.
    context = reinterpret_cast<int>(arguments);
    if (GetFlag68F105() != 0) {
        va_end(arguments);
        return handled;
    }
    if (info == 0) {
        GetCameraPosition(&position);
    } else {
        position = info->p3D->GetPosition();
    }
    position.y = SettlePositionToGround00420BD0(&position, 0) + g_float_005ec3f8;
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
            handled = 1;
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
unsigned char WorldCursorNodeShowMessageOnce(int command, W8WorldCursorNode* node, int context)
{
    int message_id;
    const char* folder;
    int type;
    unsigned char enabled;
    unsigned char result;
    char* shown;
    int slot;
    char path[512];
    wchar_t text[2048];

    result = 0;
    message_id = GetWorldCursorNodeParameter(node, 0);
    folder = GetLevelFolderName(GetLoadedLevelID());
    type = GetWorldCursorNodeParameter(node, 2);
    enabled = static_cast<unsigned char>(GetWorldCursorNodeParameter(node, 1));
    if (enabled != 0 && g_status.search_mode == 0 &&
        g_status.party_modifiers_22e3.detect_secrets_46 == 0) {
        for (slot = 0; slot < W8_PARTY_SLOT_COUNT; ++slot) {
            if (g_status.buffers.XChar[slot].fOccupied != 0 &&
                CharacterHasTrait(g_status.buffers.Char + slot, 0xc)) {
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
        result = 1;
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
unsigned char WorldCursorNodeShowContextMessage(int command, W8WorldCursorNode* node, int context)
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
unsigned char WorldCursorNodeShowMessage(int command, W8WorldCursorNode* node, int context)
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
unsigned char WorldCursorNodeApplyItemEffect004D9560(int command, W8WorldCursorNode* node,
                                                     int context)
{
    if (command == 1) {
        ApplyItemEffectToRandomCharacter(g_character_event_kind_005ee5f4, -1, 0,
                                         g_effect_argument_005ed8c8);
    }
    return 1;
}

/* Type-9 nodes: entering while the fact-0x14c gate holds queues the male
   event for the indexed party member when that member is male. */
// FUNCTION: WIZ8 0x004D9590
unsigned char WorldCursorNodeMaleCharacterEvent(int command, W8WorldCursorNode* node, int context)
{
    W8Character* character;

    if (command == 1 && g_status.rpc_active_2489 != 0) {
        character = g_status.buffers.Char + g_status.sedexus_party_slot_247f;
        if (character->gender == W8_GENDER_MALE) {
            QueueCharacterEvent(character, g_character_event_kind_005ee63c, 0,
                                g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        }
    }
    return 1;
}

/* Predicate installed in the master-function callback table. */
// FUNCTION: WIZ8 0x004D95F0
unsigned char IsMasterFunctionTypeEight(int command, W8WorldCursorNode* node, int context)
{
    return command == 8;
}

/* Type-5 nodes: the Ascension Peak "seen bodies" check. Once all three relic
   items are on the party and neither blocking fact holds, queue the reaction
   and latch the AP_SeenBodies location variable. Retail runs the fact-0x133
   clear twice. */
// FUNCTION: WIZ8 0x004D9600
unsigned char WorldCursorNodeSeenBodies(int command, W8WorldCursorNode* node, int context)
{
    if (command == 1) {
        if (GetLocationVarIDByName("AP_SeenBodies") == -1) {
            if (CountAscensionPeakItems() == 3) {
                if (GetFact(0x5c) != 0 || GetFact(0x97) != 0) {
                    ApplyItemEffectToRandomCharacter(g_character_event_kind_005ee688, -1, 0,
                                                     g_effect_argument_005ed8c8);
                    CreateLocationVar("AP_SeenBodies", 1);
                    if (GetFact(0x133) != 0) {
                        SetFact(0x133, 0, 0);
                    }
                    if (GetFact(0x133) != 0) {
                        SetFact(0x133, 0, 0);
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
unsigned char WorldCursorNodeApplyItemEffect004D96C0(int command, W8WorldCursorNode* node,
                                                     int context)
{
    if (command == 1) {
        ApplyItemEffectToRandomCharacter(g_character_event_kind_005ee688, -1, 0,
                                         g_effect_argument_005ed8c8);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004D96F0
void ClearValue6834D4(void)
{
    g_active_cursor_node = 0;
}

/* Master-function values 0x10 and 0x26 select the same path once either of
   the two enabling facts has been set. */
// FUNCTION: WIZ8 0x004D9700
int NormalizeMasterFunctionValue(int value)
{
    if (value == 0x10 || value == 0x26) {
        if (GetFact(0x5b) == 0 && GetFact(0x1ce) == 0) {
            return 0x26;
        }
        value = 0x10;
    }
    return value;
}

#define MASTER_FUNCTION_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MasterFunctionList.cpp"

// GLOBAL: WIZ8 0x006834DD
bool g_flag_006834dd;
// GLOBAL: WIZ8 0x006109F0
bool g_flag_6109f0 = true;
// GLOBAL: WIZ8 0x006834E0
int g_value_6834e0;
// GLOBAL: WIZ8 0x00652DA5
bool g_flag_652da5;

/* Fill the away camp chest from the level-0x26 item records stored in the
   current save. Records that are not world-persistent move into the chest
   trigger's container item; persistent records are flattened back into the
   load vector (so their own groups are visited too) and then freed. */
// FUNCTION: WIZ8 0x004D9740
void LoadAwayCampChest(void)
{
    W8GrowableVector<W8WorldItem*> items(5);
    W8Prop* pChest;
    Trigger* pTrigger;
    W8WorldItem* pContainer;
    int index;

    pChest = FindPropByName(g_world, "AwayCampChest");
    if (pChest == 0) {
        srAssertFail("pChest", MASTER_FUNCTION_CPP, 0x5a4, 0);
    }
    pTrigger = pChest->GetValue18();
    if (pTrigger == 0) {
        srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5a6, 0);
    }
    pContainer = pTrigger->GetOrCreateItemGroup(1);
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
unsigned char WorldCursorNodePartyVoice(int command, W8WorldCursorNode* node, int context)
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
            slot = PickRandomPartySpeaker(event_type, -1);
            if (slot != -1) {
                QueueCharacterEvent(g_status.buffers.Char + slot, event_type, 0,
                                    g_effect_argument_005ed8c8, g_effect_argument_005ed914);
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
    g_flag_006834dd = false;
    g_flag_6109f0 = true;
    g_value_6834e0 = level;
    g_flag_652da5 = false;
    switch (level) {
    case 0:
        pTrigger = FindTriggerByName("ChaosMolori");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x74,
                         "Missing trigger 'ChaosMolori'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaChaosMolori;
        pTrigger = FindTriggerByName("Maddmook");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x78,
                         "Missing trigger 'Maddmook'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaMaddmook;
        pTrigger = FindTriggerByName("CMbox");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x7c,
                         "Missing trigger 'CMbox'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaCMbox;
        pTrigger = FindTriggerByName("AstralDominae");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x80,
                         "Missing trigger 'AstralDominae'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaAstralDominae;
        pTrigger = FindTriggerByName("BallSlot");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x84,
                         "Missing trigger 'BallSlot'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaBallSlot;
        pTrigger = FindTriggerByName("Flightrecordertrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x88,
                         "Missing trigger 'Flightrecordertrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaFlightRecorder;
        pTrigger = FindTriggerByName("ULLspawn");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x8c,
                         "Missing trigger 'ULLspawn'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AllowTriggerActivation;
        pTrigger = FindTriggerByName("Mookholo");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x90,
                         "Missing trigger 'Mookholo'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaMookholo;
        pTrigger = FindTriggerByName("MookFrontDoor");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x9c,
                         "Missing trigger 'MookFrontDoor'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaMookFrontDoor;
        pTrigger = FindTriggerByName("YellowButton");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xa0,
                         "Missing trigger 'YellowButton'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaYellowButton;
        pTrigger = FindTriggerByName("Vaultalarmdoor");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xa4,
                         "Missing trigger 'Vaultalarmdoor'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaVaultAlarmDoor;
        pTrigger = FindTriggerByName("Exitbutton");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xa8,
                         "Missing trigger 'Exitbutton'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaExitButton;
        pTrigger = FindTriggerByName("GenVault-2-door");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xac,
                         "Missing trigger 'GenVault-2-door'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaGenVaultDoor;
        pTrigger = FindTriggerByName("ARN11");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xb8,
                         "Missing trigger 'ARN11'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = CosmicCircleReturnFalse;
        pTrigger = FindTriggerByName("RedButton");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xbd,
                         "Missing trigger 'RedButton'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaRedButton;
        pTrigger = FindTriggerByName("El1-TopButtons");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xc1,
                         "Missing trigger 'El1-TopButtons'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaEl1TopButtons;
        pTrigger = FindTriggerByName("El1-BottomButtons");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xc5,
                         "Missing trigger 'El1-BottomButtons'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaEl1BottomButtons;
        pTrigger = FindTriggerByName("GreenButton");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xca,
                         "Missing trigger 'GreenButton'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaGreenButton;
        pTrigger = FindTriggerByName("Elevator-02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xce,
                         "Missing trigger 'Elevator-02'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaElevator02Trigger;
        pTrigger = FindTriggerByName("LazerScanner");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xd8,
                         "Missing trigger 'LazerScanner'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ArnikaLazerScanner;
        pTrigger = FindTriggerByName("ScannerDoor");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xdc,
                         "Missing trigger 'ScannerDoor'! It's not in the LVL file!");
        }
        pTrigger->m_lData1 = 0;
        pTrigger->activation_callback_360 = ArnikaScannerDoor;
        ArnikaLevelSetup();
        return;
    case 1:
        pTrigger = FindTriggerByName("RampUp");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x116,
                         "Missing trigger 'RampUp'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionRampUp;
        pTrigger = FindTriggerByName("ChaosATrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x11a,
                         "Missing trigger 'ChaosATrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionChaosATrigger;
        pTrigger = FindTriggerByName("ChaosBTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x11e,
                         "Missing trigger 'ChaosBTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionChaosBTrigger;
        pTrigger = FindTriggerByName("LifeATrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x122,
                         "Missing trigger 'LifeATrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionLifeATrigger;
        pTrigger = FindTriggerByName("LifeBTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x126,
                         "Missing trigger 'LifeBTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionLifeBTrigger;
        pTrigger = FindTriggerByName("KnowATrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x12a,
                         "Missing trigger 'KnowATrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionKnowATrigger;
        pTrigger = FindTriggerByName("KnowBTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x12e,
                         "Missing trigger 'KnowBTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionKnowBTrigger;
        pTrigger = FindTriggerByName("RampUp");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x132,
                         "Missing trigger 'RampUp'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionDarkSavantSpawn;
        pTrigger = FindTriggerByName("Path1Camera");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x136,
                         "Missing trigger 'Path1Camera'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = AscensionPath1Camera;
        pTrigger = FindTriggerByName("Shaker");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!", "Shaker")));
        }
        pTrigger->activation_callback_360 = AscensionShaker;
        AscensionPeakInit();
        return;
    case 4:
        CosmicCircleSetup();
        pTrigger = FindTriggerByName("CC_TRIGGERPLANE1HEDRA");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x33a,
                         "Missing trigger 'CC_TRIGGERPLANE1HEDRA'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = CosmicCircleTriggerPlane1Hedra;
        break;
    case 5:
        MartensBluff1Setup();
        pTrigger = FindTriggerByName("MR109");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x144,
                         "Missing trigger 'MR109'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1Teleporter;
        pTrigger = FindTriggerByName("MR110");
        if (pTrigger != 0) {
            pTrigger->activation_callback_360 = BlockTriggerActivation;
        }
        pTrigger = FindTriggerByName("MR111");
        if (pTrigger != 0) {
            pTrigger->activation_callback_360 = BlockTriggerActivation;
        }
        pTrigger = FindTriggerByName("MR112");
        if (pTrigger != 0) {
            pTrigger->activation_callback_360 = BlockTriggerActivation;
        }
        pTrigger = FindTriggerByName("F-Handlock");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x154,
                         "Missing trigger 'F-Handlock'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1FHandlock;
        pTrigger = FindTriggerByName("ButtonGigas");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x158,
                         "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1ButtonGigas;
        pTrigger = FindTriggerByName("ButtonTrang");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x15c,
                         "Missing trigger 'ButtonTrang'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1ButtonTrang;
        pTrigger = FindTriggerByName("ButtonRift");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x160,
                         "Missing trigger 'ButtonRift'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1ButtonRift;
        pTrigger = FindTriggerByName("ButtonMaten");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x164,
                         "Missing trigger 'ButtonMaten'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1ButtonMaten;
        pTrigger = FindTriggerByName("WireTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x168,
                         "Missing trigger 'WireTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1WireTrigger;
        pTrigger = FindTriggerByName("ButtonGigas");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x16c,
                         "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1ButtonGigas;
        pTrigger = FindTriggerByName("Controller");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x170,
                         "Missing trigger 'Controller'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1Controller;
        pTrigger = FindTriggerByName("Dial-A");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x174,
                         "Missing trigger 'Dial-A'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1DialA;
        pTrigger = FindTriggerByName("Dial-B");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x178,
                         "Missing trigger 'Dial-B'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1DialB;
        pTrigger = FindTriggerByName("Dial-C");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x17c,
                         "Missing trigger 'Dial-C'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1DialC;
        pTrigger = FindTriggerByName("Gas-Switch");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x180,
                         "Missing trigger 'Gas-Switch'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1GasSwitch;
        pTrigger = FindTriggerByName("J-Doorcontroller");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x184,
                         "Missing trigger 'J-Doorcontroller'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1JDoorController;
        pTrigger = FindTriggerByName("MartenBook");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x188,
                         "Missing trigger 'trigger16254'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff1MartenBook;
        return;
    case 6:
        MartensBluff2Setup();
        pTrigger = FindTriggerByName("Arrowtraptrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x192,
                         "Missing trigger 'Arrowtraptrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = TriggerArrowTrap;
        pTrigger = FindTriggerByName("Spikeballtrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x196,
                         "Missing trigger 'Spikeballtrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2Spikeballtrigger;
        pTrigger = FindTriggerByName("DoorBolt");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x19a,
                         "Missing trigger 'DoorBolt'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2DoorBolt;
        pTrigger = FindTriggerByName("DummyLever");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x19e,
                         "Missing trigger 'DummyLever'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2DummyLever;
        pTrigger = FindTriggerByName("Dummy");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1a2,
                         "Missing trigger 'Dummy'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2Dummy;
        pTrigger = FindTriggerByName("PerfumeBox");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1a6,
                         "Missing trigger 'PerfumeBox'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2PerfumeBox;
        pTrigger = FindTriggerByName("StoneIdol");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1aa,
                         "Missing trigger 'StoneIdol'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2StoneIdol;
        pTrigger = FindTriggerByName("BlueFlowers");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1ae,
                         "Missing trigger 'BlueFlowers'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2BlueFlowers;
        pTrigger = FindTriggerByName("SquisherControls");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1b2,
                         "Missing trigger 'SquisherControls'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2SquisherControls;
        pTrigger = FindTriggerByName("DoorControls");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1b6,
                         "Missing trigger 'DoorControls'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MartensBluff2DoorControls;
        return;
    case 8:
        ClearTextForBarTrigger();
        pTrigger = FindTriggerByName("roach_trigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1c0,
                         "Missing trigger 'roach_trigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnRoachTriggerActivated;
        pTrigger = FindTriggerByName("spider_trigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1c4,
                         "Missing trigger 'spider_trigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnSpiderTriggerActivated;
        pTrigger = FindTriggerByName("Bartrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1c8,
                         "Missing trigger 'Bartrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnBarTriggerActivated;
        pTrigger = FindTriggerByName("Coffinlide");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1cc,
                         "Missing trigger 'Coffinlide'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnCoffinlideActivated;
        pTrigger = FindTriggerByName("Coffinlidg");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1d0,
                         "Missing trigger 'Coffinlidg'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnCoffinlidgActivated;
        pTrigger = FindTriggerByName("wheel_star");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "wheel_star")));
        }
        pTrigger->activation_callback_360 = OnWheelStarActivated;
        return;
    case 9:
        pTrigger = FindTriggerByName("bell_button");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1d8,
                         "Missing trigger 'bell_button'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Monastery2BellButton;
        pTrigger = FindTriggerByName("micro_door2");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1dc,
                         "Missing trigger 'micro_door2'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Monastery2MicroDoor2;
        return;
    case 0xc:
        MtGigas1Setup();
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT1");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x219,
                         "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas1Lift1;
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT2");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x21d,
                         "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas1Lift2;
        pTrigger = FindTriggerByName("PRESSUREPLATE");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x221,
                         "Missing trigger '_VOC_EWAXXLIFT2'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas1PressurePlate;
        pTrigger = FindTriggerByName("mudWallTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x225,
                         "Missing trigger 'mudWallTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas1MudWall;
        return;
    case 0xd:
        MtGigas2Setup();
        pTrigger = FindTriggerByName("_VOC_EWAXXTRAIN");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x22e,
                         "Missing trigger '_VOC_EWAXXTRAIN'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2Train;
        pTrigger = FindTriggerByName("redwire");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x232,
                         "Missing trigger 'redwire'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2RedWire;
        pTrigger = FindTriggerByName("bluewire");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x236,
                         "Missing trigger 'bluewire'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2BlueWire;
        pTrigger = FindTriggerByName("yellowwire");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x23a,
                         "Missing trigger 'yellowwire'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2YellowWire;
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT3");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x23e,
                         "Missing trigger '_VOC_EWAXXLIFT3'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2Lift3;
        pTrigger = FindTriggerByName("_VOC_EWAXXTOPDOOR1");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x242,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2TopDoor1;
        pTrigger = FindTriggerByName("_VOC_EWAXXOFFICER1");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x246,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2Officer1;
        pTrigger = FindTriggerByName("_VOC_EWAXXOFFICER2");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x24a,
                         "Missing trigger '_VOC_EWAXXTOPDOOR1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2Officer2;
        pTrigger = FindTriggerByName("triggerPlaneLaserAlarm");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x24e,
                         "Missing trigger 'triggerPlaneLaserAlarm'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2LaserAlarm;
        pTrigger = FindTriggerByName("triggerPlaneLaserAlarm01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x252,
                         "Missing trigger 'triggerPlaneLaserAlarm01'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2LaserAlarm;
        pTrigger = FindTriggerByName("accessHatch");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x256,
                         "Missing trigger 'accessHatch'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2AccessHatch;
        pTrigger = FindTriggerByName("wiringMalfunction");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x25a,
                         "Missing trigger 'wiringMalfunction'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas2WiringMalfunction;
        return;
    case 0xe:
        ProcessFlagPosition();
        pTrigger = FindTriggerByName("_VOC_EWAXXLIFT1");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1ff,
                         "Missing trigger '_VOC_EWAXXLIFT1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = MtGigas1Lift1;
        pTrigger = FindTriggerByName("crank");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x203,
                         "Missing trigger 'crank'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnCrankTriggerActivated;
        pTrigger = FindTriggerByName("Security Button");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x207,
                         "Missing trigger 'Security Button'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnSecurityButtonActivated;
        pTrigger = FindTriggerByName("VOC_EWAXXSENTRYtrig");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x20b,
                         "Missing trigger 'VOC_EWAXXSENTRYtrig'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnSentryTriggerActivated;
        pTrigger = FindTriggerByName("ewaxxdoortrigger03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x20f,
                         "Missing trigger 'ewaxxdoortrigger03'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnEwaxxDoor03Activated;
        pTrigger = FindTriggerByName("dummytrigger");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "dummytrigger")));
        }
        pTrigger->activation_callback_360 = OnDummyTriggerActivated;
        return;
    case 0xf:
        pTrigger = FindTriggerByName("_VOC_EWAXXCANNON1");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1e2,
                         "Missing trigger '_VOC_EWAXXCANNON1'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnEwaxxCannon1Activated;
        pTrigger = FindTriggerByName("EwaxxLanding");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1ea,
                         "Missing trigger 'EwaxxLanding'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnEwaxxLandingActivated;
        pTrigger = FindTriggerByName("catchCord");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1ee,
                         "Missing trigger 'catchCord'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnCatchCordActivated;
        pTrigger = FindTriggerByName("painActivatorTrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1f2,
                         "Missing trigger 'painActivatorTrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnPainActivatorActivated;
        pTrigger = FindTriggerByName("_VOC_EWAXXTOPDOOR2");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x1f6,
                         "Missing trigger '_VOC_EWAXXTOPDOOR2'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = OnEwaxxTopDoor2Activated;
        return;
    case 0x10:
        if (GetFact(0x1ce) == 0) {
            LoadAwayCampChest();
            SetFact(0x1ce, 0, 0);
        }
        pTrigger = FindTriggerByName("prisondoor06");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x308,
                         "Missing trigger 'prisondoor06'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = CampPrisonDoor06;
        pTrigger = FindTriggerByName("prisondoor04");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x30c,
                         "Missing trigger 'prisondoor04'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = CampPrisonDoor04;
        pTrigger = FindTriggerByName("prisondoor03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x310,
                         "Missing trigger 'prisondoor03'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = CampPrisonDoor03;
        return;
    case 0x12:
        pTrigger = FindTriggerByName("AltarBox");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x29c,
                         "Missing trigger 'AltarBox'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxMainFloorAltarBox;
        pTrigger = FindTriggerByName("platformtrigger");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2a0,
                         "Missing trigger 'platformtrigger'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxMainFloorPlatform;
        pTrigger = FindTriggerByName("platformtrigger01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2a4,
                         "Missing trigger 'platformtrigger01'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxMainFloorPlatform01;
        pTrigger = FindTriggerByName("platformtrigger02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2a8,
                         "Missing trigger 'platformtrigger02'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxMainFloorPlatform02;
        return;
    case 0x13:
        pTrigger = FindTriggerByName("AirBox");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x290,
                         "Missing trigger 'AirBox'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxUpperFloorAirBox;
        pTrigger = FindTriggerByName("DoorDone");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x294,
                         "Missing trigger 'DoorDone'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = RapaxUpperFloorDoorDone;
        return;
    case 0x15:
        pTrigger = FindTriggerByName("Fireantspawn");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x27a,
                         "Missing trigger 'Fireantspawn'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Rift1Fireantspawn;
        pTrigger = FindTriggerByName("Sexspawn");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x27e,
                         "Missing trigger 'Sexspawn'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Rift1Sexspawn;
        pTrigger = FindTriggerByName("Hotstuff");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x282,
                         "Missing trigger 'Hotstuff'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Rift1Hotstuff;
        pTrigger = FindTriggerByName("Gate");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x286,
                         "Missing trigger 'Gate'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Rift1Gate;
        pTrigger = FindTriggerByName("AshLock");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!", "AshLock")));
        }
        pTrigger->activation_callback_360 = Rift1AshLock;
        pTrigger = FindTriggerByName("TimeDorado");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "TimeDorado")));
        }
        pTrigger->activation_callback_360 = Rift1TimeDorado;
        return;
    case 0x16:
        g_flag_652da5 = true;
        g_byte_652da6 = FindItemOnParty(0x254, 0, 0, 0, 0);
        pTrigger = FindTriggerByName("HigardiChest01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2b1,
                         "Missing trigger 'HigardiChest01'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SeaCavesHigardiChest01;
        pTrigger = FindTriggerByName("HigardiChest02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2b5,
                         "Missing trigger 'HigardiChest02'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SeaCavesHigardiChest02;
        pTrigger = FindTriggerByName("HigardiChest03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2b9,
                         "Missing trigger 'HigardiChest03'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SeaCavesHigardiChest03;
        pTrigger = FindTriggerByName("HigardiChest04");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2bd,
                         "Missing trigger 'HigardiChest04'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SeaCavesHigardiChest04;
        pTrigger = FindTriggerByName("HigardiChest05");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2c1,
                         "Missing trigger 'HigardiChest05'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SeaCavesHigardiChest05;
        pTrigger = FindTriggerByName("doortomb");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "doortomb")));
        }
        pTrigger->activation_callback_360 = SeaCavesDoorTomb;
        return;
    case 0x18:
        pTrigger = FindTriggerByName("gas_trig_plane01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2ca,
                         "Missing trigger 'gas_trig_plane01'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2ce,
                         "Missing trigger 'gas_trig_plane02'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2d2,
                         "Missing trigger 'gas_trig_plane03'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane04");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2d6,
                         "Missing trigger 'gas_trig_plane04'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane05");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2da,
                         "Missing trigger 'gas_trig_plane05'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane06");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2de,
                         "Missing trigger 'gas_trig_plane06'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("gas_trig_plane07");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2e2,
                         "Missing trigger 'gas_trig_plane07'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampGasPlane;
        pTrigger = FindTriggerByName("oil_pool");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2e6,
                         "Missing trigger 'oil_pool'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampOilPool;
        pTrigger = FindTriggerByName("onelid");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2ea,
                         "Missing trigger 'onelid'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SwampOnelid;
        pTrigger = FindTriggerByName("fire_trig_plane01");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane01")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane02");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane02")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane03");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane03")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane04");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane04")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane05");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane05")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane06");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane06")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        pTrigger = FindTriggerByName("fire_trig_plane07");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                         reinterpret_cast<const char*>(
                             String(/* reinterpret-ok: SGP rotating debug buffer */
                                    "Missing trigger '%s'! It's not in the LVL file!",
                                    "fire_trig_plane07")));
        }
        pTrigger->activation_callback_360 = SwampFirePlane;
        return;
    case 0x19:
        pTrigger = FindTriggerByName("Fount_randomFX");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x2fa,
                         "Missing trigger 'Fount_randomFX'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie1FountRandomFX;
        EnsureTrynnie1KilledVar();
        return;
    case 0x1a:
        pTrigger = FindTriggerByName("GoodaVine_A");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x31a,
                         "Missing trigger 'GoodaVine_A'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie2GoodaVineA;
        pTrigger = FindTriggerByName("GoodaVine_B");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x31e,
                         "Missing trigger 'GoodaVine_B'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie2GoodaVineB;
        pTrigger = FindTriggerByName("Meat_Maker");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x322,
                         "Missing trigger 'Meat maker'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie2MeatMaker;
        pTrigger = FindTriggerByName("Meat_Box");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x326,
                         "Missing trigger 'Meat_Box! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie2MeatBox;
        pTrigger = FindTriggerByName("Give_Zulu");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x32a,
                         "Missing trigger 'Give Zulu'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = Trynnie2GiveZulu;
        pTrigger = FindTriggerByName("URN_Trigger_01");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_01")));
        }
        pTrigger->activation_callback_360 = Trynnie2UrnTrigger;
        pTrigger = FindTriggerByName("URN_Trigger_02");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_02")));
        }
        pTrigger->activation_callback_360 = Trynnie2UrnTrigger;
        pTrigger = FindTriggerByName("URN_Trigger_03");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_03")));
        }
        pTrigger->activation_callback_360 = Trynnie2UrnTrigger;
        pTrigger = FindTriggerByName("URN_Trigger_04");
        if (pTrigger == 0) {
            srAssertFail(
                "pTrigger", MASTER_FUNCTION_CPP, 0x5ee,
                reinterpret_cast<const char*>(
                    String(/* reinterpret-ok: SGP rotating debug buffer */
                           "Missing trigger '%s'! It's not in the LVL file!", "URN_Trigger_04")));
        }
        pTrigger->activation_callback_360 = Trynnie2UrnTrigger;
        EnsureTrynnie2KilledVar();
        return;
    case 0x1b:
        pTrigger = FindTriggerByName("Liche");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xe7,
                         "Missing trigger 'Liche'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = ConnectiveTissueLiche;
        return;
    case 0x24:
        SavantTowerSyncButtonBlocker();
        pTrigger = FindTriggerByName("Triangle");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xf2,
                         "Missing trigger 'Triangle'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SavantTowerShape;
        pTrigger = FindTriggerByName("Circle");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xf6,
                         "Missing trigger 'Circle'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SavantTowerShape;
        pTrigger = FindTriggerByName("Square");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xfa,
                         "Missing trigger 'Square'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SavantTowerShape;
        pTrigger = FindTriggerByName("Star");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0xfe,
                         "Missing trigger 'Star'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SavantTowerShape;
        pTrigger = FindTriggerByName("ButtonBlocker");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", MASTER_FUNCTION_CPP, 0x102,
                         "Missing trigger 'ButtonBlocker'! It's not in the LVL file!");
        }
        pTrigger->activation_callback_360 = SavantTowerButtonBlocker;
        return;
    }
    return;
}
