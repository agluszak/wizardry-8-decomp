#include "wiz8/save_game.h"
#include "wiz8/virtual_file.h"

#include <windows.h>

#include <errno.h>
#include <io.h>
#include <malloc.h>
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

/* The attribute word is Windows', not SGP's. This build's FileGetAttributes
   copies each attribute bit it knows to the same bit position and returns the
   result, so 0x004054F0 is a filter rather than a renumbering: 0x20 to 0x20,
   0x02 to 0x02, 0x10 to 0x10 and so on. The vendored SFI release does renumber
   - its FILE_IS_ARCHIVE is 0x10 and its FILE_IS_DIRECTORY is 0x02 - so Ghidra
   labels the two tests below with names that are wrong for this image. Read as
   Windows attributes the pair says "is a directory" and "is not read-only",
   which is what a function that verifies save directories is asking, and the
   constants come from windows.h rather than being restated here. */

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

// FUNCTION: WIZ8 0x00512D00
/* Makes sure the three save directories exist and are writable before anything
   is written to them. The names are a table of fixed 60-byte slots terminated
   by an empty one rather than a count, which is why the walk asks strlen and
   not an index: the canonical steps a cursor by 0x3C and re-runs the inlined
   strlen at the bottom of the loop.

   The empty fourth slot is initialized from a string literal, not zeroed in
   place, so it is spelled as one here. */
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

// FUNCTION: WIZ8 0x00514BE0
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

// FUNCTION: WIZ8 0x00514C80
/* Reads the same chain back. Each record carries its predecessor's next
   pointer as a file-resident flag: a non-null value only means another record
   follows, and the real link is rebuilt here. Every failure after the first
   allocation abandons the partial chain, which the original does too. */
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
