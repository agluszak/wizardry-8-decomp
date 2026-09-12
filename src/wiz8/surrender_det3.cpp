#include "surrender/srMath.h"

/* Out-of-line 3x3 determinant helper. Live query: 0x0049BD00 is a gap between
   Engine Code\stParticle.cpp (upper 0x0049AD10) and Engine Code\OctSubMesh.cpp
   (lower 0x0049E5D0). srMatrix4T<float>::Invert calls this body; it is not a
   header inline and has no proved original TU. */

// FUNCTION: WIZ8 0x0049BD00
float Det3(float param_1, float param_2, float param_3, float param_4, float param_5, float param_6,
           float param_7, float param_8, float param_9)
{
    return (param_2 * param_6 - param_3 * param_5) * param_7 +
           ((param_5 * param_9 - param_6 * param_8) * param_1 -
            (param_2 * param_9 - param_3 * param_8) * param_4);
}
