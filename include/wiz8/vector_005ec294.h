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

/* The reviewed vtable belongs directly to this template specialization. */
// VTABLE: WIZ8 0x005ec294
// class W8GrowableVector<W8VectorElement005EC294*>

void DestroyLightVector(
    W8GrowableVector<W8VectorElement005EC294*>* vector); /* 0x004A8C50 */

#endif
