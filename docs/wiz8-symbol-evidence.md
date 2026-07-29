# Symbol and type evidence in a build with no debug information

Wizardry 8 shipped without symbols. Every Wiz8 executable in the corpus - demo, GOG retail, the
official 2001-12-23 retail build, and both fan-patched builds - has no PE debug directory, no COFF
symbol table, and `IMAGE_FILE_LOCAL_SYMS_STRIPPED` set. The demo is not an exception: the CodeView
records the corpus does contain all belong to library modules (`sr.dll`, the `srDD_*`/`srEXT_*`
plugins, the MSVC runtime), and each carries only a PDB *path*, never a PDB.

The game was also linked without RTTI, so its own classes have no type descriptors.

What survives is indirect. This document records how those surviving channels are produced, what
each can and cannot say, and how to join them to answer the questions recovery actually asks -
which unit a function belongs to, what type sits in a frame slot, which function to port next.

Counts are deliberately absent; `just wiz8 report status` and the producers' own JSON summaries
report those.

## Producing and refreshing

Six producers write the snapshots under `evidence/snapshots/`:

```sh
uv run wiz8 evidence refresh eh-metadata       # exception tables
uv run wiz8 evidence refresh surrender-abi     # SurRender export ABI
uv run wiz8 evidence refresh call-sites        # assertion / instance-name literals
uv run wiz8 evidence refresh polymorphism      # vtables, slots, vptr writes
uv run wiz8 evidence refresh globals           # globals and references
uv run wiz8 evidence refresh function-census   # function starts and call graph
```

Run bare, each one regenerates its report under `build/reports/` and **fails if the result differs
from the tracked snapshot**. That is the freshness gate: a producer change that silently alters
recorded evidence cannot pass unnoticed. Pass `--update-snapshot` to accept a reviewed change.

They are snapshots rather than ordinary generated reports because reproducing them needs the
proprietary binaries, which are never committed - the exception
[`evidence-policy.md`](evidence-policy.md) defines for exactly this case.

Three practical constraints:

* `surrender-abi`, `polymorphism` and `eh-metadata` require `llvm-undname` on `PATH`. Decorated
  names are decoded by a maintained demangler rather than approximated here, because a partial MSVC
  demangler fails silently on the templated and nested names that matter most.
* Every row is keyed by `program`, a `wiz8--<variant>--<module>--<hash>` identity. Builds with
  byte-identical payloads are recorded once under the canonical variant, and each producer reports
  the aliases it collapsed.
* The protected 2001-12-23 retail build is skipped wherever a conclusion would need its code. Its
  rows say so rather than carrying invented values, and it is excluded from resolution rates.

## Channels

### Exception metadata - `evidence/snapshots/eh-metadata/`

C++ exception tables survive `/GR-` because the runtime needs them to destroy locals while
unwinding. Three facts fall out.

**EH setup anchors.** MSVC emits one `__ehhandler` thunk per `FuncInfo` record (`mov eax, <record>`
then a jump into the runtime), and the owning function's frame setup pushes that thunk. Both hops
are one-to-one, so `frame_setup` and its preceding `eh_setup_start` are exact addresses inside the
owning function. They are not unconditional function entries: VC6 sometimes installs the EH frame
after an FS load, argument work, or an early-return region. A Ghidra consumer should resolve the
containing function from the setup address instead of creating a function there.

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
function identity. Exception metadata supplies a stronger interior anchor through `frame_setup`, but
the containing function still has to be resolved rather than assumed from the setup sequence.

The derived anchors are nonetheless trustworthy enough to attribute a function to a translation unit,
and the translation-unit report consumes them for that. The check that licenses it is agreement: on
the functions both sources know, the snapshot's derived enclosing function and the reviewed table's
Ghidra-resolved `containing_function` name the same unit every time. A function whose assertions name
two units has been inlined into and anchors neither.

### Polymorphism ABI - `evidence/snapshots/polymorphism/`

Vtables are found through the relocation table, which is the image's own complete index of absolute
addresses: a vtable is a run of consecutive relocated `.rdata` slots pointing into code, and a value
that is not a relocated slot cannot be part of one.

Ending a run is the part that is easy to get wrong. Two tables of one class sit adjacent, so a naive
maximal run reports a single table holding the sum of both slot counts. The rule is that a table must
be referred to in order to be used, so a run is cut at any slot address appearing as a relocated
operand in code. Splitting only at decodable constructor stores is *not* sufficient - it misses
additional vftables whose store does not decode, and those are exactly the cases where a neighbouring
count was overstated.

Vptr stores are collected separately because they carry an addressing observation a bare reference
does not. The `N` in `mov [reg+N], offset table` is only the instruction's store displacement. It is
a root-relative vptr placement only when register provenance proves that `reg` still denotes the
complete object. A shifted register can produce an arbitrary or negative displacement, so zero does
not prove a primary table and nonzero does not prove a base subobject.

Keep the evidence states separate:

1. **Additional vftable** — a distinct table boundary is proven; placement and owner are unknown.
2. **Polymorphic subobject at offset N** — root-relative placement is proven; base versus embedded
   member remains unknown.
3. **Secondary base vftable** — constructor, destructor, dispatch, adjustor, or base-pointer evidence
   proves multiple inheritance.
4. **Virtual-base vftable** — a vbptr/vbtable or imported ABI declaration proves virtual inheritance.

Slot kinds are read rather than guessed. `_purecall` is resolved through the import table by name, so
pure-virtual slots need no per-build address, and a slot pointing at a jump thunk is reported with the
demangled library method the class inherited.

### Global variables - `evidence/snapshots/globals/`

The same relocation table enumerates every global reference, because an absolute address appears in
code only if the loader fixes it up. A reference the table does not list is not a global reference,
which is what makes this exhaustive rather than pattern-matched.

The reference carries type evidence the address alone does not. The instruction's operand size is the
variable's width; whether the operand is read, written, or only taken as an address separates a
scalar from an array or structure; and the distance to the next referenced global bounds the extent
from above. A global with one consistent width and at least one write is a scalar whose size is
known.

Most of the game's mutable state lives in the uninitialised tail of `.data`, past the bytes the file
stores. Those are ordinary globals with no initialiser, reported as `storage = bss` rather than
dropped as unmapped - which is what a scan restricted to file-backed bytes would do.

Import-table slots are classified separately. They are global pointers, but the loader's rather than
the game's, and their reference counts measure calls to a library function.

The per-reference list is a generated report under `build/`, not a tracked artifact: it is an order
of magnitude larger than the per-global table and is derived from the same run.

### Function candidates and the call graph - `evidence/snapshots/functions/`

Every other channel keys on a function start, and until this one nothing enumerated them. Four
independent things attest a start and they fail differently, so the census records which agree
rather than merging them: an exception record (read through the handler thunk, the only exact
source), a direct `call`, a code address stored in data, and the byte after inter-function padding -
much the noisiest, because alignment padding also appears inside functions and after data.

Byte scanning proposes; the disassembler disposes. A candidate must be an instruction boundary in
the resynchronising linear decode, which is what rejects an address that is really data. The call
graph is built from *decoded* `call` instructions rather than a byte search for `0xE8`: a byte
search is fine for proposing a target, since each is validated afterwards, but it invents edges from
an `0xE8` inside an immediate, and nothing downstream would catch that.

A recognised prologue is recorded and deliberately **not** required. Measured against the vtable
slots the polymorphism census already proves are entry points, the prologue shapes cover about two
thirds - a leaf opening with `mov eax, [esp+4]`, or a body opening with `mov eax, fs:[0]`, has no
frame to set up - so gating on one would reject a third of the real functions.

`verdict` is `exact` for an exception record; `strong` when a call or data reference lands on an
instruction boundary; `padding-only` when nothing but padding attests it, which is kept visible but
never accepted; `decode-disagrees` when something genuinely refers to the address but the sweep did
not land on it, which flags a sweep that stayed out of phase rather than filing real evidence under
"rejected"; and `rejected` otherwise.

These are candidates, not identities. A recovered identity is accepted by putting its address marker
against the owned C++ declaration; provenance that source cannot express belongs in
`evidence/reviewed/wiz8/claims.csv`. Regenerating the snapshot can touch neither.

## Using it

Each recipe below is a join between snapshots. Addresses are canonical-build examples; the join
keys, not the values, are the point.

For a joined packet around one function, including its current Ghidra decompile and listing, run:

```sh
just wiz8 report context 0x004f88f0
```

The command writes JSON and readable Markdown under `build/reports/recovery-context/`. It joins the
function's direct or interval-inferred translation unit, assertions, EH cleanup slots, global
references, vptr writes, observed vtables and reviewed identities without creating tracked source
scaffolding or promoting observations into semantic names.

The canonical Ghidra project owns the reviewed result: vtable targets, pointer data, scalar widths,
assertion/EH comments, and instance-name annotations are edited and reviewed there. Observations do
not automatically assign class, function, field, or global names, split an existing function, or
clear conflicting code/data. Conflicts remain explicit review work.

### Which translation unit does this function belong to?

`call-sites/assertions.csv` pairs a `function_start` with the `source_path` its assertions name.

    0041aa40  ->  C:\Projects\Wizardry 8\Engine Code\GameData.cpp

The translation-unit report already consumes this, so `build/reports/translation-units/` answers the
question for a whole address range, including functions that carry no assertion of their own but
fall inside a unit's interval. Prefer that for anything range-shaped; use the raw table when you want
the direct evidence rather than an interval inference. A function whose assertions name two units
was inlined into and anchors neither.

### Which function should I port next to prove a layout?

This is the `wiz8-8ga.4` loop, and it is now a query rather than a hunt. Rank assertion sites by the
size of their containing function - the distance to the next `function_start` - and keep the ones
whose expression dereferences a member:

    ~ 80B  004f30f0  Controls.cpp:399    m_uiRegionSetId != REGSET_NULL
    ~ 96B  004503c0  3dapi.cpp:976       pWorld->psrCamera
    ~112B  00520880  PC Item.cpp:4003    pPCItem->iItemNo != -1

The first row is a useful check on the method: `004f30f0` is the source-owned byte proof for
`Controls`, so the ranking rediscovers a function that was found by hand and used
successfully for exactly this purpose. Port the body, then confirm the paired entity with reccmp's
relocation-masked exact mode; linked-image percentage alone remains diagnostic.

### What type is the object at `[ebp-N]` in this function?

`eh-metadata/unwind.csv` places a destructor on a frame slot, and names the type outright when the
destructor is a library import:

    FuncInfo 005effa0  [ebp-0x144]  ->  public: __thiscall srStringTable::~srStringTable(void)

Join `functions.csv` on `funcinfo`, then resolve the function containing its exact `frame_setup`
address in Ghidra. Do not create a function at `eh_setup_start`; that sequence may occur after work
already performed by the owning function.

The reviewed Ghidra project types stack variables only for slots whose class is proven: a
demangled library destructor import, or a destructor the reviewed identity layer already assigns
to a class. The example above decompiles as a
`srStringTable` local named `eh_srStringTable_144`, constructed and destroyed in place. A slot VC6
reuses for differently-typed temporaries - or shares with a destructor no evidence names - is
left untyped. The funclet's
`[ebp-N]` is relative to the frame pointer the EH runtime passes, which is the stack pointer just
before the owning function pushes its registration node. Ghidra's stack analysis handles both VC6
prologue shapes (classic `push ebp` and the frameless ESP-relative form).

### What does this vptr store prove?

`polymorphism/vptr-writes.csv` gives the raw `store_displacement`; `vtables.csv` aggregates those
displacements per table. Neither classifies the table. Follow the receiver from entry `this` through
register copies and `lea` adjustments to establish a root-relative placement. Then use lifecycle and
dispatch evidence to decide whether the placement is a base or an embedded polymorphic member.

Treat a slot count as bounded by `boundary`. A table ending in `code-reference-boundary` was split
because the next address is referred to from code - that is what separates adjacent tables of one
class, and skipping it is how three reviewed counts came to be the sum of a table and its successor.

### What is this global?

`globals/globals.csv` describes an address without needing a name:

    0068edcc  storage=bss  widths=4  access_kinds="read write"  extent_bytes=4

One consistent width and at least one write is a scalar of known size; only ever address-taken means
an array or structure; `extent_bytes` bounds the size from above. `kind` separates strings, code
pointer tables and import slots from ordinary data. The full per-reference list, including the
referencing function of every access, is generated under `build/reports/globals/references.csv`.

### What is this function called in the demo?

`eh-metadata/functions.csv` carries `unwind_signature`, which excludes every address, so the same
source compiled into another build hashes the same. Group by signature, keep the ones that occur
once on each side, and resolve the function containing each row's `frame_setup`:

    demo 0041cc70  <->  gog-base 0041c930

These are *candidates*. They belong in `evidence/reviewed/cross-build/mappings.csv` only after
review, like any other candidate.

### Which functions does this one call, and who calls it?

`functions/calls.csv` is the deduplicated call graph, one row per `(caller, callee)` with the number
of sites. `caller` is the accepted function containing the call site, so an edge is only as sound as
the boundary beneath it - which is why the census emits candidates and edges together rather than
either alone. Joining callees against the unit attribution above suggests an owning unit for
functions that carry no assertion of their own, but a callee reached from one unit is weaker
evidence than a function whose own assertion names it, and belongs at a lower confidence.

### What does this SurRender call expect?

`surrender-abi/exports.csv` carries a demangled signature and calling convention for every exported
symbol, so a call into `sr.dll` can be typed exactly rather than guessed. `virtuality` and
`vftable_base` describe the library's polymorphism, which is what lets a first-party class that
derives from a SurRender base be modelled with the right vtable shape.

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
