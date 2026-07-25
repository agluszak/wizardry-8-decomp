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
