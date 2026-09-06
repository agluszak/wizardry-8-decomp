# Layout evidence

Use emitted code to settle layout and type facts:

- allocation size immediately before construction bounds the most-derived object size;
- stack-frame use bounds a by-value local;
- signed/unsigned branches type comparisons;
- byte-register operations expose byte-sized fields and returns;
- constructor base calls, vptr replacement, destructor restoration, adjustor thunks, and normalized
  base receivers establish subobject layout and inheritance.

Assertions may name or suggest fields, but the asserting body's memory operands place them. Normalize
each receiver to the complete-object root before treating a displacement as an object offset. Two
nearby vptr writes may describe an object and an embedded polymorphic member. Require lifecycle and
dispatch evidence before inferring multiple inheritance.

Recover the canonical declaration and let the affected compiler output test it. Do not introduce an
opaque wrapper or duplicate type merely to make a cast convenient.
