#ifndef WIZ8_LAYOUTS_GAME_STATUS_H
#define WIZ8_LAYOUTS_GAME_STATUS_H

#include "wiz8/difficulty.h"
#include "Types.h"

#include "wiz8/gameplay_modifiers.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/levels.h"
#include "wiz8/layouts/party_formation.h"
#include "wiz8/layouts/world.h"

#include <stddef.h>

struct W8PartySlotRow;

struct W8Character;

enum { W8_PARTY_SLOT_COUNT = 8 };

#pragma pack(push, 1)
struct W8StatusBuffers {
    float save_version;
    /* Char and XChar, spelled by the gStatus.Char[uiChar] and
       gStatus.XChar[uiSlot] assertions. */
    W8Character* Char;
    W8PartySlotRow* XChar;
};

enum { W8_CHARACTER_SERIALIZED_SIZE = 0x1862 };

struct W8ItemSpellUsageRecord {
    unsigned int cast_count;
    unsigned int usable_cast_count;
    unsigned char unknown_08[8];
};

struct W8CharacterSpellUsageRecord {
    unsigned char unknown_00[4];
    unsigned int cast_count;
    unsigned int usable_cast_count;
    unsigned char unknown_0c[4];
};

struct W8GlobalStatus {
    W8StatusBuffers buffers;
    bool game_started; /* 0x000c */
    /* 0x000d..0x0018: the three join counters the party-add entry advances:
       the regular-member, auxiliary and total counts. */
    int regular_member_count;
    int auxiliary_member_count;
    int total_member_count;
    unsigned int party_gold;
    int selected_character;
    W8ItemInstance party_item_pool[500];
    unsigned int party_item_count;
    /* 0x1795: signed 16-bit text-box line cursor. Every retail access is a
       word load/store or MOVSX; a 32-bit type would overlap the legacy save
       fields at +0x1797. */
    short text_line_cursor;
    unsigned int legacy_text_box_lines[2][3];
    /* 0x17af: the party's twelve effect slots, the same 0x11-byte records the
       monster and combat tables hold. The trailing run is opaque. */
    W8EffectSlot effect_slots[12];
    unsigned char unknown_187b[0x55];
    int party_facing;
    unsigned int party_heading;
    int world_clock;
    unsigned int world_clock_ms;
    unsigned int party_order_slots[8];
    int current_level;
    /* 0x1904..0x1a03: the 0x100-byte STAT header block. Assertion evidence
       names the dword at +0xd4 uiTurnsElapsed; the surrounding bytes remain a
       save/load blob. */
    unsigned char status_header_prefix[0xd4];
    unsigned int uiTurnsElapsed; /* 0x19d8 */
    unsigned char status_header_suffix[0x28];
    W8LevelProgressRow level_progress[W8_LEVEL_COUNT];
    unsigned char unknown_2013[0x294];
    /* 0x22a7: CamPos staged by recall when the anchor is on another level;
       LoadLevel restores it after the new world exists. */
    W8WorldCameraState pending_move_location;
    /* 0x22e3: the party-wide modifier block the effect rebuild clears and
       refills. Its +0x4a flag is the light gate the monster-sight threshold
       pass reads. */
    W8GameplayModifierBlock party_modifiers;
    int next_group_id;
    int next_monster_location_id;
    int next_world_item_id;
    int next_trigger_id;
    bool item_in_cursor;
    W8ItemInstance item_in_hand;
    /* 0x2367: per-slot flags the character-load path consults at 0x006874D7. */
    unsigned char flags[0x20];
    unsigned int game_time_ms;
    unsigned int aging_accumulator;
    /* 0x238f: search mode toggle. Mirrors the submenu search button, slows
       party movement, and scales the monster-sight threshold while set. */
    unsigned char search_mode;
    /* 0x2390: cleared by the main-game frame; HP/SP and condition updates
       skip work while it is set, and encounter culling treats it as the
       force-despawn gate. */
    bool world_suspended;
    /* 0x2391/0x2395: session accumulators ConsumeLevelElapsedTime
       folds the level's pending elapsed times into; the 0x00502D00 wait
       pass sums them against zero. */
    float real_elapsed;
    float frame_elapsed;
    unsigned int wait_state;
    unsigned int item_recharge_ms;
    W8PartyFormationState formation;
    int game_time_days;
    bool iron_man;
    /* 0x242a: world-clock stamp of the last NPC-binding reset; the event
       pass waits 0x3c ticks past it. */
    int binding_reset_clock;
    /* 0x242e: binding-reset grace period in effect; cleared once the sweep
       runs after the clock elapses. */
    bool binding_reset_pending;
    unsigned char padding_242f;
    /* 0x2430: one-shot gate for the NPC event pass. */
    bool npc_restore_pending;
    /* 0x2431: raised by the .nsf quote audit while it runs; ShowNotice counts
       each notice's wrapped lines under it. */
    unsigned char quote_audit;
    /* 0x2432: ShowNotice sets it under quote_audit when a notice wraps
       past seven lines; the audit reports those as "Long Quote". */
    unsigned char long_quote;
    bool party_fatigued;
    /* 0x2434: index of the party member the main-game selection flow is on.
       The screen reset writes 0xff and the 0x00526E90 handler reads and
       updates it while walking the 0x1862-byte character records. */
    unsigned char selected_party_member;
    /* 0x2435: read as a gate by the main-game frame's world-cursor path. */
    unsigned char world_cursor_gate;
    unsigned int camp_tick_ms;
    /* 0x243a: the five RPC race ids AssayDialog walks as NUM_RPC_RACES. */
    unsigned char rpc_races[5];
    unsigned char unknown_243f[5];
    /* Character creation skips the loose CHR collision check when set. */
    unsigned char skip_loose_character_check;
    /* 0x2445: latched once the Trynnie2 Zulu/0x1c3 use-item action has been
       handled at a cursor node; later uses take the Mystical Shaman branch. */
    bool use_item_latch;
    bool infatuation_pending;
    W8Difficulty difficulty;
    /* 0x244b: the save file's creation-time pair XOR-masked by SaveGame's
       two data constants; both halves are written as dwords. */
    unsigned int save_filetime_xor[2];
    wchar_t monster_name_buffer[22];
    /* 0x247f: party slot selected by the Sedexus path before rpc_active
       is armed; later capture, fact and death handling reuse the same slot. */
    int sedexus_party_slot;
    unsigned int stamina_tick_ms;
    unsigned char condition13_clock;
    /* 0x2488: one-shot gate; when set, the next condition-change and
       condition-cleared reaction is swallowed and the flag cleared. */
    unsigned char skip_next_condition_reaction;
    bool rpc_active; /* 0x2489: fact 0x14c gate */
    /* 0x248a: armed by the long NPC reward event; the event also stamps
       0x2493 with the world clock. */
    bool fact_b8_pending;
    unsigned int condition13_stamp;
    int pending_condition_party_slot;
    int fact_b8_clock;
    bool greeting_pending;
    unsigned int camp_fatigue_count;
    /* 0x249c: party slot fact 0x39 hands to RemoveCharacterCondition. */
    int party_slot;
    /* 0x24a0: per-spell 0x10-byte stat records; TrackItemSpellSource
       walks records[0..149] bumping usable_cast_count for spells the character
       carries and cast_count for the selected source spell. */
    W8ItemSpellUsageRecord item_spell_usage[200];
    unsigned char log_fact_checks;
    int status_ints[1000];
    bool fact_88_latch;
    unsigned char unknown_40c2[0xc];
    /* Retail addresses cast_count through +0x40c2 + spell_id * 0x10 and
       usable_cast_count through +0x40c6 + spell_id * 0x10. These are the
       +4/+8 fields of records rooted at +0x40ce and indexed by spell_id - 1.
       The typed extent is the proven player-spell domain, IDs 1..0x71.
       Spell 0x72 is item-routed in the audited producers; the unchecked cast
       update would enter the following unknown storage if it were supplied,
       so +0x47de is not an independently established original member boundary. */
    W8CharacterSpellUsageRecord spell_usage[0x71];
    unsigned char unknown_47de[0x194];
    /* 0x4972: set once the Cosmic Circle arena monsters have been spawned by
       the level-4 setup; the setup skips its work while this or world_suspended
       holds. */
    bool cc_arena_spawned;
    /* 0x4973/0x4977: GetTickCount stamps. NpcScriptSavantHackDone writes the
       first; UpdateNpcEvents retires NPC 0x1b3 fifty ticks later and starts
       the second, which gates monster group 0x1b6's Bela cycle after five
       seconds. */
    int savant_hack_tick;
    int bela_cycle_tick;
    unsigned char unknown_497b[4];
    /* 0x497f: combat difficulty band counters, indexed by EvaluateCombatDifficulty. */
    unsigned int combat_difficulty_counts[3];
    /* 0x498b: NPC group event counter, cleared once the group event runs. */
    int vi_event_stage;
    /* 0x498f/0x4993: pending-stage flags set by FACT_QUE_ENDGAME2/3; the book
       callback queues ENDGAME2's script notice while either holds. */
    int endgame2_queued;
    int endgame3_queued;
    unsigned int text_box_lines_used[4];
    unsigned int text_box_lines_shown[4];
    /* 0x49b7: world-clock stamp the 0x49bb reward event compares against. */
    int trang_check_clock;
    bool trang_check_pending;
    bool intro_shown;
    bool flag;
    unsigned char padding_49be[2];
    /* 0x49c0: set when the endgame transition starts; saves carrying either
       this or flag are filtered from the load list. */
    bool endgame_started;
    /* 0x49c1: latched while g_dev_mode is set at teardown; persisted into
       the save slot as dev_flagged. */
    bool dev_flagged;
};
#pragma pack(pop)

extern W8GlobalStatus g_status;

#include "wiz8/evidence/game_status_layout.inc"

#endif
