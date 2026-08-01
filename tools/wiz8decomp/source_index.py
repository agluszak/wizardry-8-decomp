"""Thin Wizardry adapter for reccmp's compiler-backed source index.

reccmp emits one Clang AST per compile command and joins the result with the
reccmp markers. Done literally that is one container per translation unit, run
one at a time, each unit serialising its whole AST to JSON so Python can walk it
back: 147 units here, each paying container startup before clang sees a byte,
with the machine's other 27 cores idle.

This adapter keeps reccmp's join exactly as it is - the declarations, classes and
markers it produces are the ones reccmp would have derived itself - and changes
only how the semantic records are obtained:

* a fingerprint short-circuit, so an index that is already current is not
  rebuilt at all;
* one container for the whole batch instead of one per unit;
* the units in that batch parsed in parallel inside it;
* each unit emitting the index records directly from Clang's AST, rather than
  the whole AST as JSON for Python to rebuild them from.

The last of those is `tools/source-indexer/indexer.cpp`, and it is the reason
this is fast rather than merely parallel. The JSON documents were 3.7 GB, of
which the index kept a few per cent; loading them back cost about a minute of
json.loads and tree walking, some fifty times what compiling the same sources
costs. The same records come to ~34 MB of NDJSON, and the whole index is a few
seconds. Caching the JSON documents was tried first and was the wrong trade: it
bought about seven seconds for several gigabytes, because the reload dominated.

What stays in reccmp: deduplication across translation units is spelled out
below because the collector is the boundary, but marker parsing, marker-to-
declaration binding, asserted sizes and vtable addresses all come from
`SourceIndex._from_collector`, which owns them.
"""

from __future__ import annotations

import hashlib
import json
import os
import shlex
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from reccmp.parser.marker import match_marker
from reccmp.source import SourceIndex

# `_ast_command` builds reccmp's own argument list by stripping the output flags
# out of a compile command. Reusing it rather than restating it keeps the
# arguments the indexer replays identical to the ones reccmp would have compiled.
# reccmp is pinned to an exact revision, so this is a fixed surface.
from reccmp.source.index import (
    SourceClass,
    SourceDeclaration,
    SourceField,
    SourceIndexError,
    _ast_command,
    _AstCollector,
)

from .build import LINT_BUILD_DIR, VC6_IMAGE, build_source_indexer
from .config import Settings
from .subprocesses import resolve_executable

_SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
_HEADER_SUFFIXES = frozenset({".h", ".hpp", ".inc"})
_CACHE_DIR = "build/source-index-cache"
_INDEX_PATH = "build/source-index.json"
_INDEXER_CONTAINER_PATH = "/indexer/indexer"
_CLANG_CL = "/usr/bin/clang-cl"


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

    A unit's records depend on the headers it pulls in, not only on its own
    text, so a per-unit key over the `.cpp` alone would happily serve a stale
    record set after a header edit. Hashing the whole header surface makes a
    header change invalidate every unit: coarser than tracking real include
    edges, and the error is always in the safe direction. Content is hashed
    rather than mtime because a jj rebase rewrites the working copy and would
    otherwise invalidate everything on every history operation.
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


def _index_command(entry: dict[str, Any]) -> list[str]:
    """The indexer invocation for one compile command.

    reccmp's own argument list is the starting point, with clang-cl named as the
    compiler so the driver inside the indexer selects cl mode from the program
    name exactly as the real build does, and the indexer itself in front of it.
    reccmp's `-ast-dump=json` request is the one thing dropped: the records come
    out of the AST rather than a serialised copy of it. The build's other
    `-Xclang` options have to survive, so the pair is removed by position rather
    than by name.
    """

    arguments = _ast_command(entry, None, (_INDEXER_CONTAINER_PATH, _CLANG_CL))
    position = arguments.index("-ast-dump=json")
    del arguments[position - 1 : position + 1]
    return arguments


class _RecordCollector(_AstCollector):
    """reccmp's own collector, filled from the indexer's records.

    It stays reccmp's class because `SourceIndex._from_collector` reads its
    state directly; only the way that state arrives changes. Deduplication is
    the one piece of the collector restated here, because the records arrive
    already reduced: a declaration is replaced when a later unit contributes the
    definition, a class record is kept from the first unit that gave it a source
    line, and conflicting size assertions are an error rather than a
    last-one-wins.
    """

    def __init__(self, repository: Path) -> None:
        super().__init__(repository)
        self.source_metadata: dict[str, dict[str, Any]] = {}

    def collect_records(self, records: str) -> None:
        for line in records.splitlines():
            if line:
                self._collect(json.loads(line))

    def _collect(self, record: dict[str, Any]) -> None:
        kind = record.pop("record")
        if kind == "declaration":
            source_signature = record.pop("source_signature", None)
            parameter_references = record.pop("parameter_references", None)
            declaration = SourceDeclaration(
                **{**record, "parameter_types": tuple(record["parameter_types"])}
            )
            previous = self.declarations.get(declaration.semantic_id)
            if previous is None or (declaration.is_definition and not previous.is_definition):
                self.declarations[declaration.semantic_id] = declaration
                if isinstance(source_signature, str):
                    self.source_metadata[declaration.semantic_id] = {
                        "source_signature": source_signature,
                        "parameter_references": parameter_references or [],
                    }
        elif kind == "class":
            source_class = SourceClass(
                **{
                    **record,
                    "bases": tuple(record["bases"]),
                    "fields": tuple(SourceField(**field) for field in record["fields"]),
                    "virtual_declarations": tuple(record["virtual_declarations"]),
                }
            )
            previous = self.classes.get(source_class.semantic_id)
            if previous is None or (not previous.line and source_class.line):
                self.classes[source_class.semantic_id] = source_class
        elif kind == "size-assertion":
            name = record["qualified_name"]
            asserted_size = record["asserted_size"]
            previous_size = self.size_assertions.get(name)
            if previous_size is not None and previous_size != asserted_size:
                raise SourceIndexError(
                    f"{name} has conflicting size assertions: "
                    f"{previous_size:#x} and {asserted_size:#x}"
                )
            self.size_assertions[name] = asserted_size
        else:
            raise SourceIndexError(f"the source indexer emitted an unknown record: {kind!r}")


def _emit_batch(
    settings: Settings,
    indexer: Path,
    pending: list[tuple[str, list[str]]],
    scratch: Path,
) -> None:
    """Index every pending unit in one container, in parallel.

    Each unit gets its own script so the arguments keep the quoting reccmp gave
    them, and `xargs` only ever passes a filename. The exit status is recorded
    next to the output because a unit that legitimately contributes no records -
    a translation unit holding only `LIBRARY:` markers - emits nothing at all,
    and must not be confused with one that failed.
    """

    repository = settings.repo_dir.resolve()
    jobs = scratch / "jobs"
    jobs.mkdir(parents=True)
    (scratch / "out").mkdir(parents=True, exist_ok=True)

    for index, (key, arguments) in enumerate(pending):
        script = " ".join(shlex.quote(argument) for argument in arguments)
        (jobs / f"{index:05d}.sh").write_text(
            f"{script} > /work/out/{key}.ndjson 2> /work/out/{key}.err\n"
            f"echo $? > /work/out/{key}.status\n",
            encoding="utf-8",
        )

    docker = resolve_executable("docker") or "docker"
    parallelism = max(1, min(len(pending), os.cpu_count() or 1))
    command = (
        docker,
        "run",
        "--rm",
        "--init",
        "--network",
        "none",
        # clang writes temporary files, and the image points TMP at a Wine path.
        "--env",
        "TMPDIR=/tmp",
        "--volume",
        f"{repository}:/repo:ro",
        "--volume",
        f"{repository / LINT_BUILD_DIR}:/out:ro",
        "--volume",
        f"{indexer.parent}:/indexer:ro",
        "--volume",
        f"{settings.work_dir / 'fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4'}:/zlib:ro",
        "--volume",
        f"{scratch}:/work",
        "--workdir",
        "/out",
        "--entrypoint",
        "/bin/sh",
        VC6_IMAGE,
        "-c",
        f"ls /work/jobs/*.sh | xargs -P {parallelism} -n 1 /bin/sh",
    )
    subprocess.run(command, check=False, capture_output=True)


def build_source_index(settings: Settings) -> tuple[SourceIndex, dict[str, dict[str, Any]]]:
    """Replay clang-cl compile commands and join their records with the markers."""

    repository = settings.repo_dir.resolve()
    compilation_database = repository / LINT_BUILD_DIR / "compile_commands.json"
    _validate_database(repository, compilation_database)
    indexer = build_source_indexer(settings)

    database = json.loads(compilation_database.read_text(encoding="utf-8"))
    units = [(entry, _index_command(entry)) for entry in database]

    collector = _RecordCollector(repository)
    with tempfile.TemporaryDirectory(prefix="wiz8-index-", dir=repository / "build") as directory:
        scratch = Path(directory)
        _emit_batch(
            settings,
            indexer,
            [(f"{index:05d}", arguments) for index, (_, arguments) in enumerate(units)],
            scratch,
        )
        for index, (entry, _arguments) in enumerate(units):
            key = f"{index:05d}"
            status = scratch / "out" / f"{key}.status"
            payload = scratch / "out" / f"{key}.ndjson"
            failure = scratch / "out" / f"{key}.err"
            detail = (
                failure.read_text(encoding="utf-8", errors="replace") if failure.is_file() else ""
            )
            if not status.is_file() or status.read_text(encoding="utf-8").strip() != "0":
                raise SourceIndexError(
                    f"the source indexer failed on {entry.get('file')}: {detail}"
                )
            collector.collect_records(payload.read_text(encoding="utf-8"))

    targets = {
        "WIZ8": _source_paths(repository, "wiz8"),
        "SURRENDER": _source_paths(repository, "surrender"),
    }
    indexes = [
        SourceIndex._from_collector(repository, target, paths, collector)
        for target, paths in targets.items()
    ]
    index = SourceIndex(
        declarations=(item for index in indexes for item in index.declarations),
        classes=(item for index in indexes for item in index.classes),
        markers=(item for index in indexes for item in index.markers),
    )
    return index, collector.source_metadata


def _summary(settings: Settings, index: SourceIndex) -> dict[str, Any]:
    return {
        "path": _INDEX_PATH,
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
    }


def _index_document(
    repository: Path, index: SourceIndex, source_metadata: dict[str, dict[str, Any]]
) -> dict[str, Any]:
    """Retain marker properties that reccmp's v1 projection omits.

    A `folded` marker names another compiler emission whose original body shares
    an address with the canonical source function. It is evidence for emitted
    code, not a second owner of that address. reccmp's parser already records
    this distinction, but SourceMarker does not yet carry it into JSON, so
    recover it from the adjacent source marker while writing our disposable
    projection.
    """

    document = index.to_dict()
    for declaration in document["declarations"]:
        metadata = source_metadata.get(declaration["semantic_id"])
        if metadata is not None:
            declaration.update(metadata)
    for marker in document["markers"]:
        declaration = marker.get("declaration")
        if declaration:
            metadata = source_metadata.get(declaration["semantic_id"])
            if metadata is not None:
                declaration.update(metadata)
    source_lines: dict[str, list[str]] = {}
    for item in document["markers"]:
        source_file = item["source_file"]
        lines = source_lines.get(source_file)
        if lines is None:
            lines = (repository / source_file).read_text(encoding="utf-8").splitlines()
            source_lines[source_file] = lines

        marker_line = int(item["line"]) - 2
        while marker_line >= 0:
            marker = match_marker(lines[marker_line])
            if marker is None:
                break
            if (
                marker.offset == int(item["address"])
                and marker.type.name == item["marker_kind"]
                and marker.extra is not None
                and marker.extra.casefold() == "folded"
            ):
                item["folded"] = True
                break
            marker_line -= 1
    return document


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
        b"wiz8.source-index.inputs.v2",
        compilation_database.read_bytes(),
        _header_fingerprint(repository).encode(),
        (repository / "tools/source-indexer/indexer.cpp").read_bytes(),
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

    index, source_metadata = build_source_index(settings)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(
        json.dumps(_index_document(repository, index, source_metadata), indent=2) + "\n",
        encoding="utf-8",
    )
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(fingerprint + "\n", encoding="utf-8")
    return _summary(settings, index)
