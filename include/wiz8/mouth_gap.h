#pragma once

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
void Function5E2F40(int sound_handle, W8MouthGapTrack* state);
