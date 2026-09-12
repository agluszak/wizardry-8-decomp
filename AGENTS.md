# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation.

## Evidence and ownership

- Retail instructions/call sites and accepted original-source oracles outrank inferred Ghidra types,
  generated output, comparison scores, and workflow documentation. Correct errors at their owner;
  keep unresolved facts unknown.
- Ghidra owns live analysis: signatures/parameter storage, symbols, references, types/fields, vtables,
  comments, decompiler state, and the original binary translation-unit layout. Git/C++ owns recovered
  source, declarations, current source placement, matching annotations, compiler/build configuration,
  and provenance claims; current placement is checked against the Ghidra TU layout, not derived from
  it. Provenance explains accepted identities and authority limits; do not duplicate either model into
  another database. Generated `build/` projections are disposable.
- Search source and accepted oracles before declaring or implementing. One entity has one canonical
  owner and one evidence-backed type. Cross-TU functions/globals are declared in the owning header;
  callers include it. No local `.cpp` externs, except actual C/OS/vendor interfaces without an
  existing project or dependency header.
- Type disagreement is a source-model defect, not a cast-site problem. Trace producers, consumers,
  callers, callees, loads, and stores; correct the canonical declaration and Ghidra model. Do not use
  casts, integer/pointer substitution, duplicate declarations, or wrapper types to conceal disagreement.
- Do not invent wrappers, aliases, opaque replacement types, raw vtable calls, or parallel inventories.
  Repository-owned Wizardry and SurRender code is unconditional C++; `extern "C"` requires proven
  C linkage. Do not add C fallback APIs.
- Never commit binaries, extracted trees, live Ghidra projects, or build products. Only reviewed GZF
  checkpoints listed in `vendor/ghidra/exports/manifest.json` may be tracked. Preserve source licences
  and required notices.

## Task skills

`.agents/skills` is the canonical shared tree; integrations consume it, never separate copies.
Load the applicable skill and only the references needed for the question:

- [matching-decomp](.agents/skills/matching-decomp/SKILL.md): source recovery, native Ghidra,
  comparison, source oracles, and type investigation.
- [class-triage](.agents/skills/class-triage/SKILL.md): new class boundaries, inheritance/subobjects,
  or deciding whether lifecycle/vtable families are distinct authored classes; not ordinary methods.
- [runtime-bringup](.agents/skills/runtime-bringup/SKILL.md): runtime behavior, UI/input, persistence,
  loading, and startup.

Recovery tooling is agent-only. Use existing primitives and native APIs; do not add query protocols,
wrapper layers, report frameworks, inventories, or human/JSON modes. Filter before printing; put large
listings in named `build/` files. Operational recipes belong in skills, not README duplicates.

## Source fidelity

- Faithfulness is mandatory; exact byte identity is incremental. Recover plausible authored
  circa-2000 C++ and VC6 ABI, not compiler lowering. Never invent, omit, stub, or approximate retail code.
- Establish behavior, then name it: an entity with understood behavior gets a descriptive,
  evidence-backed name in the same change. `FunctionXXXXXX`, `FUN_...`, and `unknown_...` are
  placeholders, not identities. Rename the definition, its declaration, every call site, and any
  ownership/handoff record together; do not leave a body address-named after describing what it does.
  Where the original spelling is unknown, use a behavior-descriptive name, not a made-up original one.
- Preserve counted `for` loops instead of reproducing guarded `do`/`while` lowering. Do not add
  redundant counters, artificial scopes, duplicate cleanup, return temporaries, or rearranged
  expressions merely to change registers, CFG, or score.
- A vtable, lifecycle body, address, or template emission alone does not prove an authored class.
  Compare canonical bases/templates first; generic definitions belong in canonical headers.
- Scalar/vector deleting destructors are compiler-generated MSVC glue: marker-only `SYNTHETIC`
  identities, never handwritten bodies, hidden flags parameters, or destruct-and-maybe-free helpers.
- Preserve TU ownership/order in `src/wiz8/sources.cmake`; keep address-qualified template emissions
  separate until ownership is proved. Recover placement before optimizer control: ordinary functions
  stay unannotated; header bodies need cross-TU visibility evidence; inline controls need call-site
  evidence plus improvement of the complete ABI bundle.
- Legitimate `reinterpret_cast` sites express storage the type system cannot: external ABI, raw
  serialized/pixel memory, tagged storage, deliberate address/bit reinterpretation, or an explicitly
  unresolved site. New casts require a same-line `reinterpret-ok: <reason>` comment (`uv run wiz8 check`
  enforces this). A marker does not justify concealing known type disagreement.

## Scope and completion

Complete the requested coherent task. Do not turn focused recovery into a repository-wide cleanup
merely because the pattern exists elsewhere. Expand only when a shared owner/ABI/layout requires it,
the source model would otherwise become inconsistent, or the task explicitly requests an audit.
Fix blockers at their existing owner with the smallest reliable change; keep exploratory scripts disposable.

Stop when the requested functions, ABI bundle, or behavior meet acceptance criteria. Exact/effective
bodies need no independent rediscovery. If no evidence-backed correction remains, retain faithful
source and report the unresolved mismatch or missing evidence; do not relabel uncertainty as success.

## Runtime linking

- Never add handwritten fake implementations to make a runtime link succeed.
- An unrecovered retail function may be represented by a build-generated
  `// STUB:` trap in the runnable products only. `STUB` is not source recovery;
  `FUNCTION` means a body has actually been recovered.
- `uv run wiz8 build runtime`/`runtime-test` derives the stub set from the real
  unresolved symbols before the runnable link; stub generation is an internal
  build phase, not a public command.
- If stubgen reports an unresolved identity that maps to an already recovered
  address, fix the declaration/linkage/signature mismatch; do not suppress the
  diagnostic.
- `Wiz8.exe` may keep `/FORCE:UNRESOLVED` for matching. `Wiz8Runtime.exe` and
  `Wiz8RuntimeTest.exe` must link without it.

## Verification

Use the smallest existing check capable of detecting the relevant failure. Validate coherent changes,
not each textual edit. Reuse a successful result until a relevant input changes.

- Normal recovered function: focused `uv run wiz8 compare ADDRESS...` (builds itself).
- Layout/vtable/lifecycle/ABI: affected comparison bundle plus relevant declaration/ABI and
  `uv run wiz8 vtable CLASS` checks.
- Behavior/runtime: relevant runtime scenario and the requested observable behavior.
- Python/tooling: relevant existing tests (`uv run pytest -q PATH`) and lint/type checks; shared build
  or validation machinery needs checks appropriate to its reach.
- Prose/skill-only changes: inspect the diff.

`uv run wiz8 check` is the fast public lane (ruff, pyright, validators, tests); `uv run wiz8 lint` is
the clang-cl compile lane. Do not repeat unrelated baseline failures or rerun checks after
descriptions, change IDs, bookmarks, or pushes alone. When a focused check reports diagnostics,
fix them at their owner even if they pre-existed the current change. Do not skip a gating
failure as "baseline" or "not introduced here." If a diagnostic appears only in one
environment, identify why that lane disagrees with the other and fix the lane (path prefixes,
compile-database roots, header filters) rather than suppressing the finding.

Lint and tidy configuration must match first-party trees by repository-relative path. Do not
hard-code the docker `/repo` mount into `.clang-tidy` or compile-database consumers that also
run in a local checkout.

Format manually owned C/C++ files you changed with
`uv run clang-format --style=file -i <paths>` and check with
`uv run clang-format --style=file --dry-run --Werror --fail-on-incomplete-format <paths>`
before completion. Do not reformat imported/vendor source such as `src/sgp`. Formatting alone
does not require another build or comparison run. Use `// clang-format off` / `on` only around
genuinely formatter-hostile declarations, tables, or macros.

Do not add tests by default. Add them only when requested or for a concrete observed correctness bug
existing checks miss, with independently justified expectations. Test behavior, not source spelling,
documentation, inventory counts, generated snapshots, deleted files, or internal implementation details.
Do not add Python tests inspecting recovered source. Remove obsolete tests/helpers with their machinery;
do not create frameworks merely to support tests.

## Workspace and publication

- Use Jujutsu here; preserve unrelated work. Never create a worktree, workspace, clone, sibling, or
  baseline checkout unless explicitly requested. Existing additional checkouts need unique absolute
  `WIZ8_WORK_DIR` values; never share, copy, or hardlink a live Ghidra project.
- Resume the task's mutable change; keep one per coherent task and unfinished work off `main`.
  Bookmarks are optional until publication. Only one agent may switch, rebase, or publish a shared checkout.
- Use `main@origin` as the upstream base; fetch/rebase when upstream work is needed or immediately
  before authorized integration. Use an assigned Bead; update records only for coordination or durable
  findings, never as a task prerequisite.
- Publish once within authorization. Move `main` only for completed authorized direct integration;
  never rewrite remote `main` or discard others' changes. Successful push completes publication;
  investigate actual rejections without routine post-push proofs. Commands:
  [contributor workflow](docs/contributor-workflow.md).

## Cursor Cloud environment layout

The prebuilt Cursor Cloud Agent environment already contains every external tool and the licensed
inputs, at fixed absolute paths. These paths apply only to that provisioned VM; a local checkout
still follows the README (`.env` + `config/local-inputs.yml`). Do not re-download or re-search for
these; use them directly.

- Machine config is regenerated on every boot by the environment `start` script, so `.env` and
  `config/local-inputs.yml` (both gitignored) are already present in the checkout. `.env` pins
  `GHIDRA_INSTALL_DIR=/home/ubuntu/ghidra/ghidra_12.1.2_PUBLIC`, `WIZ8_INPUT_DIR=/home/ubuntu/wiz8-inputs`,
  `WIZ8_WORK_DIR=/home/ubuntu/wiz8-work`, `DOCKER_BUILDKIT=0`, and
  `JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64`.
- Python: `uv` is at `~/.local/bin` (ensure it is on `PATH`). Sync with `uv sync --frozen`; run the
  CLI as `uv run wiz8 …`.
- Licensed retail input: `$WIZ8_INPUT_DIR/setup_wizardry_8_2001_12_23_(22306).exe` (the `gog-media`
  role). The materialized canonical matching target is `$WIZ8_WORK_DIR/variants/gog-base`
  (`Wiz8.exe`, `sr.dll`, `Dll/`); pinned public source dependencies are under
  `$WIZ8_WORK_DIR/fid/sources/unpacked`. Regenerate with `uv run wiz8 prepare` (or, for the
  canonical target only, extract the `gog-media` role and materialize the `gog-base` variant).
- VC6 matching compiler: Docker image `wizardry8-msvc600:sp5` (Debian trixie, clang/LLVM 19).
  The Docker daemon is started per boot by `start` using the `fuse-overlayfs` storage driver; build
  images with the legacy builder (`DOCKER_BUILDKIT=0`). Rebuild the image with
  `uv run wiz8 toolchain build vc6-sp5`.
- Ghidra: `12.1.2 PUBLIC` at `$GHIDRA_INSTALL_DIR`, driven through PyGhidra by the CLI (for example
  `uv run wiz8 ghidra import`); the JDK is at `$JAVA_HOME`.
- Host tooling: `7z`, `innoextract`, `cabextract`, `unshield`, `git-lfs`, `llvm-undname`, and host
  `wine` (used by reccmp's cvdump and by `just run`) are all on `PATH`.
- Quick check: `uv run wiz8 doctor` validates every path and tool above.
