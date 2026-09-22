/* Shared gameplay actions built on RuntimeCase: each helper owns one bounded
   interaction, prints its expected/observed context through the case's own
   reporting, and fails with the same step names the old scenario bodies used. */

#include "gameplay_actions.h"

#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/MGSKeyboard.h"

#include "sgp.h"

#include <stdio.h>

namespace {

struct DisplacementCheck {
    srVector3T<float> before;
    float min_distance;
};

bool HorizontalDisplacement(const GameplaySnapshot& now, void* context)
{
    DisplacementCheck* check = static_cast<DisplacementCheck*>(context);
    srVector3T<float> delta = now.position - check->before;
    delta.y = 0;
    return delta.Length() > check->min_distance;
}

struct YawCheck {
    float before;
    float min_delta;
};

bool YawChanged(const GameplaySnapshot& now, void* context)
{
    YawCheck* check = static_cast<YawCheck*>(context);
    float delta = now.yaw - check->before;
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta > check->min_delta;
}

struct QuickSaveCheck {
    int slot;
};

bool QuickSaveWritten(const GameplaySnapshot& now, void* context)
{
    QuickSaveCheck* check = static_cast<QuickSaveCheck*>(context);
    for (int slot = 1; slot <= 3; ++slot) {
        char path[64];
        sprintf(path, "Saves\\Quick %d.SAV", slot);
        if (GetFileAttributesA(path) != static_cast<DWORD>(-1)) {
            check->slot = slot;
            return true;
        }
    }
    return false;
}

struct QuickLoadCheck {
    bool saw_loading;
};

bool QuickLoadCompleted(const GameplaySnapshot& now, void* context)
{
    QuickLoadCheck* check = static_cast<QuickLoadCheck*>(context);
    check->saw_loading = check->saw_loading || now.screen == W8_SCREEN_PLEASE_WAIT ||
                         now.pending == W8_SCREEN_PLEASE_WAIT;
    return check->saw_loading && now.screen == W8_SCREEN_MAIN_GAME && now.pending == -1;
}

bool AutomapScreenActive(const GameplaySnapshot& now, void*)
{
    return now.screen == W8_SCREEN_AUTOMAP && now.pending == -1;
}

} // namespace

bool MoveUntilDisplaced(RuntimeCase& test, int command, const char* step, float min_distance,
                        unsigned long budget_ms)
{
    GameplaySnapshot before;
    if (!test.snapshot(before, step)) {
        return false;
    }
    HeldCommand held(test, command);
    if (!held.begin()) {
        return false;
    }
    DisplacementCheck check;
    check.before = before.position;
    check.min_distance = min_distance;
    test.expected("horizontal displacement > %.2f from (%.2f %.2f)", min_distance,
                  before.position.x, before.position.z);
    if (!test.wait_until("horizontal-displacement", budget_ms, HorizontalDisplacement, &check,
                         &held)) {
        return false;
    }
    test.step(step);
    return true;
}

bool MoveAwayFrom(RuntimeCase& test, int command, const RuntimeCheckpoint& anchor, const char* step,
                  float min_distance)
{
    if (!MoveUntilDisplaced(test, command, step, min_distance)) {
        return false;
    }
    const GameplaySnapshot& now = test.last_snapshot();
    srVector3T<float> delta = now.position - anchor.anchor.position;
    delta.y = 0;
    if (delta.Length() < min_distance) {
        test.expected("horizontal distance >= %.2f from anchor (%.2f %.2f)", min_distance,
                      anchor.anchor.position.x, anchor.anchor.position.z);
        return test.fail(step, "not-away-from-anchor");
    }
    return true;
}

bool TurnUntilYawChanged(RuntimeCase& test, int command, const char* step, float min_delta,
                         unsigned long budget_ms)
{
    GameplaySnapshot before;
    if (!test.snapshot(before, step)) {
        return false;
    }
    HeldCommand held(test, command);
    if (!held.begin()) {
        return false;
    }
    YawCheck check;
    check.before = before.yaw;
    check.min_delta = min_delta;
    test.expected("yaw delta > %.3f from %.3f", min_delta, before.yaw);
    if (!test.wait_until("yaw-changed", budget_ms, YawChanged, &check, &held)) {
        return false;
    }
    test.step(step);
    return true;
}

bool WaitCameraSettled(RuntimeCase& test, GameplaySnapshot& settled, unsigned long budget_ms)
{
    if (!test.snapshot(settled, "save-position")) {
        return false;
    }
    unsigned long started = GetTickCount();
    while (GetTickCount() - started < budget_ms && gfProgramIsRunning) {
        Sleep(20);
        GameplaySnapshot now;
        if (!test.snapshot(now, "save-position")) {
            return false;
        }
        if ((now.position - settled.position).Length() < 0.05f) {
            settled = now;
            return true;
        }
        settled = now;
    }
    return test.fail("save-position", "camera-not-settled");
}

bool QuickSave(RuntimeCase& test, RuntimeCheckpoint& out)
{
    if (!WaitCameraSettled(test, out.anchor)) {
        return false;
    }
    out.quick_slot = -1;
    if (!test.tap(W8_MGS_COMMAND_QUICK_SAVE, "save")) {
        return test.fail("save", "binding-missing");
    }
    QuickSaveCheck check;
    check.slot = -1;
    test.expected("a Saves\\Quick 1..3.SAV file");
    if (!test.wait_until("quick-save-written", 5000, QuickSaveWritten, &check)) {
        return false;
    }
    out.quick_slot = check.slot;
    test.step("game-saved");
    return true;
}

bool QuickLoad(RuntimeCase& test, const RuntimeCheckpoint&)
{
    if (!test.tap(W8_MGS_COMMAND_QUICK_LOAD, "load")) {
        return test.fail("load", "binding-missing");
    }
    QuickLoadCheck check;
    check.saw_loading = false;
    test.expected("PLEASE_WAIT then main game restored");
    return test.wait_until("quick-load-completed", 7000, QuickLoadCompleted, &check);
}

bool ExpectRestoredPosition(RuntimeCase& test, const RuntimeCheckpoint& anchor, float tolerance)
{
    GameplaySnapshot now;
    if (!test.snapshot(now, "load")) {
        return false;
    }
    /* Compare the serialized horizontal anchor, excluding ground-contact
       settling. */
    srVector3T<float> delta = now.position - anchor.anchor.position;
    delta.y = 0;
    if (delta.Length() >= tolerance) {
        test.expected("position within %.2f of (%.2f %.2f)", tolerance, anchor.anchor.position.x,
                      anchor.anchor.position.z);
        return test.fail("load", "saved-position-not-restored");
    }
    test.step("load-position-restored");
    return true;
}

bool OpenAutomap(RuntimeCase& test)
{
    if (!test.tap(W8_MGS_COMMAND_AUTOMAP, "automap-open")) {
        return test.fail("automap-open", "binding-missing");
    }
    test.expected("automap screen active and no pending transition");
    if (!test.wait_until("automap-screen-active", 3000, AutomapScreenActive, 0)) {
        return false;
    }
    test.step("automap-opened");
    return true;
}

bool CloseAutomap(RuntimeCase& test)
{
    if (!test.tap(W8_MGS_COMMAND_AUTOMAP, "automap-close")) {
        return test.fail("automap-close", "binding-missing");
    }
    GameplayWait restored = test.wait_gameplay_ready(3000, "automap-close");
    if (restored == GAMEPLAY_EXECUTOR_UNRESPONSIVE) {
        return false;
    }
    if (restored != GAMEPLAY_READY) {
        return test.fail("automap-close", "main-game-not-restored");
    }
    test.step("automap-closed");
    return true;
}
