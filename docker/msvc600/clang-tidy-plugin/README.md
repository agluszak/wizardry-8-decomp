# Whole-program scalar facts

`ScalarFacts.h` owns canonical declaration/parameter identity and the fact writer.
Concrete template specializations carry their canonical Clang owner identity:
members of different `Array<T>` instantiations never share a node merely because
they occupy the same template source location. Uninstantiated dependent owners
do not provide concrete storage nodes. Shared editable source atoms still require
agreement across every specialization before a patch can change them.
`ScalarFacts.cpp` collects source declarations, transfers, constants, operations,
body coverage, type atoms and source hashes, comparison/mask operands,
typedef/pointee identities, callback slots, and escapes across the supplied translation units. Parameters use
canonical function identity plus parameter position, including unnamed header
parameters and differently named definitions. Function-return nodes, fields and
locals participate in the same graph. Dependent/incomplete template types are
excluded until a concrete layout exists; member pointers retain pointer domain.

`BoolDomainVisitor` adds the existing byte-domain annotations. Pending typed-callee
escapes and body coverage use the canonical declaration's file, line, column and
qualified name, so same-name/same-arity overloads cannot supply each other's body.
Fixed directly spelled `unsigned char[N]` fields have one indexed byte-domain
owner. Indexed assignments and truth tests contribute to that owner; exact
zero/one whole-array fills are understood. Partial fills, raw-pointer escapes,
numeric element operations and non-boolean writes block a candidate. This is
observed source-domain evidence, not proof of initialization or original spelling. The bool solver
reports byte-domain observations with the same naming, suppression, escape and
missing-body rules. Generic escapes and source type metadata do not establish
historical types. These observations remain review guidance; actual compiler
errors fail the wrapper before potentially incomplete facts are read.

`scalar_facts.py` aggregates the shared facts and implements the report-only
integer solver and other clients of the same constraint graph. Width, signedness and semantic domain are separate properties;
the current C++ spelling is retained as observation metadata. Current AST types
and arithmetic are not independent retail evidence. Ordinary source copies,
returns and call arguments are directed transfers; arithmetic is an operation to
review. Each property interprets a transfer by its semantic class (see
[Property-specific propagation](#property-specific-propagation)). The collector
also records pointer, enum, character, bool and floating domains, but the integer
solver preserves those domains. A separate known-enum domain solver accepts
independent header/symbol/source evidence for an enum already present in the
facts. It propagates that identity through pure copies, blocking bare numeric
producers, different enums, operations and escapes. It cannot invent an enum
from a numeric range.

The domain client distinguishes mask use from exclusive comparisons and scalar
arithmetic. It propagates finite values directionally, reports boolean-valued,
tri-state and sentinel-bearing chains, and keeps incomplete producers/cycles
unknown. These observations do not invent enums or turn status returns into
bool. Negative and all-ones sentinel production **or comparison** anywhere in a
component blocks unsigned propagation pending domain review.

The pointer client accepts an independently proven existing object pointee and
checks complete copy chains through `void*`, including explicit void/object
conversions. Conflicting pointees, qualifiers, address-taking, serialization,
aggregate escapes, non-null numeric producers and unreviewed operations block
promotion. By-value pointer passing is distinguished from pointer-storage
mutation; function and member pointers are outside this client.

The nominal client accepts independently owned existing typedefs and reviewed
roles (`ID`, `index`, `count`, `handle`, `timer`, `status`, `flags`). Equal width is
not sufficient: representation, existing owner identity, uses and sentinels must
agree. It never invents wrapper classes or normalizes int/long spelling.

Callback fields/variables have synthetic argument and return nodes in the same
graph. Resolved function bindings and indirect invocations connect implementation
parameters/returns to callers across TUs. Arity, calling convention and variadic
mismatches are blockers. This models signature dependencies; it does not claim
complete callback-table/ABI recovery or remove the bool solver's existing escape
rules. Unresolved callback producers, external ABI slots and width changes remain
blockers. Floating and class-ownership clients remain future work.

## Property-specific propagation

The solver classifies every `F` transfer from both endpoint declarations:
`copy`, `sign-change` (same width, different signedness), `alias-change`
(different typedef, same representation), `binding`
(callback slot ABI), `widening`/`narrowing`, `domain-change`, `erasure`
(`T*` to `void*`), `pointer-conversion` and explicit `conversion`. Each property
gives every class one meaning:

| class | signedness | width | enum domain | pointee | nominal | character |
| --- | --- | --- | --- | --- | --- | --- |
| copy, binding | equal | equal | equal | equal | equal | equal |
| sign-change | barrier | equal | equal | - | - | review |
| alias-change | equal | equal | equal | equal | producer | equal |
| widening | review producer | review | review | - | barrier | review |
| narrowing | barrier | review | review | - | barrier | review |
| domain-change | review producer | review | enum<->int equal, else producer | barrier | barrier | review |
| erasure | - | - | - | producer | - | - |
| conversion | barrier unless widening/domain change | barrier | barrier | barrier | barrier | barrier |

`equal` joins one component. `barrier` makes the endpoints independent storage.
`review` blocks a change at either endpoint and `review producer` blocks changing
the producer whose representation the conversion consumes (sign- versus
zero-extension). `producer` is directional: an erased consumer may change only
when its typed producer already has the recovered value, and a typed producer
never seeds the erased consumer. `FooId -> int` or `T* -> void*` therefore needs
evidence on the consumer itself. The collector no longer emits width-change
escapes; an explicit-conversion escape at the same site as its modeled edge is
handled by that edge. One questionable transfer now blocks only the property that
it actually affects instead of merging and blocking a whole component.

## Aggregate storage escapes

Passing a record (as a receiver, pointer or reference) to a direct callee is an
escape of its fields only when that callee's body was not collected anywhere in
the corpus; a collected body records its own field uses. A virtual call is
contained only when every override in its collected override closure has a body
(pure declarations dispatch elsewhere). Template instantiation bodies are not
traversed, so calls into them remain escapes.

## Source-boundary completeness

The campaign writes the configured unit census as `expected-units.txt` before
collection. `LK` records linkage and DLL import/export for functions and globals.
The solver itself establishes the source half of a boundary: every configured
unit was collected, a function or parameter has a collected body, and no DLL
linkage, address-taking, virtual/function-pointer use, reference binding or
aggregate/indirect storage argument exposes the entity outside the corpus. Every
changed width boundary requires this machine-checked completeness in addition to
its reviewed claim, and any change to a DLL-visible declaration is blocked
because it changes the decorated ABI.

## Run a report

Inside the compiler image, select `wiz8-scalar-facts` (enabled in the normal
lint profile) and supply these optional environment variables. The separate
`wiz8-bool-like-byte` client supplies byte-domain observations and predicate-name metadata:

- `WIZ8_SCALAR_FACTS_DIR`: retain facts for the supplied TUs. Start with an empty
  directory for a new source revision/compilation; reuse only within that run.
- `WIZ8_SCALAR_REPORT`: write the integer report without changing lint's bool
  diagnostics or exit status.
- `WIZ8_SCALAR_EVIDENCE`: independently reviewed evidence JSON to seed recovery.

Without evidence, the report inventories declarations, flows and blockers, with
unknown recovery properties. Reuse saved facts without running Clang again:

```sh
clang-tidy --wiz8-scalar-report /out/scalar-facts \
  --evidence /out/reviewed-scalar-evidence.json \
  --output /out/scalar-report.json
```

## Simplify established boolean expressions

`uv run wiz8 analyze scalar-facts --patch --boolean-expressions` also emits
reviewable comparison edits for fields, globals, locals, parameters and indexed
storage whose canonical AST type is already `bool`, plus direct calls without
arguments whose canonical return type is `bool`. It replaces direct comparisons
with zero/one or false/true by the expression or its negation. It also replaces numeric
zero/one literals at established bool arguments, assignments, initializers and
returns using the existing producer-location facts. Operand facts select the
exact declaration, so a same-named byte or integer stays unchanged. This mode does
not infer historical types or turn raw byte flags into bool.

All collected source hashes, including owner headers, must still match. Arithmetic,
macros and unfamiliar expression spelling are left unchanged. The patch report
lists expression edits separately from declaration changes; source is never edited
by the command.

## Propagate accepted source enums

For source consistency after recovering an enum owner, use the same collector and
patch writer with an explicit opt-in:

```sh
uv run wiz8 analyze scalar-facts --patch \
  --propagate-enum W8Condition --propagate-enum W8NpcDialogueLayout
```

The separate `enum_propagation` report starts from current typed declarations.
These are accepted source owners, not independent historical evidence; they do
not seed `integer_components` or relax evidence validation. Only builtin locals
with complete, same-width, same-signedness enum copy producers or explicit
casts into accepted enum owners are patched. Casts preserve the input storage
boundary: a packed byte remains a byte, while its named enum result can propagate.
Missing or ambiguous cast destinations remain blocked.
Comparisons, indexing and switches can consume the identity. Numeric writes,
arithmetic, unknown producers, unseeded cycles, conflicting enums, escapes,
width-sensitive consumers and incomplete collection block automatic changes.
Globals, signatures, fields and overloaded consumers are reported for manual review, preserving packed storage
and independently established ABI boundaries. Dependent editable locals form one
patch group, using the existing source-hash, redeclaration and shared-type-atom
checks. The command writes a patch for review and never applies it.

Saved facts can be replayed with the repository's current Python solver, without
recollecting unchanged C++:

```sh
python3 docker/msvc600/clang-tidy-plugin/clang-tidy-wrapper.py \
  --wiz8-scalar-report build/clang/scalar-campaigns/run-EXAMPLE/facts \
  --propagate-enum W8Condition --output build/enum-propagation.json \
  --repository . --patch build/enum-propagation.patch
```

The report is bounded by the supplied TUs, including bodies and call sites. It
is not a whole-binary reachability claim. No finding automatically edits source or
fails lint. A separate 32-bit predicate inventory requires ABI and independent
symbol/signature evidence before any bool conversion.

## Independent evidence

The input schema is `wiz8.scalar-evidence-v1`, with a `claims` array. Each claim
binds an observed value to a canonical declaration key from the fact report.
Bindings and completeness are reviewed inputs, not conclusions from projected
Ghidra/PDB type names. The collector does not disassemble retail or automatically
match instruction operands to source declarations. Reuse the existing Ghidra
inspection/receiver attribution for those bindings; do not infer them from a
coincident displacement or an AL-only instruction alone.

For example, a reviewed unsigned duration comparison can seed this property:

```json
{
  "schema": "wiz8.scalar-evidence-v1",
  "claims": [{
    "key": "<canonical duration declaration key>",
    "property": "signedness",
    "value": "unsigned",
    "basis": {
      "kind": "retail",
      "reference": "Wiz8.exe <SHA-256>:0x0052e855..0x0052e85c",
      "reason": "receiver-attributed duration compare against 120 uses JNC",
      "mnemonic": "jnc",
      "value_width": 32,
      "operand_width": 32
    },
    "covered_uses": [{
      "file": "src/wiz8/local_code/QuoteManager.cpp",
      "line": 1907,
      "operation": "<"
    }]
  }]
}
```

Use the actual key, source line and operation from the report. A covered use
means the reviewer traced that operation to the independent evidence. Coverage
is property-specific; signedness evidence does not approve a width change.

Signedness accepts signed/unsigned branches, division, extensions, and shifts
whose high bits are relevant. Promoted-operand width mismatches are rejected.
Extensions must identify the narrow input with `operand_role: "input"`;
an extension's output does not establish its declaration's signedness. A
storage-width mismatch also requires a separately reviewed width claim.
Other accepted origins are released external APIs, decorated exports and
original-source oracles. Source projection, recomp PDB types and heuristic
inference cannot seed this solver.

Width claims use values 8, 16 or 32. Retail evidence identifies `value_width`;
function returns additionally require `complete_return_boundary: true` after
reviewing producer definitions and all caller consumption. Fields require
`complete_storage_accesses: true` after reviewing receiver identity, widths,
offsets, stride and layout. Neither narrow constants nor a partial EAX write
establish these contracts. These flags now assert only the retail side of the
boundary; the solver verifies source-side completeness itself.
Parameter width changes require `complete_argument_boundary: true` for the
argument producers and callee consumption, including stack-slot promotions.
Every changed field, parameter or return in a propagated width chain needs its
own reviewed width claim; completeness at the seed does not cover other nodes.

Known-enum claims use `property: "domain"`, `value: "enum:W8Foo"` and independent
external API, decorated symbol or source-oracle provenance. The enum must already
exist among the declarations. Retail numeric operations cannot seed enum identity.

A property propagates through a complete copy component only with one consistent
seed and no unreviewed operation, escape, missing body/ABI contract or declaration
conflict. Reports retain every blocker and evidence reference. Unknown or
conflicting width does not silently resolve signedness, or vice versa. Even a
candidate is a recovery queue item, not permission to rewrite int/long spelling,
invent an enum, add const, or change a known external signature.

## Fact records

Each compiler process appends `facts-<pid>.tsv` in the run directory. Fields
escape backslashes, tabs and line breaks; the reader restores their exact text. `D` records
include identity, source location, bool presentation/candidate flags, current
width, signedness, domain and spelling. `F` records directed source-to-target
transfers and their role; `K` constants; `U` operations; `A` generic escapes;
`H` body coverage; `G` observed expression domains. `W/X/E/S` remain bool-client
annotations, plus `P` pending record escapes and `B` callee bodies, read through
the shared aggregator. Pending bool escapes resolve after all TUs are read. Unknown/malformed records fail
closed; duplicate declarations merge presentation flags and retain conflicting
type metadata as a blocker.


## Generate a recovery patch

Accepted whole-component proposals can now produce source changes for review:

```sh
clang-tidy --wiz8-scalar-report /out/scalar-facts \
  --evidence /out/reviewed-scalar-evidence.json \
  --output /out/scalar-report.json --repository /repo \
  --patch /out/recovery.patch
```

The patch writer handles existing enum domains, builtin signedness at an unchanged width, known integer
typedef atoms, concrete `void` pointees, character element types and literal
array extents. `--padding` additionally deletes layout-preserving explicit padding
members (whole lines, including a trailing comment). It preserves int/long spelling and
qualifiers, updates every observed redeclaration, rejects a component if any
changed declaration lacks a safe span, and rejects atoms shared with unchanged
declarations. SHA-256 and original token bytes must match current files before
any patch is written. It never mutates source; review full project/API coverage,
apply the patch, then recollect and compile affected consumers. Width changes,
aliases lacking independent owners and redundant
cast removal require further recovery.

`pointee` evidence uses the exact canonical pointee identity from `T` metadata
(e.g. `struct W8Foo`), with external-api/source-oracle/decorated-export provenance;
same-pointer ABI is insufficient. `nominal` evidence uses an existing typedef
identity and also supplies a reviewed `role`. All operation coverage remains
bound to the exact declaration, source file, line and operation.

Additional records share the same fact stream: `T` type identity, `O` consumed
constant/operation, `V` explicit conversion endpoints/types, `J` callback slot,
`C` implementation binding, `L` source type atom/snapshot, `FR` field reference,
`CH` storage character/byte-count observation and `LK` linkage. `HB` records every
collected function body (including void/record returns), `OV` a virtual override
edge and `PV` a pure virtual. Legacy bool
records retain their meanings. Report schema `wiz8.scalar-report-v2` keeps one
component list per property under `integer_components`, plus `pointer_components`,
`nominal_components`, `array_components`, `domain_inventory` and `callbacks`;
there is no parallel semantic-debt database.


## Source corrections in this pass

The ten main-game and two combat clock members now retain the original SGP
`TIMER` owner through their complete `GetClock`/`SetCountdownClock` and
`ClockIsTicking` consumer families. This uses an existing independently released
API typedef, preserving width and signedness rather than choosing a different
int/long spelling.

The live message countdown and its TEXT disk-record counterpart use `TIMER`.
Their saved duration fields are now unsigned `UINT32 saved_remaining_ms`, not
signed fields named as though `ClockIsTicking` returns a predicate. The original
SGP implementation returns the milliseconds remaining, and LoadSaveGame passes
that value directly to `SetCountdownClock` after loading. The producer, live
record, serialized record and reload consumer are corrected together. The
0x24-byte disk stride and +0x08/+0x0c layout assertions are retained; serialized
32-bit representations remain unchanged. No SGP source was modified.

Run the configured corpus through the same collector and solvers with:

```sh
uv run wiz8 analyze scalar-facts --evidence config/type-recovery/sgp-clocks.json --patch
```

This includes recovered targets, runtime consumers and retained SGP bodies. Each
invocation retains a fresh fact directory, the exact TU lists, compiler-image and
compile-database identities, reviewed-evidence digest, compiler logs, report and
bounded summary under `build/clang/scalar-campaigns/`. A compiler or solver failure
leaves a failed manifest; it does not publish partial facts as a completed run.
`--patch` writes an artifact for review and never edits source.

The clock evidence also seeds the independently signed samples in released SGP
mouse handling. This deliberately exposes a conflicting whole-program component:
unsigned clock producers do not license rewriting signed source-oracle consumers.
The game countdown owners can be recovered by their complete producer/consumer
census while that larger automatic proposal remains blocked.

The reader streams and deduplicates identical records across TUs, preserving
conflicting observations. Unknown properties do not repeat every escape in every
solver's report. The 32-bit 0/1 inventory works with the generic collector alone,
requires independent ABI/source evidence before edits, and does not classify every
such function as an authored bool. Callback topology is blocked when a parameter
has no scalar node; implicit method receivers and their base fields participate in
the aggregate escape census.


## Array and record clients

`ARR` records a fixed array extent, element storage width and a synthetic element
node. Indexed scalar loads/stores participate in the same flow graph as fields,
parameters and returns. `AU` records indexed, initializer, call and escape uses.
Function-pointer arrays and aggregate callback-table initializers bind their
implementations to shared callback argument/return slots; every table entry must
agree before signature recovery is possible. Unsupported aggregate bases are
excluded rather than misidentified as fields.

`REC` and `RF` record compiled source size/alignment, `#pragma pack` and field
key/offset/size/alignment/type. The structural client inventories arrays, string
initializers, duplicate layouts, packing that changes source size and packing
that changes nothing at all (`redundant_packing`, report only). These
observations do not establish retail packing, authored extents or shared
original record identity. Inheritance, polymorphism, unions, bitfields and
attributed fields exclude natural layout replay.

### Character storage

`CH` records what each consumer does with array storage: a system-header API
parameter's pointee (`char`, `wchar_t` including the VC6 `WCHAR`/`wchar_t`
typedefs, or `raw-byte` for `void`/byte pointers), a project callee's pointee, a
string literal's kind and length (`units:N`) and constant byte counts passed to
size parameters (`bytes:N`). These observations do not establish the historical
type. `character` evidence (`char`, `wchar_t` or `raw-byte`, retail code-unit
width or independent declarations) seeds the element node. Every API/literal
consumer must agree (`raw-byte` agrees with either), every project consumer must
already take the recovered pointee, and element value flows obey the character
column above. An element width change also needs `extent` evidence. `raw-byte`
is report-only: its historical spelling needs nominal recovery.

### Extents

`extent` evidence names its `extent_kind`: retail `indexing-range`,
`serialized-extent`, `allocation-stride` or `field-boundary` (each with
`complete_storage_accesses: true`), or an independent `declaration-contract`.
`sizeof(array)` must be covered, constant indices and literal lengths must fit,
consumer byte counts must fit, and shrinking needs every dynamic index covered.
Only a literal extent is rewritten; a symbolic extent belongs to its constant's
owner. After character/extent proposals are accepted, every affected record is
replayed: all other member offsets, the record size and alignment must remain
unchanged or every proposal touching that record is blocked.

### Padding

A member named like `pad`, `padding_N` or `align_N` with byte/short storage is
removable only when it has no reference (`FR`, array use, flow, operation or
escape), its record has no positional aggregate initializer, it owns a sole
declaration, and replaying the record without it reproduces every surviving
offset, the size and the alignment. Candidates are evaluated greedily per record.

Evidence may select an exact `(file, kind, name)` owner instead of a line-based
key. Parameter selectors use qualified function name and position (`::#0`),
independent of parameter spelling. Zero or multiple matches fail closed. The
clock evidence now cites the immutable ancestor at `72697ddaac1c`, not editable
`src/sgp`; retained Wizardry SGP bodies are reported as SGP translation units.

The full-corpus array client exposed `SaveMonsterRecord::script_name` as a
16-bit-element array used by ANSI `strcpy` and a 64-byte disk write. Retail
`0x005147a0` and the loader's 64-byte ANSI string consumer establish `char[64]`.
Its two-byte initial seed remains a raw copy, preserving the retail global read;
the string operation no longer needs a reinterpret cast.

The enum flow inventory exposed `srGERD::Device::back_buffer_type_34c_` between
back-buffer producers and the exported `e_backBuffer` getter (`0x1001cd10`).
Recover the existing enum field and constants, retaining zero as the closed-window
sentinel. The SDK module-handle flow at `srTimer +0x848` is now `HMODULE`; retail
`0x100610f0`, `0x10060b90`, `0x10062480` and `0x10063600` establish the Win32
producer/consumer family. Five pointer-erasure casts are removed. Layouts and
serialization widths are preserved.

Protected `L` records identify a node and declarator component before the file, byte
offset/length, source tokens and SHA256. Components include return/parameter types,
pointees, array elements/extents, and callback returns/parameters. Callback typedef
uses share the typedef's source atoms: every collected owner must agree before an
edit is emitted. Arity, calling convention, variadics, unresolved producers and ABI
escapes remain blockers; bound implementations must have collected bodies. Array
extent spans change only through the extent client; `field-declaration` spans
cover a complete sole member declaration for padding removal.

## Import independent declaration contracts

After collecting a complete campaign, generate ordinary scalar evidence from
explicit retained-source correspondences and already-paired retail exports:

```sh
uv run wiz8 analyze scalar-evidence \
  --campaign build/clang/scalar-campaigns/run-... \
  --oracle config/type-recovery/sgp-oracle.json --exports
```

The source importer extracts byte-preserved headers from a full immutable Git
revision and records each original path, blob and SHA256. An accepted source
identity is insufficient by itself: each correspondence explicitly reviews that
its declaration contract was retained. Clang parses the released declarations;
the configuration supplies correspondence and reviewed typedef roles, not hand
copied width/signedness/type facts. A paired fixed array also contributes its
`declaration-contract` extent and, for `char`/`wchar_t` elements, its character kind.

The export importer reads the canonical retail `sr.dll`, verifies its inventory
SHA256, and uses existing reccmp source-index address pairings. LLVM decodes the
export signatures; Clang collects their scalar properties using each paired
source TU's configured include environment. Constructors, operators, thunks,
ambiguous folded addresses and unmatched names are skipped. Function arity,
calling convention and variadic shape must agree before a contract seeds the
current graph. This does not infer erased typedef aliases, record layouts or
historical plain-char signedness from exports.

Artifacts under the campaign parent include original facts, extraction manifest,
exact mappings, provenance, collection logs and `evidence.json`. Replay the normal
`--wiz8-scalar-report` / patch writer with that evidence; importing a declaration
contract does not remove operation, escape or component-consistency blockers.
