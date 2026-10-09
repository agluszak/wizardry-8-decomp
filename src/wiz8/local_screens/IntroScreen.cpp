#include "vsurface.h"
#include "wiz8/bink_video.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"

#include "FileMan.h"
#include "LibraryDataBase.h"
#include "Font.h"
#include "english.h"
#include "input.h"
#include "line.h"

#include <stdio.h>

/* Local Screens\IntroScreen.cpp is named by the gpVideo assertion at line 98.
   The canonical state-zero row owns this enter/frame/leave bundle. */

#include "wiz8/local_code/Configuration.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/layouts/game_status.h"

// GLOBAL: WIZ8 0x0064d8ac
w8_ulong g_intro_video_index = 6;
// GLOBAL: WIZ8 0x0064D8B0
static char g_intro_video_names[7][40] = {
    "Wizardry8.bik", "unaligned.bik", "Umpani.bik",  "T'Rang.bik",
    "virgin.bik",    "darkend.bik",   "sirtech.bik",
};

// FUNCTION: WIZ8 0x005AE770
void ContinueAfterDarkEndingVideo(void)
{
    BeginEndgameSequence();
}
// GLOBAL: WIZ8 0x0069C258
W8BinkVideo* gpVideo;

// FUNCTION: WIZ8 0x005ae510
unsigned char IntroScreenEnter(void)
{
    char path[500];

    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    SetFontDestBuffer(FRAME_BUFFER, 0, 0, 0x280, 0x1e0, 0);
    ClearSurfaceRect(0, 0, 0x280, 0x1e0);
    if (g_intro_video_index == 0 && g_settings.intro_seen && !g_status.intro_shown) {
        return 1;
    }
    sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index]);
    if (!FileExists(path)) {
        if (!FindGameDataPath(gzCdDirectory, 3)) {
            return 1;
        }
        sprintf(path, "%sData\\Flics\\Intro\\%s", gzCdDirectory,
                g_intro_video_names[g_intro_video_index]);
        if (!FileExists(path)) {
            return 1;
        }
    }
    StopMusicPlaylist(true);
    DisableCursorScene();
    gpVideo = new W8BinkVideo();
    if (gpVideo == 0) {
        srAssertFail("gpVideo", "C:\\Projects\\Wizardry 8\\Local Screens\\IntroScreen.cpp", 98, 0);
    }
    gpVideo->SetTarget(BeginVideoPresentation());
    if (!gpVideo->Open(path, 0)) {
        delete gpVideo;
        gpVideo = 0;
    }
    return 1;
}

static void AdvanceIntroScreen(void);

// FUNCTION: WIZ8 0x005ae6f0
void IntroScreenFrame(void)
{
    InputAtom input;

    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input)) {
            switch (input.usEvent) {
            case KEY_DOWN:
                if (input.usParam != ESC) {
                    break;
                }
                AdvanceIntroScreen();
                return;
            case LEFT_BUTTON_UP:
            case RIGHT_BUTTON_UP:
                AdvanceIntroScreen();
                return;
            }
        }
    }
    if (gpVideo == 0 || gpVideo->UpdateFrame()) {
        AdvanceIntroScreen();
    }
}

// FUNCTION: WIZ8 0x005ae780
static void AdvanceIntroScreen(void)
{
    char path[500];
    bool first_video_available = false;

    if (g_intro_video_index == 6 && !g_settings.intro_seen) {
        g_intro_video_index = 0;
        sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index]);
        first_video_available = FileExists(path);
        if (!first_video_available && FindGameDataPath(gzCdDirectory, 3)) {
            sprintf(path, "%sData\\Flics\\Intro\\%s", gzCdDirectory,
                    g_intro_video_names[g_intro_video_index]);
            first_video_available = FileExists(path);
        }
        if (first_video_available && gpVideo != 0 && gpVideo->Open(path, 0)) {
            return;
        }
    }
    /* A valid first-video path also closes the presentation when gpVideo is null. */
    if (gpVideo != 0 || first_video_available) {
        FinishVideoPresentation();
    }
    delete gpVideo;
    gpVideo = 0;
    RequestScreenTransition();
    switch (g_intro_video_index) {
    case 0:
    case 6:
        SetPendingScreenState(W8_SCREEN_MAIN_MENU);
        g_settings.intro_seen = true;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        if (!g_status.intro_shown) {
            g_pending_screen_state.mode = 0;
            SetPendingScreenState(W8_SCREEN_PLEASE_WAIT);
        } else {
            g_status.intro_shown = false;
            if (GetPendingScreenState() != 7) {
                SetPendingScreenState(W8_SCREEN_MAIN_GAME);
            }
        }
        break;
    case 5:
        BeginScreenFade(0, 0, 1, ContinueAfterDarkEndingVideo, true, 1);
        break;
    }
    EnableCursorScene();
}

// FUNCTION: WIZ8 0x005ae940
unsigned char IntroScreenLeave(int)
{
    if (gpVideo != 0) {
        FinishVideoPresentation();
        delete gpVideo;
    }
    gpVideo = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005ae980
unsigned char IntroScreenRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        break;
    case LEFT_BUTTON_UP:
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            AdvanceIntroScreen();
        }
        break;
    default:
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005AE9C0
void SetIntroVideoIndex(w8_ulong value)
{
    g_intro_video_index = value;
}
