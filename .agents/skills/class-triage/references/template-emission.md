# Template emission

`srClassSupport<Derived, Base, RegistrationFlag, ClassID>` supplies registry identity, class-node
lookup, ordinary clone behavior, constructor registration, and destructor unregistration. Those
client-emitted bodies do not prove that `Derived` authored duplicate methods.

Check decorated base names and the retail `getClassNode` shape. A generic ClassID lookup followed by
registration through `Derived::sGetClassName()` and `Base::sGetClassNode()` is template evidence.
Mark emitted instantiations with `TEMPLATE` and keep the generic definition in its canonical header.

Treat `clone` separately when member state requires behavior beyond base assignment. The virtual
clone slot returns `srClass*` throughout the hierarchy; reconstructing it as `Base*` can cause VC6
C2555 errors in derived instantiations.

Never hand-write a scalar-deleting destructor. Declare an ordinary virtual destructor and use typed
`delete`; VC6 owns the deleting wrapper. A deleting destructor, construction-phase table, final vptr
write, or zero-storage lifecycle body remains compiler-emission evidence until storage, behavior, or
source identity independently proves an authored boundary.

`RegistrationFlag` controls registry instance-index allocation; it does not represent C++ abstractness.
`ClassID` identifies the registered class.
