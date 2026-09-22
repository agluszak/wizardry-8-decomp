#pragma once

#include "input.h"
#include "wiz8/vector.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/message_box.h"
#include "wiz8/local_code/Controls.h"

/* Local Screens\NPCInteractionSubscreen.cpp owns the NPC dialogue state and
   the keyword/transcript tables. */

class W8DialogBase;
class W8NpcDialogueScrollWidget;
class W8NpcDialogueTextController;
class W8TextControl;
class W8Widget;
struct W8ControlsRect;
struct W8ExperienceNoticePayload;
struct W8NpcQuoteEntry;
struct W8NpcScriptQuote;
struct W8NpcState;
struct W8Region;
struct W8SkillNoticePayload;

/* The 0x50-byte panel stored at W8MainScreenState+0x1a8 (bounds
   0x17,0x166-0xa4,0x1c2); it hosts the six option buttons at +0x170..+0x184.
   Its SetEnabled keeps those six inactive unless the expanded NPC dialogue
   layout (dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) is up, and its Redraw substitutes m_value_4c for
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

    int m_value_4c; /* 0x4c: catalog image used while dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX */
};
static_assert(sizeof(W8MainGamePanel005EE9F0) == 0x50, "W8MainGamePanel005EE9F0_size");

/* The Controls-sized panel stored at W8MainScreenState+0x1c0 (bounds
   0x1dc,0x166-0x269,0x1c0). Enabling it starts text-input scheme 1 and
   installs the typed-dialogue input field; Redraw also draws the input-frame
   image at y 0x19b while where_is_query is raised, else 0x18b. The constructor is the
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

struct W8PendingNoticeLine {
    wchar_t* text;
    int npc_kind;
};

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
extern W8PendingNotice g_pending_notice_68ee60;
extern wchar_t g_wchar_0068ee58[4];

/* W8MainScreenState::dialogue_layout - which NPC dialogue layout is up. The
   layout-1 caption is "MAGIC" (Charm/Mindread/Use Item services) and the
   layout-5 caption "TRADE"; both spellings come from the StringData.DAT
   captions each open routine loads. A mode 6 is closed everywhere but no
   open path is recovered. */
enum W8NpcDialogueLayout {
    W8_DIALOGUE_LAYOUT_NONE = 0,
    W8_DIALOGUE_LAYOUT_SERVICES = 1,
    W8_DIALOGUE_LAYOUT_TOPIC_MENU = 2,
    W8_DIALOGUE_LAYOUT_TRANSCRIPT = 3,
    W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX = 4,
    W8_DIALOGUE_LAYOUT_TRADE = 5
};

/* W8MainScreenState::trade_mode - the active tab of the option/trade
   layouts. The StringData.DAT captions the tabs and headers load are
   "Give", "Sell", "Buy" and "Shoplift": mode 2 lists the party's items
   with the purse as row zero, mode 3 the character/party sell pools, and
   modes 4/5 the NPC's own inventory. 0 while no tab is selected. */
enum W8NpcTradeMode {
    W8_NPC_TRADE_NONE = 0,
    W8_NPC_TRADE_GIVE = 2,
    W8_NPC_TRADE_SELL = 3,
    W8_NPC_TRADE_BUY = 4,
    W8_NPC_TRADE_SHOPLIFT = 5
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

struct W8MainScreenState {
    /* 0x000: a word 0x0056CAD0 clears while the dialogue opens. */
    short value_000;
    unsigned char unknown_002[0xee];
    W8MainUiMode saved_main_ui_mode; /* 0x0f0: the pre-dialogue display mode 0x56cad0 saves */
    /* 0x0f4: the party slot 0x0056D030 picks as the dialogue's leading
       speaker - the occupied row whose character leads skill 0x16. */
    int dialogue_speaker;
    int target_location_id_f8;
    /* 0x0fc: the layout currently up, a W8NpcDialogueLayout value. 0x104: the
       layout the current one replaced; back-out paths reopen it. */
    int dialogue_layout;
    /* 0x100: the active trade tab, a W8NpcTradeMode value. */
    int trade_mode;
    int previous_dialogue_layout;
    W8ItemInstance* trade_item;
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
       dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX. Created as plain W8TextControls (regions 0x75..0x7a) by
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
    /* 0x1c4/0x1c8: the cursor position of the last mouse event the region
       handler acted on; repeat events at the same point are dropped. */
    int last_mouse_x;
    int last_mouse_y;
    /* 0x1cc: the text-box line the mouse/wheel handlers last selected, -1 for
       none. */
    int hovered_text_line;
    /* 0x1d0: the trade-list filter mask the six option buttons toggle: bit 0
       keeps only items the selected character can use, bit 6 items any party
       member can use, and bits 2-5 exclude the equipment slot groups
       NpcTradeItemAllowed00573190 tests. */
    int trade_filter;
    W8NpcState* dialogue_npc;
    /* 0x1d8: the active transcript category filter, a W8DialogueCategory
       value; the screen reset writes -1 (all). 0x1ec: the transcript
       "Sort Alphabetically" toggle, reset to 0. */
    signed char dialogue_category_filter;
    /* 0x1d9: raised while the "Where Is" submit path runs, so the typed text
       is wrapped in the "Where is %s" template instead of a plain keyword
       lookup; the input panel draws its frame one row lower while set. */
    bool where_is_query;
    unsigned char unknown_1da[2];
    /* 0x1dc: the running NPC-dialogue transcript - the records that
       0x00575070 clears, 0x005750D0/0x00575290 load and save, and
       0x0055E840 replays. */
    /* The retail constructor (0x0056B9A0) writes the W8GrowableVector vtable
       0x005EE9D0 and then the derived table 0x005EE9DC here, so this member is
       the thin derived W8Vector, not the base. */
    W8Vector<W8DialogueTranscriptRecord*> dialogue_transcript;
    unsigned char transcript_sorted;
    /* 0x1ed: the item a pending NPC notice carries; the queued-notice block
       at 0x0068EE60 copies it here when the dialogue opens. */
    W8ItemInstance pending_item_1ed;
    /* 0x1f9: pending_item_1ed came in through the cursor item; dialogue close
       returns it to the hand and the consume path drains its stack. */
    bool held_item_pending;
    unsigned char script_busy; /* 0x1fa: set 0xff during script execution */
    unsigned char unknown_1fb;
    /* 0x1fc: the aux_data argument the NPC-dialog dispatch stashes when the
       request is a 0x12/0x1e price check; the reply handler runs it as the
       accepted script line, tells it as a fact, or runs its kind-0x17 decline
       entries. */
    int pending_fact_1fc;
    /* 0x200: a kind-0x12/0x1e price-check modal is awaiting its yes/no reply;
       the reply handler matches the accepted string against pending_price_204.
       0x201: the request was kind 0x1e, so the accept path skips the
       TellNpcFact call 0x12 makes. */
    bool price_check_pending;
    unsigned char price_check_skip_fact;
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
    /* Same derived-table evidence as dialogue_transcript: 0x005EE9D4 then
       0x005EE9D8. */
    W8Vector<W8PendingNoticeLine*> pending_notice_lines; /* 0x218 */
    /* 0x228: SetNpcDialogueHidden stores its argument here: the whole
       dialogue UI is parked (right-click hide) and the world input path is
       live until it is shown again. */
    unsigned char dialogue_hidden;
    /* 0x229: the option layout was reached through the Exit/farewell path, so
       backing out of it reopens the topic menu instead of the transcript. */
    unsigned char reopen_topics;
    unsigned char unknown_22a[2];
    /* 0x22c: the gold the player put on the trade table - the split-amount
       dialog result, spent by ConfirmNpcTradePurchase00575710. */
    int trade_gold;
    /* 0x230: g_settings_6850c8.main_ui_mode saved while the NPC dialogue is
       suppressed and handed back to ApplyMainGameModeFlag when it reopens. */
    W8MainUiMode saved_mode_230;
    /* 0x234: the next UpdateNpcDialogueSubMode must reapply the
       trade_pc_items toggle state; armed when the option layout opens and
       when a re-open is staged through SyncDialogueNpcState*. */
    bool pending_trade_toggle;
    unsigned char unknown_235[3];
    /* 0x238: a layout staged for reopen; the frame update closes the current
       layout and opens this one, then clears it. */
    int pending_layout;
    /* 0x23c: cleared when the dialogue ends on a refusal/abrupt dismissal, in
       which case the close path queues a delayed party reaction event. */
    unsigned char suppress_parting_reaction;
    /* 0x23d: LookAtDialogueNpc aimed the camera at the NPC, so the close
       restores saved_camera_pitch_240. */
    unsigned char camera_redirected;
    unsigned char unknown_23e[2];
    /* 0x240/0x244: the camera pitch and yaw saved while the dialogue opens so
       its close can restore them. */
    float saved_camera_pitch_240;
    float saved_camera_yaw_244;
    unsigned char quote_notice_kind;
    unsigned char unknown_249[3];
    /* 0x24c: notice payload discriminated by quote_notice_kind: 1 takes
       `experience`, 2 `skill_notices`, 3 `level_up_slot`; producers arrive
       through the `raw` member of a SetNpcQuoteBubbleVisible parameter. */
    W8MessageBoxPayload quote_notice_payload;
    /* 0x250: a modal W8NpcDialog is up over the dialogue; transcript word
       clicks, layout keys other than Escape and layout leave paths bail. */
    bool modal_dialog_open;
    unsigned char flag_251;
    /* 0x252: the dialogue session runs as queued script lines without the
       interactive panel; input, portrait and panel paths gate on it. */
    unsigned char scripted_dialogue;
    unsigned char unknown_253;
    int trade_quantity;
    /* 0x258: the screen reset writes -1 here, the no-selection value. */
    int selected_trade_row;
    /* 0x25c: how many times the transcript layout has opened this session;
       the first open (count still below 1) queues the greeting lines. */
    int transcript_open_count;
    /* 0x260: the "PC Items"/"Party Items" pool toggle of the trade layouts:
       raised by the screen reset and SelectNpcTradeMode1 ("PC Items"),
       cleared by SelectNpcTradeMode0 ("Party Items"); it mirrors the button
       pair so UpdateNpcDialogueSubMode can restore it. */
    unsigned char trade_pc_items;
    /* 0x261: the main text box is collapsed to a single line while the
       dialogue is fresh; cleared when the next transcript line scrolls. */
    unsigned char text_box_collapsed;
    bool dialogue_panel_hidden; /* 0x262 */
    unsigned char unknown_263;
    int last_notice_npc_kind; /* 0x264 */
};
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
static_assert(offsetof(W8MainScreenState, dialogue_hidden) == 0x228,
              "W8MainScreenState_dialogue_hidden");
static_assert(offsetof(W8MainScreenState, dialogue_panel_hidden) == 0x262,
              "W8MainScreenState_dialogue_panel_hidden");
static_assert(offsetof(W8MainScreenState, last_notice_npc_kind) == 0x264,
              "W8MainScreenState_last_notice_npc_kind");

extern W8MainScreenState* g_screen_state_00649f1c;

void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress);
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress,
                          int arg);         /* 0x0056C5E0 */
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
/* 0x00575E60: OpenNpcDialog — the modal request is the script's
   W8NpcQuoteEntry itself; the dialog discriminates kind_00 0x05 (option
   list), 0x12/0x1e (price check) and 0x13 (keyword entry). */
void LookAtDialogueNpc(void);        /* 0x005767F0 */
void CloseNpcDialogueIfActive(void); /* 0x00576B80 */
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                              int force);   /* 0x0056C6D0 */
void BeginScriptedWorldAction(void);        /* 0x00577520 */
void CloseNpcDialogueLayout00570A20(void);  /* 0x00570A20 */
void OpenNpcDialogueTranscriptLayout(void); /* 0x00570CF0 */
void DispatchPendingNpcScriptNotice(void);  /* 0x0056CA90 */
bool CanOpenNpcDialogue(void);
bool IsNpcDialogueTextBoxActive577830(void);         /* 0x00577830 */
bool IsNpcDialogueTextBoxActive(void);               /* 0x0056EFD0 */
unsigned char SetNpcDialoguePanelVisible(int value); /* 0x00577880 */
bool ProcessPendingEvent00577A40(void);
void SyncDialogueNpcStateAndMarkPending00577220(void);
void ClearMainGameTargetState(void);
/* 0x0068F0F9: a script notice is staged in g_pending_notice_68ee60 */
extern unsigned char g_flag_68f0f9;
void SyncNpcServiceButtons0056EE20(int party_slot); /* 0x0056EE20 */
/* Forward mouse events to W8MainScreenState control slots indexed by
   callback_id from dialogue_text_10c (ids 1..37, 39; id 0x27 is ignored). */
unsigned char MainScreenControlRegionEvent(const InputAtom* event,
                                           struct W8Region* region); /* 0x0056F020 */
void SwitchNpcDialogueLayout(int interact_id);                       /* 0x00570120 */
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                      int force); /* 0x0056CA60 */
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item,
                                   unsigned char force);            /* 0x0056CAD0 */
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags);          /* 0x0056D030 */
void CreateNpcDialogueControls(void);                               /* 0x0056D1D0 */
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect); /* 0x0056ECD0 */
void SetNpcDialogueLayoutMode(int value);                           /* 0x0056EDD0 */
void CloseNpcDialogueMode1Layout(void);                             /* 0x00573DD0 */
void CloseNpcDialogueTranscriptLayout(void);                        /* 0x00571370 */
void CloseNpcDialogueOptionLayout(void);                            /* 0x00572320 */
void CloseNpcDialogueMode5Layout(void);                             /* 0x00573570 */
void ShowNpcDialogueTopicMenu(void);                                /* 0x00570760 */
void HandleNpcDialogueDeparture(int value);                         /* 0x00577290 */
unsigned char HandleNpcDialogueItem(W8ItemInstance* item);          /* 0x00575810 */
unsigned char AcceptNpcDialogueItem005B1740(W8NpcState* npc, W8ItemInstance* item,
                                            int mode); /* folded at 0x005B1740 */
void TranslateDialogueKeyword0056C440(const wchar_t* source, wchar_t* destination); /* 0x0056C440 */
void ResetNpcDialogueItemEditor(void);                                              /* 0x0056FED0 */
void SetNpcDialogueHidden(char value);                                              /* 0x00576850 */
/* While NPC script deferral holds character events, drain Escape / click so
   the open dialogue layout can dismiss without the normal input path. */
void DrainNpcDialogueDeferralInput(void); /* 0x00575C50 */
/* When value_2435 is set, discard queued input after a mouse-position hook so
   the world-cursor gate does not process stale events. */
void FlushInputWhileWorldCursorGate(void);                  /* 0x00577560 */
void HandleNpcDialogueReply(wchar_t* text, char echo);      /* 0x00574250 */
void HandleNpcDialogueInput(void);                          /* 0x005743B0 */
void OpenNpcDialog(W8NpcQuoteEntry* request, int aux_data); /* 0x00575E60 */
void OnNpcDialogClosed(W8DialogBase* dialog);               /* 0x00576E20 */
void ConfirmNpcTradePurchase00575710(void);                 /* 0x00575710 */
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
void EndNpcDialogueSession0056E800(int);
/* Whether an open NPC dialogue transcript covers the party slot's portrait:
   dialogue mode up, scripted_dialogue clear, the controller enabled, and its top
   edge above the slot's band. Portrait and character-update paths skip the
   covered rows through this. */
unsigned char IsPortraitObscuredByNpcDialogue(unsigned int party_slot); /* 0x0056EC90 */
void RecordLevelEntryDialogueState(void);
unsigned char IsNpcDialogueCursorActive(void); /* 0x0056EFB0 */
/* 0x0056EFF0: forward a portrait pick into an active NPC dialogue. Unresolved
   gap body; declared for PortraitSelectRegionEvent. */
void TryNpcDialoguePickpocket0056EFF0(int party_slot);
void ShortenTextToWidth00577410(wchar_t* output, const wchar_t* text, unsigned int width, int font);
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event);
void SetDialogueFieldKeyword(wchar_t* keyword, unsigned char append);
void ActivateNpcDialoguePanels0056ECF0(unsigned char active); /* 0x0056ECF0 */
bool HasNpcDialogueDirtyPanels0056ED80(void);                 /* 0x0056ED80 */
unsigned char NpcDialogueTextBoxRegionEvent(const InputAtom* event,
                                            W8Region* region);        /* 0x0056F1D0 */
void NpcDialogueTextBoxWheelAt(short x, unsigned short y, char flag); /* 0x0056F490 */
/* True when an NPC quote/portrait session is active: finish voice playback and
   report that the click was consumed. */
unsigned char FinishNpcVoiceIfSessionActive00577A20(void); /* 0x00577A20 */
void ToggleNpcTradeFilter00573660(void);
void ToggleNpcTradeFilter00573730(void);
void ToggleNpcTradeFilter00573800(void);
void ToggleNpcTradeFilter005738D0(void);
void ToggleNpcTradeFilter005739A0(void);
void ToggleNpcTradeFilter00573A10(void);
void BackOutNpcDialogue00570000(void);
void SubmitNpcDialogueInput00575B00(void);
void SubmitNpcDialogueInput00575B40(void);
void SelectNpcDialogueService00570AD0(void);
void SelectNpcDialogueTalk00570B80(void);
void SelectNpcDialogueExit00570C20(void);
void PromptNpcDispositionChange(void);
void OnNpcDispositionPromptClosed(W8DialogBase* dialog);
void EnterNpcServiceLayout(void);
void LeaveNpcDialogueLayout(void);
void EnterNpcTradeOptions(void);
void ShowNpcDialogueNotice(void);
void RequestNpcJoinParty(void);
void ScrollNpcDialogueUp(void);
void ScrollNpcDialogueDown(void);
void SubmitNpcDialogueKeyword(void);
void RefreshNpcDialogueTranscript(void);
void SyncNpcDialogueListFilter(void);
void SelectNpcDialogueCategory1(void);
void SelectNpcDialogueCategory2(void);
void SelectNpcDialogueCategory0(void);
void SelectNpcDialogueCategory3(void);
void SelectNpcDialogueCategoryAll(void);
void SetNpcDialogueSubMode3(void);
void SetNpcDialogueSubMode2(void);
void SetNpcDialogueSubMode5(void);
void SelectNpcTradeMode1(void);
void SelectNpcTradeMode0(void);
void OpenNpcItemAssay(void);
void OnNpcAssayDialogClosed(W8DialogBase* dialog);
void ConfirmNpcTradeSlot(void);
void RequestNpcSpellService3(void);
void RequestNpcSpellService41(void);
void RequestNpcCharacterService(void);
void OnNpcTradeDialogClosed00575520(W8DialogBase* dialog);
void UpdateNpcTradeSelection0056FAC0(int index, int, int);
void OpenNpcTradeSplitDialog00572780(void);
/* The dialogue text-box's button-up/double-click handlers, dispatched from
   NpcDialogueTextBoxRegionEvent. */
void NpcDialogueTextBoxLeftUp0056F530(int x, int y);      /* 0x0056F530 */
void NpcDialogueTextBoxRightUp0056F6B0(int x, int y);     /* 0x0056F6B0 */
void NpcDialogueTextBoxDoubleClick0056F840(int x, int y); /* 0x0056F840 */
/* W8SplitAmountDialog destroy callback installed by OpenNpcTradeSplitDialog00572780. */
void OnNpcTradeSplitDialogDestroy00572870(W8DialogBase* dialog);         /* 0x00572870 */
W8ItemInstance* ResolveNpcTradeRow005729C0(int index, char, char, char); /* 0x005729C0 */
unsigned char NpcTradeItemAllowed00573190(W8ItemInstance* item);         /* 0x00573190 */
void EnableNpcTradeFilterButtons00573630(void);                          /* 0x00573630 */
W8ItemInstance* GetNpcTradeSlotItem00573F80(int index);                  /* 0x00573F80 */
void HandleNpcDialogueKeyEvent00574BB0(const InputAtom* event);          /* 0x00574BB0 */
unsigned char LoadNpcDialogueTranscript005750D0(unsigned int file);      /* 0x005750D0 */
unsigned char SaveNpcDialogueTranscript00575290(unsigned int file);      /* 0x00575290 */
void HandleNpcDialogueItemChoice00575B70(void);                          /* 0x00575B70 */
void RefreshNpcTradePartyGold00575BC0(void);                             /* 0x00575BC0 */
void RefreshNpcTradePrice00575C00(void);                                 /* 0x00575C00 */
void ResolveNpcPickpocket00576BA0(int party_slot);                       /* 0x00576BA0 */
