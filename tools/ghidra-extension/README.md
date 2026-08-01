# Wiz8 recovery exporter (Ghidra extension)

The Java semantic engine behind `uv run wiz8 ghidra export-cpp`. Input is the
live analyzed Ghidra program; output is recovered-style C++ text with reccmp
entity markers. It is a selected-definition exporter: it emits only the chosen
definitions — no headers, no data-type definitions, no referenced-global
declarations, and no second source authority.

The printer walks the decompiler's marked-up token tree
(`DecompileResults.getCCodeMarkup()`), never the flat pseudo-C string. Every
recognizer either rewrites a construct on positive evidence or declines, in
which case the verbatim token writer reproduces Ghidra's own rendering. An
unrecognized construct or an analyzer error therefore degrades output quality
for that spot only; it never blocks the batch.

## Build and load

`wiz8 ghidra export-cpp` compiles these sources on demand into
`build/ghidra-extension/wiz8-recovery.jar` and rebuilds only when the source
hash, the Ghidra install path, or the pinned Ghidra version changes
(`stamp.json`). Compilation needs a JDK:

- `javac` is found via `JAVA_HOME`, then `PATH`. Ghidra 12.1.2 runs on JDK 21,
  so a matching JDK is normally already present; the build compiles with
  `--release 21`.
- The compile classpath is every module jar under
  `$GHIDRA_INSTALL_DIR/Ghidra/*/*/lib/*.jar`. The actual dependencies are
  `Decompiler.jar`, `SoftwareModeling.jar`, and `Utility.jar`; the glob is
  used because it is robust and free.

The jar is attached to the running JVM with
`ClassLoader.getSystemClassLoader().addPath(jar)`. Ghidra replaces the system
class loader with `ghidra.GhidraClassLoader`; a jar passed through
`launcher.add_classpaths` would land on the parent loader, which cannot see
the Ghidra module jars, and every `ghidra.*` import would fail with
`NoClassDefFoundError`. (`launcher.add_class_files` is the pre-start
equivalent PyGhidra applies through the same `addPath` call.)

## Usage

```sh
# raw C++ on stdout
uv run wiz8 ghidra export-cpp 0x004a5e50

# several functions, an inclusive entry range, or a file target
uv run wiz8 ghidra export-cpp 0x004a5e50 0x004a5f00 0x004a6610
uv run wiz8 ghidra export-cpp 0x004a5e50:0x004a6610 --output build/export-cpp/grcycle.cpp
```

With `--output` the command prints the usual summary record instead of the
text. Note the default `DecompileOptions()` are used, so rendering can differ
slightly from a CodeBrowser window whose tool options were customized.

## Manual corpus check

Java code runs only inside the live project, so it is validated against the
recovered corpus rather than by `just check`/`just test`:

```sh
uv run wiz8 ghidra export-cpp 0x004a5e50 0x004a5f00 0x004a5f20 0x004a6610 \
    0x004aded0 0x004a6e20 0x004ae270
```

Expected: every body carries its `// FUNCTION: WIZ8 0x…` marker; the scalar
deleting destructor 0x004a5f00 prints exactly the two-line `SYNTHETIC` block
from `src/wiz8/engine_code/GrCycle.cpp` and no body; ordinary functions match
Ghidra's decompiler text byte for byte.

## Lifecycle lifting

Constructors and destructors are lifted from the MSVC lowering back into
C++ when every recognizer finds positive evidence (`Msvc6Patterns.java`):

- `Class::Class(params)` / `Class::~Class()` signatures without the explicit
  `this` parameter or the lowered return type;
- leading base/member subobject constructions become initializers (base
  classes by the repository's `base`/`base_*` field convention); empty
  argument lists stay implicit;
- the compiler's null-preserving derived-to-base conversion
  (`p == 0 ? 0 : &p->base_X`) collapses back into the `(X *)p` cast;
- vftable-pointer stores into `this`, the VC6 `ExceptionList` registration
  frame (link/handler/state slots, their declarations, and every state-slot
  update), and trailing automatic member/base destructor calls disappear;
- `T::~T(x); operator_delete(x);` and a direct
  `'scalar_deleting_destructor'(x, flags)` call collapse to typed
  `delete`/`delete[]`; `x = operator_new(n); T::T(x, …);` collapses to
  `x = new T(…)`.

Every rule declines on ambiguity: a constructor or destructor whose
subobject lifecycle calls cannot all be proven prints fully verbatim, and a
recognizer failure never blocks the batch. Guarded allocation forms and
member-store initializer placement are later-milestone work; expect them
verbatim.

## Typed member access, dispatch, and type spellings (M3)

Statement-local rewrites run over the whole body — conditions and loop heads
included — before the lifecycle recognizers, which therefore read the lifted
text:

- **Type spellings** (`TypeNames.java`): the project's bracket-template
  encoding becomes C++ (`W8GrowableVector[stLight_#]` →
  `W8GrowableVector<stLight*>`, nested arguments and multi-word primitives
  included), and Ghidra primitive aliases are spelled out (`uint`,
  `undefined4` → `unsigned int`, `byte`/`undefined1` → `unsigned char`,
  `float10` → `long double`, …). `code` is deliberately left alone: a
  surviving `code` cast marks dispatch the recognizers declined.
- **Member access**: Ghidra's base-subobject navigation returns to the C++
  inheritance model — `(x->base).f` → `x->f`, `this->f` → bare `f`,
  `&this->base_X` → `this` (the implicit upcast). Only the real `this`
  parameter qualifies (`HighSymbol.isThisPointer`); a local that Ghidra
  happened to name `this` keeps its explicit accesses. `vftable` accesses
  are never rewritten — a surviving one marks declined dispatch.
- **Null constants**: `(T *)0x0` prints as `0`.
- **Direct method calls**: `Class::Method(receiver, args)` becomes member
  syntax — bare `Method(args)` on `this` or a base subobject,
  `field.Method(args)` on a member, `v->Method(args)` /
  `x->f.Method(args)` / `((T *)expr)->Method(args)` on external receivers.
  Callees with a `__return_storage_ptr__` parameter decline (struct-return
  lowering is later work).
- **Virtual dispatch**: `(**(code **)((int)recv.vftable + 0xNN))(args)`
  resolves the receiver's static class, picks its vftable symbol (the
  complete-object table only for offset 0; a `{for_'Base'}` table only via
  the matching base subobject), reads the slot from program memory, and
  prints the named call. Slot reads are bounded by the next vftable symbol
  so an off-table slot never borrows a neighbour's entry. A slot that holds
  a purecall, an unnamed function, or anything outside a class namespace
  declines. A slot holding a scalar deleting destructor with flag `1`/`3`
  prints the source-level `delete`/`delete[] receiver`.
- **Ordinary method signatures**: `void __thiscall Class::M(Class *this, …)`
  loses the convention and the `this` parameter.

Naming comes from the live project only. A pure-virtual slot cannot be named
from the vtable (it stores `purecall`), and unnamed slot functions
(`FUN_…`) decline until the project names them.

## Exception-handling lifetimes (M4)

`EhModel.java` re-derives the function's MSVC exception model from program
memory: the prolog's handler-thunk store names the `__ehhandler` thunk
(`mov eax, &FuncInfo`), and the FuncInfo record supplies the try-block count
and the unwind map, whose cleanup funclets are decoded when they have the
direct object shape (`lea ecx, [ebp+disp]` + `jmp`/`call` destructor).

- **Scaffolding suppression** runs for every function kind once the model
  resolves with zero try blocks: the registration statements
  (`ExceptionList` link/handler/state slots), their declarations, and every
  state-slot update disappear. A function whose FuncInfo declares try blocks
  keeps everything verbatim — that registration carries source semantics. An
  unresolvable model falls back to the frame-shape-only suppression that
  constructors and destructors used in M2. A completeness gate reverts the
  suppression whenever any body token would still reference a dropped slot
  (for example a state store folded into a condition).
- **Stack-local lifetimes**: an unwind state that destroys a directly
  addressed frame object places a typed local there. When the body contains
  exactly one same-class constructor call on that slot (`T::T(&local, …)`)
  and at least one direct destructor call, the constructor call becomes the
  local's definition (`T local(…);` — argument-free construction prints
  `T local;`), the hoisted untyped declaration disappears, and every direct
  destructor call on the slot disappears (source never spells the
  scope-end destructor). The registration link always sits at `ebp-12`,
  which anchors the ebp-to-decompiler stack-offset conversion.
- **Declines**: inlined constructor/destructor expansions (the common VC6
  outcome for small classes — vftable stores and member stores appear
  instead of a call), `pointer`-kind funclets (heap cleanups for `new`
  expressions), vector `__ehvec` helpers, and receivers that do not render
  as `&local` all stay verbatim.

## Assertion strings (M5)

A reference to a defined narrow-string datum prints as the quoted literal
the source contained — `srAssertFail("pInfo",
"C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x1ba, 0)` — instead
of Ghidra's synthetic `s_…_00607cf0` identifier, whose embedded path
characters cannot even lex. The bytes come from program memory, so the full
text survives Ghidra's symbol-name truncation. Only the canonical
address-of reference shape (`PTRSUB(0, address)` with no varnode) is
claimed; assignment targets and loaded values never match. Wide strings
decline. Escaping is VC6-exact, including splitting the literal when a hex
escape would swallow a following hex digit.

Deterministic renaming from assertion/alloc-failure strings with
transactional Ghidra write-back is the plan's optional M5 slice and stays
future work; the recovered literals already carry the expression, file, and
line into the exported source.

## Regression harness

`uv run wiz8 recover regress 0x004a5e50 …` measures zero-edit
regenerability: it exports each function, splices the block over the
recovered body in its owning TU (span from the source index), builds the
product, runs the relocation-masked comparison for that address, and always
restores the file. It needs the live project plus the VC6 toolchain, so it
is a manual/milestone gate, not part of `just check`/`just test`.
