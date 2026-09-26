# Retail bugs

Defects in the shipped Wizardry 8 executable (GOG build) that the recovered
source reproduces on purpose. The source-fidelity rules in `CLAUDE.md` require
keeping a retail bug once retail instructions establish it: the recovered body
must behave like retail, not like what the author probably meant. Each entry
names the function, the defect, and the evidence. The source carries a comment
at the site.

Add an entry when a recovery confirms a retail defect from the instructions.
Behavior that only looks odd, or an unmatched recompiled body, is not an entry.

## Logic and data bugs

| Function | Address | Defect | Evidence |
| --- | --- | --- | --- |
| `CountItemOnCharacter` | `0x005211A0` | A matching backpack slot adds the stack count of the *equipped* slot with the same index, not its own. | The backpack loop compares `[edx]` (`backpack[slot].iItemNo`, from character offset `0x1029`) but reads the count from `[edx - 0xC8]`, which is offset `0xF61 + 12*slot`: `EquippedItem[slot].stack_count`. |
| `GetFact` / `SetFact` | `0x00506280` / `0x005061A0` | The range check is `fact_id > 1000`, so id 1000 passes. It reads and writes one past the 1000-entry `g_fact_values`, into `g_npc_name_buffer[0]`. | The bound constant is 1000 and `g_fact_values` ends exactly at `g_npc_name_buffer`. |
| `TrimAndLowercaseString` | `0x00497940` | The trailing-space check looks at `text[length]`, the terminator, so trailing spaces are never trimmed. | The first trailing test is `cmp byte ptr [text + length], 0x20` (`0x0049796E`), the terminator, so the trim loop never starts. |
| `W8Octree::UpdateVisibility`, `W8Octree::UpdatePathVisualization` | `0x004304A0`, `0x00434170` | The cursor position guard tests `y` twice and never tests `x`. | Both bodies compare the same `y` component twice against the same constant. |
| Octree segment hit test | `0x004353F0` | The reported hit offset is the last colliding candidate's, not the nearest one's. | Documented at the recovered body (`Octree.cpp`); not re-verified against the instructions for this list. |
| `PathNodeObstructed` / `BuildPathLists` | — | The support and block appends write the slot before the `> 29` assertion runs, so a 30th entry overruns the array. | The store precedes the assertion in both appends. |
| `MonGen::SetEncounterTable` | `0x0048CC50` | Sets the HARASSMENT bit (bit 5) for a harassment table but never clears it when a later table is not a harassment table. | The body's only flag write is `or dword ptr [esi], 0x20` (`0x0048CC8B`). |
| NPC dialogue notice region callback | `0x0056F1D0` | Left-button release and double-click also raise the right-button-held flag. | Documented at the recovered body (`NPCInteractionSubscreen.cpp`); not re-verified against the instructions for this list. |

## Memory and resource bugs

| Function | Address | Defect | Evidence |
| --- | --- | --- | --- |
| `CreateMessageBox` | `0x00518510` | The 16-byte image-name buffer receives full paths such as `Data\Message Box\Ok.sti`, and `strcpy` overruns into the saved-register area. | The frame reserves 16 bytes for the name. Retail never reads the clobbered slots again, so the overrun is harmless in practice. |
| `FormatCharacterQuoteText` | `0x0052D0B0` | Stores zero at `buffer[wcslen(buffer) - 1]` without checking the reader result, so an empty string writes `buffer[-1]`. | There is no test of the read result or of the length before the store. |
| `OctPreTree::WriteOctFile` | `0x004683F0` | Every write-failure return skips `FileClose`, leaking the handle. | Verified at `0x004686B4` and the following error paths. |
| `OctPreTree::SplitMeshes` | `0x00469670` | The allocation-failure paths leak the five sort arrays. | The failure returns do not free them. |

## Uninitialized reads

Several loaders read locals that a short-circuited `FileRead` chain left
unassigned. The source keeps these reads and does not add initializers; the
lint lane relaxes `-Wsometimes-uninitialized` for them per file, each marked
`uninit-ok`. Affected sites include:

- `ReadWorldEnvironment` (`0x004BC9D0`)
- `ReadMesh` material and group readers
- `Trigger` loaders
- the spell database loader
- `AnimObj` frame reads
- `stListBox` track edges
- `FireMissileSourceToTarget` (`0x00544630`), when the target monster is missing

## Retail behavior the recovery cannot reproduce exactly

In these cases retail's result depends on compiler-owned stack contents, and
this build's stack holds something different. The source substitutes the value
retail observably produced, or the behavior it clearly intended, and says so at
the site. They are known departures, not fixes.

| Function | Address | Retail behavior | Recovery |
| --- | --- | --- | --- |
| `ReadWorldLights` | `0x004BBAD0` | When no light loads an AI path, `path_success` is returned uninitialized. The stack residue is nonzero in practice, so level loading continues. | Initializes `path_success = true`. With the recompiled stack's residue the level load failed, which broke every level in the runtime tests. |
| `Trigger::RunDestination` | `0x00440DD0` | On the named-entity path `location_id` and `entrance` are uninitialized and read the stack slot holding `this`, so a level transition to a nonsense id is requested. | Uses the current level, a same-level move. |
| `stModelInstance` render walk | — | The `FLAG_TERMINATE` child pointer is never stored; its slot overlaps dead locals, so the walk effectively never fired. | Seeds the pointer to zero; the uninitialized read faults under this build's layout. |
