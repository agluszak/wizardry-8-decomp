#pragma once

#include <stddef.h>
#include "wiz8/wiz8_windows.h"
#include "wiz8/vector.h"

unsigned char VerifyDataSubdirs(void);
unsigned char FindStartupQuickSave(char* slot_name);
int GetSaveGameLevel(const char* slot_name);

/* The SHOT record. SaveGame writes 0x2588 bytes; the save-screen producer
   copy-constructs one complete record, including its trailing padding.
   The 80 by 60 16-bit surface begins at offset 6. */
struct W8SaveScreenshot {
    float version;
    unsigned char capture_result;
    unsigned char unknown_05;
    unsigned short pixels[60][80];
};

static_assert(sizeof(W8SaveScreenshot) == 0x2588,
              "W8SaveScreenshot_must_be_0x2588");

/* Save-list entries are allocated as 0x2640 bytes by both the enumerator and
   the Options controller. The SHOT chunk occupies the embedded screenshot. */
struct W8SaveSlot {
    wchar_t name[64];
    FILETIME local_write_time;
    SYSTEMTIME timestamp;
    int game_time_days;
    int game_time_ms;
    int level_id;
    unsigned char iron_man;
    unsigned char unknown_0a5[3];
    W8SaveScreenshot screenshot;
    int version_major;
    int version_minor;
    int version_patch;
    unsigned char flag_263c;
    unsigned char unknown_263d[3];
};

static_assert(sizeof(W8SaveSlot) == 0x2640, "W8SaveSlot_size");
static_assert(offsetof(W8SaveSlot, screenshot) == 0xa8, "W8SaveSlot_screenshot_offset");

void CaptureSaveScreenshot(W8SaveScreenshot* screenshot);
void FillCurrentSaveSlot(W8SaveSlot* slot);
unsigned char EnumerateSaveSlots(W8GrowableVector<W8SaveSlot*>* slots);

unsigned char SaveGame(const char* name, W8SaveScreenshot* screenshot);

unsigned char AutoSaveIfAllowed(char forced);

unsigned char TakePendingSaveFlag(void);

struct W8Character;
struct W8Chunk;
struct W8GlobalStatus;
unsigned char SaveSlotFileExists(const char* slot_name);
unsigned char LoadCharacter(const char* name, W8Character* character, int slot,
                            char report_failure);
void BuildCharacterFilePath00514FA0(char* destination, const char* filename,
                                    int slot);
void BuildCharacterPath00514EC0(char* destination, const wchar_t* name,
                                int slot);
unsigned char SaveGameExists(void);
void LoadGameStatus(W8Chunk* chunks, W8GlobalStatus* status);
/* 0x00512920: load a save slot by name; the Please Wait screen drives it. */
unsigned int LoadGame(const char* slot_name);

unsigned char SaveLevelStatus(const char* path);
unsigned char LoadLevelStatus(const char* path, int level);
void BuildLevelStatusPath(char* path, unsigned int level);
unsigned char LoadStatusHeader(W8Chunk* chunk);
unsigned char SaveStatusHeader(W8Chunk* chunks);
/* The per-level section reader behind LoadLevelStatus, and the already-open
   save scan behind SaveLevelStatus. Both keep their address names until a
   reviewed body supplies a semantic one. */
unsigned char LoadItemStatus(W8Chunk* chunk, int level);
unsigned char MeasureLevelStatusChunks00514DF0(
    W8Chunk* chunk, int level, unsigned int* empty_percent);

extern unsigned char g_flag_659756;
extern unsigned char g_save_pending_00689f98;
extern unsigned char g_save_notice_shown_0068506b;

char Function5155B0(const char* path, int slot, W8Character* character);
char Function5156C0(const char* path, W8Character* character);
void Function515B00(void);
void SaveMonsterStatus(W8Chunk* chunks);                         /* 0x005145A0 */


void Function512C40(void);

