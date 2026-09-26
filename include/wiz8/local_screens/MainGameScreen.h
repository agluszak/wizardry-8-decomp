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
struct W8NpcQuoteEntry;
struct W8NpcState;
struct W8NpcScriptQuote;
struct W8ScreenRect;

void RequestRedrawParty(void);
void RedrawCombatMonsterList(void); /* 0x00565440 */
void RefreshSelectedPartyPortrait(unsigned int party_slot);
void ClearHighlightIfItIs(const int* item);

#include "wiz8/layouts/main_game_screen.h"
/* MainGameScreen.cpp GLOBAL at 0x006068E4: the "%s" display format. */
extern const wchar_t g_format_s[];
/* MainGameScreen.cpp GLOBAL at 0x0064BAB0: the "%d%%" display format. */
extern const wchar_t g_format_d_percent[];
/* MainGameScreen.cpp GLOBAL at 0x0061C3E0: the "%s: %s" display format. */
extern const wchar_t g_format_s_colon_s[];
/* MainGameScreen.cpp GLOBAL at 0x006481B4: the "%s: %s (%d)" display format. */
extern const wchar_t g_format_s_colon_s_paren_d[];
/* MainGameScreen.cpp GLOBAL at 0x0064DA8C: the " %s : " display format. */
extern const wchar_t g_format_s_spaced_colon[];
/* MainGameScreen.cpp GLOBAL at 0x0061A700: the "%s (%d)" display format. */
extern const wchar_t g_format_s_paren_d[];

extern W8MainGameResourceSlot g_main_game_resource_slots[17];
extern W8ScreenRect g_viewport_modes[];
/* String-list ids naming each trap row; Traps.cpp indexes it with the
   trigger's trap type for the disarm/spring notices. */
extern unsigned short g_value_0061e9ec[];
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
                             int line_count, const unsigned short* line_string_ids,
                             unsigned int* region_set);
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
    const unsigned short* m_line_string_ids_0ac;
    int m_selected_line_0b0;
    int m_hover_line_0b4;
    int m_first_visible_line_0b8;
    W8RangeListener* m_range_listener_0bc;
};
static_assert(sizeof(W8MainGameTextKeyHandler) == 0xc0, "W8MainGameTextKeyHandler_size");
/* The secondary W8RangeListener subobject sits at +0x34. */
W8_ASSERT_BASE_END(W8MainGameTextKeyHandler, W8RangeListener, m_range_038, 0x34);

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
    bool m_input_blocked_bc;
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
    /* 0x84: the timed progress text block is displayed. */
    unsigned char m_progress_display_084;
    unsigned char m_pad_085[3];
    float m_progress_duration_088;
    float m_progress_elapsed_08c;
    int m_progress_drawn_090; /* widest progress extent drawn so far */
    W8GameTimer m_timer_094;
    W8ControlsRect m_text_bounds_0b8;
    W8TextBuffer m_text_buffer_0c8;
    W8GameTimer m_timer_118;
    int m_marker_anim_time_13c; /* drives the 12-frame target-changed marker */
    bool m_target_changed_140;
    /* 0x141: draw the animated target-changed marker this frame;
       raised while m_fDirty, consumed by Redraw. */
    bool m_target_marker_pending_141;
    unsigned char m_pad_142[2];
};
static_assert(sizeof(W8MainGameTextPanel) == 0x144, "W8MainGameTextPanel_size");
/* Retail secondary vftable 0x005eeba0 places W8TextControl::Listener at +0x4c
   and W8RangeListener at +0x50; the panel's own members begin at +0x54. */
W8_ASSERT_BASE_END(W8MainGameTextPanel, W8RangeListener, m_entries_054, 0x50);

/* The 0x00588A90 constructor establishes a Controls-derived status panel.
   Ordinary destructor 0x00588DB0; 0x00588D90 is the scalar deleting wrapper. */
// VTABLE: WIZ8 0x005eebc0
class W8MainGameStatusPanel : public Controls {
public:
    W8MainGameStatusPanel();          /* 0x00588A90 */
    virtual ~W8MainGameStatusPanel(); /* 0x00588DB0 */
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
static_assert(sizeof(W8MainGameStatusPanel) == 0x6c, "W8MainGameStatusPanel_size");

/* 0x0055DE40 constructs this Controls-derived NPC dialogue text controller:
   Controls base, six dwords, then the W8DialogTextArea at +0x64 for a 0xBC
   total. W8NpcInteractionState stores the live instance at +0x1b0. */
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
    /* Open the transcript upward to fit its lines (capped at 0xff pixels)
       and enable its scroll region. */
    void Expand(); /* 0x0055E1E0 */
    /* Shrink the transcript back to one line, clearing and redrawing the
       area it covered. */
    void Collapse(); /* 0x0055E2C0 */
    /* Clear and redraw the backdrop the transcript covers. */
    void ClearBackground(); /* 0x0055EAE0 */
    bool IsExpanded();      /* 0x0055E2B0 */

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
   (constructed by 0x0055E570, stored in W8NpcInteractionState at +0x130, parented
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
   per-pin pattern byte copied from Trigger::lock_state.device_state.pins; +0x40 is the
   pin's current pixel offset, 0x22 at rest. */
// VTABLE: WIZ8 0x005eea60
class W8LockTumbler : public W8Widget {
public:
    /* Retail ICF folds this deleting destructor onto
       W8NpcDialogueScrollWidget's retained body at 0x0055E5B0; there is no
       distinct retail emission to mark. */

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
   g_lock_pin_target_height, the fall timer returns it to rest. */
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
    int m_phase_78;                            /* 0x78: sway accumulator feeding g_lock_phase */
    W8GameTimer m_phase_timer_7c;              /* 0x7c: 0.04s */
    W8GameTimer m_rise_timer_a0;               /* 0xa0: 0.03s */
    W8GameTimer m_fall_timer_c4;               /* 0xc4: 0.01s */
    W8LockTumblerPanelListener* m_listener_e8; /* 0xe8 */
};
static_assert(sizeof(W8LockTumblerPanel) == 0xec, "W8LockTumblerPanel_size");
/* The secondary W8LockTumblerListener subobject sits at +0x4c (secondary
   vftable 0x005eeaa8). */
W8_ASSERT_BASE_END(W8LockTumblerPanel, W8LockTumblerListener, m_tumbler_count_50, 0x4c);
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
    /* Knock-knock resolution: rolls the per-level chance over a shuffled pin
       order, raising (or on backfire dropping) them, then re-derives the
       control enables and enters state 7. The flag parameter is unused. */
    void ApplyKnockKnock(int level, int flag, char backfire); /* 0x005871A0 */

    Trigger* m_trigger_08;  /* 0x08 */
    int m_tumbler_count_0c; /* 0x0c: trigger->lock_state.difficulty clamped to [2,8] */
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
/* Retail secondary vftable 0x005eead0 places W8TextControl::Listener at +0x4. */
W8_ASSERT_BASE_END(W8LockInteraction, W8TextControl::Listener, m_trigger_08, 0x4);
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
class W8MainGameTextSelectionListener {
public:
    virtual void SelectTextEntry(int index) = 0;
};

// VTABLE: WIZ8 0x005eebd8
// VTABLE: WIZ8 0x005eebd0 W8TextControl::Listener
class W8MainGameScreen : public W8MainGameTextSelectionListener, public W8TextControl::Listener {
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
    W8MainGameStatusPanel* m_status_panel_010;
    Controls* m_action_panel_014;
    int m_disarm_state_018;
    int m_selected_character_01c;
    W8TextControl* m_action_controls_020[5];
    int m_device_id_034;
    int m_difficulty_038;
    unsigned char m_column_filled_03c[8];
    unsigned char m_slot_attempted_044[8];
    int m_slot_columns_04c[8][8];
    int m_target_difficulty_14c;
    int m_sound_handle_150;
    W8GameTimer m_timer_154;
};
static_assert(sizeof(W8MainGameScreen) == 0x178, "W8MainGameScreen_size");
/* Retail secondary vftable 0x005eebd0 places W8TextControl::Listener at +0x4. */
W8_ASSERT_BASE_END(W8MainGameScreen, W8TextControl::Listener, m_owner_008, 0x4);

extern W8LevelRuntimeBlock* g_level_block;
extern W8MainGameScreen* g_main_game_screen;

/* Insanity (spell 0x3c) world-cursor extent rows and the per spell-power
   index into them; CastSpellFromSource scans the same extents when it
   places the insanity point. */
extern double g_world_cursor_extent_table[18];
extern signed char g_spell_power_extent_index[8];

class W8DialogBase;
struct W8ItemInstance;

extern W8DialogBase* g_modal_owner;
void OpenModal(W8DialogBase* owner);
extern W8DialogBase* g_pending_main_game_dialog;

/* Open the assay (item info) dialog for an item, evaluated against the party
   slot's character; -1 means no character. The current modal owner, if any,
   moves to the pending slot. */
void OpenMonsterInfoDialog(int location_id);                    /* 0x0056AD60 */
void OpenAssayDialog(W8ItemInstance* item, int character_slot); /* 0x0056AE20 */

void OnQuitGameDialogClosed(W8DialogBase* dialog);
void OnLeaveGameConfirmClosed(W8DialogBase* dialog); /* 0x00560A70 */

void PauseMainGameWorld(void);
void ResumeMainGameWorld(void);
void ResetMainGameScreenState(void);

struct W8NpcScriptQuote;
struct W8NpcQuoteEntry;

void RefreshTrackedPortraitOverlay(void); /* 0x00563890 */
/* Which party portrait the pointer is over, if any. */
unsigned char HitTestPartyPortrait(const InputAtom* event);
void ClearCombatSelection(void);                                       /* 0x0056A5A0 */
void UpdateWorldViewCursor(const InputAtom* event, int target_needed); /* 0x0056A5D0 */
void RequestRefreshPartyState(void);
void RefreshFlaggedMainGameState(void);
bool IsScreenIdle(void);
bool IsModalOpen(void);

void RequestRedraw(unsigned int mask);
void ApplyMainGameRedrawFlags(void);           /* 0x00562E40 */
void DrawMainGameScreen(void);                 /* 0x00562A80 */
void SetTooltipSubject(int kind, int subject); /* 0x00569C60 */
int IsScreenInputBlocked(void);
void DisableCombatRegions(void);

extern unsigned char g_radar_panel_shown;
extern unsigned char g_action_panel_shown;
extern unsigned char g_formation_panel_shown;
extern bool g_mouselook_active;
extern bool g_mouselook_left_held;
extern bool g_node_cull_pending;
extern int g_main_game_mode;
extern int g_selected_party_slot;
int GetValue64C1C8(void); /* 0x00593320 */
void RequestLevelTransition(int level, int entry, unsigned char flag);
extern bool g_build_level_links;
extern int g_next_link_level;
extern bool g_navigator_position_changed;
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
void DrawPortraitVitalsOverlay(int party_slot);      /* 0x00564710 */
void DrawPortraitConditionOverlay(int party_slot);   /* 0x00564BA0 */
void DrawPortraitStatusOverlay(int party_slot);      /* 0x00564D80 */
void DrawPortraitEnchantmentOverlay(int party_slot); /* 0x005651F0 */
void SelectPartyCharacter(int party_slot);           /* 0x00565740 */
void RefreshLockInteractionControls(void);           /* 0x00587A30 */
void EnableLockInteractionPanels(void);              /* 0x00587C20 */
void RefreshMainGameActionPanel(void);               /* 0x0058A860 */
void EnableTrapInteractionPanelRegions(void);        /* 0x0058A880 */
void OpenAutomapScreen(void);                        /* 0x00561480 */
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
void SetMainGameMode(int mode);                                /* 0x00568390 */
void SetFormationBoardVisible(unsigned char visible);          /* 0x00569390 */
void ToggleMainGamePause(void);                                /* 0x0056ABE0 */
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
bool IsMGSActionKeyEnabled(short command);                               /* 0x0056AF80 */
void RunMGSActionKey(short command);                                     /* 0x0056B270 */
void LoadMainGameCursorResources(void);                                  /* 0x00568E10 */
short GetMainGameViewportMode(void);                                     /* 0x005698C0 */
void CloseMainGameOverlays(void);                                        /* 0x00569570 */
void SetRadarMapVisible(unsigned char visible);                          /* 0x00568EB0 */
void SetActionPanelVisible(unsigned char visible);                       /* 0x00569120 */
void OpenCharacterScreenForPartySlot(unsigned int party_slot, int flag); /* 0x00560E10 */
void RebuildNpcTradeItemList(bool scroll_to_top);
/* 0x005ADAA0: the trade-stock index behind a visible NPC item row. */
int ResolveNpcTradeStockIndex(int index);
/* 0x005AD950: append one NPC stock item's name and price lines to the trade
   text box. */
void ShowNpcTradeItemNotice(W8ItemInstance* item);
/* 0x005ADBE0: refill the trade text box from the pending item pool or the NPC
   stock, then re-enable the filter buttons. */
void PopulateNpcTradeList(void);
/* 0x005AE1F0: validate the pending trade selection; queues a refusal quote and
   fails when the NPC declines the item or the party cannot pay. */
bool ValidateNpcTradeSelection(void);
/* 0x005AE2A0: run one NPC trade offer; the result selects the accepted,
   refused or offended script path. */
bool AttemptNpcItemTrade(W8ItemInstance* item, unsigned char quantity, int index);
/* 0x005AE1A0: destroy callback on the NPC trade split dialog; commits the
   chosen count to the editor slot and refreshes the trade selection. */
void NpcTradeSplitDialogResult(W8DialogBase* dialog);
void RefreshFormationPanel(unsigned char show_portraits); /* 0x005B2980 */
void EndLockInteractMode(char suspend);                   /* 0x005879A0 */
void UpdateMainGameScreen(void);                          /* 0x0058A750 */
void EndTrapInteractMode(char suspend);                   /* 0x0058A790 */
int GetPartySlotSkill10Level(int slot);
int OpenLockInteraction(Trigger* trigger);
int OpenTrapInteraction(Trigger* trigger);
/* 0x0056A770: when a slot's committed action cannot execute, re-choose a
   fallback hand, breath or character attack, or reroute spell/item aiming. */
void FallbackFromUnreachableAction(int party_slot);
void SetCombatAction(int value);    /* 0x0056A480 */
void SetCombatSelection(int value); /* 0x00569F70 */
void SetCombatTarget(int value);    /* 0x0056A2D0 */

void RequestRedrawCombatBar(void);             /* 0x005699B0 */
void UpdateScreenOverlays(int frame);          /* 0x0056AF20 */
bool LoadCurrentLevelData(void);               /* 0x00560A20 */
void ResetMainGameMode(void);                  /* 0x00560C60 */
void CreateSurpriseFade(void);                 /* 0x0056B4E0 */
void ReverseSurpriseFade(void);                /* 0x0056B5F0 */
void DestroySurpriseFade(void);                /* 0x0056B690 */
unsigned char UpdateSurpriseFade(void);        /* 0x0056B6F0 */
void DisableMainRegionSet(void);               /* 0x00561FB0 */
void EnableMainRegionSet(void);                /* 0x00561FA0 */
unsigned char OpenUseItemSelectView(int slot); /* 0x0059C930 */

unsigned char MainGameScreenInitialize(void);
unsigned char MainGameScreenEnter(void);
void MainGameScreenFrame(void);
unsigned char MainGameScreenLeave(int leaving);
void ShowMainGameNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, int confirmation,
                            int cancel); /* 0x00569A50 */

unsigned char CombatBarRegionEvent(const InputAtom* event, struct W8Region* region);
unsigned char DialogueTranscriptRegionEvent(const InputAtom* event, struct W8Region* region);
void SetNpcDialogueSubMode4(void);
void ConfirmNpcTradeItem(void);
void RestockNpcTradeStock(void);
void OpenNpcTradeQuantityDialog(void);
