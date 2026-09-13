#ifndef WIZ8_BINK_VIDEO_H
#define WIZ8_BINK_VIDEO_H

#include "bink.h"
#include "wiz8/wiz8_windows.h"

#include <stddef.h>

struct W8MouthGapRange {
    int start;
    int end;
    W8MouthGapRange* next;
};

struct W8MouthGapTrack {
    int range_count;
    int unused_04;
    W8MouthGapRange* head;
    W8MouthGapRange* tail;
    unsigned char mouth_open;
    unsigned char padding_11[3];
};

static_assert(sizeof(W8MouthGapRange) == 0x0c, "W8MouthGapRange_size");
static_assert(offsetof(W8MouthGapTrack, range_count) == 0x00, "W8MouthGapTrack_range_count_offset");
static_assert(offsetof(W8MouthGapTrack, unused_04) == 0x04, "W8MouthGapTrack_unused_04_offset");
static_assert(offsetof(W8MouthGapTrack, head) == 0x08, "W8MouthGapTrack_head_offset");
static_assert(offsetof(W8MouthGapTrack, tail) == 0x0c, "W8MouthGapTrack_tail_offset");
static_assert(offsetof(W8MouthGapTrack, mouth_open) == 0x10, "W8MouthGapTrack_mouth_open_offset");
static_assert(sizeof(W8MouthGapTrack) == 0x14, "W8MouthGapTrack_size");

void Function5E2D10(char* path, W8MouthGapTrack* gap_data);
void Function5E2EF0(W8MouthGapTrack* state);

/* First-party owner around the closed Bink middleware handle. Engine
   Code\Bink.cpp is named by the retained failure path in its surface copy. */
class W8BinkVideo {
public:
    W8BinkVideo();
    ~W8BinkVideo();

    unsigned char Open(const char* path, int flags);
    unsigned char UpdateFrame();
    unsigned char CopyFrameToPrimarySurface();
    unsigned char CopyFrameToTargetSurface();
    void SetTarget(IDirectDrawSurface2* target);

private:
    HBINK m_handle;                /* 0x00 */
    int m_value_04;                /* 0x04: constructor clears; use unresolved */
    IDirectDrawSurface2* m_target; /* 0x08 */
};

static_assert(sizeof(W8BinkVideo) == 0x0c, "W8BinkVideo_must_be_0x0c");

#endif
