# Source-model audit

This campaign starts at `c8d01aab` (integration PR #811). Retail instructions,
call sites, exported names and accepted C-source boundaries establish facts;
source-index/PDB types and projected Ghidra names do not independently prove them.
The tables are a burn-down snapshot, not new suppressions or an alternative source
model. “Unexplained” retains a question; it does not authorize changing layout.

## Ownership and calling conventions

### Base-to-derived self casts

The whole authored tree, including multiline C++ casts and C-style self casts,
contains one concrete base-to-derived self-cast family: the three expressions in
`W8Navigator::UpdateLinkedNavigator` (0x00454d70). The navigator is the secondary
subobject at complete-object +0x18. Retail keeps that receiver in ESI, accesses
navigator members at that base, and performs null-preserving ESI-0x18 conversions
before calling 0x004ca2a0; it also converts the linked navigator before the monster
animation-bounds helper. The method is called by navigation mode 0x201 in
`W8Navigator::UpdateNavigation004553A0`. Moving it onto Monster or making these
queries virtual would change the established receiver/call boundary. Retain this
restricted monster-link path. The casts express an actual retail precondition;
they are not proof that arbitrary navigators are monsters.

`srClassSupport` has two CRTP self-downcasts: copy assignment to Derived and
clone copy construction from Derived. The instantiated support owns a Derived
object, and its registry/lifecycle/clone family establishes that relationship.
Its other cast is an ordinary upcast to Base. No additional concrete self-downcast
family was found. `srQuadWord`/`srPixelConvert` self casts are bit/block views;
OctPreTree/OctBuildTree/stModelInstance self-to-void conversions are storage/debug
operations. Two GrCycle const-self casts remain lookup API constness questions.

### SurRender secondary bases

The illuminator constructor at 0x1004c7d0 writes primary vptr 0x10077074 at
complete +0, and secondary vptr 0x10077068 at +0x138. Those exported vtable names
identify srClassSupport<srIlluminator,srNode,...> and srVertexProcessor. At
0x1004c9ad the process method forms complete +0x138 before pushVertexProcessor.
This agrees with the existing base-extent assertion and group mask at +0x13c.
The hierarchy is retained; the redundant explicit srVertexProcessor upcasts in
illuminator/light use the ordinary implicit reference conversion instead. No
reinterpretation or new base is needed.

### Stack-cleaning free helpers

All five non-Win32 Wizardry __stdcall definitions were inspected as one family.
Each consumes its explicit input from the stack; ECX is either unused on entry or
loaded from a stack-provided object before a real member call. RET-N is not alone
sufficient to call a routine free: the argument source is the deciding evidence.

| Helper | Retail address | Stack cleanup | Owner decision |
| --- | --- | --- | --- |
| SetOctreeGameData | 0x0046d7d0 | RET 4 | Stack pointer stored to global; no incoming receiver. |
| IsNavigatorAtTarget | 0x004347d0 | RET 4 | Movement record loaded from stack; internal path cursor is not an incoming this. |
| StepPathCell | 0x004622d0 | RET 12 | Direction and coordinate pointers are stack parameters. |
| LoadSurfacePixels | 0x0047bc80 | RET 12 | Surface loaded from stack before its vtable is called. |
| LoadSurface | 0x0047c090 | RET 8 | File/header loader returns allocated surface; no incoming receiver. |

SurRender's 0x10035620 has exactly three incoming references, all direct CALLs
from srGlobalRecycler::allocate at 0x1003578b/0x10035863/0x100358a1. There is no
observed address-taking/function-pointer registration. It loads size from ESP+4,
loads global heap 0x100a48d0 into ECX, calls srHeap::allocate at 0x100362a0 and
returns with RET 4. This is a TU-local ordinary adapter, named
`AllocateRecyclerStorage`; exact original spelling is not claimed. It is neither
a discovered allocator callback nor a compiler thunk with a moving receiver.

srTimer::getTick/RDTSC are already static members stored in m_read_tick's
__stdcall pointer type. Retail 0x10061060/0x10061080 each reads its output
pointer from the stack, writes both quad-word halves, returns 1 and RET 4;
ECX is loaded from the argument rather than consumed as an incoming receiver. Window-out stream/window/timer handlers have explicit
Win32 callback contracts. Plugin/Info-ZIP adapters have external callback/export
contracts and are inventoried separately below; the noteArchive password-slot
argument mismatch remains a compatibility investigation, not a new member.

## Campaign decisions

| Request | Result / remaining evidence needed |
| --- | --- |
| 1. Self-downcasts | Audited all self-cast forms; navigator and CRTP ownership supported as above. |
| 2. Secondary bases | Illuminator +0x138/vtables supported; redundant explicit upcasts removed. |
| 3. Decompiler identifiers | Magic label is finish_difficulty_adjustment; srNode's remaining numbered data token is now an address citation. No numbered labels/function/data identifiers remain in authored source. |
| 4. Defined placeholders | semantic-debt-v2 reports unresolved_function_identities, including bodies, deduplicated by target/semantic ID with definition location preferred. Nine-digit legacy spelling and scoped names are recognized. |
| 5. Recycler adapter | Named local stack-to-heap adapter; direct-call xrefs exclude an observed callback role. |
| 6. __stdcall | All five Wizardry helpers retain independently observed stack arguments/RET-N. Static timer and external callback families accounted for. |
| 7. Free/member inverse | No change is justified by adjacency alone. Recycler and octree setter are demonstrated free functions; movement/texture helpers consume stack records. Other free/member candidates require caller ECX and source/export evidence. |
| 8. Packed types | Every explicit pack-scope declaration is listed below with one of the four requested categories. Unexplained runtime records remain open. |
| 9. Scope leakage | Every lexical push/pop balances; no include is inside an active packed scope. No direct pack reset was found. Removing packing still requires stride/allocation evidence. |
| 10. Disk/runtime | NPC scripts intentionally load the same packed allocation then overwrite presence slots with pointers. This is in-place decoding, not evidence for a second runtime allocation. ReadMesh already converts compressed-face records into 0x29 faces; face arrays are also directly read for uncompressed files. Do not widen either runtime allocation speculatively. |
| 11. Debug fill | Both comparisons exist in retail. Name VC6 CRT poisoned-block patterns at compat/debug_heap.h; retain the checks and the existing initialization paths. They are not valid game-domain location IDs/pointers. |
| 12. Class IDs | srClassSupport exposes its template ClassID as CLASS_ID; remove stTextureAnim's repeated numeric definition and replace 13 consumer literals with the owner class identity. |
| 13. Versions | W8OctFileHeader::VERSION owns 0x22 for writer and both reader comparisons. Other format-version candidates need a shared format owner, not a numeric-value-wide replacement. |
| 14. Layout assertions | Move 238 assertions from five large headers to included evidence fragments. All assertions stay compiled in the same owning TUs; no layout/source assertions are dropped. |
| 15. Archaeology | Offset evidence is separated with those assertions; useful member semantics and required FUNCTION/GLOBAL/VTABLE markers remain. Further long comment extraction should preserve the existing evidence, not erase ownership notes. |
| 16. Opaque subrecords | 159 unknown-array declarations form the inspection queue below. Consumer clustering must establish complete subrecord extents before replacing them; anonymous blocks alone do not establish a new record. |
| 17. Overlapping types | Mesh-strip hash now stores W8MeshStripPolygon* instead of int; cached/automap polygon arrays now consistently use unsigned long* through their existing public API. Seven casts removed at owners; delete-vs-delete[] behavior is retained. Low-byte scene state/quad cells remain explicit investigations. |
| 18. Message union | Remove raw; AddMessageBoxLine and the eight-argument SetNpcQuoteBubbleVisible now transport the four-byte union by value. Every producer selects text, argument, skill_notices, experience or level_up_slot; dispatch retains the tag and existing ownership. Flags use argument, eliminating three pointer/int casts. Retail loads/stores one stack dword per payload at 0x00528a80/0x00576060; the five-argument bubble wrapper passes a zero payload at 0x00576030. |
| 19. reinterpret-ok | Classified every annotation below; fix two container families and scheduler void* down-conversion. Raw representation and external contracts remain attributable; unresolved records have concrete owner queues. |
| 20. bool-byte-ok | Audit every exception below. One generic all_clear suppression removed in favor of a real bool; cure-result notes now describe unnormalized byte forwarding/bitwise arithmetic. C gap, serialization and raw-copy cases retained. |
| 21. Const APIs | Remove 63 caller casts in SGP/CRT text families. Existing Wiz8ToSgpWideText owns historical SGP conversion; CRT const format arguments need no cast. FileOpen/Exists/age and button calls retain released mutable SGP ABI; GrCycle lookup and Chunk file API constness remain canonical-owner work. |
| 22. C/C++ ownership | gap.c is C; released SGP remains C; Info-ZIP windll_subset/infozip_adapter are C boundaries. Assertion paths saying .cpp and genuine class methods are C++ evidence. No bulk C-linkage conversion. |
| 23. C linkage | NoOct is referenced by src/sgp/sgp.c:816 and its canonical declaration supplies C linkage. Lifecycle/video hooks similarly have released C callers; mouth-gap hooks belong to gap.c. Preserve decorated SurRender member exports and C plugin entry points. |
| 24. Tiny wrappers | Recycler adapter is a real stack-convention bridge, not a duplicate member. Forwarding getters/import wrappers and different receiver/overload bodies must not be merged solely for textual identity. Ordinary-wrapper candidates remain a call/xref/ICF queue. |
| 25. Empty functions | Existing ScreenLifecycleSuccess/region callbacks have table contracts and folded retention evidence. Do not add an authored empty body for every retained address or alias no-ops across signatures. Further empty candidates require raw slot/caller and folding evidence. |
| 26. Callback typedefs | W8DialogButtonCallback, W8ControlCallback, W8DialogDestroyCallback, W8RegionCallback and W8MasterFunction already centralize distinct contracts. Automap void() erasure remains one API-family question; plugin external prototypes must preserve independent ABI. Monster-cycle/path callback unification remains open until registration and invocation signatures are independently checked. |
| 27. Return domains | Surface loaders already return pointers and IsNavigatorAtTarget returns bool. Active polygon array API now agrees with storage. Byte cure statuses must not become bool. Weak integer-return candidates need all caller domains/production width, not one use. |
| 28. Hidden results | No new result struct inferred merely from several outputs. Animation bounds is two independently filled vectors with a boolean result; conversion to hidden record return needs caller/callee ABI evidence. |
| 29. Constant pools | Navigator already owns TU-local linked-distance/turn/drop constants, while common constants are shared at real addresses. Exposing an FPU load from constant storage does not establish public semantic ownership; relocation/xref review remains per address. |
| 30. Static ownership | Keep local RecyclerAccess/AllocateRecyclerStorage together; no public allocator adapter introduced. Mesh-order helper/record/table family remains in ReadMesh. Existing source-local helper/constants should not be republished solely for comparison pairing. |

## Remaining packed-type questions

Category identifies the current storage role. Serialized records that later hold
runtime pointers are still file-ABI constraints unless allocation/copy evidence
shows a separate representation. A pack(4) SurRender interface is external ABI;
it is not an unexplained pack(1) game state. Runtime “unexplained” entries need
independent array stride, allocation extent, member offsets and natural-alignment
comparison. In particular inspect main-screen dialogue/level state, monster
combat/visibility rows, target source/combat slots and NPC state before removing
any pragma.

| Owner | Declaration | Pack | Category | Evidence / next check |
| --- | --- | --- | --- | --- |
| `include/surrender/srDD.h` | `ClearValues` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srStat.h` | `srStat` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srTriMeshPipeline.h` | `srTriMeshPipeline` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srTriMeshPipeline.h` | `Record` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srTriMeshPipeline.h` | `Pass` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexPipe.h` | `srVertexPipe` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexPipe.h` | `Input` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexPipe.h` | `Record` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexPipe.h` | `ColorSource` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexPipe.h` | `Scratch` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexProcessor.h` | `srVertexProcessor` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/surrender/srVertexProcessor.h` | `MaterialInfo` | 4 | external ABI | SurRender provider/consumer record under pack(4); exported interface and vtable/subobject layout are ABI constraints. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFilePathNode` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileScaledPathNode` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFilePathAI` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileFramePosition` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileCompressedFace` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileMesh` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileLightExtra` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileItemRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileClippingPlaneRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileLight` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileAnimLight` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileMonster` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTriggerPosition` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTriggerHotSpot` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileCamera` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileDoor` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileDoorRef` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileLinkedRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileSwitch` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFilePlane` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileInvisible` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileSound` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileSuperTrigger` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTrigger` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileFrame` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileLODMesh` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileMorph` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTransform` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileBounds` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileAnimObj` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileProp` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelParticleRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileParticleSystem` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileNamedPosition` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileBlock` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFile` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/engine_code/Missile.h` | `W8MissileTableRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/engine_code/Navigator.h` | `W8Navigator` | 4 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/engine_code/OctPath.h` | `W8PathEdge` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/engine_code/OctPreTree.h` | `W8OctFileHeader` | 1 | serialized/file ABI | CreateOctFile/ReadOctFile transfer sizeof(header); version and serialized offsets agree. |
| `include/wiz8/engine_code/ReadMesh.h` | `W8ReadMeshFace` | 1 | serialized/file ABI | ReadSingleLevelMeshBody reads sizeof(face) arrays directly; compressed input has its own decoded record. |
| `include/wiz8/engine_code/materials.h` | `W8MaterialRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/gameplay_modifiers.h` | `W8EffectSlot` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/gameplay_modifiers.h` | `W8GameplayModifierBlock` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/item_spawning.h` | `W8WorldItem` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8Enchantment` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8CharacterAttribute` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8CharacterSkill` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8CharacterResistance` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8HandAttack` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8CharacterConditionRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8Character` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/character.h` | `W8SkillAttributes` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/combat_state.h` | `W8PartySlotRow` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/combat_state.h` | `W8CombatHandRecord` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/combat_state.h` | `W8CombatCharacterRow` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/combat_state.h` | `W8CombatState` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/encounter_tables.h` | `W8EncounterTableDiskHeader` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/encounter_tables.h` | `W8EncounterScriptName` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/game_status.h` | `W8StatusBuffers` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/game_status.h` | `W8ItemSpellUsageRecord` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/game_status.h` | `W8CharacterSpellUsageRecord` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterAttack` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8SpellRuntimeRecord` | 1 | serialized/file ABI | InitializeSpellDatabase reads sizeof(W8SpellRuntimeRecord) directly into the live array after skipping each name prefix; no separate runtime conversion/allocation is observed. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8FactDatabaseRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcItemStockRule` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcCharacterTemplate` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcDatabaseRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8LevelDatabaseRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterTreasureEntry` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterTreasureBlock` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8EncounterCompanionRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/item_instance.h` | `W8ItemInstance` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/item_tables.h` | `W8ItemTableEntry` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/item_tables.h` | `W8ItemTableRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/item_tables.h` | `W8ItemRequirement` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/item_tables.h` | `W8ItemDatabaseRecord` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/levels.h` | `W8LevelFolderRecord` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/levels.h` | `W8LevelProgressRow` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/layouts/main_game_screen.h` | `W8DialogueTextState` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/main_game_screen.h` | `W8LevelRuntimeBlock` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/npc_state.h` | `W8NpcState` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/party_formation.h` | `W8PartyFormationPosition` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/party_formation.h` | `W8PartyFormationState` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/layouts/targeting.h` | `W8TargetSource` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/layouts/targeting.h` | `W8CombatSlot` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/ConditionsAndEnchantments.h` | `W8ConditionImmunity` | 2 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/Configuration.h` | `W8GameSettings` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/local_code/Factions.h` | `W8FactionRuntimeRecord` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/MonsterGroup.h` | `W8MonsterGroup` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterManagerEntry` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterCombatState` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/MonsterManager.h` | `W8VisibilityRecord` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterInfo` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_code/SpellEffect.h` | `W8SpellEffectResult` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `include/wiz8/local_screens/MGSKeyboard.h` | `MGSKeyBinding` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `include/wiz8/monster_generators.h` | `W8EncounterTableRuntime` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/npc_script_file.h` | `W8NpcQuoteSubEntry` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/npc_script_file.h` | `W8NpcQuoteEntry` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/npc_script_file.h` | `W8NpcScriptQuote` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/npc_script_file.h` | `W8NpcScriptFile` | 1 | serialized/file ABI | File/database/save record; reader/writer uses this packed storage. |
| `include/wiz8/xstatus.h` | `W8XStatus` | 1 | unexplained | Packing scope is established; necessity/source ABI is not yet proven. |
| `src/wiz8/engine_code/OctBuildPreTree.cpp` | `W8CubRegionRecord` | 1 | serialized/file ABI | LoadRegionFile transfers exactly 0x6a bytes through ReadFile before decoding corners. |
| `src/wiz8/engine_code/ReadMesh.cpp` | `W8CompressedReadMeshFace` | 1 | serialized/file ABI | Record belongs to the serialized reader/writer family; runtime consumers may reuse its storage. |
| `src/wiz8/local_code/InputMapper.cpp` | `MGSKeyName` | 1 | proven runtime packing | Retail LoadDefaults at 0x0055d91b advances the static name table by six bytes; key load at 0x0055d987 uses index*6 plus +4. |

## Cast exception burn-down

Current classified snapshot: 145 genuine ABI/raw-byte operation, 75 historical compiler/platform issue, 1 project-owned type mismatch, 39 unresolved record.

These are reviewed triage categories, not automatic exemptions. Revisit any
external prototype discrepancy at the provider/consumer boundary. Preserve
known raw/debug behavior; do not hide a project type mismatch under an ABI label.

| Site | Category | Attribution / next action |
| --- | --- | --- |
| `include/surrender/srGERD.h` (annotation 1301) | genuine ABI/raw-byte operation | the hash mixes the stored interface addresses. — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srGERD.h` (annotation 1303) | unresolved record | as above. — Investigate producers, discriminants and storage extent before changing the type. |
| `include/surrender/srHash.h` (annotation 26) | genuine ABI/raw-byte operation | pointer-keyed hashing mixes the pointer's integer value — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srHeap.h` (annotation 38) | genuine ABI/raw-byte operation | raw address alignment is storage the type system — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srHeap.h` (annotation 44) | genuine ABI/raw-byte operation | byte-granular advance past the head fill. — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srPixelConvert.h` (annotation 55) | genuine ABI/raw-byte operation | packed pixel-format block compare — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srPixelConvert.h` (annotation 57) | genuine ABI/raw-byte operation | packed pixel-format block compare — Permanent representation operation; keep explicit extent/format attribution. |
| `include/surrender/srQuadWord.h` (annotation 25) | genuine ABI/raw-byte operation | deliberate bit reinterpretation of the pair as one qword — Permanent representation operation; keep explicit extent/format attribution. |
| `include/wiz8/dialog_code/ButtonUserData.h` (annotation 13) | historical compiler/platform issue | SGP userdata slot carries the pointer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `include/wiz8/dialog_code/ButtonUserData.h` (annotation 19) | historical compiler/platform issue | SGP userdata slot carries the pointer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `include/wiz8/engine_code/Navigator.h` (annotation 196) | unresolved record | candidate array aliases the one target-location slot — Investigate producers, discriminants and storage extent before changing the type. |
| `include/wiz8/engine_code/Octree.h` (annotation 107) | unresolved record | the — Investigate producers, discriminants and storage extent before changing the type. |
| `include/wiz8/engine_code/Octree.h` (annotation 129) | unresolved record | the — Investigate producers, discriminants and storage extent before changing the type. |
| `include/wiz8/engine_code/stModelInstance.h` (annotation 51) | unresolved record | scene purge reads the low byte at +0x170; its relation to highlight alpha remains unresolved — Investigate producers, discriminants and storage extent before changing the type. |
| `include/wiz8/sgp_text.h` (annotation 13) | historical compiler/platform issue | SGP byte-text ABI uses UINT8* for char storage — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `include/wiz8/sgp_text.h` (annotation 19) | historical compiler/platform issue | SGP byte-text ABI uses mutable UINT8* for input text — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `include/wiz8/sgp_text.h` (annotation 25) | historical compiler/platform issue | SGP UINT16* wide-text ABI; Win32 wchar_t is 16-bit — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `include/wiz8/sgp_text.h` (annotation 31) | historical compiler/platform issue | SGP UINT16* wide-text ABI; Win32 wchar_t is 16-bit — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/srext_unzip/plugin.cpp` (annotation 332) | historical compiler/platform issue | Info-ZIP password slot cannot express the adapter argument — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/srext_unzip/plugin.cpp` (annotation 337) | historical compiler/platform issue | Info-ZIP service slot reuses the print body — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/color_surface.cpp` (annotation 472) | genuine ABI/raw-byte operation | the retail setter writes only the pixel word's low bytes — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/color_surface.cpp` (annotation 480) | genuine ABI/raw-byte operation | a two-byte write into the raw pixel word. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/color_surface.cpp` (annotation 844) | genuine ABI/raw-byte operation | packed ARGB dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/color_surface.cpp` (annotation 845) | genuine ABI/raw-byte operation | packed ARGB dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/color_surface.cpp` (annotation 1773) | genuine ABI/raw-byte operation | each 24-bit pixel record swaps its low word plus — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/color_surface.cpp` (annotation 2472) | genuine ABI/raw-byte operation | the 24-bit pixel record copies its low word — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/config.cpp` (annotation 305) | genuine ABI/raw-byte operation | free pool entries thread the next-free pointer — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/config.cpp` (annotation 314) | genuine ABI/raw-byte operation | the free list link lives in the name pointer. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/config.cpp` (annotation 397) | genuine ABI/raw-byte operation | the free list link lives in the name pointer. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/debug_vp.cpp` (annotation 314) | unresolved record | the forwarded pointer arguments are OR-ed together — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/fog.cpp` (annotation 114) | genuine ABI/raw-byte operation | VP dword fill — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/fog.cpp` (annotation 126) | genuine ABI/raw-byte operation | VP dword fill — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/fog.cpp` (annotation 137) | genuine ABI/raw-byte operation | VP dword fill  0x3f800000, count); — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/fog.cpp` (annotation 144) | genuine ABI/raw-byte operation | VP dword fill — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/generic_vp.cpp` (annotation 1930) | genuine ABI/raw-byte operation | sign-magnitude float bits converted to a — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/generic_vp.cpp` (annotation 1940) | genuine ABI/raw-byte operation | linear-ordered key converted back to float bits. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 1393) | genuine ABI/raw-byte operation | the public pick key arrives as ulong bits — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 1759) | genuine ABI/raw-byte operation | 16-bit accumulation pixels are filled as dword lanes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 3681) | unresolved record | retail streams the HWND-valued handle through — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/gerd.cpp` (annotation 3769) | unresolved record | the device stats mirror stores the — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/gerd.cpp` (annotation 4643) | genuine ABI/raw-byte operation | the DD receives the palette entries as raw dwords. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 4724) | genuine ABI/raw-byte operation | the DD receives the palette entries as raw dwords. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5447) | genuine ABI/raw-byte operation | the accum row scratch is raw dword storage reused as — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5457) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5498) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5620) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5641) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5662) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/gerd.cpp` (annotation 5719) | genuine ABI/raw-byte operation | ARGB row buffer through the dword pixel-row ABI — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/heap.cpp` (annotation 68) | genuine ABI/raw-byte operation | block header pointer rounding to the 0x20-aligned — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 181) | genuine ABI/raw-byte operation | pooled chunks carry their 0x20-byte header immediately — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 274) | genuine ABI/raw-byte operation | the carved chunk starts at a byte offset inside the — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 385) | genuine ABI/raw-byte operation | the owning block pointer sits five bytes under the user — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 409) | genuine ABI/raw-byte operation | the unaligned back-pointer slot overlaps the chunk tail; — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 431) | genuine ABI/raw-byte operation | allocation tag byte immediately preceding the user area. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 435) | genuine ABI/raw-byte operation | pooled chunks carry their 0x20-byte header — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 442) | genuine ABI/raw-byte operation | system allocations carry their block pointer five — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 505) | genuine ABI/raw-byte operation | allocation tag byte immediately preceding the user — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 516) | genuine ABI/raw-byte operation | the freed user word stores the next free pointer. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 565) | genuine ABI/raw-byte operation | block alignment is computed on the raw allocation bits. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/heap.cpp` (annotation 592) | genuine ABI/raw-byte operation | the name string is stored right after the user area. — Allocator/free-list storage is representation-dependent; retain allocation extent/lifetime evidence. |
| `src/surrender/huffman.cpp` (annotation 74) | genuine ABI/raw-byte operation | the byte cache is read as an unaligned dword. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/huffman.cpp` (annotation 497) | genuine ABI/raw-byte operation | radix pass extracts byte pass of each key. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/light.cpp` (annotation 356) | genuine ABI/raw-byte operation | manual 32-byte alignment of raw VP scratch storage. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/light.cpp` (annotation 362) | genuine ABI/raw-byte operation | raw aligned scratch reinterpreted as the direction array. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/light.cpp` (annotation 399) | genuine ABI/raw-byte operation | retail pushes the float bit pattern into — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/light.cpp` (annotation 491) | unresolved record | the scalar broadcast fills the v3 array as — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/light.cpp` (annotation 505) | genuine ABI/raw-byte operation | elementwise float subtraction across the v3 array — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/memory_pool.cpp` (annotation 57) | genuine ABI/raw-byte operation | pool alignment arithmetic requires the raw address. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/memory_pool.cpp` (annotation 339) | genuine ABI/raw-byte operation | retail subtracts the raw pointer value, not an offset. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 362) | genuine ABI/raw-byte operation | retail compares the packed color dwords — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 851) | genuine ABI/raw-byte operation | packed color dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 855) | genuine ABI/raw-byte operation | packed color dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 938) | genuine ABI/raw-byte operation | the surface API exchanges packed srARGB rows as dwords — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 948) | genuine ABI/raw-byte operation | packed pixel value as srARGB — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1063) | genuine ABI/raw-byte operation | packed leaf color bytes — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1122) | genuine ABI/raw-byte operation | packed dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1189) | unresolved record | packed — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/palette.cpp` (annotation 1193) | genuine ABI/raw-byte operation | byte-wise bounds — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1197) | genuine ABI/raw-byte operation | byte-wise bounds — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1207) | genuine ABI/raw-byte operation | packed bounds dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1209) | genuine ABI/raw-byte operation | packed bounds dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1268) | genuine ABI/raw-byte operation | byte bounds merge — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1271) | genuine ABI/raw-byte operation | byte bounds merge — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1274) | genuine ABI/raw-byte operation | byte bounds merge — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/palette.cpp` (annotation 1318) | genuine ABI/raw-byte operation | packed color dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 787) | genuine ABI/raw-byte operation | 24-bit source records load their high two — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 840) | genuine ABI/raw-byte operation | quantize reads the packed BGR bytes of the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 852) | genuine ABI/raw-byte operation | quantize reads the packed BGR bytes of the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 864) | genuine ABI/raw-byte operation | quantize reads the packed BGR bytes of the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 879) | genuine ABI/raw-byte operation | quantize reads the packed BGR bytes of the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 899) | genuine ABI/raw-byte operation | palette entries are packed srARGB dwords; only the low — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 1040) | genuine ABI/raw-byte operation | the gray table entry is a packed BGRA pixel read — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 1055) | genuine ABI/raw-byte operation | the gray table entry is a packed BGRA pixel read — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 1067) | genuine ABI/raw-byte operation | 24-bit source records load their high two bytes as — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 1073) | genuine ABI/raw-byte operation | the gray table entry is a packed BGRA pixel read — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 1089) | genuine ABI/raw-byte operation | the gray table entry is a packed BGRA pixel read — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 2343) | genuine ABI/raw-byte operation | the 24-bit pixel record stores its low word plus — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 2374) | genuine ABI/raw-byte operation | the 24-bit pixel record stores its low word plus — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 2568) | genuine ABI/raw-byte operation | the 24-bit pixel record stores its low word plus — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 2604) | genuine ABI/raw-byte operation | the 24-bit pixel record stores its low word plus — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/pixel_convert.cpp` (annotation 2732) | genuine ABI/raw-byte operation | 24-bit source records load their high two bytes as a — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 33) | genuine ABI/raw-byte operation | radix pass extracts byte pass of each key. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 147) | genuine ABI/raw-byte operation | the triples rebase as flat dwords. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 325) | genuine ABI/raw-byte operation | the dword fill is the shader-agnostic byte fill the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 328) | unresolved record | as above. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 335) | genuine ABI/raw-byte operation | 1.0f's bit pattern goes in through the dword — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 339) | unresolved record | as above. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 393) | unresolved record | the second key word rides the pass's value slot. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 421) | genuine ABI/raw-byte operation | the texture table is a dword stream to the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 445) | unresolved record | the stage-0 table entries are the — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 451) | unresolved record | same table form for stage 1. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 480) | genuine ABI/raw-byte operation | the key maps the depth's IEEE sign bit onto a — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 607) | genuine ABI/raw-byte operation | the q stream is a dword stream to the — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 617) | genuine ABI/raw-byte operation | 1.0f's bit pattern fills the q0 — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/renderer.cpp` (annotation 656) | unresolved record | the projection matrix is consumed elementwise. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/surrender/renderer.cpp` (annotation 746) | genuine ABI/raw-byte operation | the packed byte stream is folded through word loads. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/scene.cpp` (annotation 115) | genuine ABI/raw-byte operation | the pick key is the node pointer itself. — Identity/hash/debug token represents an address or class-ID word. |
| `src/surrender/stream.cpp` (annotation 193) | genuine ABI/raw-byte operation | in-place swap of the word's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 214) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 232) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 250) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 268) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 550) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 565) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 578) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 593) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/stream.cpp` (annotation 606) | genuine ABI/raw-byte operation | byteSwap rewrites the object's raw bytes. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/timer.cpp` (annotation 442) | historical compiler/platform issue | Win32 GetProcAddress returns untyped FARPROC; retail calls the — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/timer.cpp` (annotation 448) | historical compiler/platform issue | Win32 FARPROC has no parameter typing — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/triangle_culler.cpp` (annotation 39) | genuine ABI/raw-byte operation | the per-vertex outside test reads the distances' raw — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/triangle_culler.cpp` (annotation 172) | genuine ABI/raw-byte operation | the negative test reads the distances' raw IEEE sign — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/triangle_culler.cpp` (annotation 346) | genuine ABI/raw-byte operation | the caller-provided flag scratch holds one SRBYTE per — Permanent representation operation; keep explicit extent/format attribution. |
| `src/surrender/type_registry.cpp` (annotation 1244) | genuine ABI/raw-byte operation | retail prints the numeric class id through — Identity/hash/debug token represents an address or class-ID word. |
| `src/surrender/window.cpp` (annotation 9) | historical compiler/platform issue | Win32 window handle arrives as a raw ulong across the — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/window.cpp` (annotation 23) | historical compiler/platform issue | Win32 window handle arrives as a raw ulong across the — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/window.cpp` (annotation 36) | historical compiler/platform issue | Win32 window handle arrives as a raw ulong across the — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/surrender/window_out.cpp` (annotation 129) | historical compiler/platform issue | GWL_USERDATA — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 141) | historical compiler/platform issue | GWL_USERDATA — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 181) | historical compiler/platform issue | the EDITSTREAM — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 184) | historical compiler/platform issue | the — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 284) | historical compiler/platform issue | the — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 467) | historical compiler/platform issue | the Win32 window handle arrives as a raw ulong — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/surrender/window_out.cpp` (annotation 472) | historical compiler/platform issue | the Win32 window handle arrives as a raw ulong across — Win32 cookie/thread callback ABI; preserve the documented native signature. |
| `src/wiz8/dialog_code/DialogFactoryDialogs.cpp` (annotation 1608) | unresolved record | retail uses the button address as the fallback coordinate — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/dialog_code/PortraitQuote.cpp` (annotation 623) | genuine ABI/raw-byte operation | raw locked pixel memory — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/dialog_code/PortraitQuote.cpp` (annotation 633) | genuine ABI/raw-byte operation | raw locked pixel memory — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/GDFileIO.cpp` (annotation 935) | historical compiler/platform issue | String returns UINT8* — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/GDFileIO.cpp` (annotation 1353) | historical compiler/platform issue | String returns UINT8* — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/GDFileIO.cpp` (annotation 1362) | historical compiler/platform issue | String returns UINT8* — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/GameData.cpp` (annotation 821) | genuine ABI/raw-byte operation | retail clears the sign bit of the spilled dy float — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/Item.cpp` (annotation 233) | historical compiler/platform issue | SGP String returns unsigned text bytes — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 259) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 856) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 858) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 864) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 905) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 907) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 993) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 996) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 1186) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 1954) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 1957) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 1970) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 1971) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/LevelFile.cpp` (annotation 2107) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/Missile.cpp` (annotation 646) | unresolved record | word buffer is the "%s" sscanf target — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/Missile.cpp` (annotation 666) | unresolved record | ambient word buffer read as text — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/Monster.cpp` (annotation 593) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/Monster.cpp` (annotation 757) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/Monster.cpp` (annotation 1870) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/Monster.cpp` (annotation 4881) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/OctBuildTree.cpp` (annotation 526) | unresolved record | dead kind-10 path reads the proven ushort region-index pair at +0x2c as a link head — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/OctPreTree.cpp` (annotation 1614) | historical compiler/platform issue | String returns UINT8* — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/engine_code/OctSubMesh.cpp` (annotation 361) | unresolved record | unresolved srShader/render-flag table type — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/Octree.cpp` (annotation 1683) | genuine ABI/raw-byte operation | packed colour storage — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/UpdateMesh.cpp` (annotation 349) | unresolved record | quad-cell object elements are never populated in retail; element layout is unresolved — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/UpdateMesh.cpp` (annotation 352) | unresolved record | embedded pair-table at +0x1c resolved by retail offset — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/UpdateMesh.cpp` (annotation 354) | unresolved record | slot index field at +0x30 — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 665) | historical compiler/platform issue | COM QueryInterface returns the interface through void** — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 694) | historical compiler/platform issue | COM QueryInterface returns the interface through void** — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 769) | genuine ABI/raw-byte operation | SurRender takes the window handle as an integer — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 799) | historical compiler/platform issue | FORMAT_MESSAGE_ALLOCATE_BUFFER writes an LPSTR through this argument. — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 812) | historical compiler/platform issue | FORMAT_MESSAGE_ALLOCATE_BUFFER writes an LPSTR through this argument. — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 857) | historical compiler/platform issue | COM QueryInterface returns the interface through void** — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 877) | genuine ABI/raw-byte operation | SurRender takes the window handle as an integer — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 1786) | historical compiler/platform issue | Win32 ClientToScreen takes LPPOINT; RECT is two adjacent POINTs — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 1788) | historical compiler/platform issue | Win32 ClientToScreen takes LPPOINT; RECT is two adjacent POINTs — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/engine_code/Video2.cpp` (annotation 3945) | genuine ABI/raw-byte operation | opaque pick token — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/materials.cpp` (annotation 1006) | genuine ABI/raw-byte operation | status cells cleared through the raw buffer — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/materials.cpp` (annotation 1674) | unresolved record | texture-name text — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/materials.cpp` (annotation 1809) | unresolved record | texture-name text — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/materials.cpp` (annotation 2232) | genuine ABI/raw-byte operation | retail formats the pointer — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/engine_code/stCube.cpp` (annotation 187) | genuine ABI/raw-byte operation | packed colour storage — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stLight.cpp` (annotation 456) | unresolved record | the — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/engine_code/stLight.cpp` (annotation 488) | genuine ABI/raw-byte operation | the 0x80-byte save — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 332) | genuine ABI/raw-byte operation | FillDwordBuffer takes the float bit pattern — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 344) | genuine ABI/raw-byte operation | packed DIG as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 345) | genuine ABI/raw-byte operation | packed lights as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 370) | genuine ABI/raw-byte operation | FillDwordBuffer takes the float bit pattern — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 415) | genuine ABI/raw-byte operation | packed DIG as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 416) | genuine ABI/raw-byte operation | packed lights as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 434) | genuine ABI/raw-byte operation | packed DIG as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stMeshModel.cpp` (annotation 436) | genuine ABI/raw-byte operation | packed DIG as float* — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stModelInstance.cpp` (annotation 832) | genuine ABI/raw-byte operation | dword view of the vec3 scratch buffer. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stModelInstance.cpp` (annotation 843) | genuine ABI/raw-byte operation | float lanes of the vec3 scratch buffer. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/engine_code/stModelInstance.cpp` (annotation 846) | genuine ABI/raw-byte operation | float lanes of the mesh positions. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 220) | unresolved record | retail passes the optional argument slot as context. — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 646) | unresolved record | function entry stored as data — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 843) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1063) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1221) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1351) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1360) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1403) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1467) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1476) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1485) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1494) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1503) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1512) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1521) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1572) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1581) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1590) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/level_specific_code/MasterFunctionList.cpp` (annotation 1599) | historical compiler/platform issue | SGP rotating debug buffer — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_code/Combat Attack.cpp` (annotation 1039) | genuine ABI/raw-byte operation | the assert message slot carries the failing hand index — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/local_code/Combat Attack.cpp` (annotation 1070) | genuine ABI/raw-byte operation | the assert message slot carries the failing hand index — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/local_code/Combat Attack.cpp` (annotation 1092) | genuine ABI/raw-byte operation | the assert message slot carries the failing hand index — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/local_code/Combat.cpp` (annotation 1409) | unresolved record | retail stores the context word into the generic output slots after the FALSE assert — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_code/Combat.cpp` (annotation 1411) | unresolved record | retail stores the context word into the generic output slots after the FALSE assert — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_code/ItemManager.cpp` (annotation 921) | genuine ABI/raw-byte operation | pointer-valued assert message argument. — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/local_code/LoadSaveGame.cpp` (annotation 843) | genuine ABI/raw-byte operation | the 64-byte — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_code/PC Item.cpp` (annotation 2023) | historical compiler/platform issue | String returns a logging buffer — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/local_code/PC Item.cpp` (annotation 3827) | historical compiler/platform issue | SGP's String returns UINT8* and SoundPlay takes char* — Released SGP String returns UINT8*; centralize text conversion at the SGP boundary. |
| `src/wiz8/local_code/Party Import.cpp` (annotation 128) | genuine ABI/raw-byte operation | raw serialized file image; unaligned header short — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_code/Party Import.cpp` (annotation 130) | genuine ABI/raw-byte operation | raw serialized file image; unaligned header short — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_code/Traps.cpp` (annotation 99) | unresolved record | raw low byte of the angle record's trailing slot — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_code/Traps.cpp` (annotation 104) | unresolved record | raw low byte of the angle record's trailing slot — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_code/VideoObjectManager.cpp` (annotation 1098) | historical compiler/platform issue | retail forwards the same 32-bit pitch word to the SGP lock API — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 434) | project-owned type mismatch | void() vs button* — Correct the owning container/API element type and all consumers. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1369) | genuine ABI/raw-byte operation | vertex-light floats zeroed via dword fill. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1384) | genuine ABI/raw-byte operation | vertex-light floats zeroed via dword fill. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1559) | genuine ABI/raw-byte operation | vertex-light floats filled via dword fill. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1561) | genuine ABI/raw-byte operation | fill pattern read as dword. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1596) | genuine ABI/raw-byte operation | vertex-light floats filled via — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/AutomapScreen.cpp` (annotation 1599) | genuine ABI/raw-byte operation | fill pattern as dword. — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 1231) | unresolved record | contiguous pointer-member run the retail loop walks — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 1237) | unresolved record | contiguous pointer-member run the retail loop walks — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 1317) | unresolved record | contiguous pointer-member run the retail loop walks — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 1345) | unresolved record | contiguous pointer-member run the retail loop walks — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 1447) | unresolved record | retail indexes contiguous control* slots from dialogue_text_10c — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 2467) | historical compiler/platform issue | SGP text ABI — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` (annotation 4565) | historical compiler/platform issue | SGP text ABI — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_screens/OptionsScreen.cpp` (annotation 1902) | unresolved record | retail passes the panel's contiguous origin_x/origin_y/right/bottom — Investigate producers, discriminants and storage extent before changing the type. |
| `src/wiz8/local_screens/OptionsScreen.cpp` (annotation 2206) | genuine ABI/raw-byte operation | raw byte view of the wide name buffer — Permanent representation operation; keep explicit extent/format attribution. |
| `src/wiz8/local_screens/RCSItemsPage.cpp` (annotation 1247) | historical compiler/platform issue | SGP frame-buffer bytes to W8TextBuffer's byte view — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_screens/ReviewCharacterScreen.cpp` (annotation 448) | historical compiler/platform issue | SGP's historical UINT16 text ABI stores wchar_t data — Keep conversion at the external compatibility boundary; verify callback ABI. |
| `src/wiz8/local_screens/mipe.cpp` (annotation 737) | genuine ABI/raw-byte operation | retail stores the null group pointer as the count sentinel — Tagged/debug argument word; preserve the established value and discriminant. |
| `src/wiz8/engine_code/AnimObj.cpp` (annotation 730) | historical compiler/platform issue | VC6 debug CRT freed-block poison compared as a pointer, never dereferenced — Keep poisoned-storage recognition at the compatibility boundary. |

## Byte exceptions

| Site | Permanent attribution / unresolved question |
| --- | --- |
| `include/wiz8/engine_code/AnimRep.hpp` | SetSetting6C stores and GetSetting6C returns the caller's byte. */ |
| `include/wiz8/engine_code/AnimRep.hpp` | copied directly from file-backed W8AnimObj byte. */ |
| `include/wiz8/engine_code/Monster.h` | copied raw from the C gap track byte |
| `include/wiz8/engine_code/Monster.h` | automap save/restore (0x0057E660/0x0057FB40) copies it through a byte array unnormalized |
| `include/wiz8/local_code/NPCScripting.h` | staged-byte copy stays unnormalized |
| `include/wiz8/local_screens/PartySelectionScreen.h` | W8ScreenStateHandlers leave slot |
| `include/wiz8/local_screens/Screens.h` | screen-table unsigned char (*)() slot |
| `include/wiz8/local_screens/Screens.h` | screen-table unsigned char (*)() slot |
| `include/wiz8/mouth_gap.h` | C gap.c uses this one-byte state and VC6 C has no bool */ |
| `src/wiz8/local_code/Magic Effects.cpp` | Unnormalized byte-valued cure result: forwarded by RevealItemBindingsToTarget; bitwise intersected by ProcessSpellEffectTargets. |
| `src/wiz8/local_code/Magic Effects.cpp` | Unnormalized byte-valued cure result: forwarded by RevealItemBindingsToTarget; bitwise intersected by ProcessSpellEffectTargets. |
| `src/wiz8/local_code/NPC Scripting.cpp` | stages the raw quote_active byte |
| `src/wiz8/local_code/QuoteManager.cpp` | copied raw from the C gap track byte |
| `src/wiz8/local_code/RegionManager.cpp` | W8RegionCallback result |
| `src/wiz8/local_code/Text_Input.cpp` | JA2 declares fEnabled as BOOLEAN (UINT8) |

## Opaque-block queue

These are declarations, not recovered subrecords. Repeated field names across
different owners are not assumed to share layout. Inspect accesses by canonical
receiver/type before attributing a cluster; retain unknown extents when there are
no consumers.

| Owner file | Enclosing declaration | Opaque block |
| --- | --- | --- |
| `include/surrender/srColorSurfaceIFace.h` | `SurfaceDesc` | `unsigned char unknown_18_[0x04];` |
| `include/surrender/srGERD.h` | `State` | `unsigned char unknown_12c4_[6];` |
| `include/surrender/srGERD.h` | `State` | `unsigned char unknown_13c4_[4];` |
| `include/surrender/srGERD.h` | `ClearState` | `unsigned char unknown_2c_[4];` |
| `include/surrender/srGERD.h` | `EnvironmentState` | `unsigned char unknown_0c_[4];` |
| `include/surrender/srGERD.h` | `EnvironmentState` | `unsigned char unknown_19f4_[4];` |
| `include/surrender/srGERD.h` | `EnvironmentState` | `unsigned char unknown_2045_[3];` |
| `include/surrender/srStat.h` | `srStat` | `unsigned char unknown_04_[4]; /* 0x04: alignment hole, never written */` |
| `include/surrender/srTimer.h` | `` | `unsigned char unknown_004_[0x4];` |
| `include/surrender/srTimer.h` | `` | `unsigned char unknown_82c_[0x4];` |
| `include/surrender/srTriMeshPipeline.h` | `Record` | `unsigned char unknown_2c_[0x30];` |
| `include/wiz8/chunk.h` | `W8ChunkHead` | `unsigned char unknown_06[2];` |
| `include/wiz8/dialog_code/AssayDialog.h` | `W8AssayDialog` | `unsigned char unknown_14d[3];` |
| `include/wiz8/dialog_code/DialogBase.h` | `W8DialogBase` | `unsigned char unknown_038[8];` |
| `include/wiz8/dialog_code/DialogButton.h` | `W8DialogButton` | `unsigned char unknown_03d[3];` |
| `include/wiz8/dialog_code/MessageDialogBase.h` | `W8MessageDialogBase` | `unsigned char unknown_064[0x10];` |
| `include/wiz8/dialog_code/MessageDialogBase.h` | `W8MessageDialogBase` | `unsigned char unknown_07c[0x10];` |
| `include/wiz8/dialog_code/MessageDialogBase.h` | `W8MessageDialogBase` | `unsigned char unknown_096[2];` |
| `include/wiz8/dialog_code/NpcDialog.h` | `W8NpcDialog` | `unsigned char unknown_069[3];` |
| `include/wiz8/engine_code/AniMesh.h` | `W8AniMesh` | `unsigned char unknown_02[2];` |
| `include/wiz8/engine_code/AniMesh.h` | `W8AniMesh` | `unsigned char unknown_29[3];` |
| `include/wiz8/engine_code/GrObject.h` | `W8GrObject` | `unsigned char unknown_005[3];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFilePathAI` | `unsigned char unknown_06[4];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileLight` | `unsigned char unknown_06[2];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileMonster` | `unsigned char unknown_1a[4];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTriggerPosition` | `unsigned char unknown_00[0x1c];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileTriggerHotSpot` | `unsigned char unknown_00[0x85];` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFileSuperTrigger` | `float unknown_4b3[3];             /* version_00 > 1 */` |
| `include/wiz8/engine_code/LevelFile.h` | `W8LevelFile` | `unsigned char unknown_6b9[4];` |
| `include/wiz8/engine_code/Missile.h` | `W8MissileRep` | `unsigned char unknown_0b4[0x24];` |
| `include/wiz8/engine_code/Missile.h` | `W8MissileTableRecord` | `unsigned char unknown_000[0x100];` |
| `include/wiz8/engine_code/Missile.h` | `W8MissileTableRecord` | `unsigned char unknown_148[8];` |
| `include/wiz8/engine_code/Missile.h` | `W8MissileTableRecord` | `unsigned char unknown_169[0x7c];` |
| `include/wiz8/engine_code/SpellEmitterHost.h` | `W8SpellEmitterHost` | `unsigned char unknown_0b4[0x24];` |
| `include/wiz8/engine_code/Trigger.hpp` | `W8TriggerEvent` | `unsigned char unknown_036[2];` |
| `include/wiz8/engine_code/Trigger.hpp` | `W8TriggerActionData` | `unsigned char unknown_005[3];` |
| `include/wiz8/engine_code/Trigger.hpp` | `W8LockState` | `unsigned char unknown_011[3];` |
| `include/wiz8/engine_code/Trigger.hpp` | `Trigger` | `unsigned char unknown_0b5[3];` |
| `include/wiz8/engine_code/Trigger.hpp` | `Trigger` | `unsigned char unknown_0ca[2];` |
| `include/wiz8/engine_code/Trigger.hpp` | `Trigger` | `unsigned char unknown_10d[3];` |
| `include/wiz8/engine_code/Trigger.hpp` | `Trigger` | `unsigned char unknown_351[3];` |
| `include/wiz8/engine_code/Trigger.hpp` | `Trigger` | `unsigned char unknown_365[3];` |
| `include/wiz8/engine_code/stSound3D.h` | `stSound3D` | `unsigned char unknown_14d[3];` |
| `include/wiz8/gameplay_modifiers.h` | `W8GameplayModifierBlock` | `unsigned char unknown_4c[0x1b]; /* 0x4c .. 0x66 */` |
| `include/wiz8/item_spawning.h` | `W8WorldItem` | `unsigned char unknown_21[4];` |
| `include/wiz8/item_spawning.h` | `W8WorldItem` | `unsigned char unknown_3d[0x70];` |
| `include/wiz8/layouts/character.h` | `W8CharacterAttribute` | `unsigned char unknown_0c[8];` |
| `include/wiz8/layouts/character.h` | `W8CharacterSkill` | `unsigned char unknown_14[0x12];` |
| `include/wiz8/layouts/character.h` | `W8CharacterResistance` | `unsigned char unknown_08[8];` |
| `include/wiz8/layouts/character.h` | `W8HandAttack` | `unsigned char unknown_37[2];` |
| `include/wiz8/layouts/character.h` | `W8HandAttack` | `unsigned char unknown_43[0x18];` |
| `include/wiz8/layouts/character.h` | `W8CharacterConditionRecord` | `unsigned char unknown_09[8];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0025[0x44];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_00c9[0x14];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0171[0x28];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_07b3[0x23a];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0a51[0x14];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0ac5[0x3c];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0b3d[8];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0b5d[8];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0ba9[0x10];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_0ed5[4];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_1089[0xc0];` |
| `include/wiz8/layouts/character.h` | `W8Character` | `unsigned char unknown_11ff[0xb6];` |
| `include/wiz8/layouts/combat_state.h` | `W8CombatCharacterRow` | `unsigned char unknown_04[0x30];` |
| `include/wiz8/layouts/game_status.h` | `W8ItemSpellUsageRecord` | `unsigned char unknown_08[8];` |
| `include/wiz8/layouts/game_status.h` | `W8CharacterSpellUsageRecord` | `unsigned char unknown_00[4];` |
| `include/wiz8/layouts/game_status.h` | `W8CharacterSpellUsageRecord` | `unsigned char unknown_0c[4];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_187b[0x55];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_2013[0x294];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_243f[5];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_40c2[0xc];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_47de[0x194];` |
| `include/wiz8/layouts/game_status.h` | `W8GlobalStatus` | `unsigned char unknown_497b[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterAttack` | `unsigned char unknown_1e[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8SpellRuntimeRecord` | `unsigned char unknown_040[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8SpellRuntimeRecord` | `unsigned char unknown_11b[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8SpellRuntimeRecord` | `unsigned char unknown_145[2];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcCharacterTemplate` | `unsigned char unknown_01a[0x44];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcCharacterTemplate` | `unsigned char unknown_1fd[9];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcDatabaseRecord` | `unsigned char unknown_06c[2];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcDatabaseRecord` | `unsigned char unknown_071[3];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcDatabaseRecord` | `unsigned char unknown_2da[0x10];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8NpcDatabaseRecord` | `unsigned char unknown_2ef[0x1a];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8LevelDatabaseRecord` | `unsigned char unknown_000[0x3c];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8LevelDatabaseRecord` | `unsigned char unknown_058[0x80];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterTreasureBlock` | `unsigned char unknown_00[0x30];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | `unsigned char unknown_0de[2];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | `unsigned char unknown_17d[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | `unsigned char unknown_185[2];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | `unsigned char unknown_24b[4];` |
| `include/wiz8/layouts/gameplay_databases.h` | `W8MonsterRecord` | `unsigned char unknown_273[0x24];` |
| `include/wiz8/layouts/item_tables.h` | `W8ItemTableRecord` | `unsigned char unknown_1d1[4];` |
| `include/wiz8/layouts/item_tables.h` | `W8ItemTableRecord` | `unsigned char unknown_1d9[0x18];` |
| `include/wiz8/layouts/item_tables.h` | `W8ItemDatabaseRecord` | `unsigned char unknown_043[3];` |
| `include/wiz8/layouts/item_tables.h` | `W8ItemDatabaseRecord` | `unsigned char unknown_0b5[4]; /* 0x0b5 .. 0x0b8 */` |
| `include/wiz8/layouts/levels.h` | `W8LevelProgressRow` | `unsigned char unknown_11[0x10];` |
| `include/wiz8/layouts/main_game_screen.h` | `W8LevelRuntimeBlock` | `unsigned char unknown_2b8[8];` |
| `include/wiz8/layouts/main_game_screen.h` | `W8LevelRuntimeBlock` | `unsigned char unknown_2e4[4];` |
| `include/wiz8/layouts/party_formation.h` | `W8PartyFormationPosition` | `unsigned char unknown_04[8];` |
| `include/wiz8/layouts/party_formation.h` | `W8PartyFormationState` | `unsigned char unknown_74[0x10];` |
| `include/wiz8/layouts/targeting.h` | `W8TargetSource` | `unsigned char unknown_22[0x12];` |
| `include/wiz8/local_code/Configuration.h` | `W8GameSettings` | `unsigned char unknown_002[0x4];` |
| `include/wiz8/local_code/Configuration.h` | `W8GameSettings` | `unsigned char unknown_02d[0x1];` |
| `include/wiz8/local_code/Configuration.h` | `W8GameSettings` | `unsigned char unknown_051[0x53];` |
| `include/wiz8/local_code/Controls.h` | `Controls` | `unsigned char unknown_35[3];` |
| `include/wiz8/local_code/Factions.h` | `W8FactionRuntimeRecord` | `unsigned char unknown_0b[3];` |
| `include/wiz8/local_code/MonsterGroup.h` | `W8MonsterGroup` | `unsigned char unknown_d4[0x57];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterManagerEntry` | `unsigned char unknown_029[0x4c];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterAction` | `unsigned char unknown_2d[3];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8PartyThreatRecord` | `unsigned char unknown_00[4];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8PartyThreatRecord` | `unsigned char unknown_26[0x0a];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8VisibilityRecord` | `unsigned char unknown_09[2];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8VisibilityRecord` | `unsigned char unknown_29[8];           /* 0x29 */` |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterInfo` | `unsigned char unknown_256[0x30];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterInfo` | `unsigned char unknown_2ed[4];` |
| `include/wiz8/local_code/MonsterManager.h` | `W8MonsterInfo` | `unsigned char unknown_37d[0xa8];` |
| `include/wiz8/local_code/NPCScripting.h` | `W8NpcScriptingState` | `unsigned char unknown_00[0x64];` |
| `include/wiz8/local_code/NPCScripting.h` | `W8NpcScriptingState` | `unsigned char unknown_72[6];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Item` | `short unknown_02[5];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_014[0x10];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_028[0x18];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_130[0x40];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_19a[0x98];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_233[4];` |
| `include/wiz8/local_code/PartyImport.h` | `W8Wiz7Character` | `unsigned char unknown_23c[0xc];` |
| `include/wiz8/local_screens/MGSSpellCasting.h` | `W8SpellCastingView` | `unsigned char unknown_000[0xf8];` |
| `include/wiz8/local_screens/MGSTextBox.h` | `W8MessageStorageRecord` | `unsigned char unknown_1c[8];` |
| `include/wiz8/local_screens/MainGameScreen.h` | `W8LockTumblerPanel` | `unsigned char unknown_75[3];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_002[0xee];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_14c[4];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_18c[4];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_1da[2];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_202[2];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_215[3];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_22a[2];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_235[3];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_23e[2];` |
| `include/wiz8/local_screens/NPCInteractionSubscreen.h` | `W8NpcInteractionState` | `unsigned char unknown_249[3];` |
| `include/wiz8/local_screens/OptionsScreen.h` | `W8OptionsScreen` | `unsigned char unknown_030[8];` |
| `include/wiz8/local_screens/ReviewCharacterScreen.h` | `W8CampScreenState` | `unsigned char unknown_d1c[4];` |
| `include/wiz8/local_screens/ReviewCharacterScreen.h` | `W8CampScreenState` | `unsigned char unknown_d38[7];` |
| `include/wiz8/local_screens/mipe.h` | `W8MipeState` | `unsigned char unknown_11[0x13];` |
| `include/wiz8/npc_script_file.h` | `W8NpcScriptFile` | `unsigned char unknown_00[4];` |
| `include/wiz8/video_object_catalog.h` | `W8VideoObjectSlot` | `unsigned char unknown_06[2];` |
| `include/wiz8/video_object_catalog.h` | `W8VideoFrame` | `unsigned char unknown_35[3];` |
| `include/wiz8/xstatus.h` | `W8XStatus` | `unsigned char unknown_049[4];` |
| `src/surrender/color_surface.cpp` | `` | `unknown_18_[1] = other.unknown_18_[1];` |
| `src/surrender/gerd.cpp` | `srGERD` | `unsigned char unknown_5d_[3];` |
| `src/wiz8/dialog_code/PortraitQuote.cpp` | `W8PortraitQuoteBubble` | `unsigned char unknown_0a[2];` |
| `src/wiz8/dialog_code/PortraitQuote.cpp` | `W8PortraitQuoteBubble` | `unsigned char unknown_16[2];` |
| `src/wiz8/engine_code/Trigger.cpp` | `W8TriggerShakeEvent` | `unsigned char unknown_041[3];` |
| `src/wiz8/local_code/Combat.cpp` | `` | `if (npc != 0 && npc->record->unknown_2ef[0] != 0) {` |
| `src/wiz8/local_code/GameplayCode.cpp` | `` | `attack->unknown_37[0] = 0;` |
| `src/wiz8/local_code/GameplayCode.cpp` | `` | `attack->unknown_37[1] = 0;` |
| `src/wiz8/local_code/LoadSaveGame.cpp` | `W8StatusHeader` | `unsigned char unknown_114[0x200];` |
| `src/wiz8/local_screens/AutomapScreen.cpp` | `W8AutomapState` | `unsigned char unknown_000[0xf4];` |
| `src/wiz8/local_screens/NPCInteractionSubscreen.cpp` | `` | `g_npc_interaction_state->dialogue_npc->record->unknown_2ef[1] != 0) &&` |
| `src/wiz8/local_screens/Screens.cpp` | `` | `g_level_block->unknown_2e4[0] = 0;` |

## Identical small bodies and callbacks

The compiler source index exposes 39 groups of identical small bodies. This is
textual triage, not ICF proof: receiver types, overloads, virtual slots and
constructor lifetimes remain distinct. Representative forwarded operations:

| Shared body | Source identities | Decision |
| --- | --- | --- |
| `reset();` | `srModeler::Triangle::Triangle`, `srModeler::Vertex::Vertex`, `srInlineString::srInlineString` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `open(path);` | `srBinIFStream::srBinIFStream`, `srBinIOFStream::srBinIOFStream`, `srBinOFStream::srBinOFStream` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return srHeap.allocate(size);` | `srRegistry::ClassNode::IDIndex::operator new`, `AllocateRecyclerStorage` | Class operator new and local __stdcall adapter have distinct source/ABI contracts; retain both. |
| `value = stream.getChar(); return stream;` | `operator>>`, `operator>>` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `value = stream.getWord(); return stream;` | `operator>>`, `operator>>` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `value = stream.getDWord(); return stream;` | `operator>>`, `operator>>`, `operator>>` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return sGetClassID();` | `srGERD::getClassID`, `srRuntimeClass::getClassID` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return sGetClassNode();` | `srClass::getClassNode`, `srGERD::getClassNode`, `srRuntimeClass::getClassNode` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `mesh = getTriMesh();` | `srMeshModel::getTriMesh`, `stMeshModel::getTriMesh` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return case_sensitive_24 != 0 ? strcmp(first, second) == 0 : _stricmp(first, second) == 0;` | `srConfig::Index::namesEqual`, `srRegistry::ClassNode::NameIndex::namesEqual` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return pseek(position);` | `srBinIFStream::seek`, `srBinIOFStream::seek`, `srBinOFStream::seek` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return pseek(position, direction);` | `srBinIFStream::seek`, `srBinIOFStream::seek`, `srBinOFStream::seek` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return ptell();` | `srBinIFStream::tell`, `srBinIOFStream::tell`, `srBinOFStream::tell` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return fputc(character, file_08) != -1 ? 0 : 0xffff;` | `srBinIOFStream::vput`, `srBinOFStream::vput` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return fread(destination, 1, size, file_08);` | `srBinIFStream::vread`, `srBinIOFStream::vread` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `return fwrite(source, 1, size, file_08);` | `srBinIOFStream::vwrite`, `srBinOFStream::vwrite` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `Seed(from, to);` | `W8OctreeTrace::W8OctreeTrace`, `W8OctreeTrace::Reseed` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `Clear();` | `MGSKeyboard::~MGSKeyboard`, `stScript::~stScript` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `W8StatInfoDialogBase::DestroyControls();` | `W8AttributeInfoDialog::~W8AttributeInfoDialog`, `W8SecondaryAttributeInfoDialog::~W8SecondaryAttributeInfoDialog`, `W8SkillInfoDialog::~W8SkillInfoDialog`, `W8StatInfoDialogBase::~W8StatInfoDialogBase` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `DestroyAllControls();` | `W8CampStatsControls::~W8CampStatsControls`, `W8PartySelectionCharacterGridPanel::~W8PartySelectionCharacterGridPanel`, `W8PartySelectionCharacterPanel::~W8PartySelectionCharacterPanel`, `W8PartySelectionPartySlotPanel::~W8PartySelectionPartySlotPanel` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `DestroyControls();` | `W8CharacterSummaryDialog::~W8CharacterSummaryDialog`, `W8MessageDialogBase::~W8MessageDialogBase`, `W8NpcDialog::~W8NpcDialog`, `W8SplitAmountDialog::~W8SplitAmountDialog`, `W8SplitItemDialog::~W8SplitItemDialog` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `EnableRegionSet(0); DestroyAllControls();` | `W8LockTumblerPanel::~W8LockTumblerPanel`, `W8MainGameTextPanel::~W8MainGameTextPanel` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `invalidateFrameHandle(frame_handle);` | `stTexture2D::~stTexture2D`, `stTexture2D::invalidate` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `SetPlaneFromThreePoints(plane, first, second, third);` | `BuildPlaneFromPoints`, `BuildTrianglePlane` | Two free-wrapper candidates: inspect raw callers/stack cleanup and TU assertion ownership before collapsing. |
| `EnableRegionSet(0);` | `W8CharacterSkillsPage::Deactivate`, `W8CharacterStatsPage::Deactivate` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `PushButtonSoundScheme(0, 1); W8TextControl::OnLeftButtonDoubleClick(event);` | `W8CampInfoLabel::OnLeftButtonDoubleClick`, `W8HelpTextControl::OnLeftButtonDoubleClick` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `PushButtonSoundScheme(0, 1); W8TextControl::OnLeftButtonDown(event);` | `W8CampInfoLabel::OnLeftButtonDown`, `W8HelpTextControl::OnLeftButtonDown` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `PushButtonSoundScheme(0, 1);` | `W8LockTumbler::OnLeftButtonDown`, `W8CharacterSpellList::OnMouseEnter`, `W8HorizontalRangeThumb::OnMouseEnter`, `W8VerticalRangeThumb::OnMouseEnter` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `PushButtonSoundScheme(0, 1); W8TextControl::OnLeftButtonUp(event);` | `W8CampInfoLabel::OnLeftButtonUp`, `W8HelpTextControl::OnLeftButtonUp` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |
| `bool enabled = m_enabled; m_enabled = 1; W8TextControl::OnRightButtonUp(event); m_enabled = enabled;` | `W8CharacterStatsRecordControl::OnRightButtonUp`, `W8CharacterStatsValue::OnRightButtonUp` | Distinct receiver/overload/lifecycle identities; no alias inferred from body equality. |

Fifteen small return-zero bodies and ten return-one bodies also share text;
these include distinct exported virtual defaults and screen/region callback
contracts. Their folded address retention cannot establish an authored alias.

The source-index __stdcall inventory additionally accounts for Info-ZIP
discardMessage, discardPrintOrService, discardReplace, noteArchive,
DllMessagePrint, DummySound, UzpPassword, Wiz_Init, Wiz_NoPrinting,
Wiz_StatReportCB and srWizUnzipToMemory. These belong to the plugin/C provider
boundary; parameter erasure must be checked there rather than converted to game
members. DllMain and released SGP WinMain/window/input/timer callbacks have
platform-defined conventions and are excluded from the free-game-helper queue.

## Comparison review

The first changed-source comparison covers 5,111 Wizardry functions and 1,666
SurRender functions, with zero analysis failures. The named recycler adapter
compares clean. SurRender's outcome census has no changes against the preceding
saved report. Implicit secondary-base conversion still emits the retail null
check and +0x138 adjustment; illuminator's remaining difference is the existing
getEyeSpaceLocation result/temporary representation.

The Wizardry outcome census crosses the integration of #811; it is not a pure
PR-head/merge-base performance measurement. Thirteen previously clean entries
now differ. Review of their saved diffs and source changes gives the following
classification; none justifies register/stack/source shaping:

| Function / address | Difference and source review |
| --- | --- |
| SetScaledMotion, 0x00421800 | Different temporary load order; identical component destinations. Function source unchanged. |
| TestPropSunBit, 0x0042e400 | Byte result versus high-register concatenation and signedness inference. Function source unchanged; this TU only changes version constants. |
| stModelInstance::setModel, 0x0047f0c0 | srModel* versus int* inference around the same virtual call. Function source unchanged. |
| srClientSupport<srClipPlane,5376>::vClone, 0x004bdf90 | Assignment call versus inlined base/plane copies; same receiver and five member copies. No clone-body change. |
| GetDistanceToPlayer, 0x004c7cb0 | Retail decompiler infers hidden float10 storage and loses the final clamp; recomp exposes the float return. Function source unchanged. Retain as a return-analysis question, not an invented result record. |
| CheckArrayLength, 0x004cfb70 | uint versus undefined4 return spelling only. Function source unchanged. |
| stGroundShadow copy constructor, 0x004d6430 | Receiver type/pointer arithmetic spelling; same field offsets and calls. Function source unchanged. |
| MonsterFleeAction, 0x004eb980 | Low-byte result/undefined upper-register propagation from AimFleeingMonster. Function source unchanged. |
| BeginNpcScriptedScene, 0x00529be0 | ResetLevelDataVectors inlining loads the global pointer again and reorders independent zero stores. Same reset offsets; function source unchanged. |
| BeginSedexusCapture, 0x00529ef0 | Same reset-vector expansion family; function source unchanged. |
| PauseMainGameWorld, 0x0056aa30 | Same reset-vector expansion family; function source unchanged. |
| PleaseWaitScreenEnsureLevelArchive, 0x00591620 | Inlined CRT varargs/virtual call argument inference exposes stack return-address values and an extra virtual argument. Function source unchanged; retains a varargs/callee-prototype analysis question. |
| CampScreenLeave, 0x005a3ee0 | Reordered independent font/palette loads with identical stores. Function source unchanged. |

The typed-message recomp instructions independently confirm that the union is
passed as one dword, without a hidden record pointer: AddMessageBoxLine loads
ESP+0x18/0x1c and stores directly to queue +0x10/+0x1c after its three register
pushes/allocation argument, exactly as retail. The five-argument quote-bubble
wrapper pushes eight words and cleans 0x20 bytes. The eight-argument receiver
loads the payload stack word and writes +0x24c directly; its frame size differs
by four bytes, so absolute ESP displacements differ accordingly. The discriminants and successful-path payload allocation/delete ownership are
unchanged. The comparison exposed one pre-existing failure-path recovery error:
AddMessageBoxLine checked Add's result and deleted the line when growth failed.
Retail 0x00528aec-0x00528af7 restores the old vector pointer and returns without
freeing the newly allocated line. The recovered method now ignores Add's return,
preserving that retail leak; no speculative cleanup is added. Final recomp
0x0056f07e-0x0056f087 has the same pop/restore/return failure path and no free
call.

## Validation

Original VC6 Wizardry and SurRender builds passed. The native source index and
208-file gating lint passed. `wiz8 pr-check` passed every required lane, including
852 tests (one integration test deselected), source/type/cast/linkage/identity
checks and SurRender export checks. `wiz8 report merge-preservation --base
origin/main` passed without marker losses or waivers. The defined-placeholder
regression covers definition/declaration order, nine-digit spelling and target
selection. Both targets' semantic-debt reports now contain zero numbered
Function identities, and authored Wizardry/SurRender source has zero LAB_*,
FUN_* or DAT_* identifiers.

The first comparison covers 5,111 Wizardry and 1,666 SurRender functions, with
zero analysis failures. The final typed-message-family comparison covers 2,142
functions: 985 clean, 1,148 with differences, nine accepted non-emissions and zero
analysis failures. The five-argument quote-bubble wrapper is clean. The full
bubble retains existing formatting/temporary/control-flow representation
differences; its payload stack/store ABI is confirmed above. After its failure-path correction, AddMessageBoxLine (0x00528a80) compares
clean in the final focused run, with zero analysis failures. The original-compiler
rebuild and all PR/preservation gates were refreshed successfully for that fix.

Against the preceding audit snapshot, seven outcomes become clean, including
MonsterFleeAction and all three ResetLevelDataVectors consumers previously
listed. Eleven become differing: five are only explicit __cdecl spelling
(CatchUpCombatActor, DetachMonsterGroup, GetItemSpellPresentation,
SpendCharacterSpellPoints, ResetGameStatus); GetItemUnitWeight only changes a
parameter name; IsLevelCdMissing represents the same zero test as bool instead
of subtracting a char predicate; GetPointDistanceToPlayer represents ST0 as a
10-byte array instead of float10; MGSKeyboard::FindBinding changes int/-1 to
undefined4/0xffffffff; RemoveAllGroupMembers and BeginEndgameSequence expose
undefined upper bits of byte values. These function bodies are unchanged by the
payload batch. No payload-word displacement or record-layout regression was
found. Detailed local artifacts live under build/recovery/model-*,
typed-message-* and message-queue-*; product/evidence files are not tracked.
