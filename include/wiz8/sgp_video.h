#pragma once

/* video.h is a C header, but several of its prerequisite headers reach CRT
   overloads before its own extern-C block. Include those prerequisites first,
   then give the complete video-manager interface the linkage its C owner
   exports. */
#include <windows.h>
#include <ddraw.h>
#include <process.h>

#include "Local.h"
#include "Debug.h"
#include "Types.h"
#include "DirectDraw Calls.h"
#include "VSurface.h"

extern "C" {
#include "Mutex Manager.h"
#include "video.h"
}
