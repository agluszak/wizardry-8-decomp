# Choose a comparison tool

| Question | Existing primitive |
| --- | --- |
| How does the recovered function differ from retail? | `uv run wiz8 compare ADDRESS...` compares the existing products with Ghidra/Ghidriff, reusing identical completed results. |
| What does an existing comparison say? | `uv run wiz8 report compare` reads saved results only; filter by address, --file or --outcome. |
| Do class vtable slots and targets agree? | `uv run wiz8 vtable CLASS` compares matching class names and refuses zero-entity success. |
| Does reviewed global data agree? | `uv run wiz8 datacmp` compares reviewed globals through reccmp. |
| Are matching annotations structurally valid? | `uv run wiz8 check` runs reccmp `decomplint` over every configured source target. |
| What is the paired original/recompiled address, and why are they paired? | `uv run wiz8 addr ADDRESS...` looks either side up in reccmp's catalog. |

Use the project command where one exists; use reccmp directly for capabilities the project does not
wrap. Do not build a generic comparison framework, a second differ, or an equivalence engine.

## Selected functions

```sh
uv run wiz8 compare 0x0044e010
uv run wiz8 compare 0x1004a5a0 --program sr.dll
uv run wiz8 compare --build --changed --sxs
```

The selected Ghidra program determines the reccmp target: the default `wiz8` program selects `WIZ8`,
while `--program sr.dll` selects `SURRENDER`. Pass `--build` after source edits to build current
inputs; without it the command reads the existing comparison product and source index.

`compare` runs `reccmp-reccmp` for the selected original addresses. reccmp gives both programs the
same names for every catalog pair, analyzes both without debug information, decompiles each selected
pair with Ghidra, and diffs the normalized C. It also compares the contents of the data each function
refers to, so two different literals behind equal labels still show up. The first run analyzes both
binaries (a few minutes); the original's analysis is then kept in `build/reccmp-ghidra`, and later
runs only analyze the new recompiled build. Completed results are also reused when the full manifest,
binary contents, reviewed signature facts, analysis configuration and installed reccmp/Ghidriff
implementations agree. Failed analyses are retried. Native reccmp's --no-cache bypasses reuse.

Each selected function gets one outcome:

| Outcome | Meaning |
| --- | --- |
| `differences` | Decompiled code or referenced data differs. Inspect the diff. |
| `no-differences` | Ghidra and Ghidriff show no difference. Useful evidence, not proof. |
| `unpaired` | reccmp has no retail counterpart for the function. |
| `analysis-failed` | The comparison did not complete; `failures` says why (no Ghidra function at the entry, an entry inside another function, a decompiler error). |
| `header-emission` | An inline header body with no paired rebuild emission, including an unpaired retail marker. |
| `internal-non-emission` | A current indexed TU-local definition has no standalone procedure in the complete rebuild PDB catalog. Its body may be inlined or eliminated; retain the gap for review, with no independent body comparison or equivalence claim. |
| `template-non-emission` | A marker-only retail template emission has no paired rebuild emission in a changed-file comparison; retain it in the report without failing the authored-function gate. |
| `missing` | The address is not a function reccmp knows. |

`ok` is false only for `analysis-failed`, `unpaired` and `missing`. Differences are
review material because source edits can change decompiled output. Template non-emissions
remain visible in the report for codegen review without failing the authored-function gate.
Source-based selections also classify internal non-emissions only when the defining file matches
the compiler index digest. An emitted symbol with failed pairing, an external definition, or stale
source remains a failure. Review the containing caller comparisons when a helper is inlined; do not
change compiler flags, linkage or authored source merely to obtain a standalone procedure.

A row carries `data` findings (a paired object whose contents differ, or referenced literals that
differ in contents), `failures`, `basis` (how reccmp paired the function), `source`, and, for a code
difference, `code_diff.artifact`: the unified diff under the returned run directory. The run's
`summary.json` and Ghidriff's report (`<TARGET>.ghidriff.md`, plus `sxs_html/` with `--sxs`) sit in the
same directory. Each invocation keeps a separate build/reports/compare/<target>/run-* directory;
latest points to the last completed run. A focused comparison never deletes the full inventory.
Keep the returned summary path for triage and use it with --report. Read the side-by-side HTML when
changed spans inside lines are useful.

## Inspect saved results

```sh
uv run wiz8 report compare --program sr.dll --outcome differences --limit 20
uv run wiz8 report compare 0x10014110 --program sr.dll --diff-lines 80
uv run wiz8 report compare --program sr.dll --report build/reports/compare/surrender/run-EXAMPLE/summary.json --file src/surrender/scheduler.cpp
uv run wiz8 report compare --program sr.dll --against build/reports/compare/surrender/run-EARLIER/summary.json
```

These reads neither build nor open Ghidra. Output contains matched/omitted counts, bounded function
rows and a total diff-line budget (default zero). --against shows changed results at original-address
identities: resolved, newly-different, changed, failed/unpaired, or only-current/only-previous.
Absent selections are not fixed or removed functions. The native summary retains raw unpaired
emissions; changed-file gate classifications are reported by compare itself.

## Reading a difference

Decide whether each hunk reflects a source-model divergence or decompiler representation:

- a different comparison operator, constant, callee, field offset, string or global is a logical
  difference until evidence shows otherwise;
- renamed temporaries (`iVar3`/`uVar5`), reordered commutative operands, different local names or
  stack-variable names, and `goto`/`break` restructuring of the same branches are usually
  representation;
- a signedness change in a temporary's type (`uint` versus `int`) can matter: check the comparison
  that uses it.

Never distort recovered source to reduce a decompiler diff. When the diff is large and structurally
unstable (big switch dispatchers, heavy x87 code), do not build another fallback engine: inspect the
specific behavioral frontier and its callees, split the investigation at real function boundaries,
use retail Ghidra decompilation/disassembly where needed, and establish behavior with the runtime
differential scenarios.

## Identical-code folding

The comparison link uses `/OPT:NOICF` and `/FIXED:NO` (base relocations retained); retail folded
identical functions. A caller then calls a different recompiled function than retail's retained body
(for example `PLLength` where retail calls the folded `ILLength` body), and a vtable slot can point
at an unpaired recompiled function (`wiz8 vtable` reports it as `unpaired`). Once the fold is
independently established, keep the type-correct source callee. Do not change link modes to hide the
difference, and do not add a `FOLDED` marker: retail ICF is linked-image evidence, not source identity.

Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes.

## Specialized repository gates

`uv run wiz8 check` invokes reccmp's `decomplint` engine for every target in `reccmp-project.yml` that
has a source root, including non-WIZ8 targets. Project-specific waivers for source/link order and marker
style are applied centrally; syntax, duplicate identities, stray markers, and other non-waived alerts
remain fatal. Do not run a second hand-written marker parser as a substitute.

`uv run wiz8 vtable` and `uv run wiz8 datacmp` are licensed-input comparisons over existing products.
They emit structured JSON and exit non-zero when `ok` is false, so callers (including CI) can invoke
them directly without wrapping JSON schema knowledge. `vtable` separates a slot at a different paired
function (`different`) from a slot at an unpaired function (`unpaired`, typically a retail fold). Use
`--program` for a non-WIZ8 product when that product is current. `datacmp` is for reviewed `GLOBAL`
data; it is not a raw whole-section equality test.

`uv run wiz8 report status` reports source coverage, which recovered functions reccmp pairs and on
what basis, and the counts of the last `compare` report when it compared the current build. It never
runs Ghidra.

Not every upstream reccmp console entry point belongs in routine verification. `roadmap` is a placement
report, `cvdump` is a lower-level CodeView inspection tool, and `project`/`ghidra-import` overlap
project-owned setup/import workflows. Use them directly for a concrete diagnostic need rather than
adding unconditional CI passes. `verexp` can diagnose a DLL export-table question, but partial DLL
reconstruction makes whole-export equality unsuitable as a blanket gate until the relevant product's
export surface is intended to be complete.
