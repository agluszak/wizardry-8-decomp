#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctBuildTree.h"

#include <stdlib.h>

/* This zero-storage node form is constructed by OctBuildTree when its
   pre-tree ownership mode is active.  Its immediately following conversion
   methods and the first assertion-backed boundary at 0x004B19F0 establish
   Engine Code\OctBuildPreTree.cpp as the owning cluster. */
// FUNCTION: WIZ8 0x004af760
W8CountedOctBuildNode004AF760::W8CountedOctBuildNode004AF760()
{
    ++g_value_65be60;
}

// FUNCTION: WIZ8 0x004af780
W8CountedOctBuildNode004AF760::~W8CountedOctBuildNode004AF760()
{
    if (children_00[1] != 0) {
        free(children_00[1]);
    }
    if (g_value_65be60 != 0) {
        --g_value_65be60;
    }
}
