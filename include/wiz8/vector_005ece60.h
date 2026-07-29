#ifndef WIZ8_VECTOR_005ECE60_H
#define WIZ8_VECTOR_005ECE60_H

#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

/* The Spells.cpp host destructor at 0x004AB1C0 walks every element and hands
   it directly to DestroyVector005EC294.  That closes the formerly anonymous
   element type without changing this vector family's reviewed vtable. */
typedef W8Vector005EC294 W8VectorElement005ECE60;

class W8Vector005ECE60 :
    public W8GrowableVector<W8VectorElement005ECE60*> {
public:
    W8Vector005ECE60(int initial_capacity = 5);
    virtual ~W8Vector005ECE60() override;
};                                       /* 0x10 */

#endif
