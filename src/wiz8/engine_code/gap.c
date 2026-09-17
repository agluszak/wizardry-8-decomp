#include "wiz8/mouth_gap.h"

#include "FileMan.h"
#include "soundman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Engine Code\gap.c (demo source-path string, line 0xd9 inside the update's
   mouth-open branch; retail sits exactly between IList.cpp's tail and
   Bink.cpp's first emission). Voice samples ship a companion file at
   <voice dir>\GAP\<voice name>.gap holding little-endian int32 (start, end)
   millisecond pairs on the SoundGetPosition clock. While playback sits
   strictly inside a range the track reports the mouth held open; between
   ranges the consumers animate freely. */

// FUNCTION: WIZ8 0x005E2D10
void LoadMouthGapTrack(char* path, W8MouthGapTrack* track)
{
    char gap_path[256];
    char directory[256];
    unsigned char index;
    char* extension;
    unsigned int start;
    unsigned int end;
    unsigned int bytes_read;
    HWFILE file;
    W8MouthGapRange* previous = 0;
    W8MouthGapRange* range;

    track->range_count = 0;
    track->unused_04 = 0;
    track->head = 0;
    track->cursor = 0;
    track->mouth_open = 0;

    strcpy(gap_path, path);
    for (index = strlen(gap_path) - 1; gap_path[index] != '\\'; --index) {
    }
    strncpy(directory, gap_path, index);
    directory[index] = '\0';
    sprintf(gap_path, "%s\\GAP\\%s", directory, path + index + 1);
    for (index = 0; gap_path[index] != '.'; ++index) {
    }
    extension = gap_path + index;
    extension[1] = 'g';
    extension[2] = 'a';
    extension[3] = 'p';
    extension[4] = '\0';

    file = FileOpen(gap_path, FILE_ACCESS_READ, FALSE);
    if (file != 0) {
        FileRead(file, &start, 4, &bytes_read);
        while (FileCheckEndOfFile(file) == 0) {
            FileRead(file, &end, 4, &bytes_read);
            range = (W8MouthGapRange*)malloc(sizeof(W8MouthGapRange));
            if (previous != 0) {
                previous->next = range;
            } else {
                track->head = track->cursor = range;
            }
            ++track->range_count;
            range->next = 0;
            range->start_ms = start;
            range->end_ms = end;
            previous = range;
            FileRead(file, &start, 4, &bytes_read);
        }
        track->mouth_open = 0;
        track->unused_04 = 0;
        FileClose(file);
    }
}

// FUNCTION: WIZ8 0x005E2EF0
void FreeMouthGapTrack(W8MouthGapTrack* track)
{
    W8MouthGapRange* range = track->head;

    if (range != 0) {
        W8MouthGapRange* next;
        for (next = range->next; next != 0; next = next->next) {
            free(range);
            range = next;
        }
        free(range);
    }
    track->head = 0;
    track->cursor = 0;
    track->range_count = 0;
}

// FUNCTION: WIZ8 0x005E2F40
void UpdateMouthGapTrack(unsigned int sound_id, W8MouthGapTrack* track)
{
    if (track != 0) {
        if (track->range_count > 0) {
            unsigned int position = SoundGetPosition(sound_id);
            W8MouthGapRange* range = track->head;
            while (position > range->end_ms) {
                range = range->next;
                if (range == 0) {
                    goto clear;
                }
            }
            if (range->start_ms < position && position < range->end_ms) {
                track->mouth_open = 1;
                return;
            }
        }
    clear:
        track->mouth_open = 0;
    }
}
