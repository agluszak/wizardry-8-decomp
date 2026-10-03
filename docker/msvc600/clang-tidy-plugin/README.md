# Whole-program scalar facts

`ScalarFacts.h` owns canonical declaration/parameter identity and the fact writer.
`ScalarFacts.cpp` collects source declarations, transfers, constants, operations,
body coverage, and escapes across the supplied translation units. Parameters use
canonical function identity plus parameter position, including unnamed header
parameters and differently named definitions. Function-return nodes, fields and
locals participate in the same graph. Dependent/incomplete template types are
excluded until a concrete layout exists; member pointers retain pointer domain.

`BoolDomainVisitor` adds the existing byte-domain annotations. The bool solver
reports byte-domain observations with the same naming, suppression, escape and
missing-body rules. Generic escapes and source type metadata do not establish
historical types. These observations remain review guidance; actual compiler
errors fail the wrapper before potentially incomplete facts are read.

`scalar_facts.py` aggregates the shared facts and implements the report-only
integer solver. Width, signedness and semantic domain are separate properties;
the current C++ spelling is retained as observation metadata. Current AST types
and arithmetic are not independent retail evidence. Ordinary source copies,
returns and call arguments connect the graph; arithmetic is an operation to
review, and explicit/width-changing conversions remain barriers. The collector
also records pointer, enum, character, bool and floating domains, but the integer
solver preserves those domains. A separate known-enum domain solver accepts
independent header/symbol/source evidence for an enum already present in the
facts. It propagates that identity through pure copies, blocking bare numeric
producers, different enums, operations and escapes. It cannot invent an enum
from a numeric range. Pointer and floating recovery solvers are not implemented.

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
is not a whole-binary reachability claim. No integer finding edits source or
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
