#pragma once

// Force-included into the IJG stdio managers: fread/fwrite resolve to the SurRender bridge in stream_bridge.cpp, which never touches a CRT FILE.
#undef _CRTIMP
#define _CRTIMP
#include <stdio.h>

#define fflush(stream) (0)
#undef ferror
#define ferror(stream) (0)
