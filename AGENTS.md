# Repository instructions

This is a Jujutsu repository for evidence-driven Wizardry 8 matching decompilation. Use Beads for
durable task state and `just` for the supported daily workflow.

## Authority and ownership

- Ghidra owns operational analysis state: function boundaries and names, signatures, labels,
  namespaces, comments, structures, fields, enums, vtables, applied types, cross-references, and
  decompiler state.
- Git owns recovered C++ source, declarations, translation-unit ownership, source/link order,
  compiler settings, matching markers, and provenance claims.
- Provenance explains why an identity is accepted, its origin and authority ceiling, confidence,
  aliases, and supporting observations. It must not clone Ghidra's complete function or type model.
- Generated projections under `build/` bridge canonical authorities. They are disposable and must
  never become editable evidence.
- Do not reverse engineer implementations whose source or declarations are available. Windows,
  MSVC runtime, SGP, zlib, IJG JPEG, and Info-ZIP behavior comes from pinned source/header oracles.
  SurRender is closed middleware and remains a valid recovery subject.
- For every SGP call, type, handle, constant, or record, search `third_party/sfi-sgp/sgp` first and
  include its owning header; never restate that interface in a Wizardry source or shared header.
  When a public SGP header requires a missing product header such as `Local.h` or `WizLibs.h`, use
  or extend its original include name under `include/wiz8/sgp-compat` and expose that shared
  directory to the target.
  Do not work around the dependency by inventing a compatible prototype, typedef, or struct.
- Search before declaring anything. A function, global, class, field, vtable, import, or constant
  has one canonical owner. Extend it; do not add duplicate externs, raw vtable calls, wrappers,
  guessed aliases, or parallel inventories.
- Preserve proven original translation-unit ownership and the explicit matching-sensitive order in
  `src/wiz8/sources.cmake`. Keep address-qualified template emissions separate until ownership is
  proved.
- Never commit game binaries, extracted trees, live Ghidra projects, or build products. The
  reviewed GZF checkpoints declared by `vendor/ghidra/exports/manifest.json` are the sole approved
  tracked analysis artifacts.
- Preserve the SFI-SCLA and upstream notices under `third_party/sfi-sgp`.

## Recovery loop

Restore the reviewed project once, then edit and version that project directly:

```sh
uv run wiz8 ghidra restore
uv run wiz8 ghidra sync-source --apply
just context 0x<address>
```

The context command is the supported joined interface. It combines provenance, source ownership,
match state, cross-build mappings, strings/assertions, callers/callees, decompilation, and relevant
fields. Use the Ghidra UI/API for ordinary listing, symbol, type, and cross-reference work. For a
speculative experiment, clone or version the project, use undo, or use a temporary GZF.
`ghidra sync-source` applies source-owned names and resolvable prototypes transactionally;
unresolved source types are reported explicitly rather than replaced with guessed Ghidra types.
There is no generic `just ghidra query` command.

Recover the immediate call graph needed for the next visible product transition. Tooling is retained
only when it repeatedly saves recovery work, proves emitted code or runtime behavior, or preserves
evidence Ghidra cannot express.

Use the ordinary circa-2000/2001 C++ program model as the default hypothesis. Apply Occam's razor:
prefer normal language, compiler, ABI, and container semantics over bespoke mechanisms unless
concrete evidence requires otherwise. If a routine has the shape of a regular constructor or
destructor, recover it as the regular constructor or destructor; if a layout and its operations
have the shape of a vector, list, string, or other familiar container, model that container rather
than inventing an unrelated wrapper or one-off procedure. In templated ownership code, recover the
general typed template operation and use ordinary typed construction or deletion so the compiler
emits the expected calls. Treat special calling conventions, explicit allocation/deallocation
calls, custom lifecycle helpers, and novel container abstractions as claims that require positive
evidence, not as starting assumptions.

Load the task's skill before starting: porting or near-matching a function body uses
`matching-decomp`; promoting candidate classes or deciding what an unnamed
constructor/destructor constructs uses `class-triage`.

## Matching expectations

Faithfulness is absolute; byte-identity is incremental. A first-pass port will usually not be
byte-identical, and that is an acceptable, recordable state once the body is structurally
faithful. The two hard invariants are completeness in both directions: never invent code the
retail binary does not contain, and never skip, stub, or approximate code it does contain.
Every retail instruction must be accounted for by the ported source even while the emitted
encoding still differs.

The pinned VC6 build is a falsifier, not a score generator. Static analysis, source archaeology,
and language intuition propose a source model; the emitted instruction sequence decides whether
that model survives. Run a focused comparison immediately after changing field widths or order,
signedness, return types, calling conventions, constructor/destructor shape, container ownership,
or control flow. A worse exact/effective result or an earlier structural divergence falsifies the
change unless the instruction diff proves that the measurement is stale or otherwise unrelated.
Do not reason past a compiler result merely because the proposed model looked cleaner.

Negative experiments are durable evidence. Record the symbol and address, the hypothesis, the
source shape tried, the exact comparison command, the before/after status or first divergence, and
the conclusion in the Bead. Revert source that the experiment showed to be worse; do not leave a
known-regressed shape in the recovered code, but do not erase the result with a silent revert.

Spend matching effort where it changes evidence. When the residual divergence comes down to
inliner, scheduling, or register-allocation choice, do not run an exhaustive search over
inlining combinations to win the final fraction of instruction similarity; record the
divergence and move on. Return for byte-identity when new evidence — a caller, a layout, a
compiler-flag proof — makes it cheap, not by brute force.

**Register allocation choice does not matter.** A body whose instructions correspond one for
one with the retail body, in the same order and with the same operands, is done — even when
the two disagree about which scratch register each site uses. A register permutation carries
no evidence: it does not change what the code reads, writes, or calls, and it cannot falsify a
field offset, a signedness, a calling convention, or a control-flow shape. Do not iterate on
source shape to chase one, do not report it as a defect, and do not let it hold a body back
from being recorded as recovered. Note it in a sentence and move on.

The corollary matters as much: a register difference is only ever *residual*. Register roles
that are permuted **because the control-flow shape is wrong** are a real finding, and the
tell is that the instruction sequence itself diverges — a different count, a different order,
a missing or extra test. Line the two bodies up instruction by instruction first. If they
correspond and only the register names differ, stop; if they do not correspond, the registers
were a symptom and the shape is the bug.

Marker policy follows reccmp's entity conventions (enforced by `just check`):

- `// FUNCTION:` must sit immediately above its C++ declaration.
- `// TEMPLATE:` must be followed immediately by the specialization-symbol comment and then the
  template definition.
- `// LIBRARY:` is address-only and has no owned declaration adjacency requirement.

Put explanatory prose above the marker sequence.

## Inlining policy

- Recover source placement before attempting to control optimizer decisions.
- Ordinary recovered functions remain unannotated and use the pinned compiler's normal
  optimization heuristics.
- Define a function in a header or class body only when evidence establishes that its original
  body was visible in multiple translation units.
- Do not add `__forceinline`, `__declspec(noinline)`, or inline-control pragmas to improve one
  isolated comparison score.
- An explicit inline-control annotation requires concrete call-site evidence and must improve the
  complete callee/caller/thunk comparison bundle without regressing an exact boundary.
- Treat constructors, destructors, virtual methods, deleting destructors, and vtable-related
  thunks as one ABI bundle.
- Revisit and remove explicit inline controls after later source recovery; they are not substitutes
  for correct declarations, bodies, ownership, or source order.

## Validation

Choose the narrowest lane while iterating. Run the complete applicable lane after the task is
finished and rebased onto current `main`, immediately before integration:

| Change | Required validation |
| --- | --- |
| Python, docs, evidence, marker, or source inventory | `just check` |
| C++ class declarations or inheritance | `just lint` |
| Ordinary local work | `just test` |
| Ported/recovered body | `just build WIZ8`, focused reccmp exact/effective comparison, then `just test` |
| Reviewed Ghidra change | representative `just context` checks, `uv run wiz8 ghidra index`, then an intentional checkpoint refresh |
| Complete product/integration gate | `just verify` |

`just test` is the fast lane: it refreshes the source index and runs the unit and repository
suites without rebuilding the clang lint lane. The lint build itself runs only in `just lint`,
`just check`, and `just verify`; after changing the source inventory or CMake configuration,
run `just lint` once so the source index sees a current compile database (the index refuses a
stale one).

Relocation-masked selected-function comparison is the exact/effective body authority. Whole-image
comparison is diagnostic because relocated globals can reduce its raw score even for an exact body.
Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes.

Use `just compare ADDRESS...` or `just compare --file SOURCE...` for one batch comparison and
`just triage ADDRESS...` or `just triage --file SOURCE...` for reccmp's structured first
divergence. `just vtable`, `just datacmp`, and `just addr` expose reccmp's owned
vtable/data/pairing analyses; they do not supersede function comparison.

Use `just runtime-test` for the separate in-process semantic product. Its input is queued from the
test driver but consumed by the real screen handler on the UI thread; forward/reverse normalized
observations are the determinism criterion. Do not add coordinate automation or test branches to
matching bodies.

`just check` owns Python formatting, lint, typing, source inventory, fixture-based unit tests, one
checked-tree repository lane, marker policy, repository hygiene, and reccmp decomplint. Do not put
live counts or generated reports into tests or Markdown.

`just lint` is a compile-only C++14 Clang lane over the recovered sources and the real VC6 headers.
It enforces override and overloaded-virtual diagnostics; it never supplies matching evidence or
participates in the linked product.

A red integration gate is not excused by calling it “pre-existing.” When `just verify` fails on the
topic, reproduce the same command in a clean sibling workspace at the exact rebased `main` commit.
Integration is allowed only when every narrower required lane passes and the topic adds no failure:
record the base commit, the baseline failure identities, the topic failure identities, and their
empty delta in the Bead and handoff. Do not fix unrelated baseline failures unless the task scope is
explicitly expanded.

## Workspace and publication

Give every checkout its own absolute `WIZ8_WORK_DIR`. The live Ghidra project lives inside the
checkout at `ghidra-project/`; never point two checkouts at one project and never hardlink or
copy a live project directory. Product builds use a per-checkout lock under `build/decomp`.

At session start, run `bd prime`, `bd dolt pull`, inspect `bd ready`, and claim or create a Bead
before substantial work. Record partial and negative evidence; close only after acceptance and push
Beads state. End a session by handing off what changed, how it was verified, the status of every
touched Bead, and any step that stayed blocked, naming the exact command and error.

Use Jujutsu, not raw Git, for repository history. Start substantial work from current integrated
`main` on a unique named bookmark such as `agent/<bead>-<topic>`. Keep the whole task or coherent
batch isolated there. Commits inside that stack are review and recovery checkpoints; they are not
individual integration events. Never move `main` merely because one intermediate commit passes.
The exact workspace, Beads, rebase, checkpoint-push, PR, and integration commands live in
[the contributor workflow](docs/contributor-workflow.md).

Jujutsu workspaces have separate working copies but share bookmarks and operation history. `main`
is therefore repository-global, not workspace-local. Never point `main` at unfinished, conflicted,
or partially validated work. Keep implementation on its task bookmark, and publish that bookmark
for backup or review when useful. Do not continuously rebase just because another agent advanced
`main`; rebase when upstream evidence is needed and once more immediately before integration.

Integrate a task exactly once, after its Bead scope and acceptance criteria are complete, its stack
has been rebased onto current `main`, and the final applicable gates have been run on that rebased
tip. If `main` or `main@origin` moves during the final sequence, stop, inspect the new commits,
rebase the whole task stack again, and rerun the gates affected by the rebase. Never overwrite or
push over another workspace's bookmark movement.

When direct publication to `main` is authorized, perform the final fetch, stack rebase, validation,
bookmark move, and push as one controlled integration sequence. After the push, fetch again and
prove that local `main` and `main@origin` resolve to the published commit and have an empty tree
diff. This final proof is required even when the push itself reported success.
