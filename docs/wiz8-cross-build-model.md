# Wizardry executable cross-build model

`config/analysis/cross-build-map.csv` is the reviewed address map. It is intentionally separate
from generated `build/evidence/cross-build-candidates.csv`: regenerating heuristic candidates must
not replace accepted or rejected identities.

The first reviewed batch contains 42 mappings for 14 named canonical functions across the demo,
1.261, and 1.28 executables. It covers the three Wizardry-facing zlib wrappers plus constructors,
copy constructors, complete destructors, and scalar deleting destructors for `GrCycle`, `Monster`,
and `MonsterInfoDialog`.

## Classification

Each row retains both the automated relationship and the review result:

* `exact` means the complete Ghidra function body hash is byte-identical;
* `relocation-equivalent` is reserved for a relocation-normalized byte identity and is not claimed
  by this batch;
* `structurally-strong` means a unique instruction fingerprint agrees with size and supporting
  strings or call shape;
* `candidate` is insufficient on its own and is accepted only when vtable position,
  constructor/destructor adjacency, object layout, and callee identity resolve the ambiguity;
* `manually-confirmed` and `rejected` are review decisions, not heuristic scores.

Reviewed false positives live in `config/analysis/cross-build-rejections.csv`. They demonstrate two
important failure modes: same-address coincidence after patch insertion and non-unique
compiler-generated destructor bodies.

## Oracle boundaries

`config/analysis/cross-build-oracles.csv` records why each build is used. The protected official
retail executable is retained as an official oracle, but its `stxt`-protected body is not assigned
speculative static function addresses. The 1.28 executable is a fan-patch target: confirmed
original-body mappings do not make `Wiz8.dll`, `cfagent1.28.dll`, detours, or injected code original
Wizardry source.

The already reviewed SGP mappings live in `evidence/reviewed/wiz8/functions.csv`, with
per-build observations in `evidence/snapshots/sgp/harness.csv`. Those source-built identities are
independent evidence and should be joined with this map by canonical address when preparing a
function for recovery.

## Fan-patch symbol oracle

`cfagent1.28.dll` contains an embedded English-executable address and signature database. Its
initializer at `0x1000DF50` waits for the executable, loads fixed seed addresses at `0x10004810`,
signature-scans 49 target regions at `0x10004050`, verifies expected bytes for 88 named functions
and hook sites at `0x10004F90`, and installs hooks through `0x10003850`. The byte checker at
`0x10007880` uses `ReadProcessMemory`; the hook installer uses `WriteProcessMemory` only after
validating the target prologue.

This makes names such as `pW8FUNC_StartCombat`, `pW8FUNC_GetFact`, and `pW8FUNC_SetFact` strong
external symbol evidence, not original source. The first 47 unambiguous and semantically useful
seed identities are tracked in `evidence/reviewed/wiz8/functions.csv`; corroborating,
derived, or placeholder-style patch labels were deliberately not applied. The 26 named fixes and
hooks are inventoried separately in `config/analysis/fan-patch-128-hooks.csv` and are explicitly
owned by the fan patch.
