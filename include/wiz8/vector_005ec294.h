#ifndef WIZ8_VECTOR_005EC294_H
#define WIZ8_VECTOR_005EC294_H

#include "wiz8/vector.h"

class W8VectorElement005EC294;

/* Address-qualified empty derived vector whose reviewed vtable is 0x005EC294. */
class W8Vector005EC294 :
    public W8GrowableVector<W8VectorElement005EC294*> {
public:
    W8Vector005EC294(int initial_capacity);
    virtual ~W8Vector005EC294();
};                                       /* 0x10 */

#endif
