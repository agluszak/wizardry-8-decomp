# Compiler emission metadata

Authored source carries `FUNCTION`, `GLOBAL` and `VTABLE` annotations. Compiler
special members, deleting destructors, initializers, thunks and template
instantiations are binary entities identified outside the source tree. MSVC
emits them from ordinary C++ declarations, template bodies and use sites.
Never add a declaration or handwritten implementation solely to expose an
emission to reccmp.

`tools/wiz8decomp/emissions.py` generates these disposable data sources:

- `build/generated/reccmp/wiz8-emissions.csv`
- `build/generated/reccmp/surrender-emissions.csv`
- `build/generated/reccmp/srext-jpegimporter-emissions.csv`
- `build/generated/reccmp/srext-unzip-emissions.csv`

Each target lists its CSV in `reccmp-project.yml`. `wiz8 build` and `wiz8 check`
bootstrap the CSVs before reccmp reads them. Generate just the metadata without
compiling with:

```sh
uv run wiz8 analyze emissions
```

The original-address seed is `evidence/observations/compiler-emissions.csv`.
It migrated the former source annotations without changing their identities.
`legacy-marker` provenance explicitly means these labels are inherited recovery
claims, not independently recovered original symbols. `source_files` preserves
historical ownership for changed-file/template selection; it does not attach an
emission to a C++ declaration or prove its original translation unit.

With rebuilt images, PDBs and the compiler-backed source index available, enrich
the metadata through reccmp's existing catalog:

```sh
uv run wiz8 analyze emissions --derive
```

A product build also enriches the targets whose images/PDBs are available when
the source index exists. Generation uses exact linker symbols from existing
catalog pairs. For unpaired synthetic identities, it can associate deleting
destructors through paired vtable slots. It uses raw slot symbols so that an
adjustor/thunk identity is not replaced by its resolved destination. Conflicting
slots and symbols serving multiple original addresses remain unresolved.
Explicit inventory symbols cannot be silently replaced by a contrary PDB pair.

The output format is
`address|symbol|name|type|recomp_selector|selector_is_symbol`. `synthetic` and `template` both
map to reccmp function entities; the distinction remains useful for diagnostics
and template selection. Independent original symbols stay in `symbol`. Reconstruction hypotheses and
PDB enrichment go in `recomp_selector`; `selector_is_symbol` distinguishes exact
linker symbols from exact names. The display name retains unresolved original
template spelling instead of presenting a reconstruction label as retail evidence. Generation never guesses from address order, nearby functions,
normalized demangled spellings or uncorroborated body resemblance.

Ambiguous associations belong in `config/reccmp/emission_overrides.csv`, using the inventory columns, including optional
`recomp_selector` and `selector_is_symbol`. This file is initially empty.
Keep the original inventory as evidence; overrides record reviewed corrections
or additional identities. Source-model lint rejects reintroduced emission
markers, and merge preservation recognizes source functions reclassified into
binary metadata while continuing to protect authored identities.

For remaining unpaired inventory functions, explicitly run the expensive analysis:

```sh
uv run wiz8 analyze emissions --correlate --target WIZ8
```

reccmp prepares private Ghidra programs and composes the existing Ghidriff/Ghidra
exact-byte, normalized-instruction and structural correlators. Exact bytes are
verified; instruction/structural candidates additionally require equal complete,
warning-free normalized decompilation. Both sides must remain globally one-to-one
across the full unpaired populations and the PDB symbol must be unique. Conflicts,
duplicate template bodies and folded identities stay unresolved for review. The
correlation JSON beside the CSV records binary hashes, evidence and unresolved
addresses. This does not modify the live reviewed retail ProgramDB or source.

Generation stages baseline catalogs separately from the previous generated output,
so a prior hypothesis cannot confirm itself. Failed derivation leaves the previous
CSV intact. A scoped generation updates only the selected targets. The legacy
inventory remains original-address evidence; a recomp PDB cannot independently
recover every original address. Live pairing/report parity and correlation still
require validation with compiled images and a compiler-backed source index.
