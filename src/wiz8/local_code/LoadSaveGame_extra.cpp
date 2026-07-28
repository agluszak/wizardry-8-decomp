#include "wiz8/gameplay_boundaries.h"

#include <stdio.h>

/*
 * Local Code\LoadSaveGame.cpp, continued.
 *
 * Save-slot bookkeeping: the flag the loader hands back once, whether a slot's
 * file is there at all, and the chunk walk that reads a saved game back.
 */

/* The two chunk tags the walk recognises, as the four-character codes the
   comparison spells them. */
enum { W8_SAVE_TAG_CHAR = 0x52414843, W8_SAVE_TAG_LVLS = 0x534c564c };

extern unsigned char g_save_pending_00689f98;
extern unsigned char g_save_flag_00687599;
extern unsigned char g_save_notice_shown_0068506b;
extern int g_screen_state_0068ec78;
extern int g_current_level;
extern void ShowNotice(int channel, void* notice, int a, int b, int c);  /* 0x0058AC00 */
extern unsigned char FileExists(const char* path);
extern int SaveChunkCount(void);                                         /* 0x0055C6C0 */
extern void SaveChunkSeek(int arg_1, int arg_2);                         /* 0x0055C6D0 */
extern unsigned char SaveChunkAtEnd(void);                               /* 0x0055CB60 */
extern int SaveChunkTag(void);                                           /* 0x0055C660 */
extern void ReadCharacterChunk(int arg_1);                               /* 0x0055C1E0 */
extern void SaveChunkOpen(void);                                         /* 0x0055C3F0 */
extern void SaveChunkRead(void* buffer, int size, int arg_3);            /* 0x0055CA20 */
extern void SaveChunkRewind(void);                                       /* 0x0055CAE0 */
extern void SaveChunkClose(void);                                        /* 0x0055C5A0 */
extern void SaveChunkNext(void);                                         /* 0x0055C390 */
extern void SaveChunkRelease(void);                                      /* 0x0055C930 */

/* Take the pending-save flag and clear it in one go, so the caller that reads
   it is the only one that sees it. */
// FUNCTION: WIZ8 0x00515910
unsigned char TakePendingSaveFlag(void)
{
    unsigned char pending = g_save_pending_00689f98;

    g_save_pending_00689f98 = 0;
    return pending;
}

/* Whether one save slot's file is on disk. The path is built into a MAX_PATH
   buffer from the saves directory, the slot name and the extension. */
// FUNCTION: WIZ8 0x00512F70
unsigned char SaveSlotFileExists(const char* slot_name)
{
    char path[260];

    sprintf(path, "%s%s%s", "Saves", slot_name, ".SAV");
    return FileExists(path);
}

/* Note that the save could not be written. The notice is only shown on the
   screen that owns saving, but the flag is raised either way. */
// FUNCTION: WIZ8 0x00515AC0
void ReportSaveFailed(char quiet)
{
    if (quiet == 0 || g_save_flag_00687599 != 0) {
        g_save_notice_shown_0068506b = 1;
        if (g_screen_state_0068ec78 == 7) {
            ShowNotice(0xc, g_notices[0x1e0c / 4], -1, -1, 0);
        }
    }
}

/* Walk every chunk of a saved game. Character chunks are read straight in; a
   level chunk is read only for the level the party is actually on, and one for
   any other level is rewound and read as a character chunk instead. */
// FUNCTION: WIZ8 0x00514D50
void ReadSaveChunks(int destination)
{
    int remaining = SaveChunkCount();
    int level;

    if (remaining <= 0) {
        return;
    }
    do {
        SaveChunkSeek(0, 0);
        if (!SaveChunkAtEnd()) {
            if (SaveChunkTag() == W8_SAVE_TAG_CHAR) {
                ReadCharacterChunk(destination);
            }
            else if (SaveChunkTag() == W8_SAVE_TAG_LVLS) {
                SaveChunkOpen();
                SaveChunkRead(&level, 4, 0);
                if (level != g_current_level) {
                    SaveChunkRewind();
                    ReadCharacterChunk(destination);
                }
                SaveChunkClose();
            }
        }
        SaveChunkNext();
        SaveChunkRelease();
        --remaining;
    } while (remaining != 0);
}
