#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/monster_generators.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/combat_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/notices.h"
#include "wiz8/chunk.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/save_game.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

/* GETFILESTRUCT is library layout and comes from the vendored SGP header rather
   than being restated: the 0x44-dword clear the body below opens with is exactly
   its 272 bytes. */
#include "FileMan.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/stLight.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/FormationAndFacing.h"

#include <windows.h>

#include <errno.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "wiz8/game_status.h"

/* Local Code\LoadSaveGame.cpp. The unit is established by its own assertions:
   evidence/observations/wiz8/assertions.csv places line 870 at 0x00512E80 and
   line 3507 at 0x00516580, with the line numbers rising with the address, so
   the bodies below sit inside the interval rather than being assigned to it by
   subsystem guesswork. */

/* The attribute word this gate tests is a Windows attribute word, so the two
   constants come from windows.h and are not restated here. Ghidra labels the
   pair with the vendored SFI release's SGP names, which number those bits
   differently; take the labels as belonging to that release rather than to this
   image. Read as Windows attributes the tests say "is a directory" and "is not
   read-only", which is what a function that verifies save directories asks. */

/* 0x004F8130, ItemManager.cpp line 998: asserts the item is non-null, then
   reports whether the flag word at +0x29 has any of the caller's bits set. The
   original spells the result through NEG/SBB/NEG, which is what VC6 emits for a
   bool conversion, so the return type is bool rather than the mask. */
/* 0x00659756: set to 1 by LoadLevel (0x0042A6F0) around its restore call at
   0x005135D0 and cleared immediately after, and read only from the save and
   load paths. It gates the bit-3 clear below. The meaning is not established
   beyond "a level restore is in progress", so the name stays positional. */

/* The object W8WorldItem::owner points at, and the entity it owns at +0x14.
   Method4B8890 is a 24-byte
   __thiscall getter with 18 call sites that hands back the entity's position;
   its body is not ported here, so the declaration stays unresolved at link like
   the other recovered callees. The canonical RET 4 fits an out-parameter and a
   12-byte by-value return equally, and both spellings compile to the same call
   site here, so the weaker of the two is the one declared. */
#define LOADSAVEGAME_CPP "C:\\Projects\\Wizardry 8\\Local Code\\LoadSaveGame.cpp"

/* 0x0050F6A0 and 0x0048C750, not yet identified; named by address as elsewhere
   in src/wiz8. The first is told about every group that survives the load, the
   second only about those two of its flags select. */

/* 0x004E3720, 0x004F69F0 and 0x00443A50, not yet identified; named by address
   as elsewhere in src/wiz8. All three take no argument and return nothing, and
   run before the header is read, so they read as teardown of whatever the
   previous level left behind. */

/* The fixed 0x314-byte header every save begins with. Only the fields
   LoadStatusHeader forwards are established; the rest is read and kept. */
struct W8StatusHeader {
    float version;                     /* 0x000 */
    int status_count_004;              /* 0x004: meaning not established */
    int next_monster_location_id;      /* 0x008 */
    int next_world_item_id;            /* 0x00c */
    int next_trigger_id;               /* 0x010 */
    unsigned char status_block[0x100]; /* 0x014 */
    unsigned char unknown_114[0x200];
}; /* 0x314 */

static_assert(sizeof(W8StatusHeader) == 0x314, "W8StatusHeader_must_be_0x314");

/* Established save-side callees without shared declarations yet. Their
   positional names preserve the current identity ceiling; the orchestration
   below establishes only their argument shape and section ownership. */

/* 0x005156C0, 0x00517A90 and 0x00518510, not yet identified; named by address
   as elsewhere in src/wiz8. The first loads a character from somewhere other
   than a loose file, the second builds the failure notice the third posts. */

/* FileWrite, FileExists, FileClearAttributes and FILE_IS_READONLY come from the
   vendored SGP FileMan.h already on this target's include path, so they are not
   restated here. */

/* 0x0068517C selects where characters live, and flags_2367 is a per-slot byte
   consulted only when it is set. The failure notice comes out of the shared
   notice array, and 0x00683678 is passed alongside; neither is established
   beyond that, so both keep positional names. */

/* Build the loose character/NPC path in the two forms used by the save code.
   The first accepts an already formatted filename or wildcard; the second
   appends the canonical CHR extension to a character's wide name first.  When
   characters are being supplied by an archive, a flagged slot or the external
   character sentinel keeps the caller's name unqualified. */
// FUNCTION: WIZ8 0x00514fa0
void BuildCharacterFilePath00514FA0(char* destination, const char* filename, int slot)
{
    char directory[260];

    if (!g_status_685170.game_started) {
        strcpy(directory, slot == -1 ? "Saves\\Characters" : "Saves\\NPCs");
        sprintf(destination, "%s\\%s", directory, filename);
        return;
    }
    if (slot != -1 && !g_status_685170.flags_2367[slot]) {
        sprintf(destination, "%s\\%s", "Saves\\NPCs", filename);
        return;
    }
    strcpy(destination, filename);
}

// FUNCTION: WIZ8 0x00514ec0
void BuildCharacterPath00514EC0(char* destination, const wchar_t* name, int slot)
{
    char filename[16];
    char directory[260];

    sprintf(filename, "%ls.%s", name, "CHR");
    if (!g_status_685170.game_started) {
        strcpy(directory, slot == -1 ? "Saves\\Characters" : "Saves\\NPCs");
    } else if (slot == -1 || g_status_685170.flags_2367[slot]) {
        strcpy(destination, filename);
        return;
    } else {
        strcpy(directory, "Saves\\NPCs");
    }
    sprintf(destination, "%s\\%s", directory, filename);
}

/* Loads one character record, either from a loose file under Saves\Characters
   or Saves\NPCs, or through 0x005156C0 when 0x0068517C says characters are not
   loose. The two spellings of the path share one sprintf: the branch that
   already has a directory literal jumps into the arm that formats one, which is
   what writing the call in both arms compiles to.
   The record is cleared before the read, and the read is two calls: a four-byte
   length and then that many bytes. A short or failed second read leaves the
   record cleared and reports failure, and the file is closed either way. */
// FUNCTION: WIZ8 0x005152b0
unsigned char LoadCharacter(const char* name, W8Character* character, int slot, char report_failure)
{
    char path[60];
    char directory[260];
    unsigned int size;
    unsigned int transferred;
    unsigned char loaded = 0;
    int handle;

    if (g_status_685170.game_started) {
        if (slot != -1 && g_status_685170.flags_2367[slot] == 0) {
            sprintf(path, "%s\\%s", "Saves\\NPCs", name);
        } else {
            strcpy(path, name);
        }
    } else {
        strcpy(directory, slot != -1 ? "Saves\\NPCs" : "Saves\\Characters");
        sprintf(path, "%s\\%s", directory, name);
    }

    if (g_status_685170.game_started && (slot == -1 || g_status_685170.flags_2367[slot] != 0)) {
        loaded = Function5156C0(path, character);
    } else {
        handle = FileOpen(path, 1, 0);
        if (handle == 0) {
            goto report;
        }
        memset(character, 0, sizeof(W8Character));
        if (FileRead(handle, &size, 4, &transferred) &&
            FileRead(handle, character, size, &transferred)) {
            loaded = 1;
        }
        FileClose(handle);
        /* The same read-only repair VerifyDataSubdirs makes, for the one errno
           that means exactly that. */
        if (_access(path, 2) != 0 && errno == EACCES) {
            _chmod(path, _S_IREAD | _S_IWRITE);
        }
    }
    if (loaded) {
        return loaded;
    }
report:
    if (report_failure) {
        wchar_t* notice = FormatWideString(gppStringList[W8_NOTICE_CHARACTER_LOAD_FAILED], name,
                                           g_small_font_683678, 1, 1, 0, 0);
        Function518510(notice);
    }
    return loaded;
}

// FUNCTION: WIZ8 0x00511df0
void FillCurrentSaveSlot(W8SaveSlot* slot)
{
    slot->name[0] = 0;
    slot->level_id = GetLoadedLevelID();
    slot->game_time_ms = g_status_685170.game_time_ms;
    slot->game_time_days = g_status_685170.game_time_days;
    slot->iron_man = g_status_685170.iron_man;
    GetLocalTime(&slot->timestamp);
    CaptureSaveScreenshot(&slot->screenshot);
    slot->version_major = 1;
    slot->version_minor = 2;
    slot->version_patch = 4;
}

// FUNCTION: WIZ8 0x00511e70
unsigned char EnumerateSaveSlots(W8GrowableVector<W8SaveSlot*>* slots)
{
    W8Chunk chunks;
    WIN32_FIND_DATAA find_data;
    char path[260];
    W8GlobalStatus status;

    sprintf(path, "%s\\*.%s", "Saves", "SAV");
    int first = slots->count;
    memset(&find_data, 0, sizeof(find_data));
    HANDLE search = FindFirstFileA(path, &find_data);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            sprintf(path, "%s\\%s", "Saves", find_data.cFileName);
            if (strcmp(path, "Saves\\CurrentGame.SAV") != 0 && strlen(find_data.cFileName) < 64 &&
                chunks.OpenRead(path)) {
                W8SaveSlot* slot = new W8SaveSlot;
                slot->screenshot.capture_result = 0;
                slot->version_major = 1;
                slot->version_minor = 0;
                slot->version_patch = 0;
                int count = chunks.ChunkCount();
                for (int index = 0; index < count; ++index) {
                    chunks.OpenChunk(0, 0);
                    if (!chunks.CurrentChunkAtEnd()) {
                        switch (chunks.CurrentChunkId()) {
                        case 0x41545347:
                            AllocateStatusBuffers(&status.buffers);
                            LoadGameStatus(&chunks, &status);
                            FreeStatusBuffers(&status.buffers);
                            slot->flag_263c = status.flag_49c1;
                            break;
                        case 0x52455647:
                            chunks.Read(&slot->version_major, 4, 0);
                            chunks.Read(&slot->version_minor, 4, 0);
                            chunks.Read(&slot->version_patch, 4, 0);
                            break;
                        case 0x544f4853:
                            chunks.Read(&slot->screenshot, 0x2588, 0);
                            break;
                        }
                    }
                    chunks.SkipCurrentChunk();
                    chunks.ReleaseCurrentChunk();
                }
                chunks.Close();
                if (status.flag_49bd == 0 && status.flag_49c0 == 0) {
                    char* extension = strrchr(find_data.cFileName, '.');
                    if (extension != 0) {
                        *extension = 0;
                    }
                    find_data.cFileName[63] = 0;
                    swprintf(slot->name, L"%hs", find_data.cFileName);
                    FileTimeToLocalFileTime(&find_data.ftLastWriteTime, &slot->local_write_time);
                    FileTimeToSystemTime(&slot->local_write_time, &slot->timestamp);
                    slot->level_id = status.current_level;
                    slot->game_time_ms = status.game_time_ms;
                    slot->iron_man = status.iron_man;
                    slot->game_time_days = status.game_time_days;
                    int position;
                    for (position = first; position < slots->count; ++position) {
                        if (CompareSGPFileTimes(&slot->local_write_time,
                                                &(*slots->GetAt(position))->local_write_time) > 0) {
                            break;
                        }
                    }
                    slots->InsertAt(position, slot);
                }
            }
        } while (FindNextFileA(search, &find_data));
    }
    FindClose(search);
    return 1;
}

/* Open one save slot and read only its game-status chunk. Startup needs the
   saved level before it commits to the full load, so every other top-level
   chunk is skipped and released without being materialized. */
// FUNCTION: WIZ8 0x00512290
int GetSaveGameLevel(const char* slot_name)
{
    W8Chunk chunks;
    char path[260];
    W8GlobalStatus status;
    int count;
    int index;

    sprintf(path, "%s\\%s.%s", "Saves", slot_name, "SAV");
    if (chunks.OpenRead(path)) {
        count = chunks.ChunkCount();
        for (index = 0; index < count; ++index) {
            chunks.OpenChunk(0, 0);
            if (!chunks.CurrentChunkAtEnd() && chunks.CurrentChunkId() == 0x41545347) {
                AllocateStatusBuffers(&status.buffers);
                LoadGameStatus(&chunks, &status);
                FreeStatusBuffers(&status.buffers);
                chunks.Close();
                return status.current_level;
            }
            chunks.SkipCurrentChunk();
            chunks.ReleaseCurrentChunk();
        }
        chunks.Close();
    }
    return 0;
}

/* Build the level-specific status path the save code falls back to when the
   current-game save has no matching level section. The regular levels use the
   database row's own folder and level names and level 56 is the shared default
   test level. The binary is explicit here (0x00512E80): CMP ESI,0x39 branches
   at level < 57, CMP ESI,0x38 handles 56, and every other level indexes the
   table at 0x00604478 with stride 0x6B. That table has 47 entries, so levels
   47-55 read the adjacent rdata, even though LevelBuildInfoByID treats those
   ten slots as test levels. The recovered units disagree exactly as retail
   does; no non-OOB branch exists at the call site. */
// FUNCTION: WIZ8 0x00512e80
void BuildLevelStatusPath(char* path, unsigned int level)
{
    W8LevelInfo info;

    if (!LevelBuildInfoByID(level, &info)) {
        srAssertFail("LevelFilesExist(ulLevel, &LevelName)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\LoadSaveGame.cpp", 870, 0);
    }
    *strchr(info.level_file_name, '.') = '\0';
    if (level < 57) {
        if (level == 56) {
            sprintf(path, "%s\\Test\\DefaultLevel.%s", "Levels", "STS");
        } else {
            sprintf(path, "%s\\%s\\%s.%s", "Levels", g_level_folders[level].folder_name,
                    g_level_folders[level].level_name, "STS");
        }
    } else {
        sprintf(path, "%s\\Test\\Level%c.%s", "Levels", level - 56, "STS");
    }
}

/* Reads and validates the header, then publishes the four counts and the block
   it carries. The version gate is an equality test against 2.0f held in .rdata,
   not a range, so a save written by any other version is refused outright.
   Each count is published first and only then corrected, rather than being
   tested before the store: the canonical writes all four globals, loads 1 once,
   and revisits each that turned out to be zero. */
// FUNCTION: WIZ8 0x00513090
unsigned char LoadStatusHeader(W8Chunk* chunk)
{
    unsigned int transferred;
    W8StatusHeader header;

    InitializeMonsterManagerState();
    InitializeItemManagerState();
    ResetNextTriggerId();
    if (!chunk->Read(&header, sizeof(header), &transferred)) {
        return 0;
    }
    if (header.version != 2.0f) {
        return 0;
    }
    g_status_685170.status_count_234a = header.status_count_004;
    g_status_685170.next_monster_location_id_234e = header.next_monster_location_id;
    g_status_685170.next_world_item_id_2352 = header.next_world_item_id;
    g_status_685170.next_trigger_id_2356 = header.next_trigger_id;
    if (header.status_count_004 == 0) {
        g_status_685170.status_count_234a = 1;
    }
    if (header.next_monster_location_id == 0) {
        g_status_685170.next_monster_location_id_234e = 1;
    }
    if (header.next_world_item_id == 0) {
        g_status_685170.next_world_item_id_2352 = 1;
    }
    if (header.next_trigger_id == 0) {
        g_status_685170.next_trigger_id_2356 = 1;
    }
    memcpy(g_status_685170.status_header_block_1904, header.status_block,
           sizeof(header.status_block));
    return 1;
}

/* Persist the current game status to one path. An existing current-game save
   that holds more than half its bytes in already-consumed level sections is
   first rolled into a CleanUp save and renamed into place; any other existing
   file is reopened for append. A fresh path is created outright. */
// FUNCTION: WIZ8 0x00513160
unsigned char SaveLevelStatus(const char* path)
{
    W8Chunk chunk;
    unsigned char opened;
    unsigned char result = 0;

    if (!chunk.OpenReadWrite(const_cast<char*>(path))) {
        opened = chunk.OpenWrite(const_cast<char*>(path));
    } else {
        unsigned int empty_percent;

        MeasureLevelStatusChunks00514DF0(&chunk, g_status_685170.current_level, &empty_percent);
        chunk.Close();
        if (empty_percent > 0x32 && _stricmp(path, "Saves\\CurrentGame.SAV") == 0) {
            SaveGame("CleanUp", 0);
            FileDelete("Saves\\CurrentGame.SAV");
            rename("Saves\\CleanUp.SAV", "Saves\\CurrentGame.SAV");
            return 0;
        }
        opened = chunk.OpenAppend(const_cast<char*>(path));
    }
    if (opened != 0) {
        result = SaveStatusHeader(&chunk);
        chunk.Close();
    }
    return result;
}

/* Serialize the complete per-level group. LVLS is a grouped chunk: its level
   id leads a sequence of ordinary child chunks. The restore path deliberately
   writes the four transient sections only; an ordinary save writes the live
   automation, trigger, prop, cube, generator, lock, ambient, particle and
   light sections. */
// FUNCTION: WIZ8 0x00513260
unsigned char SaveStatusHeader(W8Chunk* chunks)
{
    W8StatusHeader header;
    unsigned int count;
    unsigned int index;

    DestroyUngroupedMonsters();
    chunks->OpenChunk(0x534c564c, 0); /* LVLS */
    chunks->OpenGroup();
    chunks->Write(&g_status_685170.current_level, sizeof(g_status_685170.current_level), 0);

    chunks->OpenChunk(0x54415453, 0); /* STAT */
    memset(&header, 0, sizeof(header));
    header.version = 2.0f;
    header.status_count_004 = g_status_685170.status_count_234a;
    header.next_monster_location_id = g_status_685170.next_monster_location_id_234e;
    header.next_world_item_id = g_status_685170.next_world_item_id_2352;
    header.next_trigger_id = g_status_685170.next_trigger_id_2356;
    memcpy(header.status_block, g_status_685170.status_header_block_1904,
           sizeof(header.status_block));
    if (!chunks->Write(&header, sizeof(header), &count)) {
        chunks->ReleaseCurrentChunk();
    }
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x534e4f4d, 0); /* MONS */
    SaveMonsterStatus(chunks);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x4d455449, 0); /* ITEM */
    count = PLLength(gXStatus.plsItemList);
    chunks->Write(&count, sizeof(count), 0);
    for (index = 0; index < count; ++index) {
        if (!SaveItemFile(chunks->m_hFile, ItemInfo(index))) {
            chunks->ReleaseCurrentChunk();
            break;
        }
    }
    chunks->ReleaseCurrentChunk();

    if (g_flag_00659756) {
        chunks->OpenChunk(0x45425543, 0); /* CUBE */
        Function48EAD0(chunks->m_hFile);
        Function48E6D0(chunks->m_hFile);
        chunks->ReleaseCurrentChunk();

        chunks->OpenChunk(0x474e4f4d, 0); /* MONG */
        SaveEncounterState(chunks->m_hFile);
        chunks->ReleaseCurrentChunk();

        chunks->OpenChunk(0x4b434f4c, 0); /* LOCK */
        SaveTriggerRuntimeStates0043CB30(g_world, chunks->m_hFile, g_flag_00659756);
        chunks->ReleaseCurrentChunk();

        chunks->OpenChunk(0x53455254, 0); /* TRES */
        SaveTriggerActionData0043D120(g_world, chunks->m_hFile);
        chunks->ReleaseCurrentChunk();
        if (g_flag_00659756) {
            chunks->ReleaseGroup();
            chunks->ReleaseCurrentChunk();
            return 1;
        }
    }

    chunks->OpenChunk(0x4f545541, 0); /* AUTO */
    Function581CE0(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    if (g_world->triggers->count != 0) {
        chunks->OpenChunk(0x47495254, 0); /* TRIG */
        SaveWorldTriggers0043C810(g_world, chunks->m_hFile);
        chunks->ReleaseCurrentChunk();
    }

    chunks->OpenChunk(0x54535041, 0); /* APST */
    Function44E830(g_world, chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x53425543, 0); /* CUBS */
    Function48E6D0(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x534e474d, 0); /* MGNS */
    SaveMonsterGenerators(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x534b434c, 0); /* LCKS */
    SaveTriggerRuntimeStates0043CB30(g_world, chunks->m_hFile, g_flag_00659756);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x53424d41, 0); /* AMBS */
    SaveAmbientSoundList0047B140(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x54524150, 0); /* PART */
    SaveParticleStates0049B150(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x5448474c, 0); /* LGHT */
    Function49D120(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->ReleaseGroup();
    chunks->ReleaseCurrentChunk();
    return 1;
}

/* Open a per-level status file and hand it to the section reader. A file that
   cannot be opened reports failure without touching the live status. */
// FUNCTION: WIZ8 0x005135d0
unsigned char LoadLevelStatus(const char* path, int level)
{
    W8Chunk chunk;
    unsigned char result = 0;

    if (chunk.OpenRead(const_cast<char*>(path))) {
        result = LoadItemStatus(&chunk, level);
        chunk.Close();
    }
    return result;
}

/* Reads one saved monster group and files it under the species or the encounter
   list. The record's own size leads it, and the assertion that bounds it names
   the record: uiSize <= sizeof(*pMonsterGroup), at line 1517 of this unit.
   The size is read into the incoming parameter's stack slot. That is the same
   dead-slot reuse SaveFactState documents: the chunk pointer is already in a
   register by then, so its home slot is free, and the canonical spends exactly
   four bytes of locals for the encounter flag and nothing more.
   A record whose database entry is marked deleted is read and then dropped: it
   is neither listed nor given a monster list, and the function still reports
   success. */
// FUNCTION: WIZ8 0x00513c20
unsigned char LoadMonsterGroup(W8Chunk* chunk)
{
    /* The record size lands in the incoming parameter's own stack slot. The
       chunk pointer is copied into a register first, so the slot is dead
       scratch, and a separate local would cost four bytes of frame the
       canonical does not spend: it opens with a one-byte push, not a sub. The
       copy is taken after the record is cleared, not on entry, because the
       clear itself wants the register the pointer would otherwise be sitting
       in. */
    unsigned int* size = (unsigned int*)&chunk;
    W8MonsterGroup* group;
    W8MonsterRecord* record;
    W8Chunk* stream;
    int index;
    char is_encounter = 0;

    group = (W8MonsterGroup*)malloc(sizeof(W8MonsterGroup));
    if (group == 0) {
        return 0;
    }
    memset(group, 0, sizeof(W8MonsterGroup));
    stream = chunk;
    stream->Read(size, 4, 0);
    if (*size > sizeof(W8MonsterGroup)) {
        srAssertFail("uiSize <= sizeof(*pMonsterGroup)", LOADSAVEGAME_CPP, 0x5ed, 0);
    }
    stream->Read(group, *size, 0);
    if (group->version >= 2) {
        stream->Read(&is_encounter, 1, 0);
    }
    if (group->version < 3) {
        group->flag_ca = 0;
    }
    record = MonsterDBFromSpecies(group->monster_id);
    if (record == 0) {
        free(group);
        return 0;
    }
    if (record->deleted == 0) {
        group->monsters = ILCreate();
        if (group->monsters == 0) {
            free(group);
            return 0;
        }
        group->member_count = 0;
        group->active_member_count = 0;
        group->flag_28 = 0;
        group->flag_29 = 0;
        if (is_encounter) {
            index = PLAdoptAppend(gXStatus.plsMonsterGroupEncounterList, group);
        } else {
            index = PLAdoptAppend(gXStatus.plsMonsterGroupList, group);
        }
        if (index == -1) {
            free(group);
            return 0;
        }
        ActivateGroupMembers(group, 0);
        if (group->flag_c3 != 0 && group->leader_group_id == 0) {
            Function48C750(group);
        }
    }
    return 1;
}

/* Makes sure the three save directories exist and are writable before anything
   is written to them. The names are a table of fixed 60-byte slots terminated
   by an empty one rather than a count, which is why the walk asks strlen and
   not an index: the canonical steps a cursor by 0x3C and re-runs the inlined
   strlen at the bottom of the loop.
   The empty fourth slot is initialized from a string literal, not zeroed in
   place, so it is spelled as one here. */
// FUNCTION: WIZ8 0x00512d00
unsigned char VerifyDataSubdirs(void)
{
    char directories[4][60] = {"Saves", "Saves\\Characters", "Saves\\NPCs", ""};
    char* directory;
    unsigned int attributes;

    for (directory = directories[0]; strlen(directory) != 0; directory += 60) {
        if (!DirectoryExists(directory) && !MakeFileManDirectory(directory)) {
            return 0;
        }
        /* A read-only directory left behind by an earlier install is repaired
           rather than reported, but only for the one errno that means exactly
           that. */
        if (_access(directory, 2) != 0 && errno == EACCES) {
            _chmod(directory, _S_IREAD | _S_IWRITE);
        }
        attributes = FileGetAttributes(directory);
        if (attributes == 0xffffffff) {
            return 0;
        }
        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return 0;
        }
        if (attributes & FILE_ATTRIBUTE_READONLY) {
            return 0;
        }
    }
    return 1;
}

/* Walks the item's sibling chain and writes each record whole. Two reads go
   through the head of the chain instead of the item being written: the sector
   value copied into the current record is read from pItemInfo->pOwner, and the
   bit-3 clear lands on pItemInfo rather than pItem. The canonical holds the
   head in EDI for the whole loop and never reloads it, so this is the original
   source naming the parameter where it meant the cursor, not a scheduling
   artifact, and it is reproduced literally.
   As in SaveFactState, the bytes-written out-parameter is the address of the
   function's own second parameter: the head is already live in a register, so
   the incoming stack slot is dead and doubles as the scratch the callee
   requires. That is why the head is copied into a local at all -- reading the
   parameter directly costs a reload at every use, because taking its address
   keeps VC6 from enregistering it.
   The position is copied field by field rather than as a whole vector: a class
   assignment makes VC6 inline the generated operator=, which materializes the
   destination address into a register and costs two bytes the canonical does
   not spend. Written out, VC6 issues the three loads ahead of the three stores,
   which is the canonical encoding exactly.
   What is left is the epic's recurring register-role swap, and only in the
   entry pair: the canonical loads the head into EDI and copies EDI to ESI,
   while VC6 here loads ESI and copies ESI to EDI. Size, instruction count and
   every other encoding agree, and neither declaration order nor a guarded
   do-while moves it. */
// FUNCTION: WIZ8 0x00514be0
unsigned char SaveItemFile(int handle, W8WorldItem* item_info)
{
    W8WorldItem* first = item_info;
    W8WorldItem* item = first;

    while (item != 0) {
        item->saved_marker = 1;
        if (item->unknown_08 != 0) {
            srVector3T<float> position;
            item->owner->m_pRep->GetLocation004B8890(&position);
            item->position = position;
            item->entity_flags = static_cast<W8ItemRep*>(first->owner->m_pRep)->flags;
        }
        if (g_flag_00659756 != 0) {
            first->entity_flags &= ~8;
        }
        if (!FileWrite(handle, item, sizeof(W8WorldItem), (unsigned int*)&item_info)) {
            return 0;
        }
        item = item->next;
    }
    return 1;
}

/* Reads the same chain back. Each record carries its predecessor's next
   pointer as a file-resident flag: a non-null value only means another record
   follows, and the real link is rebuilt here. Every failure after the first
   allocation abandons the partial chain, which the original does too. */
// FUNCTION: WIZ8 0x00514c80
W8WorldItem* LoadItem(int handle, char add_to_list)
{
    W8WorldItem* previous = 0;
    W8WorldItem* first = 0;
    W8WorldItem* item;
    unsigned int done;

    item = (W8WorldItem*)malloc(sizeof(W8WorldItem));
    while (item != 0) {
        if (first == 0) {
            first = item;
        }
        if (!FileRead(handle, item, sizeof(W8WorldItem), &done)) {
            return 0;
        }
        item->sector_id = -2;
        item->unknown_08 = 0;
        item->owner = 0;
        if (ItemHasFlags(item, 1)) {
            RegisterSearchableWorldItem00516E20(item);
        }
        if (previous != 0) {
            previous->next = item;
        } else if (add_to_list && PLAdoptAppend(gXStatus.plsItemList, item) == -1) {
            return 0;
        }
        if (g_flag_00659756 != 0) {
            item->entity_flags &= ~8;
        }
        previous = item;
        if (item->next == 0) {
            return first;
        }
        item = (W8WorldItem*)malloc(sizeof(W8WorldItem));
    }
    return 0;
}

/* Reports whether any save exists other than the autosave. The main menu stores
   this and greys its second item out when it is clear, which is what makes the
   continue entry unavailable on a fresh install. */
// FUNCTION: WIZ8 0x00512fb0
unsigned char SaveGameExists(void)
{
    GETFILESTRUCT find;
    char path[260];
    unsigned char found;

    found = 1;
    memset(&find, 0, sizeof(find));
    sprintf(path, "%s\\%s", "Saves", "*.*");
    if (GetFileFirst(path, &find)) {
        sprintf(path, "%s%s", "Saves", find.zFileName);
        if (strcmp(path, "Saves\\CurrentGame.SAV") != 0) {
            goto done;
        }
        if (GetFileNext(&find)) {
            goto done;
        }
    }
    found = 0;

done:
    GetFileClose(&find);
    return found;
}

/* Writes one character record back to Saves\\Characters or Saves\\NPCs. The
   file name is the character's own wide name with a CHR extension, and the
   record is written as a four-byte length followed by that many bytes, which is
   the pair LoadCharacter reads back.
 
   The two directory spellings do not share a sprintf the way LoadCharacter's do:
   with characters loose, the NPC path is copied whole because the name already
   carries no directory, while the other two arms format one. An existing
   read-only file has its attribute cleared first, and a failure to clear it is
   treated exactly like a failure to open.
 
   Failure reporting has two shapes. With report_failure set the caller gets the
   save-failed notice and the continuation is dropped; without it the
   continuation runs instead. Either way the answer is failure. */
// FUNCTION: WIZ8 0x00515090
unsigned char SaveCharacter(W8Character* character, int slot, char report_failure,
                            void (*continuation)(void))
{
    char file_name[16];
    char path[260];
    char directory[260];
    unsigned char saved = 1;
    unsigned int size;
    unsigned int transferred;
    int handle;

    character->record_version = 1;
    sprintf(file_name, "%ls.%s", character->name, "CHR");
    if (g_status_685170.game_started == 0) {
        strcpy(directory, slot != -1 ? "Saves\\NPCs" : "Saves\\Characters");
        sprintf(path, "%s\\%s", directory, file_name);
    } else if (slot == -1 || g_status_685170.flags_2367[slot] != 0) {
        strcpy(path, file_name);
    } else {
        sprintf(path, "%s\\%s", "Saves\\NPCs", file_name);
    }

    if (g_status_685170.game_started == 0) {
        if (FileExists(path) && (FileGetAttributes(path) & FILE_IS_READONLY) != 0 &&
            FileClearAttributes(path) == 0) {
            goto report;
        }
        handle = FileOpen(path, 0x22, 0);
        if (handle == 0) {
            goto report;
        }
        size = sizeof(W8Character);
        if (FileWrite(handle, &size, 4, &transferred) == 0 ||
            FileWrite(handle, character, sizeof(W8Character), &transferred) == 0) {
            saved = 0;
        }
        FileClose(handle);
    } else {
        saved = Function5155B0(path, slot, character);
    }
    if (saved) {
        return saved;
    }
report:
    if (report_failure) {
        wchar_t* notice =
            FormatWideString(gppStringList[W8_NOTICE_CHARACTER_SAVE_FAILED], character->name,
                             g_small_font_683678, 1, 1, 0, continuation);
        Function518510(notice);
        return 0;
    }
    if (continuation != 0) {
        continuation();
    }
    return 0;
}

/* Save-slot bookkeeping from the same established
   Local Code\LoadSaveGame.cpp translation unit. */

/* The two chunk tags the walk recognises, as the four-character codes the
   comparison spells them. */
enum { W8_SAVE_TAG_CHAR = 0x52414843, W8_SAVE_TAG_LVLS = 0x534c564c };

// GLOBAL: WIZ8 0x00689f98
unsigned char g_save_pending_00689f98;

// GLOBAL: WIZ8 0x0068506b
unsigned char g_save_notice_shown_0068506b;
/* 0x0061A144, the save-file extension. It sits in writable .data with 16
   reference sites across 10 functions rather than in .rdata with the format
   literals, so it is a mutable character array rather than a string literal;
   this build initialises it to "SAV". */
// GLOBAL: WIZ8 0x0061A144
char g_save_extension[] = "SAV";

/* Delete both files a current game occupies: the slot the current save name
   selects, and the fixed CurrentGame file. Each delete is preceded by the same
   read-only repair the rest of this unit makes - EACCES from _access is the one
   errno that means the file is there but not writable. */
// FUNCTION: WIZ8 0x00515920
void DeleteCurrentSaveFiles(void)
{
    char path[260];

    sprintf(path, "%s\\%s.%s", "Saves", ConvertWideStringToString(GetLastSaveName()),
            g_save_extension);
    if (_access(path, 2) != 0 && errno == EACCES) {
        _chmod(path, _S_IREAD | _S_IWRITE);
    }
    FileDelete(path);
    if (_access("Saves\\CurrentGame.SAV", 2) != 0 && errno == EACCES) {
        _chmod("Saves\\CurrentGame.SAV", _S_IREAD | _S_IWRITE);
    }
    FileDelete("Saves\\CurrentGame.SAV");
}

/* Two gates with no established meaning beyond their position in the chain, so
   both keep positional names. Both are zero in the shipped image. */

/* gXStatus.fCombatMode and gXStatus.fCampMode reach this unit through
   xstatus.h. */
/* Byte-sized, not int: the refusal below returns through `mov al,1` and the
   save arm returns this result unchanged, so both share one byte register. */

/* Autosave, if every gate allows it. Declining is reported as success, which is
   why the whole chain is one condition with a single trailing `return 1` rather
   than a run of early returns: the canonical has one epilogue for the refusal
   and one for the save. The chain breaks around each call because a call cannot
   be hoisted into a short-circuit, which is what the decompiler's nesting is.

   g_status_685170.iron_man does double duty: it both admits a save that the
   0x0068510d gate would otherwise refuse for a forced call, and selects the
   name, so a save made under it overwrites the current slot instead of the
   fixed AutoSave one. */
// FUNCTION: WIZ8 0x005159e0
unsigned char AutoSaveIfAllowed(char forced)
{
    char name[64];

    g_save_notice_shown_0068506b = 0;
    if (g_status_685170.value_2435 == 0 && AnyMonsterDying() == 0 &&
        ((g_settings_6850c8.auto_save != 0 && forced == 0) || g_status_685170.iron_man != 0) &&
        gXStatus.fCombatMode == 0 && IsSightRangeOverridden() == 0 &&
        (char)IsLevelDataFlag4EffectivelySet() != 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fCampMode == 0) {
        /* The copy is written out in both arms rather than selecting the source
           into one call. VC6 tail-merges the two inlined copies but keeps each
           arm's own destination `lea` and source load, which is the canonical
           encoding; funnelling both arms through one pointer costs the extra
           move that a selected argument needs. */
        if (g_status_685170.iron_man != 0) {
            strcpy(name, ConvertWideStringToString(GetLastSaveName()));
        } else {
            strcpy(name, "AutoSave");
        }
        return SaveGame(name, 0);
    }
    return 1;
}

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
// FUNCTION: WIZ8 0x00512f70
unsigned char SaveSlotFileExists(const char* slot_name)
{
    char path[260];

    sprintf(path, "%s%s%s", "Saves", slot_name, ".SAV");
    return FileExists(path);
}

/* Note that the save could not be written. The notice is only shown on the
   screen that owns saving, but the flag is raised either way. */
// FUNCTION: WIZ8 0x00515ac0
void ReportSaveFailed(char quiet)
{
    if (quiet == 0 || g_status_685170.iron_man != 0) {
        g_save_notice_shown_0068506b = 1;
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            ShowNotice(0xc, gppStringList[0x1e0c / 4], -1, -1, 0);
        }
    }
}

/* Choose the numbered quick-save slot that the next quick save should write.
   A missing slot wins immediately; when all three exist, replace the one with
   the oldest modification time. */
// FUNCTION: WIZ8 0x00516670
unsigned char SelectQuickSaveSlotForWrite(char* slot_name)
{
    SGP_FILETIME creation_time;
    SGP_FILETIME access_time;
    SGP_FILETIME write_time;
    SGP_FILETIME oldest_write_time;
    int oldest_slot = 1;
    int slot;
    int handle;

    for (slot = 1; slot <= 3; ++slot) {
        sprintf(slot_name, "%s\\%s %d.%s", "Saves", "Quick", slot, "SAV");
        handle = FileOpen(slot_name, 1, 0);
        if (!handle) {
            goto format_slot;
        }
        GetFileManFileTime(handle, &creation_time, &access_time, &write_time);
        FileClose(handle);
        if (slot > 1) {
            if (CompareSGPFileTimes(&write_time, &oldest_write_time) < 0) {
                oldest_write_time = write_time;
                oldest_slot = slot;
            }
        } else {
            oldest_write_time = write_time;
        }
    }
    slot = oldest_slot;
format_slot:
    sprintf(slot_name, "%s %d", "Quick", slot);
    return 1;
}

/* Select the newest numbered quick save for command-line startup. The three
   candidates are real save files named Quick 1 through Quick 3; the unnumbered
   Quick slot is accepted only when none of those files exists. */
// FUNCTION: WIZ8 0x00516740
unsigned char FindStartupQuickSave(char* slot_name)
{
    int newest_slot = 0;
    SGP_FILETIME creation_time;
    SGP_FILETIME access_time;
    SGP_FILETIME write_time;
    SGP_FILETIME newest_write_time;
    char path[260];
    int slot;
    int handle;

    for (slot = 1; slot <= 3; ++slot) {
        sprintf(slot_name, "%s\\%s %d.%s", "Saves", "Quick", slot, "SAV");
        handle = FileOpen(slot_name, 1, 0);
        if (handle) {
            GetFileManFileTime(handle, &creation_time, &access_time, &write_time);
            FileClose(handle);
            if (slot > 1) {
                if (CompareSGPFileTimes(&write_time, &newest_write_time) <= 0) {
                    continue;
                }
                newest_write_time = write_time;
            } else {
                newest_write_time = write_time;
            }
            newest_slot = slot;
        }
    }
    if (newest_slot > 0) {
        sprintf(slot_name, "%s %d", "Quick", newest_slot);
        return 1;
    }
    sprintf(path, "%s\\%s.%s", "Saves", "Quick", "SAV");
    if (FileExists(path)) {
        strcpy(slot_name, "Quick");
        return 1;
    }
    return 0;
}

/* Walk every chunk of a saved game. Character chunks are read straight in; a
   level chunk is read only for the level the party is actually on, and one for
   any other level is rewound and read as a character chunk instead. */
// FUNCTION: WIZ8 0x00514d50
void ReadSaveChunks(W8Chunk* source, W8Chunk* destination)
{
    int remaining = source->ChunkCount();

    if (remaining > 0) {
        int level;
        unsigned int tag;

        do {
            source->OpenChunk(0, 0);
            if (!source->CurrentChunkAtEnd()) {
                tag = source->CurrentChunkId();
                if (tag == W8_SAVE_TAG_CHAR) {
                    destination->CopyCurrentChunkFrom(source);
                } else if (tag == W8_SAVE_TAG_LVLS) {
                    source->Read(&level, 4, 0);
                    if (level != g_status_685170.current_level) {
                        source->RewindCurrentChunk();
                        destination->CopyCurrentChunkFrom(source);
                    }
                }
            }
            source->SkipCurrentChunk();
            source->ReleaseCurrentChunk();
            --remaining;
        } while (remaining != 0);
    }
}

/* Walk every top-level chunk of an already-open save. A matching LVLS section
   is marked consumed in place, and the caller receives the percentage of the
   file that sits in at-end sections, which is what decides whether the save
   is rolled into CleanUp. Retail wraps the percentage in an unguarded DIV, so
   a zero total would trap there as well. */
// FUNCTION: WIZ8 0x00514df0
unsigned char MeasureLevelStatusChunks00514DF0(W8Chunk* chunk, int level,
                                               unsigned int* empty_percent)
{
    unsigned char found = 0;
    unsigned int total = 0;
    unsigned int empty_total = 0;
    int remaining = chunk->ChunkCount();

    if (remaining > 0) {
        do {
            chunk->OpenChunk(0, 0);
            total += chunk->CurrentChunkExtent();
            if (chunk->CurrentChunkAtEnd() != 0) {
                empty_total += chunk->CurrentChunkExtent();
            } else if (chunk->CurrentChunkId() == 0x534c564c) { /* LVLS */
                int stored_level;

                chunk->OpenGroup();
                chunk->Read(&stored_level, 4, 0);
                if (stored_level == level) {
                    found = 1;
                    chunk->SetCurrentChunkAtEnd();
                }
                chunk->SkipCurrentChunk();
                chunk->ReleaseGroup();
            }
            chunk->SkipCurrentChunk();
            chunk->ReleaseCurrentChunk();
            --remaining;
        } while (remaining != 0);
    }
    if (empty_percent != 0) {
        /* The binary (0x00514DF0) divides by the accumulated extent with no
           zero test; an empty chunk file reaches this unsigned DIV. */
        *empty_percent = empty_total * 100 / total;
    }
    return found;
}

/* Read the complete GSTA payload and its two eight-record collections. The
   pointers at the head of the fixed block are process ownership, so they are
   preserved across the serialized read. Old status blocks get the one retail
   compatibility migration retained by this build. Character records repair
   the pre-v2 profession field, while live global party rows rebuild or clear
   every transient pointer rather than trusting saved addresses. */
// FUNCTION: WIZ8 0x00515cf0
void LoadGameStatus(W8Chunk* chunks, W8GlobalStatus* status)
{
    W8Character* characters = status->buffers.characters;
    W8PartySlotRow* party_rows = status->buffers.party_rows;
    unsigned int size;
    unsigned int slot;

    if (characters == 0) {
        srAssertFail("pStatus->Char != NULL", LOADSAVEGAME_CPP, 0xcc6, 0);
    }
    if (party_rows == 0) {
        srAssertFail("pStatus->XChar != NULL", LOADSAVEGAME_CPP, 0xcc7, 0);
    }

    memset(status, 0, sizeof(*status));
    chunks->Read(&size, sizeof(size), 0);
    if (size > sizeof(*status)) {
        srAssertFail("uiSize <= sizeof(*pStatus)", LOADSAVEGAME_CPP, 0xccd, 0);
    }
    chunks->Read(status, size, 0);

    if (status->buffers.save_version < 1.1f) {
        for (slot = 0; slot != 3; ++slot) {
            status->text_box_lines_used_4997[slot] = status->legacy_text_box_lines_1797[0][slot];
            status->text_box_lines_shown_49a7[slot] = status->legacy_text_box_lines_1797[1][slot];
        }
        status->text_box_lines_used_4997[3] = 0;
        status->text_box_lines_shown_49a7[3] = 0;
    }

    status->buffers.characters = characters;
    status->buffers.party_rows = party_rows;

    W8Character* character = characters;
    for (slot = 0; slot != 8; ++slot, ++character) {
        memset(character, 0, sizeof(*character));
        chunks->Read(&size, sizeof(size), 0);
        if (size > sizeof(*character)) {
            srAssertFail("uiSize <= sizeof(*&pStatus->Char[uiChar])", LOADSAVEGAME_CPP, 0xce4, 0);
        }
        chunks->Read(character, size, 0);
        if (character->record_version < 2 && character->original_profession == 0 &&
            character->profession_levels[0] == 0) {
            character->original_profession = character->current_profession;
        }
    }

    W8PartySlotRow* party_row = party_rows;
    for (slot = 0; slot != 8; ++slot, ++party_row) {
        memset(party_row, 0, sizeof(W8PartySlotRow));
        chunks->Read(&size, sizeof(size), 0);
        if (size > sizeof(W8PartySlotRow)) {
            srAssertFail("uiSize <= sizeof(*&pStatus->XChar[uiChar])", LOADSAVEGAME_CPP, 0xcf2, 0);
        }
        chunks->Read(party_row, size, 0);

        if (status == &g_status_685170) {
            W8ItemInstance* item = 0;
            signed char origin = static_cast<signed char>(party_row->item_origin);
            short item_slot = static_cast<short>(party_row->item_slot);
            if (party_row->occupied != 0 && party_row->pending_action == 8 && origin != -1 &&
                item_slot != -1) {
                item = FindCharacterItemAt(slot, static_cast<unsigned char>(origin),
                                           static_cast<unsigned short>(item_slot));
            }
            party_row->pending_action_detail_015.item_use.item = item;
            party_row->action_detail_045.item_use.item = 0;
            party_row->spell_target.pPCItem = 0;
            party_row->item_target.pPCItem = 0;
            party_row->target_context_5.pPCItem = 0;
        }
    }
    RebuildPartyStatus00555FA0(&status->formation);
}

/* Write the global status as one GSTA chunk. The two pointed-to collections
   follow the fixed status object in record-sized pieces so each record remains
   an independently sized save field. */
// FUNCTION: WIZ8 0x00515fa0
void SaveGlobalStatus(W8Chunk* chunks, W8GlobalStatus* status)
{
    unsigned int size;
    unsigned int slot;

    chunks->OpenChunk(0x41545347, 0);
    size = sizeof(*status);
    chunks->Write(&size, sizeof(size), 0);
    chunks->Write(status, size, 0);
    for (slot = 0; slot != 8; ++slot) {
        size = sizeof(W8Character);
        chunks->Write(&size, sizeof(size), 0);
        chunks->Write(&status->buffers.characters[slot], size, 0);
    }
    for (slot = 0; slot != 8; ++slot) {
        size = sizeof(W8PartySlotRow);
        chunks->Write(&size, sizeof(size), 0);
        chunks->Write(&status->buffers.party_rows[slot], size, 0);
    }
    chunks->ReleaseCurrentChunk();
}
