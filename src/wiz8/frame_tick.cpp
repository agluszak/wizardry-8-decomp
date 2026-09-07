#include "wiz8/music_playlist.h"
#include "wiz8/startup_runtime_state.h"
#include "wiz8/screen_state.h"
#include "Container.h"
#include "sgp.h"
#include "surrender/srTypeRegistry.h"

#include <string.h>

unsigned char State5Enter005C2DE0(void);
void State5Frame005C3120(void);
unsigned char State5Tick005C30B0(int leaving);
unsigned char Function5B1740(void);
unsigned char IntroScreenEnter(void);
void IntroScreenFrame(void);
unsigned char IntroScreenLeave(int leaving);
unsigned char InitializeSubsystemFlag(void);
unsigned char MainMenuScreenFunction005BC810(void);
void MainMenuScreenFrame(void);
unsigned char MainMenuScreenLeave(int leaving);
void Screen2Finish(void);
unsigned char PleaseWaitScreenInitialize(void);
unsigned char PleaseWaitScreenEnter(void);
void PleaseWaitScreenFrame(void);
unsigned char PleaseWaitScreenLeave(char leaving);
unsigned char InitializeCampScreen(void);
unsigned char InitializeStartupGrid(void);
unsigned char MainGameScreenEnter0055F8C0(void);
unsigned char MainGameScreenLeave00560660(int leaving);
unsigned char CreateList005EEA28(void);
unsigned char Screen8Finalize(void);
unsigned char AllocateSmallStartupSubsystem(void);
unsigned char OptionsScreenEnter005A9B50(void);
void OptionsScreenFrame005A9CC0(void);
unsigned char OptionsScreenLeave005A9C70(int leaving);
unsigned char FreeSmallStartupSubsystem(void);
unsigned char InitializeJournalFont(void);
unsigned char FinalizeJournalFont(void);
unsigned char Screen12Enter(void);
void Screen12Finish(void);

/*
 * The per-frame tick WinMain calls when no message is waiting and the
 * application is active. It drives a screen-state stack: the current state
 * descriptor sits at 0x0068EC78 and the pending one immediately after it at
 * 0x0068ED10, both 0x98 bytes, which is the element size InitializeGameData gives
 * CreateStack. A state transition copies pending over current, and the
 * displaced state is pushed so it can be returned to.
 *
 * Each state owns five dwords in the table at 0x00647BC8: initialize, enter,
 * frame, leave, and finalize. Startup and shutdown walk the outside pair; the
 * transition loop dispatches the middle three.
 */

static unsigned char ScreenReady(void) { return 1; }
static void ScreenIdle(void) {}
static unsigned char ScreenLeave(int) { return 1; }

/* Rows 3, 8, and 9 are now identified as Character, Automap, and Credits.
   Their live callbacks are 005B1750/005B18E0/005B1840,
   0057E660/0057F1F0/0057EFE0, and 005BC130/005BC530/005BC420 respectively.
   Until those bodies are recovered, these local callbacks keep the runtime
   projection honest about its incomplete implementation. */

/* WIZ8_RUNTIME currently retains the reviewed main-menu callback but not the
   complete thirteen-record lifecycle table.  Keep this bridge local and
   unclaimed: it selects the exact menu body through the same typed dispatch
   shape while wiz8-a69 completes the remaining records. */
static unsigned char EnterMainMenu(void)
{ return MainMenuScreenFunction005BC810(); }

#define g_screen_state g_screen_state_0068ec78
#define g_pending_state g_dword_68ed10
W8ScreenStateHandlers g_screen_handlers[13] = {
    { Function5B1740, IntroScreenEnter, IntroScreenFrame, IntroScreenLeave,
      Function5B1740 },
    { InitializeSubsystemFlag, EnterMainMenu, MainMenuScreenFrame,
      MainMenuScreenLeave, Function5B1740 },
    { Function5B1740, Function5B1740, Screen2Finish,
      (unsigned char (*)(int))Function5B1740, Function5B1740 },
    { Function5B1740, ScreenReady, ScreenIdle, ScreenLeave, Function5B1740 },
    { PleaseWaitScreenInitialize, PleaseWaitScreenEnter, PleaseWaitScreenFrame,
      (unsigned char (*)(int))PleaseWaitScreenLeave, Function5B1740 },
    { Function5B1740, State5Enter005C2DE0, State5Frame005C3120,
      State5Tick005C30B0, Function5B1740 },
    { InitializeCampScreen, ScreenReady, ScreenIdle, ScreenLeave,
      Function5B1740 },
    { InitializeStartupGrid, MainGameScreenEnter0055F8C0, ScreenIdle,
      MainGameScreenLeave00560660,
      Function5B1740 },
    { CreateList005EEA28, ScreenReady, ScreenIdle, ScreenLeave,
      Screen8Finalize },
    { Function5B1740, ScreenReady, ScreenIdle, ScreenLeave, Function5B1740 },
    { AllocateSmallStartupSubsystem, OptionsScreenEnter005A9B50,
      OptionsScreenFrame005A9CC0, OptionsScreenLeave005A9C70,
      FreeSmallStartupSubsystem },
    { InitializeJournalFont, ScreenReady, ScreenIdle, ScreenLeave,
      FinalizeJournalFont },
    { Function5B1740, Screen12Enter, Screen12Finish, MainMenuScreenLeave,
      Function5B1740 }
};
extern unsigned char g_flag_68edac;
int g_dword_647bc0;
int g_dword_647bc4;
extern void* g_stack_68eda8;

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

    SoundServiceStreams();
    Function48F9E0();
    state = g_screen_state.id;
    if (g_flag_68edac) {
        g_dword_647bc0 = state;
        ReleaseScreenTransitionObjects();
        if (!g_screen_handlers[g_screen_state.id].leave(1)) {
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
        if (!g_screen_handlers[g_screen_state.id].leave(0)) {
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
    g_screen_handlers[state].frame();
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
            g_screen_handlers[g_screen_state.id].leave(1);
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
