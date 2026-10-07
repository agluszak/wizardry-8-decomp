#pragma once

void SyncNpcDialogueTranscriptScrollButtons();

#include "input.h"
#include "wiz8/vector.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/layouts/npc_dialogue.h"
#include "wiz8/message_box.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/xstatus.h"

/* Usability filters and the exclusive equipment categories in the trade UI.
   The category bits use the same positions as W8ItemEquipSlotGroup. */
enum W8NpcTradeFilter {
    W8_NPC_TRADE_USABLE_BY_CHARACTER = 0x01,
    W8_NPC_TRADE_HAND = 0x04,
    W8_NPC_TRADE_BODY = 0x08,
    W8_NPC_TRADE_ACCESSORY = 0x10,
    W8_NPC_TRADE_OTHER = 0x20,
    W8_NPC_TRADE_CATEGORY_MASK = 0x3c,
    W8_NPC_TRADE_USABLE_BY_PARTY = 0x40
};

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

/* The panel hosting the six option buttons. SetEnabled keeps them inactive
   unless the main-text-box dialogue layout is up, and Redraw substitutes
   m_main_text_box_image for m_catalogImage in that layout. */
// VTABLE: WIZ8 0x005ee9f0
class W8NpcDialogueOptionsPanel : public Controls {
public:
    W8NpcDialogueOptionsPanel(int left, int top, int new_right, int new_bottom, int catalog_object,
                              int catalog_frame, int catalog_image)
        : Controls(left, top, new_right, new_bottom, catalog_object, catalog_frame, catalog_image)
    {
        m_main_text_box_image = 0x11;
    }
    virtual void SetEnabled(bool enable) override;
    virtual void Redraw() override;

    int m_main_text_box_image;
};
static_assert(sizeof(W8NpcDialogueOptionsPanel) == 0x50, "W8NpcDialogueOptionsPanel_size");

/* The typed-dialogue panel. Enabling it starts text-input scheme 1 and
   installs the input field; Redraw draws the input frame one row lower while
   where_is_query is raised. */
// VTABLE: WIZ8 0x005ee9e4
class W8NpcTypedDialoguePanel : public Controls {
public:
    W8NpcTypedDialoguePanel(int left, int top, int new_right, int new_bottom, int catalog_object,
                            int catalog_frame, int catalog_image)
        : Controls(left, top, new_right, new_bottom, catalog_object, catalog_frame, catalog_image)
    {
    }
    virtual void SetEnabled(bool enable) override;
    virtual void Redraw() override;
};
static_assert(sizeof(W8NpcTypedDialoguePanel) == 0x4c, "W8NpcTypedDialoguePanel_size");

struct W8PendingNoticeLine {
    wchar_t* text;
    int npc_kind;
};

/* An NPC script notice waiting for DispatchPendingNpcScriptNotice. */
struct W8PendingNotice {
    W8NpcState* npc;
    W8ItemInstance item;
    int line;
    bool flag;
    unsigned char force;
};
extern W8PendingNotice g_pending_notice;
extern wchar_t g_dialogue_empty_text[4];

/* W8NpcInteractionState::trade_mode - the active tab of the option/trade
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
   W8NpcInteractionState::dialogue_category_filter and on each
   W8DialogTextEntry::m_category. The five filter buttons spell them
   "Items", "People", "Places", "Misc" and "All". A keyword is classified by
   scanning the item records, the NPC records plus the
   named-monster table and the fixed place-name string table, in that
   order. -1 shows every category. */
enum W8DialogueCategory {
    W8_DIALOGUE_CATEGORY_ALL = -1,
    W8_DIALOGUE_CATEGORY_ITEMS = 0,
    W8_DIALOGUE_CATEGORY_PEOPLE = 1,
    W8_DIALOGUE_CATEGORY_PLACES = 2,
    W8_DIALOGUE_CATEGORY_MISC = 3
};

/* One saved transcript keyword. */
struct W8DialogueTranscriptRecord {
    wchar_t text[100];
    signed char category;
};
static_assert(sizeof(W8DialogueTranscriptRecord) == 0xca, "W8DialogueTranscriptRecord_size");

enum W8NpcDialogueControlSlot {
    W8_NPC_CONTROL_NPC_NAME = 0,
    W8_NPC_CONTROL_TEXT_1 = 1,
    W8_NPC_CONTROL_TEXT_2 = 2,
    W8_NPC_CONTROL_TEXT_3 = 3,
    W8_NPC_CONTROL_TEXT_4 = 4,
    W8_NPC_CONTROL_TEXT_5 = 5,
    W8_NPC_CONTROL_TEXT_6 = 6,
    W8_NPC_CONTROL_TEXT_7 = 7,
    W8_NPC_CONTROL_WIDGET = 8,
    W8_NPC_CONTROL_SCROLL = 9,
    W8_NPC_CONTROL_SCROLL_UP_BUTTON = 10,
    W8_NPC_CONTROL_SCROLL_DOWN_BUTTON = 11,
    W8_NPC_CONTROL_TEXT_12 = 12,
    W8_NPC_CONTROL_TEXT_13 = 13,
    W8_NPC_CONTROL_SORT_BUTTON = 15,
    W8_NPC_CONTROL_PEOPLE_BUTTON = 17,
    W8_NPC_CONTROL_PLACES_BUTTON = 18,
    W8_NPC_CONTROL_ITEMS_BUTTON = 19,
    W8_NPC_CONTROL_MISC_BUTTON = 20,
    W8_NPC_CONTROL_ALL_BUTTON = 21,
    W8_NPC_CONTROL_TEXT_22 = 22,
    W8_NPC_CONTROL_TEXT_23 = 23,
    W8_NPC_CONTROL_TEXT_24 = 24,
    W8_NPC_CONTROL_OPTION_BUTTONS = 25,
    W8_NPC_CONTROL_TEXT_31 = 31,
    W8_NPC_CONTROL_TEXT_33 = 33,
    W8_NPC_CONTROL_TEXT_34 = 34,
    W8_NPC_CONTROL_TEXT_35 = 35,
    W8_NPC_CONTROL_TEXT_36 = 36,
    W8_NPC_CONTROL_TEXT_37 = 37,
    W8_NPC_CONTROL_TEXT_38 = 38,
};

/* Payload ownership on portrait-quote dismissal; retained as a byte. */
typedef unsigned char W8NpcQuoteNoticeKind;

enum {
    W8_QUOTE_NOTICE_TEXT = 0,
    W8_QUOTE_NOTICE_EXPERIENCE = 1,
    W8_QUOTE_NOTICE_SKILL_INCREASE = 2,
    W8_QUOTE_NOTICE_LEVEL_UP = 3
};

struct W8NpcInteractionState {
    short value_000; /* cleared while the dialogue opens */
    unsigned char unknown_002[0xee];
    W8MainUiMode saved_main_ui_mode; /* the pre-dialogue display mode */
    /* The dialogue's leading speaker: the occupied row whose character leads
       skill 0x16. */
    int dialogue_speaker;
    int target_location_id;
    W8NpcDialogueLayout dialogue_layout;
    W8NpcTradeMode trade_mode;
    /* The layout the current one replaced; back-out paths reopen it. */
    W8NpcDialogueLayout previous_dialogue_layout;
    W8ItemInstance* trade_item;
    W8Widget* dialogue_controls[39]; /* indexed by region callback id */
    Controls* dialogue_panels[7];
    /* The cursor position of the last mouse event the region handler acted
       on; repeat events at the same point are dropped. */
    int last_mouse_x;
    int last_mouse_y;
    /* The text-box line the mouse/wheel handlers last selected, -1 for none. */
    int hovered_text_line;
    int trade_filter; /* W8NpcTradeFilter bits */
    W8NpcState* dialogue_npc;
    signed char dialogue_category_filter; /* W8DialogueCategory */
    /* Raised while the "Where Is" submit path runs, so the typed text is
       wrapped in the "Where is %s" template instead of a plain keyword lookup. */
    bool where_is_query;
    W8Vector<W8DialogueTranscriptRecord*> dialogue_transcript;
    bool transcript_sorted;
    /* The item a pending NPC notice carries; copied from g_pending_notice
       when the dialogue opens. */
    W8ItemInstance pending_item;
    /* pending_item came in through the cursor item; dialogue close returns it
       to the hand and the consume path drains its stack. */
    bool held_item_pending;
    unsigned char script_busy; /* set 0xff during script execution */
    /* The aux_data argument the NPC-dialog dispatch stashes when the
       request is a 0x12/0x1e price check; the reply handler runs it as the
       accepted script line, tells it as a fact, or runs its kind-0x17 decline
       entries. */
    int pending_fact;
    /* A kind-0x12/0x1e price-check modal is awaiting its yes/no reply; the
       reply handler matches the accepted string against pending_price. */
    bool price_check_pending;
    /* The request was kind 0x1e, so the accept path skips the TellNpcFact
       call 0x12 makes. */
    bool price_check_skip_fact;
    /* The haggled price the NPC dialogue's price-check popup displays and the
       submit path acts on. */
    int pending_price;
    int quote_bubble;
    short quote_x;
    short quote_y;
    short quote_width;
    short quote_height;
    bool quote_visible;
    W8Vector<W8PendingNoticeLine*> pending_notice_lines;
    /* The whole dialogue UI is parked (right-click hide) and the world input
       path is live until it is shown again. */
    unsigned char dialogue_hidden;
    /* The option layout was reached through the Exit/farewell path, so backing
       out of it reopens the topic menu instead of the transcript. */
    bool reopen_topics;
    int trade_gold; /* gold put on the trade table */
    /* g_settings.main_ui_mode saved while the NPC dialogue is suppressed. */
    W8MainUiMode saved_mode;
    /* The next UpdateNpcDialogueSubMode must reapply the trade_pc_items
       toggle state. */
    bool pending_trade_toggle;
    /* A layout staged for reopen; the frame update closes the current layout
       and opens this one, then clears it. */
    W8NpcDialogueLayout pending_layout;
    /* Cleared when the dialogue ends on a refusal/abrupt dismissal, in which
       case the close path queues a delayed party reaction event. */
    bool suppress_parting_reaction;
    /* LookAtDialogueNpc aimed the camera at the NPC, so the close restores
       the saved camera angles. */
    bool camera_redirected;
    float saved_camera_pitch;
    float saved_camera_yaw;
    W8NpcQuoteNoticeKind quote_notice_kind;
    /* Discriminated by quote_notice_kind: 1 takes `experience`, 2
       `skill_notices`, 3 `level_up_slot`. */
    W8MessageBoxPayload quote_notice_payload;
    /* A modal W8NpcDialog is up over the dialogue; transcript word clicks,
       layout keys other than Escape and layout leave paths bail. */
    bool modal_dialog_open;
    bool farewell_queued; /* a refusal/farewell line was queued for the exit path */
    /* The dialogue session runs as queued script lines without the
       interactive panel. */
    bool scripted_dialogue;
    int trade_quantity;
    int selected_trade_row; /* -1 for none */
    /* How many times the transcript layout has opened this session; the
       first open queues the greeting lines. */
    int transcript_open_count;
    /* The "PC Items"/"Party Items" pool toggle of the trade layouts. */
    bool trade_pc_items;
    /* The main text box is collapsed to a single line while the dialogue is
       fresh; cleared when the next transcript line scrolls. */
    bool text_box_collapsed;
    bool dialogue_panel_hidden;
    int last_notice_npc_kind;
};
static_assert(offsetof(W8NpcInteractionState, dialogue_controls) == 0x10c,
              "W8NpcInteractionState_dialogue_controls");
static_assert(offsetof(W8NpcInteractionState, dialogue_panels) == 0x1a8,
              "W8NpcInteractionState_dialogue_panels");
static_assert(sizeof(W8NpcInteractionState) == 0x268, "W8NpcInteractionState_size");
static_assert(offsetof(W8NpcInteractionState, script_busy) == 0x1fa,
              "W8NpcInteractionState_script_busy");
static_assert(offsetof(W8NpcInteractionState, dialogue_npc) == 0x1d4,
              "W8NpcInteractionState_dialogue_npc");
static_assert(offsetof(W8NpcInteractionState, quote_bubble) == 0x208,
              "W8NpcInteractionState_quote_bubble");
static_assert(offsetof(W8NpcInteractionState, quote_visible) == 0x214,
              "W8NpcInteractionState_quote_visible");
static_assert(offsetof(W8NpcInteractionState, quote_notice_kind) == 0x248,
              "W8NpcInteractionState_quote_notice_kind");
static_assert(offsetof(W8NpcInteractionState, quote_notice_payload) == 0x24c,
              "W8NpcInteractionState_quote_notice_payload");
static_assert(offsetof(W8NpcInteractionState, pending_notice_lines) == 0x218,
              "W8NpcInteractionState_pending_notice_lines");
static_assert(offsetof(W8NpcInteractionState, dialogue_hidden) == 0x228,
              "W8NpcInteractionState_dialogue_hidden");
static_assert(offsetof(W8NpcInteractionState, dialogue_panel_hidden) == 0x262,
              "W8NpcInteractionState_dialogue_panel_hidden");
static_assert(offsetof(W8NpcInteractionState, last_notice_npc_kind) == 0x264,
              "W8NpcInteractionState_last_notice_npc_kind");

extern W8NpcInteractionState* g_npc_interaction_state;

void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, bool suppress);
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, bool suppress,
                          unsigned char arg);
void FlushPendingNoticeLines(void);
/* Zero W8NpcInteractionState, write its reset values, and reload the
   keyword lists through the loader below. */
void ResetMainScreenStateBlock(void);
/* The dialogue keyword tables, one file list per language;
   element zero is English_Keywords.txt and element one the translated list.
   A file list holds one line list per line, and a line list one malloc'd wide
   word per '/'-separated field. */
extern W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*> g_keyword_lists;
/* Both files are loaded and the tables are usable. Raised once the
   second file loads and lowered whenever the tables are released. */
extern bool g_keyword_lists_loaded;
/* Replace the keyword lists with the contents of
   Data\Strings\English_Keywords.txt and Data\Strings\translated_Keywords.txt. */
void ReloadKeywordLists(void);
/* Release every file list, its lines and its words. */
void ClearKeywordLists(void);
/* Load one keyword file into a file list. */
unsigned char LoadKeywordFile(const char* path,
                              W8GrowableVector<W8GrowableVector<wchar_t*>*>* file);
/* Copy the next '/'-terminated field out of a keyword line into
   the caller's buffer and return the cursor past it, or null at the end. */
wchar_t* ParseKeywordToken(wchar_t* line, wchar_t* field);
void SyncDialogueNpcState(void);
/* Store a transcript keyword, inferring its category when category is -1. */
void AddDialogueTranscriptKeyword(const wchar_t* name, signed char category);
bool IsDialoguePlaceKeyword(const wchar_t* name);
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette);
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette,
                              W8NpcQuoteNoticeKind notice_kind, W8MessageBoxPayload payload,
                              int npc_kind);
void DrawNpcQuoteBubble(void);
void LookAtDialogueNpc(void);
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, unsigned char flags,
                              unsigned char force);
void BeginScriptedWorldAction(void);
void CloseNpcDialogueLayout(void);
void OpenNpcDialogueTranscriptLayout(void);
void DispatchPendingNpcScriptNotice(void);
bool CanOpenNpcDialogue(void);
bool IsNpcDialogueTextInputActive(void);
bool IsNpcDialogueTextBoxActive(void);
unsigned char SetNpcDialoguePanelVisible(unsigned char value);
bool ProcessPendingEvent(void);
void SyncDialogueNpcStateAndMarkPending(void);
void ClearMainGameTargetState(void);
/* A script notice is staged in g_pending_notice */
extern bool g_pending_notice_queued;
void SyncNpcServiceButtons(int party_slot);
/* Forward mouse events to W8NpcInteractionState control slots indexed by
   callback_id in dialogue_controls (id 0x27 is ignored). */
unsigned char MainScreenControlRegionEvent(const InputAtom* event, struct W8Region* region);
void SwitchNpcDialogueLayout(W8NpcDialogueLayout layout);
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, unsigned char flags,
                      unsigned char force);
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item, bool force);
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags);
void CreateNpcDialogueControls(void);
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect);
void SetNpcDialogueLayoutMode(W8NpcDialogueLayout value);
void CloseNpcDialogueMode1Layout(void);
void CloseNpcDialogueTranscriptLayout(void);
void CloseNpcDialogueOptionLayout(void);
void CloseNpcDialogueMode5Layout(void);
void ShowNpcDialogueTopicMenu(void);
void HandleNpcDialogueDeparture(unsigned char value);
unsigned char HandleNpcDialogueItem(W8ItemInstance* item);
unsigned char AcceptNpcDialogueItem(W8NpcState* npc, W8ItemInstance* item, int mode);
void TranslateDialogueKeyword(const wchar_t* source, wchar_t* destination);
void ResetNpcDialogueItemEditor(void);
void SetNpcDialogueHidden(char value);
/* While NPC script deferral holds character events, drain Escape / click so
   the open dialogue layout can dismiss without the normal input path. */
void DrainNpcDialogueDeferralInput(void);
/* When world_cursor_gate is set, discard queued input after a mouse-position hook so
   the world-cursor gate does not process stale events. */
void FlushInputWhileWorldCursorGate(void);
void HandleNpcDialogueReply(wchar_t* text, bool echo);
void HandleNpcDialogueInput(void);
void OpenNpcDialog(W8NpcQuoteEntry* request, int aux_data);
void OnNpcDialogClosed(W8DialogBase* dialog);
void ConfirmNpcTradePurchase(void);
/* Learn one keyword into the dialogue transcript. category -1
   auto-classifies the text against items, NPC/named-monster names and the
   place-name table; a nonzero play_chime rings the keyword chime. */
void AddNpcDialogueKeyword(wchar_t* text, signed char category, int play_chime);
void ClearNpcDialogueTranscript(void);
void CloseNpcDialogueForCamp(void);
void OpenNpcDialogueOptionLayout(void);
void OpenNpcDialogueMode1Layout(void);
void OpenNpcDialogueMode5Layout(void);
void UpdateNpcDialogueSubMode(void);
/* Restate the five transcript category buttons so only the
   active dialogue_category_filter's button shows its secondary state. */
void SyncDialogueCategoryButtons(void);
void EndNpcDialogueSession(bool skip_exit_actions);

// FUNCTION: WIZ8 0x00576b80
inline void CloseNpcDialogueIfActive(void)
{
    if (gXStatus.fNpcDialogueMode) {
        EndNpcDialogueSession(false);
    }
}
/* Whether an open NPC dialogue transcript covers the party slot's portrait:
   dialogue mode up, scripted_dialogue clear, the controller enabled, and its top
   edge above the slot's band. Portrait and character-update paths skip the
   covered rows through this. */
bool IsPortraitObscuredByNpcDialogue(unsigned int party_slot);
void RecordLevelEntryDialogueState(void);
unsigned char IsNpcDialogueCursorActive(void);
/* Forward a portrait pick into an active NPC dialogue. */
void TryNpcDialoguePickpocket(int party_slot);
void ShortenTextToWidth(wchar_t* output, const wchar_t* text, unsigned int width, int font);
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event, W8Region* region);
void SetDialogueFieldKeyword(wchar_t* keyword, bool append);
void ActivateNpcDialoguePanels(bool active);
bool HasNpcDialogueDirtyPanels(void);
unsigned char NpcDialogueTextBoxRegionEvent(const InputAtom* event, W8Region* region);
void NpcDialogueTextBoxWheelAt(short x, unsigned short y, bool flag);
/* True when an NPC quote/portrait session is active: finish voice playback and
   report that the click was consumed. */
bool FinishNpcVoiceIfSessionActive(void);
void ToggleNpcHandItemFilter(void);
void ToggleNpcAccessoryItemFilter(void);
void ToggleNpcBodyItemFilter(void);
void ToggleNpcOtherItemFilter(void);
void ToggleNpcCharacterUsabilityFilter(void);
void ToggleNpcPartyUsabilityFilter(void);
void BackOutNpcDialogue(void);
void SubmitNpcWhereIsQuery(void);
void SubmitNpcDialogueInput(void);
void SelectNpcDialogueService(void);
void SelectNpcDialogueTalk(void);
void SelectNpcDialogueExit(void);
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
void SelectNpcPeopleTopics(void);
void SelectNpcPlaceTopics(void);
void SelectNpcItemTopics(void);
void SelectNpcMiscTopics(void);
void SelectNpcDialogueCategoryAll(void);
void SelectNpcSellMode(void);
void SelectNpcGiveMode(void);
void SelectNpcShopliftMode(void);
void SelectNpcPartyItems(void);
void SelectNpcStockItems(void);
void OpenNpcItemAssay(void);
void OnNpcAssayDialogClosed(W8DialogBase* dialog);
void ConfirmNpcTradeSlot(void);
void RequestNpcSpellService3(void);
void RequestNpcSpellService41(void);
void RequestNpcCharacterService(void);
void OnNpcTradeDialogClosed(W8DialogBase* dialog);
void UpdateNpcTradeSelection(int index, int, int);
void OpenNpcGoldAmountDialog(void);
/* The dialogue text-box's button-up/double-click handlers, dispatched from
   NpcDialogueTextBoxRegionEvent. */
void NpcDialogueTextBoxLeftUp(int x, int y);
void NpcDialogueTextBoxRightUp(int x, int y);
void NpcDialogueTextBoxDoubleClick(int x, int y);
/* W8SplitAmountDialog destroy callback installed by OpenNpcGoldAmountDialog. */
void OnNpcTradeSplitDialogDestroy(W8DialogBase* dialog);
W8ItemInstance* ResolveNpcTradeRow(int index, bool, char, char);
bool NpcTradeItemAllowed(W8ItemInstance* item);
void EnableNpcTradeFilterButtons(void);
W8ItemInstance* GetNpcTradeSlotItem(int index);
void HandleNpcDialogueKeyEvent(const InputAtom* event);
unsigned char LoadNpcDialogueTranscript(unsigned int file);
unsigned char SaveNpcDialogueTranscript(unsigned int file);
void HandleNpcDialogueItemChoice(void);
void RefreshNpcTradePartyGold(void);
void RefreshNpcTradePrice(void);
void ResolveNpcPickpocket(int party_slot);
