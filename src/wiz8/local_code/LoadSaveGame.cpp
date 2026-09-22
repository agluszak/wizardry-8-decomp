#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/monster_generators.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/notices.h"
#include "wiz8/chunk.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/sr_api.h"
#include "wiz8/save_game.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

/* GETFILESTRUCT is library layout and comes from the vendored SGP header rather
   than being restated: the 0x44-dword clear the body below opens with is exactly
   its 272 bytes. */
#include "FileMan.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/Video2.h"
#include "surrender/srColorSurface.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Video2.h"
#include "surrender/srColorSurface.h"
#include "wiz8/location_variables.h"
#include "wiz8/fact_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/engine_code/stScript.h"
#include "surrender/srTypeRegistry.h"

#include "timer.h"

#include <windows.h>

#include <errno.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/npc_interaction.h"
#include "soundman.h"
#include "timer.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"

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
    int next_group_id;                 /* 0x004 */
    int next_monster_location_id;      /* 0x008 */
    int next_world_item_id;            /* 0x00c */
    int next_trigger_id;               /* 0x010 */
    unsigned char status_block[0x100]; /* 0x014 */
    unsigned char unknown_114[0x200];
}; /* 0x314 */

static_assert(sizeof(W8StatusHeader) == 0x314, "W8StatusHeader_must_be_0x314");

/* Same-unit bodies SaveGame reaches before their definitions. */
void ReadSaveChunks(W8Chunk* source, W8Chunk* destination);
void SaveGlobalStatus(W8Chunk* chunks, W8GlobalStatus* status);

unsigned char SaveMonsterRecord005147A0(W8Chunk* chunks, unsigned int index);

/* 0x0061A134/0x0061A138: the two XOR masks SaveGame applies to the file's
   creation-time pair before it lands in the status block. */
// GLOBAL: WIZ8 0x0061A134
unsigned int g_save_filetime_xor_low_0061a134 = 0x6b24e9f0;
// GLOBAL: WIZ8 0x0061A138
unsigned int g_save_filetime_xor_high_0061a138 = 0xe77c28c1;

/* Established save-side callees without shared declarations yet. Their
   positional names preserve the current identity ceiling; the orchestration
   below establishes only their argument shape and section ownership. */

/* 0x00517A90, not yet identified; named by address as elsewhere in src/wiz8.
   It builds the failure notice CreateMessageBox posts. */

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
   or Saves\NPCs, or through LoadCharacterFromCurrentGame when 0x0068517C says
   characters are not loose. The two spellings of the path share one sprintf:
   the branch that already has a directory literal jumps into the arm that
   formats one, which is what writing the call in both arms compiles to.
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
    bool loaded = false;
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
        loaded = LoadCharacterFromCurrentGame(path, character) != 0;
    } else {
        handle = FileOpen(path, 1, 0);
        if (handle == 0) {
            goto report;
        }
        memset(character, 0, sizeof(W8Character));
        if (FileRead(handle, &size, 4, &transferred) &&
            FileRead(handle, character, size, &transferred)) {
            loaded = true;
        }
        FileClose(handle);
        /* The same read-only repair VerifyDataSubdirs makes, for the one errno
           that means exactly that. */
        if (_access(path, 2) != 0 && errno == EACCES) {
            _chmod(path, _S_IREAD | _S_IWRITE);
        }
    }
    if (loaded) {
        return 1;
    }
report:
    if (report_failure) {
        CreateMessageBox(FormatWideString(gppStringList[W8_NOTICE_CHARACTER_LOAD_FAILED], name),
                         g_small_font_683678, 1, 1, 0, 0);
    }
    return loaded ? 1 : 0;
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

/* Write one whole save file: the live status block, the version triple, the
   SHOT screenshot (rendered through an offscreen surface when the caller did
   not supply one), then the TEXT, TVAR, NPCI, NPCT, NPCF, FATA, JRNL and HYPN
   sections, and the header last. Before any of that the live state is folded
   into the status block: the camera position goes to pending_move_location,
   every shown message line snapshots whether its countdown was still ticking,
   the gameplay timer restarts, ungrouped monsters are destroyed, the master
   functions save, and an iron-man game additionally stores the file's creation
   time XOR-masked by the two data constants. A save of anything but
   CurrentGame first absorbs the loose character chunks from
   Saves\CurrentGame.SAV through ReadSaveChunks. The NPCI section is the only
   writer whose failure aborts the whole save; a failed header write leaves the
   file open and returns zero. */
// FUNCTION: WIZ8 0x005123F0
unsigned char SaveGame(const char* name, W8SaveScreenshot* screenshot)
{
    W8Chunk chunks;
    W8Chunk current_game;
    W8ScreenRect bounds;
    SGP_FILETIME creation_time;
    SGP_FILETIME access_time;
    SGP_FILETIME write_time;
    srColorSurface* surface;
    char path[260];
    unsigned char generated;
    short cursor;
    int saved;
    int version_major;
    int version_minor;
    int version_patch;
    int region;
    unsigned int index;

    sprintf(path, "%s\\%s.%s", "Saves", name, g_save_extension);
    if (_access(path, 2) != 0 && errno == EACCES) {
        _chmod(path, _S_IREAD | _S_IWRITE);
    }
    if (chunks.OpenWrite(path) == 0) {
        return 0;
    }
    if (_stricmp(name, "CurrentGame") != 0 &&
        current_game.OpenRead("Saves\\CurrentGame.SAV") != 0) {
        ReadSaveChunks(&current_game, &chunks);
        current_game.Close();
    }
    GetWorldCameraState(GetWorld(), &g_status_685170.pending_move_location);
    for (region = 0; region != 4; ++region) {
        for (index = 0; index < g_status_685170.text_box_lines_shown_49a7[region]; ++index) {
            g_message_storage_68f2d8[region][index].clock_ticking_0c =
                ClockIsTicking(g_message_storage_68f2d8[region][index].clock_08);
        }
    }
    gXStatus.gameplay_timer->Restart();
    DestroyUngroupedMonsters();
    SaveMasterFunctions004D8EC0();
    g_status_685170.buffers.save_version = 1.1f;
    g_status_685170.difficulty = g_settings_6850c8.difficulty;
    if (g_status_685170.iron_man != 0) {
        GetFileManFileTime(chunks.m_hFile, &creation_time, &access_time, &write_time);
        g_status_685170.save_filetime_xor_244b[0] =
            creation_time.dwLowDateTime ^ g_save_filetime_xor_low_0061a134;
        g_status_685170.save_filetime_xor_244b[1] =
            creation_time.dwHighDateTime ^ g_save_filetime_xor_high_0061a138;
    }
    cursor = g_status_685170.text_line_cursor_1795;
    if (cursor == 2) {
        saved = 2;
        cursor = 2;
        g_status_685170.text_line_cursor_1795 = 0;
        static_cast<void>(saved);
    }
    SaveGlobalStatus(&chunks, &g_status_685170);
    g_status_685170.text_line_cursor_1795 = cursor;
    chunks.OpenChunk(0x52455647, 0); /* GVER */
    version_major = 1;
    version_minor = 2;
    version_patch = 4;
    chunks.Write(&version_major, 4, 0);
    chunks.Write(&version_minor, 4, 0);
    chunks.Write(&version_patch, 4, 0);
    chunks.ReleaseCurrentChunk();
    generated = screenshot == 0;
    if (generated != 0) {
        screenshot = new W8SaveScreenshot;
        screenshot->version = 1.0f;
        bounds.left = 0;
        bounds.top = 0;
        bounds.right = 0x280;
        bounds.bottom = 0x1e0;
        surface = new W8ColorSurface(srPixelConvert::SURFACE_ARGB1555, screenshot->pixels, 0x50,
                                     0x3c, 0xa0);
        SetRendererOption4Enabled(0);
        screenshot->capture_result = RenderWorldToSurface00426F80(surface, &bounds, 1);
        RenderFrame();
        SetRendererOption4Enabled(1);
        surface->release();
    }
    chunks.OpenChunk(0x544f4853, 0); /* SHOT */
    chunks.Write(screenshot, 0x2588, 0);
    chunks.ReleaseCurrentChunk();
    if (generated != 0) {
        delete screenshot;
    }
    chunks.OpenChunk(0x54584554, 0); /* TEXT */
    SaveMessageStorage0058FB50(chunks.m_hFile);
    chunks.ReleaseCurrentChunk();
    if (g_location_variable_values_00659990.count != 0) {
        chunks.OpenChunk(0x52415654, 0); /* TVAR */
        SaveLocationVariables004441E0(chunks.m_hFile);
        chunks.ReleaseCurrentChunk();
    }
    chunks.OpenChunk(0x4943504e, 0); /* NPCI */
    if (SaveNpcDialogueTranscript00575290(chunks.m_hFile) == 0) {
        chunks.ReleaseCurrentChunk();
        return 0;
    }
    chunks.ReleaseCurrentChunk();
    chunks.OpenChunk(0x5443504e, 0); /* NPCT */
    SaveNpcStates00509F00(&chunks);
    chunks.ReleaseCurrentChunk();
    chunks.OpenChunk(0x4643504e, 0); /* NPCF */
    SaveFactState(chunks.m_hFile);
    chunks.ReleaseCurrentChunk();
    chunks.OpenChunk(0x41544146, 0); /* FATA */
    SaveFactionState00536030(chunks.m_hFile);
    chunks.ReleaseCurrentChunk();
    chunks.OpenChunk(0x4c4e524a, 0); /* JRNL */
    SaveFactJournal00558A90(chunks.m_hFile);
    chunks.ReleaseCurrentChunk();
    if (FindMonsterControlSpellEffect() != 0) {
        chunks.OpenChunk(0x4e505948, 0); /* HYPN */
        SaveMonsterControlSpellEffect00516580(&chunks);
        chunks.ReleaseCurrentChunk();
    }
    if (SaveStatusHeader(&chunks) == 0) {
        return 0;
    }
    chunks.Close();
    return 1;
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
    g_status_685170.next_group_id_234a = header.next_group_id;
    g_status_685170.next_monster_location_id_234e = header.next_monster_location_id;
    g_status_685170.next_world_item_id_2352 = header.next_world_item_id;
    g_status_685170.next_trigger_id_2356 = header.next_trigger_id;
    if (header.next_group_id == 0) {
        g_status_685170.next_group_id_234a = 1;
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
    memcpy(g_status_685170.status_header_prefix_1904, header.status_block,
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
    header.next_group_id = g_status_685170.next_group_id_234a;
    header.next_monster_location_id = g_status_685170.next_monster_location_id_234e;
    header.next_world_item_id = g_status_685170.next_world_item_id_2352;
    header.next_trigger_id = g_status_685170.next_trigger_id_2356;
    memcpy(header.status_block, g_status_685170.status_header_prefix_1904,
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
        SaveWorldCursorNodes0048EAD0(chunks->m_hFile);
        SaveWorldCursorNodeStates0048E6D0(chunks->m_hFile);
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
    SaveAutomapNotes(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    if (g_world->triggers->count != 0) {
        chunks->OpenChunk(0x47495254, 0); /* TRIG */
        SaveWorldTriggers0043C810(g_world, chunks->m_hFile);
        chunks->ReleaseCurrentChunk();
    }

    chunks->OpenChunk(0x54535041, 0); /* APST */
    SaveWorldProps0044E830(g_world, chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->OpenChunk(0x53425543, 0); /* CUBS */
    SaveWorldCursorNodeStates0048E6D0(chunks->m_hFile);
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
    SaveLightStates0049D120(chunks->m_hFile);
    chunks->ReleaseCurrentChunk();

    chunks->ReleaseGroup();
    chunks->ReleaseCurrentChunk();
    return 1;
}

/* MONS chunk: the group and monster totals, then every group's 0x12b-byte
   record restamped to save version 3 with an "encountered" byte, then every
   monster record. A missing group or a failed monster record releases the
   chunk and fails the section. */
// FUNCTION: WIZ8 0x005145a0
unsigned char SaveMonsterStatus(W8Chunk* chunks)
{
    unsigned int group_count;
    unsigned int monster_count;
    unsigned int index;
    unsigned int record_size;
    W8MonsterGroup* group;

    group_count =
        PLLength(gXStatus.plsMonsterGroupEncounterList) + PLLength(gXStatus.plsMonsterGroupList);
    monster_count = PLLength(gXStatus.plsUnbornMonsterList) + PLLength(gXStatus.plsMonsterList);
    chunks->Write(&group_count, 4, 0);
    chunks->Write(&monster_count, 4, 0);
    for (index = 0; index < group_count; ++index) {
        if (index < PLLength(gXStatus.plsMonsterGroupList)) {
            unsigned char encountered;

            group = GetMonsterGroupByListIndex(index);
            if (group == 0) {
                goto fail;
            }
            group->version = 3;
            record_size = 0x12b;
            chunks->Write(&record_size, 4, 0);
            chunks->Write(group, record_size, 0);
            encountered = PListIndexOf(gXStatus.plsMonsterGroupEncounterList, group) != -1;
            chunks->Write(&encountered, 1, 0);
        } else {
            unsigned char encountered;

            group =
                GetMonsterGroupByListIndex(index - PLLength(gXStatus.plsMonsterGroupList) + 0x2710);
            if (group == 0) {
                goto fail;
            }
            group->version = 3;
            record_size = 0x12b;
            chunks->Write(&record_size, 4, 0);
            chunks->Write(group, record_size, 0);
            encountered = PListIndexOf(gXStatus.plsMonsterGroupEncounterList, group) != -1;
            chunks->Write(&encountered, 1, 0);
        }
    }
    for (index = 0; index < monster_count; ++index) {
        if (index < PLLength(gXStatus.plsMonsterList)) {
            if (SaveMonsterRecord005147A0(chunks, index) == 0) {
                goto fail;
            }
        } else {
            if (SaveMonsterRecord005147A0(chunks, index - PLLength(gXStatus.plsMonsterList) +
                                                      0x2710) == 0) {
                goto fail;
            }
        }
    }
    return 1;

fail:
    chunks->ReleaseCurrentChunk();
    return 0;
}

/* One monster's save record: the version-7 tag, the 0x425-byte W8MonsterInfo
   with position/angle refreshed while the entry is live, then the optional
   script name and pending conditions, the unborn flag, the navigator movement
   state and the order/patrol fields the loader reads back in record-version
   order. */
// FUNCTION: WIZ8 0x005147a0
unsigned char SaveMonsterRecord005147A0(W8Chunk* chunks, unsigned int index)
{
    unsigned short script_name[0x20] = {g_empty_ambient_name_65a110};
    int script_wait = -1;
    int script_line = -1;
    unsigned char has_script = 0;
    unsigned int record_version = 7;
    unsigned int record_size;
    int queue_count;
    int point_count;
    int i;
    int component;
    float patrol_value;
    unsigned char unborn;
    unsigned char value;
    srVector3T<float> location;
    srVector3T<float> point;
    W8MonsterInfo* info;
    W8Monster* monster;

    info = MonsterGetScriptPartByLocationIndex(index);
    chunks->Write(&record_version, 4, 0);
    if (info->fActive != 0) {
        MonsterGetLocation(info->monster, &location);
        location.y = SettlePositionToGround00420BD0(&location, 0);
        info->position_17.x = location.x;
        info->position_17.y = location.y;
        info->position_17.z = location.z;
        info->derived_23 = MonsterGetAngleD4004C5770(info->monster);
    }
    record_size = sizeof(*info);
    chunks->Write(&record_size, 4, 0);
    chunks->Write(info, record_size, 0);
    if (info->monster->script_238 == 0) {
        chunks->Write(&has_script, 1, 0);
    } else {
        has_script = 1;
        chunks->Write(&has_script, 1, 0);
        memset(script_name, 0, sizeof(script_name));
        strcpy(reinterpret_cast<char*>(script_name), // reinterpret-ok: the 64-byte
               // save field stores the narrow script name packed as bytes
               info->monster->script_238 != 0 ? info->monster->script_238->getName() : 0);
        script_wait = info->monster->script_wait_240;
        script_line = info->monster->script_line_23c;
        chunks->Write(script_name, 0x40, 0);
        chunks->Write(&script_wait, 4, 0);
        chunks->Write(&script_line, 4, 0);
        queue_count = info->monster->script_conditions_244.GetCount();
        chunks->Write(&queue_count, 4, 0);
        for (i = 0; i < queue_count; ++i) {
            value = *info->monster->script_conditions_244.GetAt(i);
            chunks->Write(&value, 1, 0);
        }
    }
    unborn = PListIndexOf(gXStatus.plsUnbornMonsterList, info) != -1;
    chunks->Write(&unborn, 1, 0);
    monster = info->monster;
    monster->SaveMovementState004549D0(chunks->m_hFile);
    value = monster->defining_orders_28c;
    chunks->Write(&value, 1, 0);
    value = monster->order_mode_28e;
    chunks->Write(&value, 1, 0);
    value = monster->orders_finished_28d;
    chunks->Write(&value, 1, 0);
    value = monster->deaf_28f;
    chunks->Write(&value, 1, 0);
    patrol_value = monster->patrol_distance_294;
    chunks->Write(&patrol_value, 4, 0);
    patrol_value = monster->patrol_variation_298;
    chunks->Write(&patrol_value, 4, 0);
    value = monster->patrol_index_2ac;
    chunks->Write(&value, 1, 0);
    point_count = monster->vector_29c.GetCount();
    chunks->Write(&point_count, 4, 0);
    for (i = 0; i < point_count; ++i) {
        point = *monster->vector_29c.GetAt(i);
        for (component = 0; component < 3; ++component) {
            chunks->Write(&point.x + component, 4, 0);
        }
    }
    point.x = monster->direction_x_2b0;
    point.y = monster->direction_y_2b4;
    point.z = monster->direction_z_2b8;
    for (component = 0; component < 3; ++component) {
        chunks->Write(&point.x + component, 4, 0);
    }
    value = monster->face_party_290;
    chunks->Write(&value, 1, 0);
    value = monster->stay_home_291;
    chunks->Write(&value, 1, 0);
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

/* Walk one status file's top-level chunks and apply the saved section for the
   requested level. A section at the file's end carries no payload and is skipped; a non-matching
   section is released without walking its children. On a save load the level's
   shipped status is folded in first, so baseline state exists under the saved
   overrides. The chunk ids dispatch as a flat chain; LOCK and LCKS share the
   trigger-state loader. */
// FUNCTION: WIZ8 0x00513650
unsigned char LoadItemStatus(W8Chunk* chunk, int level)
{
    unsigned int file_level;
    W8Chunk* stream = chunk;
    unsigned char result = 0;
    int outer_count = stream->ChunkCount();
    unsigned int index;

    for (int outer = 0; outer < outer_count; ++outer) {
        if (result != 0) {
            return result;
        }
        stream->OpenChunk(0, 0);
        if (stream->CurrentChunkId() == 0x534c564c) { /* LVLS */
            if (stream->CurrentChunkAtEnd() != 0) {
                stream->OpenGroup();
                stream->Read(&file_level, 4, 0);
                stream->SkipCurrentChunk();
            } else {
                stream->OpenGroup();
                stream->Read(&file_level, 4, 0);
                if (level == static_cast<int>(file_level)) {
                    if (g_flag_00659756 == 0) {
                        LoadDefaultLevelStatus(level);
                    }
                    result = 1;
                    for (int inner = stream->ChunkCount(); inner > 0; --inner) {
                        stream->OpenChunk(0, 0);
                        if (stream->CurrentChunkAtEnd() == 0) {
                            unsigned long chunk_id = stream->CurrentChunkId();

                            if (chunk_id == 0x54415453) { /* STAT */
                                LoadStatusHeader(stream);
                            } else if (chunk_id == 0x534e4f4d) { /* MONS */
                                unsigned int group_count;
                                unsigned int monster_count;

                                stream->Read(&group_count, 4, 0);
                                stream->Read(&monster_count, 4, 0);
                                for (index = 0; index < group_count; ++index) {
                                    if (LoadMonsterGroup(stream) == 0) {
                                        goto chunk_done;
                                    }
                                }
                                for (index = 0; index < monster_count; ++index) {
                                    if (LoadMonster(stream) == 0) {
                                        goto chunk_done;
                                    }
                                }
                                ReapplyMonsterGroupFormations();
                                RepairMonsterGroupLeaderLinks();
                                ApplyDefaultMonsterGroupSounds();
                            } else if (chunk_id == 0x4d455449) { /* ITEM */
                                unsigned int item_count;

                                stream->Read(&item_count, 4, 0);
                                for (index = 0; index < item_count; ++index) {
                                    if (LoadItem(stream->m_hFile, 1) == 0) {
                                        break;
                                    }
                                }
                            } else if (chunk_id == 0x45425543) { /* CUBE */
                                if (g_flag_00659756 == 0) {
                                    ReleaseWorldCursorNodes0048DB30();
                                }
                                LoadWorldCursorNodes0048E7B0(stream->m_hFile);
                                if (g_flag_00659756 != 0) {
                                    LoadWorldCursorNodeStates0048E470(stream->m_hFile);
                                }
                            } else if (chunk_id == 0x474e4f4d) { /* MONG */
                                if (g_flag_00659756 == 0) {
                                    DestroyMonsterGenerators();
                                }
                                MonGen::LoadAll(stream->m_hFile);
                            } else if (chunk_id == 0x4b434f4c || /* LOCK */
                                       chunk_id == 0x534b434c) { /* LCKS */
                                LoadTriggerRuntimeStates0043CCF0(stream->m_hFile);
                            } else if (chunk_id == 0x53455254) { /* TRES */
                                LoadTriggerActionData0043D1F0(stream->m_hFile);
                            } else if (chunk_id == 0x4f545541) { /* AUTO */
                                LoadAutomapNotes(stream->m_hFile);
                            } else if (chunk_id == 0x47495254) { /* TRIG */
                                LoadWorldTriggers0043C860(g_world, stream->m_hFile);
                            } else if (chunk_id == 0x54535041) { /* APST */
                                LoadWorldProps0044E9A0(g_world, stream->m_hFile);
                            } else if (chunk_id == 0x53425543) { /* CUBS */
                                LoadWorldCursorNodeStates0048E470(stream->m_hFile);
                            } else if (chunk_id == 0x534e474d) { /* MGNS */
                                LoadMonsterGenerators(stream->m_hFile);
                            } else if (chunk_id == 0x53424d41) { /* AMBS */
                                LoadAmbientSoundList0047B270(stream->m_hFile);
                            } else if (chunk_id == 0x54524150) { /* PART */
                                LoadParticleStates0049B3B0(stream->m_hFile);
                            } else if (chunk_id == 0x5448474c) { /* LGHT */
                                LoadLightStates0049D390(stream->m_hFile);
                            }
                        }
                    chunk_done:
                        stream->SkipCurrentChunk();
                        stream->ReleaseCurrentChunk();
                    }
                }
            }
            stream->ReleaseGroup();
        }
        stream->SkipCurrentChunk();
        stream->ReleaseCurrentChunk();
    }
    return result;
}

/* Fold the shipped per-level status file into the live state - the baseline a
   save's section is layered over. The path build is the same table walk
   BuildLevelStatusPath spells out, but the basename search starts four
   characters into the file name and the file itself is opened and walked here.
   Only the persistent-state chunks are taken: cursor nodes, generators and
   both trigger-state records. The level number the LVLS group carries is read
   and discarded; the file is already level-specific. */
// FUNCTION: WIZ8 0x005139c0
unsigned char LoadDefaultLevelStatus(unsigned int level)
{
    W8Chunk chunk;
    W8LevelInfo info;
    char path[256];
    int file_level;
    int count;

    if (LevelBuildInfoByID(level, &info) == 0) {
        srAssertFail("LevelFilesExist(ulLevel, &LevelName)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\LoadSaveGame.cpp", 0x366, 0);
    }
    *strchr(info.level_file_name + 4, '.') = '\0';
    if (level < 0x39) {
        if (level == 0x38) {
            sprintf(path, "%s\\Test\\DefaultLevel.%s", "Levels", "STS");
        } else {
            sprintf(path, "%s\\%s\\%s.%s", "Levels", g_level_folders[level].folder_name,
                    g_level_folders[level].level_name, "STS");
        }
    } else {
        sprintf(path, "%s\\Test\\Level%c.%s", "Levels", level - 0x38, "STS");
    }
    if (chunk.OpenRead(path) != 0) {
        chunk.OpenChunk(0, 0);
        chunk.OpenGroup();
        chunk.Read(&file_level, 4, 0);
        for (count = chunk.ChunkCount(); count > 0; --count) {
            chunk.OpenChunk(0, 0);
            if (chunk.CurrentChunkAtEnd() == 0) {
                unsigned long chunk_id = chunk.CurrentChunkId();

                if (chunk_id == 0x4b434f4c) { /* LOCK */
                    LoadTriggerRuntimeStates0043CCF0(chunk.m_hFile);
                } else if (chunk_id == 0x45425543) { /* CUBE */
                    LoadWorldCursorNodes0048E7B0(chunk.m_hFile);
                } else if (chunk_id == 0x474e4f4d) { /* MONG */
                    MonGen::LoadAll(chunk.m_hFile);
                } else if (chunk_id == 0x53455254) { /* TRES */
                    LoadTriggerActionData0043D1F0(chunk.m_hFile);
                }
            }
            chunk.SkipCurrentChunk();
            chunk.ReleaseCurrentChunk();
        }
        chunk.ReleaseGroup();
        chunk.SkipCurrentChunk();
        chunk.ReleaseCurrentChunk();
        chunk.Close();
        return 1;
    }
    return 0;
}

/* Reads one saved monster group and files it under the species or the encounter
   list. The record's own size leads it, and the assertion that bounds it names
   the record: uiSize <= sizeof(*pMonsterGroup), at line 1517 of this unit.
   A record whose database entry is marked deleted is read and then dropped: it
   is neither listed nor given a monster list, and the function still reports
   success. */
// FUNCTION: WIZ8 0x00513c20
unsigned char LoadMonsterGroup(W8Chunk* chunk)
{
    unsigned int record_size;
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
    stream->Read(&record_size, 4, 0);
    if (record_size > sizeof(W8MonsterGroup)) {
        srAssertFail("uiSize <= sizeof(*pMonsterGroup)", LOADSAVEGAME_CPP, 0x5ed, 0);
    }
    stream->Read(group, record_size, 0);
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
        group->fInCombat = 0;
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
            RegisterActiveEncounterGroup(group);
        }
    }
    return 1;
}

/* One saved monster entry: a version dword, the uiSize-prefixed
   W8MonsterInfo record, then the script block, the unborn-list flag, the
   navigator movement state and - for newer records - the order, patrol and
   facing fields. The monster is adopted into the live or unborn list,
   rejoined to its group, activated, given back its condition and effect
   visuals and its script, then dropped again if its database record was
   deleted and started dying when the record says it is dead. */
// FUNCTION: WIZ8 0x00513d80
unsigned char LoadMonster(W8Chunk* chunk)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8MonsterGroup* monster_group;
    W8Monster* monster;
    W8PList* plist;
    W8GrowableVector<unsigned char> script_conditions;
    srVector3T<float> read_point;
    srVector3T<float> point;
    char script_name[0x40];
    unsigned int record_version;
    unsigned int record_size;
    unsigned int transferred;
    int script_wait;
    int script_line;
    int queue_count;
    int point_count;
    int list_index;
    int index;
    int component;
    unsigned char unborn = 0;
    unsigned char has_script;
    unsigned char value;
    float patrol_value;

    sprintf(script_name, "");
    chunk->Read(&record_version, 4, 0);
    monster_info = static_cast<W8MonsterInfo*>(malloc(sizeof(W8MonsterInfo)));
    if (monster_info == 0) {
        return 0;
    }
    memset(monster_info, 0, sizeof(W8MonsterInfo));
    chunk->Read(&record_size, 4, 0);
    if (record_size > sizeof(W8MonsterInfo)) {
        srAssertFail("uiSize <= sizeof(*pMonsterInfo)", LOADSAVEGAME_CPP, 0x65e, 0);
    }
    chunk->Read(monster_info, record_size, 0);
    chunk->Read(&has_script, 1, 0);
    if (has_script != 0) {
        chunk->Read(script_name, 0x40, &transferred);
        chunk->Read(&script_wait, 4, &transferred);
        chunk->Read(&script_line, 4, &transferred);
        chunk->Read(&queue_count, 4, 0);
        for (index = 0; index < queue_count; ++index) {
            chunk->Read(&value, 1, 0);
            script_conditions.Add(value);
        }
    }
    monster_info->fActive = 0;
    monster_info->monster = 0;
    monster_info->fInCombat = 0;
    monster_info->pCombat = 0;
    if (record_version >= 5) {
        chunk->Read(&unborn, 1, 0);
    }
    plist = gXStatus.plsMonsterList;
    if (unborn != 0) {
        plist = gXStatus.plsUnbornMonsterList;
    }
    list_index = PLAdoptAppend(plist, monster_info);
    if (list_index == -1) {
        free(monster_info);
        return 0;
    }
    record = MonsterDBFromSpecies(monster_info->monster_species);
    if (record == 0) {
        free(monster_info);
        return 0;
    }
    if (record->deleted == 0) {
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x698, LOADSAVEGAME_CPP, monster_info->monster_group_id, 1));
        if (monster_group == 0) {
            free(monster_info);
            return 0;
        }
        IListAdd(monster_group->monsters, monster_info->location_id);
        if (static_cast<unsigned int>(monster_group->value_9f) == 0xcdcdcdcdU ||
            static_cast<unsigned int>(monster_group->value_9f) <
                static_cast<unsigned int>(monster_info->location_id)) {
            monster_group->value_9f = monster_info->location_id;
        }
        ++monster_group->member_count;
        RequestRedrawParty();
        if (monster_info->highest_condition < 0xd) {
            ++monster_group->active_member_count;
        }
    }
    ActivateMonster(monster_info, 0);
    ActivateMonsterInWorld(monster_info);
    monster = monster_info->monster;
    if (monster_info->highest_condition != 0) {
        for (index = 0; index < W8_CONDITION_COUNT; ++index) {
            if (monster_info->condition_turns[index] != 0) {
                SetMonsterSpellIcon(monster, index - 1, 1);
            }
        }
    }
    for (index = 0; index < 8; ++index) {
        if (monster_info->enchantments[index].value_08 != 0) {
            SetMonsterSpellIcon(monster, index + 0x10, 1);
        }
    }
    for (index = 0; index < 12; ++index) {
        if (monster_info->effect_slots_10f[index].duration_0d != 0) {
            SetMonsterSpellIcon(
                monster, g_effect_visual_table[monster_info->effect_slots_10f[index].effect_id][1],
                1);
        }
    }
    if (monster_info->effect_2de > 0) {
        SetMonsterSpellIcon(monster, SPELL_ICON_CHARMED, 1);
    }
    if (monster_info->summoned_2da != 0) {
        SetMonsterSpellIcon(monster, SPELL_ICON_SUMMONED, 1);
    }
    if (record_version >= 2) {
        monster->LoadMovementState00454AD0(chunk->m_hFile);
    }
    if (record_version >= 3) {
        chunk->Read(&value, 1, 0);
        monster->defining_orders_28c = value;
        chunk->Read(&value, 1, 0);
        monster->order_mode_28e = value;
        chunk->Read(&value, 1, 0);
        monster->orders_finished_28d = value;
        chunk->Read(&value, 1, 0);
        monster->deaf_28f = value;
        chunk->Read(&patrol_value, 4, 0);
        monster->patrol_distance_294 = patrol_value;
        chunk->Read(&patrol_value, 4, 0);
        monster->patrol_variation_298 = patrol_value;
        chunk->Read(&value, 1, 0);
        monster->patrol_index_2ac = value;
        chunk->Read(&point_count, 4, 0);
        for (index = 0; index < point_count; ++index) {
            for (component = 0; component < 3; ++component) {
                chunk->Read(&read_point.x + component, 4, 0);
            }
            point = read_point;
            monster->vector_29c.Add(point);
        }
        if (record_version >= 4) {
            for (component = 0; component < 3; ++component) {
                chunk->Read(&read_point.x + component, 4, 0);
            }
            point = read_point;
            monster->direction_x_2b0 = point.x;
            monster->direction_y_2b4 = point.y;
            monster->direction_z_2b8 = point.z;
        }
        if (record_version >= 6) {
            chunk->Read(&value, 1, 0);
            monster->face_party_290 = value;
        }
        if (record_version >= 7) {
            chunk->Read(&value, 1, 0);
            monster->stay_home_291 = value;
        }
        monster_info->flag_255 |= 0x80;
    }
    if (script_name[0] != '\0') {
        monster_info->flag_255 |= 0x10;
        monster->SetScript004C7F10(script_name, 0);
        monster->script_wait_240 = script_wait;
        monster->script_line_23c = script_line;
        while (script_conditions.GetCount() != 0) {
            monster->script_conditions_244.Add(*script_conditions.GetAt(0));
            script_conditions.RemoveAt(0);
        }
    }
    if (record->deleted != 0) {
        RemoveMonster(list_index, 1);
    } else if (monster_info->hp_current == 0) {
        MonsterStartsDying(monster_info, 1);
    }
    return 1;
}

/* Write the MONS section: the group and monster counts, then every group
   record followed by the flag that says whether the group is also on the
   encounter list, then every monster record through SaveMonster. Live list
   indices come first; the encounter groups and the unborn monsters are the
   same records under indices biased by 10000. A null group or a failed
   monster write abandons the section by releasing the current chunk. */

/* Write one monster's record inside the open MONS section: the version and
   record-size words, the W8MonsterInfo image, then the script state, the
   unborn flag, the navigator movement state and the version-3 order/patrol
   fields, mirroring the layout LoadMonster reads back. The script name is
   seeded from the shared empty-name word before the copy the way the ambient
   and spell code seed theirs. */

/* Tear down the live session before a new-game or save load replaces it:
   unload the current level, empty queued character events and spell effects,
   and reset the main-game screen and gameplay status blocks. */
// FUNCTION: WIZ8 0x00512c40
void ResetLiveSessionForLoad(void)
{
    int index;

    if (g_status_685170.current_level != -1) {
        UnloadLevel("");
        SoundEmptyCache();
    }
    if (gXStatus.character_event_queue != 0) {
        gXStatus.character_event_queue->DestroyAllEvents();
    }
    ResetMainGameScreenState();
    ClearNpcMessageQueue();
    ResetMainScreenStateBlock();
    for (index = g_spell_effects.GetCount() - 1; index >= 0; --index) {
        g_spell_effects.RemoveAtAndDelete(index);
    }
    ReleaseAllTriggers();
    ResetGameplayStatusBlock();
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
        if (item->fActive != 0) {
            srVector3T<float> position;
            item->p3D->m_pRep->GetLocation004B8890(&position);
            item->position = position;
            item->entity_flags = static_cast<W8ItemRep*>(first->p3D->m_pRep)->flags;
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
        item->fActive = 0;
        item->p3D = 0;
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
    bool saved = true;
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
            saved = false;
        }
        FileClose(handle);
    } else {
        saved = SaveCharacterToCurrentGame(path, slot, character) != 0;
    }
    if (saved) {
        return 1;
    }
report:
    if (report_failure) {
        CreateMessageBox(
            FormatWideString(gppStringList[W8_NOTICE_CHARACTER_SAVE_FAILED], character->name),
            g_small_font_683678, 1, 1, 0, continuation);
        return 0;
    }
    if (continuation != 0) {
        continuation();
    }
    return 0;
}

/* The two chunk tags the walk recognises, as the four-character codes the
   comparison spells them. */
enum { W8_SAVE_TAG_CHAR = 0x52414843, W8_SAVE_TAG_LVLS = 0x534c564c };

/* Find a live CHAR chunk in Saves\\CurrentGame.SAV whose 64-byte name matches
   and mark it consumed so a later append can supersede it. */
// FUNCTION: WIZ8 0x005154a0
char MarkCurrentGameCharacterChunkConsumed(const char* path)
{
    W8Chunk chunk;
    char name[64];
    bool found = false;
    int index = 0;
    int count;

    if (chunk.OpenReadWrite(const_cast<char*>("Saves\\CurrentGame.SAV")) != 0) {
        count = chunk.ChunkCount();
        if (count > 0) {
            do {
                if (found) {
                    break;
                }
                chunk.OpenChunk(0, 0);
                if (chunk.CurrentChunkAtEnd() == 0 && chunk.CurrentChunkId() == W8_SAVE_TAG_CHAR) {
                    chunk.Read(name, 0x40, 0);
                    if (_stricmp(name, path) == 0) {
                        chunk.SetCurrentChunkAtEnd();
                        found = true;
                    }
                }
                ++index;
            } while (index < count);
        }
        chunk.Close();
    }
    return found ? 1 : 0;
}

/* Append one character record to Saves\\CurrentGame.SAV. Retail writes the
   64-byte name, size and body without opening a CHAR chunk header first; the
   matching load walk still keys on CHAR tags produced by other writers. */
// FUNCTION: WIZ8 0x005155b0
char SaveCharacterToCurrentGame(const char* path, int /*slot*/, W8Character* character)
{
    W8Chunk chunk;
    char name[64];
    unsigned int size;

    MarkCurrentGameCharacterChunkConsumed(path);
    strncpy(name, path, 0x3f);
    name[0x3f] = 0;
    if (chunk.OpenAppend(const_cast<char*>("Saves\\CurrentGame.SAV")) != 0) {
        chunk.Write(name, 0x40, 0);
        size = W8_CHARACTER_SERIALIZED_SIZE;
        chunk.Write(&size, 4, 0);
        chunk.Write(character, size, 0);
        chunk.Close();
        return 1;
    }
    return 0;
}

/* Load one character record from a CHAR chunk in Saves\\CurrentGame.SAV. */
// FUNCTION: WIZ8 0x005156c0
char LoadCharacterFromCurrentGame(const char* path, W8Character* character)
{
    W8Chunk chunk;
    char name[64];
    bool found = false;
    unsigned int size;
    int index = 0;
    int count;

    if (chunk.OpenRead(const_cast<char*>("Saves\\CurrentGame.SAV")) != 0) {
        count = chunk.ChunkCount();
        if (count > 0) {
            do {
                if (found) {
                    break;
                }
                chunk.OpenChunk(0, 0);
                if (chunk.CurrentChunkAtEnd() == 0 && chunk.CurrentChunkId() == W8_SAVE_TAG_CHAR) {
                    chunk.Read(name, 0x40, 0);
                    if (_stricmp(name, path) == 0) {
                        memset(character, 0, sizeof(W8Character));
                        chunk.Read(&size, 4, 0);
                        if (size > W8_CHARACTER_SERIALIZED_SIZE) {
                            srAssertFail("uiSize <= sizeof(*pPC)", LOADSAVEGAME_CPP, 0xba2, 0);
                        }
                        chunk.Read(character, size, 0);
                        found = true;
                    }
                }
                ++index;
            } while (index < count);
        }
        chunk.Close();
    }
    return found ? 1 : 0;
}

/* Render the world into the slot's embedded 80x60 ARGB1555 pixel buffer.
   Option 4 is suppressed so the HUD does not bleed into the thumbnail, the
   full frame is re-rendered afterwards to restore the screen. */
// FUNCTION: WIZ8 0x00515840
void CaptureSaveScreenshot(W8SaveScreenshot* screenshot)
{
    W8ScreenRect rect;
    srColorSurface* surface;

    screenshot->version = 1.0f;
    rect.top = 0;
    rect.left = 0;
    rect.right = 640;
    rect.bottom = 480;
    surface =
        new srColorSurface(srPixelConvert::SURFACE_ARGB1555, screenshot->pixels, 0x50, 0x3c, 0xa0);
    SetRendererOption4Enabled(0);
    screenshot->capture_result = RenderWorldToSurface00426F80(surface, &rect, 1);
    RenderFrame();
    SetRendererOption4Enabled(1);
    surface->release();
}

/* Save-slot bookkeeping from the same established
   Local Code\LoadSaveGame.cpp translation unit. */

// GLOBAL: WIZ8 0x00689f98
unsigned char g_save_pending_00689f98;

/* 0x0061A144, the save-file extension. It sits in writable .data with 16
   reference sites across 10 functions rather than in .rdata with the format
   literals, so it is a mutable character array rather than a string literal;
   this build initialises it to "SAV". */
/* 0x0061A134/0x0061A138: the mask pair SaveGame XORs the iron-man save file's
   creation FILETIME with before storing it in the status block. */

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

    gXStatus.save_notice_shown = 0;
    if (g_status_685170.value_2435 == 0 && AnyMonsterDying() == 0 &&
        ((g_settings_6850c8.auto_save != 0 && forced == 0) || g_status_685170.iron_man != 0) &&
        gXStatus.fCombatMode == 0 && IsSightRangeOverridden() == 0 &&
        IsLevelDataFlag4EffectivelySet() != 0 && gXStatus.fNpcDialogueMode == 0 &&
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
        gXStatus.save_notice_shown = 1;
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            ShowNotice(0xc, gppStringList[0x1e0c / 4], -1, -1, 0);
        }
    }
}

/* Deferred main-game autosave. The first eligible frame after the gameplay
   timer elapses raises the notice and restarts the timer; the next eligible
   frame clears that flag and calls SaveGame. Iron Man overwrites the current
   slot name the same way AutoSaveIfAllowed does. Declining the second-pass
   gates is reported as success so the success notice still posts. */
// FUNCTION: WIZ8 0x00515b00
void ProcessMainGameAutoSave(void)
{
    char name[64];
    char saved;

    if (g_status_685170.value_2435 != 0) {
        return;
    }
    if (AnyMonsterDying() != 0) {
        return;
    }
    if (g_settings_6850c8.auto_save == 0 && g_status_685170.iron_man == 0) {
        return;
    }
    if (gXStatus.fCombatMode != 0) {
        return;
    }
    if (IsSightRangeOverridden() != 0) {
        return;
    }
    if (IsLevelDataFlag4EffectivelySet() == 0) {
        return;
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        return;
    }
    if (gXStatus.fCampMode != 0) {
        return;
    }
    if (gXStatus.save_notice_shown == 0) {
        if (gXStatus.gameplay_timer->GetProgress() <= g_float_005ebb38) {
            return;
        }
        gXStatus.save_notice_shown = 1;
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            ShowNotice(0xc, gppStringList[0x1e0c / 4], -1, -1, 0);
        }
        gXStatus.gameplay_timer->Restart();
        return;
    }
    gXStatus.save_notice_shown = 0;
    if (g_status_685170.value_2435 == 0 && AnyMonsterDying() == 0 &&
        (g_settings_6850c8.auto_save != 0 || g_status_685170.iron_man != 0) &&
        gXStatus.fCombatMode == 0 && IsSightRangeOverridden() == 0 &&
        IsLevelDataFlag4EffectivelySet() != 0 && gXStatus.fNpcDialogueMode == 0 &&
        gXStatus.fCampMode == 0) {
        if (g_status_685170.iron_man != 0) {
            strcpy(name, ConvertWideStringToString(GetLastSaveName()));
        } else {
            strcpy(name, "AutoSave");
        }
        saved = SaveGame(name, 0);
    } else {
        saved = 1;
    }
    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
        return;
    }
    if (saved == 0) {
        ShowNotice(0xc, gppStringList[0x1e14 / 4], -1, -1, 0);
        return;
    }
    ShowNotice(0xc, gppStringList[0x1e10 / 4], -1, -1, 0);
}

/* Serialize the live monster-control effect into the open HYPN chunk. The
   pointer-list vectors and the trailing result are deliberately skipped; the
   loader rebuilds them. The assertion names the local pLure and belongs to
   this unit at source line 3507. */
// FUNCTION: WIZ8 0x00516580
void SaveMonsterControlSpellEffect00516580(W8Chunk* chunks)
{
    W8SpellEffectEntry* lure = FindMonsterControlSpellEffect();

    if (lure == 0) {
        srAssertFail("pLure", LOADSAVEGAME_CPP, 0xdb3, 0);
    }
    chunks->Write(&lure->kind, 4, 0);
    chunks->Write(&lure->turns_remaining, 4, 0);
    chunks->Write(&lure->Source, sizeof(lure->Source), 0);
    chunks->Write(&lure->target, sizeof(lure->target), 0);
    chunks->Write(&lure->OrigSource, sizeof(lure->OrigSource), 0);
    chunks->Write(lure->unknown_03c, sizeof(lure->unknown_03c), 0);
    chunks->Write(&lure->flag_120, 1, 0);
    chunks->Write(&lure->flag_121, 1, 0);
    chunks->Write(&lure->flag_122, 1, 0);
    chunks->Write(&lure->flag_123, 1, 0);
    chunks->Write(&lure->definition, sizeof(lure->definition), 0);
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
    W8Character* characters = status->buffers.Char;
    W8PartySlotRow* party_rows = status->buffers.XChar;
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

    status->buffers.Char = characters;
    status->buffers.XChar = party_rows;

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
            character->original_profession = character->iProfession;
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
            if (party_row->fOccupied != 0 && party_row->pending_action == 8 && origin != -1 &&
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
        chunks->Write(&status->buffers.Char[slot], size, 0);
    }
    for (slot = 0; slot != 8; ++slot) {
        size = sizeof(W8PartySlotRow);
        chunks->Write(&size, sizeof(size), 0);
        chunks->Write(&status->buffers.XChar[slot], size, 0);
    }
    chunks->ReleaseCurrentChunk();
}

/* Collect one level's saved world items out of the fixed current-save file.
   The LVLS groups are walked for a matching level number; every ITEM record
   inside is loaded and appended to the caller's vector. The scan stops once
   the level's group has been processed. */
// FUNCTION: WIZ8 0x00516070
unsigned char LoadSavedLevelItems00516070(int level, W8GrowableVector<W8WorldItem*>* items)
{
    W8Chunk chunk;
    unsigned int file_level;
    unsigned int item_count;
    unsigned int index;
    int inner;
    int outer_count;
    int outer;
    unsigned char found = 0;

    if (chunk.OpenRead(const_cast<char*>("Saves\\CurrentGame.SAV")) == 0) {
        return 0;
    }
    outer_count = chunk.ChunkCount();
    for (outer = 0; outer < outer_count; ++outer) {
        if (found != 0) {
            break;
        }
        chunk.OpenChunk(0, 0);
        if (chunk.CurrentChunkId() == 0x534c564c) { /* LVLS */
            if (chunk.CurrentChunkAtEnd() != 0) {
                chunk.OpenGroup();
                chunk.Read(&file_level, 4, 0);
                chunk.SkipCurrentChunk();
            } else {
                chunk.OpenGroup();
                chunk.Read(&file_level, 4, 0);
                if (level == static_cast<int>(file_level)) {
                    found = 1;
                    for (inner = chunk.ChunkCount(); inner > 0; --inner) {
                        chunk.OpenChunk(0, 0);
                        if (chunk.CurrentChunkAtEnd() == 0 &&
                            chunk.CurrentChunkId() == 0x4d455449) { /* ITEM */
                            chunk.Read(&item_count, 4, 0);
                            for (index = 0; index < item_count; ++index) {
                                W8WorldItem* item = LoadItem(chunk.m_hFile, 0);

                                if (item != 0) {
                                    items->Add(item);
                                }
                            }
                        }
                        chunk.SkipCurrentChunk();
                        chunk.ReleaseCurrentChunk();
                    }
                }
            }
            chunk.ReleaseGroup();
        }
        chunk.SkipCurrentChunk();
        chunk.ReleaseCurrentChunk();
    }
    chunk.Close();
    return found;
}

/* Pick a free autosave slot for the ending sequence: "Ending", then
   "Ending1" through "Ending20" until Saves\<name>.<ext> does not exist.
   Writes the chosen bare name into `name`; returns 0 when all twenty-one
   slots are taken. */
// FUNCTION: WIZ8 0x00516890
unsigned char FindFreeEndingSaveName00516890(char* name)
{
    char path[260];
    int index;

    strcpy(name, "Ending");
    sprintf(path, "%s\\%s.%s", "Saves", name, g_save_extension);
    if (FileExists(path) == 0) {
        return 1;
    }
    for (index = 1; index <= 20; ++index) {
        sprintf(name, "%s%d", "Ending", index);
        sprintf(path, "%s\\%s.%s", "Saves", name, g_save_extension);
        if (FileExists(path) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Write a full save slot: repair the target's read-only bit, fold the running
   CurrentGame sections forward unless this save is CurrentGame itself, refresh
   the camera anchor and per-box countdown snapshots, then emit the GVER
   version triple, the SHOT screenshot, TEXT message storage, the optional TVAR
   location variables, and the NPCI/NPCT/NPCF/FATA/JRNL chain before
   SaveStatusHeader appends the level sections. The transcript writer declining
   NPCI ends the chain early. */

/* Load a save slot: tear down the live session the way
   ResetLiveSessionForLoad does, copy the slot over Saves\CurrentGame.SAV, then
   dispatch each chunk to its section loader. Afterward the four
   message-storage countdown clocks are re-armed from the snapshots the save
   recorded, the gameplay timer restarts, the held-item cursor is restored,
   and the loaded items are normalized. */
// FUNCTION: WIZ8 0x00512920
unsigned char LoadGame(const char* slot_name)
{
    W8Chunk chunks;
    char path[260];
    int count;
    int index;
    int box;

    if (g_status_685170.current_level != -1) {
        UnloadLevel("");
        SoundEmptyCache();
    }
    if (gXStatus.character_event_queue != 0) {
        gXStatus.character_event_queue->DestroyAllEvents();
    }
    ResetMainGameScreenState();
    ClearNpcMessageQueue();
    ResetMainScreenStateBlock();
    for (index = g_spell_effects.GetCount() - 1; index >= 0; --index) {
        g_spell_effects.RemoveAtAndDelete(index);
    }
    ReleaseAllTriggers();
    ResetGameplayStatusBlock();
    sprintf(path, "%s\\%s.%s", "Saves", slot_name, g_save_extension);
    if (_access("Saves\\CurrentGame.SAV", 2) != 0 && errno == EACCES) {
        _chmod("Saves\\CurrentGame.SAV", _S_IREAD | _S_IWRITE);
    }
    FileDelete("Saves\\CurrentGame.SAV");
    FileCopy(path, "Saves\\CurrentGame.SAV", 0);
    if (_access("Saves\\CurrentGame.SAV", 2) != 0 && errno == EACCES) {
        _chmod("Saves\\CurrentGame.SAV", _S_IREAD | _S_IWRITE);
    }
    if (chunks.OpenRead(const_cast<char*>("Saves\\CurrentGame.SAV")) == 0) {
        return 0;
    }
    count = chunks.ChunkCount();
    for (index = 0; index < count; ++index) {
        chunks.OpenChunk(0, 0);
        if (!chunks.CurrentChunkAtEnd()) {
            switch (chunks.CurrentChunkId()) {
            case 0x41545347: /* GSTA */
                LoadGameStatus(&chunks, &g_status_685170);
                break;
            case 0x54584554: /* TEXT */
                LoadMessageStorage0058FC30(chunks.m_hFile);
                break;
            case 0x52415654: /* TVAR */
                LoadLocationVariables00444310(chunks.m_hFile);
                break;
            case 0x4943504e: /* NPCI */
                LoadNpcDialogueTranscript005750D0(chunks.m_hFile);
                break;
            case 0x5443504e: /* NPCT */
                LoadNpcStates00509FC0(&chunks);
                break;
            case 0x4643504e: /* NPCF */
                LoadFactState(chunks.m_hFile);
                break;
            case 0x41544146: /* FATA */
                LoadFactionState00536070(chunks.m_hFile);
                break;
            case 0x4c4e524a: /* JRNL */
                LoadJournalEntries00558B20(chunks.m_hFile);
                break;
            case 0x4e505948: /* HYPN */
                LoadMonsterControlSpellEffect00516310(&chunks);
                break;
            }
        }
        chunks.SkipCurrentChunk();
        chunks.ReleaseCurrentChunk();
    }
    chunks.Close();
    for (box = 0; box < 4; ++box) {
        for (index = 0; index < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[box]);
             ++index) {
            g_message_storage_68f2d8[box][index].clock_08 =
                SetCountdownClock(g_message_storage_68f2d8[box][index].clock_ticking_0c);
        }
    }
    gXStatus.gameplay_timer->Restart();
    ResetMainGameScreenState();
    if (g_status_685170.item_in_hand_235b.iItemNo == -1) {
        ClearHeldItemDisplay();
    } else {
        SetItemCursor(0);
    }
    SanitizeLoadedItems00522EF0();
    return 1;
}

/* Render the world onto an 80x60 ARGB1555 surface backed by the record's
   pixel store. Renderer option 4 is suppressed while Function426F80 captures
   the frame, then RenderFrame repaints the real front buffer before the
   option is restored. */

/* Read the HYPN record: one live spell-effect entry for the monster-control
   effect, rebuilt through the entry constructor and pushed onto
   g_spell_effects. */
// FUNCTION: WIZ8 0x00516310
void LoadMonsterControlSpellEffect00516310(W8Chunk* chunks)
{
    W8SpellEffectEntry* effect = new W8SpellEffectEntry;

    chunks->Read(&effect->kind, 4, 0);
    chunks->Read(&effect->turns_remaining, 4, 0);
    chunks->Read(&effect->Source, 0x34, 0);
    chunks->Read(&effect->target, 0x20, 0);
    chunks->Read(&effect->OrigSource, 0x34, 0);
    chunks->Read(effect->unknown_03c, 0x20, 0);
    chunks->Read(&effect->flag_120, 1, 0);
    chunks->Read(&effect->flag_121, 1, 0);
    chunks->Read(&effect->flag_122, 1, 0);
    chunks->Read(&effect->flag_123, 1, 0);
    chunks->Read(&effect->definition, sizeof(effect->definition), 0);
    AddSpellEffect(effect);
}

/* Write the monster-control spell effect as the HYPN record: the same fields
   the loader reads, none of the runtime vectors or result state. */

/* The byte-vector Grow LoadMonster's script-condition copy emits; the linker
   kept this unit's instance for AddItem as well. */
// TEMPLATE: WIZ8 0x005169a0
// W8GrowableVector<unsigned char>::Grow

/* The remove-and-delete emission LoadGame calls while ResetLiveSessionForLoad
   inlines it (0x00516A00) is instantiated explicitly in vector.cpp. */
