#include "gameplay_boundaries.h"
#include "sr_api.h"

// FUNCTION: WIZ8 0x005222D0
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot)
{
    unsigned int index;
    unsigned char* equipped;
    unsigned char* carried;

    if (item == 0) {
        srAssertFail(
            "pPCItem != NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\PC Item.cpp",
            0x151b,
            0);
    }

    equipped = (unsigned char*)(g_party_characters + character_index) + 0x1029;
    for (index = 0; index < 8; ++index) {
        if (item == equipped + index * 0xc) {
            *origin = 0;
            *slot = (unsigned short)index;
            return;
        }
    }

    carried = (unsigned char*)(g_party_characters + character_index) + 0xf5d;
    for (index = 0; index < 12; ++index) {
        if (item == carried + index * 0xc) {
            *origin = 1;
            *slot = (unsigned short)index;
            return;
        }
    }

    if (g_shared_item_pool_count > 0) {
        for (index = 0; index < g_shared_item_pool_count; ++index) {
            if (item == g_shared_item_pool + index * 0xc) {
                *origin = 2;
                *slot = (unsigned short)index;
                return;
            }
        }
    }

    *origin = 0xff;
    *slot = 0xffff;
}
