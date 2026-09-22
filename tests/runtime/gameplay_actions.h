#pragma once

#include "runtime_case.h"

/* A saved anchor: the settled camera snapshot plus the Quick save slot the
   product chose, so a later load can verify the position round-trips. */
struct RuntimeCheckpoint {
    GameplaySnapshot anchor;
    int quick_slot;
};

bool MoveUntilDisplaced(RuntimeCase& test, int command, const char* step, float min_distance = 1.0f,
                        unsigned long budget_ms = 3000);
bool MoveAwayFrom(RuntimeCase& test, int command, const RuntimeCheckpoint& anchor, const char* step,
                  float min_distance = 1.0f);
bool TurnUntilYawChanged(RuntimeCase& test, int command, const char* step, float min_delta = 0.01f,
                         unsigned long budget_ms = 3000);
bool WaitCameraSettled(RuntimeCase& test, GameplaySnapshot& settled,
                       unsigned long budget_ms = 2000);
bool QuickSave(RuntimeCase& test, RuntimeCheckpoint& out);
bool QuickLoad(RuntimeCase& test, const RuntimeCheckpoint& anchor);
bool ExpectRestoredPosition(RuntimeCase& test, const RuntimeCheckpoint& anchor,
                            float tolerance = 1.0f);
bool OpenAutomap(RuntimeCase& test);
bool CloseAutomap(RuntimeCase& test);
