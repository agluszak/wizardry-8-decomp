# Continuous integration

`.github/workflows/ci.yml` runs public checks on every pull request and push to `main`. On trusted
`main` runs, the same already-prepared runner then continues into the licensed Wizardry 8 runtime
suite. Same-repository pull requests authored by the repository owner also use the licensed input to
build reccmp products and publish the current recovery statistics as a PR comment.

## Public checks

Every pull request and push to `main` uses only public or tracked repository inputs. It:

- installs Python 3.13, Java 21, Wine/Xvfb, and the pinned Ghidra 12.1.2 PUBLIC release, verifying
  Ghidra's official SHA-256 before use;
- builds the pinned VC6 analysis image and fetches the pinned public zlib/JPEG/Info-ZIP sources;
- runs `uv run wiz8 doctor`, `uv run wiz8 check`, and `uv run wiz8 lint`;
- runs the VC6/Ghidra recovery lifecycle self-test;
- restores the reviewed tracked Ghidra checkpoint through the normal live PyGhidra project-opening
  path.

The public steps never receive the GOG URL or game files. The workflow intentionally uses
`pull_request`, not `pull_request_target`.

The public zlib/JPEG/Info-ZIP trees are also product build inputs: the VC6 build mounts them directly.
Keeping licensed work on the same runner means those already verified sources and the already built
VC6 image are reused instead of downloaded and built again.

## Licensed-input steps

On trusted pushes to `main`, the same job continues by downloading the canonical GOG installer from
the repository secret `WIZ8_GOG_URL`, verifying the reviewed installer SHA-256, running the normal
`wiz8 prepare` path, verifying the resulting corpus, running the licensed comparison tests, and then
running `uv run wiz8 runtime-test --check-order`.

A same-repository pull request authored by the repository owner also downloads and verifies the
installer and prepares the corpus. It then runs `uv run wiz8 report status --build`, formats the
existing structured reccmp statistics into Markdown, and exports that Markdown as a job output. A
separate comment-only job creates or updates one `reccmp status` comment on the pull request. The
comment reports project totals and a collapsible per-target table; subsequent CI runs update the same
comment instead of adding another one. PRs do not run the licensed runtime suite.

The owner-and-same-repository guard is deliberate. reccmp needs the original licensed binaries, so a
PR that computes live matching statistics necessarily has access to those files while it runs. Fork
PRs, Dependabot PRs, and same-repository PRs authored by anyone other than the repository owner stay on
the public-only path and never receive `WIZ8_GOG_URL` or the game files. Do not replace this with
`pull_request_target`: that would give privileged workflow context to untrusted pull-request code.

The main CI job retains only `contents: read`. The `pull-requests: write` permission exists only on the
small `comment-reccmp-status` job, which is skipped before dispatch unless the PR is an owner-authored
same-repository PR and the main CI job succeeded. That job does not check out or execute repository
code; it only receives the rendered Markdown and upserts the PR comment. Checkout credentials on the
main job remain disabled.

The download uses pinned `gdown==5.2.1` through `uvx`. For Google Drive, it passes `--fuzzy` so an
ordinary share URL such as `https://drive.google.com/file/d/.../view?usp=sharing` is resolved to the
actual file instead of downloading the HTML share page. `gdown` handles Drive's confirmation flow for
large files. It runs quietly so the secret URL is not printed. A normal direct HTTP/HTTPS URL also
works.

The installer is simpler than storing a pre-extracted tree: it is one known input with one known hash,
and the normal project code remains responsible for extraction and materialization. CI does not upload
the installer, extracted tree, work directory, or runtime stages to Actions artifacts or caches. The
licensed files are removed in the final step and the GitHub-hosted runner is disposable.

## Setup

1. Put `setup_wizardry_8_2001_12_23_(22306).exe` somewhere CI can fetch. For Google Drive, sharing it
   as **Anyone with the link** is sufficient for this setup. The expected SHA-256 is
   `48e31728ef2615f4e1ee3e34c901feec8ea428a474fc9c7386cc11018311a68d`.
2. In GitHub, open **Settings -> Secrets and variables -> Actions -> New repository secret**.
3. Create `WIZ8_GOG_URL` containing the Google Drive share URL or other download URL. Do not put the
   URL in workflow YAML, repository variables, issues, or comments.

Create the secret before the first trusted `main` run or owner PR that needs a stats comment. Licensed
CI intentionally fails with a clear error when it is missing rather than silently skipping the work.

An "anyone with the link" Drive file uses bearer-link security: the repository and Actions logs do not
publish the link, but anyone who obtains it can download the installer. The SHA-256 verification is the
hard integrity gate before any project tooling consumes the downloaded bytes.

If stronger access control is eventually needed, replace the single URL with provider-specific
credentials or OIDC. That is deliberately not the default here because it adds substantially more CI
configuration for little benefit on a single-user project.

Do not expose game files through Actions artifacts, caches, container images, or a public package
registry.
