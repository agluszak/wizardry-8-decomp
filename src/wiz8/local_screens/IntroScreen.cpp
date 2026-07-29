#include "wiz8/gameplay_boundaries.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"

#include "FileMan.h"
#include "english.h"
#include "input.h"

#include <stdio.h>

/* Local Screens\IntroScreen.cpp is named by the gpVideo assertion at line 98.
   The canonical state-zero row owns this enter/frame/leave bundle. */

class W8BinkVideo005E2F90 {
public:
    W8BinkVideo005E2F90();
    ~W8BinkVideo005E2F90();

    unsigned char Open(const char* path, int flags);
    unsigned char Finished();
    void SetTarget(void* target);

private:
    unsigned char unknown_00[0x0c];
};

typedef char W8BinkVideo005E2F90_must_be_0x0c[
    sizeof(W8BinkVideo005E2F90) == 0x0c ? 1 : -1];

extern "C" {

extern unsigned char g_flag_68510e;
extern unsigned char g_flag_689b2c;
extern unsigned char g_flag_68ed14;
extern char g_path_6e0fa0[];

extern void ConfigurePresentation00413FD0(int a, int b, int c, int d, int e);
extern void SetRenderClip00407220(int target, int left, int top, int right,
                                 int bottom, int flags);
extern void InvalidateScreenRect004263F0(int left, int top, int right, int bottom);
extern unsigned char FindGameDataPath0042B590(char* path, int drive);
extern void PrepareVideoPlayback0048FF00(int value);
extern unsigned char ClearFlag603C60(void);
extern void* GetPrimaryRenderTarget00423390(void);
extern void FinishVideoPresentation004234A0(void);
extern unsigned char DispatchScreenInput004F1910(const void* event);
extern int GetActiveScreenState0055EC10(void);
extern void SetPendingScreenState(int state);
extern void RequestScreenTransition(void);
extern unsigned char SetFlag603C60(void);
extern void ContinueAfterDarkEndingVideo005AE770(void);
extern void ShowModalMessage005A6620(int a, int b, int c,
                                    void (*callback)(void), int d, int e);

int g_intro_video_index_0064d8ac = 6;
static const char g_intro_video_names[7][40] = {
    "Wizardry8.bik",
    "unaligned.bik",
    "Umpani.bik",
    "T'Rang.bik",
    "virgin.bik",
    "darkend.bik",
    "sirtech.bik",
};
W8BinkVideo005E2F90* gpVideo;

// FUNCTION: WIZ8 0x005AE510
unsigned char IntroScreenEnter(void)
{
    char path[500];

    ConfigurePresentation00413FD0(0x500, 0, 0, 0x280, 0x1e0);
    SetRenderClip00407220(-14, 0, 0, 0x280, 0x1e0, 0);
    InvalidateScreenRect004263F0(0, 0, 0x280, 0x1e0);
    if (g_intro_video_index_0064d8ac == 0 && g_flag_68510e && !g_flag_689b2c) {
        return 1;
    }
    sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index_0064d8ac]);
    if (!FileExists(path)) {
        if (!FindGameDataPath0042B590(g_path_6e0fa0, 3)) {
            return 1;
        }
        sprintf(path, "%sData\\Flics\\Intro\\%s", g_path_6e0fa0,
                g_intro_video_names[g_intro_video_index_0064d8ac]);
        if (!FileExists(path)) {
            return 1;
        }
    }
    PrepareVideoPlayback0048FF00(1);
    ClearFlag603C60();
    gpVideo = new W8BinkVideo005E2F90();
    if (gpVideo == 0) {
        srAssertFail(
            "gpVideo",
            "C:\\Projects\\Wizardry 8\\Local Screens\\IntroScreen.cpp",
            98,
            0);
    }
    gpVideo->SetTarget(GetPrimaryRenderTarget00423390());
    if (!gpVideo->Open(path, 0)) {
        delete gpVideo;
        gpVideo = 0;
    }
    return 1;
}

void AdvanceIntroScreen(void);

// FUNCTION: WIZ8 0x005AE6F0
void IntroScreenFrame(void)
{
    InputAtom input;

    while (DequeueEvent(&input) == 1) {
        if (!DispatchScreenInput004F1910(&input)) {
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
    if (gpVideo == 0 || gpVideo->Finished()) {
        AdvanceIntroScreen();
    }
}

// FUNCTION: WIZ8 0x005AE780
void AdvanceIntroScreen(void)
{
    char path[500];
    W8BinkVideo005E2F90* video;

    if (g_intro_video_index_0064d8ac == 6 && !g_flag_68510e) {
        g_intro_video_index_0064d8ac = 0;
        sprintf(path, "Data\\Flics\\Intro\\%s", g_intro_video_names[g_intro_video_index_0064d8ac]);
        if (!FileExists(path)) {
            if (!FindGameDataPath0042B590(g_path_6e0fa0, 3)) {
                goto ordinary_destroy;
            }
            sprintf(path, "%sData\\Flics\\Intro\\%s", g_path_6e0fa0,
                    g_intro_video_names[g_intro_video_index_0064d8ac]);
            if (!FileExists(path)) {
                goto ordinary_destroy;
            }
        }
        if (gpVideo != 0 && gpVideo->Open(path, 0)) {
            return;
        }
        FinishVideoPresentation004234A0();
        video = gpVideo;
    } else {
ordinary_destroy:
        if (gpVideo == 0) {
            goto cleared;
        }
        FinishVideoPresentation004234A0();
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
        SetPendingScreenState(1);
        g_flag_68510e = 1;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        if (!g_flag_689b2c) {
            g_flag_68ed14 = 0;
            SetPendingScreenState(4);
        } else {
            g_flag_689b2c = 0;
            if (GetActiveScreenState0055EC10() != 7) {
                SetPendingScreenState(7);
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

// FUNCTION: WIZ8 0x005AE940
unsigned char IntroScreenLeave(int)
{
    if (gpVideo != 0) {
        FinishVideoPresentation004234A0();
        delete gpVideo;
    }
    gpVideo = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005AE980
unsigned char IntroScreenRegionEvent(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= 0x40;
        break;
    case LEFT_BUTTON_UP:
        if (region->flags & 0x40) {
            AdvanceIntroScreen();
        }
        break;
    default:
        return 0;
    }
    return 1;
}

}
