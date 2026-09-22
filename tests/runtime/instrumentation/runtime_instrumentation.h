#ifndef WIZ8_RUNTIME_INSTRUMENTATION_H
#define WIZ8_RUNTIME_INSTRUMENTATION_H

#ifndef WIZ8_RUNTIME_TESTS
#error Runtime instrumentation is only available in Wiz8RuntimeTest.exe
#endif

#include <stdio.h>

enum RuntimeEventKind {
    RUNTIME_SCREEN_CHANGED,
    RUNTIME_MAIN_GAME_ENTERED,
    RUNTIME_FRAME_BEGIN,
    RUNTIME_WORLD_RENDER_BEGIN,
    RUNTIME_WORLD_VIEWPORT_APPLIED,
    RUNTIME_WORLD_RENDER_END,
    RUNTIME_FRAME_SUBMITTED,
    RUNTIME_MOUSE_DISPATCH,
    RUNTIME_REGION_ACTIVATED,
    RUNTIME_VOICE_STARTED,
    RUNTIME_VOICE_TIMING,
    RUNTIME_MOUTH_CHANGED,
    RUNTIME_PORTRAIT_FRAME_CHANGED,
    RUNTIME_PORTRAIT_BLIT,
    RUNTIME_PORTRAIT_REFRESH_STATE,
    RUNTIME_PORTRAIT_REFRESH_REQUESTED,
    RUNTIME_EVENT_KIND_COUNT
};

struct RuntimeWorldRenderData {
    int level;
    unsigned long flags;
    unsigned long scene_children;
    unsigned long visible_meshes;
    int viewport[4];
    unsigned long renderer_size[2];
    unsigned long applied_viewport[4];
    float camera[3];
};

struct RuntimeEvent {
    unsigned long sequence;
    RuntimeEventKind kind;
    unsigned long a;
    unsigned long b;
    unsigned long c;
    RuntimeWorldRenderData world;
};

void RuntimeInstrumentationInitialize();
void RuntimeObserve(RuntimeEventKind kind, unsigned long a, unsigned long b, unsigned long c);
void RuntimeObserveWorld(RuntimeEventKind kind, const RuntimeWorldRenderData& world);
unsigned long RuntimeEventCount(RuntimeEventKind kind);
unsigned long RuntimeCopyRecentEvents(RuntimeEvent* output, unsigned long capacity);
void RuntimeWriteRecentEvents(FILE* stream, const char* scenario);
const char* RuntimeEventName(RuntimeEventKind kind);

#endif
