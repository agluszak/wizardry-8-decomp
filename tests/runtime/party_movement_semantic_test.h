#pragma once

struct PartyMovementSemanticResult {
    unsigned char cancel_clears_active_move;
    unsigned char completion_marks_finished;
    unsigned char completion_clears_mode;
    unsigned char completion_releases_ui;
};

bool RunPartyMovementSemanticTest(PartyMovementSemanticResult* result);
void PrintPartyMovementSemanticResults(const PartyMovementSemanticResult* result);
