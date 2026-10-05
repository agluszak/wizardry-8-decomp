# Comparison consolidation plan

Goal: fewer concepts, one producer for each fact, and reports that consume results
without reconstructing comparison policy. No additional comparison backends,
normalization modes, rule registry, compatibility aliases or equivalence engine.

## Ownership

| Owner | Responsibilities |
| --- | --- |
| Ghidriff | Generic decompiler spelling normalization, declaration/body splitting, text diffs and similarity. |
| reccmp | Pairing and identity, reference-aware address normalization, Ghidra preparation, referenced data, analysis diagnostics and comparison-pass selection. |
| Wizardry | Reviewed project facts, products and source selections, emission gates, CI orchestration and presentation. |

Generic parameter/temporary names belong in Ghidriff. Catalog-dependent addresses
belong in reccmp. Project-specific assertion contracts are facts supplied by
Wizardry through existing preparation inputs, not hardcoded generic rules.

## Batch 1: one text-comparison owner

- Add one immutable Ghidriff text result holding normalized text, declaration
  diff, body diff, body similarity and the existing narrow signedness tag.
- Use one comparison operation in Ghidriff's engine and reccmp's ordinary/inline
  paths. Remove reccmp's independent unified-diff implementation.
- Consolidate lexical handling. Identifier normalization must preserve strings,
  character literals and comments. Keep intentional warning/export placement
  normalization separate from code-token transformations.
- Move generic default parameter naming out of reccmp into Ghidriff.
- Audit existing equality, bit-test, auto-temporary and local-step normalization.
  Require deterministic, idempotent output and negative fixtures for changed
  widths, signed comparisons, literals and floating-point order.
- Retain referenced-data checks even when body text agrees.

Acceptance: different quoted FUN_/DAT_/address-like strings remain different;
signature-only changes remain separate; all text diff/score producers call the
same operation. No source model changes to improve scores.

## Batch 2: one complete function result

- Keep ordinary and optional inline evidence in separate immutable pass records.
  Each contains its text result, referenced-data findings and diagnostics.
- Give temporary Ghidra mutations one transaction owner. Flush caches around
  rollback. Retry reference collection must not overwrite ordinary evidence.
- Select an eligible retry deterministically, never by score. Preserve failed
  retries as analysis failures and retain ordinary evidence for
  inspection. Avoid retries for already-clean ordinary comparisons.
- Replace overlapping normal_diff/inline_normalized_diff/code_diff payloads with
  pass records and an explicit selected-pass identifier. Update consumers
  together; remove obsolete fields instead of maintaining aliases.
- Publish the selected score in reccmp's result. Remove Wizardry's join against
  Ghidriff's ordinary modified-function ratios. Aggregate complete reccmp facts.
- Preserve Ghidra warnings and preparation corrections with their evidence
  provenance. Known incomplete analyses use the existing analysis-failure
  outcome; warnings are not silently converted into equivalence or confidence.
- Investigate #877's portrait tail-call this-argument inference in the existing
  preparation path. Use independent retail call/callee evidence, not projected
  source signatures. Test focused versus larger selections and cache reuse.

Acceptance: outcome, diff and score identify the same pass. Ordinary evidence is
unchanged by retries. Data changes and incomplete analysis remain visible.

## Batch 3: one reproducible CI path

- Build each source revision using its own build configuration. Compare both
  products with the same pinned comparison implementation and policy.
- Remove dependence on importing whichever Wizardry implementation is checked
  out while keeping head dependencies installed with --no-sync. Supply explicit
  product paths and manifests to the established comparison owner.
- Use the provided CI checkout; do not add another clone or worktree.
- Publish source/binary/selection and analysis/preparation/normalization
  fingerprints, reusing existing cache identities. Centralize their construction
  rather than introducing new cache stages or refresh switches.
- Keep head artifacts when baseline execution fails. Identify the failing stage
  and revision explicitly. Fix broken baseline source rather than waive gates.
- Remove obsolete helpers, lifecycle handling, tests and documentation as their
  replacement lands. Historical reports stay archived evidence; the live
  pipeline does not acquire permanent compatibility readers for retired formats.

Acceptance: CI evaluates base and head with explicit comparable tooling inputs;
cache reuse cannot mix policies or selections; reports are reproducible from
their recorded inputs and retain partial evidence on failure.

## Regression evidence and boundaries

Use existing #884, #893, #877 and #900 reports as the starting evidence. Track
small authored fixtures for demonstrated bugs, not extracted products or full
licensed decompilation inventories.

- #893: constructor declaration-only changes must not become body regressions.
- #884: unsigned condition comparisons remain body differences. High-bit values
  invalidate an unconditional signed/unsigned equivalence claim.
- Literal corruption: strings, characters, multiline comments and escaped
  quotes must survive identifier normalization.
- Inline retries: record both passes, preserve changed helper data, expose
  failures and calculate the selected pass's own score.
- Tail calls: investigate dropped ECX/this arguments and inference dependence
  on selection/cached analysis before changing source.
- Clamp/control-flow and vector-length cases remain investigation candidates.
  Do not suppress arbitrary branch restructuring or floating-point reassociation.
- Any evidence-backed source correction is a separate source batch, preserving
  canonical ownership, retail bugs, widths and observable behavior.

## Validation and publication

Batch changes before checks. Pure text/report tests and saved-fixture replay are
the normal local lane. A small existing Ghidra integration lane checks transaction
isolation, preparation consistency and cache reuse. No combinatorial test matrix.
Product builds and fresh retail comparisons remain in CI.

Publish tested changes directly to our Ghidriff main and reccmp master, then pin
exact commits in Wizardry through its Jujutsu/PR workflow. No upstream submissions.
Invalidate prepared caches when preparation changes; implementation fingerprints
already invalidate completed comparisons. Separate tool representation changes
from source changes in reported results.

## Progress

- Batch 1: implemented. Generic normalization and text comparison now live in
  Ghidriff; reccmp consumes the shared result and lexer. Literal/comment safety,
  meaningful-difference preservation and idempotence fixtures pass.
  Fork revisions: Ghidriff `63d09e8`, reccmp `33778b3e`; 59 focused Ghidriff tests and 70 reccmp
  tests passed. Wizardry integration/pinning is part of the same batch PR.
- Batch 2: implemented. Ordinary and inline passes own their text, score, data
  findings, failures and warnings. Results identify one selected pass; failed
  retries still gate completion. Retry references no longer overwrite ordinary
  references, and one transaction owner flushes caches around temporary flags.
  Removed the overlapping diff fields and Wizardry's ordinary-score join.
  Preparation diagnostics persist in private ProgramDB properties with provenance.
  Direct tail-only callees now participate in binary-derived parameter inference;
  focused, larger and repeated-state native fixtures recover the ECX argument.
  This fixes a demonstrated preparation gap; the full #877 portrait result still
  needs CI comparison. Fork revisions: reccmp `ee66e5b5`, then `b6268d6a` fixing
  the CI-discovered absent recomp address in unpaired-row warning collection. The
  combined reccmp batch has 87 passing focused tests, including six native Ghidra
  cases; focused typing and lint checks passed.
- Batch 3: implemented. Wizardry freezes selections and products in reccmp's
  canonical manifest; reccmp replays it without rediscovering the current catalog
  and rejects replaced binaries. One immutable input record supplies completed
  cache identity and report fingerprints. Reports include source revision, PDB
  and source-index hashes alongside binary, selection and tooling inputs.
  CI snapshots the head Python package once, builds baseline source with its own
  build configuration, and compares using the same head policy and dependencies.
  The Python baseline lifecycle replaces the shell helper, records failures by
  stage/revision, retains partial evidence and always attempts head restoration.
  Delta gates reject different original binaries or comparison policies. Product
  builds and fresh retail results remain CI validation; no new match-rate claim
  is made from the local tooling fixtures.

Starting completed CI inventory (#900): WIZ8 5,389 analyzed, 2,657 clean, 2,732
different, 81.69% average similarity; SURRENDER 2,245 analyzed, 1,532 clean, 713
different, 88.22% average similarity. Both had zero analysis failures. These
historical averages still mix ordinary scores with inline outcomes and are not
the acceptance criterion for this refactor.
