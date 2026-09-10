# Choose a comparison tool

| Question | Existing primitive |
| --- | --- |
| Does the actual recomp-linked function match? | `just compare ADDRESS...` |
| Is a COFF code/data contribution identical independently of final linkage? | `uv run reccmp-reccmp --object ...` below |
| Which original absolute targets correspond to COFF relocations in an exact pair? | `original_absolute_relocation_targets` below |
| Do class vtable slots and targets agree? | `just wiz8 vtable CLASS` compares matching class names and refuses zero-entity success. |
| Does reviewed global data agree? | `just wiz8 datacmp` compares reviewed globals through reccmp. |
| What is the paired original/recompiled address? | `just wiz8 addr ADDRESS...` translates either side in one process. |

Use the project command where one exists; use reccmp directly for capabilities the project does not
wrap. Do not build a generic comparison framework or wrap all of reccmp.

## Linked-image selected function

```sh
just compare 0x0044e010
```

The normal recovered-Wizardry path builds current inputs and returns structured selected results.
Mismatch details include `difference`, `reason`/`location` where available, and a bounded
`instruction_window`; use the first meaningful divergence already reported. Whole-image comparison
is diagnostic, not a substitute for selected-function evidence.

Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes. The comparison link uses `/OPT:NOICF`
and `/FIXED:NO` (base relocations retained); retail folding can therefore produce a `call_target`
mismatch even for the type-correct source callee. Do not change these modes to hide a difference.

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
Keep the callee appropriate to the canonical type; do not cast `W8PList*` to `W8IList*` just to force
the retained address.

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
