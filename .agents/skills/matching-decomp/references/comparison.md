# Choose a comparison tool

| Question | Existing primitive |
| --- | --- |
| Does the actual recomp-linked function match? | `uv run wiz8 compare ADDRESS...` |
| Does an almost-matching function differ mainly in stack-slot layout? | `uv run reccmp-stackcmp --target TARGET ADDRESS` diagnoses stack offsets; see below. |
| Is a COFF code/data contribution identical independently of final linkage? | `uv run reccmp-reccmp --object ...` below |
| Which original absolute targets correspond to COFF relocations in an exact pair? | `original_absolute_relocation_targets` below |
| Do class vtable slots and targets agree? | `uv run wiz8 vtable CLASS` compares matching class names and refuses zero-entity success. |
| Does reviewed global data agree? | `uv run wiz8 datacmp` compares reviewed globals through reccmp. |
| Are matching annotations structurally valid? | `uv run wiz8 check` runs reccmp `decomplint` over every configured source target. |
| What is the paired original/recompiled address? | `uv run wiz8 addr ADDRESS...` translates either side in one process. |

Use the project command where one exists; use reccmp directly for capabilities the project does not
wrap. Do not build a generic comparison framework or wrap all of reccmp.

## Linked-image selected function

```sh
uv run wiz8 compare 0x0044e010
uv run wiz8 compare 0x1004a5a0 --program sr.dll
```

The selected Ghidra program determines the reccmp target: the default `wiz8` program selects
`WIZ8`, while `--program sr.dll` selects `SURRENDER`. Pass `--build` after source edits to build
current inputs; without it the command reads the existing
comparison product and source index. Both paths return structured selected results.
Mismatch details include `first_difference` (named original/recompiled entities and instruction
indexes), `difference`, `reason`/`location` where available, a bounded `instruction_window`, and an
`artifacts.diff` path under `build/reports/compare/`. Use that first meaningful divergence; do not
scrape JSON for a second renderer. Whole-image comparison is diagnostic, not a substitute for
selected-function evidence.

Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes. The comparison link uses `/OPT:NOICF`
and `/FIXED:NO` (base relocations retained); retail folding can therefore produce a `call_target`
mismatch even for the type-correct source callee. Do not change these modes to hide a difference, and
do not add a `FOLDED` source marker to make the mismatch score exact. Retail ICF belongs to comparison
evidence, not source identity.

## Stack layout diagnosis

Use `stackcmp` only after a focused linked comparison shows an almost-matching function whose remaining
differences repeatedly involve `ebp`/`esp` stack operands, local ordering, or apparently shifted local
slots while the surrounding instruction structure still lines up. It is a diagnostic for forming a
source hypothesis, not a matching target in its own right.

```sh
uv run reccmp-stackcmp --target WIZ8 0x0044e010
uv run reccmp-stackcmp --target SURRENDER 0x1004a5a0
```

The address is always the original address. `stackcmp` reads the existing recompiled image and PDB and
does not build them; build the owning target first when source edits made those products stale. It maps
original stack operands to recomp stack operands and, where CodeView data permits, names recomp locals.
A one-to-one mapping at different offsets can support a hypothesis about local declaration order or
lifetime. A non-bijective mapping is evidence that the remaining difference may be structural rather
than mere slot order. Structural mismatch warnings mean the stack map is incomplete and the ordinary
instruction diff must be inspected first.

Do **not** treat stack positions as authored-source evidence. VC6 is free to reuse parameter/local/spill
temporary storage. Never alias variables, overwrite parameters early, add overlapping storage, or
change semantics merely to make `stackcmp` prettier. Reconcile any stack-layout hypothesis with types,
callers, lifetimes, source oracles, and the actual instruction behavior.

`reccmp-stackcmp` is deliberately not a repository/CI gate: it is per-function and its current CLI can
print non-bijective/structural warnings while still exiting successfully. Agents should invoke it when
the focused mismatch calls for it and interpret the output, not run it mechanically over every body.

## COFF contribution versus original

Use object mode when dead stripping or archive extraction prevents a contribution from surviving
into the recomp PE, folding makes linked targets misleading, or an original-source TU/contribution
needs validation independently of final linkage. It also supports data and static symbols.

```sh
uv run reccmp-reccmp \
  --target WIZ8 \
  --object PATH_TO_OBJ \
  --symbol 'EXACT_COFF_LINKER_SYMBOL' \
  --orig-address 0xXXXXXXXX \
  --size SIZE
```

The console entry point is `reccmp-reccmp`, even though the implementation module/tool is historically
called asmcmp. Do not search for `reccmp-asmcmp`.

Supply the exact current object path, exact decorated COFF linker symbol, one independently known
original address, and independently known original extent in bytes. Establish the extent from retail
function/data boundaries or accepted source evidence, never alignment padding or the candidate's size.
Object mode does not build: compile changed inputs through the existing owning build target when
needed; reuse an already-current object. No recompiled PE/PDB or surviving linked symbol is required.

An `exact` / `relocation-masked-object` result means size and non-relocation bytes agree after
supported COFF relocation operands/original base relocations are masked. It does **not** establish
relocation-target identity, source-level syntax, or semantic equivalence by itself. Unsupported or
unpaired evidence stays inconclusive; do not infer success from a percentage.

For example, `PLLength` and `ILLength` can share an ICF-folded retail body. A caller's exact masked
contribution establishes equality outside relocations; it does not alone prove that a differing call
target is merely that fold. Reconcile the linked diff with relocation/reference/type evidence.
Keep the callee appropriate to the canonical type; do not cast `W8PList*` to `W8IList*` or attach
the retained address to `PLLength` just to force an exact linked comparison. If comparison-side
equivalence metadata ever becomes necessary, keep it outside recovered C++ ownership.

## Specialized repository gates

`uv run wiz8 check` invokes reccmp's `decomplint` engine for every target in `reccmp-project.yml` that
has a source root, including non-WIZ8 targets. Project-specific waivers for source/link order and marker
style are applied centrally; syntax, duplicate identities, stray markers, and other non-waived alerts
remain fatal. Do not run a second hand-written marker parser as a substitute.

`uv run wiz8 vtable` and `uv run wiz8 datacmp` are licensed-input comparisons over existing products.
They emit structured JSON and exit non-zero when `ok` is false, so callers (including CI) can invoke
them directly without wrapping JSON schema knowledge. Use `--program` for a non-WIZ8 product when that
product is current. `datacmp` is for reviewed `GLOBAL` data; it is not a raw whole-section equality
test.

Not every upstream reccmp console entry point belongs in routine verification. `roadmap` is a placement
report, `aggregate` combines saved reports, `cvdump` is a lower-level CodeView inspection tool, and
`project`/`ghidra-import` overlap project-owned setup/import workflows. Use them directly for a concrete
diagnostic need rather than adding unconditional CI passes. `verexp` can diagnose a DLL export-table
question, but partial DLL reconstruction makes whole-export equality unsuitable as a blanket gate until
the relevant product's export surface is intended to be complete.

## Original absolute COFF relocation targets

The pinned public API is:

```python
from pathlib import Path
from reccmp.compare.exact import original_absolute_relocation_targets
from reccmp.formats import detect_image
from reccmp.formats.coff import parse_coff_object
from reccmp.formats.pe import PEImage

# Use the original PE from reccmp-user.yml's WIZ8 target and the paired object.
original = detect_image(Path("PATH_TO_ORIGINAL_PE"))
assert isinstance(original, PEImage)
obj = parse_coff_object(Path("PATH_TO_OBJ"))
# symbol (str), address (int), size (int): reuse the already exact pair's inputs.
targets = original_absolute_relocation_targets(original, obj, symbol, address, size)
for target in targets:
    print(target.symbol.name, hex(target.offset), hex(target.address))
```

Pass the same exact COFF symbol, original address, and independent extent used for the exact pair.
The returned records hold a COFF `symbol`, contribution-relative operand `offset`, and original target
`address`. For absolute i386 DIR32 operands, the API subtracts the COFF operand's addend from its
paired original operand modulo 2**32. This is not a relative-call relocation resolver.

These are diagnostic observations, not independent identity proof. Reconcile them with call sites,
references, source ownership, types, or other evidence before assigning original target identities.
Use this API instead of searching reccmp implementation for a relocation mapper.
