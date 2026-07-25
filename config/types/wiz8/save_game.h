#ifndef WIZ8_FORMATS_SAVE_GAME_H
#define WIZ8_FORMATS_SAVE_GAME_H

#include <stdint.h>

#pragma pack(push, 1)

typedef uint32_t W8SaveSectionTag;

enum {
    W8_SAVE_GVER = 0x52455647,
    W8_SAVE_GSTA = 0x41545347,
    W8_SAVE_SHOT = 0x544f4853,
    W8_SAVE_TEXT = 0x54584554,
    W8_SAVE_TVAR = 0x52415654,
    W8_SAVE_NPCI = 0x4943504e,
    W8_SAVE_NPCT = 0x5443504e,
    W8_SAVE_NPCF = 0x4643504e,
    W8_SAVE_FATA = 0x41544146,
    W8_SAVE_JRNL = 0x4c4e524a,
    W8_SAVE_HYPN = 0x4e505948,
    W8_SAVE_LVLS = 0x534c564c,
    W8_SAVE_STAT = 0x54415453,
    W8_SAVE_MONS = 0x534e4f4d,
    W8_SAVE_ITEM = 0x4d455449,
    W8_SAVE_CUBE = 0x45425543,
    W8_SAVE_MONG = 0x474e4f4d,
    W8_SAVE_LOCK = 0x4b434f4c,
    W8_SAVE_TRES = 0x53455254,
    W8_SAVE_AUTO = 0x4f545541,
    W8_SAVE_TRIG = 0x47495254,
    W8_SAVE_APST = 0x54535041,
    W8_SAVE_CUBS = 0x53425543,
    W8_SAVE_MGNS = 0x534e474d,
    W8_SAVE_LCKS = 0x534b434c,
    W8_SAVE_AMBS = 0x53424d41,
    W8_SAVE_PART = 0x54524150,
    W8_SAVE_LGHT = 0x5448474c
};

typedef struct W8SaveStatusHeader {
    float version;                      /* 0x000: 2.0 in the canonical build */
    int32_t unknown_004;
    int32_t unknown_008;
    int32_t unknown_00c;
    int32_t unknown_010;
    uint8_t global_status[0x100];       /* 0x014 */
    uint8_t reserved_114[0x200];        /* 0x114: zeroed by SaveStatusHeader */
} W8SaveStatusHeader;                   /* 0x314 */

#pragma pack(pop)

#endif
