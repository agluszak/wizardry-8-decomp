#ifndef WIZ8_VECTOR_005ECE60_H
#define WIZ8_VECTOR_005ECE60_H

#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

/* The Spells.cpp host destructor at 0x004AB1C0 walks every element and hands
   it directly to DestroyLightVector.  That closes the formerly anonymous
   element type without changing this vector family's reviewed vtable. */
typedef W8LightVector W8VectorElement005ECE60;

// VTABLE: WIZ8 0x005ece60
// class W8GrowableVector<W8VectorElement005ECE60*>

#endif
