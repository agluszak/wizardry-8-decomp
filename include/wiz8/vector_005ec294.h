#ifndef WIZ8_VECTOR_005EC294_H
#define WIZ8_VECTOR_005EC294_H

#include "wiz8/vector.h"

class stLight;

typedef W8GrowableVector<stLight*> W8LightVector;

/* The reviewed vtable belongs directly to this template specialization. */
// VTABLE: WIZ8 0x005ec294
// class W8GrowableVector<stLight*>

void DestroyLightVector(W8LightVector* vector); /* 0x004A8C50 */

#endif
