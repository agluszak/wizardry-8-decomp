#pragma once

#include "wiz8/engine_code/Levels.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/saved_location.h"

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
    unsigned char game_started;          /* 0x000c */
    unsigned char unknown_000d[0x0c];
    unsigned int party_gold;
    int selected_character;
    W8ItemInstance party_item_pool_0021[500];
    int party_item_count_1791;
    unsigned char unknown_1795[2];
    unsigned int legacy_text_box_lines_1797[2][3];
    unsigned char unknown_17af[0x121];
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
    unsigned char unknown_22e3[0x67];
    int status_count_234a;
    int next_monster_location_id_234e;
    int next_world_item_id_2352;
    int next_trigger_id_2356;
    unsigned char item_in_cursor;
    W8ItemInstance item_in_hand_235b;
    unsigned char unknown_2367[0x20];
    int game_time_ms;
    unsigned char unknown_238b[0x16];
    W8PartyFormationState formation;
    int game_time_days;
    unsigned char unknown_2429[0x6e];
    unsigned char flag_2497;
    unsigned char unknown_2498[0x24ff];
    unsigned int text_box_lines_used_4997[4];
    unsigned int text_box_lines_shown_49a7[4];
    unsigned char unknown_49b7[0x0b];
};
#pragma pack(pop)

static_assert(sizeof(W8StatusBuffers) == 0x0c,
              "W8StatusBuffers_must_be_0x0c");
static_assert(sizeof(W8PartyFormationRow) == 0x03,
              "W8PartyFormationRow_must_be_0x03");
static_assert(sizeof(W8PartyFormationPosition) == 0x0c,
              "W8PartyFormationPosition_must_be_0x0c");
static_assert(sizeof(W8PartyFormationState) == 0x84,
              "W8PartyFormationState_must_be_0x84");
static_assert(offsetof(W8GlobalStatus, party_gold) == 0x19,
              "W8GlobalStatus_party_gold_offset");
static_assert(offsetof(W8GlobalStatus, selected_character) == 0x1d,
              "W8GlobalStatus_selected_character_offset");
static_assert(offsetof(W8GlobalStatus, party_facing) == 0x18d0,
              "W8GlobalStatus_party_facing_offset");
static_assert(offsetof(W8GlobalStatus, current_level) == 0x1900,
              "W8GlobalStatus_current_level_offset");
static_assert(offsetof(W8GlobalStatus, formation) == 0x23a1,
              "W8GlobalStatus_formation_offset");
static_assert(offsetof(W8GlobalStatus, text_box_lines_used_4997) == 0x4997,
              "W8GlobalStatus_migrated_values_offset");
static_assert(sizeof(W8GlobalStatus) == 0x49c2,
              "W8GlobalStatus_must_be_0x49c2");

extern W8GlobalStatus g_status_685170;

#define g_party_facing (g_status_685170.party_facing)
#define g_party_heading (g_status_685170.party_heading)
#define g_world_clock_00686a48 (g_status_685170.world_clock)
#define g_current_level (g_status_685170.current_level)
#define g_level_progress (g_status_685170.level_progress)
#define g_pending_move_location_00687417 \
    (g_status_685170.pending_move_location)
#define g_saved_world_position_00687417 \
    (g_status_685170.pending_move_location.point)
#define g_game_time_ms (g_status_685170.game_time_ms)
#define g_game_time_days (g_status_685170.game_time_days)

void Function554580(unsigned char* storage);
