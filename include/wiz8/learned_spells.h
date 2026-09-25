#pragma once

#include "wiz8/layouts/learned_spells.h"

struct W8Character;

/* 0x004F9600: original TU unresolved in the ItemManager/Magic boundary gap. */
void BuildLearnedSpellState(W8LearnedSpellState* scratch, W8Character* character);
