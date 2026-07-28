#include "wiz8/chunk.h"
#include "wiz8/sr_api.h"
#include "wiz8/save_game.h"
#include "wiz8/virtual_file.h"

/* GETFILESTRUCT is library layout and comes from the vendored SGP header rather
   than being restated: the 0x44-dword clear the body below opens with is exactly
   its 272 bytes. */
#include "FileMan.h"

#include <windows.h>

#include <errno.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* Local Code\LoadSaveGame.cpp. The unit is established by its own assertions:
   evidence/observations/wiz8/assertions.csv places line 870 at 0x00512E80 and
   line 3507 at 0x00516580, with the line numbers rising with the address, so
   the bodies below sit inside the interval rather than being assigned to it by
   subsystem guesswork. */

/* SGP's FileMan.c, reviewed in evidence/reviewed/wiz8/functions.csv at
   0x004051D0, 0x004051F0 and 0x004054F0. Declared here rather than included
   from third_party/sfi-sgp, as the other recovered units declare their SGP
   callees: this track does not build or modify the vendored tree. */
extern unsigned char DirectoryExists(const char* directory);
extern unsigned char MakeFileManDirectory(const char* directory);
extern unsigned int FileGetAttributes(const char* path);

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
extern bool ItemHasFlags(W8WorldItem* item, unsigned int mask);
/* 0x00516E20, search.cpp line 293: appends the item to the global searchable
   array. Not yet identified beyond that, so it keeps an address name. */
extern void Function516E20(W8WorldItem* item);

/* 0x00659756: set to 1 by LoadLevel (0x0042A6F0) around its restore call at
   0x005135D0 and cleared immediately after, and read only from the save and
   load paths. It gates the bit-3 clear below. The meaning is not established
   beyond "a level restore is in progress", so the name stays positional. */
extern unsigned char g_flag_659756;

/* The object W8WorldItem::unknown_04 points at, and the entity it owns at
   +0x14. Only the two slots these serializers walk are established, so both
   carry address names, as W8Prop's members do. Method4B8890 is a 24-byte
   __thiscall getter with 18 call sites that hands back the entity's position;
   its body is not ported here, so the declaration stays unresolved at link like
   the other recovered callees. The canonical RET 4 fits an out-parameter and a
   12-byte by-value return equally, and both spellings compile to the same call
   site here, so the weaker of the two is the one declared. */
struct W8WorldEntity {
    unsigned char unknown_00[4];
    srVector3T<float> position;          /* 0x04 */
    unsigned char unknown_10[0x80];
    int unknown_90;                      /* 0x90 */

    void Method4B8890(srVector3T<float>* position);
};

struct W8WorldItemOwner {
    unsigned char unknown_00[0x14];
    W8WorldEntity* entity;               /* 0x14 */
};

#define LOADSAVEGAME_CPP "C:\\Projects\\Wizardry 8\\Local Code\\LoadSaveGame.cpp"

/* 0x0050F6A0 and 0x0048C750, not yet identified; named by address as elsewhere
   in src/wiz8. The first is told about every group that survives the load, the
   second only about those two of its flags select. */
extern void Function50F6A0(W8MonsterGroup* group, int unknown);
extern void Function48C750(W8MonsterGroup* group);

/* 0x004E3720, 0x004F69F0 and 0x00443A50, not yet identified; named by address
   as elsewhere in src/wiz8. All three take no argument and return nothing, and
   run before the header is read, so they read as teardown of whatever the
   previous level left behind. */
extern void Function4E3720(void);
extern void Function4F69F0(void);
extern void Function443A50(void);

/* Four counts the header carries, each falling back to one when the save
   records zero, and the 256-byte block it hands over whole. Their meaning is
   not established, so all five keep positional names. */
extern int g_status_count_6874ba;
extern int g_status_count_6874be;
extern int g_status_count_6874c2;
extern int g_status_count_6874c6;
extern unsigned char g_status_block_686a74[0x100];

/* The fixed 0x314-byte header every save begins with. Only the fields
   LoadStatusHeader forwards are established; the rest is read and kept. */
struct W8StatusHeader {
    float version;                       /* 0x000 */
    int value_004;                       /* 0x004 */
    int value_008;                       /* 0x008 */
    int value_00c;                       /* 0x00c */
    int value_010;                       /* 0x010 */
    unsigned char block_014[0x100];      /* 0x014 */
    unsigned char unknown_114[0x200];
};                                       /* 0x314 */

/* 0x00404C80 and 0x00404E10, declared as in game_databases.cpp. */
extern int FileOpen(const char* path, int mode, int flags);
extern void CloseVirtualFile(int handle);

/* 0x005156C0, 0x00517A90 and 0x00518510, not yet identified; named by address
   as elsewhere in src/wiz8. The first loads a character from somewhere other
   than a loose file, the second builds the failure notice the third posts. */
extern char Function5156C0(const char* path, W8Character* character);
extern void* Function517A90(void* target, const char* name, int value,
                            int a, int b, int c, void* callback);
extern void Function518510(void* notice);

/* 0x0068517C selects where characters live, and 0x006874D7 is a per-slot byte
   consulted only when it is set. The failure notice comes out of the shared
   notice array, and 0x00683678 is passed alongside; neither is established
   beyond that, so both keep positional names. */
extern unsigned char g_flag_68517c;
extern unsigned char g_flags_6874d7[];
extern int g_value_683678;

/* Loads one character record, either from a loose file under Saves\Characters
   or Saves\NPCs, or through 0x005156C0 when 0x0068517C says characters are not
   loose. The two spellings of the path share one sprintf: the branch that
   already has a directory literal jumps into the arm that formats one, which is
   what writing the call in both arms compiles to.
   The record is cleared before the read, and the read is two calls: a four-byte
   length and then that many bytes. A short or failed second read leaves the
   record cleared and reports failure, and the file is closed either way. */
// FUNCTION: WIZ8 0x005152B0
unsigned char LoadCharacter(const char* name, W8Character* character, int slot,
                            char report_failure)
{
    char path[60];
    char directory[260];
    unsigned int size;
    unsigned int transferred;
    unsigned char loaded = 0;
    int handle;

    if (g_flag_68517c) {
        if (slot != -1 && g_flags_6874d7[slot] == 0) {
            sprintf(path, "%s\\%s", "Saves\\NPCs", name);
        } else {
            strcpy(path, name);
        }
    } else {
        strcpy(directory, slot != -1 ? "Saves\\NPCs" : "Saves\\Characters");
        sprintf(path, "%s\\%s", directory, name);
    }

    if (g_flag_68517c && (slot == -1 || g_flags_6874d7[slot] != 0)) {
        loaded = Function5156C0(path, character);
    } else {
        handle = FileOpen(path, 1, 0);
        if (handle == 0) {
            goto report;
        }
        memset(character, 0, sizeof(W8Character));
        if (ReadVirtualFile(handle, &size, 4, &transferred)
            && ReadVirtualFile(handle, character, size, &transferred)) {
            loaded = 1;
        }
        CloseVirtualFile(handle);
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
        void* notice = Function517A90(g_notices[W8_NOTICE_CHARACTER_LOAD_FAILED], name,
                                      g_value_683678, 1, 1, 0, 0);
        Function518510(notice);
    }
    return loaded;
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

    Function4E3720();
    Function4F69F0();
    Function443A50();
    if (!chunk->Read(&header, sizeof(header), &transferred)) {
        return 0;
    }
    if (header.version != 2.0f) {
        return 0;
    }
    g_status_count_6874ba = header.value_004;
    g_status_count_6874be = header.value_008;
    g_status_count_6874c2 = header.value_00c;
    g_status_count_6874c6 = header.value_010;
    if (header.value_004 == 0) {
        g_status_count_6874ba = 1;
    }
    if (header.value_008 == 0) {
        g_status_count_6874be = 1;
    }
    if (header.value_00c == 0) {
        g_status_count_6874c2 = 1;
    }
    if (header.value_010 == 0) {
        g_status_count_6874c6 = 1;
    }
    memcpy(g_status_block_686a74, header.block_014, 0x100);
    return 1;
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
// FUNCTION: WIZ8 0x00513C20
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
        group->monsters = IListCreate();
        if (group->monsters == 0) {
            free(group);
            return 0;
        }
        group->member_count = 0;
        group->active_member_count = 0;
        group->flag_28 = 0;
        group->flag_29 = 0;
        if (is_encounter) {
            index = PListAdd(g_monster_group_encounter_list, group);
        } else {
            index = PListAdd(g_monster_group_species_list, group);
        }
        if (index == -1) {
            free(group);
            return 0;
        }
        Function50F6A0(group, 0);
        if (group->flag_c3 != 0 && group->unknown_a3 == 0) {
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
// FUNCTION: WIZ8 0x00512D00
unsigned char VerifyDataSubdirs(void)
{
    char directories[4][60] = { "Saves", "Saves\\Characters", "Saves\\NPCs", "" };
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
// FUNCTION: WIZ8 0x00514BE0
unsigned char SaveItemFile(int handle, W8WorldItem* item_info)
{
    W8WorldItem* first = item_info;
    W8WorldItem* item = first;

    while (item != 0) {
        item->saved_marker = 1;
        if (item->unknown_08 != 0) {
            srVector3T<float> position;
            ((W8WorldItemOwner*)item->unknown_04)->entity->Method4B8890(&position);
            item->position.x = position.x;
            item->position.y = position.y;
            item->position.z = position.z;
            item->unknown_25 = ((W8WorldItemOwner*)first->unknown_04)->entity->unknown_90;
        }
        if (g_flag_659756 != 0) {
            first->unknown_25 &= ~8;
        }
        if (!WriteVirtualFile(handle, item, sizeof(W8WorldItem), (unsigned int*)&item_info)) {
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
// FUNCTION: WIZ8 0x00514C80
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
        if (!ReadVirtualFile(handle, item, sizeof(W8WorldItem), &done)) {
            return 0;
        }
        item->sector_id = -2;
        item->unknown_08 = 0;
        item->unknown_04 = 0;
        if (ItemHasFlags(item, 1)) {
            Function516E20(item);
        }
        if (previous != 0) {
            previous->next = item;
        } else if (add_to_list && PListAdd(g_world_item_list, item) == -1) {
            return 0;
        }
        if (g_flag_659756 != 0) {
            item->unknown_25 &= ~8;
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
// FUNCTION: WIZ8 0x00512FB0
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
