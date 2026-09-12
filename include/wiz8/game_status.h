#pragma once

#include "Types.h"

#include "gameloop.h"

#include "wiz8/engine_code/Levels.h"
#include "wiz8/gameplay_modifiers.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/saved_location.h"
#include "wiz8/text_types.h"

#include <stddef.h>

struct W8PartySlotRow;
struct W8Character;

enum { W8_PARTY_SLOT_COUNT = 8 };

#pragma pack(push, 1)
struct W8StatusBuffers {
    float save_version;
    W8Character* characters;
    W8PartySlotRow* party_rows;
};

enum { W8_CHARACTER_SERIALIZED_SIZE = 0x1862 };

struct W8PartyFormationRow {
    signed char slots[3];
};

struct W8PartyFormationPosition {
    unsigned char row;
    unsigned char unknown_01[2];
    signed char facing;
    unsigned char unknown_04[8];
};

struct W8PartyFormationState {
    W8PartyFormationRow rows[5];
    unsigned char flags_0f[5];
    W8PartyFormationPosition positions[8];
    unsigned char unknown_74[0x10];
};

struct W8GlobalStatus {
    W8StatusBuffers buffers;
    unsigned char game_started; /* 0x000c */
    /* 0x000d..0x0018: the three join counters the party-add entry advances:
       the regular-member, auxiliary and total counts. */
    int unknown_000d[3];
    unsigned int party_gold;
    int selected_character;
    W8ItemInstance party_item_pool_0021[500];
    int party_item_count_1791;
    /* 0x1795: signed 16-bit text-box line cursor. Every retail access is a
       word load/store or MOVSX; a 32-bit type would overlap the legacy save
       fields at +0x1797. */
    short text_line_cursor_1795;
    unsigned int legacy_text_box_lines_1797[2][3];
    /* 0x17af: the party's twelve effect slots, the same 0x11-byte records the
       monster and combat tables hold. The trailing run is opaque. */
    W8EffectSlot effect_slots_17af[12];
    unsigned char unknown_187b[0x55];
    int party_facing;
    unsigned int party_heading;
    int world_clock;
    unsigned char unknown_18dc[4];
    unsigned int dwords_18e0[8];
    int current_level;
    unsigned char status_header_block_1904[0x100];
    W8LevelProgressRow level_progress[47];
    unsigned char unknown_2013[0x294];
    W8SavedLocation pending_move_location;
    /* 0x22e3: the party-wide modifier block the effect rebuild clears and
       refills. Its +0x4a flag is the light gate the monster-sight threshold
       pass reads. */
    W8GameplayModifierBlock party_modifiers_22e3;
    int status_count_234a;
    int next_monster_location_id_234e;
    int next_world_item_id_2352;
    int next_trigger_id_2356;
    unsigned char item_in_cursor;
    W8ItemInstance item_in_hand_235b;
    /* 0x2367: per-slot flags the character-load path consults at 0x006874D7. */
    unsigned char flags_2367[0x20];
    int game_time_ms;
    unsigned char unknown_238b[4];
    /* 0x238f: scales the monster-sight threshold while set. */
    unsigned char flag_238f;
    /* 0x2390: cleared by the main-game frame; the rest of the run is opaque. */
    unsigned char value_2390;
    unsigned char unknown_2391[0x10];
    W8PartyFormationState formation;
    int game_time_days;
    unsigned char iron_man;
    /* 0x242a: world-clock stamp the 0x2497 event compares against. */
    int value_242a;
    /* 0x242e: the mark the NPC-binding reset stamps next to the clock. */
    unsigned char flag_242e;
    unsigned char unknown_242f;
    /* 0x2430: one-shot gate for the NPC event pass. */
    unsigned char flag_2430;
    unsigned char unknown_2431[3];
    /* 0x2434: index of the party member the main-game selection flow is on.
       The screen reset writes 0xff and the 0x00526E90 handler reads and
       updates it while walking the 0x1862-byte character records. */
    unsigned char selected_party_member_2434;
    /* 0x2435: read as a gate by the main-game frame's world-cursor path. */
    unsigned char value_2435;
    unsigned char unknown_2436[0x0e];
    /* Character creation skips the loose CHR collision check when set. */
    unsigned char skip_loose_character_check_2444;
    unsigned char unknown_2445[2];
    int difficulty;
    unsigned char unknown_244b[8];
    W8WideChar monster_name_buffer_2453[22];
    unsigned char alternate_name_slot_247f;
    unsigned char unknown_2480[9];
    unsigned char flag_2489; /* 0x2489: fact 0x14c gate */
    /* 0x248a: armed by the long NPC reward event; the event also stamps
       0x2493 with the world clock. */
    unsigned char flag_248a;
    unsigned char unknown_248b[8];
    int value_2493;
    unsigned char flag_2497;
    unsigned char unknown_2498[0xc88];
    unsigned char log_fact_checks_3120;
    /* 0x3121 (ABS 0x688291): 1000 consecutive dwords. EndCombat walks exactly
       this run flipping 1 -> 2; the extent is representation-proven even
       though the semantics are not. */
    int status_ints_3121[1000];
    /* 0x40c1: the 0x88 fact reads and writes this byte. */
    unsigned char flag_40c1;
    unsigned char unknown_40c2[0x17b];
    /* 0x423d: party-slot-like dword the 0x14c fact compares against occupied
       slots. */
    int value_423d;
    unsigned char unknown_4241[0x732];
    /* 0x4973/0x4977: GetTickCount stamps that retire NPC 0x1b3 and then start
       the 0x1b6 cycle. */
    int value_4973;
    int value_4977;
    unsigned char unknown_497b[0x10];
    /* 0x498b: NPC group event counter, cleared once the group event runs. */
    int value_498b;
    unsigned char unknown_498f[8];
    unsigned int text_box_lines_used_4997[4];
    unsigned int text_box_lines_shown_49a7[4];
    /* 0x49b7: world-clock stamp the 0x49bb reward event compares against. */
    int value_49b7;
    unsigned char flag_49bb;
    unsigned char flag_49bc;
    unsigned char flag_49bd;
    unsigned char unknown_49be[2];
    unsigned char flag_49c0;
    unsigned char flag_49c1;
};
#pragma pack(pop)

static_assert(sizeof(W8StatusBuffers) == 0x0c, "W8StatusBuffers_must_be_0x0c");
static_assert(sizeof(W8PartyFormationRow) == 0x03, "W8PartyFormationRow_must_be_0x03");
static_assert(sizeof(W8PartyFormationPosition) == 0x0c, "W8PartyFormationPosition_must_be_0x0c");
static_assert(sizeof(W8PartyFormationState) == 0x84, "W8PartyFormationState_must_be_0x84");
static_assert(offsetof(W8GlobalStatus, party_gold) == 0x19, "W8GlobalStatus_party_gold_offset");
static_assert(offsetof(W8GlobalStatus, selected_character) == 0x1d,
              "W8GlobalStatus_selected_character_offset");
static_assert(offsetof(W8GlobalStatus, party_item_pool_0021) == 0x21,
              "W8GlobalStatus_party_item_pool_offset");
static_assert(offsetof(W8GlobalStatus, party_item_count_1791) == 0x1791,
              "W8GlobalStatus_party_item_count_offset");
static_assert(offsetof(W8GlobalStatus, text_line_cursor_1795) == 0x1795,
              "W8GlobalStatus_text_line_cursor_offset");
static_assert(offsetof(W8GlobalStatus, flags_2367) == 0x2367, "W8GlobalStatus_flags_2367_offset");
static_assert(offsetof(W8GlobalStatus, monster_name_buffer_2453) == 0x2453,
              "W8GlobalStatus_monster_name_buffer_offset");
static_assert(offsetof(W8GlobalStatus, alternate_name_slot_247f) == 0x247f,
              "W8GlobalStatus_alternate_name_slot_offset");
static_assert(offsetof(W8GlobalStatus, flag_2497) == 0x2497, "W8GlobalStatus_flag_2497_offset");
static_assert(offsetof(W8GlobalStatus, log_fact_checks_3120) == 0x3120,
              "W8GlobalStatus_log_fact_checks_offset");
static_assert(offsetof(W8GlobalStatus, text_box_lines_shown_49a7) == 0x49a7,
              "W8GlobalStatus_text_box_lines_shown_offset");
static_assert(offsetof(W8GlobalStatus, flag_49bc) == 0x49bc, "W8GlobalStatus_flag_49bc_offset");
static_assert(offsetof(W8GlobalStatus, party_facing) == 0x18d0,
              "W8GlobalStatus_party_facing_offset");
static_assert(offsetof(W8GlobalStatus, current_level) == 0x1900,
              "W8GlobalStatus_current_level_offset");
static_assert(offsetof(W8GlobalStatus, formation) == 0x23a1, "W8GlobalStatus_formation_offset");
static_assert(offsetof(W8GlobalStatus, value_2390) == 0x2390, "W8GlobalStatus_value_2390_offset");
static_assert(offsetof(W8GlobalStatus, selected_party_member_2434) == 0x2434,
              "W8GlobalStatus_selected_party_member_offset");
static_assert(offsetof(W8GlobalStatus, value_2435) == 0x2435, "W8GlobalStatus_value_2435_offset");
static_assert(offsetof(W8GlobalStatus, text_box_lines_used_4997) == 0x4997,
              "W8GlobalStatus_migrated_values_offset");
static_assert(offsetof(W8GlobalStatus, flag_2489) == 0x2489, "W8GlobalStatus_flag_2489_offset");
static_assert(offsetof(W8GlobalStatus, flag_40c1) == 0x40c1, "W8GlobalStatus_flag_40c1_offset");
static_assert(offsetof(W8GlobalStatus, value_423d) == 0x423d, "W8GlobalStatus_value_423d_offset");
static_assert(sizeof(W8GlobalStatus) == 0x49c2, "W8GlobalStatus_must_be_0x49c2");

extern W8GlobalStatus g_status_685170;

void InitializePartyFormation(unsigned char* storage);
