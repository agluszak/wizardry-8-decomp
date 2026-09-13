#pragma once

#include "wiz8/mouth_gap.h"
#include "wiz8/message_box.h"
#include "surrender/srMath.h"

#include <stddef.h>

struct W8NpcState;
struct W8RecordFile0055A480;
struct W8ItemInstance;
struct W8Character;
struct W8MonsterGroup;
struct W8WorldItem;

#pragma pack(push, 1)
struct W8NpcDialogueStagingRestore {
    int value_494;
    int value_498;
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
    W8RecordFile0055A480* record_file;
    W8NpcState* npc;
    int voice_handle;
    unsigned int value_84;
    unsigned int value_88;
    W8GrowableVector<W8MessageBoxLine*> message_lines;
    W8GrowableVector<int*> pending_script_values;
    W8MouthGapTrack gap_track;
    unsigned int last_tick;
    unsigned char flag_c4;
    unsigned char flag_c5;
    unsigned char flag_c6;
    unsigned char flag_c7;
    unsigned char flag_c8;
    unsigned char flag_c9;
    unsigned char flag_ca;
    unsigned char flag_cb;
};

static_assert(offsetof(W8NpcScriptingState, staging_restore) == 0x64,
              "W8NpcScriptingState_staging_restore_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_00) == 0x00,
              "W8NpcScriptingState_unknown_00_offset");
static_assert(offsetof(W8NpcScriptingState, unknown_72) == 0x72,
              "W8NpcScriptingState_unknown_72_offset");
static_assert(offsetof(W8NpcScriptingState, flag_70) == 0x70, "W8NpcScriptingState_flag_70_offset");
static_assert(offsetof(W8NpcScriptingState, flag_71) == 0x71, "W8NpcScriptingState_flag_71_offset");
static_assert(offsetof(W8NpcScriptingState, record_file) == 0x78,
              "W8NpcScriptingState_record_file_offset");
static_assert(offsetof(W8NpcScriptingState, npc) == 0x7c, "W8NpcScriptingState_npc_offset");
static_assert(offsetof(W8NpcScriptingState, voice_handle) == 0x80,
              "W8NpcScriptingState_voice_handle_offset");
static_assert(offsetof(W8NpcScriptingState, value_84) == 0x84,
              "W8NpcScriptingState_value_84_offset");
static_assert(offsetof(W8NpcScriptingState, value_88) == 0x88,
              "W8NpcScriptingState_value_88_offset");
static_assert(offsetof(W8NpcScriptingState, message_lines) == 0x8c,
              "W8NpcScriptingState_message_lines_offset");
static_assert(offsetof(W8NpcScriptingState, pending_script_values) == 0x9c,
              "W8NpcScriptingState_pending_script_values_offset");
static_assert(offsetof(W8NpcScriptingState, gap_track) == 0xac,
              "W8NpcScriptingState_gap_track_offset");
static_assert(offsetof(W8NpcScriptingState, last_tick) == 0xc0,
              "W8NpcScriptingState_last_tick_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c4) == 0xc4, "W8NpcScriptingState_flag_c4_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c5) == 0xc5, "W8NpcScriptingState_flag_c5_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c6) == 0xc6, "W8NpcScriptingState_flag_c6_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c7) == 0xc7, "W8NpcScriptingState_flag_c7_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c8) == 0xc8, "W8NpcScriptingState_flag_c8_offset");
static_assert(offsetof(W8NpcScriptingState, flag_c9) == 0xc9, "W8NpcScriptingState_flag_c9_offset");
static_assert(offsetof(W8NpcScriptingState, flag_ca) == 0xca, "W8NpcScriptingState_flag_ca_offset");
static_assert(offsetof(W8NpcScriptingState, flag_cb) == 0xcb, "W8NpcScriptingState_flag_cb_offset");
static_assert(sizeof(W8NpcScriptingState) == 0xcc, "W8NpcScriptingState_size");

extern W8NpcScriptingState g_npc_scripting;

void UpdateNpcDialogueVoiceAndCursor(void);  /* 0x00524DA0 */
void ProcessNpcScriptingFrame(void);         /* 0x00524EB0 */
void SetFlag68C500(unsigned char value);     /* 0x0052A1A0 */
void Function5289B0(int kind, int argument); /* 0x005289B0 */
void Function529510(void);
void Function528830(int a, int b, int c, int d);                          /* 0x00528830 */
void Function529BE0(void);                                                /* 0x00529BE0 */
void Function529EF0(void);                                                /* 0x00529EF0 */
void Function50C440(W8NpcState* npc, int value);                          /* 0x0050C440 */
void Function50C1C0(char name_style, int value, const char* entity_name); /* 0x0050C1C0 */
unsigned char ClearNpcScheduledItem(W8NpcState* npc, int item_id,
                                    W8ItemInstance* out);         /* 0x0050BA80 */
void ReleaseNpcMonsterByKind(int kind);                           /* 0x0050C680 */
void SetFactionDispositionBand(char faction, unsigned char band); /* 0x00535B40 */
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility,
                              char recurse);            /* 0x00547570 */
void Function553AD0(W8Character* character, int value); /* 0x00553AD0 */
void Function553C10(W8Character* character, int skill); /* 0x00553C10 */
void Function420F90(srVector3T<float>* position);       /* 0x00420F90 */
void Function4F6CF0(W8WorldItem* item);                 /* 0x004F6CF0 */
void Function5A6580(void);                              /* 0x005A6580 */
void SetFlag68C4F4(void);                               /* 0x00529560 */
void SetFlag68C4F7(void);                               /* 0x00529BC0 */
void ClearFlag68C4F7(void);                             /* 0x00529BD0 */
