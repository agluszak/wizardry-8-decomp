# Pre-portability tasks

This is the work queue for the matching reconstruction, before a separate portable tree.
Completion means reviewed source/evidence, not a matching percentage. Generated queues live in
`wiz8 report semantic-debt`; no platform substitutions or RAII conversions belong in this batch.

1. **Semantic model** — extend the scalar report to identify ABI-dependent declarations; review
   the existing storage, cast, enum, stride, duplicate-layout and source-shaping queues by owner.
   TODO: resolve reviewed width/signedness/domain defects across complete consumer families.
2. **ABI width** — inventory `long`, `size_t`, wide text, enum representation, pointer/integer
   transport and pointer-size contracts. TODO: review aliases and every transport boundary;
   preserve original spellings in matching source.
3. **Metadata** — inventory structured recovery/comparison comments separately from prose.
   TODO: make unstructured annotations precise and verify stripping only recovery markers
   preserves behavioral comments. Emission identity remains owned by reccmp.
4. **Stale comments** — shorten unresolved-TU diaries without claiming new ownership; scan
   tentative prose independently of recovered function identity. TODO: review remaining claims.
5. **Source/TU ownership** — consume the existing unmapped-source/anchor queue.
   TODO: establish original paths only from accepted anchors or source oracles; retain unknowns.
6. **Lifetime** — inventory pointer-bearing records and reviewed allocator/ownership contracts.
   TODO: review array/scalar destruction, copies, intrusive references and process globals.
7. **Binary layout** — attach disk/external/runtime/mixed classifications to canonical records.
   TODO: review unclassified layout assertions and packed records, especially mixed records.
8. **Platform dependencies** — report subsystem entry points with their source owner.
   TODO: review gameplay exceptions against existing SGP and SurRender interfaces.
9. **Header leakage** — separate Windows from DirectDraw includes; report platform tokens in
   public headers. TODO: use compiler include dependencies to verify each remaining include.
10. **Compiler boundary** — report calling conventions, declspecs, pragmas, CRT extensions and
    each inline assembly site. TODO: classify each assembly block from instructions; keep
    compatibility, external ABI and game semantics distinct.
11. **Windows x64 analysis** — add an isolated compile-only x64 diagnostics configuration and
    run it. TODO: review failures without disabling layout contracts or changing product ABI.
12. **Behavioral oracles** — retain boot/menu, new/load game, world, combat, equipment,
    save/load, 2D and renderer references through the existing dynamic oracle.
    TODO: capture repeatable retail/recomp observations with exact binaries, providers,
    fixtures and tool provenance. Missing or failing captures block the fork.
13. **Components** — record source/substitution decisions for Bink, Miles, DirectX, zlib/unzip,
    JPEG, SGP, SurRender and CRT/STL. TODO: decide closed middleware replacements and the
    SFI distribution boundary before portable implementation.
14. **Portable conventions** — gameplay stays free of platform conditionals; use existing
    subsystem interfaces; SDL types stay private; semantic widths and disk representations
    are explicit; no pointer storage in 32-bit integers, global compatibility macro growth,
    or gameplay rewrites. Port first, modernize in separate behavior-preserving changes.
15. **Fork gate** — enumerate unresolved semantic, ownership, width, platform and capture debt.
    TODO: review the queues, validate the behavioral references and meet the fork criteria.
    First portable milestone: modern Windows x86/x64 using the same game architecture.

The audit is source/projection evidence, not independent retail proof. Lexical findings are
candidates, not declarations or confirmed bugs. Neither an inventory nor an x64 compile failure
completes a recovery task; this checklist stays open until its review obligations are met.

## Current implementation

`uv run wiz8 report semantic-debt` includes the ABI, compiler, assembly, public-header,
layout and pointer-field queues, with reviewed classifications in `config/pre-portability.json`.
The saved scalar report also lists ABI-dependent declarations and attributed pointer/integer
transport edges, without using those facts as retail width evidence.

`uv run wiz8 diagnostics --x64` configures a separate compile-only tree and retains failures
in `build/logs/clang-x64-diagnostics.json`. The initial run is **toolchain-blocked**: VC6's
headers do not define an x64 Windows target and disagree with the compiler's 64-bit `size_t`.
Source layout failures are visible but need review with suitable x64 SDK/library headers.
This lane does not replace the i686 source projection or alter product layout assertions.

`uv run wiz8 analyze freeze-oracle differential.json --output reference.json` retains the
existing dynamic oracle's successful differential and complete event captures. It checks
capture hashes, binary/provider/MAP/fixture provenance and retail repeatability, and refuses
overwrites. Captures remain optional local artifacts; they are not added to routine CI.
The nine required behavioral references are still missing; inventory and freezer code are
not substitute captures. Closed Bink/Miles substitutions remain undecided.

The first source batch names 14 established shared floating-point constants across their
consumers, retaining each definition, value and `GLOBAL` identity. Generic zero/one names
avoid assigning a single gameplay role to shared storage. The monster-create failure path
now assigns integer zero directly, as established by its null-return branch. The SGP text
boundary asserts 16-bit Wizardry code units. The scalar collector distinguishes function
addresses from call results; saved report inputs carry hashes and explicitly unverified
source freshness. Existing pointer-valued mesh/path hash specializations must not be
reopened from older scalar snapshots that still describe them as integers.

The next source batch names 115 spatial/pathing/world fields across 193 marked
function regions. It distinguishes borrowed triangle and node views from owned
region/search storage and adopted light references. Mesh float positions now use
the existing vector type; allocation and transfer sizes retain retail's doubled
non-LOD reservation. Sixteen level-file records have reviewed disk/mixed layout
classifications, and eight additional allocator/lifetime contracts identify their
canonical owners. Unresolved payload types and unused bitset roles remain open.
