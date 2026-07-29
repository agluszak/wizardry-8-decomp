#pragma once

/* vsurface_private.h is implemented by the vendored C translation unit. Keep
   its declarations under C linkage when they are consumed by recovered C++
   code; otherwise /FORCE can turn the decorated unresolved call into an image-
   base jump that only fails once renderer bring-up wraps the primary surface. */
#include <windows.h>
#include <ddraw.h>

#include "Types.h"
#include "VSurface.h"

extern "C" {
#include "vsurface_private.h"
}
