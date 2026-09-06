#include "wiz8/music_playlist.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/monster_generators.h"
#include "wiz8/utility.h"
#include "wiz8/screen_state.h"
#include "wiz8/sgp_video.h"
#include "Container.h"
#include "LibraryDataBase.h"
#include "shading.h"
#include "sgp.h"

#include <string.h>
#include <stdlib.h>

extern void Function54AF30(unsigned char release);
extern void InitializeGameplayRuntimeObjects(void);
extern unsigned char Function54A760(W8MonsterRecord** records);
extern unsigned char VerifyDataSubdirs(void);
extern void InitializeItemVideoObjects(void);
extern unsigned char FindStartupQuickSave(char* slot_name);
extern int GetSaveGameLevel(const char* slot_name);
extern void SetPendingScreenState(int value);
extern unsigned char InitializeSpellDatabase(void);
extern void ReleaseGenericItemNames(void);
extern void UnloadEncounterTables(void);
extern void ReleaseSpellDatabase(void);
extern void ReleaseAllTriggers(void);
extern void ReleasePointer689B40(void);

/*
 * The data bring-up gate BringUpEngine calls last. It stamps the version
 * string, opens the archives and string data, then walks every database
 * loader in turn, abandoning the sequence the moment one fails. Its callees
 * are mostly unidentified and carry address-derived names; the ones already
 * recovered elsewhere keep theirs.
 */

extern unsigned char InitializeMenuStartupSubsystems(void);

extern unsigned char InitializeSlfArchives(void);
extern int LoadPatchSlfArchives(const char* directory);
extern void LoadLocalizedStrings(const char* path);
extern void LoadGameConfiguration(void);
extern unsigned char InitializeMenuFonts(void);
extern void InitializeMessageBoxState(void);
extern void InitializeRegionHelpState(void);
extern void SetMessageBoxModeDisabled(void);
extern void SetMessageBoxModeEnabled(void);
extern void SetMessageBoxWord(unsigned short value);
extern void UpdateHeldItemCursor(void);
extern void Function479010(void);
extern unsigned char LoadMissileDatabase(void);
extern unsigned char LoadHitSoundDatabase(void);
extern unsigned int GetTotalPhysicalMemory(void);

extern void* g_stack_68eda8;
extern unsigned short g_word_6850ed;
extern unsigned short* g_font_state_palettes_68ee1c[15];
extern unsigned char g_flag_65beaf;

// FUNCTION: WIZ8 0x004e2f40
unsigned char InitializeGameData(void)
{
    char version[64];
    void* buffer;
    UINT32 count;
    unsigned char ok;

    version[0] = '\0';
    strcat(version, "Wizardry 8 ");
    strcat(version, FormatString("v%d.%d.%d", 1, 2, 4));
    strcat(version, FormatString(" (build %d)", 0xdb));
    strcat(version, FormatString(" %s", "2001/12/24 15:36"));
    InitializeSlfArchives();
    LoadPatchSlfArchives("Patches");
    LoadLocalizedStrings(gzStringDataOverride ? gzStringDataOverride : "Data\\Strings\\StringData.DAT");
    buffer = LockPrimarySurface(&count);
    memset(buffer, 0, count * 0x1e0);
    UnlockPrimarySurface();
    LoadGameConfiguration();
    g_screen_state_0068ec78.id = -1;
    g_dword_68ed10.id = -1;
    g_stack_68eda8 = CreateStack(5, 0x98);
    if (!g_stack_68eda8) {
        return 0;
    }
    if (!InitializeMenuStartupSubsystems()) {
        return 0;
    }
    SetShadeTablePercent((FLOAT)0.66);
    BuildShadeTable();
    if (!InitializeMenuFonts()) {
        return 0;
    }
    InitializeMessageBoxState();
    InitializeRegionHelpState();
    if (g_settings_6850c8.field_00c) {
        SetMessageBoxModeEnabled();
    } else {
        SetMessageBoxModeDisabled();
    }
    SetMessageBoxWord(g_word_6850ed);
    Function54AF30(0);
    InitializeGameplayRuntimeObjects();
    UpdateHeldItemCursor();
    if (!VerifyDataSubdirs()) {
        return 0;
    }
    if (!InitializeItemDatabase()) {
        return 0;
    }
    if (!InitializeItemTables()) {
        return 0;
    }
    if (!Function54A760(0)) {
        return 0;
    }
    if (!InitializeNpcDatabase()) {
        return 0;
    }
    if (!InitializeFactDatabase()) {
        return 0;
    }
    if (!InitializeLevelDatabase()) {
        return 0;
    }
    InitializeItemVideoObjects();
    Function479010();
    SetPendingScreenState(0);
    g_status_685170.current_level = -1;
    if (gfLoadAtStartup && FindStartupQuickSave(g_dword_68ed10.name)) {
        g_dword_68ed10.mode = 1;
        g_dword_68ed10.parameter = GetSaveGameLevel(g_dword_68ed10.name);
        SetPendingScreenState(4);
    }
    InitializeEncounterTables();
    if (!LoadMissileDatabase()) {
        ShutdownWithErrorBox("Could not load missile database!");
    }
    if (!InitializeSpellDatabase()) {
        ShutdownWithErrorBox("Could not load spell database!");
    }
    if (!LoadHitSoundDatabase()) {
        ShutdownWithErrorBox("Could not load hit sound database!");
    }
    memset(g_message_storage_68f2d8, 0, sizeof(g_message_storage_68f2d8));
    if (!InitializeMusicPlaylist()) {
        return 0;
    }
    ok = (unsigned char)(0x4000000 < GetTotalPhysicalMemory());
    g_flag_65beaf = ok;
    return 1;
}

extern void ReleaseHitSoundDatabase(void);
extern void ReleaseMissileDatabase(void);

// FUNCTION: WIZ8 0x004e3290
void ShutdownGameData(void)
{
    int index;

    ReleaseHitSoundDatabase();
    ReleaseMissileDatabase();
    for (index = 0; index < 15; ++index) {
        free(g_font_state_palettes_68ee1c[index]);
        g_font_state_palettes_68ee1c[index] = 0;
    }
    if (g_stack_68eda8) {
        DeleteStack(g_stack_68eda8);
        g_stack_68eda8 = 0;
    }
    ShutDownFileDatabase();

}
