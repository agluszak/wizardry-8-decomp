#pragma once

#include <windows.h>

typedef void (*RuntimeGameThreadCallback)(void* context);

/* Initialize once after the window exists; shut down after WinMain and the driver
   have both returned. Each invocation borrows context until completion. A pending
   timeout cancels the request before returning false. A running timeout terminates
   the scenario process, because unwinding the caller could invalidate its context. */
bool InitializeRuntimeGameThreadExecutor(HWND window);
void ShutdownRuntimeGameThreadExecutor();
bool RunOnGameThread(RuntimeGameThreadCallback callback, void* context,
                     unsigned long timeout_ms = 5000);
