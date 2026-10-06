#pragma once

#include <stddef.h>
#include "wiz8/wiz8_windows.h"
#include "wiz8/vector.h"

bool VerifyDataSubdirs(void);
bool FindStartupQuickSave(char* slot_name);
int GetSaveGameLevel(const char* slot_name);

/* The SHOT record. SaveGame writes 0x2588 bytes; the save-screen producer
   copy-constructs one complete record, including its trailing padding.
   The 80 by 60 16-bit surface begins at offset 6. */
struct W8SaveScreenshot {
    float version;
    unsigned char capture_result;
    unsigned char padding_05;
    unsigned short pixels[60][80];
};

static_assert(sizeof(W8SaveScreenshot) == 0x2588, "W8SaveScreenshot_must_be_0x2588");

/* Save-list entries are allocated as 0x2640 bytes by both the enumerator and
   the Options controller. The SHOT chunk occupies the embedded screenshot. */
struct W8SaveSlot {
    wchar_t name[64];
    FILETIME local_write_time;
    SYSTEMTIME timestamp;
    int game_time_days;
    unsigned int game_time_ms;
    int level_id;
    bool iron_man;
    unsigned char padding_0a5[3];
    W8SaveScreenshot screenshot;
    int version_major;
    unsigned int version_minor;
    unsigned int version_patch;
    bool dev_flagged; /* saved copy of status dev_flagged */
    unsigned char padding_263d[3];
};

static_assert(sizeof(W8SaveSlot) == 0x2640, "W8SaveSlot_size");
static_assert(offsetof(W8SaveSlot, screenshot) == 0xa8, "W8SaveSlot_screenshot_offset");

void CaptureSaveScreenshot(W8SaveScreenshot* screenshot);
void FillCurrentSaveSlot(W8SaveSlot* slot);
bool EnumerateSaveSlots(W8GrowableVector<W8SaveSlot*>* slots);

bool SaveGame(const char* name, W8SaveScreenshot* screenshot);

struct W8Character;
bool SaveCharacter(W8Character* character, int slot, bool report_failure,
                   void (*continuation)(void)); /* 0x00515090 */

bool AutoSaveIfAllowed(bool forced);

bool TakePendingSaveFlag(void);

struct W8Chunk;
struct W8GlobalStatus;
unsigned char SaveSlotFileExists(const char* slot_name);
bool LoadCharacter(const char* name, W8Character* character, int slot, bool report_failure);
void BuildCharacterFilePath(char* destination, const char* filename, int slot);
void BuildCharacterPath(char* destination, const wchar_t* name, int slot);
bool SaveGameExists(void);
void LoadGameStatus(W8Chunk* chunks, W8GlobalStatus* status);
/* 0x00512920: load a save slot by name; the Please Wait screen drives it. */
bool LoadGame(const char* slot_name);

bool SaveLevelStatus(const char* path);
bool LoadLevelStatus(const char* path, int level);
void BuildLevelStatusPath(char* path, unsigned int level);
bool LoadStatusHeader(W8Chunk* chunk);
bool SaveStatusHeader(W8Chunk* chunks);
/* The per-level item-section reader and the already-open level-status scan. */
bool LoadItemStatus(W8Chunk* chunk, int level);
bool MeasureLevelStatusChunks(W8Chunk* chunk, int level, unsigned int* empty_percent);

extern bool g_save_pending;

/* Mark a matching CHAR payload consumed in Saves\\CurrentGame.SAV. */
bool MarkCurrentGameCharacterChunkConsumed(const char* path); /* 0x005154A0 */
/* Append one character record to Saves\\CurrentGame.SAV. `slot` is unused. */
bool SaveCharacterToCurrentGame(const char* path, int slot,
                                W8Character* character);                     /* 0x005155B0 */
bool LoadCharacterFromCurrentGame(const char* path, W8Character* character); /* 0x005156C0 */
/* Deferred main-game autosave: notice first, then SaveGame on the next tick. */
void ProcessMainGameAutoSave(void);                  /* 0x00515B00 */
bool SaveMonsterStatus(W8Chunk* chunks);             /* 0x005145A0 */
void SaveMonsterControlSpellEffect(W8Chunk* chunks); /* 0x00516580 */
void LoadMonsterControlSpellEffect(W8Chunk* chunks); /* 0x00516310 */

bool LoadMonsterGroup(W8Chunk* chunk); /* 0x00513C20 */
bool LoadMonster(W8Chunk* chunk);      /* 0x00513D80 */
/* 0x005139C0: folds the shipped per-level status file in before a save's
   section is applied. */
bool LoadDefaultLevelStatus(unsigned int level);

void ResetLiveSessionForLoad(void); /* 0x00512C40 */

bool SelectQuickSaveSlotForWrite(char* slot_name); /* 0x00516670 */
bool FindFreeEndingSaveName(char* name);           /* 0x00516890 */

extern char g_save_extension[]; /* 0x0061A144: initialized "SAV" */

void DeleteCurrentSaveFiles(void); /* 0x00515920 */

void ReportSaveFailed(bool quiet); /* 0x00515AC0 */
