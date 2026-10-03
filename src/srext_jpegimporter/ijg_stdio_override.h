#pragma once

// The target IJG stdio managers resolve fread/fwrite to the SurRender bridge
// in stream_bridge.cpp and do not inspect or flush a CRT FILE. Force-including
// this overlay before jdatasrc.c/jdatadst.c reproduces that source-level
// configuration without modifying the pristine downloaded IJG tree.
// compiler.h includes stddef.h first, which has already selected DLL imports.
// Override that selection before stdio.h declares the bridge entry points.
#undef _CRTIMP
#define _CRTIMP
#include <stdio.h>

#define fflush(stream) (0)
#undef ferror
#define ferror(stream) (0)
