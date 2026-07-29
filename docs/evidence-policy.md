# Evidence and artifact policy

The repository keeps distinct facts, not multiple independently edited representations of the
same fact. For any recovered fact there must be one unambiguous answer to: **which file do I edit?**

## Artifact roles

Every tracked data artifact has one of these roles:

- **Configuration** changes how a command executes. Operational inputs belong under `config/`.
- **Observation** records what a binary, source oracle, matcher, decompiler, or other tool exposed.
  Irreplaceable or reviewed observations belong under `evidence/observations/`.
- **Reviewed conclusion** records a human decision such as an accepted identity, rejected mapping,
  proven layout, or ownership assignment. Conclusions belong under `evidence/reviewed/` when the
  claim carries provenance or relationships that Ghidra cannot express by itself.
- **Ghidra analysis state** is the live representation of functions, symbols, signatures, data
  types, vtables, comments, references, and decompiler decisions. It lives only in the checkout's
  untracked project under `WIZ8_WORK_DIR/ghidra`; do not mirror it in a new tracked table merely to
  replay a native Ghidra object.
- **Projection or report** is deterministically derived from canonical configuration, evidence, the
  Ghidra project, or build results. It belongs under the gitignored `build/` directory and must not
  be edited or tracked.
- **Snapshot** is a generated observation retained because ordinary contributors cannot reproduce
  it without proprietary or otherwise unavailable inputs. Snapshots belong under
  `evidence/snapshots/`, declare their producer and input identities, and require a freshness check
  wherever those inputs are available.
- **Recovery seed** is a derived binary checkpoint used only to recover the live Ghidra project.
  The canonical Wiz8 GZF under `vendor/ghidra/exports/` is the sole current exception. Its manifest
  pins the original binary and tool runtime. It restores into the one canonical project only when
  requested; ordinary queries never create clones or replay it automatically. No accepted fact may
  exist only in that archive.

Directory placement is the primary role declaration. Evidence must not be placed under `config/`
merely because an analysis tool consumes it; generate a compatibility input when one is required.

## Canonical data

1. A fact has one canonical editable home. Other forms are generated projections.
2. Machine observations and reviewed conclusions are separate even when they share an address.
3. Native Ghidra analysis objects are edited in Ghidra. Tracked evidence records provenance,
   externally reproducible observations, cross-build relationships, matcher claims, and other facts
   that are not adequately represented by the Program database itself.
4. A function identity is unique by `(program, address)`. Multiple origins are evidence records,
   aliases, or provenance tokens rather than duplicate identity rows.
5. Classes, functions, assertions, imports, source paths, formats, and cross-build relationships are
   distinct entities and may remain separate tables when those tables own provenance rather than a
   duplicate Ghidra layout.
6. A new CSV must represent a new entity or relationship. A subset, reordered view, source-specific
   view, or report does not justify another tracked CSV.
7. Compatibility inputs for Ghidra, reccmp, or another consumer are generated from canonical data
   and protected by freshness tests until the consumer can read that data directly.

## Generated material and documentation

- Generated outputs go under `build/` by default. They may be tracked only through the snapshot or
  recovery-seed exceptions above.
- Markdown contains stable reasoning, architecture, procedures, examples, and unresolved questions.
  It does not duplicate exhaustive inventories, hashes, match percentages, or current counts.
- Counts, percentages, ownership coverage, and status tables are generated. Do not maintain them by
  hand in prose. Use `just wiz8 report status`, which writes `build/reports/status.md` and
  `status.json`.
- YAML is for small hierarchical execution configuration, CSV is for flat canonical records, and
  JSON is for generated manifests and reports. Choose the format from the role rather than history.
- Beads is the durable task and progress store. Do not add planning CSVs, audit JSONL, or progress
  Markdown.

## Review checklist

Before adding or changing a tracked artifact, answer:

1. What entity or relationship does it own?
2. What is its stable key?
3. Is it configuration, an observation, a reviewed conclusion, or an exceptional snapshot?
4. What produces it and what consumes it?
5. Can the fact live directly in Ghidra instead?
6. Can an existing canonical artifact represent the non-Ghidra part of the fact?
7. If it is generated, why can it not remain under `build/`?

Tests should validate schemas, keys, provenance vocabularies, and generated compatibility surfaces.
Prefer directory- and schema-based enforcement over another exhaustive hand-maintained artifact
registry, which would itself be liable to drift.
