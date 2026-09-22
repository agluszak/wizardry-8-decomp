# Function ID corpus

Project-owned FID databases are reproducibly built from the pinned toolchain and
library definitions in `config/static-libraries.yml`. Ghidra's packed database
bytes contain internal identity, so the adjacent JSON manifests are the
reproducibility surface rather than byte-for-byte `.fidb` equality.

Proprietary compiler files and extracted COFF members stay under
`WIZ8_WORK_DIR`; only reviewed configuration, manifests, and exported FID
databases are tracked.

## Rebuild

```sh
uv run wiz8 toolchain build
uv run wiz8 ghidra fid build-seeds
uv run wiz8 ghidra fid extract-libraries
uv run wiz8 ghidra fid build
uv run wiz8 ghidra fid match --program wiz8--gog-base--wiz8--18a74ff61c65
```

The hidden `ghidra fid` commands are corpus-maintenance primitives, not the
ordinary recovery interface. Seed provenance determines name authority:
source-built objects carry source-backed names, original precompiled archives
carry ABI-backed names, and a database with no recorded seed provenance does
not acquire authority merely by matching.

Library/toolchain identities and recipes belong to
`config/static-libraries.yml`; accepted source boundaries belong to the
library/target documents and reviewed evidence. Do not copy their current match
counts into this page.

## Rejected false friend: `Wiz8.exe` `0x004146E0`

FID offers IJG's `jzero_far` for `0x004146E0`, and the small body is
byte-identical. The surrounding ownership evidence rejects that transfer: its
callers are Sir-Tech/SGP code, the main executable has no neighboring IJG
corpus, and JPEG importing is provided through the separate plug-in boundary.

This address is therefore explicitly excluded from FID-based library ownership
in `wiz8decomp.source_oracle`. A generic identical helper body is not enough to
move source ownership across components.

The MSVC static archives are support-code oracles; their presence in the FID
corpus does not imply that the product linked the static CRT.
