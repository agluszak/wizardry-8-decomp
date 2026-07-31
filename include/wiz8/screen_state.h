#pragma once

/* The current and pending screen records begin at the two globals whose first
   dwords the reviewed setters address directly. One storage object preserves
   that identity instead of mirroring the id into a synthetic runtime record.
   The five leading dwords and the name are what lifecycle record 4's entry
   handler reads out of this record to fill its own descriptor; the four bytes
   before the name and everything past it stay positional, and the name's bound
   is the record's end rather than a proved one.

   Lifecycle enter 0x005B1750 reads the mode dword at +0x04 and the context
   pointer at +0x10 when constructing the screen object it parks at
   0x0069C2E8; those are mode and parameter_3 below. */
struct W8ScreenStateRuntime {
    int id;                        /* 0x00 */
    int mode;                      /* 0x04 */
    int parameter;                 /* 0x08 */
    int parameter_2;               /* 0x0c */
    void* parameter_3;             /* 0x10: save payload / screen-object context */
    unsigned char unknown_14[4];   /* 0x14 */
    char name[0x80];               /* 0x18 */
};

extern "C" W8ScreenStateRuntime g_screen_state_0068ec78;
extern "C" W8ScreenStateRuntime g_dword_68ed10;

static_assert(sizeof(W8ScreenStateRuntime) == 0x98, "W8ScreenStateRuntime_must_be_0x98");

/* 0x0069C0F4: the camp screen's state block, allocated while that screen is up.
   Lifecycle record 6 - the camp record, which the screen enum's W8_SCREEN_CAMP
   selects - clears the pointer from its initializer, and the redraw router reads
   the flag word through it. Only that word is established. */
struct W8CampScreenState0069C0F4 {
    unsigned char m_positional_000[0xf8];
    unsigned int redraw_flags;            /* 0xf8 */
};

extern "C" W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
