#include "wiz8/bink_video.h"

#include "bink.h"
#include "cursor.h"
#include "input.h"
#include "random.h"
#include "soundman.h"

#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/NpcDialog.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/level_specific_code/Ascension.h"
#include "wiz8/message_box.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/notices.h"
#include "wiz8/location_variables.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/regions.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"

#include "FileMan.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

struct W8NpcScriptRegionName {
    wchar_t name[50];
    int region;
};

// GLOBAL: WIZ8 0x0061b788
W8NpcScriptRegionName g_npc_script_region_names[] = {{L"Monastery", 1},
                                                     {L"Arnika", 2},
                                                     {L"Trynton", 3},
                                                     {L"Swamp", 4},
                                                     {L"Marten's Bluff", 5},
                                                     {L"Mine Tunnels", 6},
                                                     {L"Sea Caves", 7},
                                                     {L"Bayjin", 8},
                                                     {L"Rapax Castle", 9},
                                                     {L"Rapax Rift", 10},
                                                     {L"Mt. Gigas", 11},
                                                     {L"Ascension Peak", 12},
                                                     {L"Rapax Away Camp", 13},
                                                     {L"Cosmic Circle", 14},
                                                     {L"", 0}};

// FUNCTION: WIZ8 0x00528f10
bool GetNpcScriptRegionName(int region, wchar_t* name)
{
    for (int index = 0; g_npc_script_region_names[index].name[0] != 0; ++index) {
        if (g_npc_script_region_names[index].region == region) {
            wcscpy(name, g_npc_script_region_names[index].name);
            return true;
        }
    }
    return false;
}

/* The named-person table FindNpcNameOrPlaceQuote resolves keywords against:
   the mask bit the speaking NPC's record must carry to answer as that person,
   and the quote id the keyword maps to. */
struct W8NpcNamedQuote {
    wchar_t name[0x32];
    unsigned int mask;
    int quote;
};

// GLOBAL: WIZ8 0x0061aea8
W8NpcNamedQuote g_npc_named_quotes_0061aea8[] = {
    {L"Phoozang", 0x4000, 116},    {L"Cosmic Lords", 0x8000, 116}, {L"Yamir", 0x10000, 111},
    {L"Z'Ant", 0x20000, 109},      {L"Vi Domina", 0x40000, 106},   {L"Al-Sedexus", 0x80000, 113},
    {L"Astral Dominae", 1, 106},   {L"Destinae Dominus", 2, 112},  {L"Chaos Moliri", 4, 106},
    {L"Helm of Serinity", 8, 107}, {L"Mook", 0x10, 106},           {L"Umpani", 0x20, 111},
    {L"T'Rang", 0x40, 109},        {L"Higardi", 0x80, 106},        {L"Trynnie", 0x100, 107},
    {L"Rapax", 0x200, 114},        {L"Rynjin", 0x400, 114},        {L"Rattkin", 0x800, 107},
    {L"Dark Savant", 0x1000, 106}, {L"Marten", 0x2000, 112},
};
/* No full sentinel row exists in retail: the name[0] scan in
   FindNpcNameOrPlaceQuote reads one row past the end, where the following
   zero word happens to terminate the loop. */

/* Region-by-level-band quote table: one 0x12-int row per
   g_npc_script_region_names region id; band 0 is unused. */
// GLOBAL: WIZ8 0x0061bda0
int g_npc_region_quotes_0061bda0[15][0x12] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, 117, 210, 210, 210, 210, 210, 213, 210, 211, 211, 211, 211, 213, 210, -1, 210, 213},
    {-1, 211, 117, 210, 210, 210, 210, 213, 210, 213, 213, 211, 211, 213, 210, -1, 210, 213},
    {-1, 211, 211, 117, 213, 213, 213, 213, 213, 213, 213, 211, 211, 213, 213, -1, 212, 213},
    {-1, 211, 212, 212, 117, 210, 213, 213, 211, 211, 211, 211, 211, 213, 211, -1, 212, 211},
    {-1, 211, 212, 212, 211, 117, 211, 213, 211, 211, 211, 211, 211, 213, 211, -1, 212, 211},
    {-1, 211, 212, 212, 212, 212, 117, 213, 212, 211, 211, 211, 211, 213, 212, -1, 212, 211},
    {-1, 213, 212, 212, 212, 212, 212, 117, 212, 211, 211, 211, 211, 213, 212, -1, 212, 211},
    {-1, 211, 212, 212, 210, 210, 210, 213, 117, 211, 211, 212, 212, 213, 210, -1, 212, 213},
    {-1, 212, 212, 210, 210, 210, 210, 210, 212, 117, 210, 212, 212, 210, 210, -1, 212, 211},
    {-1, 212, 212, 210, 210, 210, 210, 210, 210, 211, 117, 212, 212, 210, 210, -1, 212, 211},
    {-1, 210, 210, 210, 210, 210, 210, 210, 210, 213, 213, 117, 213, 210, 210, -1, 210, 210},
    {-1, 212, 210, 210, 210, 210, 210, 210, 210, 213, 213, 212, 117, 210, 210, -1, 212, 212},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 211, 211, 211, 117, -1, -1, 212, 211},
    {-1, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 212, 212},
};

/* Find the quote index of the entry that consumes `item_id`: kind 0x0b takes
   the item, kind 0x0f merely names it; `grants_item` reports which when given.
   Both out-pointers are optional. */
// FUNCTION: WIZ8 0x00528CD0
int FindNpcScriptItemQuote(int item_id, short* index, unsigned char* grants_item)
{
    int entry_index;
    int quote_index;

    for (quote_index = 0;
         quote_index < static_cast<int>(g_npc_scripting.npc->script_file->quote_count);
         ++quote_index) {
        W8NpcScriptQuote* quote = g_npc_scripting.npc->script_file->quotes + quote_index;
        for (entry_index = 0; entry_index < static_cast<int>(quote->entry_count); ++entry_index) {
            W8NpcQuoteEntry* entry = quote->entries + entry_index;
            if ((entry->kind_00 == 0xb || entry->kind_00 == 0xf) && entry->operand_01 == item_id) {
                if (index != 0) {
                    *index = static_cast<short>(entry_index);
                }
                if (grants_item == 0) {
                    return quote_index;
                }
                *grants_item = entry->kind_00 != 0xf;
                return quote_index;
            }
        }
    }
    return -1;
}

/* Resolve an NPC-name, named-person, or region keyword to the quote id the
   speaker answers with; -1 when nothing applies. The record's alias mask
   selects which named persons this NPC will speak for. */
// FUNCTION: WIZ8 0x00528D50
int FindNpcNameOrPlaceQuote(W8NpcState* npc, wchar_t* text)
{
    int index;
    int band;

    if (CompareWideTextIgnoreAsciiCase00402920(npc->record->source_name_004, text) == 0) {
        return 0x75;
    }
    for (index = 0; g_npc_named_quotes_0061aea8[index].name[0] != 0; ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(text, g_npc_named_quotes_0061aea8[index].name) ==
            0) {
            if ((npc->record->name_alias_mask_060 & g_npc_named_quotes_0061aea8[index].mask) == 0) {
                return -1;
            }
            return g_npc_named_quotes_0061aea8[index].quote;
        }
    }
    if (CompareWideTextIgnoreAsciiCase00402920(text, L"Trang") == 0 &&
        (npc->record->name_alias_mask_060 & 0x40) != 0) {
        return 0x6d;
    }
    for (index = 0; g_npc_script_region_names[index].name[0] != 0; ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(text, g_npc_script_region_names[index].name) ==
            0) {
            band = GetLevelBand(g_status_685170.current_level);
            return g_npc_region_quotes_0061bda0[g_npc_script_region_names[index].region][band];
        }
    }
    if (CompareWideTextIgnoreAsciiCase00402920(text, L"Mt Gigas") == 0 ||
        CompareWideTextIgnoreAsciiCase00402920(text, L"Gigas") == 0) {
        band = GetLevelBand(g_status_685170.current_level);
        return g_npc_region_quotes_0061bda0[11][band];
    }
    if (CompareWideTextIgnoreAsciiCase00402920(text, L"the swamp") == 0) {
        band = GetLevelBand(g_status_685170.current_level);
        return g_npc_region_quotes_0061bda0[4][band];
    }
    if (CompareWideTextIgnoreAsciiCase00402920(text, L"the Monastery") != 0) {
        return -1;
    }
    band = GetLevelBand(g_status_685170.current_level);
    return g_npc_region_quotes_0061bda0[1][band];
}

/* Remove one stack unit of the matching item: the character backpacks first
   (the slot whose address equals `item` wins, or any slot carrying `item_id`
   when `match_item_id` is set), then the party pool, then the pending
   trade item. The first hit announces the loss through the quote bubble or
   the layout-4 path and returns. */
// FUNCTION: WIZ8 0x00528FF0
void RemoveNpcScriptItem(W8ItemInstance* item, int match_item_id, int item_id)
{
    wchar_t text[200];
    char sound_path[128];
    SOUNDPARMS sound_parms;
    unsigned int slot;
    unsigned int index;
    W8ItemInstance* slot_item;

    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }
        for (index = 0; index < 8; ++index) {
            slot_item = &g_status_685170.buffers.Char[slot].backpack[index];
            if (slot_item->iItemNo != -1 &&
                (slot_item == item || (match_item_id != 0 && slot_item->iItemNo == item_id))) {
                if (slot_item->stack_count != 0) {
                    --slot_item->stack_count;
                }
                if (slot_item->stack_count == 0) {
                    EmptyItemRecord(slot_item, &g_status_685170.buffers.Char[slot], 1);
                }
                swprintf(text, gppStringList[0x7ec], g_status_685170.buffers.Char[slot].name);
                if (g_screen_state_00649f1c->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
                    RebuildNpcTradeItemList005ADB10(0);
                    return;
                }
                SetNpcQuoteBubbleVisible(true, text, 0, -1, 0x47);
                g_npc_scripting.voice_playing = 0;
                g_npc_scripting.message_duration_ms = 2000;
                g_npc_scripting.last_tick = GetTickCount();
                g_npc_scripting.message_started_at = GetTickCount();
                memset(&sound_parms, 0xff, sizeof(SOUNDPARMS));
                g_npc_scripting.quote_active = 1;
                sprintf(sound_path, "Data\\Sound\\misc\\startgame.wav");
                sound_parms.EOSCallback = 0;
                SoundPlay(sound_path, &sound_parms);
                return;
            }
        }
    }
    for (slot = 0; slot < static_cast<unsigned int>(g_status_685170.party_item_count_1791);
         ++slot) {
        slot_item = &g_status_685170.party_item_pool_0021[slot];
        if (slot_item->iItemNo != -1 &&
            (slot_item == item || (match_item_id != 0 && slot_item->iItemNo == item_id))) {
            if (slot_item->stack_count != 0) {
                --slot_item->stack_count;
            }
            if (slot_item->stack_count == 0) {
                EmptyPartyPoolEntry00521CD0(slot);
            }
            swprintf(text, gppStringList[0x7ed]);
            if (g_screen_state_00649f1c->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
                RebuildNpcTradeItemList005ADB10(0);
                return;
            }
            SetNpcQuoteBubbleVisible(true, text, 0, -1, 0x47);
            g_npc_scripting.voice_playing = 0;
            g_npc_scripting.message_duration_ms = 2000;
            g_npc_scripting.last_tick = GetTickCount();
            g_npc_scripting.message_started_at = GetTickCount();
            memset(&sound_parms, 0xff, sizeof(SOUNDPARMS));
            g_npc_scripting.quote_active = 1;
            sprintf(sound_path, "Data\\Sound\\misc\\startgame.wav");
            sound_parms.EOSCallback = 0;
            SoundPlay(sound_path, &sound_parms);
            return;
        }
    }
    if (g_screen_state_00649f1c->held_item_pending != 0 && item != 0 &&
        g_screen_state_00649f1c->pending_item_1ed.iItemNo == item->iItemNo) {
        if (item->stack_count != 0) {
            --item->stack_count;
        }
        if (item->stack_count == 0) {
            g_screen_state_00649f1c->held_item_pending = 0;
            if (gXStatus.fNpcDialogueMode == 0) {
                ClearHeldItemDisplay();
            }
        }
    }
}

// FUNCTION: WIZ8 0x00528f60
void StripNpcKeywordPunctuation(wchar_t* text)
{
    wchar_t stripped[200];
    int length = static_cast<int>(wcslen(text));
    int count = 0;
    for (int index = 0; index < length; ++index) {
        wchar_t character = text[index];
        if (character != L'!' && character != L'?' && character != L'@' && character != L'#' &&
            character != L'$' && character != L',' && character != L'.' && character != L'"' &&
            character != L':') {
            stripped[count++] = character;
        }
    }
    stripped[count] = 0;
    wcscpy(text, stripped);
}

// GLOBAL: WIZ8 0x0068c3c4
int g_staged_value_68c3c4;
// GLOBAL: WIZ8 0x0068c3c8
int g_staged_value_68c3c8;
// GLOBAL: WIZ8 0x0068c3ce
short g_staged_short_68c3ce;
// GLOBAL: WIZ8 0x0068c3d0
unsigned char g_staged_flag_68c3d0;
// GLOBAL: WIZ8 0x0068c3d8
W8NpcScriptFile* g_staged_value_68c3d8;
// GLOBAL: WIZ8 0x0068c3dc
W8NpcState* g_staged_npc_68c3dc;

/* Alternates between the two string-list ids SpeakNpcSubquote substitutes for
   an "EMPTY"/"BLANK" quote on the closing quotes it allows. */
// GLOBAL: WIZ8 0x0068C4FC
static int g_empty_quote_text_index;

// GLOBAL: WIZ8 0x0068c500
unsigned char g_npc_script_event_active;
// GLOBAL: WIZ8 0x0068C501
unsigned char g_message_queue_idle_68c501;

// GLOBAL: WIZ8 0x0068C358
int g_pending_npc_travel_level;

// GLOBAL: WIZ8 0x0061aea0
int g_sedexus_sound_handle_61aea0 = -1;
// GLOBAL: WIZ8 0x0061c324
const char g_sedexus_moaning_sound_0061c324[] = "Data\\Sound\\Ambients\\Al_Sedexus Moaning.wav";
// GLOBAL: WIZ8 0x00614b44
const wchar_t g_format_al_s_00614b44[] = L"Al-%s";

// GLOBAL: WIZ8 0x005EE634
int g_effect_005ee634 = 43;

/* Quote indices 'F'..'R' fall outside the scripted world-action dispatch in
   RunNpcScriptLine's kind-0x0d/0x14 entries. */
// GLOBAL: WIZ8 0x005EE6A0
int g_world_action_quote_min_005ee6a0 = 'F';
// GLOBAL: WIZ8 0x005EE6D0
int g_world_action_quote_max_005ee6d0 = 'R';

/* Local Code\NPC Scripting.cpp. The NPC-scripting flag gates the scripted
   monster state; the four accessors below are its only owners. */

// FUNCTION: WIZ8 0x00524BD0
void FormatNpcVoiceSoundPath(W8NpcState* npc, char* output)
{
    const char* name = GetNpcDisplayName(npc);

    if (npc->is_grouped != 0 && g_npc_script_event_active == 0) {
        sprintf(output, "RPC_%s", name);
    } else if (npc->record->voice_script_2ea != 0) {
        sprintf(output, "VOC_%s", name);
    } else {
        sprintf(output, "NPC_%s", name);
    }
}

// FUNCTION: WIZ8 0x00524CA0
void ReloadNpcScriptResources(W8NpcState* npc)
{
    const char* name = GetNpcDisplayName(npc);
    const char* prefix;
    char script_name[128];
    char resource_name[128];

    if (npc->is_grouped != 0 && g_npc_script_event_active == 0) {
        prefix = "RPC_%s";
    } else if (npc->record->voice_script_2ea != 0) {
        prefix = "VOC_%s";
    } else {
        prefix = "NPC_%s";
    }
    sprintf(script_name, prefix, name);
    sprintf(resource_name, "Data\\NPC Scripts\\%s.nsf", script_name);
    npc->script_file = LoadNpcScriptFile0055A480(resource_name);
    if (npc->script_file != 0) {
        W8Monster* monster = GetNpcMonster(npc);
        if (monster != 0) {
            sprintf(resource_name, "%s.msf", script_name);
            monster->SetScript004C7F10(resource_name, 1);
            unsigned int list_index = MonsterGetIndexByLocationID(
                0x315, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster->location_id_1e4, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(list_index);
            if (monster_info != 0) {
                GetMonsterDataForInfo(monster_info);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00524DA0
void UpdateNpcDialogueVoiceAndCursor(void)
{
    DWORD tick_count;
    W8Monster* monster;

    if (g_npc_scripting.portrait_message_active != 0) {
        tick_count = GetTickCount();
        if (g_npc_scripting.message_duration_ms < tick_count - g_npc_scripting.message_started_at) {
            FinishNpcVoicePlayback(1);
        }
    } else if (g_npc_scripting.quote_active != 0) {
        if (g_npc_scripting.voice_playing == 0) {
            tick_count = GetTickCount();
            if (g_npc_scripting.message_duration_ms <
                tick_count - g_npc_scripting.message_started_at) {
                FinishNpcVoicePlayback(1);
            }
        }
        if (g_npc_scripting.voice_playing != 0 && g_npc_scripting.npc->is_grouped == 0) {
            UpdateMouthGapTrack(g_npc_scripting.voice_handle, &g_npc_scripting.gap_track);
            monster = GetNpcMonster(g_npc_scripting.npc);
            if (monster != 0) {
                monster->mouth_open = g_npc_scripting.gap_track.mouth_open;
            }
        }
    }

    if (gXStatus.fNpcDialogueMode != 0) {
        if (g_screen_state_00649f1c->dialogue_hidden != 0) {
            SetTargetCursor(1);
            return;
        }
        if (g_npc_scripting.quote_active == 0) {
            if (g_npc_scripting.message_lines.GetCount() < 1) {
                if (gXStatus.iCurrentCursor != 8) {
                    SetTargetCursor(-1);
                    return;
                }
            }
        } else if (gXStatus.scripted_scene_19b7 == 0) {
            SetTargetCursor(9);
        }
    }
}

// FUNCTION: WIZ8 0x00524EB0
void ProcessNpcScriptingFrame(void)
{
    W8Character* character;
    unsigned char can_open_dialogue;
    char dialogue_ready;
    int environment;
    int party_slot;
    int selected_party_member;
    SOUNDPARMS local_sound_parms;

    if ((g_npc_scripting.sedexus_capture_pending != 0 ||
         g_npc_scripting.sedexus_release_pending != 0) &&
        ((environment = GetEnvironmentValue0060A3A8(), environment == 0) ||
         (environment = GetEnvironmentValue0060A3A8(), environment == 2))) {
        g_npc_scripting.scripted_scene_active = 0;
        if (g_npc_scripting.sedexus_release_pending == 0) {
            memset(&local_sound_parms, 0xff, sizeof(SOUNDPARMS));
            g_sedexus_sound_handle_61aea0 = (int)SoundPlayStreamedFile(
                (STR)g_sedexus_moaning_sound_0061c324, &local_sound_parms);
        } else {
            ClearMainGameTargetState();
            selected_party_member = g_status_685170.selected_party_member_2434;
            for (party_slot = 0; party_slot < 8; ++party_slot) {
                W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
                character = &g_status_685170.buffers.Char[party_slot];
                if (row->fOccupied != 0 &&
                    ((character->hp_current > 0 || character->highest_condition < 0x12) &&
                     party_slot != selected_party_member)) {
                    RemoveCharacterCondition(party_slot, 0x11, 0);
                    selected_party_member = g_status_685170.selected_party_member_2434;
                }
            }
            character = &g_status_685170.buffers.Char[selected_party_member];
            if (character->gender == W8_GENDER_MALE) {
                QueueCharacterEvent(character, g_effect_005ee634, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
            g_npc_scripting.sedexus_capture_active = 0;
            if (g_sedexus_sound_handle_61aea0 != -1) {
                SoundStop((unsigned int)g_sedexus_sound_handle_61aea0);
                g_sedexus_sound_handle_61aea0 = -1;
            }
        }
        g_npc_scripting.sedexus_capture_pending = 0;
        g_npc_scripting.sedexus_release_pending = 0;
    }
    if (g_npc_scripting.scripted_scene_active == 0 && g_npc_scripting.quote_active == 0 &&
        g_npc_scripting.portrait_message_active == 0) {
        if (g_screen_state_00649f1c->script_busy < 1) {
            ProcessMessageBoxQueue();
            if (g_npc_scripting.quote_active != 0) {
                return;
            }
            if (g_npc_scripting.portrait_message_active != 0) {
                return;
            }
        }
        if (g_npc_scripting.message_lines.GetCount() == 0) {
            if (g_npc_scripting.restore_staged_session != 0) {
                g_npc_scripting.staging_restore.staged_short_49e = g_staged_short_68c3ce;
                g_npc_scripting.quote_active = g_staged_flag_68c3d0;
                g_npc_scripting.staging_restore.finished_quote_index = g_staged_value_68c3c8;
                g_npc_scripting.script_file = g_staged_value_68c3d8;
                g_npc_scripting.npc = g_staged_npc_68c3dc;
                g_npc_scripting.staging_restore.current_quote_index = g_staged_value_68c3c4;
                g_npc_scripting.restore_staged_session = 0;
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_status_685170.world_cursor_gate_2435 == 0 &&
                gXStatus.scripted_scene_19b7 == 0 && g_screen_state_00649f1c->script_busy == 0 &&
                (can_open_dialogue = CanOpenNpcDialogue(), can_open_dialogue != 0)) {
                EndNpcDialogueSession0056E800(0);
            }
            if (g_screen_state_00649f1c->script_busy == 0 &&
                g_screen_state_00649f1c->dialogue_panel_hidden != 0 &&
                (dialogue_ready = gXStatus.character_event_queue->IsMainQueueEmpty(),
                 dialogue_ready != 0)) {
                SetNpcDialoguePanelVisible(1);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00525110
void BeginNpcScriptDialogue(W8NpcState* npc, unsigned char preserve_state)
{
    if (preserve_state != 0) {
        g_npc_scripting.restore_staged_session = 1;
        g_staged_short_68c3ce = g_npc_scripting.staging_restore.staged_short_49e;
        g_staged_flag_68c3d0 = g_npc_scripting.quote_active;
        g_staged_value_68c3c8 = g_npc_scripting.staging_restore.finished_quote_index;
        g_staged_value_68c3d8 = g_npc_scripting.script_file;
        g_staged_npc_68c3dc = g_npc_scripting.npc;
        g_staged_value_68c3c4 = g_npc_scripting.staging_restore.current_quote_index;
    }
    if (npc->has_monster == 0) {
        BindNpcToMonster(npc->name_style, 0, -1);
    }
    g_npc_scripting.staging_restore.staged_short_49e = 0;
    g_npc_scripting.quote_active = 0;
    g_npc_scripting.staging_restore.finished_quote_index = -1;
    g_npc_scripting.npc = npc;
    g_npc_scripting.script_file = npc->script_file;
}

/* End-of-sound callback SOUNDPARMS arms on the NPC voice. A natural stream
   end finishes the quote but leaves the session active for a 700 ms hold;
   the guard keeps FinishNpcVoicePlayback's manual SoundStop from recursing
   back through here. */
// FUNCTION: WIZ8 0x005251C0
static void NpcVoiceEosCallback(void* callback_data)
{
    if (g_npc_scripting.stopping_voice_playback == 0) {
        FinishNpcVoicePlayback(0);
        g_npc_scripting.quote_active = 1;
        g_npc_scripting.message_started_at = GetTickCount();
        g_npc_scripting.message_duration_ms = 700;
    }
}

/* Evaluate a kind-3 quote entry: each sub-entry pair is a (fact, expected)
   guard that must all hold, then the entry selects the response - operand_01
   directly, randomly in [operand_01, operand_09] when operand_05 is 2, or a
   queued QUOTE_ENTRY continuation line when operand_09 is 4. Returns the
   response index, -1 when a guard fails, and -2 after queueing. */
// FUNCTION: WIZ8 0x005251F0
int SelectNpcQuoteResponse(W8NpcQuoteEntry* entry)
{
    int index;
    unsigned char expected;
    char response_count;
    W8MessageBoxLine* line;

    index = 0;
    if (entry->operand_09 == 4 && entry->operand_05 != 2) {
        index = 1;
    }
    while (index < entry->sub_entry_count) {
        expected = (unsigned char)entry->sub_entries[index + 1]
                       .operand_00 /* c-style-cast-ok: packed byte operand */;
        if (GetFact(entry->sub_entries[index].operand_00) != expected) {
            return -1;
        }
        index += 2;
    }
    if (entry->operand_09 == 4 && entry->operand_05 != 2) {
        line = new W8MessageBoxLine;
        memset(line, 0, sizeof(W8MessageBoxLine));
        line->quote_entry = entry;
        line->type = W8_NPC_MSG_QUOTE_ENTRY;
        line->quote_index = -1;
        line->continuation_quote = 0;
        line->npc = g_npc_scripting.npc;
        g_npc_scripting.message_lines.Add(line);
        return -2;
    }
    if (entry->operand_05 == 2) {
        response_count = entry->operand_09 - entry->operand_01 + 1;
        return Random(response_count) + entry->operand_01;
    }
    return entry->operand_01;
}

/* Speak one subquote of the staged quote: build the voice path under
   Data\Sound\<PCs|NPCs>\<stem>\<stem>_<quote>[a..].wav, pick the display and
   plain text (EMPTY/BLANK/UNKNOWN/CLASSIFIED/SOUND are sentinels), start the
   sound with the EOS callback armed, and hang the mouth-gap track on the
   party slot's monster-manager entry or on g_npc_scripting for ungrouped
   NPCs. notice_only routes the text through ShowNotice instead. */
// FUNCTION: WIZ8 0x00525350
void SpeakNpcSubquote(W8NpcScriptQuote* quote, unsigned char subquote_index,
                      unsigned char notice_only, unsigned char force_npc_voice)
{
    W8MessageBoxLine* line;
    W8MonsterManagerEntry* entry;
    W8Monster* monster;
    int length;
    int quote_index;
    char voice_dir[12];
    SOUNDPARMS voice_parms;
    SOUNDPARMS fallback_parms;
    char voice_path[128];
    char voice_stem[128];
    char npc_name[64];
    char fallback_path[128];
    wchar_t display_text[2048];
    wchar_t plain_text[2048];
    wchar_t prefixed_text[2040];
    unsigned int voice_total_ms;
    unsigned int voice_position_ms;

    static const int s_empty_quote_text_ids[2] = {1867, 1868};

    if (quote->dialogue_alert != 0) {
        sprintf(voice_path, "Data\\Sound\\NPCs\\Dialogue Alert.wav");
        swprintf(display_text, L"%S", quote->subquotes[subquote_index]);
        if (notice_only == 0) {
            g_npc_scripting.portrait_message_active = 1;
            SetNpcQuoteBubbleVisible(1, display_text, 0, -1, -1);
            g_npc_scripting.message_duration_ms = wcslen(display_text) * 60 + 2000;
            g_npc_scripting.message_started_at = GetTickCount();
            return;
        }
        ShowNotice(0xf, display_text, 0, GetTextBoxScrollRange(), 0);
        return;
    }

    sprintf(npc_name, "%S", g_npc_scripting.npc->record->source_name_004);
    if (g_npc_scripting.npc->is_grouped != 0 && force_npc_voice == 0) {
        sprintf(voice_dir, "PCs");
    } else {
        sprintf(voice_dir, "NPCs");
    }
    if (g_npc_scripting.npc->is_grouped != 0 && g_npc_script_event_active == 0) {
        sprintf(voice_stem, "RPC_%s", GetNpcDisplayName(g_npc_scripting.npc));
    } else if (g_npc_scripting.npc->record->voice_script_2ea != 0) {
        sprintf(voice_stem, "VOC_%s", GetNpcDisplayName(g_npc_scripting.npc));
    } else {
        sprintf(voice_stem, "NPC_%s", GetNpcDisplayName(g_npc_scripting.npc));
    }
    sprintf(voice_path, "Data\\Sound\\%s\\%s\\%s_%03d", voice_dir, voice_stem, voice_stem,
            g_npc_scripting.staging_restore.current_quote_index);
    if (subquote_index > 0) {
        length = strlen(voice_path);
        voice_path[length] = subquote_index + 'a' - 1;
        voice_path[length + 1] = '\0';
    }
    strcat(voice_path, ".wav");
    if (quote->subquotes == 0) {
        swprintf(display_text, L" Missing quote #%d for %s.",
                 g_npc_scripting.staging_restore.current_quote_index,
                 g_npc_scripting.npc->record->source_name_004);
        swprintf(plain_text, L" Missing quote #%d for %s.",
                 g_npc_scripting.staging_restore.current_quote_index,
                 g_npc_scripting.npc->record->source_name_004);
    } else {
        swprintf(display_text, L" \"%S\"", quote->subquotes[subquote_index]);
        swprintf(plain_text, L"%S", quote->subquotes[subquote_index]);
    }
    if (wcslen(plain_text) == 0) {
        return;
    }
    if (notice_only == 0) {
        if (CompareWideTextIgnoreAsciiCase00402920(plain_text, L"EMPTY") == 0 ||
            CompareWideTextIgnoreAsciiCase00402920(plain_text, L"BLANK") == 0) {
            quote_index = g_npc_scripting.staging_restore.current_quote_index;
            if (quote_index != 0x67 && quote_index != 0x68 && quote_index != 0x69) {
                return;
            }
            swprintf(plain_text, gppStringList[s_empty_quote_text_ids[g_empty_quote_text_index]],
                     g_npc_scripting.npc->record->source_name_004);
            g_empty_quote_text_index = g_empty_quote_text_index + 1;
            if (g_empty_quote_text_index == 2) {
                g_empty_quote_text_index = 0;
            }
            SetNpcQuoteBubbleVisible(1, plain_text, 0, -1, 0x47);
            g_npc_scripting.voice_playing = 0;
            g_npc_scripting.message_duration_ms = 2000;
            g_npc_scripting.last_tick = GetTickCount();
            g_npc_scripting.message_started_at = GetTickCount();
            memset(&fallback_parms, 0xff, sizeof(fallback_parms));
            g_npc_scripting.quote_active = 1;
            sprintf(fallback_path, "Data\\Sound\\misc\\startgame.wav");
            fallback_parms.EOSCallback = 0;
            SoundPlay(fallback_path, &fallback_parms);
            return;
        }
        if (CompareWideTextIgnoreAsciiCase00402920(plain_text, L"UNKNOWN") == 0) {
            quote_index = Random(100) < 50 ? 0x1c : 0x1d;
            line = new W8MessageBoxLine;
            memset(line, 0, sizeof(W8MessageBoxLine));
            line->quote_index = quote_index;
            line->mark_pending = 0;
            line->suppress_entries = 0;
            line->npc = g_npc_scripting.npc;
            g_npc_scripting.message_lines.Add(line);
            return;
        }
        if (CompareWideTextIgnoreAsciiCase00402920(plain_text, L"CLASSIFIED") == 0) {
            line = new W8MessageBoxLine;
            memset(line, 0, sizeof(W8MessageBoxLine));
            line->quote_index = 0x1e;
            line->mark_pending = 0;
            line->suppress_entries = 0;
            line->npc = g_npc_scripting.npc;
            g_npc_scripting.message_lines.Add(line);
            return;
        }
        if (CompareWideTextIgnoreAsciiCase00402920(plain_text, L"SOUND") != 0 &&
            g_npc_scripting.npc->is_grouped == 0) {
            if (gXStatus.fNpcDialogueMode == 0 &&
                g_npc_scripting.npc->record->voice_script_2ea == 0) {
                swprintf(prefixed_text, L"%s: %s", g_npc_scripting.npc->record->source_name_004,
                         display_text);
                wcscpy(display_text, prefixed_text);
            }
            SetNpcQuoteBubbleVisible(1, display_text, 0,
                                     g_npc_scripting.staging_restore.current_quote_index, -1, 0, 0,
                                     g_npc_scripting.npc->partner_index_2c);
        }
        memset(&voice_parms, 0xff, sizeof(voice_parms));
        voice_parms.uiVolume = g_settings_6850c8.voice_volume * 70 / 100;
        voice_parms.EOSCallback = NpcVoiceEosCallback;
        g_npc_scripting.voice_handle = SoundPlay(voice_path, &voice_parms);
        monster = GetNpcMonster(g_npc_scripting.npc);
        if (monster != 0) {
            monster->StartTalking004C73F0(1);
        }
        g_npc_scripting.last_tick = GetTickCount();
        if (g_npc_scripting.voice_handle == -1) {
            g_npc_scripting.voice_playing = 0;
            g_npc_scripting.message_duration_ms = wcslen(display_text) * 60 + 2000;
            g_npc_scripting.message_started_at = GetTickCount();
            entry = GetNpcGroupEntry(g_npc_scripting.npc);
            if (entry != 0) {
                SetPartyPortraitEventState(g_npc_scripting.npc->group_index, 1,
                                           g_npc_scripting.staging_restore.current_quote_index,
                                           display_text, 1);
                entry->voice_sound_handle = -1;
                entry->voice_time_remaining_ms = g_npc_scripting.message_duration_ms;
            }
        } else {
            g_npc_scripting.voice_playing = 1;
            entry = GetNpcGroupEntry(g_npc_scripting.npc);
            if (entry != 0) {
                SetPartyPortraitEventState(g_npc_scripting.npc->group_index, 1,
                                           g_npc_scripting.staging_restore.current_quote_index,
                                           display_text, 1);
                entry->voice_sound_handle = g_npc_scripting.voice_handle;
                SoundGetMilliSecondPosition(g_npc_scripting.voice_handle, &voice_total_ms,
                                            &voice_position_ms);
                entry->voice_time_remaining_ms = voice_total_ms;
                LoadMouthGapTrack(
                    voice_path,
                    &gXStatus.monster_manager_entries[g_npc_scripting.npc->group_index].mouth_gap);
                g_status_685170.buffers.XChar[g_npc_scripting.npc->group_index]
                    .pending_event_type_ff = g_npc_scripting.staging_restore.current_quote_index;
                entry->pending_event_type_114 = g_npc_scripting.staging_restore.current_quote_index;
                g_screen_state_00649f1c->last_notice_npc_kind =
                    g_npc_scripting.npc->partner_index_2c;
            } else {
                LoadMouthGapTrack(voice_path, &g_npc_scripting.gap_track);
            }
        }
        g_npc_scripting.quote_active = 1;
        return;
    }
    swprintf(prefixed_text, L"%s: %s", g_npc_scripting.npc->record->source_name_004, display_text);
    wcscpy(display_text, prefixed_text);
    ShowNotice(0xf, display_text, 0, GetTextBoxScrollRange(), 0);
}

// FUNCTION: WIZ8 0x00525C50
void FinishNpcVoicePlayback(unsigned char resume_script)
{
    if (g_npc_scripting.portrait_message_active != 0) {
        g_npc_scripting.portrait_message_active = 0;
        SetNpcQuoteBubbleVisible(0, 0, 0, -1, -1);
        g_npc_scripting.staging_restore.finished_quote_index =
            g_npc_scripting.staging_restore.current_quote_index;
        return;
    }
    if (g_npc_scripting.quote_active != 0) {
        if (g_npc_scripting.voice_playing != 0) {
            g_npc_scripting.voice_playing = 0;
            if (g_npc_scripting.voice_handle != -1) {
                g_npc_scripting.stopping_voice_playback = 1;
                SoundStop(static_cast<unsigned int>(g_npc_scripting.voice_handle));
                g_npc_scripting.stopping_voice_playback = 0;
            }
            FreeMouthGapTrack(&g_npc_scripting.gap_track);
        }
        W8Monster* monster = GetNpcMonster(g_npc_scripting.npc);
        if (monster != 0) {
            monster->StopTalking004C7470();
        }
        W8MonsterManagerEntry* entry = GetNpcGroupEntry(g_npc_scripting.npc);
        if (entry != 0) {
            if (entry->active_character_event == 0) {
                SetPartyPortraitEventState(g_npc_scripting.npc->group_index, 0, -1, 0, 1);
            } else {
                gXStatus.character_event_queue->CompleteActiveEvent(entry->active_character_event);
            }
        }
        g_npc_scripting.quote_active = 0;
        g_npc_scripting.voice_handle = -1;
        g_npc_scripting.staging_restore.finished_quote_index =
            g_npc_scripting.staging_restore.current_quote_index;
        if (g_screen_state_00649f1c->script_busy == 0 && resume_script != 0) {
            if (g_npc_scripting.npc != 0 && g_npc_scripting.npc->script_file != 0 &&
                g_npc_scripting.staging_restore.current_quote_index <
                    g_npc_scripting.npc->script_file->quote_count) {
                W8NpcScriptQuote* quotes = g_npc_scripting.npc->script_file->quotes;
                SetNpcQuoteBubbleVisible(
                    0, 0, &quotes[g_npc_scripting.staging_restore.current_quote_index],
                    g_npc_scripting.staging_restore.current_quote_index, -1);
                return;
            }
            SetNpcQuoteBubbleVisible(0, 0, 0, -1, -1);
        }
    }
}

/* Guarded finish entry point: suppressed outright while the level-4 gate
   flag is set; a non-forced call inside 500 ms of the last script tick is a
   no-op, so the tick path can poll without re-finishing. */
// FUNCTION: WIZ8 0x00525D90
void TryFinishNpcVoicePlayback(unsigned char force)
{
    if (g_status_685170.world_cursor_gate_2435 == 0 || g_status_685170.current_level != 4) {
        if (force == 0 && GetTickCount() - g_npc_scripting.last_tick <= 500) {
            return;
        }
        FinishNpcVoicePlayback(1);
    }
}

// FUNCTION: WIZ8 0x00525DD0
bool IsNpcScriptSessionActive(void)
{
    return g_npc_scripting.quote_active != 0 || g_npc_scripting.portrait_message_active != 0;
}

/* Consume a world-view click while an NPC quote/portrait session is up: finish
   the voice playback (non-forced) and report the click as handled. */

// FUNCTION: WIZ8 0x00525DF0
bool ShouldDeferCharacterEventForNpcScript(unsigned char require_group_entry)
{
    if (g_npc_scripting.scripted_scene_active != 0) {
        return false;
    }
    if (g_npc_scripting.quote_active == 0 && g_npc_scripting.portrait_message_active == 0 &&
        g_npc_scripting.message_lines.GetCount() == 0) {
        return false;
    }
    if (g_npc_scripting.npc == 0) {
        return false;
    }
    if (GetNpcGroupEntry(g_npc_scripting.npc) != 0 && require_group_entry == 0) {
        return false;
    }
    return true;
}

// FUNCTION: WIZ8 0x00525E50
bool IsMessageBoxLineQueueEmpty(void)
{
    return g_npc_scripting.message_lines.GetCount() == 0;
}

// FUNCTION: WIZ8 0x00525E60
int ComputePortraitMessageDuration(wchar_t* text)
{
    return static_cast<int>(wcslen(text) * 0x3c + 2000);
}

/* Kind-4 entries carry the keyword strings a dialogue keyword can select;
   translate the keyword, then scan every quote for a subentry whose text
   matches, reporting the entry and subentry indices through the out-pointers. */
// FUNCTION: WIZ8 0x00525E80
int FindNpcScriptQuoteByKeyword(wchar_t* keyword, short* entry_index, short* sub_entry_index)
{
    W8NpcScriptFile* script;
    W8NpcScriptQuote* quote;
    W8NpcQuoteEntry* entry;
    wchar_t translated[2000];
    wchar_t text[2046];
    int quote_index;
    int item_index;
    int sub_index;

    script = g_npc_scripting.npc->script_file;
    TranslateDialogueKeyword0056C440(keyword, translated);
    for (quote_index = 0; quote_index < script->quote_count; quote_index++) {
        quote = &script->quotes[quote_index];
        for (item_index = 0; item_index < quote->entry_count; item_index++) {
            entry = &quote->entries[item_index];
            if (entry->kind_00 == 4) {
                for (sub_index = 0; sub_index < entry->sub_entry_count; sub_index++) {
                    swprintf(text, L"%S", entry->sub_entries[sub_index].text);
                    if (CompareWideTextIgnoreAsciiCase00402920(text, translated) == 0) {
                        if (entry_index != 0) {
                            *entry_index = static_cast<short>(item_index);
                        }
                        if (sub_entry_index != 0) {
                            *sub_entry_index = static_cast<short>(sub_index);
                        }
                        return quote_index;
                    }
                }
            }
        }
    }
    return -1;
}

/* Run one script quote line: stage it as current, and when the last subquote
   is reached first walk the quote's entries - each kind drives facts, NPC
   switches, items, gold, keyword learning or the follow-up quote. Then speak
   the current subquote and queue the continuation line. Quote indices
   'F'..'R' fall outside the scripted world-action dispatch. */
// FUNCTION: WIZ8 0x00525FA0
void RunNpcScriptLine(int script_line, unsigned char force_npc_voice)
{
    W8MessageBoxLine* line;
    W8NpcScriptQuote* quote;
    W8NpcQuoteEntry* entry;
    W8Monster* monster;
    W8NpcState* target;
    signed char faction;
    char response_count;
    int response;
    int entry_index;
    int index;
    bool finished;

    g_npc_scripting.staging_restore.current_quote_index = script_line;
    if (script_line != g_npc_scripting.staging_restore.finished_quote_index) {
        g_npc_scripting.staging_restore.subquote_index = 0;
    }
    if (script_line < g_npc_scripting.npc->script_file->quote_count) {
        quote = &g_npc_scripting.npc->script_file->quotes[script_line];
        if (g_npc_scripting.staging_restore.subquote_index == quote->subquote_count) {
            g_npc_scripting.staging_restore.subquote_index = 0;
        }
        response = -1;
        finished = false;
        if (g_npc_scripting.staging_restore.subquote_index == quote->subquote_count - 1 &&
            quote->entry_count != 0) {
            for (entry_index = 0; entry_index < quote->entry_count; entry_index++) {
                entry = &quote->entries[entry_index];
                switch (entry->kind_00) {
                case 2:
                    if (entry->operand_09 == 4) {
                        if (entry->operand_05 != 2) {
                            QueueNpcQuoteEntry(entry, 0, 0);
                            response = -1;
                            finished = true;
                            break;
                        }
                    } else if (entry->operand_05 != 2) {
                        response = entry->operand_01;
                        finished = true;
                        break;
                    }
                    response_count = static_cast<char>(entry->operand_09 - entry->operand_01 + 1);
                    response = Random(response_count) + entry->operand_01;
                    finished = true;
                    break;
                case 3:
                    response = SelectNpcQuoteResponse(entry);
                    if (response != -1) {
                        finished = true;
                        break;
                    }
                    break;
                case 5:
                    g_screen_state_00649f1c->script_busy = 0xff;
                    QueueNpcQuoteEntry(entry, 0, 0);
                    finished = true;
                    break;
                case 7:
                    SetFact(entry->operand_01, entry->operand_05, 0);
                    break;
                case 8:
                    AddMessageBoxLine(W8_NPC_MSG_CLOSE_DIALOGUE, 0, 0);
                    for (index = 0; index < g_npc_scripting.message_lines.GetCount(); index++) {
                        line = *g_npc_scripting.message_lines.GetAt(index);
                        if (line->type == W8_NPC_MSG_SHOW_DIALOGUE_PANEL) {
                            delete line;
                            g_npc_scripting.message_lines.RemoveAt(index);
                            break;
                        }
                    }
                    break;
                case 9:
                case 10:
                case 16:
                case 17:
                    QueueNpcQuoteEntry(entry, 0, 0);
                    break;
                case 12:
                    monster = GetNpcMonster(g_npc_scripting.npc);
                    if (monster != 0 && entry->operand_09 != 4 &&
                        monster->SetScriptLabel004CA260(entry->sub_entries->text) == 0) {
                        wchar_t script_error[100];
                        swprintf(script_error,
                                 L"Failed to load script (or failed to find %S in script)",
                                 entry->sub_entries->text);
                    }
                    break;
                case 13:
                    if (g_npc_scripting.staging_restore.current_quote_index <
                            g_world_action_quote_min_005ee6a0 ||
                        g_world_action_quote_max_005ee6d0 <
                            g_npc_scripting.staging_restore.current_quote_index) {
                        char action_name[52];
                        BeginScriptedWorldAction();
                        sprintf(action_name, "%s", entry->sub_entries->text + 4);
                        target = FindNpcStateByName(action_name);
                        if (target != 0) {
                            if (target->is_grouped == 0) {
                                ClearMainGameTargetState();
                            } else {
                                AddMessageBoxLine(
                                    W8_NPC_MSG_GROUP_ACTION,
                                    reinterpret_cast<wchar_t*>(static_cast<int>(
                                        target
                                            ->group_index)), /* reinterpret-ok: tagged int in text/argument dword */
                                    0);
                            }
                        }
                    }
                    break;
                case 14:
                    line = new W8MessageBoxLine;
                    memset(line, 0, sizeof(W8MessageBoxLine));
                    line->quote_index = -1;
                    line->type = W8_NPC_MSG_CLOSE_RESUME_NPC;
                    line->payload_10.raw = 0;
                    line->extra.raw = 0;
                    line->npc = g_npc_scripting.npc;
                    g_npc_scripting.message_lines.Add(line);
                    finished = true;
                    break;
                case 19:
                    g_screen_state_00649f1c->script_busy = 0xff;
                    QueueNpcQuoteEntry(entry, 0, 0);
                    finished = true;
                    break;
                case 20:
                    if (g_npc_scripting.staging_restore.current_quote_index <
                            g_world_action_quote_min_005ee6a0 ||
                        g_world_action_quote_max_005ee6d0 <
                            g_npc_scripting.staging_restore.current_quote_index) {
                        BeginScriptedWorldAction();
                        AddMessageBoxLine(
                            W8_NPC_MSG_GROUP_ACTION,
                            reinterpret_cast<wchar_t*>(static_cast<int>(
                                g_npc_scripting.npc
                                    ->group_index)), /* reinterpret-ok: tagged int in text/argument dword */
                            0);
                    }
                    break;
                case 21: {
                    char action_name[52];
                    sprintf(action_name, "%s", entry->sub_entries->text + 4);
                    target = FindNpcStateByName(action_name);
                    if (target != 0) {
                        ApplyNpcInteraction0050A570(target, 4, 0, 0, entry->operand_01);
                    }
                } break;
                case 22:
                    faction = FindFactionByName(entry->sub_entries->text);
                    if (faction != -1) {
                        ApplyFactionChange(3, 1, faction, entry->operand_01);
                    }
                    break;
                case 24: {
                    int event_type = entry->operand_01;
                    line = new W8MessageBoxLine;
                    memset(line, 0, sizeof(W8MessageBoxLine));
                    line->quote_index = -1;
                    line->type = W8_NPC_MSG_PARTY_SPEAKER_EVENT;
                    line->payload_10.argument = event_type;
                    line->extra.raw = 0;
                    line->npc = g_npc_scripting.npc;
                    g_npc_scripting.message_lines.Add(line);
                } break;
                case 25:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 1, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 1);
                        }
                    }
                    break;
                case 26:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 0, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 0);
                        }
                    }
                    break;
                case 27:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 2, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 2);
                        }
                    }
                    break;
                case 28:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 3, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 3);
                        }
                    }
                    break;
                case 31:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 1, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 1);
                        }
                    }
                    break;
                case 32:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 0, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 0);
                        }
                    }
                    break;
                case 33:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 2, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 2);
                        }
                    }
                    break;
                case 34:
                    if (g_settings_6850c8.simplified_npc_interaction != 0) {
                        wchar_t keyword_text[100];
                        swprintf(keyword_text, L"%S", entry->sub_entries->text);
                        if (gXStatus.fNpcDialogueMode == 0 ||
                            g_screen_state_00649f1c->scripted_dialogue == 0) {
                            AddNpcDialogueKeyword(keyword_text, 3, 1);
                        } else {
                            AddDialogueTranscriptKeyword(keyword_text, 3);
                        }
                    }
                    break;
                }
                if (finished) {
                    break;
                }
            }
        }
        if (g_npc_scripting.dialogue_cancelled_c4 != 0) {
            g_npc_scripting.dialogue_cancelled_c4 = 0;
            return;
        }
        if (response >= 0) {
            line = new W8MessageBoxLine;
            memset(line, 0, sizeof(W8MessageBoxLine));
            line->quote_index = response;
            line->mark_pending = 0;
            line->suppress_entries = 0;
            line->npc = g_npc_scripting.npc;
            g_npc_scripting.message_lines.Add(line);
        }
        SpeakNpcSubquote(quote, g_npc_scripting.staging_restore.subquote_index, 0, force_npc_voice);
        g_npc_scripting.staging_restore.subquote_index++;
        if (g_npc_scripting.staging_restore.current_quote_index == 0) {
            g_npc_scripting.npc->greeting_pending = 0;
        }
        if (g_npc_scripting.staging_restore.subquote_index < quote->subquote_count) {
            line = new W8MessageBoxLine;
            memset(line, 0, sizeof(W8MessageBoxLine));
            line->quote_index = g_npc_scripting.staging_restore.current_quote_index;
            line->mark_pending = 0;
            line->suppress_entries = 1;
            line->npc = g_npc_scripting.npc;
            g_npc_scripting.message_lines.InsertAt(0, line);
        }
    }
}

/* Execute a queued quote entry's deferred effect: switch the dialogue to the
   named NPC (staging the current session when the target is grouped), hand
   items or gold to the party with the notice bubble and chime, or open the
   modal dialog kinds. The party slot's pending portrait event clears once the
   entry has run. */
// FUNCTION: WIZ8 0x00526810
void ProcessNpcQuoteEntry(W8NpcQuoteEntry* entry, int continuation_quote)
{
    W8MessageBoxLine* line;
    W8NpcState* target;
    W8ItemInstance item;
    SOUNDPARMS sound_parms;
    char action_name[52];
    char sound_path[128];
    wchar_t notice_text[200];

    switch (entry->kind_00) {
    case 2:
        if (entry->operand_09 == 4 && entry->operand_05 != 2) {
            sprintf(action_name, "%s", entry->sub_entries->text + 4);
            target = FindNpcStateByName(action_name);
            if (target != 0) {
                if (target->name_style == 0x18 && entry->operand_01 == 0) {
                    g_npc_scripting.staging_restore.subquote_index = 0;
                }
                if (target->is_grouped == 0) {
                    SelectNpcDialogueSpeaker(target, 0);
                    if (g_status_685170.current_level != 4 ||
                        g_status_685170.world_cursor_gate_2435 == 0) {
                        LookAtDialogueNpc();
                    }
                } else {
                    g_staged_short_68c3ce = g_npc_scripting.staging_restore.staged_short_49e;
                    g_staged_flag_68c3d0 = g_npc_scripting.quote_active;
                    g_staged_value_68c3c8 = g_npc_scripting.staging_restore.finished_quote_index;
                    g_npc_scripting.restore_staged_session = 1;
                    g_staged_value_68c3d8 = g_npc_scripting.script_file;
                    g_staged_npc_68c3dc = g_npc_scripting.npc;
                    g_staged_value_68c3c4 = g_npc_scripting.staging_restore.current_quote_index;
                    if (target->has_monster == 0) {
                        BindNpcToMonster(target->name_style, 0, -1);
                    }
                    g_npc_scripting.staging_restore.staged_short_49e = 0;
                    g_npc_scripting.quote_active = 0;
                    g_npc_scripting.staging_restore.finished_quote_index = -1;
                    g_npc_scripting.script_file = target->script_file;
                    g_npc_scripting.npc = target;
                }
                line = new W8MessageBoxLine;
                memset(line, 0, sizeof(W8MessageBoxLine));
                line->quote_index = entry->operand_01;
                line->mark_pending = 0;
                line->suppress_entries = 0;
                line->npc = g_npc_scripting.npc;
                g_npc_scripting.message_lines.Add(line);
                if (target->is_grouped != 0) {
                    SetNpcDialoguePanelVisible(0);
                }
                FlushPendingNoticeLines005766B0();
            }
        }
        break;
    case 3:
        if (entry->operand_09 == 4 && entry->operand_05 != 2) {
            sprintf(action_name, "%s", entry->sub_entries->text + 4);
            target = FindNpcStateByName(action_name);
            if (target != 0) {
                if (target->is_grouped == 0) {
                    SelectNpcDialogueSpeaker(target, 0);
                    if (g_status_685170.current_level != 4 ||
                        g_status_685170.world_cursor_gate_2435 == 0) {
                        LookAtDialogueNpc();
                    }
                } else {
                    g_staged_short_68c3ce = g_npc_scripting.staging_restore.staged_short_49e;
                    g_staged_flag_68c3d0 = g_npc_scripting.quote_active;
                    g_staged_value_68c3c8 = g_npc_scripting.staging_restore.finished_quote_index;
                    g_npc_scripting.restore_staged_session = 1;
                    g_staged_value_68c3d8 = g_npc_scripting.script_file;
                    g_staged_npc_68c3dc = g_npc_scripting.npc;
                    g_staged_value_68c3c4 = g_npc_scripting.staging_restore.current_quote_index;
                    if (target->has_monster == 0) {
                        BindNpcToMonster(target->name_style, 0, -1);
                    }
                    g_npc_scripting.staging_restore.staged_short_49e = 0;
                    g_npc_scripting.quote_active = 0;
                    g_npc_scripting.staging_restore.finished_quote_index = -1;
                    g_npc_scripting.script_file = target->script_file;
                    g_npc_scripting.npc = target;
                    SetNpcDialoguePanelVisible(0);
                }
                line = new W8MessageBoxLine;
                memset(line, 0, sizeof(W8MessageBoxLine));
                line->quote_index = entry->operand_01;
                line->mark_pending = 0;
                line->suppress_entries = 0;
                line->npc = g_npc_scripting.npc;
                g_npc_scripting.message_lines.Add(line);
                if (target->is_grouped != 0) {
                    SetNpcDialoguePanelVisible(0);
                }
                FlushPendingNoticeLines005766B0();
            }
        }
        break;
    case 5:
    case 0x13:
        continuation_quote = -1;
        /* fall through */
    case 0x12:
    case 0x1e:
        OpenNpcDialog(entry, continuation_quote);
        break;
    case 9:
        ReplaceOrCreateItem(&item, entry->operand_01, 1, 1, 0);
        swprintf(notice_text, gppStringList[0x7ea], GetItemDisplayName(&item));
        if (gXStatus.fNpcDialogueMode == 0) {
            g_status_685170.item_in_hand_235b = item;
            SetItemCursor(0);
        } else {
            AddItemToPartyOrDrop(&item, 0);
            SetNpcQuoteBubbleVisible(1, notice_text, 0, -1, 0x47);
            g_npc_scripting.voice_playing = 0;
            g_npc_scripting.message_duration_ms = 2000;
            g_npc_scripting.last_tick = GetTickCount();
            g_npc_scripting.message_started_at = GetTickCount();
            memset(&sound_parms, 0xff, sizeof(sound_parms));
            g_npc_scripting.quote_active = 1;
            sprintf(sound_path, "Data\\Sound\\misc\\startgame.wav");
            sound_parms.EOSCallback = 0;
            SoundPlay(sound_path, &sound_parms);
        }
        ClearNpcItemId(g_npc_scripting.npc, entry->operand_01);
        if (g_screen_state_00649f1c->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            RebuildNpcTradeItemList005ADB10(0);
        }
        break;
    case 10:
        RemoveNpcScriptItem(0, 1, entry->operand_01);
        break;
    case 0x10:
        swprintf(notice_text, gppStringList[0x7e9], g_npc_scripting.npc->record->source_name_004,
                 entry->operand_01);
        AddPartyGold(entry->operand_01, 0);
        SetNpcQuoteBubbleVisible(1, notice_text, 0, -1, 0x47);
        g_npc_scripting.voice_playing = 0;
        g_npc_scripting.message_duration_ms = 2000;
        g_npc_scripting.last_tick = GetTickCount();
        g_npc_scripting.message_started_at = GetTickCount();
        g_npc_scripting.quote_active = 1;
        memset(&sound_parms, 0xff, sizeof(sound_parms));
        sprintf(sound_path, "Data\\Sound\\misc\\startgame.wav");
        sound_parms.EOSCallback = 0;
        SoundPlay(sound_path, &sound_parms);
        break;
    case 0x11:
        AwardPartyExperience004EEF10(entry->operand_01, 0);
        break;
    }
    if (g_npc_scripting.npc->character != 0 && g_npc_scripting.npc->group_index != -1) {
        g_status_685170.buffers.XChar[g_npc_scripting.npc->group_index].pending_event_type_ff = 0;
    }
}

/* The fade-completion callback NpcScriptTurnToBook schedules: while either
   queued endgame stage is pending - and stage three still finds PHOONZANG -
   ENDGAME2's script notice runs the book sequence. */
// FUNCTION: WIZ8 0x00526DF0
void NpcScriptQueueEndgame(void)
{
    if (g_status_685170.endgame2_queued != 0 ||
        (g_status_685170.endgame3_queued != 0 && FindNpcOfKind(W8_NPC_PHOONZANG))) {
        W8NpcState* npc = GetNpcStateByKind(W8_NPC_ENDGAME2);
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, -1, 0, 0);
        }
    }
}

/* CameraPath4 swings the view to the ascension book; the endgame queue runs
   when the fade completes. */
// FUNCTION: WIZ8 0x00526E40
void NpcScriptTurnToBook(void)
{
    UpdateCameraPathStateByName(GetWorld(), "CameraPath4", 1);
    BeginScreenFade(0, 1, 1000, NpcScriptQueueEndgame, 0, 0);
}

// FUNCTION: WIZ8 0x00526E70
void NpcScriptEndgameScreen(void)
{
    SetValue64D8AC(5);
    SetPendingScreenState(0);
}

// FUNCTION: WIZ8 0x00526E90
void ProcessMessageBoxQueue(void)
{
    W8MessageBoxLine* line;
    W8NpcState* npc;
    int index;

    if (g_npc_scripting.message_lines.GetCount() < 1) {
        if (g_message_queue_idle_68c501 == 0) {
            FlushPendingNoticeLines005766B0();
        }
        g_message_queue_idle_68c501 = 1;
        return;
    }

    line = *g_npc_scripting.message_lines.GetAt(0);
    npc = line->npc;
    if (npc != 0 && npc != g_npc_scripting.npc && line->type == W8_NPC_MSG_QUOTE) {
        BeginNpcScriptDialogue(npc, 1);
    }

    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
        index = 0;
        while (line == 0 && line->type != W8_NPC_MSG_QUOTE) {
            ++index;
            if (index == g_npc_scripting.message_lines.GetCount()) {
                return;
            }
            line = *g_npc_scripting.message_lines.GetAt(index);
        }
    }

    if (line->type == W8_NPC_MSG_QUOTE) {
        if (g_message_queue_idle_68c501 != 0) {
            g_message_queue_idle_68c501 = 0;
            if (g_npc_scripting.npc->record->voice_script_2ea == 0) {
                for (index = 0; index < g_npc_scripting.message_lines.GetCount(); ++index) {
                    W8MessageBoxLine* queued = *g_npc_scripting.message_lines.GetAt(index);
                    if (queued->type == W8_NPC_MSG_QUOTE &&
                        static_cast<char>(queued->suppress_entries) == 0) {
                        for (int pending = 0;
                             pending < g_npc_scripting.pending_script_values.GetCount();
                             ++pending) {
                            if (**g_npc_scripting.pending_script_values.GetAt(pending) ==
                                queued->quote_index) {
                                if (g_npc_scripting.npc->record->merchant_056 == 0 &&
                                    queued->quote_index != 0x76) {
                                    W8NpcScriptQuote* quote = &g_npc_scripting.npc->script_file
                                                                   ->quotes[queued->quote_index];
                                    int entry;
                                    for (entry = 0; entry < quote->entry_count; ++entry) {
                                        if (quote->entries[entry].kind_00 == 0x1d) {
                                            break;
                                        }
                                    }
                                    if (entry == quote->entry_count) {
                                        RunNpcScriptLine(0x1f, 0);
                                        return;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            for (index = 0; index < g_npc_scripting.pending_script_values.GetCount(); ++index) {
                delete *g_npc_scripting.pending_script_values.GetAt(index);
            }
            g_npc_scripting.pending_script_values.Clear();
        }

        line = *g_npc_scripting.message_lines.GetAt(0);
        if (static_cast<char>(line->mark_pending) != 0) {
            int* script_line = new int;
            *script_line = line->quote_index;
            g_npc_scripting.pending_script_values.Add(script_line);
        }

        W8NpcScriptFile* script = g_npc_scripting.npc->script_file;
        if (script != 0 && static_cast<char>(line->suppress_entries) == 0 &&
            line->quote_index < script->quote_count) {
            W8NpcScriptQuote* quote = &script->quotes[line->quote_index];
            for (index = 0; index < quote->entry_count; ++index) {
                unsigned char kind = quote->entries[index].kind_00;
                if ((kind == 0x12 || kind == 0x1e) &&
                    (kind == 0x1e || NpcKnowsFact(g_npc_scripting.npc, line->quote_index) == 0)) {
                    RunNpcScriptLine(0x12, 0);
                    g_screen_state_00649f1c->script_busy = 0xff;
                    W8MessageBoxLine* continuation = new W8MessageBoxLine;
                    memset(continuation, 0, sizeof(W8MessageBoxLine));
                    continuation->quote_index = -1;
                    continuation->quote_entry = &quote->entries[index];
                    continuation->type = W8_NPC_MSG_QUOTE_ENTRY;
                    continuation->continuation_quote = line->quote_index;
                    continuation->npc = g_npc_scripting.npc;
                    g_npc_scripting.message_lines.InsertAt(0, continuation);
                    g_npc_scripting.message_lines.Remove(line);
                    delete line;
                    return;
                }
            }
        }
        if (script != 0) {
            RunNpcScriptLine(line->quote_index, 0);
        }
        g_npc_scripting.message_lines.Remove(line);
        delete line;
        return;
    }

    switch (static_cast<int>(line->type)) {
    case W8_NPC_MSG_CLOSE_DIALOGUE:
        CloseNpcDialogueIfActive();
        g_flag_6109f0 = 1;
        break;
    case W8_NPC_MSG_QUOTE_ENTRY:
        ProcessNpcQuoteEntry(line->quote_entry, line->continuation_quote);
        break;
    case W8_NPC_MSG_REOPEN_TRANSCRIPT:
        CloseNpcDialogueLayout00570A20();
        OpenNpcDialogueTranscriptLayout();
        break;
    case W8_NPC_MSG_REMOVE_SCRIPT_ITEM:
        RemoveNpcScriptItem(line->payload_10.item, 0, -1);
        break;
    case W8_NPC_MSG_CLOSE_RESUME_NPC:
        CloseNpcDialogueIfActive();
        if (g_screen_state_00649f1c->dialogue_npc != 0) {
            ResumeNpc(g_screen_state_00649f1c->dialogue_npc, 1);
        }
        break;
    case W8_NPC_MSG_FOCUS_NPC: {
        int npc_kind = line->payload_10.argument;
        npc = GetNpcStateByKind(npc_kind);
        if (npc != 0) {
            RecruitNpcIntoParty(npc);
        }
        EndNpcDialogueSession0056E800(0);
        W8MessageBoxLine* continuation = new W8MessageBoxLine;
        memset(continuation, 0, sizeof(W8MessageBoxLine));
        continuation->npc = g_npc_scripting.npc;
        g_npc_scripting.message_lines.Add(continuation);
        break;
    }
    case W8_NPC_MSG_GROUP_ACTION: {
        int group = line->payload_10.argument;
        ClearMainGameTargetState();
        DismissNpcFromParty(group, 0, false, false);
        if (g_screen_state_00649f1c->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
            for (int party_slot = 0; party_slot < 8; ++party_slot) {
                if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
                    RegionSetDisable(party_slot + 7);
                    DisableRegionSetInput(party_slot + 7);
                }
            }
        }
        break;
    }
    case W8_NPC_MSG_JOURNAL_QUOTE:
        SetNpcQuoteBubbleVisible(1, gppStringList[0x74a], 0, -1, 0x47);
        g_npc_scripting.voice_playing = 0;
        g_npc_scripting.message_duration_ms = 2000;
        g_npc_scripting.last_tick = GetTickCount();
        g_npc_scripting.message_started_at = GetTickCount();
        g_npc_scripting.quote_active = 1;
        SoundPlay((STR) "Data\\Sound\\Misc\\Journal Entry.wav",
                  0); // c-style-cast-ok: released SGP textual API uses UINT8 pointer spelling
        break;
    case W8_NPC_MSG_PORTRAIT_STRING: {
        int string_index = line->payload_10.argument;
        g_npc_scripting.portrait_message_active = 1;
        SetNpcQuoteBubbleVisible(1, gppStringList[string_index], 0, -1, -1);
        g_npc_scripting.message_duration_ms =
            ComputePortraitMessageDuration(gppStringList[string_index]);
        g_npc_scripting.message_started_at = GetTickCount();
        break;
    }
    case W8_NPC_MSG_CALL_4DFAE0:
        SpawnAlfieChaos004DFAE0(0);
        break;
    case W8_NPC_MSG_CALL_4DFB40:
        SpawnAlfieLife004DFB40(0);
        break;
    case W8_NPC_MSG_CALL_4DFB80:
        SpawnAlfieKnow004DFB80(0);
        break;
    case W8_NPC_MSG_FINISH_ACTION:
        if (line->payload_10.raw == 0) {
            ClearMainGameTargetState();
        } else {
            BeginScriptedWorldAction();
        }
        break;
    case W8_NPC_MSG_PATH2_TRIGGER: {
        Trigger* trigger = FindTriggerByName("Path2Trigger");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        break;
    }
    case W8_NPC_MSG_MOVE_SAVANT: {
        EndNpcDialogueSession0056E800(0);
        ResetLevelDataVectors0041F0D0();
        W8MonsterGroup* group = FindFirstMonsterByID(0xc2);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x916, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("MoveSavant.msf", 1);
        }
        break;
    }
    case W8_NPC_MSG_MOVE_BELA: {
        EndNpcDialogueSession0056E800(0);
        ResetLevelDataVectors0041F0D0();
        W8MonsterGroup* group = FindFirstMonsterByID(0x18c);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb5e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("MoveBela.msf", 1);
        }
        srVector3T<float> position;
        if (FindEntityByName("NP_DSExit", &position, 0, 0)) {
            PointCameraAtTarget(&position, 0, 1);
        }
        break;
    }
    case W8_NPC_MSG_MOVE_GOLEM: {
        EndNpcDialogueSession0056E800(0);
        BeginScriptedWorldAction();
        W8MonsterGroup* group = FindFirstMonsterByID(0x13e);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb1b, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("MoveGolem.msf", 1);
        }
        break;
    }
    case W8_NPC_MSG_ALETHEIDES_LEAVES:
        RemoveAletheides();
        break;
    case W8_NPC_MSG_SKILL_NOTICES: {
        W8SkillNoticePayload* skill_changes = line->extra.skill_notices;
        if (g_settings_6850c8.skill_increase_messages == 0) {
            for (index = 0; index < skill_changes->count; ++index) {
                int party_slot = skill_changes->party_slots[index];
                int skill = skill_changes->skills[index];
                W8Character* character = &g_status_685170.buffers.Char[party_slot];
                unsigned int value = character->skills[skill].points_02;
                if (skill == g_profession_bonus_skills[character->iProfession]) {
                    value = value * 125 / 100;
                }
                PostCharacterNotice(party_slot, gppStringList[0x1d9],
                                    gppStringList[g_character_skill_name_ids_61e454[skill]], value);
            }
            delete skill_changes;
        } else {
            g_npc_scripting.portrait_message_active = 1;
            SetNpcQuoteBubbleVisible(1, line->payload_10.text, 0, -1, -1, 2, line->extra.raw, -1);
            g_npc_scripting.message_duration_ms =
                ComputePortraitMessageDuration(line->payload_10.text);
            g_npc_scripting.message_started_at = GetTickCount();
        }
        delete[] line->payload_10.text;
        break;
    }
    case W8_NPC_MSG_CALL_HENCHMAN: {
        EndNpcDialogueSession0056E800(0);
        BeginScriptedWorldAction();
        W8MonsterGroup* group = FindFirstMonsterByID(0x112);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xa3a, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetCycleCallback004CA340(0x12, NpcScriptHenchmanArrives);
            StartMonsterCycle(monster_info, 0x12, 1);
        }
        break;
    }
    case W8_NPC_MSG_HENCHMAN_LEAVES: {
        EndNpcDialogueSession0056E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0xdc);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x968, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            StartMonsterCycle(monster_info, 0x12, 1);
            monster_info->p3D->SetCycleCallback004CA340(0x12, NpcScriptHenchmanDeparted);
        }
        ClearMainGameTargetState();
        break;
    }
    case W8_NPC_MSG_PORTRAIT_EXTRA:
        g_npc_scripting.portrait_message_active = 1;
        SetNpcQuoteBubbleVisible(1, line->payload_10.text, 0, -1, -1, 1, line->extra.raw, -1);
        g_npc_scripting.message_duration_ms = ComputePortraitMessageDuration(line->payload_10.text);
        g_npc_scripting.message_started_at = GetTickCount();
        delete[] line->payload_10.text;
        break;
    case W8_NPC_MSG_PORTRAIT_MESSAGE:
        g_npc_scripting.portrait_message_active = 1;
        SetNpcQuoteBubbleVisible(1, line->payload_10.text, 0, -1, -1);
        g_npc_scripting.message_duration_ms = ComputePortraitMessageDuration(line->payload_10.text);
        g_npc_scripting.message_started_at = GetTickCount();
        delete[] line->payload_10.text;
        break;
    case W8_NPC_MSG_LEVEL_UP: {
        int party_slot = *line->extra.level_up_slot;
        g_status_685170.buffers.XChar[party_slot].portrait_advance_103 = 1;
        if (g_settings_6850c8.skill_increase_messages == 0) {
            SoundPlay((STR) "Data\\Sound\\Misc\\GainLevel.wav",
                      0); // c-style-cast-ok: released SGP textual API uses UINT8 pointer spelling
            delete[] line->payload_10.text;
        } else {
            g_npc_scripting.portrait_message_active = 1;
            SetNpcQuoteBubbleVisible(1, line->payload_10.text, 0, -1, -1, 3, line->extra.raw, -1);
            g_npc_scripting.message_duration_ms =
                ComputePortraitMessageDuration(line->payload_10.text);
            g_npc_scripting.message_started_at = GetTickCount();
            delete[] line->payload_10.text;
        }
        break;
    }
    case W8_NPC_MSG_PILLARGATE_LURE: {
        Trigger* trigger = FindTriggerByName("pillargate05");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3f);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case W8_NPC_MSG_PILLARGATE_MADEUS: {
        Trigger* trigger = FindTriggerByName("pillargate04");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3e);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case W8_NPC_MSG_PILLARGATE_ASAIZ: {
        Trigger* trigger = FindTriggerByName("pillargate01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x3d);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case W8_NPC_MSG_RESET_LEVEL_STATE:
        if (gXStatus.fCombatMode == 0 || gXStatus.fPartyMovementMode != 0) {
            if (line->payload_10.raw == 0) {
                ClearLevelDataFlag6();
            } else {
                ResetLevelDataVectors0041F0D0();
            }
        }
        break;
    case W8_NPC_MSG_SET_CONDITION_13: {
        int party_slot = line->payload_10.argument;
        g_status_685170.skip_next_condition_reaction = 1;
        SetCharacterCondition(party_slot, 0x13, 9999, 0, 0, 0);
        g_status_685170.condition13_clock_2487 = 1;
        g_status_685170.condition13_stamp_248b = g_status_685170.world_clock;
        g_status_685170.pending_condition_party_slot_248f = party_slot;
        break;
    }
    case W8_NPC_MSG_SHOW_DIALOGUE_PANEL:
        SetNpcDialoguePanelVisible(1);
        break;
    case W8_NPC_MSG_PRINCE_DISAPPEARS: {
        EndNpcDialogueSession0056E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1ab);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x983, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
        }
        group = FindFirstMonsterByID(0x15d);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, 0);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_SELF: {
        EndNpcDialogueSession0056E800(0);
        W8Monster* monster = GetNpcMonster(g_npc_scripting.npc);
        if (monster != 0) {
            monster->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_JANETTE: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x62);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                MonsterStartsDying(monster_info, 1);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbbf, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_MARTEN: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(100);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive == 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xc06, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            } else if (monster_info->p3D != 0) {
                monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
            }
        }
        break;
    }
    case W8_NPC_MSG_MOVE_GARI: {
        EndNpcDialogueSession0056E800(0);
        BeginScriptedWorldAction();
        W8MonsterGroup* group = FindFirstMonsterByID(0x162);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x78d, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("MoveGari.msf", 1);
        }
        break;
    }
    case W8_NPC_MSG_MILANO_RAT_DOOR: {
        Trigger* door = FindTriggerByName("RatDoor02");
        if (door == 0) {
            srAssertFail("pDoor", "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", 0x7b6,
                         0);
        }
        door->CompleteItemInteraction004447F0();
        EndNpcDialogueSession0056E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0xcf);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x7c1, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("Milano.msf", 1);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_SHAMAN: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x4d);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive == 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xba8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            } else {
                monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
            }
        }
        break;
    }
    case W8_NPC_MSG_PARTY_SPEAKER_EVENT: {
        unsigned int event_type = static_cast<unsigned int>(line->payload_10.argument);
        int party_slot =
            PickRandomPartySpeaker(event_type, g_status_685170.selected_party_member_2434);
        if (party_slot != -1) {
            g_status_685170.selected_party_member_2434 = static_cast<unsigned char>(party_slot);
            QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot], event_type,
                                g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
            SetNpcDialoguePanelVisible(0);
            if (g_screen_state_00649f1c->modal_dialog_open != 0) {
                g_screen_state_00649f1c->suppress_parting_reaction = 1;
            }
        }
        break;
    }
    case W8_NPC_MSG_PARTY_MEMBER_EVENT: {
        int party_slot = line->payload_10.argument;
        QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot], g_effect_005ee58c,
                            g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        break;
    }
    case W8_NPC_MSG_MOVE_RUBBLE: {
        EndNpcDialogueSession0056E800(0);
        BeginScriptedWorldAction();
        W8MonsterGroup* group = FindFirstMonsterByID(0x83);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x7a8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("MoveRubble.msf", 1);
        }
        break;
    }
    case W8_NPC_MSG_SEDEXUS_LEAVES: {
        EndNpcDialogueSession0056E800(0);
        SetTriggerVariableByName00444030("LezboDemonAppeared", 0);
        npc = GetNpcStateByKind(0x40);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbd8, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case W8_NPC_MSG_TRIGGER_FIX: {
        Trigger* trigger = FindTriggerByName("triggerFix");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_SEDEXUS_RIFT: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x42);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            if (monster_info->fActive != 0) {
                monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
            } else {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xbef, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                    monster_info->location_id, 1);
                RemoveMonster(monster_index, 1);
            }
        }
        break;
    }
    case W8_NPC_MSG_BALBRAK_HOME: {
        npc = GetNpcStateByKind(0xc);
        W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
        srVector3T<float> position;
        if (monster_info != 0 && FindEntityByName("NP_Balbrakhome", &position, 0, 0)) {
            monster_info->p3D->SetPosition(&position);
        }
        break;
    }
    case W8_NPC_MSG_MOOK_COMMENT: {
        unsigned int eligible = 0;
        int party_slot;
        for (party_slot = 2; party_slot < 8; ++party_slot) {
            if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
                g_status_685170.buffers.Char[party_slot].highest_condition < 0xf) {
                ++eligible;
            }
        }
        if (eligible > 1) {
            for (party_slot = 0; party_slot < 8; ++party_slot) {
                W8Character* character = &g_status_685170.buffers.Char[party_slot];
                if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
                    character->iRace == 10 && character->highest_condition < 0xf) {
                    QueueCharacterEvent(character, g_effect_005ee654, g_event_flag_005ed8e0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    break;
                }
            }
        }
        break;
    }
    case W8_NPC_MSG_SAVANT_HACK: {
        W8MonsterGroup* group = FindFirstMonsterByID(0x1b6);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xb2e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            StartMonsterCycle(monster_info, 0x19, 1);
            monster_info->p3D->SetCycleCallback004CA340(0x19, NpcScriptSavantHackDone);
        }
        break;
    }
    case W8_NPC_MSG_MOVE_TO_BOOK: {
        EndNpcDialogueSession0056E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1b4);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xa57, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetScript004C7F10("belapath1.msf", 1);
        }
        Trigger* trigger = FindTriggerByName("CC_TRIGGERPLANE3");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~W8_TRIGGER_ON;
        }
        trigger = FindTriggerByName("CC_TRIGGERPLANE2");
        if (trigger != 0) {
            trigger->flags_0a0 |= W8_TRIGGER_ON;
            trigger->Run(-1);
        }
        break;
    }
    case W8_NPC_MSG_MOVE_TO_BOOK2: {
        Trigger* trigger = FindTriggerByName("CC_TRIGGERPLANE3");
        if (trigger != 0) {
            trigger->flags_0a0 |= W8_TRIGGER_ON;
            trigger->Run(-1);
        }
        npc = GetNpcStateByKind(0x8d);
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, 7, 0, 0);
        }
        break;
    }
    case W8_NPC_MSG_SAVANT_APPEARS: {
        srVector3T<float> position;
        if (FindEntityByName("NP_DS1", &position, 0, 0)) {
            W8MonsterGroup* group = SpawnMonsters(0x234, 1, &position, 0, 1, 0, 0);
            int location_id = IListGetAt(group->monsters, 0);
            if (location_id != 0) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0xa9e, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", location_id,
                    1);
                W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                MonsterForwardReferencePosition(monster_info->p3D, 0);
            }
            npc = GetNpcStateByKind(0x84);
            if (npc != 0) {
                QueueNpcScriptNotice(npc, 0, -1, 0, 0);
            }
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_RPC_VI:
        if (NpcLeadHasNameStyle(0x18)) {
            npc = GetNpcStateByKind(0x18);
            if (npc != 0) {
                DismissNpcFromParty(npc->group_index, 0, true, false);
            }
            srVector3T<float> position;
            if (FindEntityByName("NP_VI1", &position, 0, 0)) {
                W8MonsterGroup* group = SpawnMonsters(0x1b9, 1, &position, 2, 1, 0, 0);
                int location_id = IListGetAt(group->monsters, 0);
                if (location_id != 0) {
                    unsigned int monster_index = MonsterGetIndexByLocationID(
                        0xafc, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                        location_id, 1);
                    W8MonsterInfo* monster_info =
                        MonsterGetScriptPartByLocationIndex(monster_index);
                    if (FindEntityByName("NP_DS1", &position, 0, 0)) {
                        monster_info->p3D->AimAtPosition(&position);
                    }
                }
            }
        }
        break;
    case W8_NPC_MSG_PHOONZANG_SPLIT: {
        npc = GetNpcStateByKind(0x84);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            StartMonsterCycle(monster_info, 0x1a, 1);
        }
        srVector3T<float> position;
        W8MonsterGroup* group;
        if (FindEntityByName("NP_PHOONZANGLEE", &position, 0, 0) &&
            (group = FindFirstMonsterByID(0x197)) != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0xacf, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->SetPosition(&position);
            MonsterForwardReferencePosition(monster_info->p3D, 0);
            StartMonsterCycle(monster_info, 0x12, 1);
        }
        npc = GetNpcStateByKind(0x8d);
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, 0x12, 0, 0);
        }
        break;
    }
    case W8_NPC_MSG_BEGIN_ENDGAME:
        ClearMainGameTargetState();
        BeginEndgameSequence005A6580();
        break;
    case W8_NPC_MSG_CLEAR_NPC_COMBAT:
        if (g_combat_state != 0) {
            int party_slot = line->payload_10.argument;
            if (g_combat_state->iActionChar == party_slot) {
                g_combat_state->eCombatActionStatus = 0;
                g_combat_state->iActionChar = -1;
            }
            ClearMainGameTargetState();
            DismissNpcFromParty(party_slot, 0, false, true);
            SetTargetToCharacter(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            g_combat_state->npc_combat_script_pending[party_slot] = false;
        }
        break;
    case W8_NPC_MSG_DISPATCH_PENDING_NOTICE:
        DispatchPendingNpcScriptNotice();
        break;
    case W8_NPC_MSG_TRAVEL_CONFIRM:
        ShowMainGameNoticeLine(gppStringList[0x7eb], OnNpcTravelConfirmationClosed, 1, 1);
        g_pending_npc_travel_level = line->payload_10.argument;
        break;
    case W8_NPC_MSG_PRINCE_NOT_HOME: {
        EndNpcDialogueSession0056E800(0);
        W8MonsterGroup* group = FindFirstMonsterByID(0x1aa);
        if (group != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x9a0, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                group->leader_location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            monster_info->p3D->BeginFadeOutAndRemove004C5040(0);
        }
        break;
    }
    case W8_NPC_MSG_PARTY_SLOT_EVENT_18: {
        int party_slot = line->payload_10.argument;
        QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot], 0x18,
                            g_event_flag_005ed8ec | g_event_flag_005ed8e0,
                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        break;
    }
    case W8_NPC_MSG_PHOONZANG_NOTICE:
        npc = GetNpcStateByKind(0x87);
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, -1, 0, 0);
        }
        break;
    case W8_NPC_MSG_TURN_TO_BOOK:
        BeginScreenFade(0, 0, 500, NpcScriptTurnToBook, 1, 1);
        break;
    case W8_NPC_MSG_REMOVE_ALETHEIDES_AD: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x2e);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8c2, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_ALETHEIDES_CM: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x2d);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8dd, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case W8_NPC_MSG_REMOVE_ALETHEIDES_DD: {
        EndNpcDialogueSession0056E800(0);
        npc = GetNpcStateByKind(0x2f);
        W8MonsterInfo* monster_info = npc == 0 ? 0 : GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x8f7, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp",
                monster_info->location_id, 1);
            RemoveMonster(monster_index, 1);
        }
        break;
    }
    case W8_NPC_MSG_SEDEXUS_PASSOUT:
        ResolveSedexusCapture();
        break;
    case W8_NPC_MSG_ENDGAME_SCREEN:
        BeginScreenFade(0, 0, 500, NpcScriptEndgameScreen, 1, 1);
        break;
    }

    g_npc_scripting.message_lines.Remove(line);
    delete line;
}

// FUNCTION: WIZ8 0x0052a1b0
void OnNpcTravelConfirmationClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog)) {
        QueueNpcTravelRefusals(g_pending_npc_travel_level);
    }
}

// FUNCTION: WIZ8 0x00528830
void QueueNpcScriptLine(int quote, unsigned char mark_pending, unsigned char prepend,
                        unsigned char suppress_entries)
{
    W8MessageBoxLine* msg_line = new W8MessageBoxLine;

    memset(msg_line, 0, sizeof(W8MessageBoxLine));
    msg_line->quote_index = quote;
    msg_line->mark_pending = mark_pending;
    msg_line->suppress_entries = suppress_entries;
    msg_line->npc = g_npc_scripting.npc;

    if (prepend == 0) {
        g_npc_scripting.message_lines.Add(msg_line);
    } else {
        g_npc_scripting.message_lines.InsertAt(0, msg_line);
    }
}

// FUNCTION: WIZ8 0x005289B0
void QueueNpcMessageLine(W8NpcMessageKind kind, int argument)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->quote_index = -1;
    line->type = kind;
    line->payload_10.argument = argument;
    line->extra.raw = 0;
    line->npc = g_npc_scripting.npc;

    g_npc_scripting.message_lines.Add(line);
}

// FUNCTION: WIZ8 0x00528a80
void AddMessageBoxLine(W8NpcMessageKind kind, wchar_t* text, void* extra)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->quote_index = -1;
    line->type = kind;
    line->payload_10.text = text;
    line->extra.raw = extra;
    line->npc = g_npc_scripting.npc;

    if (g_npc_scripting.message_lines.Add(line) < 0) {
        delete line;
    }
}

/* Resolve the player's reply text to the script line it selects. The current
   quote's option/keyword entries (kinds 5 and 0x13) carry the matchable
   phrases in their sub-entries; a hit consults the quote's kind-6 reply
   entries - role 3 supplies the per-keyword answer list, role 1 the generic
   answer. No keyword match at all falls back to the role-2 entry. -1 when the
   quote has no usable reply. */
// FUNCTION: WIZ8 0x00529300
int FindNpcReplyQuote(wchar_t* text)
{
    W8NpcScriptQuote* quote;
    W8NpcQuoteEntry* entry;
    wchar_t sub_text[1024];
    int quote_index;
    int index;
    int sub;

    quote_index = g_npc_scripting.staging_restore.current_quote_index;
    quote = g_npc_scripting.npc->script_file->quotes + quote_index;
    for (index = 0; index < quote->entry_count; ++index) {
        entry = &quote->entries[index];
        if (entry->kind_00 == 5 || entry->kind_00 == 0x13) {
            break;
        }
    }

    if (index < quote->entry_count) {
        for (sub = 0; sub < entry->sub_entry_count; ++sub) {
            swprintf(sub_text, L"%S", entry->sub_entries[sub].text);
            if (CompareWideTextIgnoreAsciiCase00402920(sub_text, text) != 0) {
                continue;
            }
            /* The reply matched an option: a keyword answer, else the generic
               answer. */
            for (index = 0; index < quote->entry_count; ++index) {
                entry = &quote->entries[index];
                if (entry->kind_00 == 6 && entry->operand_09 == 3 && entry->sub_entry_count != 0) {
                    for (sub = 0; sub < entry->sub_entry_count; ++sub) {
                        swprintf(sub_text, L"%S", entry->sub_entries[sub].text);
                        if (CompareWideTextIgnoreAsciiCase00402920(sub_text, text) == 0) {
                            return entry->operand_01;
                        }
                    }
                }
            }
            for (index = 0; index < quote->entry_count; ++index) {
                entry = &quote->entries[index];
                if (entry->kind_00 == 6 && entry->operand_09 == 1) {
                    return entry->operand_01;
                }
            }
            return -1;
        }
    }

    for (index = 0; index < quote->entry_count; ++index) {
        entry = &quote->entries[index];
        if (entry->kind_00 == 6 && entry->operand_09 == 2) {
            return entry->operand_01;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x005294c0
unsigned char GetNpcQuoteText(W8NpcState* npc, int type, wchar_t* output)
{
    W8NpcScriptFile* script_file = npc->script_file;
    W8NpcScriptQuote* record;

    if (type >= script_file->quote_count) {
        return 0;
    }
    record = &script_file->quotes[type];
    if (record->subquotes == 0) {
        return 0;
    }
    swprintf(output, L"%S", record->subquotes[0]);
    return 1;
}

/* Repost the NPC's current quote bubble after a modal sub-dialog closes, or
   clear it when the dialogue NPC/script went away. */
// FUNCTION: WIZ8 0x00529510
void RestoreCurrentNpcQuoteBubble(void)
{
    W8NpcScriptFile* script_file;

    if (g_npc_scripting.npc != 0 && (script_file = g_npc_scripting.npc->script_file) != 0 &&
        g_npc_scripting.staging_restore.current_quote_index <
            static_cast<int>(script_file->quote_count)) {
        SetNpcQuoteBubbleVisible(
            0, 0, script_file->quotes + g_npc_scripting.staging_restore.current_quote_index,
            g_npc_scripting.staging_restore.current_quote_index, 0xffffffff);
        return;
    }
    SetNpcQuoteBubbleVisible(0, 0, 0, -1, 0xffffffff);
}

/* Queue a quote-entry line carrying `continuation_quote`; `prepend` inserts it
   at the front of the queue (continuation subquotes overtake pending lines). */
// FUNCTION: WIZ8 0x00528B50
void QueueNpcQuoteEntry(W8NpcQuoteEntry* entry, int continuation_quote, unsigned char prepend)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->quote_entry = entry;
    line->type = W8_NPC_MSG_QUOTE_ENTRY;
    line->quote_index = -1;
    line->continuation_quote = continuation_quote;
    line->npc = g_npc_scripting.npc;

    if (prepend == 0) {
        g_npc_scripting.message_lines.Add(line);
    } else {
        g_npc_scripting.message_lines.InsertAt(0, line);
    }
}

// FUNCTION: WIZ8 0x00529560
void CancelNpcDialogue(void)
{
    g_npc_scripting.dialogue_cancelled_c4 = 1;
}

/* Raise the quote bubble over `text` and, when `play_sound` is set, kick off
   the generic start-game click that accompanies a silent text notice. */
// FUNCTION: WIZ8 0x00529570
void DisplayNpcQuote00529570(const wchar_t* text, char play_sound)
{
    char sound_path[128];
    SOUNDPARMS sound_parms;

    SetNpcQuoteBubbleVisible(true, text, 0, -1, 0x47);
    g_npc_scripting.voice_playing = 0;
    g_npc_scripting.message_duration_ms = 2000;
    g_npc_scripting.last_tick = GetTickCount();
    g_npc_scripting.message_started_at = GetTickCount();
    g_npc_scripting.quote_active = 1;
    if (play_sound != 0) {
        memset(&sound_parms, 0xff, sizeof(SOUNDPARMS));
        sprintf(sound_path, "Data\\Sound\\misc\\startgame.wav");
        sound_parms.EOSCallback = 0;
        SoundPlay(sound_path, &sound_parms);
    }
}

/* Run every kind-0x17 consequence entry of the given quote - the reply
   handler's decline path when the party refuses a price offer. */
// FUNCTION: WIZ8 0x00529610
void RunNpcQuoteDeclineActions(int quote_index)
{
    W8NpcScriptQuote* quote;
    int index;

    quote = g_npc_scripting.npc->script_file->quotes + quote_index;
    index = 0;
    if (quote->entry_count != 0) {
        do {
            if (quote->entries[index].kind_00 == 0x17) {
                RunNpcScriptLine(quote->entries[index].operand_01, 0);
            }
            ++index;
        } while (index < quote->entry_count);
    }
}
/* QA audit over every `Data\NPC Scripts\*.nsf`: load each script file, print
   every quote line that is not a placeholder sentinel through ShowNotice, log
   each notice that wraps past seven lines to data\longquotes.txt, and write
   per-script plus grand totals to data\quotereport.txt. Raising
   g_status_685170.quote_audit_2431 makes ShowNotice maintain
   g_notice_line_count_0069b7bc and g_status_685170.long_quote_2432. The
   report handle is used unchecked after the appending fopen, which is the
   original's own error handling. */
// FUNCTION: WIZ8 0x00529660
void AuditNpcScriptQuotes00529660(void)
{
    FILE* file;
    FILE* log_file;
    W8NpcScriptFile* script;
    W8NpcScriptQuote* quote;
    GETFILESTRUCT find;
    char date[128];
    char line[200];
    char path[512];
    char pattern[512];
    wchar_t display[2048];
    char* text;
    int file_lines;
    int file_quotes;
    int total_lines;
    int total_quotes;
    int total_scripts;
    int record_index;
    int sub_index;
    BOOLEAN found;

    total_lines = 0;
    total_quotes = 0;
    total_scripts = 0;
    file = fopen("data\\longquotes.txt", "w");
    if (file != 0) {
        fclose(file);
    }
    file = fopen("data\\quotereport.txt", "w");
    if (file != 0) {
        fclose(file);
    }
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, 0, "dddd',' MMMM dd',' yyyy", date, 0x80);
    file = fopen("data\\quotereport.txt", "a+t");
    g_status_685170.quote_audit_2431 = 1;
    sprintf(pattern, "Data\\NPC Scripts\\*.nsf");
    fprintf(file, "Script Report File: %s\n", date);
    fprintf(file, "-------------------------------------------------\n");
    found = GetFileFirst(pattern, &find);
    while (found != 0) {
        sprintf(path, "Data\\NPC Scripts\\%s", find.zFileName);
        script = LoadNpcScriptFile0055A480(path);
        if (script != 0) {
            file_lines = 0;
            file_quotes = 0;
            for (record_index = 0; record_index < script->quote_count; ++record_index) {
                quote = &script->quotes[record_index];
                for (sub_index = 0; sub_index < quote->subquote_count; ++sub_index) {
                    text = quote->subquotes[sub_index];
                    if (strlen(text) != 0 && _stricmp(text, "EMPTY") != 0 &&
                        _stricmp(text, "BLANK") != 0 && _stricmp(text, "UNKNOWN") != 0 &&
                        _stricmp(text, "CLASSIFIED") != 0) {
                        swprintf(display, L" \"%S\"", text);
                        ShowNotice(0, display, 0, GetTextBoxScrollRange(), 0);
                        if (g_status_685170.long_quote_2432 != 0) {
                            sprintf(line, "Long Quote: #%d, subquote: #%d, script file: %s \n",
                                    record_index, sub_index, find.zFileName);
                            log_file = fopen("data\\longquotes.txt", "a+t");
                            if (log_file != 0) {
                                fprintf(log_file, "%s\n", line);
                                fclose(log_file);
                            }
                        }
                        file_lines += g_notice_line_count_0069b7bc;
                        total_lines += g_notice_line_count_0069b7bc;
                        ++total_quotes;
                        ++file_quotes;
                    }
                }
            }
            fprintf(file, "Script: %s\n", find.zFileName);
            fprintf(file, "  #quotes: %d\n  #lines: %d\n", file_quotes, file_lines);
            ReleaseNpcScriptFile0055A0A0(script);
            ++total_scripts;
        }
        found = GetFileNext(&find);
    }
    ShowNotice(0, L"Quote test complete. See log file for results", -1, GetTextBoxScrollRange(), 0);
    fprintf(file, "Total lines: %d\nTotal Quotes: %d\nTotal Scripts: %d", total_lines, total_quotes,
            total_scripts);
    fclose(file);
    g_status_685170.quote_audit_2431 = 0;
}
/* Queue `text` as a floating portrait message: the string is copied so the
   queue owns it, and while the level-data flag is clear the message is
   bracketed by RESET_LEVEL_STATE markers so the dispatcher restores state
   around it. */
// FUNCTION: WIZ8 0x005299B0
void ShowString(wchar_t* text)
{
    wchar_t* copy = new wchar_t[0x200];
    wcscpy(copy, text);
    if (GetLevelDataFlag6() == 0) {
        AddMessageBoxLine(W8_NPC_MSG_RESET_LEVEL_STATE,
                          reinterpret_cast<wchar_t*>(1), // reinterpret-ok: tagged storage
                          0);
    }
    AddMessageBoxLine(W8_NPC_MSG_PORTRAIT_MESSAGE, copy, 0);
    if (GetLevelDataFlag6() == 0) {
        AddMessageBoxLine(W8_NPC_MSG_RESET_LEVEL_STATE, 0, 0);
    }
}
// FUNCTION: WIZ8 0x00529BC0
void SetScriptedSceneActive(void)
{
    g_npc_scripting.scripted_scene_active = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearScriptedSceneActive(void)
{
    g_npc_scripting.scripted_scene_active = 0;
}
/* Fact 0x1bf: raise the scripted-scene gate, drop the level's transient data
   vectors, force the single-target mode, and reopen the party-member region
   sets (7 + slot, member region 0x5a + slot) for every occupied slot. */
// FUNCTION: WIZ8 0x00529BE0
void BeginNpcScriptedScene(void)
{
    int party_slot;

    g_npc_scripting.scripted_scene_active = 1;
    ResetLevelDataVectors0041F0D0();
    gXStatus.scripted_scene_19b7 = 1;
    SetTargetingMode(1);
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
            RegionSetEnable(party_slot + 7);
            EnableRegionSetInput(party_slot + 7);
            EnableRegionInput(party_slot + 0x5a);
        }
    }
}
/* The scripted-scene wind-down: clears the targeting/level state raised by
   BeginNpcScriptedScene, resets the script facts, then classifies the picked
   character. A female pick with any non-female party member present raises
   fact 0x1c0 and queues the special event; an unanimated pick is further
   tested for the three 0x1fd-0x1ff items (success leaves fact 0x1c1 at zero
   and fills the alternate-name display), and anything else falls back to
   facts 0x1c1 or 0x227. The party-member region sets reopen on the way out. */
// FUNCTION: WIZ8 0x00529C40
void EndScriptedPortraitPick00529C40(int party_slot)
{
    W8ItemInstance* found;
    W8Character* character;
    unsigned int slot;
    bool other_gender_present;

    if (gXStatus.scripted_scene_19b7 == 0) {
        return;
    }
    if (gXStatus.fCombatMode == 0 || gXStatus.fPartyMovementMode != 0) {
        ClearLevelDataFlag6();
    }
    SetTargetingMode(0);
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0) {
            RegionSetDisable(slot + 7);
            DisableRegionSetInput(slot + 7);
            DisableRegionInput(slot + 0x5a);
        }
    }
    g_npc_scripting.scripted_scene_active = 0;
    gXStatus.scripted_scene_19b7 = 0;
    other_gender_present = false;
    SetFact(0x1c0, 0, 0);
    SetFact(0x200, 0, 0);
    SetFact(0x1c1, 0, 0);
    SetFact(0x227, 0, 0);
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
            g_status_685170.buffers.Char[slot].gender != W8_GENDER_FEMALE) {
            other_gender_present = true;
            break;
        }
    }
    character = &g_status_685170.buffers.Char[party_slot];
    if (character->gender == W8_GENDER_FEMALE && other_gender_present != 0) {
        SetFact(0x1c0, 1, 0);
        QueueCharacterEvent(character, g_effect_005ee634, g_event_flag_005ed8e0,
                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        return;
    }
    if (g_status_685170.buffers.XChar[party_slot].npc_index == -1 &&
        character->highest_condition < 0xf) {
        if (FindItemOnCharacter(character, 0x1fd, &found, 0, 0) != 0 &&
            FindItemOnCharacter(character, 0x1fe, &found, 0, 0) != 0 &&
            FindItemOnCharacter(character, 0x1ff, &found, 0, 0) != 0) {
            SetFact(0x1c1, 0, 0);
            swprintf(g_status_685170.monster_name_buffer_2453, g_format_al_s_00614b44,
                     character->name);
            g_status_685170.sedexus_party_slot_247f = party_slot;
            g_status_685170.rpc_active_2489 = 1;
            g_status_685170.infatuation_pending_2446 = 1;
            QueueCharacterEvent(character, g_special_event_0068c50c, 0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
        } else {
            SetFact(0x1c1, 1, 0);
        }
    } else {
        SetFact(0x227, 1, 0);
    }
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0) {
            RegionSetEnable(slot + 7);
            EnableRegionSetInput(slot + 7);
            EnableRegionInput(slot + 0x5a);
        }
    }
}
/* Al-Sedexus takes her pick: the scripted scene opens, the lighting fades,
   and every still-living occupied slot other than the selected character is
   put under condition 0x11. */
// FUNCTION: WIZ8 0x00529EF0
void BeginSedexusCapture(void)
{
    unsigned int party_slot;

    g_npc_scripting.scripted_scene_active = 1;
    BeginScriptedWorldAction();
    g_npc_scripting.sedexus_capture_pending = 1;
    g_npc_scripting.sedexus_capture_active = 1;
    BeginWorldLightingFade(-1000.0f);
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
            (g_status_685170.buffers.Char[party_slot].hp_current > 0 ||
             g_status_685170.buffers.Char[party_slot].highest_condition < 0x12) &&
            party_slot != static_cast<unsigned int>(g_status_685170.sedexus_party_slot_247f)) {
            SetCharacterCondition(party_slot, 0x11, 9999, 0, 0, 0);
        }
    }
}
/* The capture resolves into the templar spawn: the two NPC kinds are
   released, the LezboDemonAppeared trigger variable drops, monster 0xd8 is
   spawned on np_al-adryian51 and given proximitytemplar.msf, and the two
   gate triggers fire while the lighting fades back up. */
// FUNCTION: WIZ8 0x00529F90
void ResolveSedexusCapture(void)
{
    W8MonsterGroup* group;
    Trigger* trigger;
    srVector3T<float> position;
    int location_id;
    unsigned int monster_list_index;
    W8MonsterInfo* info;

    ReleaseNpcMonsterByKind(0x3b);
    ReleaseNpcMonsterByKind(0x40);
    SetTriggerVariableByName00444030("LezboDemonAppeared", 0);
    if (FindEntityByName("np_al-adryian51", &position, 0, 0)) {
        group = SpawnMonsters(0xd8, 1, &position, 2, 1, 0, 0);
        location_id = IListGetAt(group->monsters, 0);
        if (location_id != 0) {
            monster_list_index = MonsterGetIndexByLocationID(
                0x10e0, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", location_id, 1);
            info = MonsterGetScriptPartByLocationIndex(monster_list_index);
            if (info != 0) {
                info->p3D->SetScript004C7F10("proximitytemplar.msf", 1);
            }
        }
    }
    g_npc_scripting.scripted_scene_active = 1;
    g_npc_scripting.sedexus_release_pending = 1;
    BeginWorldLightingFade(1000.0f);
    trigger = FindTriggerByName("al-seduxusgate");
    if (trigger != 0) {
        trigger->Run(-1);
    }
    trigger = FindTriggerByName("templargate14");
    if (trigger != 0) {
        trigger->Run(-1);
    }
}
// FUNCTION: WIZ8 0x0052A070
unsigned char IsSedexusCaptureActive(void)
{
    return g_npc_scripting.sedexus_capture_active;
}
/* Cycle-0x12 arrival callback: the henchmen group spawns on the NP_HENCHMEN
   waypoint and its first member's bound NPC gets the script notice; the
   callback monster itself is parked at the origin with its position flagged
   dirty either way. */
// FUNCTION: WIZ8 0x0052A080
void NpcScriptHenchmanArrives(W8Monster* monster)
{
    srVector3T<float> position;
    srVector3T<float> origin;

    if (FindEntityByName("NP_HENCHMEN", &position, 0, 0)) {
        PointCameraAtTarget(&position, 0, 1);
        W8MonsterGroup* group = SpawnMonsters(0xdc, 1, &position, 2, 1, 0, 0);
        int location_id = IListGetAt(group->monsters, 0);
        if (location_id != 0) {
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                    0x111f, "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting.cpp", location_id,
                    1));
            if (monster_info != 0) {
                W8NpcState* npc = GetNpcStateForMonsterInfo(monster_info, 0);
                if (npc != 0) {
                    QueueNpcScriptNotice(npc, 0, -1, 0, 0);
                }
            }
        }
    }
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    monster->SetPosition(&origin);
    monster->flags_1dc |= 0x40;
}

/* Cycle-0x12 departure callback: park the monster at the origin and flag its
   position dirty. */
// FUNCTION: WIZ8 0x0052A150
void NpcScriptHenchmanDeparted(W8Monster* monster)
{
    srVector3T<float> origin;

    monster->flags_1dc |= 0x40;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    monster->SetPosition(&origin);
}

/* Cycle-0x19 completion callback: stamping savant_hack_tick lets
   UpdateNpcEvents retire NPC group 0x1b3 fifty ticks later. */
// FUNCTION: WIZ8 0x0052A190
void NpcScriptSavantHackDone(W8Monster* monster)
{
    g_status_685170.savant_hack_tick = GetTickCount();
}

// FUNCTION: WIZ8 0x0052A1A0
void SetNpcScriptEventActive(unsigned char value)
{
    g_npc_script_event_active = value;
}
