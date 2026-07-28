from __future__ import annotations

import hashlib
import json
import os
import tempfile
from collections.abc import Callable, Iterable
from pathlib import Path
from typing import Any


def sha256_file(path: Path, chunk_size: int = 4 * 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(chunk_size), b""):
            digest.update(chunk)
    return digest.hexdigest()


def atomic_write(path: Path, data: str | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        if isinstance(data, bytes):
            stream = os.fdopen(fd, "wb")
        else:
            stream = os.fdopen(fd, "w", encoding="utf-8", newline="\n")
        with stream:
            stream.write(data)  # type: ignore[arg-type]
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def atomic_json(path: Path, value: Any) -> None:
    atomic_write(path, json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n")


def json_hash(value: Any) -> str:
    payload = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def safe_relative(path: Path, root: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(root.resolve()).as_posix()
    except ValueError as error:
        raise ValueError(f"{path} is outside {root}") from error


def tree_manifest(root: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for path in sorted(
        (p for p in root.rglob("*") if p.is_file()),
        key=lambda p: p.relative_to(root).as_posix().casefold(),
    ):
        entries.append(
            {
                "path": path.relative_to(root).as_posix(),
                "size": path.stat().st_size,
                "sha256": sha256_file(path),
            }
        )
    return entries


def tree_hash(entries: Iterable[dict[str, Any]]) -> str:
    digest = hashlib.sha256()
    for entry in entries:
        digest.update(entry["path"].encode("utf-8"))
        digest.update(b"\0")
        digest.update(str(entry["size"]).encode("ascii"))
        digest.update(b"\0")
        digest.update(entry["sha256"].encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def ensure_safe_generated_target(target: Path, work_dir: Path) -> None:
    target = target.resolve()
    work_dir = work_dir.resolve()
    if target == work_dir or work_dir not in target.parents:
        raise ValueError(f"generated target must be a child of WIZ8_WORK_DIR: {target}")


def build_directory_atomically[DirectoryResult](
    destination: Path,
    work_dir: Path,
    builder: Callable[[Path], DirectoryResult],
) -> DirectoryResult:
    """Build a new directory beside its destination and publish it with one rename."""
    ensure_safe_generated_target(destination, work_dir)
    if destination.exists():
        raise RuntimeError(f"refusing to replace existing generated directory: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary_parent = Path(
        tempfile.mkdtemp(prefix=f".{destination.name}.building-", dir=destination.parent)
    )
    candidate = temporary_parent / "tree"
    try:
        result = builder(candidate)
        if not candidate.is_dir():
            raise RuntimeError(f"stage did not produce its expected directory: {candidate}")
        if destination.exists():
            raise RuntimeError(f"generated destination appeared during build: {destination}")
        candidate.replace(destination)
        return result
    finally:
        import shutil

        shutil.rmtree(temporary_parent, ignore_errors=True)
