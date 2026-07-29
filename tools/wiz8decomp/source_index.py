"""Thin Wizardry adapter for reccmp's compiler-backed source index.

reccmp emits one Clang AST per compile command and joins the result with the
reccmp markers. Done literally that is one container per translation unit, run
one at a time: 147 units here, each paying container startup before clang sees
a byte, with the machine's other 27 cores idle and every unit reparsed on every
invocation even when nothing changed.

This adapter keeps reccmp's parsing and joining exactly as they are - the AST
documents it feeds `from_ast_documents_targets` are the ones reccmp would have
produced itself - and changes only how they are obtained:

* a fingerprint short-circuit, so an index that is already current is not
  rebuilt at all;
* one container for the whole batch instead of one per unit;
* the units in that batch parsed in parallel inside it.

Caching the AST documents themselves was tried and removed. Once the batch runs
in parallel in one container the entire clang step is about seven seconds, while
the documents are 3.7 GB and reloading them still costs the twenty-odd seconds
of json.loads that then dominate - so the cache bought about seven seconds for
several gigabytes. Redundant work is avoided at the index level instead, by the
fingerprint below. The only state kept under `build/` is that fingerprint, and
it is disposable: deleting it costs one rebuild, never accuracy.
"""

from __future__ import annotations

import hashlib
import json
import os
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex

# reccmp builds the AST argument list by stripping the output flags out of a
# compile command, and joins the documents through a collector that maps the
# compiler's view of a path back onto the repository. Reusing those rather than
# restating them keeps both the arguments and the join identical to what reccmp
# would have done; its public document entry points take no compilation root, so
# the collector is the only way to keep the /repo mapping the container needs.
# reccmp is pinned to an exact revision, so this is a fixed surface.
from reccmp.source.index import _ast_command, _AstCollector

from .build import LINT_BUILD_DIR, VC6_IMAGE
from .config import Settings
from .subprocesses import resolve_executable

_SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
_HEADER_SUFFIXES = frozenset({".h", ".hpp", ".inc"})
_CACHE_DIR = "build/source-index-cache"
_INDEX_PATH = "build/source-index.json"


def _source_paths(repository: Path, stem: str) -> tuple[Path, ...]:
    roots = (repository / f"src/{stem}", repository / f"include/{stem}")
    return tuple(
        sorted(
            path for root in roots for path in root.rglob("*") if path.suffix in _SOURCE_SUFFIXES
        )
    )


def _digest(*parts: bytes) -> str:
    digest = hashlib.sha256()
    for part in parts:
        digest.update(len(part).to_bytes(8, "little"))
        digest.update(part)
    return digest.hexdigest()


def _header_fingerprint(repository: Path) -> str:
    """One digest over every header a recovered unit can include.

    An AST depends on the headers the unit pulls in, not only on its own text,
    so a per-unit key over the `.cpp` alone would happily serve a stale AST
    after a header edit. Hashing the whole header surface makes a header change
    invalidate every unit: coarser than tracking real include edges, and the
    error is always in the safe direction. Content is hashed rather than mtime
    because a jj rebase rewrites the working copy and would otherwise
    invalidate everything on every history operation.
    """

    roots = (
        repository / "include",
        repository / "src",
        repository / "config",
        repository / "third_party/sfi-sgp/sgp",
    )
    digest = hashlib.sha256()
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix in _HEADER_SUFFIXES and path.is_file():
                digest.update(path.relative_to(repository).as_posix().encode())
                digest.update(path.read_bytes())
    return digest.hexdigest()


def _entry_source(entry: dict[str, Any]) -> Path:
    return Path(entry.get("directory") or ".") / entry["file"]


def _validate_database(repository: Path, compilation_database: Path) -> None:
    if not compilation_database.is_file():
        raise FileNotFoundError(
            f"{compilation_database} is missing; run `just lint` to generate it"
        )
    database_mtime = compilation_database.stat().st_mtime
    inventories = (
        repository / "src/wiz8/sources.cmake",
        repository / "src/surrender/CMakeLists.txt",
        repository / "CMakeLists.txt",
    )
    stale = [
        path for path in inventories if path.is_file() and path.stat().st_mtime > database_mtime
    ]
    if stale:
        raise RuntimeError(
            "the compile database predates "
            + ", ".join(path.relative_to(repository).as_posix() for path in stale)
            + "; run `just lint` to reconfigure it before indexing"
        )


def _emit_batch(
    settings: Settings,
    pending: list[tuple[str, list[str]]],
    cache: Path,
) -> None:
    """Parse every pending unit in one container, in parallel.

    Each unit gets its own script so the arguments keep the quoting reccmp gave
    them, and `xargs` only ever passes a filename. A unit that fails leaves its
    stderr next to the missing output, which is what the caller reports.
    """

    repository = settings.repo_dir.resolve()
    clang_build = repository / LINT_BUILD_DIR
    jobs = cache / "jobs"
    shutil.rmtree(jobs, ignore_errors=True)
    jobs.mkdir(parents=True)

    for index, (key, arguments) in enumerate(pending):
        script = " ".join(shlex.quote(argument) for argument in arguments)
        (jobs / f"{index:05d}.sh").write_text(
            f"{script} > /ast/out/{key}.json 2> /ast/out/{key}.err\n"
            f"test -s /ast/out/{key}.json || exit 0\n"
            f"rm -f /ast/out/{key}.err\n",
            encoding="utf-8",
        )

    (cache / "out").mkdir(parents=True, exist_ok=True)
    docker = resolve_executable("docker") or "docker"
    jobs_ = max(1, min(len(pending), os.cpu_count() or 1))
    command = (
        docker,
        "run",
        "--rm",
        "--init",
        "--network",
        "none",
        "--volume",
        f"{repository}:/repo:ro",
        "--volume",
        f"{clang_build}:/out:ro",
        "--volume",
        f"{settings.work_dir / 'fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4'}:/zlib:ro",
        "--volume",
        f"{cache}:/ast",
        "--workdir",
        "/out",
        "--entrypoint",
        "/bin/sh",
        VC6_IMAGE,
        "-c",
        f"ls /ast/jobs/*.sh | xargs -P {jobs_} -n 1 /bin/sh",
    )
    subprocess.run(command, check=False, capture_output=True)
    shutil.rmtree(jobs, ignore_errors=True)


def build_source_index(settings: Settings) -> SourceIndex:
    """Replay clang-cl compile commands and join their AST with reccmp markers."""

    repository = settings.repo_dir.resolve()
    clang_build = repository / LINT_BUILD_DIR
    compilation_database = clang_build / "compile_commands.json"
    _validate_database(repository, compilation_database)

    database = json.loads(compilation_database.read_text(encoding="utf-8"))

    # reccmp prepends the command prefix in place of the recorded compiler, so
    # naming clang-cl here yields exactly the argument list it would have run,
    # minus the docker wrapper that used to be one container per unit.
    #
    # The AST documents are deliberately NOT cached between runs. Caching them
    # was measured against this tree and is the wrong trade: with the batch
    # parsed in parallel inside one container the whole clang step is about
    # seven seconds, while the documents it produces are 3.7 GB, and rebuilding
    # from them still costs the twenty-odd seconds of json.loads that dominate
    # what is left. The rebuild is skipped wholesale by the fingerprint in
    # write_source_index, which is where redundant work is actually avoided.
    units = [(entry, _ast_command(entry, None, ("/usr/bin/clang-cl",))) for entry in database]

    documents: list[tuple[dict[str, Any], Path]] = []
    with tempfile.TemporaryDirectory(prefix="wiz8-ast-", dir=repository / "build") as scratch:
        cache = Path(scratch)
        _emit_batch(
            settings,
            [(f"{index:05d}", arguments) for index, (_, arguments) in enumerate(units)],
            cache,
        )
        for index, (entry, _arguments) in enumerate(units):
            payload = cache / "out" / f"{index:05d}.json"
            if not payload.is_file() or payload.stat().st_size == 0:
                failure = cache / "out" / f"{index:05d}.err"
                detail = (
                    failure.read_text(encoding="utf-8", errors="replace")
                    if failure.is_file()
                    else ""
                )
                raise RuntimeError(f"Clang did not emit an AST for {entry.get('file')}: {detail}")
            documents.append(
                (json.loads(payload.read_text(encoding="utf-8")), _entry_source(entry))
            )

    # The compilation root is what the compiler saw, which inside the container
    # is /repo. Dropping it makes every recorded path fail to resolve back onto
    # the checkout and every marker bind to nothing.
    collector = _AstCollector(repository, Path("/repo"))
    for document, main_file in documents:
        collector.collect(document, main_file)
    targets = {
        "WIZ8": _source_paths(repository, "wiz8"),
        "SURRENDER": _source_paths(repository, "surrender"),
    }
    indexes = [
        SourceIndex._from_collector(repository, target, paths, collector)
        for target, paths in targets.items()
    ]
    return SourceIndex(
        declarations=(item for index in indexes for item in index.declarations),
        classes=(item for index in indexes for item in index.classes),
        markers=(item for index in indexes for item in index.markers),
    )


def _summary(settings: Settings, index: SourceIndex) -> dict[str, Any]:
    return {
        "path": _INDEX_PATH,
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
    }


def write_source_index(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    """Write the disposable canonical source projection.

    Skips the whole rebuild when the inputs are unchanged. Callers that only
    need a current index - the check lane, the test lane - therefore pay for it
    once rather than on every invocation.
    """

    repository = settings.repo_dir.resolve()
    destination = repository / _INDEX_PATH
    compilation_database = repository / LINT_BUILD_DIR / "compile_commands.json"
    _validate_database(repository, compilation_database)

    stamp = repository / _CACHE_DIR / "inputs.sha256"
    fingerprint = _digest(
        b"wiz8.source-index.inputs.v1",
        compilation_database.read_bytes(),
        _header_fingerprint(repository).encode(),
        _digest(
            *(
                path.read_bytes()
                for path in sorted(repository.glob("src/**/*"))
                if path.suffix in {".c", ".cpp"} and path.is_file()
            )
        ).encode(),
    )
    if (
        not force
        and destination.is_file()
        and stamp.is_file()
        and stamp.read_text(encoding="utf-8").strip() == fingerprint
    ):
        document = json.loads(destination.read_text(encoding="utf-8"))
        return {
            "path": _INDEX_PATH,
            "markers": len(document.get("markers", [])),
            "declarations": len(document.get("declarations", [])),
            "classes": len(document.get("classes", [])),
            "reused": True,
        }

    index = build_source_index(settings)
    index.write(destination)
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(fingerprint + "\n", encoding="utf-8")
    return _summary(settings, index)
