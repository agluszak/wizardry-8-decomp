#pragma once

/* Product-owned WizLibs.c entry points used by the game startup code. */
extern "C" {
unsigned char InitializeSlfArchives(void);
int LoadPatchSlfArchives(const char* directory);
}
