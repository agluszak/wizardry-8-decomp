#include "game_thread_executor.h"
#include "wiz8_crash_report.h"

namespace {

enum RequestState { REQUEST_PENDING, REQUEST_RUNNING, REQUEST_COMPLETED, REQUEST_CANCELLED };

struct RuntimeGameThreadRequest {
    unsigned long id;
    RuntimeGameThreadCallback callback;
    void* context;
    HANDLE completed;
    RequestState state;
    RuntimeGameThreadRequest* next;
};

const UINT kRequestMessage = WM_APP + 0x57;
HWND g_window;
DWORD g_thread_id;
HHOOK g_hook;
CRITICAL_SECTION g_lock;
unsigned long g_next_id;
RuntimeGameThreadRequest* g_requests;

/* All queue and state transitions hold g_lock. A removed message never carries
   a pointer: cancellation can retire the request before that message is received. */
void UnlinkRequest(RuntimeGameThreadRequest* request)
{
    RuntimeGameThreadRequest** link = &g_requests;
    while (*link != request) {
        link = &(*link)->next;
    }
    *link = request->next;
}

LRESULT CALLBACK DispatchRequest(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HC_ACTION && wparam == PM_REMOVE) {
        /* reinterpret-ok: WH_GETMESSAGE supplies the removed MSG through LPARAM. */
        MSG* message = reinterpret_cast<MSG*>(lparam);
        if (message->hwnd == g_window && message->message == kRequestMessage) {
            EnterCriticalSection(&g_lock);
            RuntimeGameThreadRequest* request = g_requests;
            while (request != 0 && request->id != message->wParam) {
                request = request->next;
            }
            message->message = WM_NULL;
            if (request != 0) {
                UnlinkRequest(request);
                request->state = REQUEST_RUNNING;
            }
            LeaveCriticalSection(&g_lock);
            if (request != 0) {
                /* Windows may otherwise swallow hook exceptions and resume the game
                   with a partially installed fixture. Preserve the ordinary report. */
                __try {
                    request->callback(request->context);
                } __except (W8ReportUnhandledException(GetExceptionInformation()),
                            EXCEPTION_EXECUTE_HANDLER) {
                    TerminateProcess(GetCurrentProcess(), 1);
                }
                EnterCriticalSection(&g_lock);
                request->state = REQUEST_COMPLETED;
                SetEvent(request->completed);
                LeaveCriticalSection(&g_lock);
            }
        }
    }
    return CallNextHookEx(0, code, wparam, lparam);
}

} // namespace

bool InitializeRuntimeGameThreadExecutor(HWND window)
{
    if (g_hook != 0) {
        return window == g_window;
    }
    g_thread_id = GetWindowThreadProcessId(window, 0);
    if (window == 0 || g_thread_id == 0) {
        return false;
    }
    InitializeCriticalSection(&g_lock);
    g_window = window;
    g_hook = SetWindowsHookExA(WH_GETMESSAGE, DispatchRequest, 0, g_thread_id);
    if (g_hook == 0) {
        DeleteCriticalSection(&g_lock);
        g_window = 0;
        return false;
    }
    return true;
}

void ShutdownRuntimeGameThreadExecutor()
{
    if (g_hook != 0) {
        UnhookWindowsHookEx(g_hook);
        g_hook = 0;
        g_window = 0;
        DeleteCriticalSection(&g_lock);
    }
}

bool RunOnGameThread(RuntimeGameThreadCallback callback, void* context, unsigned long timeout_ms)
{
    if (g_hook == 0 || callback == 0 || GetCurrentThreadId() == g_thread_id) {
        return false;
    }
    RuntimeGameThreadRequest* request = new RuntimeGameThreadRequest;
    if (request == 0) {
        return false;
    }
    request->completed = CreateEventA(0, TRUE, FALSE, 0);
    if (request->completed == 0) {
        delete request;
        return false;
    }
    request->callback = callback;
    request->context = context;
    request->state = REQUEST_PENDING;
    EnterCriticalSection(&g_lock);
    request->id = ++g_next_id;
    request->next = g_requests;
    g_requests = request;
    BOOL posted = PostMessage(g_window, kRequestMessage, request->id, 0);
    LeaveCriticalSection(&g_lock);

    if (posted) {
        WaitForSingleObject(request->completed, timeout_ms);
    }
    EnterCriticalSection(&g_lock);
    bool completed = request->state == REQUEST_COMPLETED;
    if (request->state == REQUEST_PENDING) {
        UnlinkRequest(request);
        request->state = REQUEST_CANCELLED;
    } else if (request->state == REQUEST_RUNNING) {
        LeaveCriticalSection(&g_lock);
        /* The stalled callback may hold CRT/DLL locks: neither stdio nor DLL
           detach is a safe way to enforce this deadline. */
        const char diagnostic[] = "WIZ8_RUNTIME_FAILURE step=game-thread-executor "
                                  "reason=running-callback-timeout\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), diagnostic, sizeof(diagnostic) - 1, &written, 0);
        TerminateProcess(GetCurrentProcess(), 1);
        return false;
    }
    LeaveCriticalSection(&g_lock);
    CloseHandle(request->completed);
    delete request;
    return completed;
}
