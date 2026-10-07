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
void RedrawCombatMonsterList(void);
void RefreshSelectedPartyPortrait(unsigned int party_slot);
void ClearHighlightIfItIs(const int* item);

#include "wiz8/layouts/main_game_screen.h"
extern wchar_t g_format_s[];
extern wchar_t g_format_d_percent[];
extern wchar_t g_format_s_colon_s[];
extern wchar_t g_format_s_colon_s_paren_d[];
extern wchar_t g_format_s_spaced_colon[];
extern wchar_t g_format_s_paren_d[];

extern W8MainGameResourceSlot g_main_game_resource_slots[17];
extern W8ScreenRect g_viewport_modes[];
extern const float g_float_one_fiftieth;
extern const float g_float_half_turn_degrees;
/* String-list ids naming each trap row; Traps.cpp indexes it with the
   trigger's trap type for the disarm/spring notices. */
extern unsigned short g_trap_name_string_ids[];
#include "wiz8/layouts/screen_state.h"

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/dialog_code/DialogTextArea.h"

#include <cstddef>

class W8MainGameScreen;
class W8MainGameTextPanel;

// VTABLE: WIZ8 0x005eeafc
class W8MainGameTextKeyHandler : public W8Widget, public W8RangeListener {
public:
    W8MainGameTextKeyHandler(Controls* panel, int left, int top, int right, int bottom,
                             int line_count, const unsigned short* line_string_ids,
                             unsigned int* region_set);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;
    virtual void AdjustValue(int steps) override;
    virtual void OnLeftButtonUp(int event) override;
    void SetSelectedLine(int line);
    virtual char HandleKey(unsigned short key);
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8RangeControl m_range;
    int m_line_count;
    int m_visible_lines;
    const unsigned short* m_line_string_ids;
    int m_selected_line;
    int m_hover_line;
    int m_first_visible_line;
    W8RangeListener* m_range_listener;
};
static_assert(sizeof(W8MainGameTextKeyHandler) == 0xc0, "W8MainGameTextKeyHandler_size");

W8_ASSERT_BASE_END(W8MainGameTextKeyHandler, W8RangeListener, m_range, 0x34);

/* One text-panel cell: the catalog image it shows (or -1) and a flag that
   gates mouse handling. */
// VTABLE: WIZ8 0x005eeb4c
class W8MainGameTextEntry : public W8TextControl {
public:
    W8MainGameTextEntry(Controls* panel, int index);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;

    int m_image;
    bool m_input_blocked;
};
static_assert(sizeof(W8MainGameTextEntry) == 0xc0, "W8MainGameTextEntry_size");

// VTABLE: WIZ8 0x005eeba8
// VTABLE: WIZ8 0x005eeba0 W8TextControl::Listener
class W8MainGameTextPanel : public Controls,
                            public W8TextControl::Listener,
                            public W8RangeListener {
public:
    void BeginProgress(const wchar_t* text, float duration, float hold);

    W8MainGameTextPanel();
    virtual ~W8MainGameTextPanel();
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8MainGameTextEntry* m_entries[8];
    W8MainGameTextKeyHandler* m_key_handler;
    int m_selection;
    W8MainGameScreen* m_screen;
    int* m_values;
    bool m_progress_display; /* the timed progress text block is displayed */
    float m_progress_duration;
    float m_progress_elapsed;
    int m_progress_drawn; /* widest progress extent drawn so far */
    W8GameTimer m_timer0;
    W8ControlsRect m_text_bounds;
    W8TextBuffer m_text_buffer;
    W8GameTimer m_timer1;
    int m_marker_anim_time; /* drives the 12-frame target-changed marker */
    bool m_target_changed;
    /* Draw the animated target-changed marker this frame; raised while
       m_fDirty, consumed by Redraw. */
    bool m_target_marker_pending;
};
static_assert(sizeof(W8MainGameTextPanel) == 0x144, "W8MainGameTextPanel_size");

W8_ASSERT_BASE_END(W8MainGameTextPanel, W8RangeListener, m_entries, 0x50);

// VTABLE: WIZ8 0x005eebc0
class W8MainGameStatusPanel : public Controls {
public:
    W8MainGameStatusPanel();
    virtual ~W8MainGameStatusPanel();
    virtual void Redraw() override;
    void RefreshStatusTexts();

    W8TextBuffer* m_text;
    W8TextBuffer* m_text0;
    W8TextBuffer* m_text1;
    W8TextBuffer* m_text2;
    W8TextBuffer* m_text3;
    W8TextBuffer* m_text4;
    W8TextBuffer* m_text5;
    int m_target;
};
static_assert(sizeof(W8MainGameStatusPanel) == 0x6c, "W8MainGameStatusPanel_size");

/* The NPC dialogue keyword transcript. */
// VTABLE: WIZ8 0x005ee920
class W8NpcDialogueTextController : public Controls {
public:
    W8NpcDialogueTextController(int panel_left, int panel_top, int panel_right, int panel_bottom,
                                int catalog_object, int catalog_frame, int catalog_image,
                                int margin_image, int line_image);
    virtual void Redraw() override;
    bool HandleScrollDownCommand(bool check_only);
    bool HandleScrollUpCommand(bool check_only);
    /* Add one keyword line to the transcript unless the text is already
       present; a nonzero mark puts the new entry in state 0x60, and any
       leftover "[No Keywords]" placeholder is removed afterwards. */
    unsigned char AddTranscriptEntry(const wchar_t* text, signed char category, char mark);
    /* Whether the expanded transcript's top edge reaches above the portrait
       band for an odd party slot (1 -> 0x67, 3 -> 0xbc, 5 -> 0x111, 7 ->
       always covered). Callers use it to skip portrait work on rows the
       open transcript covers. */
    bool IsSlotPortraitTranscriptCovered(unsigned int party_slot);
    /* Re-apply the category filter, rebuild the expansion and restate the
       scroll widgets. */
    void SetTranscriptCategoryFilter(signed char category);
    /* Replay every saved dialogue_transcript record into the text area,
       adding the "[No Keywords]" placeholder and forcing the all filter
       when nothing was saved. */
    void RestoreTranscriptEntries();
    /* Snapshot the transcript lines into the screen state's
       dialogue_transcript records (text plus category byte). */
    void SaveTranscriptEntries();
    /* Drop every transcript line from the text area and invalidate. */
    void ClearTranscriptEntries();
    int GetSelectedTranscriptEntryIndex();
    void SetTranscriptSorted(bool sorted);
    void RemoveSelectedTranscriptEntry();
    /* Open the transcript upward to fit its lines (capped at 0xff pixels)
       and enable its scroll region. */
    void Expand();
    /* Shrink the transcript back to one line, clearing and redrawing the
       area it covered. */
    void Collapse();
    /* Clear and redraw the backdrop the transcript covers. */
    void ClearBackground();
    bool IsExpanded();

    /* Catalog image ids whose measured heights seed margin and
       line_height/scroll_height in the constructor; never read again. */
    int margin_image;
    int line_image;
    int visible;
    int line_height;
    int margin;
    int scroll_height;
    W8DialogTextArea text_area;
    void SelectTranscriptKeywordAtPoint(int x, int y);
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

/* The hover/click target over the dialogue transcript. Entering selects the
   first visible entry, leaving clears the selection, and the button handlers
   track the press in m_flags before firing the widget callbacks. */
// VTABLE: WIZ8 0x005ee92c
class W8NpcDialogueScrollWidget : public W8Widget {
public:
    W8NpcDialogueScrollWidget(Controls* panel, unsigned int region, int left, int top, int right,
                              int bottom);

    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;

    unsigned int m_flags; /* bit 0 while the primary button is held */
};
static_assert(sizeof(W8NpcDialogueScrollWidget) == 0x38, "W8NpcDialogueScrollWidget_size");
static_assert(offsetof(W8NpcDialogueScrollWidget, m_flags) == 0x34,
              "W8NpcDialogueScrollWidget_flags_34");

class W8LockTumbler;

/* Notified by a W8LockTumbler when it is released. */
// VTABLE: WIZ8 0x005eeabc
class W8LockTumblerListener {
public:
    virtual void OnTumblerReleased(W8LockTumbler* tumbler) = 0;
};

/* One lock-picking pin. */
// VTABLE: WIZ8 0x005eea60
class W8LockTumbler : public W8Widget {
public:
    W8LockTumbler(Controls* panel, int left, int top, int right, int bottom, int pin_index)
        : W8Widget(panel, 0xffffffff, left, top, right, bottom), m_pin_set(0), m_rising(0),
          m_falling(0), m_at_top(0), m_hovered(0), m_pin_index(pin_index), m_pin_height(0x22),
          m_listener(0)
    {
    }
    virtual void Redraw(bool full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;

    bool m_pin_set; /* raised and holding */
    bool m_rising;  /* animating toward its target height */
    bool m_falling; /* dropping back to rest */
    bool m_at_top;  /* redrawn against the shared tumble phase */
    bool m_hovered;
    int m_pin_index;  /* column into the pin pattern tables */
    int m_pin_height; /* pixel offset, 0x22 at rest */
    W8LockTumblerListener* m_listener;
};
static_assert(sizeof(W8LockTumbler) == 0x48, "W8LockTumbler_size");

/* Notified by W8LockTumblerPanel with the index of a released tumbler. */
// VTABLE: WIZ8 0x005eeae0
class W8LockTumblerPanelListener {
public:
    virtual void OnTumblerPicked(int index) = 0;
};

/* The tumbler strip of the lock interaction. Owns the eight tumblers and the
   three animation timers: the phase timer steps the shared sway, the rise
   timer moves a pin toward g_lock_pin_target_height, the fall timer returns it
   to rest. */
// VTABLE: WIZ8 0x005eeaac
class W8LockTumblerPanel : public Controls, public W8LockTumblerListener {
public:
    W8LockTumblerPanel(int tumbler_count, const unsigned char* pin_data);

    virtual ~W8LockTumblerPanel();
    virtual void OnTumblerReleased(W8LockTumbler* tumbler) override;
    void UpdateTumblerAnimation();

    int m_tumbler_count; /* pins in use, clamped to [2,8] */
    W8LockTumbler* m_tumblers[8];
    bool m_animating;          /* a pin is in flight; input is locked out */
    int m_phase;               /* sway accumulator feeding g_lock_phase */
    W8GameTimer m_phase_timer; /* 0.04s */
    W8GameTimer m_rise_timer;  /* 0.03s */
    W8GameTimer m_fall_timer;  /* 0.01s */
    W8LockTumblerPanelListener* m_listener;
};
static_assert(sizeof(W8LockTumblerPanel) == 0xec, "W8LockTumblerPanel_size");

W8_ASSERT_BASE_END(W8LockTumblerPanel, W8LockTumblerListener, m_tumbler_count, 0x4c);
static_assert(offsetof(W8LockTumblerPanel, m_tumblers) == 0x54, "W8LockTumblerPanel_tumblers");
static_assert(offsetof(W8LockTumblerPanel, m_listener) == 0xe8, "W8LockTumblerPanel_listener");

/* The lock interaction's readout column: the selected character's name plus
   the lockpick-skill, spell-power and force-chance percentages. */
// VTABLE: WIZ8 0x005eeac0
class W8LockInfoPanel : public Controls {
public:
    W8LockInfoPanel(int tumbler_count);

    virtual ~W8LockInfoPanel();
    virtual void Redraw() override;
    void RefreshInfo();

    int m_tumbler_count;
    W8TextBuffer* m_text0; /* character name */
    W8TextBuffer* m_text1;
    W8TextBuffer* m_text2; /* lockpick skill */
    W8TextBuffer* m_text3;
    W8TextBuffer* m_text4; /* spell power */
    W8TextBuffer* m_text5;
    W8TextBuffer* m_text; /* force chance */
};
static_assert(sizeof(W8LockInfoPanel) == 0x6c, "W8LockInfoPanel_size");

/* The lock-picking interaction. It answers the tumbler panel's "released
   tumbler N" callback and receives the action-panel buttons; Process() is the
   per-frame state machine ProcessLockInteractMode drives. */
// VTABLE: WIZ8 0x005eead8 W8LockTumblerPanelListener
// VTABLE: WIZ8 0x005eead0 W8TextControl::Listener
class W8LockInteraction : public W8LockTumblerPanelListener, public W8TextControl::Listener {
public:
    W8LockInteraction(Trigger* trigger);

    virtual ~W8LockInteraction();
    virtual void OnTumblerPicked(int index) override;
    virtual void OnPrimary(W8TextControl* control) override;
    int ReleaseOwnedTumblers(int slot);
    void Process();
    void ResolvePick();
    void AttemptForce();
    void EnablePanels(int enable);
    void BeginUnlock();
    /* Knock-knock resolution: rolls the per-level chance over a shuffled pin
       order, raising (or on backfire dropping) them, then re-derives the
       control enables and enters state 7. The flag parameter is unused. */
    void ApplyKnockKnock(int level, int flag, char backfire);

    Trigger* m_trigger;
    int m_tumbler_count; /* trigger->lock_state.difficulty clamped to [2,8] */
    W8LockTumblerPanel* m_tumbler_panel;
    W8LockInfoPanel* m_info_panel;
    Controls* m_action_panel;
    W8TextControl* m_done_button;      /* OnPrimary target, state 9 */
    W8TextControl* m_spell_button;     /* gated by spell-0x27 power */
    W8TextControl* m_force_button;     /* gated by the force chance */
    W8TextControl* m_cancel_button;    /* OnPrimary target, state 5 (cancel) */
    int m_selected_slot;               /* party slot owning the raised pins */
    int m_picked_tumbler;              /* index OnTumblerPicked recorded */
    int m_state;                       /* Process() state */
    int m_tumbler_owner[8];            /* owning party slot per pin, -1 unset */
    unsigned char m_tumbler_locked[8]; /* pin kept when the slot is released */
    int m_slot_attempts[8];            /* pick attempts per party slot */
    W8GameTimer m_timer;               /* state-8 completion delay */
};
static_assert(sizeof(W8LockInteraction) == 0xa4, "W8LockInteraction_size");
W8_ASSERT_BASE_END(W8LockInteraction, W8TextControl::Listener, m_trigger, 0x4);
static_assert(offsetof(W8LockInteraction, m_trigger) == 0x08, "W8LockInteraction_trigger");
static_assert(offsetof(W8LockInteraction, m_tumbler_panel) == 0x10,
              "W8LockInteraction_tumbler_panel");
static_assert(offsetof(W8LockInteraction, m_state) == 0x34, "W8LockInteraction_state");
static_assert(offsetof(W8LockInteraction, m_timer) == 0x80, "W8LockInteraction_timer");

// VTABLE: WIZ8 0x005eebdc
class W8MainGameTextSelectionListener {
public:
    virtual void SelectTextEntry(int index) = 0;
};

// VTABLE: WIZ8 0x005eebd8 W8MainGameTextSelectionListener
// VTABLE: WIZ8 0x005eebd0 W8TextControl::Listener
class W8MainGameScreen : public W8MainGameTextSelectionListener, public W8TextControl::Listener {
public:
    W8MainGameScreen(Trigger* owner);
    ~W8MainGameScreen();
    virtual void SelectTextEntry(int index) override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    void Update();
    void RefreshActionPanel();
    void EnablePanelRegionSets(bool enable);
    int GetTrapInteractionChance() const;
    void ApplyInspectSuccess();
    void CastTrapSpell();
    void UseTrapItem();

    Trigger* m_owner;
    W8MainGameTextPanel* m_text_panel;
    W8MainGameStatusPanel* m_status_panel;
    Controls* m_action_panel;
    int m_disarm_state;
    int m_selected_character;
    W8TextControl* m_action_controls[5];
    int m_device_id;
    int m_difficulty;
    unsigned char m_column_filled[8];
    unsigned char m_slot_attempted[8];
    int m_slot_columns[8][8];
    int m_target_difficulty;
    int m_sound_handle;
    W8GameTimer m_timer;
};
static_assert(sizeof(W8MainGameScreen) == 0x178, "W8MainGameScreen_size");
W8_ASSERT_BASE_END(W8MainGameScreen, W8TextControl::Listener, m_owner, 0x4);

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
void OpenMonsterInfoDialog(int location_id);
void OpenAssayDialog(W8ItemInstance* item, int character_slot);

void OnQuitGameDialogClosed(W8DialogBase* dialog);
void OnLeaveGameConfirmClosed(W8DialogBase* dialog);

void PauseMainGameWorld(void);
void ResumeMainGameWorld(void);
void ResetMainGameScreenState(void);

struct W8NpcScriptQuote;
struct W8NpcQuoteEntry;

void RefreshTrackedPortraitOverlay(void);
/* Which party portrait the pointer is over, if any. */
unsigned char HitTestPartyPortrait(const InputAtom* event);
void ClearCombatSelection(void);
void UpdateWorldViewCursor(const InputAtom* event, W8TargetNeed target_needed);
void RequestRefreshPartyState(void);
void RefreshFlaggedMainGameState(void);
bool IsScreenIdle(void);
bool IsModalOpen(void);

void RequestRedraw(unsigned int mask);
void ApplyMainGameRedrawFlags(void);
void DrawMainGameScreen(void);
void CancelMouselook();
void SetTooltipSubject(int kind, int subject);
int IsScreenInputBlocked(void);
void DisableCombatRegions(void);

extern bool g_radar_panel_shown;
extern bool g_action_panel_shown;
extern bool g_formation_panel_shown;
extern bool g_mouselook_active;
extern bool g_mouselook_left_held;
extern bool g_node_cull_pending;
enum W8MainGameMode {
    W8_MAIN_GAME_DEFAULT = 0,
    W8_MAIN_GAME_NPC_DIALOGUE = 3,
    W8_MAIN_GAME_PORTRAIT_REFRESH = 4,
    W8_MAIN_GAME_MODAL = 5,
    W8_MAIN_GAME_HIGHLIGHT_OVERLAY = 6
};

extern W8MainGameMode g_main_game_mode;
extern int g_selected_party_slot;
int GetSelectedPartySlot(void);
void RequestLevelTransition(int level, int entry, unsigned char flag);
extern bool g_build_level_links;
extern int g_next_link_level;
extern bool g_navigator_position_changed;
void BeginLevelTransition(void);
void SetViewportMode(int mode);
/* Apply a main-game UI mode (0=portraits, 1=formation, 2=radar): drop raised
   panels, optionally re-raise them from settings prefs, refresh tooltip and
   region state, and sync both settings and the level-block mode field. */
void ApplyMainGameModeFlag(W8MainUiMode mode, bool enable);
unsigned char ProcessMainGameInput(void);
void TickAmbientFollowUpIdle(unsigned char input_handled);
/* Re-sync the eight party slots' region sets and portrait hit
   regions with occupancy, the monster-entry flag and the display mode; the
   party add/remove entries and the keyboard menu's close run it. */
void RefreshPartySlotRegions(void);
/* Re-sync mouse hotspot, region enables and viewport after a
   main-game mode change. */
void SyncMainGameModeRegions(void);
void ClearHighlightOverlayRegion(void);
void DismissHighlightOverlay(void);
/* The portrait-hover panel's producer; the four hover entry
   points hand it the slot, a content row count and a minimum plate width. */
void DrawHighlightOverlay(unsigned int party_slot, int row_count, unsigned int min_width);
void DrawPortraitVitalsOverlay(int party_slot);
void DrawPortraitConditionOverlay(int party_slot);
void DrawPortraitStatusOverlay(int party_slot);
void DrawPortraitEnchantmentOverlay(int party_slot);
void SelectPartyCharacter(int party_slot);
void RefreshLockInteractionControls(void);
void EnableLockInteractionPanels(void);
void RefreshMainGameActionPanel(void);
void EnableTrapInteractionPanelRegions(void);
void OpenAutomapScreen(void);
/* Clear one slot's pending portrait refresh while the screen is not in
   portrait mode, and disable that slot's portrait region set. */
void ClearPortraitRefreshSlot(int slot);
/* Clear whatever the screen was waiting on and open the options screen. */
// FUNCTION: WIZ8 0x00565970
inline void ClearScreenWait(void)
{
    g_pending_screen_state.mode = 0;
    SetPendingScreenState(W8_SCREEN_OPTIONS);
}
/* Portrait condition / enchantment orbs (help 25 / 26): hold opens the
   mode-6 hover overlay; leave and release tear it down. */
unsigned char PortraitConditionOrbRegionEvent(const InputAtom* event, struct W8Region* region);
unsigned char PortraitEnchantmentOrbRegionEvent(const InputAtom* event, struct W8Region* region);
/* Party portrait hit regions: select, target, open camp, and drag-hover. */
unsigned char PortraitSelectRegionEvent(const InputAtom* event, struct W8Region* region);
/* Help 28: portrait side bar that opens Assay on the hovered weapon/item. */
unsigned char PortraitAssaySidebarRegionEvent(const InputAtom* event, struct W8Region* region);
/* Help 24: portrait overlay hover strip; drives portrait_overlay_party_slot. */
unsigned char PortraitOverlayHoverRegionEvent(const InputAtom* event, struct W8Region* region);
/* Region set 4: the eight combat-action hit regions beside the portraits. */
unsigned char PartyCombatActionRegionEvent(const InputAtom* event, struct W8Region* region);
/* Help 31: radar-map button beside the text area. */
unsigned char RadarMapButtonRegionEvent(const InputAtom* event, struct W8Region* region);
/* Region 23: the 3D world view. Hover refreshes the combat selection/target,
   left-up runs the targeting/item/monster dispatch, right-down opens monster
   info or the assay dialog, and the mouselook latch arms and releases here. */
unsigned char WorldViewRegionEvent(const InputAtom* event, struct W8Region* region);
/* Help 36: combat monster-list hit rows beside the radar map. */
unsigned char MonsterListRegionEvent(const InputAtom* event, struct W8Region* region);
void SetMainGameMode(W8MainGameMode mode);
void SetFormationBoardVisible(bool visible);
void ToggleMainGamePause(void);
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

void TryMGSActionKey(W8MGSAction command);

bool IsMGSActionKeyEnabled(unsigned short command);
void RunMGSActionKey(unsigned short command);
void LoadMainGameCursorResources(void);
short GetMainGameViewportMode(void);
void CloseMainGameOverlays(void);
/* Overlay entry points share the radar-mode fallback. The caller captures and
   stores its return mode before or after this operation as its own path requires. */
inline void SetMainGameOverlayViewport(W8MainUiMode mode)
{
    if (mode == W8_MAIN_UI_MODE_RADAR) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, false);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
}

void SetRadarMapVisible(bool visible);
void SetActionPanelVisible(bool visible);
void OpenCharacterScreenForPartySlot(unsigned int party_slot, bool flag);
void RebuildNpcTradeItemList(bool scroll_to_top);
/* The trade-stock index behind a visible NPC item row. */
int ResolveNpcTradeStockIndex(int index);
/* Append one NPC stock item's name and price lines to the trade
   text box. */
void ShowNpcTradeItemNotice(W8ItemInstance* item);
/* Refill the trade text box from the pending item pool or the NPC
   stock, then re-enable the filter buttons. */
void PopulateNpcTradeList(void);
/* Validate the pending trade selection; queues a refusal quote and
   fails when the NPC declines the item or the party cannot pay. */
bool ValidateNpcTradeSelection(void);
/* Run one NPC trade offer; the result selects the accepted,
   refused or offended script path. */
bool AttemptNpcItemTrade(W8ItemInstance* item, unsigned char quantity, int index);
/* Destroy callback on the NPC trade split dialog; commits the
   chosen count to the editor slot and refreshes the trade selection. */
void NpcTradeSplitDialogResult(W8DialogBase* dialog);
void RefreshFormationPanel(bool show_portraits);
void EndLockInteractMode(char suspend);
void UpdateMainGameScreen(void);
void EndTrapInteractMode(char suspend);
int GetPartySlotLocksTrapsLevel(int slot);
int OpenLockInteraction(Trigger* trigger);
int OpenTrapInteraction(Trigger* trigger);
/* When a slot's committed action cannot execute, re-choose a
   fallback hand, breath or character attack, or reroute spell/item aiming. */
void FallbackFromUnreachableAction(int party_slot);
void SetCombatAction(int value);
void SetCombatSelection(int value);
void SetCombatTarget(int value);

void RequestRedrawCombatBar(void);
void UpdateScreenOverlays(int frame);
bool LoadCurrentLevelData(void);
void ResetMainGameMode(void);
void CreateSurpriseFade(void);
void ReverseSurpriseFade(void);
void DestroySurpriseFade(void);
unsigned char UpdateSurpriseFade(void);
void DisableMainRegionSet(void);
void EnableMainRegionSet(void);
unsigned char OpenUseItemSelectView(int slot);

unsigned char MainGameScreenInitialize(void);
unsigned char MainGameScreenEnter(void);
void MainGameScreenFrame(void);
unsigned char MainGameScreenLeave(int leaving);
void ShowMainGameNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, bool confirmation,
                            bool cancel);

unsigned char CombatBarRegionEvent(const InputAtom* event, struct W8Region* region);
unsigned char DialogueTranscriptRegionEvent(const InputAtom* event, struct W8Region* region);
void SelectNpcBuyMode(void);
void ConfirmNpcTradeItem(void);
void RestockNpcTradeStock(void);
void OpenNpcTradeQuantityDialog(void);
