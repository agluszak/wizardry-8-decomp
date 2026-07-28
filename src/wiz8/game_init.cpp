#include "wiz8/gameplay_boundaries.h"

#include <string.h>

/* GameplayDatabase.cpp is an original C++ translation unit.  Keep these
   declarations outside the C-linkage block: spelling them as C functions
   leaves the recovered bodies present but unreachable from this unit. */
extern void Function54AF30(unsigned char release);
extern void Function54AFD0(void);
extern unsigned char Function54A760(W8MonsterRecord** records);
extern unsigned char VerifyDataSubdirs(void);
extern void Function51B560(void);
extern void Function55EC50(int value);
extern unsigned char InitializeSpellDatabase(void);

/*
 * The data bring-up gate BringUpEngine calls last. It stamps the version
 * string, opens the archives and string data, then walks every database
 * loader in turn, abandoning the sequence the moment one fails. Its callees
 * are mostly unidentified and carry address-derived names; the ones already
 * recovered elsewhere keep theirs.
 */

extern "C" {

extern void* CreateStack(int count, int size);
extern unsigned char InitializeMenuStartupSubsystems(void);

extern unsigned char InitializeSlfArchives(void);
extern int LoadPatchSlfArchives(const char* directory);
extern void LoadLocalizedStrings(const char* path);
extern void* LockPrimarySurface(int* count);
extern void UnlockPrimarySurface(void);
extern void LoadGameConfiguration(void);
extern void SetStartupColorTransform(unsigned int value);
extern void RebuildStartupColorTable(void);
extern unsigned char InitializeMenuFonts(void);
extern void InitializeMessageBoxState(void);
extern void InitializeRegionHelpState(void);
extern void SetMessageBoxModeDisabled(void);
extern void SetMessageBoxModeEnabled(void);
extern void SetMessageBoxWord(unsigned short value);
extern void Function55EF90(void);
extern void Function479010(void);
extern unsigned char Function516740(void* slot);
extern int GetSaveGameLevel(void* slot);
extern void InitializeEncounterTables(void);
extern unsigned char Function4A5600(void);
extern unsigned char Function549B00(void);
extern void ShutdownWithErrorBox(char* message);
extern unsigned char Function48F940(void);
extern unsigned int GetTotalPhysicalMemory(void);

extern char* gzStringDataOverride;
extern unsigned char gfLoadAtStartup;
extern int g_dword_68ec78;
extern int g_dword_68ed10;
extern void* g_stack_68eda8;
extern int g_dword_686a70;
extern unsigned char g_flag_68ed14;
extern int g_dword_68ed18;
extern unsigned char g_save_slot_68ed28[];
extern unsigned char g_flag_6850d4;
extern unsigned short g_word_6850ed;
extern unsigned char g_block_68f2d8[0xc4e0];
extern unsigned char g_flag_65beaf;

// FUNCTION: WIZ8 0x004E2F40
unsigned char Function4E2F40(void)
{
    char version[64];
    void* buffer;
    int count;
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
    g_dword_68ec78 = -1;
    g_dword_68ed10 = -1;
    g_stack_68eda8 = CreateStack(5, 0x98);
    if (!g_stack_68eda8) {
        return 0;
    }
    if (!InitializeMenuStartupSubsystems()) {
        return 0;
    }
    SetStartupColorTransform(0x3f28f5c3);
    RebuildStartupColorTable();
    if (!InitializeMenuFonts()) {
        return 0;
    }
    InitializeMessageBoxState();
    InitializeRegionHelpState();
    if (g_flag_6850d4) {
        SetMessageBoxModeEnabled();
    } else {
        SetMessageBoxModeDisabled();
    }
    SetMessageBoxWord(g_word_6850ed);
    Function54AF30(0);
    Function54AFD0();
    Function55EF90();
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
    Function51B560();
    Function479010();
    Function55EC50(0);
    g_dword_686a70 = -1;
    if (gfLoadAtStartup && Function516740(g_save_slot_68ed28)) {
        g_flag_68ed14 = 1;
        g_dword_68ed18 = GetSaveGameLevel(g_save_slot_68ed28);
        Function55EC50(4);
    }
    InitializeEncounterTables();
    if (!Function4A5600()) {
        ShutdownWithErrorBox("Could not load missile database!");
    }
    if (!InitializeSpellDatabase()) {
        ShutdownWithErrorBox("Could not load spell database!");
    }
    if (!Function549B00()) {
        ShutdownWithErrorBox("Could not load hit sound database!");
    }
    memset(g_block_68f2d8, 0, sizeof(g_block_68f2d8));
    if (!Function48F940()) {
        return 0;
    }
    ok = (unsigned char)(0x4000000 < GetTotalPhysicalMemory());
    g_flag_65beaf = ok;
    return 1;
}

}
