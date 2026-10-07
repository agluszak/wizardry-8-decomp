"""Build baseline source in the CI checkout and compare with frozen head tooling."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from collections.abc import Callable
from pathlib import Path


def _reports(
    repository: Path,
    target: str,
    prefix: str,
    directory: Path,
    addresses: list[int],
    mark: Callable[[str], None],
) -> dict:
    # Zipimport supplies the frozen head package while baseline source is checked out.
    from wiz8decomp.comparison import compare_data, compare_selected
    from wiz8decomp.config import load_settings
    from wiz8decomp.source_index import write_source_index

    settings = load_settings(repository=repository)
    assert settings is not None
    mark("source-index")
    write_source_index(settings)
    mark("comparison")
    result = compare_selected(
        repository,
        target,
        addresses,
        settings.ghidra_install_dir,
        classify_source_non_emissions=True,
        classify_template_emissions=True,
    )
    (directory / f"{prefix}-base-summary.json").write_text(json.dumps(result))
    if not result["ok"]:
        raise ValueError("Baseline comparison contains unpaired or incomplete functions")
    if target == "SURRENDER":
        from wiz8decomp.surrender_exports import validate_built_surrender_exports

        mark("exports")
        exports = validate_built_surrender_exports(
            repository, settings.product_build_dir / "sr.dll"
        )
        (directory / f"{prefix}-base-exports.json").write_text(json.dumps(exports))
    mark("datacmp")
    data = compare_data(repository, target)
    (directory / f"{prefix}-base-datacmp.json").write_text(json.dumps(data))
    if not data["ok"]:
        print("::notice::merge-base datacmp reports differences", flush=True)
    return result


def compare_base(
    repository: Path,
    target: str,
    build_target: str,
    base: str,
    head: str,
    prefix: str,
    directory: Path,
) -> int:
    """One owner for revision switching, partial evidence and restoration."""
    previous_path = os.environ.get("PATH", "")
    os.environ["PATH"] = f"{repository / '.venv/bin'}{os.pathsep}{previous_path}"
    status_path = directory / f"{prefix}-base-status.json"
    status = {
        "target": target,
        "source_revision": base,
        "comparison_revision": head,
        "status": "running",
        "stage": "snapshot",
        "error": None,
    }

    def mark(name: str) -> None:
        status["stage"] = name
        status_path.write_text(json.dumps(status, indent=2) + "\n")
        print(f"[BASELINE] {target} {base}: {name}", flush=True)

    def command(*args: str) -> None:
        subprocess.run(args, cwd=repository, check=True)

    try:
        mark("selection")
        selection = json.loads((directory / f"{prefix}-head-summary.json").read_text())
        if selection["target"] != target or not selection["functions"]:
            raise ValueError("Head comparison must provide a nonempty selection for this target")
        addresses = [int(row["orig"], 16) for row in selection["functions"]]
        mark("snapshot")
        with tempfile.TemporaryDirectory(
            prefix=f"{prefix}-comparison-tools-", dir=directory
        ) as snapshot:
            archive = Path(snapshot) / "head-tools.zip"
            command(
                "git", "archive", "--format=zip", head, "tools/wiz8decomp", f"--output={archive}"
            )
            # Zipimport keeps all lazy imports bound to the same head implementation.
            package_path = f"{archive}/tools"
            sys.path.insert(0, package_path)
            try:
                mark("checkout")
                command("git", "checkout", "--detach", base)
                baseline_cli = str(repository / ".venv/bin/wiz8")
                mark("prepare")
                command(baseline_cli, "prepare", "--comparison-target", target)
                mark("build")
                command(baseline_cli, "build", build_target)
                result = _reports(repository, target, prefix, directory, addresses, mark)
                result["inputs"].update(source_revision=base, wizardry_revision=head)
                (directory / f"{prefix}-base-summary.json").write_text(json.dumps(result))
                status["status"] = "completed"
            finally:
                sys.path.remove(package_path)
    except Exception as error:  # noqa: BLE001 - persist failures at the CI process boundary
        status["status"] = "failed"
        status["error"] = str(error)
        print(f"::error::baseline {target} {base} failed at {status['stage']}: {error}", flush=True)
    finally:
        try:
            command("git", "checkout", "--detach", head)
        except (OSError, subprocess.SubprocessError) as error:
            status["status"] = "failed"
            status["restore_error"] = str(error)
            print(f"::error::could not restore head {head}: {error}", flush=True)
        os.environ["PATH"] = previous_path
        status_path.write_text(json.dumps(status, indent=2) + "\n")
    return int(status["status"] != "completed")


if __name__ == "__main__":
    target_arg, build_arg, base_arg, head_arg, prefix_arg = sys.argv[1:]
    raise SystemExit(
        compare_base(
            Path.cwd(),
            target_arg,
            build_arg,
            base_arg,
            head_arg,
            prefix_arg,
            Path(os.environ["RUNNER_TEMP"]),
        )
    )
