#include "runtime_instrumentation.h"

#include <windows.h>
#include <string.h>

enum { RUNTIME_EVENT_RING_SIZE = 512 };

static CRITICAL_SECTION g_event_lock;
static bool g_event_lock_ready;
static RuntimeEvent g_events[RUNTIME_EVENT_RING_SIZE];
static unsigned long g_event_sequence;
static unsigned long g_event_counts[RUNTIME_EVENT_KIND_COUNT];

void RuntimeInstrumentationInitialize()
{
    if (!g_event_lock_ready) {
        InitializeCriticalSection(&g_event_lock);
        g_event_lock_ready = true;
    }
}

static void StoreEvent(const RuntimeEvent& value)
{
    EnterCriticalSection(&g_event_lock);
    RuntimeEvent event = value;
    event.sequence = ++g_event_sequence;
    g_events[(event.sequence - 1) % RUNTIME_EVENT_RING_SIZE] = event;
    ++g_event_counts[event.kind];
    LeaveCriticalSection(&g_event_lock);
}

void RuntimeObserve(RuntimeEventKind kind, unsigned long a, unsigned long b, unsigned long c)
{
    RuntimeEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.a = a;
    event.b = b;
    event.c = c;
    StoreEvent(event);
}

void RuntimeObserveWorld(RuntimeEventKind kind, const RuntimeWorldRenderData& world)
{
    RuntimeEvent event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.world = world;
    StoreEvent(event);
}

unsigned long RuntimeEventCount(RuntimeEventKind kind)
{
    EnterCriticalSection(&g_event_lock);
    unsigned long count = g_event_counts[kind];
    LeaveCriticalSection(&g_event_lock);
    return count;
}

unsigned long RuntimeCopyRecentEvents(RuntimeEvent* output, unsigned long capacity)
{
    typedef BOOL(WINAPI * TryEnterCriticalSectionFn)(LPCRITICAL_SECTION);
    FARPROC resolved = GetProcAddress(GetModuleHandleA("kernel32.dll"), "TryEnterCriticalSection");
    TryEnterCriticalSectionFn try_enter = 0;
    memcpy(&try_enter, &resolved, sizeof(try_enter));
    if (try_enter == 0 || !try_enter(&g_event_lock)) {
        return 0;
    }
    unsigned long count = g_event_sequence;
    if (count > RUNTIME_EVENT_RING_SIZE) {
        count = RUNTIME_EVENT_RING_SIZE;
    }
    if (count > capacity) {
        count = capacity;
    }
    unsigned long first = g_event_sequence - count + 1;
    for (unsigned long index = 0; index < count; ++index) {
        output[index] = g_events[(first + index - 1) % RUNTIME_EVENT_RING_SIZE];
    }
    LeaveCriticalSection(&g_event_lock);
    return count;
}

void RuntimeWriteRecentEvents(FILE* stream, const char* scenario)
{
    RuntimeEvent recent[64];
    unsigned long count = RuntimeCopyRecentEvents(recent, 64);
    for (unsigned long index = 0; index < count; ++index) {
        const RuntimeEvent& event = recent[index];
        fprintf(stream,
                "WIZ8_RUNTIME_EVENT scenario=%s sequence=%lu kind=%s a=%lu b=%lu c=%lu "
                "level=%d children=%lu visible=%lu flags=%lu viewport=%d,%d,%d,%d "
                "renderer=%lu,%lu applied=%lu,%lu,%lu,%lu camera=%.2f,%.2f,%.2f\n",
                scenario, event.sequence, RuntimeEventName(event.kind), event.a, event.b, event.c,
                event.world.level, event.world.scene_children, event.world.visible_meshes,
                event.world.flags, event.world.viewport[0], event.world.viewport[1],
                event.world.viewport[2], event.world.viewport[3], event.world.renderer_size[0],
                event.world.renderer_size[1], event.world.applied_viewport[0],
                event.world.applied_viewport[1], event.world.applied_viewport[2],
                event.world.applied_viewport[3], event.world.camera[0], event.world.camera[1],
                event.world.camera[2]);
    }
}

const char* RuntimeEventName(RuntimeEventKind kind)
{
    static const char* const names[RUNTIME_EVENT_KIND_COUNT] = {"screen-changed",
                                                                "main-game-entered",
                                                                "frame-begin",
                                                                "world-render-begin",
                                                                "world-viewport-applied",
                                                                "world-render-end",
                                                                "frame-submitted",
                                                                "mouse-dispatch",
                                                                "region-activated",
                                                                "voice-started",
                                                                "voice-timing",
                                                                "mouth-changed",
                                                                "portrait-frame-changed",
                                                                "portrait-blit",
                                                                "portrait-refresh-state",
                                                                "portrait-refresh-requested"};
    if (kind < 0 || kind >= RUNTIME_EVENT_KIND_COUNT) {
        return "unknown";
    }
    return names[kind];
}
