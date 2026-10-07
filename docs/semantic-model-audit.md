# Semantic-model audit

This records the 2026-10-07 audit against main commit `45543c2c`. It is a review
record, not a declaration that reconstruction or portability is complete. The
live queues remain owned by `wiz8 report semantic-debt`, scalar facts and
`config/pre-portability.json`. Source-index and PDB facts describe recovered
source; they do not independently establish retail types.

## Corrections established by this audit

| Owner | Independent evidence | Correction |
| --- | --- | --- |
| `W8GameTimer`, `GDProp`, animation-load and camera-shake flags | Retail constructors zero the complete flag storage; VC6 copied uninitialized POD temporaries or omitted aggregate member initialization. Camera-shake retail `004ADED0` clears its complete four-byte flag word. | Initialize every aggregate member before copying the flags. |
| `LevelFile` writers | Retail `WriteAnimObj` at `004D4480` and `WriteProps` at `004D4FC0` serialize their fields in order. | Sequence all 14 groups of file writes explicitly while retaining non-short-circuit failure accumulation. |
| `W8GameData::ReadWGDList` | Retail `004476A9` reads the vertex count before the face count at `004476C3`. | Sequence the two reads explicitly while retaining the bitwise failure accumulation. |
| `srVertexProcessor` | Provider bodies `10035470` and `10035480` return true and perform no work respectively. | Restore the non-pure base virtuals. |
| `srFog` | Class ID `1210`, parent `1200`, fog assignment in `1004BEA0`, support-phase tables `10076FE8`/`10076FF4`, and constructor/destructor stores. | Restore the fog registry-support layer, ordinary base lifetime and implicit copy construction. |
| Header-visible SurRender accessors | Retail consumer instructions directly read vertex-pipe/node/scene fields or dispatch clone through its virtual slot; the corresponding standalone accessor imports are absent. | Put the evidenced bodies at their canonical header owner and retain the provider emissions. The normal texcoord mapper requests newly generated ST storage. |
| Monster-light copy | `0049D660` constructs the illuminator/support/light base phases, assigns the light and installs imported light tables before copying monster fields. | Use ordinary `srLight` copy construction instead of reconstructing the inherited fields in the derived body. |
| Default octree material | `0049EA0A` calls imported `srMaterialIFace::operator=` and then proceeds directly to the texture/shader setup. | Assign the interface base only; preserve the newly constructed material's own defaults. |
| Virtual streams | Retail virtual-file construction installs imported directional-stream tables; BitArray unwind uses the imported virtual-base destructor closures. | Restore the consumer class/lifecycle boundaries and the header-visible empty stream teardown. |
| Critical-section lifetime | Global teardown `10050300`, heap `10035A50`, scheduler `10013E30`, registry `1000EAD0` and read-job `1002EC70` drain the section before deletion. | Put Enter/Leave/Delete in `srCriticalSection::~srCriticalSection`; use ordinary deletion in callers. The global scene-graph section now receives the same teardown. |
| SurRender CRT labels | `1006FED6` returns `_onexit`'s function pointer; `1006FF02` converts it to `atexit`'s integer success/failure result. | Correct the swapped library metadata names. |
| Scalar collection | Replayed conditional declarations reused field keys; unknown layout replay was treated as disagreement. | Merge observations at their record owner and retain actual incompatible size/alignment/packing evidence. |

## Semantic review and its limits

The scalar campaign collected 325 translation units, including 295 owned units
and 30 SGP units, with 50,780 declarations, 50,713 flows and 33,893 boundaries.
The independent released-SGP and decorated-provider inputs supplied 2,120 scalar
claims. Direct comparison found no width or signedness disagreement in those
claims. Facts without an independent constraint remain unknown.

The refreshed native catalogs classify all 15,372 Wizardry and 6,496 SurRender
entities, including the six reviewed zero CRT boundary words. The report checks
paired, forward, word-aligned CRT boundaries and actual zero bytes; it does not
classify anonymous table entries as authored functions. The catalog's
inventory explicitly reports `inventory_complete=false`. A zero unclassified
catalog count does not prove discovery of every retail object or function.

Reviewed cast families include SGP text transport, Windows/COM handles, vector
bulk-word APIs, callback contexts and established retail error-path pointer
transport. UpdateMesh's quad-cell byte views still lack a surviving typed
producer; that is an evidence gap, not grounds to invent a record. Opaque fields
that are only cleared/copied and descriptive offset names remain unchanged.
Equal record layouts and coincident byte strides do not establish shared types.
The ten Wizardry byte-count candidates were reviewed as file-reserved blocks,
string buffers or aggregate clears, rather than evidence for unrelated equal-size
records. Four WGD/processed-header records are now classified as disk-format
from their retail transfer sizes, separately from runtime arrays and pointers.

Wizardry's three authored unions serve tagged or phase-dependent payloads:
octree build lists, message-box arguments and spell/item action details.
SurRender registry instance/free-list storage has evidenced active and free
phases sharing the slot. The importer/exporter registration union was artificial:
retail `1002CFA0`/`1002D090` traverse separate lists, and the typed add/find/remove
interfaces establish distinct payloads. Each list now owns a typed registration
record with the same sixteen-byte layout; no node changes kind. Exact original
private record names remain unknown. Compiler storage
reuse does not establish another authored union. Container reviews distinguish
backing allocation, value-element lifetime, pointer pointees and intrusive
references. `srHeapBuffer` still has implicit shallow copy operations; this audit
does not invent deep copying or assert an unobserved retail copy call.

The pre-portability report still contains 565 unclassified layout candidates and
1,517 pointer fields requiring review. Scalar `sizeof` expressions are no longer
misreported as records; compiler-qualified names bind to a unique visible record
owner when one exists. Ambiguous same-name records remain unclassified. The
162 current lifetime contracts include the typed registration records and list
links. Those queues are not confirmed defects, but
they prevent calling the broader ownership/layout review exhausted. Source-bound
contracts do not replace independent retail review. The nine frozen behavioral
references and the closed middleware substitution decisions also remain open.

## Specialized comparison review

Datacmp compares 2,388 Wizardry globals and 483 SurRender globals, including the
reviewed CRT boundary words. SurRender agrees.
The two Wizardry pointer differences are `g_regions[312].callback` and
`g_screen_handlers[2].leave`: retail folds their return-true bodies with
`ScreenLifecycleSuccess`. The reconstruction retains distinct authored callbacks
and links comparison products with `/OPT:NOICF`.

Wizardry has no differing vtable slots. The 12 unpaired table findings are:

- the growable-int deleting destructor and folded trigger/environment destructors;
- the two model-instance secondary-subobject destructor adjustors (subtract `312`);
- material/ground-shadow mapper teardown and folded constant-true activity slots;
- vertical/horizontal thumb, text/range/help control and lock-tumbler/widget
  destructor folding.

The 136 code-equivalent slots have complete return-only bodies. They are not
source aliases or evidence for handwritten deleting destructors.

SurRender has no differing vtable slots. Six virtual-stream warnings were paired
to their compiler vector-deleting symbols after reviewing both the deleting body
and the `sub ecx,[ecx-4]` receiver adjustment. The remaining 15 table warnings are:

| Retail slot entry | Reviewed explanation |
| --- | --- |
| `100049E0`, `10016030`, `100161D0`, `10020F70`, `1004C050`, `100554C0`, `1005DFC0` | Both builds use a direct jump to the class-name getter. These account for 12 tables; native slot resolution stops at a named entry before following its jump. |
| `100493A0` (two camera tables) | The same registry route is also emitted at `10049420`; both request class `1400` below parent `1000`. One rebuilt body leaves the two original identities ambiguous. |
| `1004C7B0` | Both builds subtract `138` and jump to the fog support-layer deleting body. The secondary adjustor remains an unpaired compiler identity. |

The final changed SurRender comparison covers 2,506 functions: 1,617 have no
differences, 761 differ, and none are unpaired or fail analysis. Two in-class
definitions lack standalone rebuild PDB procedures and are classified as inline
non-emissions using current compiler class/method ranges, source hashes and the
complete PDB symbol catalog. This includes
`srGERD::LockSurface::sGetClassName` at `1001F780`: retail emits a standalone
string-return getter while the rebuilt class folds the body into its support-layer
getter. This classification makes no independent body-equivalence claim and is
not proof of another class or source alias. Six header and 120 template
non-emissions also remain visible.

The provider export gate accepts 2,061 exports. Its two additional ordinary
compiler emissions are the `srFStreamOpener` copy constructor and
`srBinIAsyncStream` assignment. Do not hand-author wrappers to suppress them.

Remaining consumer import differences include the malformed original Miles
`_AIL_init_sample@` spelling, CRT wide-character/header expansions, missing
iostream-header initializer sentinels, SGP debug output, material constructor
fallback calls and an unused illuminator support clone assignment emission.
Original header inclusion and compiler emission choices remain unknown where
retail imports alone do not establish authored ownership. These differences are
visible; no dummy initialization or source inlining directive is added to erase
them.

Global initialization and static destruction remain an open comparison family.
The initial emission query found 16 Wizardry and 97 SurRender named
initializer/atexit emissions without pairs. Labels alone do not establish their
behavior. Retail startup passes `005FF000`/`005FF42C` to imported `_initterm`
at `004010BB` through `004010C5`; its C-initializer range is
`005FF430`/`005FF434` at `00401088` through `00401092`. SurRender's CRT attach
path passes `10098000`/`100981A8` at `100702D2` through `100702E1`, also to
imported `_initterm`. The C++ ranges contain respectively 266 and 105 non-null
entries, compared with 91 and 17 in the rebuilt products.

The reviewed retail boundaries now live in reccmp data sources. Reccmp's
earlier CRT stage created anonymous functions before matching's known-identity
gate and replaced their names. Published fork commit `264a2786` instead annotates
already-declared functions, preserving names and types, and includes the matching
PyGhidra/stub dependency pins. It passes
119 focused tests. The existing matcher pairs two Wizardry initializers and their two
destructors, plus eight SurRender initializers. All twelve comparisons complete:
eleven bodies agree and the NPC scripting initializer differs. No generated
`$E` ordinal is used as a stable source identity. The exact tested commit is
published on the reccmp fork's `master` and pinned in Wizardry's dependency lock.

Retail review establishes the eight Wizardry vector/matrix destructor helpers
as return-only bodies. Its four Assay iostream helpers directly construct or
destroy imported `std::ios_base::Init` and `std::_Winit`. All 82 SurRender
iostream registrars register helpers that route to those same imported
destructor families: 41 each. Their compiler organization and header-generated
population differ; the original include placement is unknown. Other provider
registrars route to filter, config, dummy/debug-stream, environment-mapper, heap
and scene-graph teardown. This review found the critical-section lifetime defect
above. Paired initializers are not proof that the complete table order, all
unpaired registrations or their lifetime effects agree; that review remains open.

## Serialization and lifecycle validation

The `level-file` runtime case checks explicit little-endian/IEEE golden bytes,
two animation branches across versions 1 through 9, props across versions 1
through 9, and PathAI modes 0/1/2. Its WGD golden record uses three vertices and one face,
explicit counts/indices, scaling, material/surface/chance fields and bounds.
It checks read cursors and exact writer bytes.
`save-file` exercises status versions 1.0/1.1 and character-record versions 1/2,
legacy field migration, eight character/party rows and preserved live pointers.
It also constructs both camera-shake presets in poisoned storage and checks
the complete flag word. Its synthetic save fixtures establish these migrations,
not every historical save layout.

`save-load-move` executes four save/load/continue cycles across slots 1/2/3/1,
checks exact saved-position restoration and subsequent movement, and verifies
that the save action actually changed a slot. Together with `oct-file`, all 16
runs pass across two repeats in forward and reverse order with identical
observations, including the final WGD/camera-shake changes. All 24 cases in the
full PR runtime suite pass for reconstructed Wizardry with the retail SurRender
provider, including combat round trips, databases, NPC state, audio and shutdown.
The staged `sr.dll` SHA-256 is the retail `cec1caf85861...`, not the reconstructed
provider. These cases do not establish reconstructed SurRender runtime acceptance.

The normal product reaches the menu, takes the quit path and runs the sound,
screen, graphics, input, container, filesystem and memory cleanup. One debugger
capture observes a normal process exit with status 1. Retail `WinMain` at
`004017D4` posts a zero quit message, then returns the previous message's
`wParam` from its stack at `004017DC`; a zero status is therefore not promised
by this path. The dynamic smoke's stricter zero-exit verdict remains false.

PR checks pass after the WGD, camera-shake and registration-record changes,
including 1,090 tests and structural lint. The 31-function I/O-manager comparison
completes without missing identities or analysis failures: 18 bodies agree and
13 differ; both registration inserts and the two-list dump agree. Merge preservation
reports no lost, conflicting, demoted or newly unresolved identities.

The final batch passes PR checks, including 1,094 tests, and merge preservation
with the tested reccmp commit, now published and locked. Its five-family critical-section comparison completes:
three bodies agree and two differ, with no missing identities or analysis failures.
This validates the tooling/source combination. The dependency lock now selects
the published reccmp commit and its matching released PyGhidra dependency.

The previous native Ghidra pin failed two functions in the 4,882-function
changed-source comparison. Both defects are fixed in our Ghidra fork, published
as `739398dee314ba97c20bbaf715f62cdb9a82b2e1`:

- `GetItemEquipSlotGroup`: a preceding switch merges constants and masked
  dynamic indices. Recovery used its nonzero mask as a sixteen-entry bound,
  reading beyond the twelve-byte index map. Existing value-set analysis now
  bounds merged indices conservatively, including constant bit masks.
- `AutomapScreenEnter`: a second inline indirect tail jump was unreachable in
  the partial clone and freed before lookup. Recovery now defers that branch,
  distinguishes empty deferred tables from multistage recovery, and preserves
  the expansion's caller continuation when truncating an indirect tail jump
  into a call. Nested expansions retain their own continuations.

Authored fixtures fail under the old implementation and preserve both indirect
calls, stores and final return under the fixes. The native suite passes 207 unit
tests and 792 data assertions. A fresh comparison against complete original and
rebuilt binaries succeeds in decompiling both Wizardry functions; both still
have ordinary body differences. The local override records its exact native
hash. The published release ZIP verifies against SHA-256
`2a426b4069fceb5bd2d357f06cc46cc498d0655e039b90c1bb295fc1ee2ccd56`.
Doctor passes against the new installation. A normal comparison using its
released native executable (`7d762d54273d...`), without the experimental override,
completes both functions with ordinary body differences and zero analysis failures.
The final changed-source Wizardry comparison covers 5,311 functions, with 1,846
agreeing bodies, 3,259 differing bodies, zero unpaired functions and zero analysis
failures. Nine header, six internal and 191 template non-emissions remain visible.
Wizardry now pins the released revision/checksum and the companion reccmp commit
in its resolved dependency lock.
Switch-focus policy and instruction limits are unchanged.

## Publication validation

The published dependency installation passes doctor and lock consistency. The
final source/tooling gate passes all 1,095 tests, formatting, type checks,
structural/source lint and export validation. Merge preservation passes against
`45543c2c`. The runtime and binary results above are reused because the final
reporting, classification and documentation edits do not change their product
inputs. This batch does not close the remaining layout/ownership or complete
static-lifetime review.
