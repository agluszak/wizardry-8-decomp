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
base_status="$RUNNER_TEMP/$prefix-base-status.json"
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

git checkout --detach "$merge_base"
uv run --no-sync wiz8 build "$build_target"
uv run --no-sync wiz8 analyze source-index

if [[ -s "$head_summary" ]]; then
  mapfile -t addresses < <(jq -r '.functions[].orig' "$head_summary")
  if (( ${#addresses[@]} > 0 )); then
    compare_status=0
    uv run --no-sync wiz8 compare "${addresses[@]}" --program "$program" || compare_status=$?
    latest="build/reports/compare/${target,,}/latest"
    if [[ ! -f "$latest/summary.json" ]]; then
      echo "merge-base comparison produced no summary" >&2
      exit "$compare_status"
    fi
    cp "$latest/summary.json" "$base_summary"
    cp "$latest/json/$target.ghidriff.json" "$base_ghidriff"
    if (( compare_status != 0 )); then
      echo "::notice::merge-base comparison has unpaired/incomplete selected functions"
    fi
  fi
fi

uv run --no-sync wiz8 report status > "$base_status"
datacmp_status=0
uv run --no-sync wiz8 datacmp --program "$program" > "$base_datacmp" || datacmp_status=$?
if (( datacmp_status != 0 )); then
  echo "::notice::merge-base datacmp reports differences"
fi
restore_head
trap - EXIT
