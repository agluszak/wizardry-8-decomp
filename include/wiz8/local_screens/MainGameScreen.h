#pragma once

class Trigger;
class srClass;
class stModelInstance2D;

#include "input.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/vector.h"

struct W8IList;
struct W8MipeState;
struct W8NpcState;
struct W8NpcScriptQuote;
struct W8ScreenRect;

void RequestRedrawParty(void);
void RefreshSelectedPartyPortrait(unsigned int party_slot);
void ClearHighlightIfItIs(const int* item);

/* One animated cursor resource the main-game / camp screens can install. The
   object is an stTextureAnim; size and hotspot are the unsigned shorts
   ApplyCurrentCursor feeds to the mouse-cursor surface helpers. */
struct W8MainGameResourceSlot {
    srClass* object;
    unsigned int frame_count;
    unsigned short size_x;
    unsigned short size_y;
    unsigned short hotspot_x;
    unsigned short hotspot_y;
    int image_id;
};
static_assert(sizeof(W8MainGameResourceSlot) == 0x14, "W8MainGameResourceSlot_size");
/* MainGameScreen.cpp GLOBAL at 0x006068E4: the "%s" display format. */
extern const wchar_t g_format_s_006068e4[];

extern W8MainGameResourceSlot g_main_game_resource_slots[17];
extern W8ScreenRect g_viewport_modes_647d30[];

#include "wiz8/layouts/screen_state.h"

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/dialog_code/DialogTextArea.h"

#include <cstddef>

/* Local Screens\MainGameScreen.cpp owns the live level-screen state. */

#pragma pack(push, 1)
/* The dormant typed-dialogue input state hung off the block's
   dialogue_state pointer: a plain heap object, not an srClass
   derivative. Its unrecovered consumers (0x0058D7E0..0x0058F250) establish
   this layout; 0x0058D940 frees text, pending_word and line_offsets and then
   the object with operator delete. */
struct W8DialogueTextState {
    wchar_t* text;              /* 0x00 owned buffer the input edits in place */
    wchar_t* pending_word;      /* 0x04 owned buffer carried across line wraps */
    int unknown_08;             /* 0x08 */
    unsigned int* line_offsets; /* 0x0c owned array of per-line char offsets */
    unsigned int line_count;    /* 0x10 */
    unsigned int text_capacity; /* 0x14 in characters; 0x0058D7E0 grows it */
    unsigned int line_capacity; /* 0x18 in entries; 0x0058D890 grows by 0x20 */
    int unknown_1c;             /* 0x1c */
    short character_index;      /* 0x20 selected character the input binds to */
    short unknown_22;
    unsigned int wrap_width;  /* 0x24 pixel budget per line */
    unsigned int cursor;      /* 0x28 char index of the insertion point */
    unsigned char unknown_2c; /* 0x2c */
    unsigned char dirty;      /* 0x2d set on each append */
    unsigned char unknown_2e[2];
};

struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf0];
    unsigned char flag_0f0; /* 0x0f0 */
    unsigned char unknown_0f1[3];
    unsigned int redraw_flags; /* 0x0f4 */
    unsigned char unknown_0f8[4];
    int value_0fc;                             /* 0x0fc */
    int camera_mode_100;                       /* 0x100 */
    unsigned int hover_region;                 /* 0x104 */
    unsigned char flag_108;                    /* 0x108 */
    unsigned char portrait_refresh_pending[8]; /* 0x109 */
    unsigned char unknown_111[3];
    int values_114[8]; /* 0x114 */
    int values_134[8]; /* 0x134 */
    unsigned char pick_changed_154;
    unsigned char flag_155;
    unsigned char formation_board_visible; /* 0x156: formation board shown */
    unsigned char flag_157;
    unsigned char unknown_158[0x14];
    int highlight_override;      /* 0x16c */
    int values_170[8];           /* 0x170 */
    int held_item_display_190;   /* 0x190 */
    int values_194[5];           /* 0x194 */
    unsigned int text_lines[12]; /* 0x1a8 */
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    unsigned char dialogue_open;
    unsigned char unknown_1f9[3];
    /* The dormant free-text dialogue input state. dialogue_open gates the
       unrecovered input handler at 0x0058F250 (called from the main-game input
       pump); no instruction in the GOG-base binary ever sets it or installs
       the object, so the typed-input cluster it feeds - buffer grow
       0x0058D7E0/0x0058D890, character append 0x0058D9C0, re-wrap
       0x0058DCA0, teardown 0x0058D940, region handler 0x0058E9F0 - is dead
       code here and no producer of the pointer exists. It never passes
       ReleaseObject004257F0. */
    W8DialogueTextState* dialogue_state;
    /* 0x200: tracked party slots. [0] is the slot the portrait/info overlay
       tracks; [3] is the hovered condition-button slot that owns the mode-6
       highlight overlay. -1 while untracked. */
    int values_200[4];      /* 0x200 */
    unsigned char flag_210; /* 0x210 */
    unsigned char unknown_211[3];
    unsigned int clock_214; /* 0x214 */
    unsigned char flag_218; /* 0x218 */
    unsigned char unknown_219[7];
    int dialogue_x_220;
    unsigned int dialogue_y_224; /* ClearSurfaceRect's unsigned top/bottom */
    unsigned int dialogue_height_228;
    unsigned char unknown_22c[0xc];
    int dialogue_width_238;
    int value_23c; /* 0x23c */
    /* The dialogue highlight sprite. The unrecovered dialogue-box draw at
       0x00563FC0 lazily creates it from catalog object 0x72 through
       Function4255C0 - the retail assertion spells it
       gpMGSV->pHighlightGraphic - and it is released through
       ReleaseObject004257F0 whenever mode 6 ends or the tracked party slots
       change. */
    stModelInstance2D* highlight_graphic; /* 0x240 */
    unsigned int world_update_flags;      /* 0x244 */
    unsigned int world_render_flags;      /* 0x248 */
    unsigned char unknown_24c;
    unsigned char flag_24d;
    unsigned char unknown_24e[2];
    unsigned int character_update_timer; /* 0x250 */
    unsigned int world_update_timer;     /* 0x254 */
    unsigned int countdown_258;          /* 0x258 */
    unsigned int countdown_25c;          /* 0x25c */
    unsigned char transition_active;     /* 0x260 */
    unsigned char transition_pending;    /* 0x261 */
    unsigned char unknown_262[2];
    int highlighted_item;
    int selected_item;
    unsigned int countdown_26c; /* 0x26c */
    unsigned char flag_270;
    unsigned char flag_271;
    unsigned char flag_272;
    unsigned char unknown_273;
    unsigned int tick_274; /* 0x274 */
    int value_278;         /* 0x278 */
    int pending_level;
    int pending_entry_id;
    int value_284; /* 0x284 */
    int value_288; /* 0x288 */
    int value_28c; /* 0x28c */
    unsigned char unknown_290[0x10];
    /* 0x2a0..0x2a8: the formation board's three stModelInstance2D-family
       sprites - the board art with slot markers baked in, the rotating compass
       needle tracking party_facing against party_heading, and a lazily
       created overlay.  All are produced by Function4255C0 in MGSFormation.cpp
       and released through ReleaseObject004257F0. */
    stModelInstance2D* formation_board_sprite;
    stModelInstance2D* formation_compass_sprite;
    stModelInstance2D* formation_overlay_sprite;
    int value_2ac; /* 0x2ac */
    int value_2b0; /* 0x2b0 */
    int value_2b4; /* 0x2b4 */
    unsigned char unknown_2b8[8];
    unsigned char refresh_combat_panel;
    unsigned char unknown_2c1[3];
    unsigned int combat_panel_timer;
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
    unsigned short* palette_2ec; /* 0x2ec */
    int selection_kind;
    int value_2f4; /* 0x2f4 */
    unsigned char selection_settled;
    unsigned char unknown_2f9[3];
    unsigned int tooltip_since;
    unsigned char tooltip_pending;
    unsigned char unknown_301[3];
    int tooltip_subject;
    int tooltip_kind;
    unsigned int countdown_30c; /* 0x30c */
    int combat_slot;            /* 0x310 */
    unsigned char flag_314;
    unsigned char unknown_315[3];
    int hover_combat_slot; /* 0x318 */
    unsigned char flag_31c;
    unsigned char unknown_31d[3];
    unsigned int countdown_320;
    unsigned char flag_324;
    unsigned char formation_board_alternate; /* 0x325: highlighted board art while hovered */
    unsigned char flag_326;
    unsigned char flag_327;
    unsigned char flag_328;
    unsigned char unknown_329[3];
    unsigned int countdown_32c;
};
#pragma pack(pop)

static_assert(sizeof(W8LevelRuntimeBlock) == 0x330, "W8LevelRuntimeBlock_must_be_0x330");
static_assert(offsetof(W8LevelRuntimeBlock, flag_0f0) == 0x0f0, "W8LevelRuntimeBlock_flag_0f0");
static_assert(offsetof(W8LevelRuntimeBlock, value_0fc) == 0x0fc, "W8LevelRuntimeBlock_value_0fc");
static_assert(offsetof(W8LevelRuntimeBlock, portrait_refresh_pending) == 0x109,
              "W8LevelRuntimeBlock_portrait_refresh_pending");
static_assert(offsetof(W8LevelRuntimeBlock, values_114) == 0x114, "W8LevelRuntimeBlock_values_114");
static_assert(offsetof(W8LevelRuntimeBlock, values_134) == 0x134, "W8LevelRuntimeBlock_values_134");
static_assert(offsetof(W8LevelRuntimeBlock, values_170) == 0x170, "W8LevelRuntimeBlock_values_170");
static_assert(offsetof(W8LevelRuntimeBlock, values_194) == 0x194, "W8LevelRuntimeBlock_values_194");
static_assert(offsetof(W8LevelRuntimeBlock, values_200) == 0x200, "W8LevelRuntimeBlock_values_200");
static_assert(offsetof(W8LevelRuntimeBlock, flag_210) == 0x210, "W8LevelRuntimeBlock_flag_210");
static_assert(offsetof(W8LevelRuntimeBlock, clock_214) == 0x214, "W8LevelRuntimeBlock_clock_214");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_258) == 0x258,
              "W8LevelRuntimeBlock_countdown_258");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_26c) == 0x26c,
              "W8LevelRuntimeBlock_countdown_26c");
static_assert(offsetof(W8LevelRuntimeBlock, palette_2ec) == 0x2ec,
              "W8LevelRuntimeBlock_palette_2ec");
static_assert(offsetof(W8LevelRuntimeBlock, countdown_32c) == 0x32c,
              "W8LevelRuntimeBlock_countdown_32c");

class W8MainGameScreen;
class W8MainGameTextPanel;

/* 0x00587CF0 constructs this concrete key handler.  Its primary vtable is the
   W8Widget table extended by one entry: slot 0x48 points at
   0x00588170 and accepts the key code forwarded by TextBoxHandleKey. */
// VTABLE: WIZ8 0x005eeafc
class W8MainGameTextKeyHandler : public W8Widget, public W8RangeListener {
public:
    W8MainGameTextKeyHandler(Controls* panel, int left, int top, int right, int bottom,
                             int line_count, unsigned short* field_ac, unsigned int* region_set);
    virtual ~W8MainGameTextKeyHandler() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;
    virtual void AdjustValue(int steps) override;
    virtual void OnLeftButtonUp(int event) override;
    void SetSelectedLine(int line);
    virtual char HandleKey(unsigned short key);
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8RangeControl m_range_038;
    int m_line_count_0a4;
    int m_visible_lines_0a8;
    unsigned short* m_field_0ac;
    int m_field_0b0;
    int m_field_0b4;
    int m_field_0b8;
    W8RangeListener* m_range_listener_0bc;
};
static_assert(sizeof(W8MainGameTextKeyHandler) == 0xc0, "W8MainGameTextKeyHandler_size");

/* 0xc0-byte text-panel cell. W8TextControl is 0xb8; the extra dword at +0xb8
   holds the displayed catalog image (or -1) and +0xbc gates mouse handling.
   Slot 0 is its own scalar deleting destructor at 0x00588350, not
   W8TextControl's. */
// VTABLE: WIZ8 0x005eeb4c
class W8MainGameTextEntry : public W8TextControl {
public:
    W8MainGameTextEntry(Controls* panel, int index);
    virtual ~W8MainGameTextEntry() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;

    int m_image_b8;
    unsigned char m_input_blocked_bc;
    unsigned char m_pad_bd[3];
};
static_assert(sizeof(W8MainGameTextEntry) == 0xc0, "W8MainGameTextEntry_size");

/* The text panel's constructor at 0x005884D0 begins with Controls::Controls.
   The two secondary bases are installed at 0x4c and 0x50, before its own
   fields. Ordinary destructor 0x00588790; 0x00588770 is the scalar deleting
   wrapper. */
// VTABLE: WIZ8 0x005eeba8
class W8MainGameTextPanel : public Controls,
                            public W8TextControl::Listener,
                            public W8RangeListener {
public:
    W8MainGameTextPanel();          /* 0x005884D0 */
    virtual ~W8MainGameTextPanel(); /* 0x00588790 */
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8MainGameTextEntry* m_entries_054[8];
    W8MainGameTextKeyHandler* m_key_handler_074;
    int m_selection_078;
    W8MainGameScreen* m_screen_07c;
    int* m_values_080;
    unsigned char m_flag_084;
    unsigned char m_pad_085[3];
    float m_field_088;
    float m_field_08c;
    int m_field_090;
    W8GameTimer m_timer_094;
    W8ControlsRect m_text_bounds_0b8;
    W8TextBuffer m_text_buffer_0c8;
    W8GameTimer m_timer_118;
    int m_field_13c;
    unsigned char m_target_changed_140;
    unsigned char m_flag_141;
    unsigned char m_pad_142[2];
};
static_assert(sizeof(W8MainGameTextPanel) == 0x144, "W8MainGameTextPanel_size");

/* The 0x00588A90 constructor establishes a Controls-derived status panel.
   Ordinary destructor 0x00588DB0; 0x00588D90 is the scalar deleting wrapper. */
// VTABLE: WIZ8 0x005eebc0
class W8MainGameStatusPanel005EEBC0 : public Controls {
public:
    W8MainGameStatusPanel005EEBC0();          /* 0x00588A90 */
    virtual ~W8MainGameStatusPanel005EEBC0(); /* 0x00588DB0 */
    virtual void Redraw() override;
    void RefreshStatusTexts(); /* 0x00588E60 */

    W8TextBuffer* m_text_04c;
    W8TextBuffer* m_text_050;
    W8TextBuffer* m_text_054;
    W8TextBuffer* m_text_058;
    W8TextBuffer* m_text_05c;
    W8TextBuffer* m_text_060;
    W8TextBuffer* m_text_064;
    int m_target_068;
};
static_assert(sizeof(W8MainGameStatusPanel005EEBC0) == 0x6c, "W8MainGameStatusPanel005EEBC0_size");

/* The 0x50-byte panel stored at W8MainScreenState+0x1a8 (bounds
   0x17,0x166-0xa4,0x1c2); it hosts the six option buttons at +0x170..+0x184.
   Its SetEnabled keeps those six inactive unless the expanded NPC dialogue
   layout (value_fc == 4) is up, and its Redraw substitutes m_value_4c for
   m_renderArg_20 in that mode. The constructor is inlined into 0x0056D1D0 as
   the Controls base call plus m_value_4c = 0x11; no standalone derived body
   exists. */
// VTABLE: WIZ8 0x005ee9f0
class W8MainGamePanel005EE9F0 : public Controls {
public:
    W8MainGamePanel005EE9F0(int left, int top, int new_right, int new_bottom, int render_target,
                            int render_arg_1c, int render_arg_20)
        : Controls(left, top, new_right, new_bottom, render_target, render_arg_1c, render_arg_20)
    {
        m_value_4c = 0x11;
    }
    virtual void SetEnabled(bool enable) override; /* 0x0056BC50 */
    virtual void Redraw() override;                /* 0x0056BD30 */

    int m_value_4c; /* 0x4c: catalog image used while value_fc == 4 */
};
static_assert(sizeof(W8MainGamePanel005EE9F0) == 0x50, "W8MainGamePanel005EE9F0_size");

/* The Controls-sized panel stored at W8MainScreenState+0x1c0 (bounds
   0x1dc,0x166-0x269,0x1c0). Enabling it starts text-input scheme 1 and
   installs the typed-dialogue input field; Redraw also draws the input-frame
   image at y 0x19b while flag_1d9 is raised, else 0x18b. The constructor is the
   plain Controls base call inlined at 0x0056D1D0 with no extra fields. */
// VTABLE: WIZ8 0x005ee9e4
class W8MainGamePanel005EE9E4 : public Controls {
public:
    W8MainGamePanel005EE9E4(int left, int top, int new_right, int new_bottom, int render_target,
                            int render_arg_1c, int render_arg_20)
        : Controls(left, top, new_right, new_bottom, render_target, render_arg_1c, render_arg_20)
    {
    }
    virtual void SetEnabled(bool enable) override; /* 0x0056BAC0 */
    virtual void Redraw() override;                /* 0x0056BB20 */
};
static_assert(sizeof(W8MainGamePanel005EE9E4) == 0x4c, "W8MainGamePanel005EE9E4_size");

/* 0x0055DE40 constructs this Controls-derived NPC dialogue text controller:
   Controls base, six dwords, then the W8DialogTextArea at +0x64 for a 0xBC
   total. W8MainScreenState stores the live instance at +0x1b0. */
// VTABLE: WIZ8 0x005ee920
class W8NpcDialogueTextController : public Controls {
public:
    bool HandleScrollDownCommand(unsigned int command);
    bool HandleScrollUpCommand(unsigned int command);

    int unknown_4c;
    int unknown_50;
    int visible;                /* 0x54 */
    int line_height;            /* 0x58 */
    int margin;                 /* 0x5c */
    int scroll_height;          /* 0x60 */
    W8DialogTextArea text_area; /* 0x64 */
};
static_assert(sizeof(W8NpcDialogueTextController) == 0xbc, "W8NpcDialogueTextController_size");
static_assert(offsetof(W8NpcDialogueTextController, visible) == 0x54,
              "W8NpcDialogueTextController_visible");
static_assert(offsetof(W8NpcDialogueTextController, line_height) == 0x58,
              "W8NpcDialogueTextController_line_height");
static_assert(offsetof(W8NpcDialogueTextController, margin) == 0x5c,
              "W8NpcDialogueTextController_margin");
static_assert(offsetof(W8NpcDialogueTextController, scroll_height) == 0x60,
              "W8NpcDialogueTextController_scroll_height");
static_assert(offsetof(W8NpcDialogueTextController, text_area) == 0x64,
              "W8NpcDialogueTextController_text_area");

/* The hover/click target the dialogue controller embeds for its text area
   (constructed by 0x0055E570, stored in W8MainScreenState at +0x130, parented
   to the +0x1b0 controller). Entering selects the first visible entry,
   leaving clears the selection, and the button handlers track the press in
   m_flags_34 before firing the widget callbacks. */
// VTABLE: WIZ8 0x005ee92c
class W8NpcDialogueScrollWidget : public W8Widget {
public:
    W8NpcDialogueScrollWidget(Controls* panel, unsigned int region, int left, int top, int right,
                              int bottom); /* 0x0055E570 */
    /* The ordinary destructor at 0x0055E5D0 is a pure JMP thunk to
       W8Widget::~W8Widget; there is no authored body to match. */
    // SYNTHETIC: WIZ8 0x0055E5B0
    // W8NpcDialogueScrollWidget::`scalar deleting destructor'

    virtual ~W8NpcDialogueScrollWidget() override {}
    virtual void OnMouseEnter(int event) override;     /* 0x0055E5E0 */
    virtual void OnMouseLeave(int event) override;     /* 0x0055E610 */
    virtual void OnLeftButtonDown(int event) override; /* 0x0055E640 */
    virtual void OnLeftButtonUp(int event) override;   /* 0x0055E660 */

    unsigned int m_flags_34; /* 0x34: bit 0 while the primary button is held */
};
static_assert(sizeof(W8NpcDialogueScrollWidget) == 0x38, "W8NpcDialogueScrollWidget_size");
static_assert(offsetof(W8NpcDialogueScrollWidget, m_flags_34) == 0x34,
              "W8NpcDialogueScrollWidget_flags_34");

class W8LockTumbler;

/* One-slot callback a W8LockTumbler holds at +0x44 and invokes with itself on
   left-button release (0x00585690). 0x005eeabc is the construction-phase table
   emitted for the abstract interface; the implementing secondary base sits at
   W8LockTumblerPanel+0x4c (table 0x005eeaa8). */
// VTABLE: WIZ8 0x005eeabc
class W8LockTumblerListener {
public:
    virtual void OnTumblerReleased(W8LockTumbler* tumbler) = 0;
};

/* One lock-picking pin widget (0x48 bytes, created inline inside the
   W8LockTumblerPanel constructor loop, final table 0x005eea60). +0x3c is the
   per-pin pattern byte copied from Trigger::state_370.bytes_01; +0x40 is the
   pin's current pixel offset, 0x22 at rest. */
// VTABLE: WIZ8 0x005eea60
class W8LockTumbler : public W8Widget {
public:
    /* Inlined into 0x005856E0: the W8Widget base call plus the field writes
       below; no standalone derived body exists. */
    W8LockTumbler(Controls* panel, int left, int top, int right, int bottom, int pin_index)
        : W8Widget(panel, 0xffffffff, left, top, right, bottom), m_pin_set_34(0), m_rising_35(0),
          m_falling_36(0), m_at_top_37(0), m_hovered_38(0), m_pin_index_3c(pin_index),
          m_pin_height_40(0x22), m_listener_44(0)
    {
    }
    virtual void Redraw(int full_redraw) override;   /* 0x005854B0 */
    virtual void OnMouseEnter(int event) override;   /* 0x00585610 */
    virtual void OnMouseLeave(int event) override;   /* 0x00585650 */
    virtual void OnLeftButtonUp(int event) override; /* 0x00585690 */

    bool m_pin_set_34;                    /* 0x34: raised and holding */
    bool m_rising_35;                     /* 0x35: animating toward its target height */
    bool m_falling_36;                    /* 0x36: dropping back to rest */
    bool m_at_top_37;                     /* 0x37: redrawn against the shared tumble phase */
    bool m_hovered_38;                    /* 0x38 */
    int m_pin_index_3c;                   /* 0x3c: column into the pin pattern tables */
    int m_pin_height_40;                  /* 0x40: pixel offset, 0x22 at rest */
    W8LockTumblerListener* m_listener_44; /* 0x44 */
};
static_assert(sizeof(W8LockTumbler) == 0x48, "W8LockTumbler_size");

/* One-slot callback W8LockTumblerPanel holds at +0xe8 and invokes with the
   released tumbler's index (0x00585950). 0x005eeae0 is the construction-phase
   table; W8LockInteraction implements it on its primary base. */
// VTABLE: WIZ8 0x005eeae0
class W8LockTumblerPanelListener {
public:
    virtual void OnTumblerPicked(int index) = 0;
};

/* The tumbler strip of the lock interaction (0xec bytes, ctor 0x005856E0,
   primary table 0x005eeaac, W8LockTumblerListener secondary at +0x4c with
   table 0x005eeaa8). Owns the eight tumblers and the three animation timers:
   the phase timer steps the shared sway, the rise timer moves a pin toward
   g_lock_pin_target_height_64ba80, the fall timer returns it to rest. */
// VTABLE: WIZ8 0x005eeaac
class W8LockTumblerPanel : public Controls, public W8LockTumblerListener {
public:
    W8LockTumblerPanel(int tumbler_count, const unsigned char* pin_data); /* 0x005856E0 */
    // SYNTHETIC: WIZ8 0x005858a0
    // W8LockTumblerPanel::`scalar deleting destructor'

    virtual ~W8LockTumblerPanel();                                   /* 0x005858C0 */
    virtual void OnTumblerReleased(W8LockTumbler* tumbler) override; /* 0x00585950 */
    void UpdateTumblerAnimation();                                   /* 0x00585990 */

    int m_tumbler_count_50;          /* 0x50: pins in use, clamped to [2,8] */
    W8LockTumbler* m_tumblers_54[8]; /* 0x54 */
    unsigned char m_animating_74;    /* 0x74: a pin is in flight; input is locked out */
    unsigned char unknown_75[3];
    int m_phase_78;               /* 0x78: sway accumulator feeding g_lock_phase_68f2b4 */
    W8GameTimer m_phase_timer_7c; /* 0x7c: 0.04s */
    W8GameTimer m_rise_timer_a0;  /* 0xa0: 0.03s */
    W8GameTimer m_fall_timer_c4;  /* 0xc4: 0.01s */
    W8LockTumblerPanelListener* m_listener_e8; /* 0xe8 */
};
static_assert(sizeof(W8LockTumblerPanel) == 0xec, "W8LockTumblerPanel_size");
static_assert(offsetof(W8LockTumblerPanel, m_tumblers_54) == 0x54, "W8LockTumblerPanel_tumblers");
static_assert(offsetof(W8LockTumblerPanel, m_listener_e8) == 0xe8, "W8LockTumblerPanel_listener");

/* The lock interaction's readout column (0x6c bytes, ctor 0x00585B00): the
   selected character's name plus the lockpick-skill, spell-power and
   force-chance percentages, one W8TextBuffer per row. */
// VTABLE: WIZ8 0x005eeac0
class W8LockInfoPanel : public Controls {
public:
    W8LockInfoPanel(int tumbler_count); /* 0x00585B00 */
    // SYNTHETIC: WIZ8 0x00585e00
    // W8LockInfoPanel::`scalar deleting destructor'

    virtual ~W8LockInfoPanel();     /* 0x00585E20 */
    virtual void Redraw() override; /* 0x00586120 */
    void RefreshInfo();             /* 0x00585ED0 */

    int m_tumbler_count_4c;   /* 0x4c */
    W8TextBuffer* m_text_050; /* 0x50: character name */
    W8TextBuffer* m_text_054; /* 0x54 */
    W8TextBuffer* m_text_058; /* 0x58: lockpick skill */
    W8TextBuffer* m_text_05c; /* 0x5c */
    W8TextBuffer* m_text_060; /* 0x60: spell power */
    W8TextBuffer* m_text_064; /* 0x64 */
    W8TextBuffer* m_text_068; /* 0x68: force chance */
};
static_assert(sizeof(W8LockInfoPanel) == 0x6c, "W8LockInfoPanel_size");

/* The lock-picking interaction root allocated at 0x0068F2C0 (0xa4 bytes, ctor
   0x005861A0, destructor 0x005866A0). The primary base answers the tumbler
   panel's "released tumbler N" callback; the secondary W8TextControl::Listener
   at +0x04 (table 0x005eead0) receives the action-panel buttons. Process() at
   0x00586740 is the per-frame state machine ProcessLockInteractMode drives. */
// VTABLE: WIZ8 0x005eead8
class W8LockInteraction : public W8LockTumblerPanelListener, public W8TextControl::Listener {
public:
    W8LockInteraction(Trigger* trigger); /* 0x005861A0 */
    // SYNTHETIC: WIZ8 0x00586680
    // W8LockInteraction::`scalar deleting destructor'

    virtual ~W8LockInteraction();                            /* 0x005866A0 */
    virtual void OnTumblerPicked(int index) override;        /* 0x00586B10 */
    virtual void OnPrimary(W8TextControl* control) override; /* 0x00586C00 */
    void Process();                                          /* 0x00586740 */
    void ResolvePick();                                      /* 0x00586C60 */
    void AttemptForce();                                     /* 0x00586E40 */
    void EnablePanels(int enable);                           /* 0x00586AF0 */
    void BeginUnlock();                                      /* 0x005874D0 */

    Trigger* m_trigger_08;  /* 0x08 */
    int m_tumbler_count_0c; /* 0x0c: trigger->value_36c clamped to [2,8] */
    W8LockTumblerPanel* m_tumbler_panel_10;
    W8LockInfoPanel* m_info_panel_14;
    Controls* m_action_panel_18;
    W8TextControl* m_done_button_1c;      /* 0x1c: OnPrimary target, state 9 */
    W8TextControl* m_spell_button_20;     /* 0x20: gated by spell-0x27 power */
    W8TextControl* m_force_button_24;     /* 0x24: gated by the force chance */
    W8TextControl* m_cancel_button_28;    /* 0x28: OnPrimary target, state 5 (cancel) */
    int m_selected_slot_2c;               /* 0x2c: party slot owning the raised pins */
    int m_picked_tumbler_30;              /* 0x30: index OnTumblerPicked recorded */
    int m_state_34;                       /* 0x34: Process() state */
    int m_tumbler_owner_38[8];            /* 0x38: owning party slot per pin, -1 unset */
    unsigned char m_tumbler_locked_58[8]; /* 0x58: pin kept when the slot is released */
    int m_slot_attempts_60[8];            /* 0x60: pick attempts per party slot */
    W8GameTimer m_timer_80;               /* 0x80: state-8 completion delay */
};
static_assert(sizeof(W8LockInteraction) == 0xa4, "W8LockInteraction_size");
static_assert(offsetof(W8LockInteraction, m_trigger_08) == 0x08, "W8LockInteraction_trigger");
static_assert(offsetof(W8LockInteraction, m_tumbler_panel_10) == 0x10,
              "W8LockInteraction_tumbler_panel");
static_assert(offsetof(W8LockInteraction, m_state_34) == 0x34, "W8LockInteraction_state");
static_assert(offsetof(W8LockInteraction, m_timer_80) == 0x80, "W8LockInteraction_timer");

/* 0x005eebdc is the construction-phase primary table installed at the start
   of 0x00589160; 0x005eebd8 is the complete-object table. Slot 0 is a pure
   virtual the complete object implements at 0x00589550 (the text-entry
   selection path). The ordinary destructor at 0x005894B0 is non-virtual.
   W8TextControl::Listener is the proven secondary base at +0x04: collapsing
   it into Listener-only inheritance would move Listener to +0 and shrink the
   object. */
// VTABLE: WIZ8 0x005eebdc
class W8MainGameTextSelectionListener005EEBDC {
public:
    virtual void SelectTextEntry(int index) = 0;
};

// VTABLE: WIZ8 0x005eebd8
class W8MainGameScreen : public W8MainGameTextSelectionListener005EEBDC,
                         public W8TextControl::Listener {
public:
    W8MainGameScreen(Trigger* owner); /* 0x00589160 */
    ~W8MainGameScreen();              /* 0x005894B0 */
    virtual void SelectTextEntry(int index) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    void Update();                           /* 0x00589A80 */
    void RefreshActionPanel();               /* 0x00589D90 */
    void EnablePanelRegionSets(bool enable); /* 0x0058A030 */
    void ApplyInspectSuccess();              /* 0x0058A060 */
    void CastTrapSpell();                    /* 0x0058A200 */
    void UseTrapItem();                      /* 0x0058A3E0 */

    Trigger* m_owner_008;
    W8MainGameTextPanel* m_text_panel_00c;
    W8MainGameStatusPanel005EEBC0* m_status_panel_010;
    Controls* m_action_panel_014;
    int m_state_018;
    int m_selected_character_01c;
    W8TextControl* m_action_controls_020[5];
    int m_field_034;
    int m_field_038;
    unsigned char m_unknown_03c[8];
    unsigned char m_slot_flag_044[8];
    int m_slot_values_04c[8][8];
    int m_target_14c;
    int m_field_150;
    W8GameTimer m_timer_154;
};
static_assert(sizeof(W8MainGameScreen) == 0x178, "W8MainGameScreen_size");

extern W8LevelRuntimeBlock* g_level_block;
extern W8MainGameScreen* g_main_game_screen;

class W8DialogBase;
struct W8ItemInstance;
struct W8PendingNoticeLine {
    wchar_t* text;
    int npc_kind;
};

extern W8DialogBase* g_modal_owner_0068edd0;
extern W8DialogBase* g_pending_main_game_dialog_0068edd4;

/* Open the assay (item info) dialog for an item, evaluated against the party
   slot's character; -1 means no character. The current modal owner, if any,
   moves to the pending slot. */
void OpenAssayDialog0056AE20(W8ItemInstance* item, int character_slot); /* 0x0056AE20 */

#pragma pack(push, 1)
struct W8MainScreenState {
    unsigned char unknown_000[0xf8];
    int target_location_id_f8;
    int value_fc; /* 0xfc: dialogue layout mode; 577880 requires 3 */
    unsigned char unknown_100[4];
    int value_104;
    unsigned char unknown_108[0x28];
    W8NpcDialogueScrollWidget* dialogue_scroll_130; /* 0x130 */
    W8Widget* dialogue_widget_134;                  /* 0x134 */
    W8Widget* dialogue_widget_138;                  /* 0x138 */
    unsigned char unknown_13c[0x34];
    /* The six option buttons hosted by panel_1a8; they activate only while
       value_fc == 4. Created as plain W8TextControls (regions 0x75..0x7a) by
       0x0056D1D0. */
    W8TextControl* option_buttons_170[6];
    unsigned char unknown_188[0x20];
    W8MainGamePanel005EE9F0* panel_1a8; /* 0x1a8 */
    unsigned char unknown_1ac[4];
    W8NpcDialogueTextController* npc_dialogue_controller_1b0; /* 0x1b0 */
    Controls* npc_dialogue_panel_1b4;                         /* 0x1b4 */
    unsigned char unknown_1b8[8];
    W8MainGamePanel005EE9E4* text_input_panel_1c0; /* 0x1c0 */
    unsigned char unknown_1c4[0x10];
    W8NpcState* dialogue_npc;
    /* 0x1d8 and 0x1ec: two bytes the screen reset writes 0xff and 0. */
    unsigned char flag_1d8;
    unsigned char flag_1d9;
    unsigned char unknown_1da[0x12];
    unsigned char flag_1ec;
    unsigned char unknown_1ed[0xd];
    unsigned char script_busy; /* 0x1fa: set 0xff during script execution */
    unsigned char unknown_1fb[0xd];
    int quote_bubble;
    short quote_x;
    short quote_y;
    short quote_width;
    short quote_height;
    bool quote_visible;
    unsigned char unknown_215[3];
    W8GrowableVector<W8PendingNoticeLine*> pending_notice_lines; /* 0x218 */
    unsigned char dialogue_cursor_flag;                          /* 0x228 */
    unsigned char unknown_229[0xb];
    unsigned char flag_234;
    unsigned char unknown_235[3];
    int value_238;
    unsigned char flag_23c;
    unsigned char unknown_23d[0xb];
    unsigned char quote_notice_kind;
    unsigned char unknown_249[3];
    void* quote_notice_payload;
    unsigned char flag_250;
    unsigned char unknown_251;
    unsigned char flag_252;
    unsigned char unknown_253[5];
    /* 0x258: the screen reset writes -1 here, the no-selection value. */
    int value_258;
    unsigned char unknown_25c[4];
    /* 0x260: raised by the screen reset. */
    unsigned char flag_260;
    unsigned char unknown_261;
    unsigned char dialogue_panel_hidden; /* 0x262 */
    unsigned char unknown_263;
    int last_notice_npc_kind; /* 0x264 */
};
#pragma pack(pop)
static_assert(sizeof(W8MainScreenState) == 0x268, "W8MainScreenState_size");
static_assert(offsetof(W8MainScreenState, dialogue_widget_134) == 0x134,
              "W8MainScreenState_dialogue_widget_134");
static_assert(offsetof(W8MainScreenState, dialogue_widget_138) == 0x138,
              "W8MainScreenState_dialogue_widget_138");
static_assert(offsetof(W8MainScreenState, npc_dialogue_controller_1b0) == 0x1b0,
              "W8MainScreenState_npc_dialogue_controller_1b0");
static_assert(offsetof(W8MainScreenState, npc_dialogue_panel_1b4) == 0x1b4,
              "W8MainScreenState_npc_dialogue_panel_1b4");
static_assert(offsetof(W8MainScreenState, script_busy) == 0x1fa, "W8MainScreenState_script_busy");
static_assert(offsetof(W8MainScreenState, dialogue_npc) == 0x1d4, "W8MainScreenState_dialogue_npc");
static_assert(offsetof(W8MainScreenState, quote_bubble) == 0x208, "W8MainScreenState_quote_bubble");
static_assert(offsetof(W8MainScreenState, quote_visible) == 0x214,
              "W8MainScreenState_quote_visible");
static_assert(offsetof(W8MainScreenState, quote_notice_kind) == 0x248,
              "W8MainScreenState_quote_notice_kind");
static_assert(offsetof(W8MainScreenState, quote_notice_payload) == 0x24c,
              "W8MainScreenState_quote_notice_payload");
static_assert(offsetof(W8MainScreenState, pending_notice_lines) == 0x218,
              "W8MainScreenState_pending_notice_lines");
static_assert(offsetof(W8MainScreenState, dialogue_cursor_flag) == 0x228,
              "W8MainScreenState_dialogue_cursor_flag");
static_assert(offsetof(W8MainScreenState, dialogue_panel_hidden) == 0x262,
              "W8MainScreenState_dialogue_panel_hidden");
static_assert(offsetof(W8MainScreenState, last_notice_npc_kind) == 0x264,
              "W8MainScreenState_last_notice_npc_kind");

extern W8MainScreenState* g_screen_state_00649f1c;
void OnQuitGameDialogClosed(W8DialogBase* dialog);

void PauseMainGameWorld(void);
void ResumeMainGameWorld(void);
void ForwardNpcScriptNotice(W8NpcState* npc, int value, int line, int suppress);
void Function56C5E0(W8NpcState* npc, int value, int line, int suppress, int arg); /* 0x0056C5E0 */
void ResetMainGameScreenState(void);
void FlushPendingNoticeLines005766B0(void); /* 0x005766B0 */
/* 0x0056C520: zero W8MainScreenState, write its reset values, and reload the
   keyword lists through the loader below. */
void ResetMainScreenStateBlock(void);

/* 0x0068EE80: the dialogue keyword tables, one file list per language;
   element zero is English_Keywords.txt and element one the translated list.
   A file list holds one line list per line, and a line list one malloc'd wide
   word per '/'-separated field. */
extern W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*> g_keyword_lists;
/* 0x0068F0F8: both files are loaded and the tables are usable. Raised once the
   second file loads and lowered whenever the tables are released. */
extern unsigned char g_keyword_lists_loaded_68f0f8;

/* 0x0056C200: replace the keyword lists with the contents of
   Data\Strings\English_Keywords.txt and Data\Strings\translated_Keywords.txt. */
void ReloadKeywordLists(void);
/* 0x0056C130: release every file list, its lines and its words. */
void ClearKeywordLists(void);
/* 0x0056BED0: load one keyword file into a file list. */
unsigned char LoadKeywordFile(const char* path,
                              W8GrowableVector<W8GrowableVector<wchar_t*>*>* file);
/* 0x0056BE40: copy the next '/'-terminated field out of a keyword line into
   the caller's buffer and return the cursor past it, or null at the end. */
wchar_t* ParseKeywordToken(wchar_t* line, wchar_t* field);

struct W8NpcScriptQuote;

void Function577260(void);
/* 0x005775D0: queue a named scripted action (kind 0 item, 1 NPC, 2/3 other);
   resolves the name against the item and NPC tables when kind is -1 and
   ignores duplicates already pending. */
void Function5775D0(wchar_t* name, char kind);
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette); /* 0x00576030 */
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette, unsigned char notice_kind,
                              void* payload, int npc_kind); /* 0x00576060 */
void DrawNpcQuoteBubble(void);                              /* 0x00576670 */
void LookAtDialogueNpc(void);                               /* 0x005767F0 */
void CloseNpcDialogueIfActive(void);                        /* 0x00576B80 */
void Function56C6D0(W8NpcState* npc, const W8ItemInstance* item, int b, int c,
                    int d); /* 0x0056C6D0 */
void Function577520(void);  /* 0x00577520 */
void Function570A20(void);  /* 0x00570A20 */
void Function570CF0(void);  /* 0x00570CF0 */
void Function56CA90(void);  /* 0x0056CA90 */
unsigned char CanOpenNpcDialogue(void);
bool IsNpcDialogueTextBoxActive(void);               /* 0x0056EFD0 */
unsigned char SetNpcDialoguePanelVisible(int value); /* 0x00577880 */
unsigned char Function577A40(void);
void __fastcall
CollapseNpcDialogueTextArea(W8NpcDialogueTextController* controller);               /* 0x0055E2C0 */
void __fastcall ExpandNpcDialogueTextArea(W8NpcDialogueTextController* controller); /* 0x0055E1E0 */
void __fastcall
ClearNpcDialogueTextBackground(W8NpcDialogueTextController* controller);            /* 0x0055EAE0 */
bool __fastcall IsNpcDialogueTextExpanded(W8NpcDialogueTextController* controller); /* 0x0055E2B0 */
/* Which party portrait the pointer is over, if any. */
unsigned int HitTestPartyPortrait(const InputAtom* event);
void RequestRefreshPartyState(void);
void Function593330(void);
int IsScreenIdle(void);
bool IsModalOpen(void);

void RequestRedraw(unsigned int mask);
void SetTooltipSubject(int kind, int subject); /* 0x00569C60 */
int IsScreenInputBlocked(void);
void DisableCombatRegions(void);
void Function577220(void);
void ClearMainGameTargetState(void);

extern unsigned short g_value_006840be;
extern unsigned char g_flag_00685071;
extern int g_value_00685072;
extern unsigned char g_flag_00685076;
extern signed char g_value_00685077;
extern unsigned char g_flag_006840bc;
extern unsigned char g_flag_00685070;

void HandleManualCameraHotkeys(void);
void ApplyWorldRenderHotkeys(void);
extern unsigned char g_flag_0068edbc;
extern unsigned char g_flag_0068edc8;
extern unsigned char g_flag_0068edc9;
extern unsigned char g_flag_0068edd8;
extern int g_main_game_mode_0068eddc;
extern int g_value_64c1c8;
void RequestLevelTransition005615F0(int level, int entry, unsigned char flag);
extern unsigned char g_build_level_links_0065bd2c;
extern int g_next_link_level_0068ede8;
extern unsigned char g_flag_0068edd9;
extern unsigned char g_debug_monster_cycle_0068f0fc;
extern unsigned char g_navigator_position_changed_659c11;
extern unsigned char g_flag_006840bb;
void BeginLevelTransition(void); /* 0x005611A0 */
void SetViewportMode(int mode);  /* 0x005618F0 */
/* Apply a change to the main-game mode flag at 0x006850CE. */
void ApplyMainGameModeFlag(int previous_mode, char enable); /* 0x00562580 */
/* 0x00561EC0: the region-mode pass the party-add entry runs while the
   main-game screen is current. */
void Function561EC0(void);
void Function563DD0(void);
void DismissHighlightOverlay(void); /* 0x00563EB0 */
void Function565740(int slot);
void Function568E10(void);
short Function5698C0(void);
void Function56CA60(W8NpcState* npc, int, int, int, int); /* 0x0056CA60 */
void Function56E800(int);
unsigned char Function56EC90(unsigned int party_slot);
void Function5777C0(void);
void Function587510(int value);
void Function5879A0(int);
void Function58A470(int value);
void UpdateMainGameScreen(void); /* 0x0058A750 */
void Function58A790(int);
void Function595600(void);
int GetPartySlotSkill10Level(int slot);
int OpenLockInteraction00587510(Trigger* trigger);
int OpenTrapInteraction0058A470(Trigger* trigger);
/* 0x0056A770: when a slot's committed action cannot execute, re-choose a
   fallback hand, breath or character attack, or reroute spell/item aiming. */
void FallbackFromUnreachableAction(int party_slot);
void SetCombatAction(int value);    /* 0x0056A480 */
void SetCombatSelection(int value); /* 0x00569F70 */
void SetCombatTarget(int value);    /* 0x0056A2D0 */

void RequestRedrawCombatBar(void);    /* 0x005699B0 */
void UpdateScreenOverlays(int frame); /* 0x0056AF20 */
void DisableMainRegionSet(void);      /* 0x00561FB0 */
void Function598AB0(void);
void Function59C930(int slot);
void Function598AE0(void);
void Function59B270(void);
void Function59C9C0(void);
unsigned char MainGameScreenInitialize(void);
unsigned char MainGameScreenEnter(void);
void MainGameScreenFrame(void);
unsigned char MainGameScreenLeave(int leaving);
void ShortenTextToWidth00577410(wchar_t* output, const wchar_t* text, unsigned int width, int font);
extern W8MipeState* g_mipe_state_0068f100;
void ShowMainGameNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, int confirmation,
                            int cancel); /* 0x00569A50 */
