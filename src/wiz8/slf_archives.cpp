#include "LibraryDataBase.h"

/* Wizardry's product-private WizLibs.c is not part of the released SGP tree.
   The executable does preserve its six startup records, while all archive
   parsing, handles and physical-file fallback remain owned by the released
   LibraryDataBase.c and FileMan.c implementations. */
extern "C" {

LibraryInitHeader gGameLibaries[6] = {
    {"Data\\Data.slf", 0, 1},
    {"Data\\Sound\\Sound.slf", 0, 1},
    {"Data\\Sound\\Monsters\\MonsterSound.slf", 0, 1},
    {"Data\\Music\\Music.slf", 0, 1},
    {"Data\\Monsters\\Monsters.slf", 0, 1},
    {"Levels\\Levels.slf", 0, 1}
};

extern unsigned char InitializeFileDatabase(void);

unsigned char InitializeSlfArchives(void)
{
    return InitializeFileDatabase();
}

/* Patch archives extend the fixed product catalog after startup.  Their exact
   first-party configuration remains separate from the released SGP database
   and is tracked by wiz8-ls5.5.5; the shipped six archives are sufficient for
   the normal main-menu path. */
int LoadPatchSlfArchives(const char*)
{
    return 0;
}

}
