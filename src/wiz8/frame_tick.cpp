#include "wiz8/gameplay_boundaries.h"

#include <string.h>

/*
 * The per-frame tick WinMain calls when no message is waiting and the
 * application is active. It drives a screen-state stack: the current state
 * descriptor sits at 0x0068EC78 and the pending one immediately after it at
 * 0x0068ED10, both 0x98 bytes, which is the element size Function4E2F40 gives
 * CreateStack. A state transition copies pending over current, and the
 * displaced state is pushed so it can be returned to.
 *
 * Each state owns five dwords in the table at 0x00647BC8. Function4E2F40 walks
 * slot 0 of every record as an initialiser; this walks three more: the entry
 * handler at 0x00647BCC, the one at 0x00647BD0 that closes out a frame, and the
 * tick at 0x00647BD4, which takes a flag distinguishing the leaving pass from
 * the ordinary one.
 */

extern "C" {

struct W8ScreenState {
    int id;                               /* 0x00 */
    unsigned char rest[0x94];
};

struct W8ScreenStateHandlers {
    unsigned char (*initialise)(void);    /* 0x00, walked by Function4E2F40 */
    unsigned char (*enter)(void);         /* 0x04 */
    void (*finish)(void);                 /* 0x08 */
    unsigned char (*tick)(int leaving);   /* 0x0c */
    void* unknown_10;
};

extern unsigned char MainMenuScreenFunction005BC810(void);
extern void PresentMenuOverlayFrame(void);

static unsigned char ScreenReady(void) { return 1; }
static void ScreenIdle(void) {}
static unsigned char ScreenLeave(int) { return 1; }

/* WIZ8_RUNTIME currently retains the reviewed main-menu callback but not the
   complete thirteen-record lifecycle table.  Keep this bridge local and
   unclaimed: it selects the exact menu body through the same typed dispatch
   shape while wiz8-a69 completes the remaining records. */
static void RunMainMenuFrame(void)
{
    MainMenuScreenFunction005BC810();
    PresentMenuOverlayFrame();
}

W8ScreenState g_screen_state = { -1 };
W8ScreenState g_pending_state = { -1 };
W8ScreenStateHandlers g_screen_handlers[13] = {
    { ScreenReady, ScreenReady, RunMainMenuFrame, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 },
    { ScreenReady, ScreenReady, ScreenIdle, ScreenLeave, 0 }
};
extern unsigned char g_flag_68edac;
extern unsigned char gfProgramIsRunning;
int g_dword_647bc0;
int g_dword_647bc4;
extern void* g_stack_68eda8;
extern void* g_object_683fd7;

extern void Function4095B0(void);
extern void Function48F9E0(void);
extern void Function429770(void);
extern void Function52E3B0(void);
extern unsigned char Function405C00(void* stack);
extern unsigned char Function405A70(void* stack, W8ScreenState* into);
extern void* Function405A00(void* stack, W8ScreenState* from);

void SetPendingScreenRuntime(int state)
{
    g_pending_state.id = state;
}

void Function4095B0(void) {}
void Function48F9E0(void) {}

// FUNCTION: WIZ8 0x004E3340
void Function4E3340(void)
{
    int state;

    Function4095B0();
    Function48F9E0();
    state = g_screen_state.id;
    if (g_flag_68edac) {
        g_dword_647bc0 = state;
        Function429770();
        if (!g_screen_handlers[g_screen_state.id].tick(1)) {
            gfProgramIsRunning = 0;
            g_screen_state.id = -1;
            return;
        }
        g_screen_state.id = -1;
        if (g_pending_state.id == -1) {
            if (!Function405C00(g_stack_68eda8)) {
                goto stop;
            }
            if (!Function405A70(g_stack_68eda8, &g_pending_state)) {
                goto stop;
            }
        }
        state = -1;
        g_screen_state.id = state;
        g_flag_68edac = 0;
    }
    if (g_pending_state.id == -1 || g_pending_state.id == state) {
        goto finish;
    }
    if (*((unsigned char*)g_object_683fd7 + 0x44) != 0) {
        Function52E3B0();
        state = g_screen_state.id;
    }
    if (state != -1) {
        g_dword_647bc0 = state;
        Function429770();
        if (!g_screen_handlers[g_screen_state.id].tick(0)) {
            goto clear;
        }
        g_dword_647bc4 = g_screen_state.id;
        g_stack_68eda8 = Function405A00(g_stack_68eda8, &g_screen_state);
    }
    state = g_pending_state.id;
    memcpy(&g_screen_state, &g_pending_state, sizeof(W8ScreenState));
    if (!g_screen_handlers[state].enter()) {
        goto clear;
    }
    state = g_screen_state.id;
    g_pending_state.id = -1;

finish:
    if (state == -1) {
        goto stop;
    }
    g_screen_handlers[state].finish();
    return;

clear:
    g_screen_state.id = -1;
stop:
    gfProgramIsRunning = 0;
}

}
