#!/usr/bin/env bash
set -euo pipefail

target="$1"
program="$2"
build_target="$3"
base_sha="$4"
head_sha="$5"
prefix="$6"

head_summary="$RUNNER_TEMP/$prefix-head-summary.json"
base_summary="$RUNNER_TEMP/$prefix-base-summary.json"
base_ghidriff="$RUNNER_TEMP/$prefix-base-ghidriff.json"
base_direct_calls="$RUNNER_TEMP/$prefix-base-direct-calls.json"
base_datacmp="$RUNNER_TEMP/$prefix-base-datacmp.json"

restore_head() {
  git checkout --detach "$head_sha" >/dev/null 2>&1 || true
}
trap restore_head EXIT

merge_base="$(git merge-base "$head_sha" "$base_sha" || true)"
if [[ -z "$merge_base" ]]; then
  for _ in 1 2 3 4 5; do
    git fetch --no-tags --deepen=256 origin "$head_sha" "$base_sha"
    merge_base="$(git merge-base "$head_sha" "$base_sha" || true)"
    [[ -n "$merge_base" ]] && break
  done
fi
if [[ -z "$merge_base" ]]; then
  echo "could not resolve PR merge base" >&2
  exit 1
fi

# Freeze the head comparison implementation before adopting the base source.
# Both products use the head's pinned reccmp/Ghidriff and report policy;
# build configuration stays base-owned.
comparison_tools="$(mktemp -d "$RUNNER_TEMP/$prefix-comparison-tools.XXXXXX")"
comparison_python="$PWD/.venv/bin/python"
git archive "$head_sha" tools/wiz8decomp | tar -x -C "$comparison_tools"

git checkout --detach "$merge_base"
uv run --no-sync wiz8 prepare --comparison-target "$target"
uv run --no-sync wiz8 build "$build_target"
uv run --no-sync wiz8 analyze source-index

if [[ -s "$head_summary" ]]; then
  mapfile -t addresses < <(jq -r '.functions[].orig' "$head_summary")
  if (( ${#addresses[@]} > 0 )); then
    # Use the same source/PDB classification on both sides. Numeric CLI
    # selectors deliberately skip it, and raw reccmp summaries hide this debt.
    PYTHONPATH="$comparison_tools/tools" "$comparison_python" - "$target" "$head_summary" "$base_summary" <<'PYTHON'
import json
import sys
from pathlib import Path
from wiz8decomp.comparison import compare_selected
from wiz8decomp.config import load_settings
settings = load_settings(repository=Path.cwd())
target, head, output = sys.argv[1:]
addresses = [int(row["orig"], 16) for row in json.loads(Path(head).read_text())["functions"]]
result = compare_selected(
    settings.repo_dir, target, addresses, settings.ghidra_install_dir,
    classify_source_non_emissions=True, classify_template_emissions=True,
)
Path(output).write_text(json.dumps({"target": target, "requested": len(addresses), **result}))
PYTHON
    latest="build/reports/compare/${target,,}/latest"
    cp "$latest/json/$target.ghidriff.json" "$base_ghidriff"
    if [[ -f "$latest/direct-calls.json" ]]; then
      cp "$latest/direct-calls.json" "$base_direct_calls"
    fi
  fi
fi

if [[ "$target" == "SURRENDER" ]]; then
  uv run --no-sync python - "$RUNNER_TEMP/$prefix-base-exports.json" <<'PYTHON'
import json
import sys
from pathlib import Path
from wiz8decomp.config import load_settings
from wiz8decomp.surrender_exports import validate_built_surrender_exports
settings = load_settings()
report = validate_built_surrender_exports(settings.repo_dir, settings.product_build_dir / "sr.dll")
Path(sys.argv[1]).write_text(json.dumps(report))
PYTHON
fi

datacmp_status=0
uv run --no-sync wiz8 datacmp --program "$program" > "$base_datacmp" || datacmp_status=$?
if (( datacmp_status != 0 )); then
  echo "::notice::merge-base datacmp reports differences"
fi
restore_head
trap - EXIT
