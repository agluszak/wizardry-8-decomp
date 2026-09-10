#pragma once

/* Engine Code\Quality.cpp. The startup constructor at 0x0047B500 asserts this
   unit (Quality.cpp:159) where it allocates the shared render-options record. */

void InitializeRenderQuality(void);
