#include "gameplay_boundaries.h"
#include "sr_api.h"

// FUNCTION: WIZ8 0x005222D0
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot)
{
    unsigned int equipped_index;
    unsigned int carried_index;
    unsigned int pool_index;
    unsigned char* character;
    unsigned char* equipped;

    if (item == 0) {
        srAssertFail(
            "pPCItem != NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp",
            0x151b,
            0);
    }

    /* The original holds the character base in one register and advances it in
       place for the carried array, rather than deriving each cursor afresh. */
    equipped_index = 0;
    character = (unsigned char*)(g_party_characters + character_index);
    equipped = character + 0x1029;
    for (; equipped_index < 8; ++equipped_index, equipped += 0xc) {
        if (item == equipped) {
            *origin = 0;
            *slot = (unsigned short)equipped_index;
            return;
        }
    }

    carried_index = 0;
    character += 0xf5d;
    for (; carried_index < 12; ++carried_index, character += 0xc) {
        if (item == character) {
            *origin = 1;
            *slot = (unsigned short)carried_index;
            return;
        }
    }

    for (pool_index = 0; pool_index < g_shared_item_pool_count; ++pool_index) {
        if (item == g_shared_item_pool + pool_index * 0xc) {
            *origin = 2;
            *slot = (unsigned short)pool_index;
            return;
        }
    }

    *origin = 0xff;
    *slot = 0xffff;
}
