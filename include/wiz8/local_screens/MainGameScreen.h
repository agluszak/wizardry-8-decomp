#pragma once

class Trigger;
class srClass;

#include "input.h"
#include "wiz8/vector.h"

struct W8IList;
struct W8NpcState;

void RequestRedrawParty(void);
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
extern W8MainGameResourceSlot g_main_game_resource_slots[17];

#include "wiz8/screen_state.h"

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"

#include <cstddef>

/* Local Screens\MainGameScreen.cpp owns the live level-screen state. */

#pragma pack(push, 1)
struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf0];
    unsigned char flag_0f0; /* 0x0f0 */
    unsigned char unknown_0f1[3];
    unsigned int redraw_flags; /* 0x0f4 */
    unsigned char unknown_0f8[4];
    int value_0fc;                    /* 0x0fc */
    int camera_mode_100;              /* 0x100 */
    unsigned int hover_region;        /* 0x104 */
    unsigned char flag_108;           /* 0x108 */
    unsigned char party_bytes_109[8]; /* 0x109 */
    unsigned char unknown_111[3];
    int values_114[8]; /* 0x114 */
    int values_134[8]; /* 0x134 */
    unsigned char pick_changed_154;
    unsigned char flag_155;
    unsigned char flag_156;
    unsigned char flag_157;
    unsigned char unknown_158[0x14];
    int highlight_override;    /* 0x16c */
    int values_170[8];         /* 0x170 */
    int held_item_display_190; /* 0x190 */
    int values_194[5];         /* 0x194 */
    int text_lines[12];        /* 0x1a8 */
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    unsigned char dialogue_open;
    unsigned char unknown_1f9[3];
    /* The live dialogue's object (an srClass derivative with state at +0x160,
       past the srModelInstance base). Recovered code only clears this slot
       and the three teardown slots below; the filling producers are
       unrecovered, so the concrete class stays unresolved. */
    srClass* dialogue_owner;
    int values_200[4];      /* 0x200 */
    unsigned char flag_210; /* 0x210 */
    unsigned char unknown_211[3];
    unsigned int clock_214; /* 0x214 */
    unsigned char flag_218; /* 0x218 */
    unsigned char unknown_219[0x23];
    int value_23c;                   /* 0x23c */
    int value_240;                   /* 0x240 */
    unsigned int world_update_flags; /* 0x244 */
    unsigned int world_render_flags; /* 0x248 */
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
    srClass* unknown_2a0;
    srClass* unknown_2a4;
    srClass* unknown_2a8;
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
    unsigned char flag_325;
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
static_assert(offsetof(W8LevelRuntimeBlock, party_bytes_109) == 0x109,
              "W8LevelRuntimeBlock_party_bytes_109");
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

/* 0x00587CF0 constructs this concrete key handler.  Its primary vtable is the
   W8Widget table extended by one entry: slot 0x48 points at
   0x00588170 and accepts the key code forwarded by TextBoxHandleKey. */
// VTABLE: WIZ8 0x005eeafc
class W8MainGameTextKeyHandler : public W8Widget, public W8RangeListener {
public:
    virtual char HandleKey(unsigned short key);
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8RangeControl m_range_038;
    int m_field_0a4;
    int m_field_0a8;
    int m_field_0ac;
    int m_field_0b0;
    int m_field_0b4;
    int m_field_0b8;
    W8RangeListener* m_range_listener_0bc;
};
static_assert(sizeof(W8MainGameTextKeyHandler) == 0xc0, "W8MainGameTextKeyHandler_size");

/* The text panel's constructor at 0x005884D0 begins with Controls::Controls.
   The two secondary bases are installed at 0x4c and 0x50, before its own
   fields. */
// VTABLE: WIZ8 0x005eeba8
class W8MainGameTextPanel : public Controls,
                            public W8TextControl::Listener,
                            public W8RangeListener {
public:
    W8MainGameTextPanel();          /* 0x005884D0 */
    virtual ~W8MainGameTextPanel(); /* 0x00588770 */
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnRangeChanged(W8RangeControl* control) override;

    W8TextControl* m_entries_054[8];
    W8MainGameTextKeyHandler* m_key_handler_074;
    int m_selection_078;
    W8MainGameScreen* m_screen_07c;
    int* m_values_080;
    unsigned char m_flag_084;
    unsigned char m_unknown_085[0xf];
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

/* The 0x00588A90 constructor establishes a Controls-derived status panel. */
// VTABLE: WIZ8 0x005eebc0
class W8MainGameStatusPanel005EEBC0 : public Controls {
public:
    W8MainGameStatusPanel005EEBC0();          /* 0x00588A90 */
    virtual ~W8MainGameStatusPanel005EEBC0(); /* 0x00588D90 */
    virtual void Redraw() override;

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

/* 0x005eebdc is the construction-phase primary table installed at the start
   of 0x00589160; 0x005eebd8 is the complete-object table. The primary is a
   4-byte table whose only recovered slot is a pure destructor. No independent
   base constructor, destructor, or source identity names a distinct authored
   type for that first vptr. The constructor and destructor bodies themselves
   are unrecovered, so this empty primary is retained as the ABI prefix rather
   than collapsed on the vptr observation alone. W8TextControl::Listener is the
   proven secondary base at +0x04: collapsing it into Listener-only inheritance
   would move Listener to +0 and shrink the object. */
// VTABLE: WIZ8 0x005eebdc
class W8MainGameScreenBase005EEBDC {
public:
    virtual ~W8MainGameScreenBase005EEBDC() = 0;
};

// VTABLE: WIZ8 0x005eebd8
class W8MainGameScreen : public W8MainGameScreenBase005EEBDC, public W8TextControl::Listener {
public:
    W8MainGameScreen(void* owner);        /* 0x00589160 */
    virtual ~W8MainGameScreen() override; /* 0x005894B0 */
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}

    void* m_owner_008;
    W8MainGameTextPanel* m_text_panel_00c;
    W8MainGameStatusPanel005EEBC0* m_status_panel_010;
    Controls* m_action_panel_014;
    int m_state_018;
    unsigned char m_unknown_01c[4];
    W8TextControl* m_action_controls_020[5];
    int m_field_034;
    int m_field_038;
    unsigned char m_unknown_03c[0x110];
    int m_target_14c;
    int m_field_150;
    W8GameTimer m_timer_154;
};
static_assert(sizeof(W8MainGameScreen) == 0x178, "W8MainGameScreen_size");

extern W8LevelRuntimeBlock* g_level_block;
extern W8MainGameScreen* g_main_game_screen;

class W8DialogBase;
extern W8DialogBase* g_modal_owner_0068edd0;
extern W8DialogBase* g_pending_main_game_dialog_0068edd4;

#pragma pack(push, 1)
struct W8MainScreenState {
    unsigned char unknown_000[0xf8];
    int target_location_id_f8;
    unsigned char unknown_0fc[8];
    int value_104;
    unsigned char unknown_108[0xcc];
    int value_1d4;
    /* 0x1d8 and 0x1ec: two bytes the screen reset writes 0xff and 0. */
    unsigned char flag_1d8;
    unsigned char unknown_1d9[0x13];
    unsigned char flag_1ec;
    unsigned char unknown_1ed[0x47];
    unsigned char flag_234;
    unsigned char unknown_235[3];
    int value_238;
    unsigned char unknown_23c[0x16];
    unsigned char flag_252;
    unsigned char unknown_253[5];
    /* 0x258: the screen reset writes -1 here, the no-selection value. */
    int value_258;
    unsigned char unknown_25c[4];
    /* 0x260: raised by the screen reset. */
    unsigned char flag_260;
    unsigned char unknown_261[7];
};
#pragma pack(pop)
static_assert(sizeof(W8MainScreenState) == 0x268, "W8MainScreenState_size");

extern W8MainScreenState* g_screen_state_00649f1c;
void OnQuitGameDialogClosed(W8DialogBase* dialog);

void PauseMainGameWorld(void);
void ResumeMainGameWorld(void);
void ForwardNpcScriptNotice(W8NpcState* npc, int value, int line, int suppress);
void Function56C5E0(W8NpcState* npc, int value, int line, int suppress, int arg); /* 0x0056C5E0 */
void ResetMainGameScreenState(void);
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

void Function577260(void);
unsigned char Function577850(void);
unsigned char Function577A40(void);
/* Which party portrait the pointer is over, if any. */
unsigned int HitTestPartyPortrait(const InputAtom* event);
void RequestRefreshPartyState(void);
void Function593330(void);
int IsScreenIdle(void);
bool IsModalOpen(void);

void RequestRedraw(unsigned int mask);
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
extern int g_flag_006850ce;
extern unsigned char g_flag_0068edbc;
extern unsigned char g_flag_0068edc8;
extern unsigned char g_flag_0068edc9;
extern unsigned char g_flag_0068edd8;
extern int g_main_game_mode_0068eddc;
extern unsigned char g_build_level_links_0065bd2c;
extern int g_next_link_level_0068ede8;
extern unsigned char g_flag_0068edd9;
extern unsigned char g_debug_monster_cycle_0068f0fc;
extern W8IList* g_debug_monster_ids_0068f100;
extern unsigned char g_navigator_position_changed_659c11;
extern unsigned char g_flag_006840bb;
extern unsigned char g_map_loading_00659757;

void BeginLevelTransition(void); /* 0x005611A0 */
void Function5618F0(unsigned short mode);
/* Apply a change to the main-game mode flag at 0x006850CE. */
void ApplyMainGameModeFlag(int previous_mode, char enable); /* 0x00562580 */
/* 0x00561EC0: the region-mode pass the party-add entry runs while the
   main-game screen is current. */
void Function561EC0(void);
void Function563DD0(void);
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
void Function58A790(int);
void Function595600(void);
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
