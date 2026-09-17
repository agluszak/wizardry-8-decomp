#pragma once

#include <stddef.h>

/* Mouth-open interval list loaded from a voice sample's companion .gap file by
   LoadMouthGapTrack. Each range is a (start, end) pair in milliseconds on the
   SoundGetPosition clock; UpdateMouthGapTrack sets mouth_open while playback
   is strictly inside a range. */
typedef struct W8MouthGapRange {
    unsigned int start_ms;
    unsigned int end_ms;
    struct W8MouthGapRange* next;
} W8MouthGapRange;

typedef struct W8MouthGapTrack {
    unsigned int range_count;
    int unused_04; /* written to zero at load, never read */
    W8MouthGapRange* head;
    /* Initialized to the first range at load; no retail reader. */
    W8MouthGapRange* cursor;
    unsigned char mouth_open;
    unsigned char padding_11[3];
} W8MouthGapTrack;

static_assert(sizeof(W8MouthGapRange) == 0x0c, "W8MouthGapRange_size");
static_assert(offsetof(W8MouthGapTrack, range_count) == 0x00, "W8MouthGapTrack_range_count_offset");
static_assert(offsetof(W8MouthGapTrack, unused_04) == 0x04, "W8MouthGapTrack_unused_04_offset");
static_assert(offsetof(W8MouthGapTrack, head) == 0x08, "W8MouthGapTrack_head_offset");
static_assert(offsetof(W8MouthGapTrack, cursor) == 0x0c, "W8MouthGapTrack_cursor_offset");
static_assert(offsetof(W8MouthGapTrack, mouth_open) == 0x10, "W8MouthGapTrack_mouth_open_offset");
static_assert(sizeof(W8MouthGapTrack) == 0x14, "W8MouthGapTrack_size");

#ifdef __cplusplus
extern "C" { /* C-LINKAGE: src/wiz8/engine_code/gap.c is a C translation unit */
#endif

/* Engine Code\gap.c: mouth-gap companions to voice .wav files. */
void LoadMouthGapTrack(char* path, W8MouthGapTrack* track);              /* 0x005E2D10 */
void FreeMouthGapTrack(W8MouthGapTrack* track);                          /* 0x005E2EF0 */
void UpdateMouthGapTrack(unsigned int sound_id, W8MouthGapTrack* track); /* 0x005E2F40 */

#ifdef __cplusplus
}
#endif
