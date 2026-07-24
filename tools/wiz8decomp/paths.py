from __future__ import annotations

import hashlib
import json
import os
import tempfile
from pathlib import Path
from typing import Any, Iterable


def sha256_file(path: Path, chunk_size: int = 4 * 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(chunk_size), b""):
            digest.update(chunk)
    return digest.hexdigest()


def atomic_write(path: Path, data: str | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    binary = isinstance(data, bytes)
    mode = "wb" if binary else "w"
    kwargs = {} if binary else {"encoding": "utf-8", "newline": "\n"}
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, mode, **kwargs) as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def atomic_json(path: Path, value: Any) -> None:
    atomic_write(path, json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n")


def safe_relative(path: Path, root: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(root.resolve()).as_posix()
    except ValueError as error:
        raise ValueError(f"{path} is outside {root}") from error


def tree_manifest(root: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for path in sorted((p for p in root.rglob("*") if p.is_file()), key=lambda p: p.relative_to(root).as_posix().casefold()):
        entries.append({
            "path": path.relative_to(root).as_posix(),
            "size": path.stat().st_size,
            "sha256": sha256_file(path),
        })
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

