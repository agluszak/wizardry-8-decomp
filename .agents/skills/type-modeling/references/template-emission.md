# Template emission

`srClassSupport<Derived, Base, RegistrationFlag, ClassID>` supplies registry identity, class-node
lookup, ordinary clone behavior, constructor registration and destructor unregistration. Those
client-emitted bodies do not prove that `Derived` authored duplicate methods.

Check decorated base names and the retail `getClassNode` shape. A generic ClassID lookup followed by
registration through `Derived::sGetClassName()` and `Base::sGetClassNode()` is template evidence.
Mark emitted instantiations with `TEMPLATE` and keep the generic definition in its canonical header.

## Emission identity is weaker than source type identity

A retained template body proves only the source facts that its generated instructions distinguish. It
does not automatically prove the exact template arguments written in the original source. In VC6, many
constructors, destructors, clears, lookups, storage operations and trivial copies are identical for
different 32-bit scalar, pointer, enum or trivially handled record types. A recomp PDB/source-index name
for such an emission describes the current reconstruction; it is not independent retail evidence for
that same spelling.

Recover template arguments from type-sensitive source evidence instead:

- what callers insert, retrieve, dereference, compare or pass onward;
- element construction/destruction and ownership;
- allocation/copy strides that depend on `sizeof(T)`;
- overload resolution or operations whose generated code depends on the type;
- independently obtained original decorated symbols or accepted source oracles.

When two plausible template argument sets emit the same retail body, prefer the simpler authored source
model that matches the typed producer/consumer semantics. Do not preserve pointer-to-integer round trips,
record reinterpretations or other casts merely because one body-equivalent emission was first labeled
with an integer specialization.

A `TEMPLATE` marker therefore binds a retail emission address to a template implementation family. Its
human-readable comment may name an exact specialization only when independent evidence establishes it;
otherwise describe the family/member without pretending the arguments are known. Constructor/destructor
ownership must not be used to force unrelated use sites to adopt an arbitrary ABI-equivalent
specialization.


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

Never hand-write a scalar- or vector-deleting destructor. Use ordinary virtual destruction and typed
`delete` or `delete[]`; VC6 owns the deleting wrapper. Mark the wrapper with `SYNTHETIC` and no
declaration/body. A separately emitted authored ordinary destructor has a `FUNCTION` marker;
an implicit non-template destructor normally has a marker-only `SYNTHETIC` identity. Template
destructor emissions have `TEMPLATE` markers at their canonical owner. Do not add an explicit
special-member declaration merely to force emission. When retail emits no standalone ordinary
destructor, do not invent an address for one.

A deleting destructor, construction-phase table, final vptr write, or zero-storage lifecycle body is
compiler/template evidence until storage, behavior, registration/receiver facts, or source identity
independently establishes an authored boundary.

`RegistrationFlag` controls registry instance-index allocation; it does not represent C++ abstractness.
`ClassID` identifies the registered class.
