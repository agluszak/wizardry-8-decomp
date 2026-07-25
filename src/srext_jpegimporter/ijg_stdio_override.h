#pragma once

// The target IJG stdio managers resolve fread/fwrite to the SurRender bridge
// in codec_adapter.cpp and do not call fflush.  Force-including this overlay
// before jdatasrc.c/jdatadst.c reproduces that source-level configuration
// without modifying the pristine downloaded IJG tree.
#ifndef _CRTIMP
#define _CRTIMP
#endif
#include <stdio.h>

#define fflush(stream) (0)
