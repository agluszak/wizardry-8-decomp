#pragma once

class Trigger;
class srClass;
class stModelInstance2D;

#include "input.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/vector.h"

struct W8IList;
struct W8MipeState;
struct W8NpcDialogRequest;
struct W8NpcState;
struct W8NpcScriptQuote;
struct W8ScreenRect;

void RequestRedrawParty(void);
void RedrawCombatMonsterList(void); /* 0x00565440 */
void RefreshSelectedPartyPortrait(unsigned int party_slot);
void ClearHighlightIfItIs(const int* item);

#include "wiz8/layouts/main_game_screen.h"
/* MainGameScreen.cpp GLOBAL at 0x006068E4: the "%s" display format. */
extern const wchar_t g_format_s_006068e4[];
/* MainGameScreen.cpp GLOBAL at 0x0064BAB0: the "%d%%" display format. */
extern const wchar_t g_format_d_percent_0064bab0[];
/* MainGameScreen.cpp GLOBAL at 0x0061C3E0: the "%s: %s" display format. */
extern const wchar_t g_format_s_colon_s_0061c3e0[];
/* MainGameScreen.cpp GLOBAL at 0x006481B4: the "%s: %s (%d)" display format. */
extern const wchar_t g_format_s_colon_s_paren_d_006481b4[];
/* MainGameScreen.cpp GLOBAL at 0x0064DA8C: the " %s : " display format. */
extern const wchar_t g_format_s_spaced_colon_0064da8c[];

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
// VTABLE: WIZ8 0x005eeba0 W8TextControl::Listener
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
   layout (value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) is up, and its Redraw substitutes m_value_4c for
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

    int m_value_4c; /* 0x4c: catalog image used while value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX */
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
    W8NpcDialogueTextController(int panel_left, int panel_top, int panel_right, int panel_bottom,
                                int render_target, int render_arg_1c, int render_arg_20,
                                int margin_image, int line_image); /* 0x0055DE40 */
    virtual void Redraw() override;                                /* 0x0055DF80 */
    bool HandleScrollDownCommand(unsigned int command);
    bool HandleScrollUpCommand(unsigned int command);
    /* Add one keyword line to the transcript unless the text is already
       present; a nonzero mark puts the new entry in state 0x60, and any
       leftover "[No Keywords]" placeholder is removed afterwards. */
    unsigned char AddTranscriptEntry(const wchar_t* text, signed char category,
                                     char mark); /* 0x0055E0C0 */
    /* Whether the expanded transcript's top edge reaches above the portrait
       band for an odd party slot (1 -> 0x67, 3 -> 0xbc, 5 -> 0x111, 7 ->
       always covered). Callers use it to skip portrait work on rows the
       open transcript covers. */
    unsigned char IsSlotPortraitTranscriptCovered(unsigned int party_slot); /* 0x0055E410 */
    /* Re-apply the category filter, rebuild the expansion and restate the
       scroll widgets. */
    void SetTranscriptCategoryFilter(signed char category); /* 0x0055E7C0 */
    /* Replay every saved dialogue_transcript record into the text area,
       adding the "[No Keywords]" placeholder and forcing the all filter
       when nothing was saved. */
    void RestoreTranscriptEntries(); /* 0x0055E840 */
    /* Snapshot the transcript lines into the screen state's
       dialogue_transcript records (text plus category byte). */
    void SaveTranscriptEntries(); /* 0x0055E940 */
    /* Drop every transcript line from the text area and invalidate. */
    void ClearTranscriptEntries();                  /* 0x0055EA40 */
    int GetSelectedTranscriptEntryIndex();          /* 0x0055EAB0 */
    void SetTranscriptSorted(unsigned char sorted); /* 0x0055EAC0 */
    void RemoveSelectedTranscriptEntry();           /* 0x0055EA70 */

    /* 0x4c/0x50: catalog image ids whose measured heights seed margin and
       line_height/scroll_height in the constructor; never read again. */
    int margin_image;
    int line_image;
    int visible;                                       /* 0x54 */
    int line_height;                                   /* 0x58 */
    int margin;                                        /* 0x5c */
    int scroll_height;                                 /* 0x60 */
    W8DialogTextArea text_area;                        /* 0x64 */
    void SelectTranscriptKeywordAtPoint(int x, int y); /* 0x0055E490 */
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
    /* Retail ICF folds this onto W8NpcDialogueScrollWidget's deleting destructor. */
    // SYNTHETIC: WIZ8 0x0055E5B0 FOLDED
    // W8LockTumbler::`scalar deleting destructor'

    /* Inlined into 0x005856E0: the W8Widget base call plus the field writes
       below; no standalone derived body exists. */
    W8LockTumbler(Controls* panel, int left, int top, int right, int bottom, int pin_index)
        : W8Widget(panel, 0xffffffff, left, top, right, bottom), m_pin_set_34(0), m_rising_35(0),
          m_falling_36(0), m_at_top_37(0), m_hovered_38(0), m_pin_index_3c(pin_index),
          m_pin_height_40(0x22), m_listener_44(0)
    {
    }
    virtual void Redraw(int full_redraw) override; /* 0x005854B0 */
    virtual void OnMouseEnter(int event) override; /* 0x00585610 */
    virtual void OnMouseLeave(int event) override; /* 0x00585650 */
    /* Retail folds this with W8HorizontalRangeThumb::OnMouseEnter at 0x004F58C0. */
    virtual void OnLeftButtonDown(int event) override;
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
// VTABLE: WIZ8 0x005eead0 W8TextControl::Listener
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
// VTABLE: WIZ8 0x005eebd0 W8TextControl::Listener
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

#pragma pack(push, 1)
/* 0x0068EE60: the NPC script notice queued between 0x0056C5E0 and its
   DispatchPendingNpcScriptNotice dispatch. flag and force are stored as
   independent bytes at +0x14/+0x15; the dispatch reloads +0x14 as one dword
   for the BeginNpcDialogueInternal flags argument and takes force back out of
   its high byte, leaving +0x16/+0x17 as dead tail bytes. */
struct W8PendingNotice {
    W8NpcState* npc;
    W8ItemInstance item;
    int line;
    unsigned char flag;
    unsigned char force;
    unsigned char unused_16[2];
};
#pragma pack(pop)
extern W8PendingNotice g_pending_notice_68ee60;

extern W8DialogBase* g_modal_owner_0068edd0;
void OpenModal(W8DialogBase* owner);
extern W8DialogBase* g_pending_main_game_dialog_0068edd4;

/* Open the assay (item info) dialog for an item, evaluated against the party
   slot's character; -1 means no character. The current modal owner, if any,
   moves to the pending slot. */
void OpenMonsterInfoDialog0056AD60(int location_id);                    /* 0x0056AD60 */
void OpenAssayDialog0056AE20(W8ItemInstance* item, int character_slot); /* 0x0056AE20 */

/* W8MainScreenState::value_fc - which NPC dialogue layout is up. Layout 1 is
   not evidenced. */
enum W8NpcDialogueLayout {
    W8_DIALOGUE_LAYOUT_NONE = 0,
    W8_DIALOGUE_LAYOUT_TOPIC_MENU = 2,
    W8_DIALOGUE_LAYOUT_TRANSCRIPT = 3,
    W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX = 4
};

/* Transcript keyword categories stored in
   W8MainScreenState::dialogue_category_filter and on each
   W8DialogTextEntry::m_category. The five filter buttons spell them
   "Items", "People", "Places", "Misc" and "All"; 0x00571660 classifies a
   keyword by scanning the item records, the NPC records plus the
   named-monster table and the fixed place-name string table, in that
   order. -1 shows every category. */
enum W8DialogueCategory {
    W8_DIALOGUE_CATEGORY_ALL = -1,
    W8_DIALOGUE_CATEGORY_ITEMS = 0,
    W8_DIALOGUE_CATEGORY_PEOPLE = 1,
    W8_DIALOGUE_CATEGORY_PLACES = 2,
    W8_DIALOGUE_CATEGORY_MISC = 3
};

/* One saved transcript keyword, malloc'd/freed as a 0xca-byte record:
   100 wide characters of text, then the W8DialogueCategory byte. Save-game
   serialization writes the length-prefixed text and the trailing category
   byte; it never persists the pad. */
struct W8DialogueTranscriptRecord {
    wchar_t text[100];
    signed char category;
    unsigned char pad_0c9;
};
static_assert(sizeof(W8DialogueTranscriptRecord) == 0xca, "W8DialogueTranscriptRecord_size");

#pragma pack(push, 1)
struct W8MainScreenState {
    /* 0x000: a word 0x0056CAD0 clears while the dialogue opens. */
    short value_000;
    unsigned char unknown_002[0xee];
    W8MainUiMode value_f0; /* 0x0f0: the pre-dialogue display mode 0x56cad0 saves */
    /* 0x0f4: the party slot 0x0056D030 picks as the dialogue's leading
       speaker - the occupied row whose character leads skill 0x16. */
    int dialogue_speaker;
    int target_location_id_f8;
    int value_fc;
    int value_100;
    int value_104;
    W8ItemInstance* value_108;
    W8TextControl* dialogue_text_10c; /* 0x10c: the NPC-name caption */
    W8TextControl* dialogue_text_110;
    W8TextControl* dialogue_text_114;
    W8TextControl* dialogue_text_118;
    W8TextControl* dialogue_text_11c;
    W8TextControl* dialogue_text_120;
    W8TextControl* dialogue_text_124;
    W8TextControl* dialogue_text_128;
    W8Widget* dialogue_widget_12c;
    W8NpcDialogueScrollWidget* dialogue_scroll_130; /* 0x130 */
    /* 0x134/0x138: the transcript scroll arrows, wired to
       ScrollNpcDialogueUp/ScrollNpcDialogueDown and enabled only while the
       text area is fully expanded. */
    W8Widget* dialogue_scroll_up_button;   /* 0x134 */
    W8Widget* dialogue_scroll_down_button; /* 0x138 */
    W8TextControl* dialogue_text_13c;
    W8TextControl* dialogue_text_140;
    unsigned char unknown_144[4];
    /* 0x148: the "Sort Alphabetically" toggle. 0x150..0x160: the five
       transcript category buttons in People/Places/Items/Misc/All label
       order, each wired to its SelectNpcDialogueCategory* callback. */
    W8TextControl* dialogue_sort_button;
    unsigned char unknown_14c[4];
    W8TextControl* dialogue_people_button;
    W8TextControl* dialogue_places_button;
    W8TextControl* dialogue_items_button;
    W8TextControl* dialogue_misc_button;
    W8TextControl* dialogue_all_button;
    W8TextControl* dialogue_text_164;
    W8TextControl* dialogue_text_168;
    W8TextControl* dialogue_text_16c;
    /* The six option buttons hosted by panel_1a8; they activate only while
       value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX. Created as plain W8TextControls (regions 0x75..0x7a) by
       0x0056D1D0. */
    W8TextControl* option_buttons_170[6];
    W8TextControl* dialogue_text_188;
    unsigned char unknown_18c[4];
    W8TextControl* dialogue_text_190;
    W8TextControl* dialogue_text_194;
    W8TextControl* dialogue_text_198;
    W8TextControl* dialogue_text_19c;
    W8TextControl* dialogue_text_1a0;
    W8TextControl* dialogue_text_1a4;
    W8MainGamePanel005EE9F0* panel_1a8;                       /* 0x1a8 */
    Controls* panel_1ac;                                      /* 0x1ac */
    W8NpcDialogueTextController* npc_dialogue_controller_1b0; /* 0x1b0 */
    Controls* npc_dialogue_panel_1b4;                         /* 0x1b4 */
    Controls* panel_1b8;                                      /* 0x1b8 */
    Controls* panel_1bc;                                      /* 0x1bc */
    W8MainGamePanel005EE9E4* text_input_panel_1c0;            /* 0x1c0 */
    int value_1c4;
    int value_1c8;
    int value_1cc;
    int value_1d0;
    W8NpcState* dialogue_npc;
    /* 0x1d8: the active transcript category filter, a W8DialogueCategory
       value; the screen reset writes -1 (all). 0x1ec: the transcript
       "Sort Alphabetically" toggle, reset to 0. */
    signed char dialogue_category_filter;
    unsigned char flag_1d9;
    unsigned char unknown_1da[2];
    /* 0x1dc: the running NPC-dialogue transcript - the records that
       0x00575070 clears, 0x005750D0/0x00575290 load and save, and
       0x0055E840 replays. */
    W8GrowableVector<W8DialogueTranscriptRecord*> dialogue_transcript;
    unsigned char transcript_sorted;
    /* 0x1ed: the item a pending NPC notice carries; the queued-notice block
       at 0x0068EE60 copies it here when the dialogue opens. */
    W8ItemInstance pending_item_1ed;
    unsigned char flag_1f9;
    unsigned char script_busy; /* 0x1fa: set 0xff during script execution */
    unsigned char unknown_1fb;
    /* 0x1fc: the aux_data argument the NPC-dialog dispatch stashes when the
       request is a 0x12/0x1e price check; the reply handler runs it as the
       accepted script line, tells it as a fact, or runs its kind-0x17 decline
       entries. */
    int pending_fact_1fc;
    unsigned char flag_200;
    unsigned char flag_201;
    unsigned char unknown_202[2];
    /* 0x204: the haggled price the NPC dialogue's price-check popup displays
       and the submit path acts on. */
    int pending_price_204;
    int quote_bubble;
    short quote_x;
    short quote_y;
    short quote_width;
    short quote_height;
    bool quote_visible;
    unsigned char unknown_215[3];
    W8GrowableVector<W8PendingNoticeLine*> pending_notice_lines; /* 0x218 */
    unsigned char dialogue_cursor_flag;                          /* 0x228 */
    unsigned char flag_229;
    unsigned char unknown_22a[2];
    int value_22c;
    /* 0x230: g_settings_6850c8.main_ui_mode saved while the NPC dialogue is
       suppressed and handed back to ApplyMainGameModeFlag when it reopens. */
    W8MainUiMode saved_mode_230;
    unsigned char flag_234;
    unsigned char unknown_235[3];
    int value_238;
    unsigned char flag_23c;
    unsigned char flag_23d;
    unsigned char unknown_23e[2];
    /* 0x240/0x244: the camera pitch and yaw saved while the dialogue opens so
       its close can restore them. */
    float saved_camera_pitch_240;
    float saved_camera_yaw_244;
    unsigned char quote_notice_kind;
    unsigned char unknown_249[3];
    void* quote_notice_payload;
    unsigned char flag_250;
    unsigned char flag_251;
    unsigned char flag_252;
    unsigned char unknown_253;
    int value_254;
    /* 0x258: the screen reset writes -1 here, the no-selection value. */
    int value_258;
    int value_25c;
    /* 0x260: raised by the screen reset. */
    unsigned char flag_260;
    unsigned char flag_261;
    bool dialogue_panel_hidden; /* 0x262 */
    unsigned char unknown_263;
    int last_notice_npc_kind; /* 0x264 */
};
#pragma pack(pop)
static_assert(sizeof(W8MainScreenState) == 0x268, "W8MainScreenState_size");
static_assert(offsetof(W8MainScreenState, dialogue_scroll_up_button) == 0x134,
              "W8MainScreenState_dialogue_scroll_up_button");
static_assert(offsetof(W8MainScreenState, dialogue_scroll_down_button) == 0x138,
              "W8MainScreenState_dialogue_scroll_down_button");
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
void Function560A70(W8DialogBase* dialog); /* 0x00560A70 */

void PauseMainGameWorld(void);
void ResumeMainGameWorld(void);
void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress);
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress,
                          int arg); /* 0x0056C5E0 */
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
struct W8NpcQuoteEntry;

void Function563890(void); /* 0x00563890 */
void SyncDialogueNpcState00577260(void);
/* Store a transcript keyword, inferring its category when category is -1. */
void AddDialogueTranscriptKeyword(const wchar_t* name, signed char category);
bool IsDialoguePlaceKeyword(const wchar_t* name);
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette); /* 0x00576030 */
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette, unsigned char notice_kind,
                              void* payload, int npc_kind); /* 0x00576060 */
void DrawNpcQuoteBubble(void);                              /* 0x00576670 */
/* 0x00575E60: OpenNpcDialog — quote entry and dialog request share the
   packed 0x12-byte layout at the call site in ProcessNpcQuoteEntry. */
void LookAtDialogueNpc(void);        /* 0x005767F0 */
void CloseNpcDialogueIfActive(void); /* 0x00576B80 */
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                              int force);   /* 0x0056C6D0 */
void BeginScriptedWorldAction(void);        /* 0x00577520 */
void Function570A20(void);                  /* 0x00570A20 */
void OpenNpcDialogueTranscriptLayout(void); /* 0x00570CF0 */
void DispatchPendingNpcScriptNotice(void);  /* 0x0056CA90 */
bool CanOpenNpcDialogue(void);
bool IsNpcDialogueTextBoxActive577830(void);         /* 0x00577830 */
bool IsNpcDialogueTextBoxActive(void);               /* 0x0056EFD0 */
unsigned char SetNpcDialoguePanelVisible(int value); /* 0x00577880 */
unsigned char ProcessPendingEvent00577A40(void);
void __fastcall
CollapseNpcDialogueTextArea(W8NpcDialogueTextController* controller);               /* 0x0055E2C0 */
void __fastcall ExpandNpcDialogueTextArea(W8NpcDialogueTextController* controller); /* 0x0055E1E0 */
void __fastcall
ClearNpcDialogueTextBackground(W8NpcDialogueTextController* controller);            /* 0x0055EAE0 */
bool __fastcall IsNpcDialogueTextExpanded(W8NpcDialogueTextController* controller); /* 0x0055E2B0 */
/* Which party portrait the pointer is over, if any. */
unsigned int HitTestPartyPortrait(const InputAtom* event);
void ClearCombatSelection(void);                                               /* 0x0056A5A0 */
void UpdateWorldViewCursor0056A5D0(const InputAtom* event, int target_needed); /* 0x0056A5D0 */
void RequestRefreshPartyState(void);
void RefreshFlaggedMainGameState00593330(void);
bool IsScreenIdle(void);
bool IsModalOpen(void);

void RequestRedraw(unsigned int mask);
void ApplyMainGameRedrawFlags(void);           /* 0x00562E40 */
void DrawMainGameScreen(void);                 /* 0x00562A80 */
void SetTooltipSubject(int kind, int subject); /* 0x00569C60 */
int IsScreenInputBlocked(void);
void DisableCombatRegions(void);
void SyncDialogueNpcStateAndMarkPending00577220(void);
void ClearMainGameTargetState(void);

extern unsigned short g_value_006840be;
extern unsigned char g_flag_00685071;
extern W8ItemInstance* g_value_00685072;
extern unsigned char g_flag_00685076;
extern signed char g_value_00685077;
extern unsigned char g_flag_006840bc;
extern unsigned char g_flag_00685070;

void HandleManualCameraHotkeys(void);
void ApplyWorldRenderHotkeys(void);
extern unsigned char g_flag_0068edbc;
extern unsigned char g_flag_68f0f9; /* 0x0068F0F9: a script notice is staged in
                                       g_pending_notice_68ee60 */
extern unsigned char g_flag_0068edc8;
extern unsigned char g_flag_0068edc9;
extern unsigned char g_flag_0068edd8;
extern unsigned char g_flag_0068edd9;
extern unsigned char g_flag_0068edda;
extern int g_main_game_mode_0068eddc;
extern int g_value_64c1c8;
int GetValue64C1C8(void); /* 0x00593320 */
void RequestLevelTransition005615F0(int level, int entry, unsigned char flag);
extern unsigned char g_build_level_links_0065bd2c;
extern int g_next_link_level_0068ede8;
extern unsigned char g_debug_monster_cycle_0068f0fc;
extern bool g_navigator_position_changed_659c11;
extern unsigned char g_flag_006840bb;
void BeginLevelTransition(void); /* 0x005611A0 */
void SetViewportMode(int mode);  /* 0x005618F0 */
/* Apply a main-game UI mode (0=portraits, 1=formation, 2=radar): drop raised
   panels, optionally re-raise them from settings prefs, refresh tooltip and
   region state, and sync both settings and the level-block mode field. */
void ApplyMainGameModeFlag(W8MainUiMode mode, char enable); /* 0x00562580 */
unsigned char ProcessMainGameInput(void);                   /* 0x005684E0 */
void TickAmbientFollowUpIdle(unsigned char input_handled);  /* 0x00561330 */
/* 0x00561EC0: re-sync the eight party slots' region sets and portrait hit
   regions with occupancy, the monster-entry flag and the display mode; the
   party add/remove entries and the keyboard menu's close run it. */
void RefreshPartySlotRegions(void);
/* 0x00561FD0: re-sync mouse hotspot, region enables and viewport after a
   main-game mode change. */
void SyncMainGameModeRegions(void);
void ClearHighlightOverlayRegion(void); /* 0x00563DD0 */
void DismissHighlightOverlay(void);     /* 0x00563EB0 */
/* 0x00563FC0: the portrait-hover panel's producer; the four hover entry
   points hand it the slot, a content row count and a minimum plate width. */
void DrawHighlightOverlay(unsigned int party_slot, int row_count, unsigned int min_width);
void SelectPartyCharacter(int party_slot);    /* 0x00565740 */
void Function56EE20(int party_slot);          /* 0x0056EE20 */
void Function587A30(void);                    /* 0x00587A30 */
void EnableLockInteractionPanels(void);       /* 0x00587C20 */
void Function58A860(void);                    /* 0x0058A860 */
void EnableTrapInteractionPanelRegions(void); /* 0x0058A880 */
void OpenAutomapScreen(void);                 /* 0x00561480 */
/* Clear one slot's pending portrait refresh while the screen is not in
   portrait mode, and disable that slot's portrait region set. */
void ClearPortraitRefreshSlot(int slot); /* 0x00561DB0 */
void ClearScreenWait(void);              /* 0x00565970 */
/* Portrait condition / enchantment orbs (help 25 / 26): hold opens the
   mode-6 hover overlay; leave and release tear it down. */
unsigned char PortraitConditionOrbRegionEvent(const InputAtom* event,
                                              struct W8Region* region); /* 0x005667A0 */
unsigned char PortraitEnchantmentOrbRegionEvent(const InputAtom* event,
                                                struct W8Region* region); /* 0x00566AE0 */
/* Party portrait hit regions: select, target, open camp, and drag-hover. */
unsigned char PortraitSelectRegionEvent(const InputAtom* event,
                                        struct W8Region* region); /* 0x00565990 */
/* Help 28: portrait side bar that opens Assay on the hovered weapon/item. */
unsigned char PortraitAssaySidebarRegionEvent(const InputAtom* event,
                                              struct W8Region* region); /* 0x00566E20 */
/* Help 24: portrait overlay hover strip; drives portrait_overlay_party_slot. */
unsigned char PortraitOverlayHoverRegionEvent(const InputAtom* event,
                                              struct W8Region* region); /* 0x005670C0 */
/* Region set 4: the eight combat-action hit regions beside the portraits. */
unsigned char PartyCombatActionRegionEvent(const InputAtom* event,
                                           struct W8Region* region); /* 0x005673B0 */
/* Help 31: radar-map button beside the text area. */
unsigned char RadarMapButtonRegionEvent(const InputAtom* event,
                                        struct W8Region* region); /* 0x00567600 */
/* Region 23: the 3D world view. Hover refreshes the combat selection/target,
   left-up runs the targeting/item/monster dispatch, right-down opens monster
   info or the assay dialog, and the mouselook latch arms and releases here. */
unsigned char WorldViewRegionEvent(const InputAtom* event,
                                   struct W8Region* region); /* 0x00567800 */
/* Help 36: combat monster-list hit rows beside the radar map. */
unsigned char MonsterListRegionEvent(const InputAtom* event,
                                     struct W8Region* region); /* 0x00568100 */
/* Forward mouse events to W8MainScreenState control slots indexed by
   callback_id from dialogue_text_10c (ids 1..37, 39; id 0x27 is ignored). */
unsigned char MainScreenControlRegionEvent(const InputAtom* event,
                                           struct W8Region* region); /* 0x0056F020 */
void Function568390(int value);                                      /* 0x00568390 */
void SetFormationBoardVisible(unsigned char visible);                /* 0x00569390 */
void ToggleMainGamePause(void);                                      /* 0x0056ABE0 */
/* The numbered action-key space IsMGSActionKeyEnabled, RunMGSActionKey and
   TryMGSActionKey share: the interface commands map to views and recorded
   actions, the combat commands map to ChooseAction selections, and
   W8_MGS_ACTION_REPEAT re-dispatches the slot's queued action. */
enum W8MGSAction {
    W8_MGS_ACTION_JOURNAL = 0,
    W8_MGS_ACTION_USE_ITEM_VIEW = 1,
    W8_MGS_ACTION_USE_RECORDED_ITEM = 2,
    W8_MGS_ACTION_SPELL_VIEW = 3,
    W8_MGS_ACTION_CAST_RECORDED_SPELL = 4,
    W8_MGS_ACTION_BREATHE = 5,
    W8_MGS_ACTION_BREATH_ATTACK = 6,
    W8_MGS_ACTION_ATTACK = 7,
    W8_MGS_ACTION_BERSERK = 8,
    W8_MGS_ACTION_TURN_UNDEAD = 9,
    W8_MGS_ACTION_PRAY = 0xa,
    W8_MGS_ACTION_DEFEND = 0xb,
    W8_MGS_ACTION_PROTECT = 0xc,
    W8_MGS_ACTION_EQUIP = 0xd,
    W8_MGS_ACTION_WALK = 0xe,
    W8_MGS_ACTION_RUN = 0xf,
    W8_MGS_ACTION_REPEAT = 0x10
};

void TryMGSActionKey(int command); /* 0x0056B4C0 */
/* The action-key command gate and executor the dispatcher's 0x131..0x141
   cases and TryMGSActionKey share. */
unsigned char IsMGSActionKeyEnabled(short command);                      /* 0x0056AF80 */
void RunMGSActionKey(short command);                                     /* 0x0056B270 */
void LoadMainGameCursorResources(void);                                  /* 0x00568E10 */
short GetMainGameViewportMode(void);                                     /* 0x005698C0 */
void CloseMainGameOverlays(void);                                        /* 0x00569570 */
void SetRadarMapVisible(unsigned char visible);                          /* 0x00568EB0 */
void SetActionPanelVisible(unsigned char visible);                       /* 0x00569120 */
void OpenCharacterScreenForPartySlot(unsigned int party_slot, int flag); /* 0x00560E10 */
void SwitchNpcDialogueLayout(int interact_id);                           /* 0x00570120 */
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                      int force); /* 0x0056CA60 */
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item,
                                   unsigned char force);                            /* 0x0056CAD0 */
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags);                          /* 0x0056D030 */
void CreateNpcDialogueControls(void);                                               /* 0x0056D1D0 */
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect);                 /* 0x0056ECD0 */
void SetNpcDialogueLayoutMode(int value);                                           /* 0x0056EDD0 */
void CloseNpcDialogueMode1Layout(void);                                             /* 0x00573DD0 */
void CloseNpcDialogueTranscriptLayout(void);                                        /* 0x00571370 */
void CloseNpcDialogueOptionLayout(void);                                            /* 0x00572320 */
void CloseNpcDialogueMode5Layout(void);                                             /* 0x00573570 */
void ShowNpcDialogueTopicMenu(void);                                                /* 0x00570760 */
void HandleNpcDialogueDeparture(int value);                                         /* 0x00577290 */
unsigned char HandleNpcDialogueItem(W8ItemInstance* item);                          /* 0x00575810 */
void TranslateDialogueKeyword0056C440(const wchar_t* source, wchar_t* destination); /* 0x0056C440 */
void ResetNpcDialogueItemEditor(void);                                              /* 0x0056FED0 */
void SetNpcDialogueHidden(char value);                                              /* 0x00576850 */
/* While NPC script deferral holds character events, drain Escape / click so
   the open dialogue layout can dismiss without the normal input path. */
void DrainNpcDialogueDeferralInput(void); /* 0x00575C50 */
/* When value_2435 is set, discard queued input after a mouse-position hook so
   the world-cursor gate does not process stale events. */
void FlushInputWhileWorldCursorGate(void);                     /* 0x00577560 */
void HandleNpcDialogueReply(wchar_t* text, char echo);         /* 0x00574250 */
void HandleNpcDialogueInput(void);                             /* 0x005743B0 */
void OpenNpcDialog(W8NpcDialogRequest* request, int aux_data); /* 0x00575E60 */
void OnNpcDialogClosed(W8DialogBase* dialog);                  /* 0x00576E20 */
void Function5ADB10(int value);                                /* 0x005ADB10 */
void Function58BA60(void);                                     /* 0x0058BA60 */
void Function575710(void);                                     /* 0x00575710 */
/* 0x00571660: learn one keyword into the dialogue transcript. category -1
   auto-classifies the text against items, NPC/named-monster names and the
   place-name table; a nonzero play_chime rings the keyword chime. */
void AddNpcDialogueKeyword(wchar_t* text, signed char category, int play_chime);
void ClearNpcDialogueTranscript(void);  /* 0x00575070 */
void CloseNpcDialogueForCamp(void);     /* 0x00577020 */
void OpenNpcDialogueOptionLayout(void); /* 0x00571AA0 */
void OpenNpcDialogueMode1Layout(void);  /* 0x00573AE0 */
void OpenNpcDialogueMode5Layout(void);  /* 0x005732A0 */
void UpdateNpcDialogueSubMode(void);    /* 0x00571F60 */
/* 0x00575390: restate the five transcript category buttons so only the
   active dialogue_category_filter's button shows its secondary state. */
void SyncDialogueCategoryButtons(void);
void Function56E800(int);
/* Whether an open NPC dialogue transcript covers the party slot's portrait:
   dialogue mode up, flag_252 clear, the controller enabled, and its top
   edge above the slot's band. Portrait and character-update paths skip the
   covered rows through this. */
unsigned char IsPortraitObscuredByNpcDialogue(unsigned int party_slot); /* 0x0056EC90 */
void RecordLevelEntryDialogueState(void);
void Function587510(int value);
void Function5879A0(int);
void Function58A470(int value);
void UpdateMainGameScreen(void); /* 0x0058A750 */
void Function58A790(int);
int GetPartySlotSkill10Level(int slot);
int OpenLockInteraction00587510(Trigger* trigger);
int OpenTrapInteraction0058A470(Trigger* trigger);
/* 0x0056A770: when a slot's committed action cannot execute, re-choose a
   fallback hand, breath or character attack, or reroute spell/item aiming. */
void FallbackFromUnreachableAction(int party_slot);
void SetCombatAction(int value);    /* 0x0056A480 */
void SetCombatSelection(int value); /* 0x00569F70 */
void SetCombatTarget(int value);    /* 0x0056A2D0 */

void RequestRedrawCombatBar(void);             /* 0x005699B0 */
void UpdateScreenOverlays(int frame);          /* 0x0056AF20 */
void DisableMainRegionSet(void);               /* 0x00561FB0 */
void EnableMainRegionSet(void);                /* 0x00561FA0 */
unsigned char IsNpcDialogueCursorActive(void); /* 0x0056EFB0 */
/* 0x0056EFF0: forward a portrait pick into an active NPC dialogue. Unresolved
   gap body; declared for PortraitSelectRegionEvent. */
void Function56EFF0(int party_slot);
void OpenUseItemSelectView(int slot); /* 0x0059C930 */
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

unsigned char CombatBarRegionEvent(const InputAtom* event);
unsigned char DialogueTranscriptRegionEvent(const InputAtom* event, struct W8Region* region);
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event);
void SetDialogueFieldKeyword(wchar_t* keyword, unsigned char append);
