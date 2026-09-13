#pragma once

struct SightSemanticResult {
    unsigned char blind_is_zero;
    unsigned char facing_away_reduces_range;
    unsigned char skip_fov_restores_range;
    unsigned char penalty_source_reduces_range;
    unsigned char attribute_scales_range;
    unsigned char same_primitive_party_and_monster;
};

bool RunSightSemanticTests(SightSemanticResult* result);
void PrintSightSemanticResults(const SightSemanticResult* result);
