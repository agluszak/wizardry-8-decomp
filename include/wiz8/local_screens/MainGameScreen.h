#pragma once

#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"

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

class W8MainGameScreen005EEBD8;

/* 0x00587CF0 constructs this concrete key handler.  Its primary vtable is the
   W8WidgetBase005ED5BC table extended by one entry: slot 0x48 points at
   0x00588170 and accepts the key code forwarded by TextBoxHandleKey. */
// VTABLE: WIZ8 0x005eeafc
class W8MainGameTextKeyHandler005EEAFC
    : public W8WidgetBase005ED5BC,
      public W8RangeListener {
public:
    virtual char HandleKey(unsigned short key);
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) override;

    W8RangeControl005ED74C m_range_038;
    int m_field_0a4;
    int m_field_0a8;
    int m_field_0ac;
    int m_field_0b0;
    int m_field_0b4;
    int m_field_0b8;
    W8RangeListener* m_range_listener_0bc;
};
static_assert(sizeof(W8MainGameTextKeyHandler005EEAFC) == 0xc0,
              "W8MainGameTextKeyHandler005EEAFC_size");

/* The text panel's constructor at 0x005884D0 begins with Controls::Controls.
   The two secondary bases are installed at 0x4c and 0x50, before its own
   fields. */
// VTABLE: WIZ8 0x005eeba8
class W8MainGameTextPanel005EEBA8
    : public Controls,
      public W8TextControl005ED604::Listener,
      public W8RangeListener {
public:
    W8MainGameTextPanel005EEBA8();                    /* 0x005884D0 */
    virtual ~W8MainGameTextPanel005EEBA8();           /* 0x00588770 */
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) override;

    W8TextControl005ED604* m_entries_054[8];
    W8MainGameTextKeyHandler005EEAFC* m_key_handler_074;
    int m_selection_078;
    W8MainGameScreen005EEBD8* m_screen_07c;
    int* m_values_080;
    unsigned char m_flag_084;
    unsigned char m_unknown_085[0xf];
    W8GameTimer m_timer_094;
    W8ControlsRect m_text_bounds_0b8;
    W8TextBuffer005ED5B8 m_text_buffer_0c8;
    W8GameTimer m_timer_118;
    int m_field_13c;
    unsigned char m_target_changed_140;
    unsigned char m_flag_141;
    unsigned char m_pad_142[2];
};
static_assert(sizeof(W8MainGameTextPanel005EEBA8) == 0x144,
              "W8MainGameTextPanel005EEBA8_size");

/* The 0x00588A90 constructor establishes a Controls-derived status panel. */
// VTABLE: WIZ8 0x005eebc0
class W8MainGameStatusPanel005EEBC0 : public Controls {
public:
    W8MainGameStatusPanel005EEBC0();                  /* 0x00588A90 */
    virtual ~W8MainGameStatusPanel005EEBC0();         /* 0x00588D90 */
    virtual void Redraw() override;

    W8TextBuffer005ED5B8* m_text_04c;
    W8TextBuffer005ED5B8* m_text_050;
    W8TextBuffer005ED5B8* m_text_054;
    W8TextBuffer005ED5B8* m_text_058;
    W8TextBuffer005ED5B8* m_text_05c;
    W8TextBuffer005ED5B8* m_text_060;
    W8TextBuffer005ED5B8* m_text_064;
    int m_target_068;
};
static_assert(sizeof(W8MainGameStatusPanel005EEBC0) == 0x6c,
              "W8MainGameStatusPanel005EEBC0_size");

/* The primary base supplies the pure virtual destructor table installed at
   the start of 0x00589160.  W8TextControl005ED604::Listener is the proven
   secondary base at +0x04. */
// VTABLE: WIZ8 0x005eebdc
class W8MainGameScreenBase005EEBDC {
public:
    virtual ~W8MainGameScreenBase005EEBDC() = 0;
};

// VTABLE: WIZ8 0x005eebd8
class W8MainGameScreen005EEBD8
    : public W8MainGameScreenBase005EEBDC,
      public W8TextControl005ED604::Listener {
public:
    W8MainGameScreen005EEBD8(void* owner);            /* 0x00589160 */
    virtual ~W8MainGameScreen005EEBD8() override;     /* 0x005894B0 */
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}

    void* m_owner_008;
    W8MainGameTextPanel005EEBA8* m_text_panel_00c;
    W8MainGameStatusPanel005EEBC0* m_status_panel_010;
    Controls* m_action_panel_014;
    int m_state_018;
    unsigned char m_unknown_01c[4];
    W8TextControl005ED604* m_action_controls_020[5];
    int m_field_034;
    int m_field_038;
    unsigned char m_unknown_03c[0x110];
    int m_target_14c;
    int m_field_150;
    W8GameTimer m_timer_154;
};
static_assert(sizeof(W8MainGameScreen005EEBD8) == 0x178,
              "W8MainGameScreen005EEBD8_size");

extern "C" {
extern W8LevelRuntimeBlock* g_level_block;
extern W8MainGameScreen005EEBD8* g_main_game_screen_0068f2d4;
}

void Function56AA30(void);
void Function56AAB0(void);
