#pragma once

#include "runtime_scenario.h"

class RuntimeCase;

/* A fixture is the owned reach-and-leave contract for the state a scenario
   needs: natural paths are the ordinary product flow, shortcut paths build
   the state directly (new-game party via creation functions; later saves or
   teleports). enter fails through RuntimeCase::fail; leave runs on success
   and failure and must be idempotent. */
struct RuntimeFixtureSpec {
    RuntimeFixtureId id;
    const char* name;
    RuntimePhase phase;
    RuntimeFixturePath path;
    bool (*enter)(RuntimeCase& test);
    void (*leave)(RuntimeCase& test);
};

const RuntimeFixtureSpec* FindRuntimeFixture(RuntimeFixtureId id);
