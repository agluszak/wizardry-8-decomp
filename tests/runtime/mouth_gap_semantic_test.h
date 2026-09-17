#ifndef WIZ8_TESTS_RUNTIME_MOUTH_GAP_SEMANTIC_TEST_H
#define WIZ8_TESTS_RUNTIME_MOUTH_GAP_SEMANTIC_TEST_H

struct MouthGapSemanticResult {
    unsigned char load_range_count;
    unsigned char load_ranges_match;
    unsigned char load_cursor_head;
    unsigned char load_initial_state;
    unsigned char missing_companion_zeroed;
    unsigned char position_inside_open;
    unsigned char position_between_closed;
    unsigned char position_after_closed;
    unsigned char rescan_nondestructive;
    unsigned char empty_track_closed;
    unsigned char free_clears_track;
};

bool RunMouthGapSemanticTest(MouthGapSemanticResult* result);
void PrintMouthGapSemanticResults(const MouthGapSemanticResult* result);

#endif
