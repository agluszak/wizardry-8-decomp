#ifndef WIZ8_BINK_SDK_H
#define WIZ8_BINK_SDK_H

/*
 * Bink SDK declarations used by Wizardry 8.
 *
 * The public names, types, structure prefix, callback shape and return types
 * come from the published Bink 1.5J header retained in the JA2 source oracle.
 * Wizardry 8 predates that header. Its canonical import table proves the one
 * API difference represented here: BinkSetVolume is the older trackless
 * _BinkSetVolume@8 entry, rather than 1.5J's _BinkSetVolume@12 entry.
 */

#include <windows.h>

typedef signed long S32;
typedef unsigned long U32;

struct BINK;
struct BINKSND;

typedef BINK* HBINK;

typedef S32 (__stdcall *BINKSNDOPEN)(
    BINKSND* sound, U32 frequency, S32 bits, S32 channels, U32 flags,
    HBINK bink);
typedef BINKSNDOPEN (__stdcall *BINKSNDSYSOPEN)(U32 parameter);

struct BINKRECT {
    S32 Left;
    S32 Top;
    S32 Width;
    S32 Height;
};

#define BINKMAXDIRTYRECTS 8

/* Public BINK prefix through the dirty rectangles consumed by BinkGetRects.
   The 1.5J SDK header continues with middleware-private runtime state. */
struct BINK {
    U32 Width;                              /* 0x00 */
    U32 Height;                             /* 0x04 */
    U32 Frames;                             /* 0x08 */
    U32 FrameNum;                           /* 0x0c */
    U32 LastFrameNum;                       /* 0x10 */
    U32 FrameRate;                          /* 0x14 */
    U32 FrameRateDiv;                       /* 0x18 */
    U32 ReadError;                          /* 0x1c */
    U32 OpenFlags;                          /* 0x20 */
    U32 BinkType;                           /* 0x24 */
    U32 Size;                               /* 0x28 */
    U32 FrameSize;                          /* 0x2c */
    U32 SndSize;                            /* 0x30 */
    BINKRECT FrameRects[BINKMAXDIRTYRECTS]; /* 0x34 */
};

#define BINKSURFACE555 9

extern "C" {

__declspec(dllimport) S32 __stdcall BinkDDSurfaceType(void* surface);
__declspec(dllimport) S32 __stdcall BinkCopyToBuffer(
    HBINK bink, void* destination, S32 destination_pitch,
    U32 destination_height, U32 destination_x, U32 destination_y, U32 flags);
__declspec(dllimport) S32 __stdcall BinkGetRects(HBINK bink, U32 flags);
__declspec(dllimport) S32 __stdcall BinkPause(HBINK bink, S32 paused);
__declspec(dllimport) void __stdcall BinkSetVolume(HBINK bink, S32 volume);
__declspec(dllimport) void __stdcall BinkClose(HBINK bink);
__declspec(dllimport) S32 __stdcall BinkSetSoundSystem(
    BINKSNDSYSOPEN open, U32 parameter);
__declspec(dllimport) BINKSNDOPEN __stdcall BinkOpenMiles(U32 parameter);
__declspec(dllimport) HBINK __stdcall BinkOpen(const char* path, U32 flags);
__declspec(dllimport) S32 __stdcall BinkDoFrame(HBINK bink);
__declspec(dllimport) S32 __stdcall BinkWait(HBINK bink);
__declspec(dllimport) void __stdcall BinkNextFrame(HBINK bink);

}

#endif
