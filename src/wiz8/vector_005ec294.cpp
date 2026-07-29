#include "wiz8/vector_005ec294.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   This class is constructed in many places - nine functions install
   0x005EC294 between them - but only 0x00484870 is an out-of-line
   copy of the constructor. The rest are six-byte thunks and large bodies that
   inlined it, which is what an inline constructor looks like when VC6 emits a
   copy for some call sites and folds it into others. Only the out-of-line copy
   is claimed here; a second emission would need its own file, and there is not
   one.

   The template destructor stores the base table 0x005EC298, not the
   0x005EC294 the constructor installs. The two deleting wrappers are
   compiler-generated; the markers below account for them separately without
   pretending that an empty wrapper destructor was written in source.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

// FUNCTION: WIZ8 0x00484870
W8Vector005EC294::W8Vector005EC294(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC294*>(initial_capacity)
{
}

// SYNTHETIC: WIZ8 0x00451ce0
// W8GrowableVector<W8VectorElement005EC294*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451d10
// W8Vector005EC294::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451d30
// W8GrowableVector<W8VectorElement005EC294*>::~W8GrowableVector<W8VectorElement005EC294*>
