# Record-model recovery after #825

This change started from main `0d3a84089` and is rebased onto main
`af3461d8262e63bb45f6501fdb47e2bc4fb40683`, independently of #825's
`2cb88e883`. It adopts the conditional GrCycle model assignment, the corrected
portrait-gap offset, and removal of the materials `/Ob1` override from that PR.
It does not import that PR's other source or tooling changes.

Retail WIZ8 SHA-256 is
`18a74ff61c65b8a2d4cfa11ffce82ad7fef022a94eaf0c2f217e479e981420d2`.
The live project was restored from main's reviewed checkpoint after preserving
the stale project under `wiz8-work/ghidra-project-preserved-before-record-recovery`.
Doctor passed. Source-projected class names select investigation candidates;
they do not independently establish those classes or their field types.

The investigation includes an executable-section instruction-candidate sweep,
85 decompiled functions selected through party-record and missile references,
and 421 receiver-rooted queries using the existing `field_accesses` owner.
These include 70 Octree methods, 14 3D-model methods, 11 2D-model methods,
both portrait-entry lifecycle methods, and the dialog families. Many queries
report incomplete flow through calls or pointer operations. Neither those
queries nor a linear instruction sweep proves whole-program non-use.
A further 81 parameter-rooted queries cover pointer receivers outside their
class namespaces, including SetPortraitTargetPose and RPCPtrToPCSlot. They
establish no new gap access. Raw decoding also confirms SetTarget at
0x005e31e0 stores the surface at object +8, without accessing object +4. Local
artifacts are under `build/recovery/record-model`; they are not tracked.

## Disposition of the eleven requested items

| Item | Change / evidence | Remaining limit |
| --- | --- | --- |
| 1. Path grid walker | Restore three-element `cell_00` and `step_0c`, the second secondary axis and second error channel. Name delta/reset consistently with the 3D walker. `BuildPathGridWalk` at `0x0045af60` writes all sixteen words, including zero at `+08/+14/+20/+34/+38/+3c`. Both probing consumers use the first error channel and reset. Preserve the 0x40 extent and assert the channel boundaries. | Common shape is supported; this does not prove the two walkers were one historical typedef. |
| 2. Model tail | Keep 3D RGBA and the distinct 2D render-state owner. Remove 3D `displayState()`. The purge at `0x0042654c` deliberately compares the first representation byte of 3D alpha through `memcpy`, preserving retail while avoiding an invented 3D state field. | Original reason for testing both class IDs remains unknown; no float/byte union or shared 2D/3D record is established. |
| 3. Materials helpers | Document provisional source linkage and placement. Remove the unsupported `/Ob1` TU override and provide the exported `srMaterial` default-constructor import needed by ordinary `/Ob2` emission. Preserve the five retail procedure markers and conventional helper bodies. | No missing-PDB workaround, forced inline, linkage change or marker waiver. Original authored organization remains unknown; `MaterialSort` does have its own diagnostic spelling. |
| 4. NPC bytes | Split `+2ef` as `combat_script_notice_enabled` and `+2f0` as `allow_dismissed_departure_dialogue`, update both consumers, assert both offsets, retain only the unexamined `+2f1..+308` tail. They remain bytes, not normalized bools. | Binary-loaded nonzero values do not prove historical bool spelling or names. |
| 5. Portrait entry gap | Correct the actual `+025` spelling and establish its boundary at the event pointer `+071`. Neither lifecycle method accesses the gap. Whole-record reset at `0x0054b323` clears 0x46 dwords (0x118 bytes), including this span. The candidate sweep found no direct absolute member access within the gap. | Pointer-based party operations and whole-entry clearing are not evidence for 76 named fields or padding. The span stays opaque; its semantic recovery is **open**, not completed by the rename. |
| 6. Dialog base tail | Name `+48` as destruction-callback context, retaining `void*`. Base constructor `0x005dc7f8` clears it before any derived construction; base destructor invokes the callback stored at `+44`. Trigger installs itself and consumes it in the item-picker callback. This supports base-owned callback storage rather than a field introduced only by that subclass. | Exact historical pointer spelling remains unknown. Receiver queries find no new role for `+38..+3f`; `+4c` is cleared by construction/numeric deactivation, without a recovered nonzero writer or reader. These spans remain unresolved. |
| 7. GrCycle null path | Remove the count-to-pointer storage-reuse artifact. Keep the conditional model assignment. Successful model consumption requires a nonnull cached instance: selection starts from it, and if selection remains null the cached instance must have been null. The unconditional transform read then dereferences that cached instance before the model local is used. | This bounds defined execution; it does not prove every asset avoids the earlier null dereference or recover original local initialization. No guard or default initialization is added. |
| 8. Bink word | Replace the unsupported `int` with four unknown storage bytes, clear the same extent in construction. Inspect all seven methods and the intro allocation, dispatch and deletion paths, including interior addresses and aggregate instructions. | No object `+4` reader, escape of that interior address, or aggregate object copy was found there. HBINK `+4` and the DirectDraw surface/vtable `+4` accesses are different receivers. No claim of reserved/dead storage or semantic name. |
| 9. Octree byte | Name the observed producer result `m_location_region_matched`: visibility clears it; successful region matching sets it. Preserve byte storage and Reset's omission. Inspect wider accesses crossing the byte and all 70 receiver-rooted Octree methods. | The only confirmed receiver accesses are stores at `0x004306bb` and `0x004313fc`. No reader was recovered; indirect/escaped coverage remains incomplete. The name describes the producer, not an invented consumer contract. |
| 10. Main-game layout | Name the two six-pixel mode-dependent layout insets. Replace the six constant-index party slots with condition, enchantment, character-name, vitals, combat-action and assay hover slots; update every producer/reset/drawing consumer. Assert all six offsets, preserving the 0x330 owner. | The inset producer establishes the mode association, not a recovered wider layout engine. |
| 11. Large active records | Trace all 26 ProgramDB references to the missile-table pointer in 14 functions, including loader/free, launch, hit resolution and embedded-effect copying. Inspect message-dialog constructor, controls, draw, message wrapping and destruction, plus the derived receiver census. Recover the UTF-16 display-name arrays from the canonical database payloads: missile `wchar_t[128]` and level `wchar_t[30]`, with assertions on their following field boundaries. Separate the monster remains name from the treasure owner and recover its shared flag/protection family. Preserve the remaining opaque spans below. | Missile `+148..+14f`, `+169..+1e4`, message-dialog `+064..+073/+07c..+08b`, and the remaining level/NPC/monster database blobs have no supported new subrecord/type boundary from this investigation. They remain **open**. |

## Receiver and boundary findings

The 3D constructor (`0x0047ed20..0x0047ed32`) and reset in assignment
(`0x0047ee04..0x0047ee16`) write four words. RenderMeshes independently performs
float loads of all four at `0x0047fb14/+27/+3a/+4d`, and again in its subsequent
passes. GrCycle copies all sixteen RGBA bytes. By contrast, the 2D constructor,
assignment, sprite builders and display-state setter use a single byte at
`+170`. The byte tests also occur in expanded video lifecycle paths. A class-ID
check distinguishes objects; it does not demonstrate overlapping authored
storage within the 3D object. The purge's representation read stays at the
consumer boundary instead of becoming a member API.

For GrCycle, the normal caller at `0x004a7470` stores the selected instance at
`0x004a765a` or `0x004a7869` before invoking the particle updater. Within the
updater, `0x004a7e7f` loads that cache and only performs fallback selection when
it is null. `0x004a7f0b/+17` reload the cache for imported transform calls.
SurRender `srNode::getRotation(float)` at `0x10053e13` immediately loads
`[ECX+18]`, without a null guard. Thus the selected-null path cannot reach a
model-local read in defined execution. Retail's load of the prior count slot
at `0x004a7eff` is compiler storage reuse, not an authored pointer conversion.

Base-dialog ownership is independently visible in its constructor and size:
the base initializes `+48` itself and derived state starts at `+54`.
The callback's Trigger payload does not justify narrowing every base to
`Trigger*`. The exact historical spelling of `void*` cannot be proved from one
payload, but it expresses the observed erased callback contract without
inventing a subclass overlay. The `+4c` numeric-input clears do not establish
that the slot is a counter, pointer or boolean.

Missile database loading at `0x004a5600` skips a separate 0x101-byte disk prefix
and reads exactly 0x1e5 bytes into each runtime row. The first unknown 0x100 bytes
are **inside** that runtime row, not the skipped editor prefix. The condition
copy in Magic covers only `+155..+164` and the following magnitude load uses
`+150`; neither crosses `+148..+14f`. No wider scalar is inferred from these
copies. The retained leading 0x100 bytes are a terminated UTF-16 name in every one
of the 36 canonical missile rows (Arrow, Acid Splash, Energy Blast, and so on),
followed at +100 by the independently consumed cycle basename. The full file
has exactly `8 + 36 * 0x2e6` bytes. Its SHA-256 is
`ced8957f91cbb679aa50fba9c58bfd790016536baefda9924ec1b68c39b9c65f`.
The 60 level rows similarly begin with 30 UTF-16 code units, followed by the
encounter fields at +3c. The level file has exactly `4 + 60 * 0xd8` bytes;
SHA-256 is `6edb57a5ea763212f1b843e285628df744718f5719ee7bbe9d4c53e99669773e`.
Level name buffers can retain characters after their first terminator; they
are not assumed zero-filled. Missile +148..14f and +169..1e4 are zero across
this corpus, which does not establish reserved fields or their historical
types. Level +58 contains the row index in the corpus, but without a typed
consumer it stays unknown. Missile interpretation outside these consumers,
including editor semantics, is still missing. Message-dialog control handles/images bracket
the two unknown 16-byte blocks, but their stride or proximity does not prove
two embedded control objects. Remaining database spans require typed
producers/consumers; this pass does not mechanically partition them.

## Follow-up investigation of the unresolved spans

The second investigation uses native instruction boundaries and the existing
`trace_accesses` walker directly for global roots. The public parameter-rooted
query cannot accept these roots. No replacement query framework or projected
source synchronization was introduced. Native references select candidates;
P-code and instruction operands supply the access evidence.

### Portrait record receivers and escapes

The native listing identifies 102 references to the leading eight records in
70 functions. Seeding the actual address-producing P-code outputs yields 71
root traces and 251 accesses. There is no individually attributed load/store
in +25..70 in those traces. They stop at 33 ambiguous joins and four narrow
quote-coordinate values subsequently used in address arithmetic. These stops
are retained as coverage limits, not counted as proof of non-use.

The direct escapes have distinguishable roles:

| Site | Receiver / role | Consequence for +25..70 |
| --- | --- | --- |
| `0x0050b8a2` | `GetNpcGroupEntry` returns the selected party record. NPC scripting uses its sound handle, voice duration, mouth-gap track and active event. | The return preserves a whole-record pointer; it does not establish a gap member. |
| `0x0052390a` | `SetPortraitTargetPose`, whose parameter-rooted accesses are +8d and +99. | Both accesses are beyond the gap. |
| `0x00525a79`, `0x0052d44a` | `LoadMouthGapTrack` receives the embedded track at +5. | Its 0x14-byte extent ends at +19, before the quote and gap. |
| `0x0052f934`, `0x0052fbac` | `RPCPtrToPCSlot` compares the pointer against the eight record addresses. | It identifies a slot without reading the record. |
| `0x00538940`, `0x0053aa3c` | Targeting passes the highlighted-monsters vector at +d8. | The vector has its own extent beyond the gap. |
| `0x0054b323`, `0x0050b6b6` | Reset/dismiss clear 0x46 dwords. | The gap participates in whole-record clearing. |
| `0x005e048d`, `0x005e051a`, `0x005e05e5` | Character-summary save/restore copies 0x46 dwords between the leading party record and the embedded saved record. | Copying includes the gap but does not recover its field boundaries. |

The follow-up inspects the incoming definitions at those joins rather than
assuming that every phi preserves a record pointer. Twenty-three pointer-loop
roots yield another 153 accesses. Their record-iteration backedges advance by
0x118; none supplies an individually attributed gap access. The skipped sites
include six whole-object clear/copy loops, two highlighted-vector element joins
and two portrait-bar joins. The four narrow-value stops are quote coordinates
at +1d/+1f/+21/+23, rather than pointers into the gap. Native references to
`GetNpcGroupEntry` identify three caller functions and four returned-pointer
roots; their accesses cover +1, +81, +114 and the event pointer at +71. The
mouth-gap loader still receives +5 and consumes only the preceding 0x14 bytes.
The whole-record loops must not be reinterpreted as 76 independent fields.
These bounded investigations support neither an exhaustive non-use claim nor
turning the span into padding, a text buffer or a portrait-control subobject.

### Applied portrait table and timing corrections

The former `W8PortraitTables::pose_transition[30]` included five dwords of
`0x01010101` that actually belong to the separately declared byte-per-portrait
flags at 0x0061cbc0. Quote coordinates are independently indexed ushort arrays
at 0x0061cb3c and 0x0061cb4c (`0x0052fa8f`, `0x0052fa9b`, `0x0052fab3`). The
transition table starts at 0x0061cb5c and contains 25 ints. Retail's two loads
at `0x0052e998` and `0x0052ea24` address its five rows and five columns using
one-based pose IDs. The recovered source now gives these arrays separate
owners and uses `[pose - 1][target - 1]`; its complete 100-byte extent ends
exactly where the portrait flags begin. All initializers and all 25 address
pairs were checked against the retail bytes and addressing expressions.

The record's +81 voice duration is now unsigned: `0x0052e855` compares it
against 120 and `0x0052e85c` branches with JNC, before the countdown subtract
at `0x0052e8d8`. The previous signed declaration changed the branch for values
with bit 31 set. Both sound-length and NPC message-duration producers supply
unsigned milliseconds. The five portrait clocks and two queue clocks now use
the SGP `TIMER` type produced by `SetCountdownClock` and consumed by
`ClockIsTicking`; uninitialized clocks and the -1 follow-up sentinel remain
unchanged. Portrait and NPC voice handles now share the unsigned sound-ID
type, removing the NPC shutdown's scalar cast and using `SOUND_ERROR` for
failure comparisons and resets. No new gap members are invented.

### Database receiver attribution and corpus checks

An additional 241 parameter roots cover NPC state, monster runtime state and
monster database consumers, including 28 direct monster-record parameters.
A separate 29-function/root pass follows `g_npc_records` at 0x006836a0,
`g_level_records` at 0x006836a4 and the missile-table pointer at 0x0065bde0.
The 29 root traces produce 371 accesses and stop at 306 ambiguous joins,
including repeated global-pointer versions around loader calls. They are not
an exhaustive escape proof. They recover no new individually attributed
accesses to the unknown NPC, level or missile tails. Accesses at +27c/+280/+284/+288 and +28d/+290/+294 through
`W8MonsterInfo +0c` belong to the live monster object, not to a monster database
row. Their numerical coincidence with database-tail offsets supplies no field
evidence for the database owner.

Following the canonical NPC loader's variable item-stock rules traverses all
146 records and ends exactly at byte 118674. Both +2da..2e9 and +2f1..308 are
zero in every row. The NPC payload SHA-256 is
`40f7174d99a298c7d62d23760eafb49f08ba184321538c7bbe1a9db526bc1eaf`. This supports a
bounded corpus observation, not a reserved-field declaration; mods or missing
editor semantics could assign roles that this corpus does not expose.

### Octree preprocessing and Bink ownership

A further 34 parameter roots cover the preprocessing tree, its consumers and
geometry records, including the derived OctPreTree methods and materials
helpers omitted from the original W8Octree namespace selection. None yields a
receiver-attributed access crossing +16b. Existing W8Octree calls pass embedded
buffers from +170 onward to file routines; their extents do not cover the
flag cluster. Observed full-object calls lead into the octree/spatial
family; this does not prove every indirect call target. Ambiguous joins in the rooted queries remain a limitation; no reader
contract is inferred from the producer stores.

A byte-reference check of the executable finds 14 occurrences of the gpVideo
address, all in the inspected intro span 0x005ae510..0x005aea00. SetTarget's
raw instructions store the DirectDraw surface at object +8. Neither those
methods nor the intro paths establish a consumer of object +4. An executable-section E8 target scan finds ten calls to the seven Bink
methods: eight in the intro and two from Update to the copy methods. All ten
are confirmed by the raw instruction listings; no second construction site
is found. This rules out those specific suspected escapes; it cannot recover
a historical field type.

### GrCycle null-selection contract

Retail `AnimObjDispatch` at 0x004a14d0 returns null for an absent mesh;
`AnimObjDispatchList` at 0x004a1560 returns null for absent lists/entries, and
`AnimObjListCount` at 0x004a1620 returns zero for an absent list.
`GetAniMeshFrame` at 0x004b6550 also returns null after a failed load or an
invalid frame. These are actual retail branches, not merely source types.
The representation implementations dispatch through these helpers rather than
providing a universal nonnull return contract.

In the running-animation branch of UpdateRepresentation, a zero list count
skips the cache assignment and still reaches UpdateParticleAttachments. The
non-running branch dereferences the selected instance before invoking that
updater. This strengthens the distinction between an earlier null dereference
and an uninitialized model-local read: no defined null-selection execution
reaches the latter, but a general nonnull invariant cannot be inferred merely
from the selector declarations. Asset reachability and historical local
initialization remain unknown. No initialization, guard or count-to-pointer
conversion is justified by this investigation.

### Monster database fill-pattern evidence

The canonical `MONSTERS.DBS` payload has SHA-256
`a75f4b125e344980e63024e78ac426715c2813f5c75247927fcd11da424458af`.
Its four-byte count and 595 rows of 0x297 bytes consume the file exactly,
matching the loader's row boundary. In `+273..+296`, 573 rows are all zero;
the remaining 22 are entirely `0xcd`. The same 22 rows contain only `0xcd`
at `+24b..+24e` and `+269`; all other rows have zeros at those spans.
The affected zero-based rows are 10, 11, 356..371, 378, 406, 407 and 504.

Repeated `0xcd` is consistent with VC debug-heap fill surviving serialization.
This is evidence against interpreting these nonzero bytes as meaningful
scalar values. It does not prove that the original slots were padding,
reserved or never consumed. The receiver census still establishes no new
typed consumer, so the declarations remain unknown byte storage. Other opaque
monster spans contain nonzero data without this uniform pattern; the corpus
alone does not supply their field boundaries or types. Exact checks are saved
locally in `monster-tail-fill-evidence.json`.

### Applied monster-record and protection recovery

The 48-byte prefix previously hidden inside the treasure block is now
`char remains_model_name_1c3[48]`, independently of the actual treasure block
at `+1f3`. All 595 rows contain terminated, zero-filled strings there:
55 `creaturegoo`, four `plantgoo`, and 536 empty. Data.slf contains matching
`ITEMS3D\\CREATUREGOO.ITM` and `ITEMS3D\\PLANTGOO.ITM` assets. Cosmic Forge's
export reads the narrow string at `0x004a03a0/+ad` into column 90, named
`Remains` by string resource `0x1ed`. Retail DropMonsterLoot starts the
ten-byte entry loop at chance byte `+1f8` and rolls gold at `+243`.
The treasure owner is now 0x54 bytes, with eight entries and gold dice; both
consumers and the name/treasure/gold boundary assertions are updated.

Cosmic Forge's paired Bodyguard control `0x761` reads bit 4 at
`0x00713337/+3e` and writes it at `0x00719d62/+75`. Control `0x762`,
`Vulnerable from behind`, reads and writes bit 2 independently. Retail gates
action-8 readiness at `0x00545bfc/0x00545b4c` and the armour penalty at
`0x0054695b`. The shared flags now name NPC, rear vulnerability, bodyguard and
alternate-name roles. All flag consumers use the canonical masks.
The three falsely attack-named predicates become `CanMonsterProtect`,
`CanMonsterProtectCombatant` and `CanMonsterProtectTarget`. The candidate
predicate compares disposition against friendly (2), then tests range, level
and injury/formation; its callers select protection targets and interceptors.
Their behavior, including the formation receiver, is preserved. The database
stamina dice now name the roll that initializes both current and maximum
stamina at `0x004e39de..0x004e39f5` and in the reset loop.

These are applied source changes, with the external mappings in the existing
monster-field evidence CSV. The independently downloaded CosmicForgeU.exe
4.38 SHA-256 is
`404aa27cac6bd9270cfaa8fca5958971079303f652d9bee5b6d423ce5c807c2a`.
Its binary and extracted resources remain untracked.
