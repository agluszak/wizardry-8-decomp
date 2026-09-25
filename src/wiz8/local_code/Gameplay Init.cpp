#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/monster_generators.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/item_tables.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "random.h"
#include "timer.h"
#include "wiz8/local_code/character_events.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* Local Code\Gameplay Init.cpp: the gameplay initialization/reset subsystem -
   the NPC/fact/level database loaders and destroyers, the status-buffer
   allocators, and the new-game reset family (ResetForNewGame,
   RunNewGameOpeningSequence, ResetGameplayStatusBlock, ResetTargetingState,
   ResetGameplaySlot, ResetPartySlotRow, ResetGameplaySettings).

   Attribution evidence: no assertion path survives in this span, but the
   2001-08-14 demo build embeds this TU's __FILE__ string
   ("E:\Wizardry 8\Local Code\Gameplay Init.cpp") in .data link order between
   "Local Code\GameplayDatabase.cpp" and "Local Code\Configuration.cpp", and
   these functions fill the matching .text span 0x54AAC0-0x54B560 between the
   anchored GameplayDatabase.cpp tail (0x54A9A0, LoadMonsterDatabaseRange) and
   Configuration.cpp (0x54B6D0, SaveGameConfiguration). The demo string cluster
   for this TU is nearly empty - its loaders share the pooled
   "SaveConfig: ERROR - FileOpen failed on file %s" literal. */

/* NPC.DBS records carry an optional sub-list, stored after the record when its
   leading count exceeds one and its 0x9D flag is clear: a count, then that many
   six-byte elements appended to a freshly created list. */
// FUNCTION: WIZ8 0x0054aac0
unsigned char InitializeNpcDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int entry;
    unsigned int entry_count;
    unsigned int transferred;
    int handle;
    W8NpcItemStockRule* element;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "NPC", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiNpcsInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    g_npc_records = static_cast<W8NpcDatabaseRecord*>(
        malloc(gXStatus.uiNpcsInDatabase * sizeof(*g_npc_records)));
    if (!g_npc_records) {
        return 0;
    }
    for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
        if (!FileRead(handle, &g_npc_records[index], sizeof(*g_npc_records), &transferred)) {
            FileClose(handle);
            return 0;
        }
        g_npc_records[index].item_stock_rules = 0;
        if (g_npc_records[index].no_item_stock_9d == 0 && g_npc_records[index].version > 1) {
            entry_count = 0;
            if (!FileRead(handle, &entry_count, 4, &transferred)) {
                FileClose(handle);
                return 0;
            }
            if (entry_count > 0) {
                g_npc_records[index].item_stock_rules = PLCreate();
                for (entry = 0; entry < entry_count; ++entry) {
                    element = new W8NpcItemStockRule;
                    if (!element) {
                        FileClose(handle);
                        return 0;
                    }
                    memset(element, 0, 6);
                    if (!FileRead(handle, element, 6, &transferred)) {
                        FileClose(handle);
                        return 0;
                    }
                    PListInsert(g_npc_records[index].item_stock_rules, entry, element);
                }
            }
        }
    }
    FileClose(handle);
    return 1;
}

/* The three loaders below share one shape: build Data\Databases\<NAME>.DBS,
   open it, read a record count, allocate count * stride, then read the records
   one at a time. They are written out rather than folded into a helper because
   each is its own COMDAT in the original and shares no code with the others.
   All three leak the handle when the allocation fails while every other failure
   closes it; the asymmetry is the original's and is reproduced. */

// FUNCTION: WIZ8 0x0054ad00
unsigned char InitializeFactDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "FACT", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiFactsInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    g_fact_records = static_cast<W8FactDatabaseRecord*>(
        malloc(gXStatus.uiFactsInDatabase * sizeof(*g_fact_records)));
    if (!g_fact_records) {
        return 0;
    }
    for (index = 0; index < gXStatus.uiFactsInDatabase; ++index) {
        if (!FileRead(handle, &g_fact_records[index], sizeof(*g_fact_records), &transferred)) {
            FileClose(handle);
            return 0;
        }
    }
    FileClose(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0054ae00
void DestroyFactDatabase(void)
{
    free(g_fact_records);
    g_fact_records = 0;
}

// FUNCTION: WIZ8 0x0054ae20
unsigned char InitializeLevelDatabase(void)
{
    char path[60];
    unsigned int index;
    unsigned int transferred;
    int handle;

    sprintf(path, "%s\\%s.%s", "Data\\Databases", "LEVELS", "DBS");
    handle = FileOpen(path, 1, 0);
    if (!handle) {
        return 0;
    }
    if (!FileRead(handle, &gXStatus.uiLevelsInDatabase, 4, &transferred)) {
        FileClose(handle);
        return 0;
    }
    g_level_records = static_cast<W8LevelDatabaseRecord*>(
        malloc(gXStatus.uiLevelsInDatabase * sizeof(*g_level_records)));
    if (!g_level_records) {
        return 0;
    }
    for (index = 0; index < gXStatus.uiLevelsInDatabase; ++index) {
        if (!FileRead(handle, &g_level_records[index], sizeof(*g_level_records), &transferred)) {
            FileClose(handle);
            return 0;
        }
    }
    FileClose(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0054af10
void DestroyLevelDatabase(void)
{
    free(g_level_records);
    g_level_records = 0;
}

/* Optionally releases the global status block's two buffers, then clears the
   whole block - which zeroes those pointers as a side effect, since they live
   inside it - and allocates them again. Either allocation failing leaves the
   block cleared and the other buffer live, as the original does. */
// FUNCTION: WIZ8 0x0054af30
void ResetGameStatus(unsigned char release)
{
    if (release) {
        if (g_status.buffers.Char) {
            free(g_status.buffers.Char);
            g_status.buffers.Char = 0;
        }
        if (g_status.buffers.XChar) {
            free(g_status.buffers.XChar);
            g_status.buffers.XChar = 0;
        }
    }
    memset(&g_status, 0, sizeof(g_status));
    g_status.buffers.Char =
        static_cast<W8Character*>(malloc(sizeof(W8Character) * W8_PARTY_SLOT_COUNT));
    if (!g_status.buffers.Char) {
        return;
    }
    g_status.buffers.XChar =
        static_cast<W8PartySlotRow*>(malloc(sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT));
    if (!g_status.buffers.XChar) {
        return;
    }
    memset(g_status.buffers.Char, 0, sizeof(W8Character) * W8_PARTY_SLOT_COUNT);
    memset(g_status.buffers.XChar, 0, sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT);
}

/* Clear the complete status object, including its POD gameplay-state tail. */
// FUNCTION: WIZ8 0x0054afd0
void InitializeGameplayRuntimeObjects(void)
{
    memset(static_cast<void*>(&gXStatus), 0, sizeof(gXStatus));
    gXStatus.character_event_queue = new W8CharacterEventQueue();
    gXStatus.gameplay_timer = new W8GameTimer(300.0f, 0);
}

/* Loads Data\\Databases\\SpellTables.dbs, replacing whatever is already there.
   The header is two dwords: an allocation count and the number of rows to read.
   They are not the same number, and the allocation is sized from the first while
   the loop runs over the second, which is what the original does.
 
   Each row is preceded by 0x101 bytes the loader skips rather than reads. A
   failure anywhere stops the loop and drops the whole table, but the file is
   closed and the row count published either way - including on failure, where
   the count then describes a table that is no longer there. Preserved as found.
 
   The assertion at Spells.cpp:1908 names the table s_pSpellTable, which is why
   this body treats the global as the table itself rather than as a cursor. */
// FUNCTION: WIZ8 0x0054b080
void ResetGameplayStatusBlock(void)
{
    memset(gXStatus.spell_cooldown_clocks, 0, sizeof(gXStatus.spell_cooldown_clocks));
    gXStatus.character_event_queue->DestroyAllEvents();
    gXStatus.party_moving = false;
    gXStatus.fSurprisePossible = false;
}

// FUNCTION: WIZ8 0x0054b0b0
void DestroyGameplayObjects(void)
{
    W8CharacterEventQueue* owned = gXStatus.character_event_queue;

    if (owned) {
        delete owned;
        gXStatus.character_event_queue = 0;
    }
    if (gXStatus.gameplay_timer) {
        delete gXStatus.gameplay_timer;
        gXStatus.gameplay_timer = 0;
    }
}

/* The new-game reset. It repeats ResetGameStatus's status-block cycle inline
   rather than calling it; retail: that duplication is the authored body, not a
   missing helper. It then clears the item in hand and the carried pool, and
   grants the starting items. The pool and the id list are both walked by
   address against the symbol that follows them, not by index. */
// FUNCTION: WIZ8 0x0054b100
void ResetForNewGame(void)
{
    W8ItemInstance item;
    W8ItemInstance* slot;
    unsigned int* id;
    unsigned int index;

    if (g_status.buffers.Char) {
        free(g_status.buffers.Char);
        g_status.buffers.Char = 0;
    }
    if (g_status.buffers.XChar) {
        free(g_status.buffers.XChar);
        g_status.buffers.XChar = 0;
    }
    memset(&g_status, 0, sizeof(g_status));
    g_status.buffers.Char =
        static_cast<W8Character*>(malloc(sizeof(W8Character) * W8_PARTY_SLOT_COUNT));
    if (g_status.buffers.Char) {
        g_status.buffers.XChar =
            static_cast<W8PartySlotRow*>(malloc(sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT));
        if (g_status.buffers.XChar) {
            memset(g_status.buffers.Char, 0, sizeof(W8Character) * W8_PARTY_SLOT_COUNT);
            memset(g_status.buffers.XChar, 0, sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT);
        }
    }
    ReleaseMessageStorage();
    EmptyItemRecord(&g_status.item_in_hand_235b, 0, 1);
    slot = g_status.party_item_pool_0021;
    do {
        EmptyItemRecord(slot, 0, 1);
        ++slot;
    } while (slot < g_status.party_item_pool_0021 + 500);
    id = g_starting_item_ids;
    do {
        if (*id != 0xffffffff) {
            ReplaceOrCreateItem(&item, *id, 1, 1, 1);
            item.stack_count = 1;
            AddItemToParty(&item, 0, 0);
        }
        ++id;
    } while (id < g_starting_item_ids + 6);
    g_status.party_gold = 500;
    g_status.selected_character = GetNextCharacter(1, 1, -1);
    g_status.current_level = -1;
    InitializePartyFormation(&g_status.formation);
    for (index = 0; index < 8; ++index) {
        g_status.party_order_slots[index] = 0xffffffff;
    }
}

/* Raises three flags, optionally hands the caller's target to 0x005A9E70, then
   runs a fixed opening sequence. The two calls into 0x00482720 and 0x00482740
   share one stack cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054b250
void RunNewGameOpeningSequence(unsigned char notify, const wchar_t* target)
{
    g_status.game_started = 1;
    if (target) {
        g_status.iron_man = 1;
        SetLastSaveName(target);
    }
    g_status.difficulty = g_settings.difficulty;
    gXStatus.fEncumbranceDirty = true;
    SetGameTimeMilliseconds(0x2932e00);
    SetGameTimeDays(1);
    if (notify) {
        RequestScreenTransition();
    }
    InitializeNpcStates();
    ResetNpcStates();
    InitializeFactJournal();
    ResetFactions();
    ResetMainScreenStateBlock();
    SetPendingScreenState(W8_SCREEN_GAME_START_ROUTER);
}

// FUNCTION: WIZ8 0x0054b2d0
void ResetTargetingState(void)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        ResetGameplaySlot(slot);
    }
    gXStatus.picked_monster = -1;
    gXStatus.picked_group = -1;
}

/* Resets one 0x118-byte slot. The tier it stores twice comes from the character
   the slot belongs to: the status block's first buffer is an array of
   W8Character at the 0x1862 stride, and the field at 0x0b01 - the same one
   party-member selection thresholds against 0x12 and 0x0f - decides between 1
   and 2 here. The five countdown clocks and the Random call share one stack
   cleanup, as consecutive cdecl calls do. */
// FUNCTION: WIZ8 0x0054b300
void ResetGameplaySlot(unsigned int slot)
{
    W8MonsterManagerEntry* record = &gXStatus.monster_manager_entries[slot];
    int tier;

    memset(static_cast<void*>(record), 0, sizeof(W8MonsterManagerEntry));
    record->portrait_event_active = 0;
    record->voice_sound_handle = -1;
    record->previous_portrait_frame = -1;
    record->portrait_frame = 6;
    record->portrait_pose_animation_active = 0;
    tier = 1;
    if (g_status.buffers.Char[slot].highest_condition >= 0xf) {
        tier = 2;
    }
    record->portrait_pose = tier;
    record->target_portrait_pose = tier;
    record->previous_portrait_pose = -1;
    record->portrait_pose_dirty = 0;
    record->portrait_frame_dirty = 0;
    record->portrait_frame_clock = SetCountdownClock(0);
    record->voice_time_remaining_ms = 0;
    record->portrait_pose_clock = SetCountdownClock(0);
    record->portrait_idle_clock = SetCountdownClock(Random(5000) + 5000);
    record->portrait_fx_clock = SetCountdownClock(0);
    record->damage_splat_active = 0;
    record->damage_splat_death_variant = 0;
    record->dead_portrait_revealed = 0;
    record->damage_splat_amount = 0;
    record->damage_splat_frame = -1;
    record->damage_splat_end_frame = 0;
    record->damage_splat_catalog = 0x90;
    record->effect_icon_active = 0;
    record->effect_icon_catalog = -1;
    record->effect_icon_frame = -1;
    record->effect_icon_end_frame = 0;
    record->active_character_event = 0;
    record->portrait_stats_dirty = 0;
    record->cached_hp_bar = 0;
    record->cached_stamina_bar = 0;
    record->cached_hp = 0;
    record->cached_spell_bar = 0;
    record->portrait_refresh_pinned = 0;
    record->auto_portrait_refresh = 0;
    record->keyboard_menu_open = 0;
    record->combat_portrait_dirty = 0;
    record->acting_portrait_pulse = 0;
    record->level_up_ready = 0;
    record->acting_portrait_pulse_clock = SetCountdownClock(0);
}

/* The static initializer constructs the manager's entries and vector before
   this runs; the bulk reset below deliberately wipes them along with the
   neighbouring runtime state. That matches retail exactly, including the
   wiped container headers: every later use is non-virtual (Clear, GetCount,
   direct teardown of a null backing store), so no reconstruction runs. */

// FUNCTION: WIZ8 0x0054b470
void ResetPartySlotRow(int slot)
{
    W8PartySlotRow* row = &g_status.buffers.XChar[slot];

    memset(row, 0, sizeof(W8PartySlotRow));
    row->fOccupied = 1;
    row->spell_id = 0;
    row->queued_action = 0xff;
    SetSlotAction(slot, 0, -1);
}

/* Both buffers are cleared only after both allocations succeed, so a failed
   second allocation leaves the first one live and unzeroed. */
// FUNCTION: WIZ8 0x0054b4c0
unsigned char AllocateStatusBuffers(W8StatusBuffers* status)
{
    status->Char = static_cast<W8Character*>(malloc(sizeof(W8Character) * W8_PARTY_SLOT_COUNT));
    if (!status->Char) {
        return 0;
    }
    status->XChar =
        static_cast<W8PartySlotRow*>(malloc(sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT));
    if (!status->XChar) {
        return 0;
    }
    memset(status->Char, 0, sizeof(W8Character) * W8_PARTY_SLOT_COUNT);
    memset(status->XChar, 0, sizeof(W8PartySlotRow) * W8_PARTY_SLOT_COUNT);
    return 1;
}

// FUNCTION: WIZ8 0x0054b520
void FreeStatusBuffers(W8StatusBuffers* status)
{
    if (status->Char) {
        free(status->Char);
        status->Char = 0;
    }
    if (status->XChar) {
        free(status->XChar);
        status->XChar = 0;
    }
}

/* Clears the settings block and writes its defaults. The constants 0, 1, 0x40
   and 0xff are each used many times over, which is why VC6 holds them in
   registers rather than spelling out immediates. */
// FUNCTION: WIZ8 0x0054b560
void ResetGameplaySettings(void)
{
    memset(&g_settings, 0, sizeof(g_settings));
    g_settings.sound_effects_volume = 0x40;
    g_settings.voice_volume = 0x40;
    g_settings.main_ui_mode = W8_MAIN_UI_MODE_FORMATION;
    g_settings.continuous_combat = 0;
    g_settings.auto_advance_character = 0;
    g_settings.tooltips_enabled = true;
    g_settings.formation_action_panel_preference = 1;
    g_settings.formation_radar_map_preference = 1;
    g_settings.formation_board_preference = 1;
    g_settings.portraits_action_panel_preference = 1;
    g_settings.field_000 = 0;
    g_settings.invert_mouse_y = 0;
    g_settings.field_03b = 0;
    g_settings.difficulty = W8_DIFFICULTY_NORMAL;
    g_settings.text_display_delay_ms = 0x9c4;
    g_settings.combat_delay_ms = 1000;
    g_settings.field_019 = 5000;
    g_settings.camera_rotation_mode = 1;
    g_settings.camera_rotation_style = 1;
    g_settings.tooltip_delay_ms = 600;
    g_settings.music_volume = 0x1f;
    g_settings.footstep_volume = 0x13;
    g_settings.muted_sound_effects_volume = 0xff;
    g_settings.muted_music_volume = 0xff;
    g_settings.muted_voice_volume = 0xff;
    g_settings.field_035 = 0xff;
    g_settings.monster_movement_speed = 2.5f;
    g_settings.gamma = 1.0f;
    g_settings.pc_confirmations = 1;
    g_settings.pc_subtitles = 1;
    g_settings.mouselook_toggle = 0;
    g_settings.mouselook_smoothing = 1;
    g_settings.numeric_hit_points = 1;
    g_settings.auto_save = 0;
    g_settings.field_047 = 0;
    g_settings.monster_shadows = 1;
    g_settings.smooth_monster_animations = 1;
    g_settings.smooth_world_animations = 1;
    g_settings.skill_increase_messages = 1;
    g_settings.ctrl_right_click_info = 0;
    g_settings.autoswap_weapons = 1;
    g_settings.autotarget_spells = 1;
    g_settings.autoscroll_combat_messages = 0;
    g_settings.simplified_npc_interaction = 1;
    EnableAllRenderOptions();
    if (GetTotalPhysicalMemory() <= 0x4000000) {
        DisableRenderOption(0xb);
        DisableRenderOption(0xc);
    }
    if (GetRendererFamily() != 1) {
        DisableRenderOption(0x10);
    }
}
