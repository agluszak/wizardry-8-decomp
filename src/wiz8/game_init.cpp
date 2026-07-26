#include "wiz8/gameplay_boundaries.h"

#include <string.h>

/*
 * The data bring-up gate BringUpEngine calls last. It stamps the version
 * string, opens the archives and string data, then walks every database
 * loader in turn, abandoning the sequence the moment one fails. Its callees
 * are mostly unidentified and carry address-derived names; the ones already
 * recovered elsewhere keep theirs.
 */

extern "C" {

/* A formatter returning its result; used here with three different arities. */
extern char* Function517A70(const char* format, ...);
extern void* CreateStack(int count, int size);

extern void InitializeSlfArchives(void);
extern void Function412870(const char* directory);
extern void Function518360(const char* path);
extern void* Function421F70(int* count);
extern void Function421FB0(void);
extern void Function54B810(void);
extern void Function413E80(int value);
extern void Function413D60(void);
extern unsigned char Function4E27A0(void);
extern void Function40CF60(void);
extern void Function4F11D0(void);
extern void Function40C210(void);
extern void Function40C200(void);
extern void Function40C1F0(unsigned short value);
extern void Function54AF30(int value);
extern void Function54AFD0(void);
extern void Function55EF90(void);
extern unsigned char VerifyDataSubdirs(void);
extern unsigned char Function54A760(int value);
extern void Function51B560(void);
extern void Function479010(void);
extern void Function55EC50(int value);
extern unsigned char Function516740(void* slot);
extern int GetSaveGameLevel(void* slot);
extern void InitializeEncounterTables(void);
extern unsigned char Function4A5600(void);
extern unsigned char InitializeSpellDatabase(void);
extern unsigned char Function549B00(void);
extern void Function401920(const char* message);
extern unsigned char Function48F940(void);
extern unsigned int Function428E60(void);

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
/* Five-dword records; slot zero of each is an initialiser returning success. */
extern unsigned char (*g_init_table_647bc8[])(void);
extern unsigned char g_init_table_end_647ccc[];

// FUNCTION: WIZ8 0x004E2F40
unsigned char Function4E2F40(void)
{
    char version[64];
    unsigned char (**initialiser)(void);
    void* buffer;
    int count;
    unsigned char ok;

    version[0] = '\0';
    strcat(version, "Wizardry 8 ");
    strcat(version, Function517A70("v%d.%d.%d", 1, 2, 4));
    strcat(version, Function517A70(" (build %d)", 0xdb));
    strcat(version, Function517A70(" %s", "2001/12/24 15:36"));
    InitializeSlfArchives();
    Function412870("Patches");
    Function518360(gzStringDataOverride ? gzStringDataOverride : "Data\\Strings\\StringData.DAT");
    buffer = Function421F70(&count);
    memset(buffer, 0, count * 0x1e0);
    Function421FB0();
    Function54B810();
    g_dword_68ec78 = -1;
    g_dword_68ed10 = -1;
    g_stack_68eda8 = CreateStack(5, 0x98);
    if (!g_stack_68eda8) {
        return 0;
    }
    initialiser = g_init_table_647bc8;
    do {
        if (!(*initialiser)()) {
            return 0;
        }
        initialiser += 5;
    } while (initialiser < (unsigned char (**)(void))g_init_table_end_647ccc);
    Function413E80(0x3f28f5c3);
    Function413D60();
    if (!Function4E27A0()) {
        return 0;
    }
    Function40CF60();
    Function4F11D0();
    if (g_flag_6850d4) {
        Function40C200();
    } else {
        Function40C210();
    }
    Function40C1F0(g_word_6850ed);
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
        Function401920("Could not load missile database!");
    }
    if (!InitializeSpellDatabase()) {
        Function401920("Could not load spell database!");
    }
    if (!Function549B00()) {
        Function401920("Could not load hit sound database!");
    }
    memset(g_block_68f2d8, 0, sizeof(g_block_68f2d8));
    if (!Function48F940()) {
        return 0;
    }
    ok = (unsigned char)(0x4000000 < Function428E60());
    g_flag_65beaf = ok;
    return 1;
}

}
