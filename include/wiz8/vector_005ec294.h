#ifndef WIZ8_VECTOR_005EC294_H
#define WIZ8_VECTOR_005EC294_H

#include "surrender/srNode.h"
#include "wiz8/vector.h"

/* The GrCycle teardown calls srNode::setParent on every element and reads one
   later positional field before unlinking it from the render world. That
   proves the srNode base without proving the original light subclass name. */
class W8VectorElement005EC294 : public srNode {
public:
    int world_link_234() const { return field_234_; }

private:
    unsigned char unknown_138_[0xfc];
    int field_234_;                       /* 0x234 */
};

/* Address-qualified empty derived vector with its own reviewed type identity. */
// VTABLE: WIZ8 0x005ec294
class W8Vector005EC294 :
    public W8GrowableVector<W8VectorElement005EC294*> {
public:
    W8Vector005EC294(int initial_capacity);
};                                       /* 0x10 */

void DestroyVector005EC294(W8Vector005EC294* vector); /* 0x004A8C50 */

#endif
