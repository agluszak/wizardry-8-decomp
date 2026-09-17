#pragma once

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

#pragma pack(push, 1)
struct W8NpcDialogueStagingRestore {
    int current_quote_index;
    int finished_quote_index;
    unsigned short unknown_49c;
    short staged_short_49e;
};
#pragma pack(pop)

static_assert(sizeof(W8NpcDialogueStagingRestore) == 12, "W8NpcDialogueStagingRestore_must_be_12");

struct W8NpcScriptingState {
    unsigned char unknown_00[0x64];
    W8NpcDialogueStagingRestore staging_restore;
    unsigned char flag_70;
    unsigned char flag_71;
    unsigned char unknown_72[6];
    W8NpcScriptFile* script_file;
    W8NpcState* npc;
    int voice_handle;
    unsigned int message_duration_ms;
    unsigned int message_started_at;
    W8GrowableVector<W8MessageBoxLine*> message_lines;
    W8GrowableVector<int*> pending_script_values;
    W8MouthGapTrack gap_track;
    unsigned int last_tick;
    unsigned char flag_c4;
    unsigned char restore_staged_session;
    unsigned char portrait_message_active;
    unsigned char scripted_scene_active;
    unsigned char sedexus_release_pending;
    unsigned char sedexus_capture_pending;
    unsigned char sedexus_capture_active;
    unsigned char stopping_voice_playback;
};

static_assert(offsetof(W8NpcScriptingState, staging_restore) == 0x64,
              "W8NpcScriptingState_staging_restore_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_00) == 0x00,
              "W8NpcScriptingState_unknown_00_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_72) == 0x72,
              "W8NpcScriptingState_unknown_72_offset");
static_assert(offsetof(W8NpcScriptingState, flag_70) == 0x70, "W8NpcScriptingState_flag_70_offset");
static_assert(offsetof(W8NpcScriptingState, flag_71) == 0x71, "W8NpcScriptingState_flag_71_offset");
static_assert(offsetof(W8NpcScriptingState, script_file) == 0x78,
              "W8NpcScriptingState_script_file_offset");
static_assert(offsetof(W8NpcScriptingState, npc) == 0x7c, "W8NpcScriptingState_npc_offset");
static_assert(offsetof(W8NpcScriptingState, voice_handle) == 0x80,
              "W8NpcScriptingState_voice_handle_offset");
static_assert(offsetof(W8NpcScriptingState, message_duration_ms) == 0x84,
              "W8NpcScriptingState_message_duration_ms_offset");
static_assert(offsetof(W8NpcScriptingState, message_started_at) == 0x88,
              "W8NpcScriptingState_message_started_at_offset");
static_assert(offsetof(W8NpcScriptingState, message_lines) == 0x8c,
              "W8NpcScriptingState_message_lines_offset");
static_assert(offsetof(W8NpcScriptingState, pending_script_values) == 0x9c,
              "W8NpcScriptingState_pending_script_values_offset");
static_assert(offsetof(W8NpcScriptingState, gap_track) == 0xac,
              "W8NpcScriptingState_gap_track_offset");
static_assert(offsetof(W8NpcScriptingState, last_tick) == 0xc0,
              "W8NpcScriptingState_last_tick_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c4) == 0xc4, "W8NpcScriptingState_flag_c4_offset");
static_assert(offsetof(W8NpcScriptingState, restore_staged_session) == 0xc5,
              "W8NpcScriptingState_restore_staged_session_offset");
static_assert(offsetof(W8NpcScriptingState, portrait_message_active) == 0xc6,
              "W8NpcScriptingState_portrait_message_active_offset");
static_assert(offsetof(W8NpcScriptingState, scripted_scene_active) == 0xc7,
              "W8NpcScriptingState_scripted_scene_active_offset");
static_assert(offsetof(W8NpcScriptingState, sedexus_release_pending) == 0xc8,
              "W8NpcScriptingState_sedexus_release_pending_offset");
static_assert(offsetof(W8NpcScriptingState, sedexus_capture_pending) == 0xc9,
              "W8NpcScriptingState_sedexus_capture_pending_offset");
static_assert(offsetof(W8NpcScriptingState, sedexus_capture_active) == 0xca,
              "W8NpcScriptingState_sedexus_capture_active_offset");
static_assert(offsetof(W8NpcScriptingState, stopping_voice_playback) == 0xcb,
              "W8NpcScriptingState_stopping_voice_playback_offset");
static_assert(sizeof(W8NpcScriptingState) == 0xcc, "W8NpcScriptingState_size");

extern W8NpcScriptingState g_npc_scripting;
extern unsigned char g_message_queue_idle_68c501; /* 0x0068C501 */

void RunNpcScriptLine(int script_line, unsigned char param);        /* 0x00525FA0 */
void ProcessMessageBoxQueue(void);                                  /* 0x00526E90 */
void ProcessNpcQuoteEntry(W8NpcQuoteEntry* entry, int script_line); /* 0x00526810 */
/* 0x00528FF0: the first argument is a pointer into a character or party item
   slot - the slot whose address matches is the one removed. */
void RemoveNpcScriptItem(W8ItemInstance* item, int match_item_id, int item_id);
/* 0x00528CD0: look the item's fact up; both out-pointers are optional. */
int FindNpcScriptItemQuote(int item_id, short* index, unsigned char* grants_item);
void ResolveSedexusCapture(void);                         /* 0x00529F90 */
void NpcScriptHenchmanArrives(W8Monster* monster);        /* 0x0052A080 */
void NpcScriptHenchmanDeparted(W8Monster* monster);       /* 0x0052A150 */
void NpcScriptSavantHackDone(W8Monster* monster);         /* 0x0052A190 */
void NpcScriptCallback00526E40(void);                     /* 0x00526E40 */
void NpcScriptCallback00526E70(void);                     /* 0x00526E70 */
void OnNpcTravelConfirmationClosed(W8DialogBase* dialog); /* 0x0052A1B0 */
void UpdateNpcDialogueVoiceAndCursor(void);               /* 0x00524DA0 */
void ProcessNpcScriptingFrame(void);                      /* 0x00524EB0 */
void SetFlag68C500(unsigned char value);                  /* 0x0052A1A0 */
unsigned char GetFlag68C4FA(void);                        /* 0x0052A070 */
void QueueNpcMessageLine(int kind, int argument);         /* 0x005289B0 */
/* 0x00525E80: searches the NPC script's kind-4 quote entries for a keyword
   match; both out-pointers are optional. */
int FindNpcKeywordQuote(wchar_t* keyword, short* quote_index, short* keyword_index);
/* 0x00528D50: resolves NPC-name, named-person, and region keywords to a quote
   id; returns -1 when nothing matches. */
int FindNpcNameOrPlaceQuote(W8NpcState* npc, wchar_t* text);
int FindNpcReplyQuote(wchar_t* text);            /* 0x00529300 */
void RestoreCurrentNpcQuoteBubble(void);         /* 0x00529510 */
void RunNpcQuoteDeclineActions(int quote_index); /* 0x00529610 */
void QueueNpcScriptLine(int quote, unsigned char mark_pending, unsigned char prepend,
                        unsigned char suppress_entries); /* 0x00528830 */
void BeginNpcScriptedScene(void);                        /* 0x00529BE0 */
void SetScriptedSceneActive(void);                       /* 0x00529BC0 */
void ClearScriptedSceneActive(void);                     /* 0x00529BD0 */
void BeginSedexusCapture(void);                          /* 0x00529EF0 */
unsigned char IsSedexusCaptureActive(void);              /* 0x0052A070 */
void SetNpcScriptEventActive(unsigned char value);       /* 0x0052A1A0 */
void Function553AD0(W8Character* character, int value);  /* 0x00553AD0 */
void Function553C10(W8Character* character, int skill);  /* 0x00553C10 */
/* in GameData.h: CameraLookAt (0x00420F90) */
/* in ReviewCharacterScreen.h: BeginEndgameSequence005A6580 (0x005A6580) */
void SetFlag68C4F4(void); /* 0x00529560 */
void AuditNpcScriptQuotes00529660(void);
void SetFlag68C4F7(void);   /* 0x00529BC0 */
void ClearFlag68C4F7(void); /* 0x00529BD0 */
/* 0x00524CA0: the NPC-side rebinding pass; reloads the NPC's .nsf script
   file and rebuilds its runtime bindings. */
void ReloadNpcScriptResources(W8NpcState* npc);
