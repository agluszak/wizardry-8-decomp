#pragma once

#include <stddef.h>

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

extern unsigned char g_flag_659756;
extern unsigned char g_save_pending_00689f98;
extern unsigned char g_save_notice_shown_0068506b;
extern unsigned char g_flag_0068510d;
