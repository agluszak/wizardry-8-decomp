#pragma once

#include "vobject.h"
#include "Types.h"
#include "gameloop.h"

enum {
    W8_SCREEN_INTRO = 0,
    W8_SCREEN_MAIN_MENU = 1,
    W8_SCREEN_GAME_START_ROUTER = 2,
    W8_SCREEN_CHARACTER = 3,
    W8_SCREEN_PLEASE_WAIT = 4,
    W8_SCREEN_PARTY_SELECTION = 5,
    W8_SCREEN_CAMP = 6,
    W8_SCREEN_MAIN_GAME = 7,
    W8_SCREEN_AUTOMAP = 8,
    W8_SCREEN_CREDITS = 9,
    W8_SCREEN_OPTIONS = 10,
    W8_SCREEN_JOURNAL = 11,
    W8_SCREEN_EXIT = 12,
    W8_SCREEN_COUNT = 13
};

struct W8ScreenStateHandlers {
    unsigned char (*initialize)(void);   /* +0x00, startup ownership */
    unsigned char (*enter)(void);        /* +0x04, transition into state */
    void (*frame)(void);                 /* +0x08, active frame */
    unsigned char (*leave)(int leaving); /* +0x0c, 0 = suspend, 1 = discard */
    unsigned char (*finalize)(void);     /* +0x10, shutdown ownership */
};

extern W8ScreenStateHandlers g_screen_handlers[W8_SCREEN_COUNT];
static_assert(sizeof(W8ScreenStateHandlers) == 0x14, "W8ScreenStateHandlers_size");

/* The current and pending screen records begin at the two globals whose first
   dwords the reviewed setters address directly. One storage object preserves
   that identity instead of mirroring the id into a synthetic runtime record. */
/* The five leading dwords and the name are what lifecycle record 4's entry
   handler reads out of this record to fill its own descriptor; the four bytes
   before the name and everything past it stay positional, and the name's bound
   is the record's end rather than a proved one. */
struct W8ScreenStateRuntime {
    int id;            /* 0x00 */
    int mode;          /* 0x04 */
    int parameter;     /* 0x08 */
    int parameter_2;   /* 0x0c */
    void* parameter_3; /* 0x10, the save payload the Please Wait
                                      screen's mode 2 hands to SaveGame */
    int parameter_4;   /* 0x14, Camp's entry mode */
    char name[0x80];   /* 0x18 */
};

extern W8ScreenStateRuntime g_current_screen_state;
extern W8ScreenStateRuntime g_pending_screen_state;
extern unsigned char g_screen_return_requested;
extern void* g_screen_return_stack;
extern int g_previous_screen_id;
extern int g_suspended_screen_id;
/* Retail 0x006F0628: WinMain's loop flag, set with 0x006F0630 at startup and
   cleared by the exit screen, the state machine's stop paths, and shutdown. */

static_assert(sizeof(W8ScreenStateRuntime) == 0x98, "W8ScreenStateRuntime_must_be_0x98");
