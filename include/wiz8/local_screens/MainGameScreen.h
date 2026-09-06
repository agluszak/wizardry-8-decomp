#pragma once

/* Local Screens\MainGameScreen.cpp owns the live level-screen state. */

#pragma pack(push, 1)
struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf4];
    unsigned int redraw_flags;
    unsigned char unknown_0f8[8];
    int camera_mode_100;
    unsigned int hover_region;
    unsigned char unknown_108[0x4c];
    unsigned char pick_changed_154;
    unsigned char flag_155;
    unsigned char unknown_156[0x16];
    int highlight_override;
    unsigned char unknown_170[0x38];
    int text_lines[12];
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    unsigned char dialogue_open;
    unsigned char unknown_1f9[3];
    unsigned char* dialogue_owner;
    unsigned char unknown_200[0x64];
    int highlighted_item;
    int selected_item;
    unsigned char unknown_26c[0x10];
    int pending_level;
    int pending_entry_id;
    unsigned char unknown_284[0x3c];
    unsigned char refresh_combat_panel;
    unsigned char unknown_2c1[7];
    unsigned char refresh_party_panel;
    unsigned char unknown_2c9;
    short combat_end_notification;
    int scroll_top;
    unsigned char unknown_2d0[4];
    int scroll_bottom;
    unsigned char unknown_2d8[4];
    int move_budget_2dc;
    int move_budget_2e0;
    unsigned char unknown_2e4[4];
    int value_2e8;
    unsigned char unknown_2ec[4];
    int selection_kind;
    unsigned char unknown_2f4[4];
    unsigned char selection_settled;
    unsigned char unknown_2f9[3];
    unsigned int tooltip_since;
    unsigned char tooltip_pending;
    unsigned char unknown_301[3];
    int tooltip_subject;
    int tooltip_kind;
    unsigned char unknown_30c[0x1b];
    unsigned char flag_327;
};
#pragma pack(pop)

static_assert(sizeof(W8LevelRuntimeBlock) == 0x328,
              "W8LevelRuntimeBlock_must_be_0x328");

extern "C" {
extern W8LevelRuntimeBlock* g_level_block;
extern unsigned char* g_main_game_screen_0068f2d4;
}
