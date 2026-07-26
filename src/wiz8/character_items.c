#include "gameplay_boundaries.h"

extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);

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

// FUNCTION: WIZ8 0x00518150
int GetRandomCharacter(int require_primary, int require_secondary, int excluded_slot,
                       signed char excluded_faction)
{
    int skip;
    unsigned int slot;
    unsigned int scanned;
    unsigned char matched;
    W8Character* character;

retry:
    skip = GetRandomNumber(8);
    scanned = 0;
    slot = 0;
    do {
        matched = 0;
        if (g_party_slot_rows[slot][0] != 0 && (int)slot != excluded_slot) {
            character = &g_party_characters[slot];
            if ((character->unknown_0b11 > 0 && character->unknown_0b01 < 0x12)
                || require_primary == 2) {
                if (excluded_faction == -1 || excluded_faction != character->faction) {
                    if (character->unknown_0b01 < 0xf || require_secondary == 2) {
                        matched = 1;
                        if (skip == 0) {
                            return slot;
                        }
                        skip--;
                        scanned = 0;
                    }
                }
            }
        }
        slot++;
        if (slot == 8) {
            slot = 0;
        }
        scanned++;
    } while (scanned <= 8);

    if (matched) {
        return slot;
    }
    if (require_secondary == 1) {
        require_secondary = 2;
        goto retry;
    }
    if (require_primary == 1) {
        require_primary = 2;
        goto retry;
    }
    return -1;
}
