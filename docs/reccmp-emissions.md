# Compiler emission metadata

Authored source carries `FUNCTION`, `GLOBAL` and `VTABLE` annotations. Compiler
special members, deleting destructors, initializers, thunks and template
instantiations are binary entities identified outside the source tree. MSVC emits
them from ordinary C++ declarations, template bodies and use sites. Never add a
declaration or handwritten implementation solely to expose an emission to reccmp.

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

The original-address seed is `evidence/observations/compiler-emissions.csv`. It
migrated the former source annotations without changing their identities. `legacy-
marker` provenance explicitly means these labels are inherited recovery claims, not
independently recovered original symbols. `source_files` records canonical template headers and retained historical source
associations for changed-file/template selection. It does not attach an emission
to a C++ declaration or prove its original translation unit.

With rebuilt images, PDBs and the compiler-backed source index available, enrich the
metadata through reccmp's existing catalog:

```sh
uv run wiz8 analyze emissions --derive --target WIZ8
```

Ordinary builds only bootstrap baseline CSVs and never construct a reccmp catalog.
PDB enrichment runs only through the explicit analysis command with a current source
index and the selected target's products. Generation uses exact linker symbols from
existing catalog pairs. For unpaired synthetic identities, it can associate deleting
destructors through paired vtable slots. It uses raw slot symbols so that an
adjustor/thunk identity is not replaced by its resolved destination. Conflicting
slots and symbols serving multiple original addresses remain unresolved. Explicit
inventory symbols cannot be silently replaced by a contrary PDB pair.

The output format is `address|symbol|name|type|recomp_selector|selector_is_symbol`.
`synthetic` and `template` both map to reccmp function entities; the distinction
remains useful for diagnostics and template selection. Independent original symbols
stay in `symbol`. Reconstruction hypotheses and PDB enrichment go in
`recomp_selector`; `selector_is_symbol` distinguishes exact linker symbols from
exact names. The display name retains unresolved original template spelling instead
of presenting a reconstruction label as retail evidence. Generation never guesses
from address order, nearby functions, normalized demangled spellings or
uncorroborated body resemblance.

Ambiguous associations belong in `config/reccmp/emission_overrides.csv`, using the
inventory columns, including optional `recomp_selector` and `selector_is_symbol`.
This file is initially empty. Keep the original inventory as evidence; overrides
record reviewed corrections or additional identities. Source-model lint rejects
reintroduced emission markers, and merge preservation recognizes source functions
reclassified into binary metadata while continuing to protect authored identities.

Build/check and catalog consumers automatically bootstrap the cheap baseline CSVs.
Missing, corrupt or outdated output is regenerated from inventory without compiling,
loading a PDB or collecting a source index. A per-target checksum records the seed
and output so unchanged inventories retain valid explicit PDB enrichment rather than
silently discarding it during comparison reads.

Generation stages derivation baselines separately from previous output so a prior
hypothesis cannot confirm itself. Failed derivation leaves previous CSVs intact.
Scoped generation updates only selected targets. Body/structural correlation is
outside this migration; ambiguous inventory identities remain unresolved until
independent analysis or reviewed overrides establish a match. Live pairing/report
parity requires compiled images and a compiler-backed source index.

Library identities are owned by `config/reccmp/wiz8-msvc-runtime.csv`,
`wiz8-zlib.csv`, and `surrender-libraries.csv`; authored source rejects
`LIBRARY:` just as it rejects `SYNTHETIC:` and `TEMPLATE:`.

Comparisons retain reccmp's native `summary.json` for detailed differencing and
persist the source/PDB-classified result as `classified-summary.json`. CI uses
the classified result on both PR head and merge base to report requested,
analyzed, non-emitted (internal/template/header), unpaired, and failed coverage.
A merge-base procedure that becomes an internal non-emission fails PR validation.
SurRender export validation likewise compares the extra decorated export names
against the merge-base build: existing compiler debt can shrink, but new or
replacement exports fail. Neither gate uses a function or export waiver list.
