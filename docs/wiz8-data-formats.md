# Wizardry 8 data-format model

This document records only layouts reconciled against the canonical executable and the
local corpus. Unknown fields remain unknown.

## SLF archives

The vendored SGP `LibraryDataBase.c` names the on-disk structures `LIBHEADER` and `DIRENTRY`.
Their declarations establish the field meanings; the canonical `InitializeLibrary` routine at
`0x00412bb0` independently establishes that Wizardry uses those exact sizes and offsets:

- it reads a `0x214`-byte header from offset zero;
- it seeks to `file_count * -0x118` relative to EOF;
- it reads exactly `file_count` directory records of `0x118` bytes;
- it retains records whose low status byte at `+0x108` is zero;
- it copies directory `data_size` and `data_offset` into compact `0x0c` live entries.

The local `Data/Data.slf` is 149,080,821 bytes and contains 3,314 directory entries.
Its directory begins at `0x08d4a245`. `Data/Monsters/Monsters.slf` is 127,440,105
bytes and contains 3,612 entries, with its directory beginning at `0x07892649`.
These values are observations, not identities: mtimes and corpus paths are not part of
the format.

The tracked declarations are in `config/types/wiz8/slf.h` with the SGP names canonical and the old
`W8SlfHeader`/`W8SlfDirectoryEntry` names retained only as compatibility aliases. The parser in
`tools/wiz8decomp/binary/slf.py` reads only the header and EOF directory; it does not
extract payloads.

Six packed `W8SlfConfiguration` records begin at `0x006000c8`. They correspond to the six paths
embedded by Wizardry's missing `WizLibs.c`. The initialized game allocates six
`LibraryHeaderStruct` records of `0x28` bytes and stores their pointer at
`0x006eb724`. The optional mapping fields at `+0x20` and `+0x24` are populated by
`CreateFileMappingA` and `MapViewOfFile`; these are Wizardry's extension of the released SGP
structure's `0x20`-byte common prefix.

The game wraps physical and archived files behind one integer handle API:

- `OpenVirtualFile` at `0x00404c80` selects a Win32 file or an SLF member;
- `CloseVirtualFile` at `0x00404e10` releases the corresponding slot;
- `ReadVirtualFile` at `0x00404ea0` dispatches to `ReadFile` or an archive read;
- `SeekVirtualFile` at `0x00405030` dispatches physical or archive seeks.

The seek-origin values are Wizardry-specific (`1` begin, `2` end, `4` current), not
the Win32 constants passed internally. They are declared in
`config/types/wiz8/virtual_file.h`.

## Level and waypoint files

`Levels\LEVELS.SLF` contains 4,956 entries. The structural corpus is 35 `.WPT`, 34
`.OCT`, 34 `.PVL`, 34 `.STS`, 26 optional `.RLK`, and two sky `.LVL` files; the other
entries are level-local assets. `evidence/reviewed/wiz8/formats/level-formats.csv` records this
inventory without copying any payload.

The waypoint format is now fully bounded by `OctPath::ReadWaypointFile` at `0x00459650`
and `OctPath::WriteWaypointFile` at `0x00459540`. A `0x10`-byte header contains the file
version, an unknown dword, and waypoint/link counts. All 35
canonical files use version two. Both counts include an all-zero index-zero sentinel. The
header is followed by `waypoint_count` records of `0x10` bytes and `link_count` records of
`0x0e` bytes:

```text
waypoint:  uint16 flags, uint16 first_link, float position[3]
link:      uint32 flags, uint16 source, uint16 destination,
           float distance, uint16 next_link
```

Version-one link records omit `source`; the loader reads the other fields individually and
inserts zero. Several archived version-two files have bytes beyond the computed record end.
The canonical loader closes the file immediately after the declared link array, so those
tails are retained archive/editor residue rather than part of the runtime format.

`ReadOctFile` at `0x0042bc10` is identified directly by its diagnostics. Its current section
inventory includes the header, pre-regions, octree nodes and leaves, leaf grid, polygon and
region lists, triggers, submeshes, mesh/prop/particle link and lookup tables, alpha bits, and
prop-sun bits. Those sections remain separately untyped; the name is an anchor for the next
map-format pass rather than a claim that the large `.OCT` layout is complete.

## Cosmic Forge override blobs

Fan-patch module `cfagent1.28.dll` exposes fifteen `.cfdat` filenames and English
destination addresses. They are valuable table-ownership evidence, but the unused size
arguments still must not be treated as `sizeof` values.

The loader at `0x10003dd0` receives a size-like fourth argument at every call site but
never reads it. Instead it accepts any file up to `0x1000` bytes and passes the file's
actual size to `WriteProcessMemory`. Canonical consumers now resolve most of the layouts.
The race dialog proves that `racesattrs.cfdat` is eleven playable-race records of seven
32-bit attribute minimums, or `0x134` bytes rather than the call site's `0x1c0`. The
profession counterpart is fifteen records of the same `0x1c` shape.

The largest correction is `classesskills.cfdat`. `IsCharacterSkillAvailable` indexes it
as a skill-major matrix of 41 skills by fifteen professions, making the real extent
`0x99c`; the `0x7f8` argument covers only 34 skill rows and omits the seven expert skills.
Adjacent consumers independently establish fifteen profession hit-point floats, three
ability IDs, one bonus skill, four professional skills, one caster-level offset, a byte
spellbook mask, and six starting item IDs. Race tables contain sixteen five-ability
records and sixteen resistance profiles with six adjustment pairs. The special Faerie
equipment blob is a replacement six-item row selected when race ID five is active.

`classesexpgroup.cfdat` remains rejected because its English seed destination
`0x004ef1e0` lies in executable code. `evidence/reviewed/wiz8/formats/cfdat-overrides.csv` keeps both
the patch argument and the independently proven canonical extent so the disagreement is
not erased.

## Gameplay databases

Several payloads have fixed record boundaries that are independently established by
the archive bytes and their canonical loaders:

| File | Header | Count | Disk stride | Runtime stride |
| --- | ---: | ---: | ---: | ---: |
| `Items.dbs` | `0x04` | 819 | `0x10d` | `0x10d` |
| `Monsters.dbs` | `0x04` | 595 | `0x297` | `0x297` |
| `Levels.dbs` | `0x04` | 60 | `0xd8` | `0xd8` |
| `Fact.dbs` | `0x04` | 807 | `0x1d8` | `0x1d8` |
| `SpellTables.dbs` | `0x08` | 150 | `0x2c0` | `0x1bf` |

`InitializeItemDatabase` at `0x0054a400` and `InitializeLevelDatabase` at
`0x0054ae20` load their complete fixed-size arrays. `LoadMonsterDatabaseRecord` at
`0x0054a8a0` seeks directly to `4 + index * 0x297`, reads one record, and removes
suffixes beginning with `#` from four adjacent `0x30`-byte UTF-16 name fields.
`MonsterDBFromSpecies` at `0x004e57c0` lazily allocates and caches those records.
The adjacent `MonsterManager.cpp` lookup cluster establishes a separate runtime
`W8MonsterInfo`, whose bounded layout and size assertion live in the recovered header rather
than here. Keeping these consumers in their original translation unit also reproduces the cache
lookup being inlined into `GetMonsterDataByLocationID`.
The adjacent consumer at `0x004e5990` establishes a float at monster-record offset `+0x1ba`; it
multiplies that value by the global float at `0x005ed4f0`. Its designer-facing meaning remains
unresolved, so both the source and reviewed model retain the positional `float_1ba` name.
The exact body at `0x004e5b50` also establishes the byte string at `+0x189` as a case-insensitive
`GrCycle` lookup key and selects one of the still-semantic-unknown integers at `+0x253` and `+0x257`.
Those integers remain positionally named until a canonical consumer establishes their meaning.
`GetMonsterCombatValue` at `0x004e6780` establishes unsigned values at `+0x181` and `+0x26b`: a
nonzero `+0x26b` overrides the `+0x181` base. The descriptive combat-value name comes from its two
canonical consumers. One contributes the result to a live-HP-weighted combat-strength total; the
other formats it in `MonsterInfoDialog`. This does not claim an original function or designer-field
name.

`InitializeSpellDatabase` at `0x004acc10` reads a count and version, allocates
`count * 0x1bf`, then skips `0x101` bytes before each runtime read. The ignored prefix
is retained in the disk declaration but is not represented in the runtime type.

The retained spell body begins with a 64-byte database name. Four membership bytes at
`+0x048`, `+0x05a`, `+0x11f`, and `+0x120` select the Alchemy, Wizardry, Divinity, and
Psionics books; this mapping is established by the canonical character-skill mask and by
spells shared between books. The cost per power level is the unaligned integer at `+0x049`,
followed by `W8Dice effect_dice` at `+0x051` and the level at `+0x056`.

The resource basename at `+0x05b` is consumed by the spell visual/MLS creation paths. A
64-code-unit UTF-16 display name begins at `+0x09b`. The realm at `+0x133` has the complete
six-value Fire, Water, Air, Earth, Mental, and Divine domain and directly indexes a
character's six spell-point pools. The float at `+0x121` is the effect radius or range, and
`GetSpellTargetType` at `0x004ac9d0` returns the 32-bit targeting field at `+0x137`.
`GetMinimumCasterLevelForSpell` at `0x004acba0` maps the record's spell level to the
canonical minimum caster-level sequence `1, 3, 5, 8, 11, 14, 18`.
the final `0x74` bytes at `+0x14b` form the sound basename used below
`Data\\Spells\\Sounds`. The target-type values and the remaining effect-dispatch fields
stay opaque pending their enum recovery.

The 60 level records carry more than their leading UTF-16 display names. MonGen clamps the
runtime random-encounter limit between `minimum_random_encounters` at `+0x40` and
`maximum_random_encounters` at `+0x3c`. A separate encounter budget is bounded by `+0x48`
and `+0x44`; `UpdateRandomEncounterBudget` at `0x0048c810` replenishes it using elapsed time
divided by the period at `+0x4c`. The developer menu independently identifies the runtime
value initialized from `+0x50` as the encounter culling time in seconds, and
`CullExpiredEncounters` at `0x0048c8e0` enforces it. Every record's `level_id` at `+0x58`
equals its zero-based array index; the remaining `0x7c` bytes from `+0x5c` are zero across
the reviewed GOG corpus.

`Fact.dbs` is a definition table rather than the mutable quest state. Each of its 807
`0x1d8`-byte records contains a 32-bit identifier, a 256-byte `FACT_*` symbolic name, and a
106-code-unit UTF-16 designer annotation. Only 77 annotations are nonempty in the reviewed
corpus; two retain characters after an earlier terminator, so the complete fixed buffer is
preserved rather than normalized into a single host string.

`InitializeFactDatabase` at `0x0054ad00` loads those records into `g_fact_records`. The
mutable state is a separate 1,000-byte `g_fact_values` array at `0x00689b78`; both
`SaveFactState` and `LoadFactState` persist exactly those 1,000 bytes. `InitializeFactState`
zeros the same extent before installing mode-dependent defaults. `EvaluateFact` at
`0x005080f0` returns the stored byte for ordinary IDs but computes special facts from live
party, NPC, faction, and world state, including recursive fact checks. `SetFact` stores a byte
and, unless suppressed by its third argument, dispatches `HandleFactChange` to perform the
associated trigger, NPC, faction, and location side effects. The 807 definition count and
1,000-byte state capacity are therefore intentionally distinct.

`NPC.dbs` is a mixed fixed-and-variable database rather than another fixed-stride table.
Its leading count is followed by 146 packed `0x309`-byte version-two records. Each record
then owns a 32-bit rule count and that many six-byte `W8NpcFactRule` values. The rule is a
32-bit fact ID followed by a 16-bit predicate/operator field. All five available builds have
the same 774 rules and parse exactly to byte 118,674; no archive bytes are left unexplained.

`InitializeNpcDatabase` at `0x0054aac0` performs that two-part load and constructs a runtime
rule container at record offset `+0x2ca`; `DestroyNpcDatabase` at `0x0054ac90` tears those
containers down before freeing the fixed array. Every record's dword at `+0x58` equals its
zero-based index. `CreateNpcRuntimeNode` at `0x00509aa0` binds one indexed record to a
`0x13d`-byte runtime node, initializes inventory when `+0x55` is nonzero, and allocates a
full `0x1862`-byte RPC character state when `+0x57` is nonzero. Packed display, entity, and
script aliases are retained as fixed buffers; their proprietary contents are not tracked.

`EncounterTables.dbs` is another columnar variable database. The canonical file begins with
35 fixed 256-byte name slots, followed by 72 table records. Each table has a packed `0x108`
header and parallel arrays of 16-bit monster species IDs, rarity-class bytes, time-condition
bytes, challenge-level bytes, and fixed 64-byte script names. Version-two records finish with
one extra flags byte. The four retail/patched trees contain 1,259 entries; the demo retains the
same table structure with two additional entries. Both layouts parse exactly to EOF.

`SelectEncounterCandidates` at `0x0048b9a0` establishes the selector meanings. Rarity classes
are the four exact values `3`, `7`, `20`, and `70`; challenge levels range from 1 through 50
and are compared with the party's effective level. Time condition zero accepts daytime
(05:00 through 22:00), one accepts nighttime, and two accepts either. The chosen entry's
16-bit ID is passed directly to `MonsterDBFromSpecies`, proving species ownership. The table
name, script-name payloads, and remaining header dword stay structurally typed but unnamed.

`ItemTables.dbs` contains a separate item-generation model shared unchanged by all five
builds. A count of 15 fixed 256-byte category labels is followed by 114 packed `0x1f1`-byte
tables. Every table has a 256-byte lookup name, a 32-bit category ID, forty five-byte
`W8ItemTableEntry` slots, a level-scaling byte, and 36 bytes whose meaning remains under
review. The category IDs are all valid indices into the leading label array, and all active
item IDs are valid indices into the 819-record item database.

`FindItemTableByName` at `0x004f88a0` performs the case-insensitive table lookup.
`GenerateItemsFromTable` at `0x004f88f0` ignores slots whose leading signed field is zero,
sums the one-byte weights, selects without replacement, and materializes each selected item
through `ReplaceOrCreateItem`. When `level_scaled` is nonzero it filters candidates against
the current party level and the selected item record. The exact designer meaning of the
leading nonzero selector remains unnamed because this consumer only tests it as enabled.

The source-owned `Local Code\ItemManager.cpp` reconstruction now uses the packed definitions in
`include/wiz8/item_tables.h`. Its support bodies are byte-exact: `0x004ef420` averages the level
field at `W8Character +0x89` across occupied party slots, and `0x004addf0` grows the local
vtable-`0x005ec0e0` candidate-index vector. The 951-byte generator itself is structurally strong:
all observed filtering, fallback, weighted retry, removal and item-materialization branches are
present, while its current VC6 allocation and loop shape remain a reviewed near miss.

The item and spell description files share one indexed string-database format consumed by
`GetStringFromStringDatabase` at `0x0052ff80`. A packed five-byte header stores version one
and an encoding flag. Each variable record begins with two 32-bit metadata values and a
32-bit UTF-16 code-unit count. The file ends with one 32-bit absolute offset per record,
followed by the record count and a zero dword. The reader finds the offset table relative to
EOF, so record text does not need a fixed stride. It rejects strings longer than 2,000 code
units and optionally transforms each code unit when the header flag is nonzero.

The reviewed `ItemDesc.dbs` has 1,000 records; `SpellDesc.dbs` and `SpellEffect.dbs` each
have 160. The latter currently stores empty text payloads in every slot. All available files
use the unencoded form. Demo item/spell descriptions have different byte lengths but retain
the same counts and valid footer indexes. Only sizes and structural metadata are tracked;
the description text itself is not exported.

## Save-game container

The main save is a nested tagged container, not a single raw memory image. `SaveGame` at
`0x005123f0` creates the slot and writes FourCC sections; `LoadGame` at `0x00512920`
enumerates and dispatches them. The reviewed vocabulary currently contains 28 exact tags in
`evidence/reviewed/wiz8/formats/save-game-sections.csv`. Top-level sections include version, game
status, screenshot, text, NPC, fact, journal, and optional state. Each `LVLS` record owns
nested level sections for status, monsters, items, encounter state, locks, triggers, ambient
state, particles, and lights. Tags are stored as little-endian 32-bit integers, so the C enum
retains the numeric constants used by the switch statements.

The nested `STAT` payload begins with a fixed `0x314`-byte `W8SaveStatusHeader`.
`SaveStatusHeader` at `0x00513260` zeroes the complete record, writes version `2.0`, four
32-bit state values, and a `0x100`-byte global-status block. `LoadStatusHeader` at
`0x00513090` reads exactly `0x314` bytes, rejects a different version, restores those fields,
and substitutes one for any zero among the four state values. The remaining `0x200` bytes
are reserved and zero in newly written records. Later variable-size monster, item, and
character records remain separate serializers and are not folded into this header.

The remaining fields are intentionally opaque in
`config/types/wiz8/gameplay_databases.h`. Their offsets will be named only after
reconciling canonical field accesses. `evidence/reviewed/wiz8/formats/database-records.csv`
preserves the corpus arithmetic and consumer evidence.

The item model has progressed beyond its record boundary. `FormatItemDisplayName` at
`0x0051b5c0` establishes the generic-name index at `+0x03f` and selects the leading
UTF-16 display name when an item is identified. `ReplaceOrCreateItem` establishes the
quantity-kind byte at `+0x066` and its adjacent four-byte `W8Dice` at `+0x067`.
`RollDice` at `0x00517970` confirms that layout as a signed base, count, and sides.

`MergeItems` at `0x0051f2f0` scans each item record for two order-independent ingredient
IDs at `+0x0b9` and `+0x0bd`. A skill byte of `0xff` at `+0x0c9` bypasses the skill
requirement; otherwise the function checks the minimum at `+0x0ca` and awards skill
progress. These names come from behavior in the canonical function, not the payload's
human-readable item names.

Runtime item instances are `0x0c` bytes. `ReplaceOrCreateItem` at `0x0051c020`
initializes the item ID at `+0x00`, stack quantity at `+0x04`, uses or charges at
`+0x05`, identified state at `+0x06`, and an optional flag at `+0x0b`. The party's
500-entry item pool begins at `0x00685191`; its active count is `0x00686901`. The
separate item-in-hand instance begins at `0x006874cb`, preceded by its validity byte.

The monster record now has a caller-grounded encounter and faction slice. `MonGen::GenerateEncounter`
at `0x0048ad20` rolls `W8Dice group_size` at `+0x0c1`, then considers exactly two
three-byte companion records at `+0x0c5`; each contains a signed species ID and a spawn chance.
The same function rejects the `deleted` byte at `+0x267`. Every one of the 595 records stores
its own zero-based database index at `+0x187`.

`MonsterGroupCalcDefaultDisposition` at `0x00511250` establishes the unaligned hostility
range at `+0x25b` and faction ID at `+0x25f`; the adjacent group-update path uses a positive
range as its party-proximity threshold. The faction constants are recovered from the
contiguous 21-name table beginning with `UNALIGNED`, and `GetFactionDisposition` at
`0x00535ad0` independently asserts that same `0..20` domain. It maps ratings below 34 to
hostile, ratings through 66 to neutral, and higher ratings to friendly. The byte at `+0x0d2`
is deliberately named only as a disposition-cache factor: the canonical group update squares
it and scales the result before comparing timestamps, but its original designer-facing unit is
not yet known.
