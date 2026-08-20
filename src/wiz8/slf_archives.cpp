/* LibraryDataBase.h repeats this product-owned declaration outside its own
   C-linkage block. Hide that duplicate while importing the released SGP types;
   the canonical Wizardry declaration below carries the actual ABI. */
#define gGameLibaries gGameLibariesReleasedHeaderDeclaration
#include "LibraryDataBase.h"
#undef gGameLibaries
#include "MemMan.h"
#include "wiz8/sgp-compat/WizLibs.h"

#include <stdio.h>
#include <string.h>

/* Wizardry's product-private WizLibs.c extends the released SGP catalog with
   one map flag per configuration, a 56-slot archive-state table, and dynamic
   Patch.000 through Patch.049 records.  Archive parsing and file handles stay
   in the released LibraryDataBase.c and FileMan.c implementations. */
extern "C" {

static_assert(sizeof(LibraryInitHeader) == 0x103,
              "Wizardry SLF configuration record must be 0x103 bytes");
static_assert(sizeof(LibraryHeaderStruct) == 0x28,
              "Wizardry SLF archive state must be 0x28 bytes");

LibraryInitHeader gGameLibaries[MAX_NUMBER_OF_LIBRARIES] = {
    {"Data\\Data.slf", 1, 1, 0},
    {"Data\\Sound\\Sound.slf", 1, 1, 0},
    {"Data\\Sound\\Monsters\\MonsterSound.slf", 1, 1, 0},
    {"Data\\Music\\Music.slf", 1, 1, 0},
    {"Data\\Monsters\\Monsters.slf", 1, 1, 0},
    {"Levels\\Levels.slf", 1, 1, 0}
};

static void MapSlfArchive(int library_id)
{
    LibraryHeaderStruct* library = &gFileDataBase.pLibraries[library_id];
    if (!gGameLibaries[library_id].fMapFile || !library->fLibraryOpen) {
        return;
    }

    HANDLE mapping = CreateFileMappingA(
        library->hLibraryHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping != NULL) {
        PTR view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        if (view != NULL) {
            library->hFileMapping = mapping;
            library->pFileMapping = view;
        }
    }
}

// FUNCTION: WIZ8 0x004126F0
unsigned char InitializeSlfArchives(void)
{
    int library_id;
    unsigned int size;
    unsigned char library_initialized = 0;

    gzCdDirectory[0] = '.';
    gzCdDirectory[1] = '\0';
    gFileDataBase.usNumberOfLibraries = NUMBER_OF_LIBRARIES;

    size = MAX_NUMBER_OF_LIBRARIES * sizeof(LibraryHeaderStruct);
    gFileDataBase.pLibraries = (LibraryHeaderStruct*)MemAlloc(size);
    if (gFileDataBase.pLibraries == NULL) {
        return 0;
    }
    memset(gFileDataBase.pLibraries, 0, size);

    for (library_id = 0; library_id < NUMBER_OF_LIBRARIES; ++library_id) {
        if (gGameLibaries[library_id].fInitOnStart) {
            if (OpenLibrary((short)library_id)) {
                MapSlfArchive(library_id);
                library_initialized = 1;
            } else {
                gFileDataBase.pLibraries[library_id].fLibraryOpen = 0;
            }
        }
    }
    gFileDataBase.fInitialized = library_initialized;

    size = INITIAL_NUM_HANDLES * sizeof(RealFileOpenStruct);
    gFileDataBase.RealFiles.pRealFilesOpen =
        (RealFileOpenStruct*)MemAlloc(size);
    if (gFileDataBase.RealFiles.pRealFilesOpen == NULL) {
        return 0;
    }
    memset(gFileDataBase.RealFiles.pRealFilesOpen, 0, size);
    gFileDataBase.RealFiles.iSizeOfOpenFileArray = INITIAL_NUM_HANDLES;
    return 1;
}

// FUNCTION: WIZ8 0x00412870
int LoadPatchSlfArchives(const char* directory)
{
    char patch_path[260];
    int patch_number;
    int loaded = 0;

    for (patch_number = 0; patch_number < 50; ++patch_number) {
        sprintf(patch_path, "%s\\Patch.%3.3d", directory, patch_number);
        if (FileExistsNoDB(patch_path)) {
            int library_id = gFileDataBase.usNumberOfLibraries;
            strcpy(gGameLibaries[library_id].sLibraryName, patch_path);
            ++gFileDataBase.usNumberOfLibraries;
            gGameLibaries[library_id].fOnCDrom = 0;
            gGameLibaries[library_id].fInitOnStart = 0;
            gGameLibaries[library_id].fMapFile = 0;

            if (OpenLibrary((short)library_id)) {
                MapSlfArchive(library_id);
                ++loaded;
                gFileDataBase.pLibraries[library_id].fPatchLibrary = 1;
                if (!gFileDataBase.fInitialized) {
                    gFileDataBase.fInitialized = 1;
                }
            } else {
                --gFileDataBase.usNumberOfLibraries;
            }
        }
    }
    return loaded;
}

}
