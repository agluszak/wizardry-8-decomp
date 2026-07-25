#pragma once

// The target IJG stdio managers resolve fread/fwrite to the SurRender bridge
// in codec_adapter.cpp and do not inspect or flush a CRT FILE. Force-including
// this overlay before jdatasrc.c/jdatadst.c reproduces that source-level
// configuration without modifying the pristine downloaded IJG tree.
#ifndef _CRTIMP
#define _CRTIMP
#endif
#include <stdio.h>

#define fflush(stream) (0)
#undef ferror
#define ferror(stream) (0)
