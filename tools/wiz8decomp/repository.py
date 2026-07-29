"""Environment-free checks for files that must never enter repository history."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

FORBIDDEN_SUFFIXES = frozenset(
    {
        ".7z",
        ".asi",
        ".bik",
        ".cab",
        ".dll",
        ".exe",
        ".exp",
        ".iso",
        ".lib",
        ".log",
        ".m3d",
        ".map",
        ".mp3",
        ".obj",
        ".pdb",
        ".rar",
        ".slf",
        ".swo",
        ".swp",
        ".wav",
        ".zip",
    }
)
FORBIDDEN_PARTS = frozenset({"build", ".idea", ".vscode", "__pycache__", ".pytest_cache"})
LARGE_FILE_LIMIT = 10 * 1024 * 1024


class RepositoryHygieneError(RuntimeError):
    """Tracked generated or proprietary artifacts were found."""


def tracked_paths(repository: Path) -> list[Path]:
    # History is Jujutsu-first and a checkout need not be colocated, so a bare
    # `.git` is the fallback rather than the assumption.
    if (repository / ".jj").is_dir():
        argv = ["jj", "file", "list", "--no-pager", "--quiet", "-T", 'path ++ "\\0"']
    else:
        argv = ["git", "ls-files", "-z"]
    completed = subprocess.run(
        argv,
        cwd=repository,
        capture_output=True,
        check=True,
    )
    return [
        repository / raw.decode("utf-8", errors="strict")
        for raw in completed.stdout.split(b"\0")
        if raw
    ]


def validate_repository_hygiene(repository: Path) -> dict[str, Any]:
    problems: list[str] = []
    tracked = tracked_paths(repository)
    for path in tracked:
        relative = path.relative_to(repository).as_posix()
        if not path.exists():
            continue
        approved_checkpoint = (
            relative.startswith("vendor/ghidra/exports/") and path.suffix == ".gzf"
        )
        if any(part in FORBIDDEN_PARTS for part in path.relative_to(repository).parts):
            problems.append(f"{relative}: generated/editor directory")
        elif path.suffix.casefold() in FORBIDDEN_SUFFIXES and not approved_checkpoint:
            problems.append(f"{relative}: forbidden binary, archive, or log")
        elif path.stat().st_size > LARGE_FILE_LIMIT and not approved_checkpoint:
            problems.append(f"{relative}: {path.stat().st_size} bytes exceeds {LARGE_FILE_LIMIT}")
    if problems:
        raise RepositoryHygieneError(
            "repository hygiene failed:\n  " + "\n  ".join(sorted(problems))
        )
    return {"ok": True, "tracked_files": len(tracked), "large_file_limit": LARGE_FILE_LIMIT}
