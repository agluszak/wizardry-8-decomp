# Wizardry 8 data-format model

This document records only layouts reconciled against the canonical executable and the
local corpus. Unknown fields remain unknown.

## SLF archives

The canonical `OpenSlfArchive` routine at `0x00412bb0` establishes the container
layout directly:

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

The tracked declarations are in `config/types/wiz8/slf.h`. The parser in
`tools/wiz8decomp/binary/slf.py` reads only the header and EOF directory; it does not
extract payloads.

Six packed `W8SlfConfiguration` records begin at `0x006000c8`. The initialized game
allocates six `W8SlfArchiveState` records of `0x28` bytes and stores their pointer at
`0x006eb724`. The optional mapping fields at `+0x20` and `+0x24` are populated by
`CreateFileMappingA` and `MapViewOfFile`.

The game wraps physical and archived files behind one integer handle API:

- `OpenVirtualFile` at `0x00404c80` selects a Win32 file or an SLF member;
- `CloseVirtualFile` at `0x00404e10` releases the corresponding slot;
- `ReadVirtualFile` at `0x00404ea0` dispatches to `ReadFile` or an archive read;
- `SeekVirtualFile` at `0x00405030` dispatches physical or archive seeks.

The seek-origin values are Wizardry-specific (`1` begin, `2` end, `4` current), not
the Win32 constants passed internally. They are declared in
`config/types/wiz8/virtual_file.h`.

## Cosmic Forge override blobs

Fan-patch module `cfagent1.28.dll` exposes fifteen `.cfdat` filenames and English
destination addresses. They are valuable evidence about table ownership, but they do
not yet establish record layouts.

The loader at `0x10003dd0` receives a size-like fourth argument at every call site but
never reads it. Instead it accepts any file up to `0x1000` bytes and passes the file's
actual size to `WriteProcessMemory`. Consequently, constants such as `0x1c0` for
`racesattrs.cfdat` are not safe `sizeof` evidence. Indeed, writing `0x1c0` bytes at the
seed destination `0x00614cf0` would overlap the `classesattrs.cfdat` destination at
`0x00614e24`. `classesexpgroup.cfdat` is more clearly inconsistent: its English seed
destination `0x004ef1e0` lies in executable code.

`config/analysis/wiz8/cfdat-overrides.csv` therefore preserves these call-site facts
and conflicts without installing guessed arrays in Ghidra. Record counts and fields
must come from the canonical readers and writers, not from these unused arguments.

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
`GetMonsterDataByID` at `0x004e57c0` lazily allocates and caches those records.

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

The remaining fields are intentionally opaque in
`config/types/wiz8/gameplay_databases.h`. Their offsets will be named only after
reconciling canonical field accesses. `config/analysis/wiz8/database-records.csv`
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
