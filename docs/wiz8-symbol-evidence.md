# Symbol and type evidence in a build with no debug information

Wizardry 8 shipped without symbols. Every Wiz8 executable in the corpus - demo, GOG retail, the
official 2001-12-23 retail build, and both fan-patched builds - has no PE debug directory, no COFF
symbol table, and `IMAGE_FILE_LOCAL_SYMS_STRIPPED` set. The demo is not an exception: the CodeView
records the corpus does contain all belong to library modules (`sr.dll`, the `srDD_*`/`srEXT_*`
plugins, the MSVC runtime), and each carries only a PDB *path*, never a PDB.

The game was also linked without RTTI, so its own classes have no type descriptors.

What survives is indirect, and this document records what each surviving channel can and cannot
say. Counts are deliberately absent; `just wiz8 report status` and the producers' own JSON summaries
report those.

## Channels

### Exception metadata - `evidence/snapshots/eh-metadata/`

C++ exception tables survive `/GR-` because the runtime needs them to destroy locals while
unwinding. Three facts fall out.

**Function extents.** MSVC emits one `__ehhandler` thunk per `FuncInfo` record (`mov eax, <record>`
then a jump into the runtime), and the function's frame setup pushes that thunk. Both hops are
one-to-one, and VC6 emits `push -1; push <thunk>` as the function's first instruction, so the entry
point is *read*, not guessed. This is the only boundary evidence in the project that needs no
heuristic at all.

**Placed local objects.** Each unwind state points at a cleanup funclet, and a funclet names both an
`ebp`-relative slot and the destructor that runs on it. A row therefore states "in this function, at
this frame offset, there is an object of the type this destructor belongs to". When the destructor
is a SurRender import the type is named outright; when it is first-party the destructor address is
still a strong class anchor.

**Caught types.** A `catch` of a class type still needs a `TypeDescriptor`, so a build that kept one
kept the decorated class name with it. Only the demo did: it retains a single handler, catching
`srIOManager::Error` by reference. Retail has no try blocks at all, which is a real demo-to-retail
difference rather than a gap in the extraction.

**Cross-build joining.** `unwind_signature` hashes a record's shape - state count, transitions and
frame offsets - with every address excluded, so the same source compiled into another build hashes
the same. Signatures that are unique on both sides pair demo and retail functions with no
instruction matching. This is a *candidate* relationship and belongs in
`evidence/reviewed/cross-build/mappings.csv` only after review, like any other candidate.

### SurRender export ABI - `evidence/snapshots/surrender-abi/`

`sr.dll` exports far more than the game imports, and every exported name is a decorated MSVC symbol.
Decoding them yields class names, member names, access, `virtual` versus `static`, calling
conventions, and - from exported vftable symbols - the base subobject each vftable belongs to, which
makes the library's inheritance edges explicit.

This is declaration evidence about linked library code, which
[`evidence-policy.md`](evidence-policy.md) prefers over recovering the library's bodies. Nothing in
the producer disassembles SurRender.

Signatures come from `llvm-undname` rather than a demangler written here, because a partial MSVC
demangler fails silently on exactly the templated and nested names that matter. The local parser
covers only what a signature string does not expose as a field, and the two are cross-checked: any
row whose structural class name is missing from the demangled text is reported. Reading a vftable's
base from the decorated name directly is a trap - `??_7X@@6B0@@` uses a back-reference and names `X`
itself, not a base called `0` - so that column is taken from the demangled form.

### Diagnostic call sites - `evidence/snapshots/call-sites/`

`srAssertFail(expr, file, line, msg, ...)` and `srRuntimeClass::setName(name)` are passed original
developer strings. Both are recovered statically: the arguments are pushed immediately before the
call, so disassembling a short window that ends exactly on the call recovers them. No decompiler is
involved, which is why every build is covered rather than only the one with a Ghidra project.

Three things this channel adds beyond the reviewed retail assertion table:

* **Register-indirect sites.** Calls that load the import slot into a register first are invisible to
  a byte search for the call encoding, and they are not rare.
* **Header units.** Assertions inside header-declared inline members name a `.hpp`. Those units never
  appear in the translation-unit path list, so they are visible nowhere else.
* **The message argument.** Messages frequently name the owning class in prose where the expression
  only names a member.

`function_start` in this snapshot is *derived* - the first byte after the nearest preceding padding
run, accepted only when it begins a recognised prologue - and is not a substitute for a reviewed
function identity. The exception-metadata snapshot's `function_start` is read rather than derived and
should be preferred wherever both cover the same function.

## What these channels cannot do

* They do not recover first-party class *names*. No Wiz8 class name survives anywhere in the image:
  the six imported vftable symbols are pure SurRender in every build, with no first-party name in any
  template argument. Class names still come from assertion text, source paths, and behaviour.
* `srRuntimeClass::setName` names *instances*, not types, and only the calls passing a literal are
  recoverable - the ones formatting into a buffer first are recorded with an empty name.
* The protected 2001-12-23 retail executable yields readable tables but no readable code, so its
  cleanup funclets and call sites cannot be decoded. Its rows record that rather than guessing, and
  they are excluded from resolution rates.

## Unresolved

* `srEXT_Inspector.dll` ships in the demo and exports `srNode::dumpHierarchy`, `srCore::dump`,
  `srHeap::dump` and `srExtension::dumpAll`. Loaded against a running build it would attach the
  runtime names above to a live scene graph. Whether it can be loaded at all is tracked separately.
* Whether unwind-signature pairs survive review as cross-build identities is untested; only the
  candidate relationship exists so far.
