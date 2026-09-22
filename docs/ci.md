# Continuous integration

`.github/workflows/ci.yml` keeps cheap repository validation, recovery/Ghidra integration, WIZ8
runtime/comparison work, and the SURRENDER provider build in separate jobs. The final
`Public checks, lints, and tests` job is only an aggregate gate so branch protection can keep one
stable required check while the expensive lanes run independently.

The workflow intentionally uses `pull_request`, not `pull_request_target`, and checkout credentials
are disabled.

## Change-scoped lanes

`.github/scripts/classify-ci-changes.py` classifies the changed paths before any expensive setup.

- documentation-only changes skip the heavy lanes;
- WIZ8/SGP/runtime changes select the WIZ8 lane but not SURRENDER;
- `src/surrender/**` selects SURRENDER but does not launch the WIZ8 runtime suite;
- `include/surrender/**` selects both lanes because the provider headers are also WIZ8 compile-time
  inputs;
- shared CMake/toolchain/config/tooling changes conservatively select both product lanes and the
  analysis lane;
- Ghidra/evidence changes select the analysis lane as appropriate.

Scheduled and manually dispatched runs select every lane. The classifier has focused unit tests so
these dependency boundaries do not silently drift.

## Shared repository checks

The public repository-check lane restores the compiler-backed source-index cache, loads the pinned
VC6 image, then runs:

- PR merge-preservation checks;
- `uv run wiz8 check`;
- `uv run wiz8 lint`.

The reusable analysis cache contains only generated public build state:

- `build/clang`;
- `build/reccmp-source`;
- `build/source-index.json`.

The source-index code still validates its own input digest, so restoring an older cache is only a
starting point: changed translation units are regenerated rather than trusted blindly.

## VC6 image cache

The pinned `wizardry8-msvc600:sp5` image is no longer rebuilt independently in every job. The
`Prepare VC6 analysis image` job restores a content-addressed Actions cache containing
`docker save` output. On a miss it builds the pinned image once, saves the archive, and downstream
jobs load that exact cached image through `.github/actions/load-vc6-image`.

The key includes the Docker/toolchain inputs and the pinned static-library/toolchain configuration,
so changing those inputs produces a new image cache.

## Ghidra and recovery integration

The Ghidra/recovery job is independent of WIZ8 runtime and SURRENDER product testing. It installs the
pinned Ghidra release, runs the VC6 recovery lifecycle self-test, restores the reviewed checkpoint,
and runs `tests/ghidra`.

This lane is selected by recovery/Ghidra/evidence or shared infrastructure changes rather than every
ordinary gameplay/source PR.

## Licensed input storage

The canonical GOG installer remains a release asset in the private `agluszak/wiz8-ci-inputs`
repository under tag `gog-22306`. Same-repository PRs and trusted non-PR runs may use the repository
secrets; fork PRs remain public-only.

Two encrypted Actions caches are used:

1. the canonical installer cache, keyed by the reviewed installer SHA-256 and cache-format version;
2. a prepared-work cache containing the extracted `WIZ8_WORK_DIR`, compressed and encrypted before
   it is placed in Actions cache.

Plaintext licensed files are never stored in Actions cache. The prepared cache is keyed by the
installer hash plus the extraction/variant/static-library inputs that define the prepared tree.
After restoring it, CI still runs `uv run wiz8 prepare` so checkout-local generated files, reccmp
detection, and source-index validation are refreshed normally.

The reviewed installer SHA-256 is:

```text
48e31728ef2615f4e1ee3e34c901feec8ea428a474fc9c7386cc11018311a68d
```

The installer cache-format identifier is `v1-aes256cbc-pbkdf2-200000`. If
`WIZ8_CACHE_KEY` is rotated, bump `GOG_CACHE_VERSION`.

## WIZ8 lane

WIZ8 has its own incremental `build/decomp` cache namespace. The lane:

1. restores/decrypts the licensed inputs and prepared-work cache;
2. refreshes `wiz8 prepare`;
3. builds the WIZ8 comparison product and `WIZ8_RUNTIME_TEST`;
4. starts the runtime suite;
5. while runtime is executing, runs the WIZ8 reccmp status report, vtable/datacmp audits, and the
   licensed comparison tests when applicable;
6. waits for runtime and fails the lane if the runtime or gating comparison work failed.

The runtime command no longer performs another build: product compilation is completed before the
parallel comparison/runtime phase. Runtime diagnostics are uploaded on failure.

## SURRENDER lane

SURRENDER uses a separate runner and a separate incremental product-cache namespace. It restores the
same encrypted licensed corpus, builds only the SURRENDER comparison product, and verifies that the
SURRENDER target is available to reccmp.

A provider implementation-only change therefore does not spend several minutes booting and testing
WIZ8. Conversely, WIZ8 gameplay/source changes do not rebuild SURRENDER unless a genuinely shared
input changed.

## Setup

The licensed lanes expect:

- `WIZ8_INPUTS_TOKEN`: read-only Contents access to `agluszak/wiz8-ci-inputs`;
- `WIZ8_CACHE_KEY`: a high-entropy symmetric key, for example from
  `openssl rand -base64 48`.

The private repository must contain release `gog-22306` with asset
`setup_wizardry_8_2001_12_23_.22306.exe` matching the reviewed SHA-256 above.
