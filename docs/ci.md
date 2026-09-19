# Continuous integration

`.github/workflows/ci.yml` runs public checks on every pull request and push to `main`. Trusted
`main` runs and same-repository pull requests authored by the repository owner can additionally run
the licensed Wizardry 8 comparison lane.

## Public checks

Every pull request and push to `main` uses only public or tracked repository inputs. It:

- installs Python 3.13, Java 21, Wine/Xvfb, and the pinned Ghidra 12.1.3 PUBLIC release, verifying
  Ghidra's official SHA-256 before use;
- builds the pinned VC6 analysis image and fetches the pinned public zlib/JPEG/Info-ZIP sources;
- runs `uv run wiz8 doctor`, `uv run wiz8 check`, and `uv run wiz8 lint`;
- runs the VC6/Ghidra recovery lifecycle self-test;
- restores the reviewed tracked Ghidra checkpoint and runs the live Ghidra integration tests.

The workflow intentionally uses `pull_request`, not `pull_request_target`. Checkout credentials are
disabled.

## Licensed input storage

The canonical GOG installer is stored as a release asset in the private
`agluszak/wiz8-ci-inputs` repository under tag `gog-22306`. CI accesses that repository only on a
cache miss, using the `WIZ8_INPUTS_TOKEN` repository secret. The token only needs read access to the
private input repository.

The public repository's Actions cache contains only an encrypted copy of the installer. The cache key
includes the reviewed installer SHA-256 and an explicit encryption/cache-format version. The
encryption key is held separately in the `WIZ8_CACHE_KEY` repository secret and is never stored in
the cache.

This split is deliberate: pull-request workflows can potentially read caches derived from the base
branch, so plaintext licensed input must never be cached. A fork can at worst obtain the encrypted
blob; repository secrets are not supplied to that workflow.

On a trusted licensed run CI:

1. restores the encrypted cache;
2. on a cache miss, downloads the release asset from the private repository;
3. verifies the plaintext installer SHA-256;
4. encrypts it with AES-256-CBC using PBKDF2 and a random high-entropy repository secret;
5. saves that encrypted blob to Actions cache only from trusted non-PR runs on `main`;
6. decrypts the installer into the ephemeral runner input directory;
7. verifies the plaintext SHA-256 again before `wiz8 prepare`.

The reviewed installer SHA-256 is:

```text
48e31728ef2615f4e1ee3e34c901feec8ea428a474fc9c7386cc11018311a68d
```

The current cache-format identifier is `v1-aes256cbc-pbkdf2-200000`. If
`WIZ8_CACHE_KEY` is rotated, bump `GOG_CACHE_VERSION` so CI cannot restore ciphertext encrypted
with the old key.

## Licensed-input checks

After materializing the installer, CI uses the normal project path:

- `uv run wiz8 prepare`;
- `uv run wiz8 build match`;
- reccmp status generation for trusted owner-authored same-repository PRs;
- vtable and datacmp audits;
- `tests/licensed` on trusted `main` runs;
- the full runtime suite on trusted `main` runs.

Fork PRs, Dependabot PRs, and same-repository PRs authored by anyone other than the repository owner
stay on the public-only path. They receive neither `WIZ8_INPUTS_TOKEN` nor `WIZ8_CACHE_KEY`.

The installer, extracted game tree, runtime stages, and decrypted material are removed at the end of
the job. Only the encrypted installer blob is persisted through Actions cache.

## Setup

The licensed lane expects these two repository secrets:

- `WIZ8_INPUTS_TOKEN`: a fine-grained GitHub token with read-only Contents access to
  `agluszak/wiz8-ci-inputs`;
- `WIZ8_CACHE_KEY`: a random high-entropy encryption key, for example generated with
  `openssl rand -base64 48`.

The private repository must contain release `gog-22306` with asset
`setup_wizardry_8_2001_12_23_.22306.exe` matching the reviewed SHA-256 above. CI renames that release
asset to the canonical local filename `setup_wizardry_8_2001_12_23_(22306).exe` before verification
and preparation.
