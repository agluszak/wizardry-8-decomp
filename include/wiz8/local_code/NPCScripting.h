#pragma once

extern wchar_t g_format_al_s[];

bool GetNpcScriptRegionName(int region, wchar_t* name);
void StripNpcKeywordPunctuation(wchar_t* text);

#include "wiz8/mouth_gap.h"
#include "wiz8/message_box.h"
#include "wiz8/vector.h"
#include "surrender/srMath.h"

#include <stddef.h>

struct W8NpcState;
struct W8NpcScriptFile;
struct W8NpcQuoteEntry;
struct W8ItemInstance;
struct W8Character;
struct W8MonsterGroup;
struct W8WorldItem;
class W8Monster;
class W8DialogBase;

struct W8NpcDialogueStagingRestore {
    int current_quote_index;
    int finished_quote_index;
    /* Which subquote of the staged quote is next; reset when a new quote
       is staged or the cursor runs past subquote_count. */
    unsigned char subquote_index;
    unsigned char unused;
    short staged_short;
};

static_assert(sizeof(W8NpcDialogueStagingRestore) == 12, "W8NpcDialogueStagingRestore_must_be_12");

struct W8NpcScriptingState {
    unsigned char unknown_00[0x64];
    W8NpcDialogueStagingRestore staging_restore;
    /* A quote is being presented - voice is playing or the text/EOS
       hold is counting down. Cleared by FinishNpcVoicePlayback. */
    unsigned char quote_active; // bool-byte-ok: staged-byte copy stays unnormalized
    /* The quote's voice sound actually started; gates SoundStop and the
       mouth-gap cleanup in FinishNpcVoicePlayback. */
    bool voice_playing;
    unsigned char unknown_72[6];
    W8NpcScriptFile* script_file;
    W8NpcState* npc;
    unsigned int voice_handle;
    unsigned int message_duration_ms;
    unsigned int message_started_at;
    W8GrowableVector<W8MessageBoxLine*> message_lines;
    W8GrowableVector<int*> pending_script_values;
    W8MouthGapTrack gap_track;
    unsigned int last_tick;
    /* Latched by CancelNpcDialogue; the response loop checks it at
       entries_done and abandons the pending response. */
    bool dialogue_cancelled;
    bool restore_staged_session;
    bool portrait_message_active;
    bool scripted_scene_active;
    bool sedexus_release_pending;
    bool sedexus_capture_pending;
    bool sedexus_capture_active;
    bool stopping_voice_playback;
};

static_assert(offsetof(W8NpcScriptingState, staging_restore) == 0x64,
              "W8NpcScriptingState_staging_restore_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_00) == 0x00,
              "W8NpcScriptingState_unknown_00_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_72) == 0x72,
              "W8NpcScriptingState_unknown_72_offset");
static_assert(offsetof(W8NpcScriptingState, quote_active) == 0x70,
              "W8NpcScriptingState_quote_active_offset");
static_assert(offsetof(W8NpcScriptingState, voice_playing) == 0x71,
              "W8NpcScriptingState_voice_playing_offset");
static_assert(offsetof(W8NpcScriptingState, script_file) == 0x78,
              "W8NpcScriptingState_script_file_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, npc) == 0x7c, "W8NpcScriptingState_npc_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, voice_handle) == 0x80,
              "W8NpcScriptingState_voice_handle_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, message_duration_ms) == 0x84,
              "W8NpcScriptingState_message_duration_ms_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, message_started_at) == 0x88,
              "W8NpcScriptingState_message_started_at_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, message_lines) == 0x8c,
              "W8NpcScriptingState_message_lines_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, pending_script_values) == 0x9c,
              "W8NpcScriptingState_pending_script_values_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, gap_track) == 0xac,
              "W8NpcScriptingState_gap_track_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, last_tick) == 0xc0,
              "W8NpcScriptingState_last_tick_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, dialogue_cancelled) == 0xc4,
              "W8NpcScriptingState_dialogue_cancelled_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, restore_staged_session) == 0xc5,
              "W8NpcScriptingState_restore_staged_session_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, portrait_message_active) == 0xc6,
              "W8NpcScriptingState_portrait_message_active_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, scripted_scene_active) == 0xc7,
              "W8NpcScriptingState_scripted_scene_active_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, sedexus_release_pending) == 0xc8,
              "W8NpcScriptingState_sedexus_release_pending_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, sedexus_capture_pending) == 0xc9,
              "W8NpcScriptingState_sedexus_capture_pending_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, sedexus_capture_active) == 0xca,
              "W8NpcScriptingState_sedexus_capture_active_offset");
W8_ABI_ASSERT(offsetof(W8NpcScriptingState, stopping_voice_playback) == 0xcb,
              "W8NpcScriptingState_stopping_voice_playback_offset");
W8_ABI_ASSERT(sizeof(W8NpcScriptingState) == 0xcc, "W8NpcScriptingState_size");

extern W8NpcScriptingState g_npc_scripting;
extern bool g_message_queue_idle;
/* Scripted portrait-pick / cutscene gate PortraitSelectRegionEvent
   and EndScriptedPortraitPick clear. */

void TryFinishNpcVoicePlayback(bool force);
int FindNpcScriptQuoteByKeyword(wchar_t* keyword, short* entry_index, short* sub_entry_index);
void RunNpcScriptLine(int script_line, bool force_npc_voice);
void ProcessMessageBoxQueue(void);
/* Execute a queued quote entry's deferred effect; the
   continuation quote is handed to the modal-dialog kinds (5/0x13 force -1). */
void ProcessNpcQuoteEntry(W8NpcQuoteEntry* entry, int continuation_quote);
/* Build a W8_NPC_MSG_QUOTE_ENTRY line for `entry` carrying
   `continuation_quote`; prepend != 0 inserts it at the queue front. */
void QueueNpcQuoteEntry(W8NpcQuoteEntry* entry, int continuation_quote, bool prepend);
/* The first argument is a pointer into a character or party item
   slot - the slot whose address matches is the one removed. */
void RemoveNpcScriptItem(W8ItemInstance* item, int match_item_id, int item_id);
/* Look the item's fact up; both out-pointers are optional. */
int FindNpcScriptItemQuote(int item_id, short* index, unsigned char* grants_item);
void ResolveSedexusCapture(void);
void NpcScriptHenchmanArrives(W8Monster* monster);
void NpcScriptHenchmanDeparted(W8Monster* monster);
void NpcScriptSavantHackDone(W8Monster* monster);
void NpcScriptTurnToBook(void);
void NpcScriptEndgameScreen(void);
void OnNpcTravelConfirmationClosed(W8DialogBase* dialog);
void UpdateNpcDialogueVoiceAndCursor(void);
void ProcessNpcScriptingFrame(void);
void SetNpcScriptEventActive(unsigned char value);
bool IsSedexusCaptureActive(void);
void QueueNpcMessageLine(W8NpcMessageKind kind, int argument);
/* Resolves NPC-name, named-person, and region keywords to a quote
   id; returns -1 when nothing matches. */
int FindNpcNameOrPlaceQuote(W8NpcState* npc, wchar_t* text);
int FindNpcReplyQuote(wchar_t* text);
void RestoreCurrentNpcQuoteBubble(void);
void RunNpcQuoteDeclineActions(int quote_index);
void QueueNpcScriptLine(int quote, bool mark_pending, bool prepend, bool suppress_entries);
void BeginNpcScriptedScene(void);
void SetScriptedSceneActive(void);
void ClearScriptedSceneActive(void);
/* End a scripted portrait pick against the chosen party slot. */
void EndScriptedPortraitPick(int party_slot);
void BeginSedexusCapture(void);
void CancelNpcDialogue(void);
/* Show the NPC quote bubble for the formatted line; a nonzero
   second argument also plays the startup jingle. */
void DisplayNpcQuote(const wchar_t* text, bool play_sound);
/* Advance the dialogue NPC's refusal state - each stage queues a
   different quote until the third, which stays queued. */
void QueueDialogueNpcRefusal(void);
void AuditNpcScriptQuotes(void);
/* The NPC-side rebinding pass; reloads the NPC's .nsf script
   file and rebuilds its runtime bindings. */
void ReloadNpcScriptResources(W8NpcState* npc);
