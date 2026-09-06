# Inheritance evidence

Test whether the existing base model explains the candidate before adding a derived declaration.
Useful independent evidence includes fields beyond the base extent, non-template behavior requiring
the boundary, an exact source/export name, and normalized base-constructor or adjustor-thunk receivers.

Construction runs the base first, initializes members, then installs the final vtable. Destruction
installs the class vtable before tearing down members and the base. An allocation size belongs to the
most-derived object. Normalize vptr and member receivers before assigning offsets; an embedded
polymorphic member can resemble a second base.

When the hierarchy changes, compare the affected constructor, complete destructor, deleting
destructor emission, vtables, adjustor thunks, and consumers whose dispatch or pointer adjustment can
change. Declaration-only compilation cannot detect a null linked vtable slot.

Do not add a same-signature virtual to a dllimport class merely to change access or narrow a return
type. Fix access at the canonical declaration or use the proven caller-side type. A new virtual may
link as a null slot under `/FORCE`.
