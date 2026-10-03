# Whole-program scalar facts

`ScalarFacts.h` owns canonical declaration/parameter identity and the fact writer.
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
returns and call arguments connect the graph; arithmetic is an operation to
review, and explicit/width-changing conversions remain barriers. The collector
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
rules. Array/table elements, unresolved callback producers, external ABI slots,
complex declarators and width changes remain conservative follow-up work.
Floating, array/record, packing and class-ownership clients remain future work.

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
establish these contracts. These flags record reviewed completeness assertions;
the solver does not independently prove them.
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

The patch writer handles builtin signedness at an unchanged width, known integer
typedef atoms, and concrete `void` pointees. It preserves int/long spelling and
qualifiers, updates every observed redeclaration, rejects a component if any
changed declaration lacks a safe span, and rejects atoms shared with unchanged
declarations. SHA-256 and original token bytes must match current files before
any patch is written. It never mutates source; review full project/API coverage,
apply the patch, then recollect and compile affected consumers. Width changes,
callback declarator surgery, aliases lacking independent owners and redundant
cast removal require further recovery.

`pointee` evidence uses the exact canonical pointee identity from `T` metadata
(e.g. `struct W8Foo`), with external-api/source-oracle/decorated-export provenance;
same-pointer ABI is insufficient. `nominal` evidence uses an existing typedef
identity and also supplies a reviewed `role`. All operation coverage remains
bound to the exact declaration, source file, line and operation.

Additional records share the same fact stream: `T` type identity, `O` consumed
constant/operation, `V` explicit conversion endpoints/types, `J` callback slot,
`C` implementation binding, and `L` source type atom/snapshot. Legacy bool records
retain their meanings. The report adds `domain_inventory`, `pointer_components`,
`nominal_components` and `callbacks`; there is no parallel semantic-debt database.


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

`REC` and `RF` record compiled source size/alignment and field offset/type/size.
The structural client inventories arrays, string initializers, duplicate layouts
and packing that changes source size. These observations do not establish retail
packing, authored extents or shared original record identity. Inheritance,
polymorphism, unions, bitfields and attributed fields exclude natural-size replay.

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
