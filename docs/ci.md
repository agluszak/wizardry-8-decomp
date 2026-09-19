# Continuous integration

`.github/workflows/ci.yml` runs public checks on every pull request and push to `main`. CI is
deliberately limited to checks that can run from repository-owned or publicly downloadable inputs;
it does not download or consume licensed Wizardry 8 game files.

## Public checks

Every pull request and push to `main`:

- installs Python 3.13, Java 21, Wine/Xvfb, and the pinned Ghidra 12.1.3 PUBLIC release, verifying
  Ghidra's official SHA-256 before use;
- builds the pinned VC6 analysis image and makes the public zlib/JPEG/Info-ZIP inputs available to the
  build tooling;
- runs `uv run wiz8 doctor`, `uv run wiz8 check`, and `uv run wiz8 lint`;
- runs the VC6/Ghidra recovery lifecycle self-test;
- restores the reviewed tracked Ghidra checkpoint through the normal live PyGhidra project-opening
  path, then runs `tests/ghidra` live enrichment integration tests (excluded from `wiz8 check`),
  including the lifecycle-fixture class-binding acceptance test after the recovery self-test.

The workflow intentionally uses `pull_request`, not `pull_request_target`, and checkout credentials
remain disabled.

## Licensed-input checks

Checks that need the original Wizardry 8 binaries are not run in GitHub Actions. In particular, CI
does not download the GOG installer and does not run the licensed preparation, comparison, matching,
vtable/datacmp, licensed-test, reccmp-status, or runtime-test lanes.

Those checks remain available in a local or otherwise explicitly provisioned environment after the
licensed input has been configured and prepared. Typical commands are:

```sh
uv run wiz8 prepare
uv run wiz8 build match
uv run wiz8 report status
uv run wiz8 vtable
uv run wiz8 datacmp
uv run pytest -q tests/licensed
uv run wiz8 runtime-test --build --check-order
```

This only changes CI scheduling; it does not remove the licensed tests, runtime harness, or comparison
tooling from the repository.
