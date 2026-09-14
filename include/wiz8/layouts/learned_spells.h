#pragma once

#include <stddef.h>

/* Shared learned-spell state filled at 0x004F9600 and embedded in camp UI. */
struct W8LearnedSpellState {
    int spell_ids_by_realm[6][40];
    int scroll[6];
    int learned_total;
};

static_assert(sizeof(W8LearnedSpellState) == 0x3dc, "W8LearnedSpellState_size");
static_assert(offsetof(W8LearnedSpellState, scroll) == 0x3c0, "W8LearnedSpellState_scroll_offset");
static_assert(offsetof(W8LearnedSpellState, learned_total) == 0x3d8,
              "W8LearnedSpellState_total_offset");
