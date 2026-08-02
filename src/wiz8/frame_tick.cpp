#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"
#include "Container.h"
#include "sgp.h"
#include "surrender/srTypeRegistry.h"

#include <string.h>

/*
 * The per-frame tick WinMain calls when no message is waiting and the
 * application is active. It drives a screen-state stack: the current state
 * descriptor sits at 0x0068EC78 and the pending one immediately after it at
 * 0x0068ED10, both 0x98 bytes, which is the element size InitializeGameData gives
 * CreateStack. A state transition copies pending over current, and the
 * displaced state is pushed so it can be returned to.
 *
 * Each state owns five dwords in the table at 0x00647BC8. InitializeGameData walks
 * slot 0 of every record as an initialiser; this walks three more: the entry
 * handler at 0x00647BCC, the one at 0x00647BD0 that closes out a frame, and the
 * tick at 0x00647BD4, which takes a flag distinguishing the leaving pass from
 * the ordinary one.
 */

extern "C" {

struct W8ScreenStateHandlers {
    unsigned char (*initialise)(void);    /* 0x00, walked by InitializeGameData */
    unsigned char (*enter)(void);         /* 0x04 */
    void (*finish)(void);                 /* 0x08 */
    unsigned char (*tick)(int leaving);   /* 0x0c */
    void* unknown_10;
};

extern unsigned char MainMenuScreenFunction005BC810(void);
extern void MainMenuScreenFrame(void);

static unsigned char ScreenReady(void) { return 1; }
static void ScreenIdle(void) {}
static unsigned char ScreenLeave(int) { return 1; }

/* WIZ8_RUNTIME currently retains the reviewed main-menu callback but not the
   complete thirteen-record lifecycle table.  Keep this bridge local and
   unclaimed: it selects the exact menu body through the same typed dispatch
   shape while wiz8-a69 completes the remaining records. */
static unsigned char EnterMainMenu(void)
{ return MainMenuScreenFunction005BC810(); }

#define g_screen_state g_screen_state_0068ec78
#define g_pending_state g_dword_68ed10
W8ScreenStateHandlers g_screen_handlers[13] = {
    { ScreenReady, EnterMainMenu, MainMenuScreenFrame, ScreenLeave, 0 },
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
int g_dword_647bc0;
int g_dword_647bc4;
extern void* g_stack_68eda8;

extern void Function4095B0(void);
extern void Function48F9E0(void);

void Function4095B0(void) {}
void Function48F9E0(void) {}

int g_screen_transition_object_count_654aac;
srClass** g_screen_transition_objects_654ab4;

// FUNCTION: WIZ8 0x00429770
void ReleaseScreenTransitionObjects(void)
{
    int index;
    srClass* object;

    while (g_screen_transition_object_count_654aac != 0) {
        object = g_screen_transition_objects_654ab4[0];
        if (g_screen_transition_object_count_654aac > 0) {
            for (index = 0; index < g_screen_transition_object_count_654aac - 1; ++index) {
                g_screen_transition_objects_654ab4[index] =
                    g_screen_transition_objects_654ab4[index + 1];
            }
            --g_screen_transition_object_count_654aac;
        }
        object->release();
    }
}

// FUNCTION: WIZ8 0x004e3340
void Function4E3340(void)
{
    int state;

    Function4095B0();
    Function48F9E0();
    state = g_screen_state.id;
    if (g_flag_68edac) {
        g_dword_647bc0 = state;
        ReleaseScreenTransitionObjects();
        if (!g_screen_handlers[g_screen_state.id].tick(1)) {
            gfProgramIsRunning = 0;
            g_screen_state.id = -1;
            return;
        }
        g_screen_state.id = -1;
        if (g_pending_state.id == -1) {
            if (!StackSize(g_stack_68eda8)) {
                goto stop;
            }
            if (!Pop(g_stack_68eda8, &g_pending_state)) {
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
    /* The original tests only the low byte of the vector count. Preserve that
       aliasing instead of widening the load to the field's full int type. */
    if (*reinterpret_cast<const unsigned char*>(&g_startup_runtime_state->vector_40.count) != 0) {
        g_startup_runtime_state->ProcessNextPendingEntry();
        state = g_screen_state.id;
    }
    if (state != -1) {
        g_dword_647bc0 = state;
        ReleaseScreenTransitionObjects();
        if (!g_screen_handlers[g_screen_state.id].tick(0)) {
            goto clear;
        }
        g_dword_647bc4 = g_screen_state.id;
        g_stack_68eda8 = Push(g_stack_68eda8, &g_screen_state);
    }
    state = g_pending_state.id;
    memcpy(&g_screen_state, &g_pending_state, sizeof(W8ScreenStateRuntime));
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

// FUNCTION: WIZ8 0x004e34b0
void ShutdownScreenStack(int release_screens)
{
    int state;

    if (!release_screens) {
        return;
    }
    for (;;) {
        if (g_screen_state.id != -1) {
            g_dword_647bc0 = g_screen_state.id;
            ReleaseScreenTransitionObjects();
            g_screen_handlers[g_screen_state.id].tick(1);
            g_screen_state.id = -1;
        }
        if (!StackSize(g_stack_68eda8) || !Pop(g_stack_68eda8, &g_pending_state)) {
            break;
        }
        if (g_pending_state.id != -1) {
            state = g_pending_state.id;
            memcpy(&g_screen_state, &g_pending_state, sizeof(g_screen_state));
            if (!g_screen_handlers[state].enter()) {
                g_screen_state.id = -1;
            } else {
                g_pending_state.id = -1;
            }
        }
    }
}

}
