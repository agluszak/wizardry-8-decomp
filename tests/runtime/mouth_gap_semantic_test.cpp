/* In-process semantic scenario for the Engine Code\gap.c mouth-gap track.
   The runtime environment disables Wine's audio drivers, so the scenario
   cannot start a real SGP sample; instead it stamps a channel's uiTimeStamp
   directly so SoundGetPosition reports an exact millisecond position. That
   keeps UpdateMouthGapTrack deterministic: the positions below are chosen at
   least 30 ms away from every range boundary recorded in
   PCS\F_AGGR10\GAP\F_AGGR10_004.GAP. */

#include "mouth_gap_semantic_test.h"

#include "wiz8/mouth_gap.h"

#include "FileMan.h"
#include "soundman.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" {
/* soundman.c file-scope globals. Stamping the slot timestamp is the only way
   to pin SoundGetPosition; SGP exposes no position setter. */
extern BOOLEAN fSoundSystemInit;
extern SOUNDTAG pSoundList[];
}

/* F_AGGR10_004.GAP is a flat little-endian array of (start_ms, end_ms) int32
   pairs; these are its bytes as shipped in SOUND.SLF. */
static const int g_expected_ranges[][2] = {
    {0, 35},      {913, 1047},  {1092, 1147}, {1153, 1315},
    {1823, 1854}, {1932, 1955}, {2435, 2502}, {2616, 2646},
};

static const unsigned int kTestSoundId = 0x6001abc;

static void StampSoundPosition(unsigned int position_ms)
{
    pSoundList[0].uiSoundID = kTestSoundId;
    pSoundList[0].uiTimeStamp = GetTickCount() - position_ms;
}

static unsigned char MouthOpenAt(unsigned int position_ms, W8MouthGapTrack* track)
{
    StampSoundPosition(position_ms);
    UpdateMouthGapTrack(kTestSoundId, track);
    return track->mouth_open;
}

bool RunMouthGapSemanticTest(MouthGapSemanticResult* result)
{
    BOOLEAN saved_init = fSoundSystemInit;
    SOUNDTAG saved_slot = pSoundList[0];
    W8MouthGapTrack track;
    W8MouthGapTrack missing;
    unsigned char ok = 1;

    memset(result, 0, sizeof(*result));

    LoadMouthGapTrack("Data\\Sound\\PCs\\F_AGGR10\\F_AGGR10_004.wav", &track);
    result->load_range_count = track.range_count == 8;
    result->load_cursor_head = track.head != 0 && track.cursor == track.head;
    result->load_initial_state = track.mouth_open == 0 && track.unused_04 == 0;
    {
        W8MouthGapRange* range = track.head;
        unsigned char matches = 1;
        unsigned int index;
        for (index = 0; index < 8; ++index) {
            if (range == 0 || range->start_ms != (unsigned int)g_expected_ranges[index][0] ||
                range->end_ms != (unsigned int)g_expected_ranges[index][1]) {
                matches = 0;
                break;
            }
            range = range->next;
        }
        result->load_ranges_match = matches && range == 0;
    }

    /* A voice path without a companion file must leave the track zeroed. */
    memset(&missing, 0xa5, sizeof(missing));
    LoadMouthGapTrack("Data\\Sound\\PCs\\F_AGGR10\\F_AGGR10_999.wav", &missing);
    result->missing_companion_zeroed = missing.range_count == 0 && missing.head == 0 &&
                                       missing.cursor == 0 && missing.mouth_open == 0;

    fSoundSystemInit = TRUE;

    /* Inside the second range the mouth is held open. */
    result->position_inside_open = MouthOpenAt(980, &track);
    /* Between the first and second ranges the mouth is released. */
    result->position_between_closed = MouthOpenAt(500, &track) == 0;
    /* Past the final range the scan walks off the list and closes. */
    result->position_after_closed = MouthOpenAt(3000, &track) == 0;
    /* The update rescans from head every call: reopening after a closed
       sample proves the list walk is non-destructive. */
    result->rescan_nondestructive = MouthOpenAt(980, &track);

    /* An empty track is a no-op that keeps the mouth closed. */
    result->empty_track_closed = MouthOpenAt(980, &missing) == 0;

    FreeMouthGapTrack(&track);
    result->free_clears_track = track.head == 0 && track.cursor == 0 && track.range_count == 0;

    pSoundList[0] = saved_slot;
    fSoundSystemInit = saved_init;

    ok = result->load_range_count && result->load_ranges_match && result->load_cursor_head &&
         result->load_initial_state && result->missing_companion_zeroed &&
         result->position_inside_open && result->position_between_closed &&
         result->position_after_closed && result->rescan_nondestructive &&
         result->empty_track_closed && result->free_clears_track;
    return ok;
}

void PrintMouthGapSemanticResults(const MouthGapSemanticResult* result)
{
    fprintf(stderr,
            "runtime-test mouth-gap: load_count=%u load_ranges=%u load_cursor=%u "
            "load_state=%u missing_zeroed=%u inside_open=%u between_closed=%u "
            "after_closed=%u rescan=%u empty_closed=%u free=%u\n",
            result->load_range_count, result->load_ranges_match, result->load_cursor_head,
            result->load_initial_state, result->missing_companion_zeroed,
            result->position_inside_open, result->position_between_closed,
            result->position_after_closed, result->rescan_nondestructive,
            result->empty_track_closed, result->free_clears_track);
    fflush(stderr);
}
