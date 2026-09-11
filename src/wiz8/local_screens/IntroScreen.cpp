#include "wiz8/bink_video.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/Video2.h"

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
extern unsigned char g_flag_689b2c;
// GLOBAL: WIZ8 0x00689b2c
unsigned char g_flag_689b2c;

extern void ContinueAfterDarkEndingVideo005AE770(void);

// GLOBAL: WIZ8 0x0064d8ac
unsigned long g_intro_video_index_0064d8ac = 6;
static const char g_intro_video_names[7][40] = {
    "Wizardry8.bik",
    "unaligned.bik",
    "Umpani.bik",
    "T'Rang.bik",
    "virgin.bik",
    "darkend.bik",
    "sirtech.bik",
};
W8BinkVideo* gpVideo;

// FUNCTION: WIZ8 0x005ae510
unsigned char IntroScreenEnter(void)
{
    char path[500];

    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
    ClearSurfaceRect(0, 0, 0x280, 0x1e0);
    if (g_intro_video_index_0064d8ac == 0 && g_settings_6850c8.intro_seen && !g_flag_689b2c) {
        return 1;
    }
    sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index_0064d8ac]);
    if (!FileExists(path)) {
        if (!FindGameDataPath0042B590(gzCdDirectory, 3)) {
            return 1;
        }
        sprintf(path, "%sData\\Flics\\Intro\\%s", gzCdDirectory,
                g_intro_video_names[g_intro_video_index_0064d8ac]);
        if (!FileExists(path)) {
            return 1;
        }
    }
    StopMusicPlaylist(1);
    ClearFlag603C60();
    gpVideo = new W8BinkVideo();
    if (gpVideo == 0) {
        srAssertFail(
            "gpVideo",
            "C:\\Projects\\Wizardry 8\\Local Screens\\IntroScreen.cpp",
            98,
            0);
    }
    gpVideo->SetTarget(BeginVideoPresentation());
    if (!gpVideo->Open(path, 0)) {
        delete gpVideo;
        gpVideo = 0;
    }
    return 1;

}

void AdvanceIntroScreen(void);

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
void AdvanceIntroScreen(void)
{
    char path[500];
    W8BinkVideo* video;

    if (g_intro_video_index_0064d8ac == 6 && !g_settings_6850c8.intro_seen) {
        g_intro_video_index_0064d8ac = 0;
        sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index_0064d8ac]);
        if (!FileExists(path)) {
            if (!FindGameDataPath0042B590(gzCdDirectory, 3)) {
                goto ordinary_destroy;
            }
            sprintf(path, "%sData\\Flics\\Intro\\%s", gzCdDirectory,
                    g_intro_video_names[g_intro_video_index_0064d8ac]);
            if (!FileExists(path)) {
                goto ordinary_destroy;
            }
        }
        if (gpVideo != 0 && gpVideo->Open(path, 0)) {
            return;
        }
        FinishVideoPresentation();
        video = gpVideo;
    } else {
ordinary_destroy:
        if (gpVideo == 0) {
            goto cleared;
        }
        FinishVideoPresentation();
        video = gpVideo;
    }
    if (video != 0) {
        delete video;
    }
cleared:
    gpVideo = 0;
    RequestScreenTransition();
    switch (g_intro_video_index_0064d8ac) {
    case 0:
    case 6:
        SetPendingScreenState(W8_SCREEN_MAIN_MENU);
        g_settings_6850c8.intro_seen = 1;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        if (!g_flag_689b2c) {
            g_pending_screen_state.mode = 0;
            SetPendingScreenState(W8_SCREEN_PLEASE_WAIT);
        } else {
            g_flag_689b2c = 0;
            if (GetPendingScreenState() != 7) {
                SetPendingScreenState(W8_SCREEN_MAIN_GAME);
            }
        }
        break;
    case 5:
        ShowModalMessage005A6620(0, 0, 1,
                                ContinueAfterDarkEndingVideo005AE770, 1, 1);
        break;
    }
    SetFlag603C60();
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
unsigned char IntroScreenRegionEvent(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
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
void SetValue64D8AC(unsigned long value)
{
    g_intro_video_index_0064d8ac = value;
}
