# Template emission

`srClassSupport<Derived, Base, RegistrationFlag, ClassID>` supplies registry identity, class-node
lookup, ordinary clone behavior, constructor registration and destructor unregistration. Those
client-emitted bodies do not prove that `Derived` authored duplicate methods.

Check decorated base names and the retail `getClassNode` shape. A generic ClassID lookup followed by
registration through `Derived::sGetClassName()` and `Base::sGetClassNode()` is template evidence.
Mark emitted instantiations with `TEMPLATE` and keep the generic definition in its canonical header.

Treat `vInstance` and `clone` as distinct ABI operations. Vtable slot 6 is `vInstance()`: the
virtual factory. Vtable slot 7 is `clone()`: ordinary `srClassSupport` clone behavior calls
`vInstance()` and then assigns the supported class state. `srClass::instance()` and
`srClass::clone()` are nonvirtual wrappers around those slots. Preserve the operation retail calls;
do not substitute a constructor, copy constructor, `vInstance`, `instance`, or `clone` merely to
satisfy a lint or improve codegen.

The virtual factory and clone slots return `srClass*` throughout the hierarchy. Do not introduce
covariant derived return types to avoid a cast: reconstructing the slot as `Base*` can cause VC6
C2555 errors in derived instantiations. A direct `static_cast` between modeled base and derived
record pointers/references is ordinary C++ inheritance and does not by itself indicate a source-model
disagreement. Type erasure through `void*`/byte pointers, `reinterpret_cast` between unrelated
records, or other provenance-hiding conversions remain model debt.

Never hand-write a scalar- or vector-deleting destructor. Declare an ordinary virtual destructor and
use typed `delete` or `delete[]`; VC6 owns the deleting wrapper. Mark the wrapper with `SYNTHETIC` and
no declaration/body. Give a separately emitted ordinary destructor its own `FUNCTION` marker and a
template destructor emission its own `TEMPLATE` marker. When retail emits no standalone ordinary
destructor, do not invent an address for one.

A deleting destructor, construction-phase table, final vptr write, or zero-storage lifecycle body is
compiler/template evidence until storage, behavior, registration/receiver facts, or source identity
independently establishes an authored boundary.

`RegistrationFlag` controls registry instance-index allocation; it does not represent C++ abstractness.
`ClassID` identifies the registered class.
