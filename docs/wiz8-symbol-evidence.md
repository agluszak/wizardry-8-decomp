# Binary evidence authority

Wizardry 8 shipped without first-party debug symbols and without RTTI. Binary
facts therefore come from the reviewed Ghidra project, not from tracked
byte-scanner snapshots. Stored types and prototypes remain analysis models;
retail instructions and call sites outrank incorrect inferences.

## Authority boundaries

- Ghidra owns functions, instructions, P-code, calls, exception metadata,
  globals, class layouts, vtables, references, names, and reviewed comments.
- C++ source and the compiler-backed source index own declarations, markers,
  translation-unit placement, and source spellings.
- `evidence/reviewed/` records provenance that neither authority can express.
- Focused reports under `build/` are disposable projections. They are never
  editable evidence and are not committed.
- Pinned external source and ABI oracles, including SurRender exports, remain
  tracked where reproducing them requires proprietary inputs.

## Direct inspection and optional reports

Use [direct PyGhidra](../.agents/skills/matching-decomp/references/pyghidra.md)
for functions, instructions/P-code, parameters, references, and data types.
Open the existing project once, inspect native objects, and filter before
printing. Large output belongs in disposable `build/` artifacts. A custom
query dispatcher or report schema is not required for exploratory work.

`just context ADDRESS...` remains an optional joined function/source/provenance
view. `uv run wiz8 report translation-units` uses one `TranslationUnitResolver`.
Direct ownership comes only from reviewed assertion call sites whose containing
function Ghidra resolved. Bounded interval inference is explicitly labelled and
never incorporates guessed function starts from byte padding.

Existing source-layout and lifecycle fixture checks use their separate
disposable projects; ordinary live investigation does not need another project.

## External ABI snapshots

Two snapshot producers remain because their authority is outside the reviewed
game binary:

```sh
uv run wiz8 evidence refresh debug-artifacts
uv run wiz8 evidence refresh surrender-abi
```

They describe external artifacts and SurRender declarations. They do not scan
Wizardry code to create a competing function, call, EH, global, or polymorphism
model.

## Extension rule

Use task-local Python and native Ghidra APIs first. Keep an existing recovery
algorithm when it adds useful analysis, not merely because it wraps an API.
Promote a repeated task into a small library function only when that removes
real duplication; do not grow a command catalogue, query protocol, or generic
editing framework. Apply reviewed interpretations to Ghidra and keep only their
provenance separately. Do not add tracked binary snapshots, signature ledgers,
or another Python x86 decoder.
