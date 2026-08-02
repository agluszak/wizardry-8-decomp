# Binary evidence authority

Wizardry 8 shipped without first-party debug symbols and without RTTI. Binary
facts therefore come from the reviewed Ghidra project, not from tracked
byte-scanner snapshots.

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

## Focused reports

Use `just context ADDRESS` for the joined function view. It queries the live
reviewed program for the function, calls, data references, exception metadata,
vtable references, decompilation, and optional P-code detail. It combines those
facts with source markers and reviewed assertion observations, then writes a
transient packet under `build/reports/recovery-context/`.

`uv run wiz8 report translation-units` uses one `TranslationUnitResolver`.
Direct ownership comes only from reviewed assertion call sites whose containing
function Ghidra resolved. Bounded interval inference is explicitly labelled and
never incorporates guessed function starts from byte padding.

Whole-program needs use narrow Java audits against live Ghidra objects. The
current audit bundle supplies function inventory, function call/data/vtable/EH
facts, class fields and vtable references, source-layout checks, lifecycle
symbols, and claimed-function existence. Audit JSON is written under `build/`
and discarded by callers after use.

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

When a mechanically reproducible binary observation is needed repeatedly, add
the smallest focused Ghidra query that answers it and emit only the report
result under `build/`. Promote an interpretation into Ghidra when it is reviewed.
Do not add a tracked CSV snapshot, generic upsert path, or another Python x86
decoder.
