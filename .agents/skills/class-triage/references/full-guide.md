---
name: class-triage
description: Deciding what a Wizardry 8 class really is before writing its declaration. Use when an unnamed constructor/destructor needs an owner, when a vtable looks like a new class, when choosing between a real subclass and an inherited template emission, or when a registry/clone method appears to belong to a class.
---

# Class triage: deciding what a class is before declaring it

Source is the model. There is no reviewed `classes.csv`/`vtables.csv`/`vtable-slots.csv` layer any
more, no `WIZ8_GAMEPLAY_BOUNDARIES` target, no `verify-boundaries`, and no `just ghidra query`.
A class exists when its C++ declaration exists and the compiler reproduces the retail bytes.

The joined evidence view is one command:

```sh
just context 0x<address>                       # wiz8 by default
just context 0x<address> wiz8--gog-base--sr--cec1caf85861   # SR.DLL
```

It writes `build/reports/recovery-context/<addr>.md` with the decompilation, translation-unit
ownership, callers/callees, strings/assertions, globals, and current match state. Use the Ghidra
UI/API for ordinary listing, symbol, type and xref work.

## The first question: is this a class at all?

Before inventing a class for a vtable, decide which of these it is.

1. **A real class with its own storage or independently evidenced behavior.** It has fields past
   its base's extent, a non-template operation that requires the boundary, or an exact source or
   exported symbol that names the class. Declare only what that evidence establishes.
2. **A real zero-storage subclass proved independently of compiler emissions.** This is possible,
   but a distinct vtable, final vptr write, deleting destructor, construction-phase table, registry
   bundle, or constructor that only installs a table is not proof. Require a source/export name or
   non-template behavior that cannot belong to the base/template instantiation before declaring it.
3. **Not a class.** The apparent boundary and "methods" are emissions of a template the canonical
   type instantiates. Declare nothing; record `// TEMPLATE:` markers.

Case 3 is the one that has cost this repository the most rework, so check it first.

### The srClassSupport test

`srClassSupport<Derived, Base, RegistrationFlag, ClassID>` supplies `getClassName`, `getClassID`,
`getClassNode`, `clone`, instance registration in its constructors, and unregistration in its
destructor. A class that derives from it inherits all of that as **client-emitted template code in
this binary**, not as anything it declares.

So when a candidate class appears to define the identity trio or `clone`:

- Read the retail `getClassNode` body. If it is the generic shape -- look up `ClassID`, and on miss
  register `(Derived::sGetClassName(), Base::sGetClassNode(), ClassID, flag)` with the base chain
  inlined -- it is a template emission, whatever address-qualified name the candidate carries.
- The decorated vtable base names prove the instantiation outright. SR.DLL exports, for example,
  ``srLight::`vftable'{for_`srClassSupport<srIlluminator,srNode,0,4608>'}``. When a name like that
  exists, the instantiation is not a hypothesis.
- Give the emissions `// TEMPLATE:` markers plus the instantiation-symbol comment. Nothing needs to
  follow them; the generic definition lives once in the header. A *body* under a `TEMPLATE` marker
  means an emission was mistaken for a hand-written method.

`tests/repository/test_class_abi.py` enforces this: a class whose own base is
`srClassSupport<ThatSameClass, ...>` may not declare `getClassName`, `getClassID` or
`getClassNode`, and no `srClassSupport<...>` address may carry a `FUNCTION` marker.

`clone` is deliberately outside that rule. The template's clone copies memberwise through `Derived`,
which is wrong for a class holding state its base's assignment cannot carry, and such a class
overrides it for real -- `stTextureAnim` assigns through `srTexture` and then copies each playback
field, and is byte-exact that way. Decide clone per class from its retail body, not from the base.

Do **not** preserve a zero-storage wrapper on lifecycle or vtable evidence alone. The wrapper can
be real only when evidence independent of those compiler effects establishes its authored
boundary; inherited template emissions and a distinct final table do not do so.

### The registration flag

The last argument to `srRegistry::registerClass` is **not** C++ abstractness. SR.DLL's
`registerClass` (`0x1000EC60`) calls `0x1000F7E0` only when it is non-zero, and that routine
allocates the node's own instance indices into `ClassNode+0x18` and `ClassNode+0x20`. Concrete,
constructed classes appear on both sides of it, so do not infer abstractness, and do not rename the
template parameter to a semantic guess.

## Deciding a hierarchy, not one class at a time

A registry chain is a hierarchy claim, and the compiler will falsify it. Convert a chain
**bottom-up**, one layer per experiment, comparing the whole ABI bundle each time:

```text
getClassName  getClassID  getClassNode  clone
constructor   complete destructor        deleting destructor
primary vtable            secondary vtable where applicable
```

**The clone slot returns `srClass*` at every level.** `srClass::clone` at `0x1000E860` is a
non-virtual forwarder that tail-calls vtable offset `0x1c`, that is slot 7, and returns `srClass*`,
so srClass itself types the slot. Reconstructing the template's clone as `Base*` instead makes each
level return its own base type, every level below narrows, and VC6 rejects the chain with **C2555**.

So treat a C2555 here as a defect in the reconstruction rather than proof the template is absent,
and check the return types before concluding a level cannot be an instantiation. Changing them is
byte-neutral: a return type cannot change a pointer return in EAX. Change one layer at a time, read
which declaration the compiler names, and record the negative result.

## Lifecycle reading

The decompiles settle these, in order of value:

1. **Constructor versus complete destructor.** Construction runs base first, then own fields, then
   installs its own vtable. Destruction installs its own vtable *first*, then tears down members,
   then runs the base destructor. Getting this backwards inverts a hierarchy silently. The base
   comes free: the constructor's first call is the base constructor.
2. **Object size**, from `push N; call operator new` immediately before the constructor. The size
   belongs to the *most-derived* class being constructed, so a hint sitting on an abstract base is
   really a derived class's size.
3. **Subobject placement.** A raw memory displacement is not an object offset until the receiver is
   normalized to the complete-object root. Two vtable writes in one body may be an object and an
   embedded polymorphic member rather than a hierarchy. Prove both receivers first.
4. **A name, only if the program provides one.** Registry `getClassName` strings, assertion text,
   source paths. Otherwise keep an address-qualified positional name; offsets are evidence, names
   are not. A class may register under a SurRender base's name, so treat a registry string as this
   class's name only when the getter belongs to this class's vtable.

### Deleting destructors are compiler output

Never declare, define, or call a `ScalarDeletingDestructor` method or shim. A slot-0 virtual call
with the deleting flag is the lowering of a source-level typed `delete`. Declare an ordinary
virtual destructor, write `delete object`, and let VC6 emit the wrapper. Mark the wrapper
`// SYNTHETIC:` with its decorated name; never `// FUNCTION:`, and never let it bind to an authored
declaration. The ABI gate enforces both.

### Never add a virtual to a dllimport class to change access or return type

Adding a new virtual declaration to a class carrying a class-level `SR_DLL_IMPORT` produced a
**null vtable slot**: `/FORCE` linking zero-filled the unresolved reference instead of failing, and
only the linked-vtable comparison caught it. If a caller cannot reach an inherited method, fix
access in the template that declares it, or cast at the call site. Do not introduce a
same-signature override to move access, and do not narrow a return type that way.

## Validation for any inheritance or virtual change

`just lint` compiles declarations; it cannot see a null slot or a lost body. Run the bundle:

```sh
just lint
just build WIZ8
just compare 0x<addr>...        # or --file src/wiz8/<unit>.cpp
just vtable <ClassFilter>
just test
```

`just verify` remains the integration gate. Read `just compare`'s status
(`exact`/`effective`/`mismatch`/`inconclusive`), not the raw percentage; relocated operands and
scratch-register choices depress it even for exact bodies. `just compare` gives the first structured
divergence.

**A depressed score is not automatically register noise.** Line the two bodies up instruction by
instruction before saying so. A `call` where retail has an inlined field load is a structural
difference that looks superficially like register churn in a summary number -- that misreading
shipped a regression to `main` once already.

**Relink before trusting reccmp.** A successful `just build` does not guarantee the link reran;
delete `Wiz8.exe` and `Wiz8.pdb` and rebuild if a number looks implausible. Check
`RECCMP_PROJECT_DIR_HOST` in `build/decomp/CMakeCache.txt` if you suspect another checkout's build.

## Pitfalls that keep recurring

- **VC6 emits a class's vtable and destructors only in units that construct it.** A destructor-only
  port compiles to nothing and its addresses go missing. Keep a constructing site in the same unit.
- **An empty derived-destructor-shaped body over a correctly sized base** can prove the vptr offset
  and base extent, but it does not by itself prove that the original source authored a derived
  class; a template instantiation can emit the same lifecycle shape.
- **Member initializers versus body assignments is visible in the output.** Same size and
  instruction count with one instruction misplaced usually means the vptr store landed in the wrong
  scheduling group; move field stores into the initializer list.
- **A destructor-side vtable store does not prove an authored derived class.** It establishes the
  vptr offset and a lifecycle phase, but an ordinary template instantiation can emit the same
  store and table. Count a hierarchy only from independently proved storage, behavior, or source
  identity; constructors and destructors are compiler-emission evidence, not sufficient identity
  evidence by themselves.
- **No EH frame in the canonical is a hard constraint.** Under `/GX` VC6 adds an unwind frame the
  moment something that can throw runs while a subobject still needs destroying. If your port grows
  a frame the target lacks, you are modelling subobject destructors the original does not have.
- **A null check before `operator delete` means the source said `delete p`**; the operator form does
  not null-check.
- **Each `delete` in a destructor names its member's type shape.** Through slot 0 with `push 1` is a
  virtual destructor; a direct destructor call then `operator delete` is non-virtual; a null check
  then a bare `operator delete` is a declared-but-empty destructor; a bare `operator delete` with no
  check is trivially destructible. The last two differ by one instruction.
- **`return p != 0;` and `if (!p) return 0; return 1;` are different bodies** -- one computes a
  flag, the other branches and returns literals.
- **Many writers is usually not ambiguity: sort them by size.** Thunks and bodies that inlined the
  constructor crowd out the single out-of-line copy, which is the only claimable body. Two writers
  of the *same* size is the case that needs a file per emission.
- **Scattered addresses do not break a family.** VC6 emits per-unit COMDATs and does not fold
  duplicates, so a destructor copy can sit far from its constructor. Tie a family together by which
  constructor installs the table, not by adjacency.
- Identical slot counts and identical sizes do not make two classes twins. Twins have identical
  *bodies*.
- Do not hand-edit anything under `build/reports/`; regenerate it.
