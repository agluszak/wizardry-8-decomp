"""Project paths, SYNTHETIC rules, and the compile-DB adapter for reccmp's source index."""

from __future__ import annotations

import glob
import hashlib
import json
import logging
import os
import re
import shlex
import shutil
import subprocess
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex, SourceIndexError, SourceMarker

from .config import Settings
from .paths import compile_database_relative

_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})
_SYNTHETIC_MARKER = re.compile(r"^\s*//\s*SYNTHETIC:\s+")
_SOURCE_MARKER = re.compile(r"^\s*//\s*(?:FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY|VTABLE|GLOBAL):\s+")
LINT_ONLY_SOURCE_ROOTS = ("tests/runtime",)
_ATTACHED_INCLUDE_FLAGS = (
    "-isystem",
    "-iquote",
    "-idirafter",
    "-include",
    "-I",
    "/FI",
    "-FI",
    "/I",
    "/Fo",
    "/Fd",
)
# The analysis image exports Wine TEMP/TMP as Z:\out\tmp. Linux clang++ and the
# indexer binary treat those as the process temp directory, so docker runs that
# are not Wine jobs have to point them at a real Unix path.
_ANALYSIS_LINUX_TEMP = ("-e", "TMPDIR=/tmp", "-e", "TMP=/tmp", "-e", "TEMP=/tmp")
LOGGER = logging.getLogger(__name__)


def validate_synthetic_marker_blocks(repository: Path) -> int:
    """Require marker-only synthetic identities with an explicit block end."""
    failures: list[str] = []
    count = 0
    for root_name in ("src", "include"):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for index, line in enumerate(lines):
                if not _SYNTHETIC_MARKER.match(line):
                    continue
                count += 1
                location = f"{path.relative_to(repository)}:{index + 1}"
                if index + 1 >= len(lines) or not lines[index + 1].lstrip().startswith("//"):
                    failures.append(f"{location}: SYNTHETIC lacks its identity comment")
                    continue
                if index + 2 >= len(lines):
                    continue
                following = lines[index + 2]
                if following.strip() and not _SOURCE_MARKER.match(following):
                    failures.append(
                        f"{location}: SYNTHETIC owns no declaration or body; "
                        "end the marker block before the next source entity"
                    )
    if failures:
        raise SourceIndexError("invalid SYNTHETIC marker blocks:\n" + "\n".join(failures))
    return count


def project_targets(repository: Path) -> dict[str, dict[str, Any]]:
    import yaml

    document = yaml.safe_load((repository / "reccmp-project.yml").read_text(encoding="utf-8"))
    return {str(name).upper(): value for name, value in (document.get("targets") or {}).items()}


def _source_roots(config: dict[str, Any]) -> tuple[str, ...]:
    value = config.get("source-root", ())
    return (value,) if isinstance(value, str) else tuple(value)


def load_source_index(repository: Path) -> dict[str, Any]:
    path = repository / "build/source-index.json"
    if not path.is_file():
        raise SourceIndexError(f"{path} is missing; run `uv run wiz8 check`")
    return _read_source_index_document(path)


def _read_source_index_document(path: Path) -> dict[str, Any]:
    document = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(document, Mapping):
        raise SourceIndexError(f"{path} must contain a JSON object")
    return dict(document)


def try_load_source_index(repository: Path) -> dict[str, Any] | None:
    """Return the existing compiler-backed index, or None when it cannot be read.

    Read paths must not create, refresh, or compile this file.
    """

    path = repository / "build/source-index.json"
    if not path.is_file():
        return None
    return _read_source_index_document(path)


def source_index_freshness(repository: Path, target: str = "WIZ8") -> dict[str, Any]:
    """Label existing source-index state without compiling or regenerating it."""

    from .build import clang_configure_inputs

    path = repository / "build/source-index.json"
    relative = "build/source-index.json"
    if not path.is_file():
        return {
            "state": "missing",
            "path": relative,
            "detail": f"{relative} is missing; run `uv run wiz8 check`",
        }
    try:
        document = _read_source_index_document(path)
        document["member_uses"]
    except (OSError, ValueError, KeyError) as error:
        return {
            "state": "invalid",
            "path": relative,
            "detail": f"{relative} is invalid: {error}",
        }
    indexed_at = path.stat().st_mtime_ns
    roots = indexed_targets(repository).get(target.upper(), ())
    inputs = list(clang_configure_inputs(repository))
    inputs.extend(
        candidate
        for root in roots
        for candidate in (repository / root).rglob("*")
        if candidate.suffix.lower() in _SOURCE_SUFFIXES
    )
    stale = any(item.is_file() and item.stat().st_mtime_ns > indexed_at for item in inputs)
    if stale:
        return {
            "state": "stale",
            "path": relative,
            "detail": (
                "source or build inputs are newer than build/source-index.json; "
                "native ProgramDB facts are still readable"
            ),
        }
    return {"state": "current", "path": relative, "detail": None}


def warn_if_source_index_may_be_stale(repository: Path, target: str) -> bool:
    """Warn when source/configuration inputs postdate the existing index."""

    freshness = source_index_freshness(repository, target)
    if freshness["state"] in {"missing", "invalid"}:
        load_source_index(repository)
    if freshness["state"] == "stale":
        LOGGER.warning(
            "source index may be stale; source or build inputs are newer than "
            "build/source-index.json\n"
            "         run `uv run wiz8 check` to refresh selector metadata"
        )
        return True
    return False


@dataclass(frozen=True)
class AddressBoundIdentity:
    """One explicit retail-address binding from the compiler-backed source model."""

    target: str
    address: int
    name: str
    qualified_name: str
    source_file: str
    line: int
    kind: str
    marker_kind: str | None
    semantic_id: str
    calling_convention: str | None
    return_type: str | None
    parameter_types: tuple[str, ...]
    has_this: bool
    is_variadic: bool
    owning_class: str | None
    source_signature: str | None
    is_definition: bool


def _declaration_is_variadic(entry: Mapping[str, Any], signature: str | None = None) -> bool:
    if "is_variadic" in entry:
        return bool(entry.get("is_variadic"))
    text = signature if signature is not None else str(entry.get("source_signature") or "")
    stripped = text.rstrip()
    return ", ..." in stripped or stripped.endswith(("...)", ",...)"))


def _namespace_for_source(source_file: str, targets: dict[str, dict[str, Any]]) -> str:
    for name, config in targets.items():
        roots = config.get("source-root", ())
        roots = (roots,) if isinstance(roots, str) else tuple(roots)
        if any(
            source_file == root or source_file.startswith(root.rstrip("/") + "/") for root in roots
        ):
            return name
    return ""


DeclarationKey = tuple[str, str, str]


def declarations_by_semantic_key(
    document: Mapping[str, Any],
) -> dict[DeclarationKey, dict[str, Any]]:
    """Map a declaration key ``(target, semantic_id, unit_id)`` to its Clang
    record. ``unit_id`` is empty for external declarations; TU-local ones of
    different units may share a mangled name."""

    return {
        (str(entry["target"] or ""), str(entry["semantic_id"]), str(entry["unit_id"] or "")): entry
        for entry in document["declarations"]
    }


def declaration_for_marker(
    marker: Mapping[str, Any],
    declarations_by_key: Mapping[DeclarationKey, Mapping[str, Any]],
) -> dict[str, Any]:
    """The declaration a marker's ``declaration_key`` names, or ``{}``."""

    key = marker["declaration_key"]
    if not key:
        return {}
    target, semantic_id, unit_id = key
    found = declarations_by_key.get((target or "", semantic_id, unit_id or ""))
    return dict(found) if found else {}


def bind_marker_declarations(document: Mapping[str, Any]) -> list[dict[str, Any]]:
    """Copy markers with ``declaration`` filled from ``declaration_key`` when needed."""

    keys = declarations_by_semantic_key(document)
    bound: list[dict[str, Any]] = []
    for marker in document.get("markers") or []:
        row = dict(marker)
        declaration = declaration_for_marker(row, keys)
        if declaration:
            row["declaration"] = declaration
        bound.append(row)
    return bound


def _identity_from_declaration(
    entry: dict[str, Any],
    *,
    target: str,
    address: int,
    marker_kind: str | None = None,
    kind: str | None = None,
) -> AddressBoundIdentity:
    qualified = str(entry.get("qualified_name") or "")
    return AddressBoundIdentity(
        target=target,
        address=address,
        name=qualified.rsplit("::", 1)[-1],
        qualified_name=qualified,
        source_file=str(entry.get("source_file") or ""),
        line=int(entry.get("line") or 0),
        kind=kind or ("definition" if entry.get("is_definition") else "declaration"),
        marker_kind=marker_kind,
        semantic_id=str(entry.get("semantic_id") or ""),
        calling_convention=str(entry["calling_convention"])
        if entry.get("calling_convention")
        else None,
        return_type=str(entry["return_type"]) if entry.get("return_type") else None,
        parameter_types=tuple(str(item) for item in (entry.get("parameter_types") or ())),
        has_this=bool(entry.get("has_this")),
        is_variadic=_declaration_is_variadic(entry),
        owning_class=str(entry["owning_class"]) if entry.get("owning_class") else None,
        source_signature=str(entry["source_signature"]) if entry.get("source_signature") else None,
        is_definition=bool(entry.get("is_definition")),
    )


def address_bound_identities(
    repository: Path, target: str = "WIZ8"
) -> dict[int, tuple[AddressBoundIdentity, ...]]:
    """Every explicit (target, address) source binding, including declaration-only."""

    from .identity_lint import _declaration_address, _declaration_lines

    document = load_source_index(repository)
    wanted = target.upper()
    targets = project_targets(repository)
    declarations_by_key = declarations_by_semantic_key(document)
    grouped: dict[int, list[AddressBoundIdentity]] = {}

    def add(identity: AddressBoundIdentity) -> None:
        if identity.target != wanted:
            return
        grouped.setdefault(identity.address, []).append(identity)

    for marker in document.get("markers") or []:
        marker_target = str(marker.get("target") or "").upper()
        if marker_target != wanted:
            continue
        address = int(marker["address"])
        embedded = declaration_for_marker(marker, declarations_by_key)
        marker_kind = str(marker.get("marker_kind") or "")
        kind = {
            "FUNCTION": "definition" if embedded.get("is_definition") else "declaration",
            "GLOBAL": "global",
            "VTABLE": "vtable",
            "TEMPLATE": "template",
            "SYNTHETIC": "synthetic",
            "LIBRARY": "library",
        }.get(marker_kind, marker_kind.lower() or "declaration")
        if embedded:
            add(
                _identity_from_declaration(
                    embedded,
                    target=marker_target,
                    address=address,
                    marker_kind=marker_kind,
                    kind=kind,
                )
            )
            continue
        name = str(marker.get("marker_name") or "")
        add(
            AddressBoundIdentity(
                target=marker_target,
                address=address,
                name=name.rsplit("::", 1)[-1],
                qualified_name=name,
                source_file=str(marker.get("source_file") or ""),
                line=int(marker.get("line") or 0),
                kind=kind,
                marker_kind=marker_kind,
                semantic_id="",
                calling_convention=None,
                return_type=None,
                parameter_types=(),
                has_this=False,
                is_variadic=False,
                owning_class=None,
                source_signature=None,
                is_definition=marker_kind == "FUNCTION",
            )
        )

    seen_declarations: set[tuple[str, int, int]] = set()
    for entry in document.get("declarations") or []:
        lines = _declaration_lines(repository, entry)
        if lines is None:
            continue
        address_text = _declaration_address(lines, entry["line"], entry["end_line"])
        if address_text is None:
            continue
        key = (str(entry.get("source_file") or ""), int(entry["line"]), int(entry["end_line"]))
        if key in seen_declarations:
            continue
        seen_declarations.add(key)
        namespace = str(entry.get("target") or "") or _namespace_for_source(
            str(entry.get("source_file") or ""), targets
        )
        add(
            _identity_from_declaration(
                entry,
                target=namespace.upper(),
                address=int(address_text, 16),
            )
        )

    return {address: tuple(identities) for address, identities in sorted(grouped.items())}


def target_for_program(repository: Path, program_name: str) -> str:
    """Resolve a target name, binary filename, or content-addressed Ghidra program."""

    targets = project_targets(repository)
    normalized = program_name.casefold()
    direct = [
        target
        for target, config in targets.items()
        if normalized in {target.casefold(), str(config["filename"]).casefold()}
    ]
    if len(direct) == 1:
        return direct[0]

    match = re.fullmatch(r"wiz8--.+--(.+)--[0-9a-f]{12}", normalized)
    if match:
        stem = match.group(1)
        associated = [
            target
            for target, config in targets.items()
            if re.sub(r"[^a-z0-9]+", "-", Path(config["filename"]).stem.casefold()).strip("-")
            == stem
        ]
        if len(associated) == 1:
            return associated[0]
    raise SourceIndexError(f"program has no configured reccmp target: {program_name}")


def source_functions(repository: Path, target: str = "WIZ8") -> dict[int, SourceMarker]:
    return SourceIndex.from_dict(load_source_index(repository)).functions_by_address(
        target=target.upper()
    )


def _compile_db_files(database: Path, repository: Path) -> set[str]:
    try:
        entries = json.loads(database.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return set()
    files: set[str] = set()
    for entry in entries if isinstance(entries, list) else []:
        raw = str((entry or {}).get("file", ""))
        if not raw:
            continue
        relative = compile_database_relative(raw, repository)
        if relative:
            files.add(relative)
    return files


def indexed_targets(repository: Path, database: Path | None = None) -> dict[str, tuple[str, ...]]:
    """Derive index targets from project source roots plus compile-DB coverage.

    Every reccmp target with a `source-root` is a candidate; when a compile
    database is available, keep only candidates with at least one entry whose
    file falls under one of their roots. This adds the first-party extension
    targets automatically once the lint projection emits their commands,
    without another hard-coded tuple.
    """
    targets = project_targets(repository)
    candidates = {
        target: _source_roots(config) for target, config in targets.items() if _source_roots(config)
    }
    if database is not None and database.is_file():
        covered = _compile_db_files(database, repository)
        if covered:
            filtered = {}
            for target, roots in candidates.items():
                if any(
                    candidate == root or candidate.startswith(root.rstrip("/") + "/")
                    for candidate in covered
                    for root in roots
                ):
                    filtered[target] = roots
            if filtered:
                return filtered
    return candidates


def _source_index_input_digest(repository: Path, database: Path) -> str:
    """Fingerprint inputs whose unchanged projection can safely be reused."""
    from .build import clang_configure_inputs

    digest = hashlib.sha256(b"wiz8-source-index-inputs-v1\0")
    candidates = {
        repository / "reccmp-project.yml",
        repository / "tools/wiz8decomp/source_index.py",
        *clang_configure_inputs(repository),
    }
    roots = indexed_targets(repository, database if database.is_file() else None)
    for source_roots in roots.values():
        for root in source_roots:
            candidates.update(
                path
                for path in (repository / root).rglob("*")
                if path.is_file() and path.suffix.lower() in _SOURCE_SUFFIXES
            )
    lint_headers = repository / "tools/lint/include"
    if lint_headers.is_dir():
        candidates.update(path for path in lint_headers.rglob("*") if path.is_file())
    for path in sorted(candidates, key=lambda item: item.as_posix()):
        if not path.is_file():
            continue
        try:
            identity = path.resolve().relative_to(repository.resolve()).as_posix()
        except ValueError:
            identity = str(path.resolve())
        digest.update(identity.encode() + b"\0" + path.read_bytes() + b"\0")
    if database.is_file():
        digest.update(b"compile_commands.json\0" + database.read_bytes() + b"\0")
    # reccmp's collector and the Python deriving the index from its output.
    for path in _reccmp_index_producers():
        digest.update(path.name.encode() + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def _reccmp_index_producers() -> list[Path]:
    import reccmp.call_facts
    import reccmp.parser
    import reccmp.source

    files = [Path(reccmp.call_facts.__file__)]
    for package in (reccmp.source, reccmp.parser):
        root = Path(next(iter(package.__path__)))
        files.extend(sorted(root.glob("*.py")) + sorted(root.glob("*.cpp")))
    return files


def validate_source_index(repository: Path) -> dict[str, int]:
    validate_synthetic_marker_blocks(repository)
    validate_cross_tu_declarations(repository)
    index = SourceIndex.from_dict(load_source_index(repository))
    counts = {
        target: len(index.functions_by_address(target=target))
        for target in project_targets(repository)
    }
    result: dict[str, int] = {
        "functions": sum(counts.values()),
        "wiz8_functions": counts.get("WIZ8", 0),
        "surrender_functions": counts.get("SURRENDER", 0),
        "classes": len(index.classes),
        "vtable_classes": sum(item.vtable_address is not None for item in index.classes.values()),
        "variables": len(index.variables),
        "conflicts": len(index.conflicts),
    }
    for target in ("SREXT_JPEGIMPORTER", "SREXT_UNZIP"):
        if target in counts:
            result[f"{target.lower()}_functions"] = counts[target]
    return result


_ARRAY_DIMENSION = re.compile(r"\[(\d*)\]$")


def _split_array_type(type_name: str) -> tuple[str, tuple[str, ...]]:
    """Split a C array type into its base element type and its dimensions.

    ``unsigned short[4][4]`` becomes ``("unsigned short", ("4", "4"))`` and an
    incomplete extent is the empty string, so ``char[]`` is ``("char", ("",))``.
    """
    dimensions: list[str] = []
    base = type_name.strip()
    while match := _ARRAY_DIMENSION.search(base):
        dimensions.append(match.group(1))
        base = base[: match.start()].rstrip()
    return base, tuple(reversed(dimensions))


def _array_types_compatible(types: list[str]) -> bool:
    """Whether every spelling describes the same array.

    An incomplete extent (``T[]``) is compatible with any complete extent at
    that dimension; two different known extents are not; the base element type
    and its qualifiers must match, and the array ranks must agree.
    """
    parsed = [_split_array_type(item) for item in types]
    base = parsed[0][0]
    if any(item[0] != base for item in parsed):
        return False
    shapes = [item[1] for item in parsed]
    if len({len(shape) for shape in shapes}) != 1:
        return False
    for level in range(len(shapes[0])):
        known = {shape[level] for shape in shapes if shape[level]}
        if len(known) > 1:
            return False
    return True


def _signatures_compatible(kind: str, signatures: list[tuple[Any, ...]]) -> bool:
    if kind == "variable" and all(len(signature) >= 1 for signature in signatures):
        if len({signature[1:] for signature in signatures}) > 1:
            return False
        return _array_types_compatible([str(signature[0]) for signature in signatures])
    return len(set(signatures)) <= 1


def validate_cross_tu_declarations(repository: Path) -> int:
    """Require one canonical type per external symbol in the Clang index."""
    document = load_source_index(repository)
    rendered: list[str] = []

    recorded: dict[tuple[str, str, str], dict[tuple[str, ...], list[str]]] = {}
    for conflict in document.get("conflicts") or ():
        key = (
            str(conflict.get("target") or ""),
            str(conflict.get("record_kind", "")),
            str(conflict.get("semantic_id", "")),
        )
        variants = recorded.setdefault(key, {})
        for variant in conflict.get("variants") or ():
            signature = tuple(variant.get("signature") or ())
            locations = variants.setdefault(signature, [])
            for location in variant.get("locations") or ():
                if location not in locations:
                    locations.append(location)
    for (target, kind, semantic_id), variants in sorted(recorded.items()):
        if len(variants) < 2:
            continue
        # TU-local spellings never collide at link time: two `static`
        # definitions with different types are independent entities. Only a
        # disagreement involving an externally linked spelling can escape
        # separate compilation undetected.
        if not any(signature and signature[-1] == "external" for signature in variants):
            continue
        if _signatures_compatible(kind, list(variants)):
            continue
        rendered.append(f"{semantic_id} ({kind}) [{target or 'shared'}]")
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    functions: dict[tuple[str, str], dict[tuple[Any, ...], list[str]]] = {}
    for item in document.get("declarations") or ():
        if item.get("linkage", "") != "external":
            continue
        target = str(item.get("target") or "")
        semantic_id = str(item.get("semantic_id", ""))
        if not semantic_id or semantic_id.startswith("?"):
            continue
        name = re.sub(r"@\d+$", "", semantic_id.lstrip("_@"))
        if not name:
            continue
        signature = (
            item.get("semantic_kind", ""),
            item.get("calling_convention", ""),
            item.get("return_type", ""),
            tuple(item.get("parameter_types") or ()),
        )
        location = f"{item.get('source_file', '?')}:{item.get('line', '?')}"
        functions.setdefault((target, name), {}).setdefault(signature, []).append(location)
    for (target, name), variants in sorted(functions.items()):
        if len(variants) < 2:
            continue
        rendered.append(f"{name} [{target or 'shared'}]")
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    variables: dict[tuple[str, str], dict[tuple[Any, ...], list[str]]] = {}
    for item in document.get("variables") or ():
        if item.get("linkage", "") != "external":
            continue
        target = str(item.get("target") or "")
        semantic_id = str(item.get("semantic_id", ""))
        if not semantic_id:
            continue
        signature = (item.get("type", ""),)
        location = f"{item.get('source_file', '?')}:{item.get('line', '?')}"
        variables.setdefault((target, semantic_id), {}).setdefault(signature, []).append(location)
    for (target, semantic_id), variants in sorted(variables.items()):
        if len(variants) < 2:
            continue
        if _signatures_compatible("variable", list(variants)):
            continue
        rendered.append(f"{semantic_id} [{target or 'shared'}]")
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    if rendered:
        raise SourceIndexError(
            "external symbols have divergent declarations:\n" + "\n".join(rendered)
        )
    return len(functions) + len(variables)


def _guest_host_roots(repository: Path, settings: Settings) -> tuple[tuple[str, str], ...]:
    """Guest mount prefixes used by the lint compile database, longest first."""
    from .build import LINT_BUILD_DIR

    sources = settings.work_dir / "fid" / "sources" / "unpacked"
    pairs = (
        ("/repo", str(repository.resolve())),
        ("/out", str((repository / LINT_BUILD_DIR).resolve())),
        ("/zlib", str((sources / "zlib-1.0.4" / "zlib-1.0.4").resolve())),
        ("/jpeg", str((sources / "ijg-jpeg-6" / "jpeg-6").resolve())),
        ("/infozip", str((sources / "infozip-unzip-5.4").resolve())),
    )
    return tuple(sorted(pairs, key=lambda item: len(item[0]), reverse=True))


def rewrite_compile_token(token: str, roots: tuple[tuple[str, str], ...]) -> str:
    """Map one compile-command token from the analysis-image mounts onto the host."""
    for guest, host in roots:
        trimmed = guest.rstrip("/")
        if token == trimmed:
            return host
        if token.startswith(trimmed + "/"):
            return host + token[len(trimmed) :]
        for flag in _ATTACHED_INCLUDE_FLAGS:
            if token.startswith(flag + trimmed):
                return flag + host + token[len(flag) + len(trimmed) :]
    return token


def rewrite_compile_entry(
    entry: dict[str, Any], roots: tuple[tuple[str, str], ...]
) -> dict[str, Any]:
    """Rewrite one compile-database record onto the host filesystem."""
    rewritten = dict(entry)
    for key in ("file", "directory", "output"):
        if key in rewritten and rewritten[key] is not None:
            rewritten[key] = rewrite_compile_token(str(rewritten[key]), roots)
    arguments = rewritten.get("arguments")
    if isinstance(arguments, list):
        rewritten["arguments"] = [rewrite_compile_token(str(token), roots) for token in arguments]
    elif "command" in rewritten:
        rewritten["command"] = shlex.join(
            rewrite_compile_token(token, roots) for token in shlex.split(str(rewritten["command"]))
        )
    return rewritten


def _reject_unowned_repo_entries(
    entries: list[dict[str, Any]], roots: dict[str, tuple[str, ...]]
) -> None:
    """Fail if a first-party compile entry sits outside every configured source-root."""
    for entry in entries:
        raw = str(entry.get("file", ""))
        if not raw.startswith("/repo/"):
            continue
        candidate = raw.removeprefix("/repo/")
        if any(
            candidate == root or candidate.startswith(root.rstrip("/") + "/")
            for source_roots in roots.values()
            for root in source_roots
        ):
            continue
        if any(
            candidate == root or candidate.startswith(root.rstrip("/") + "/")
            for root in LINT_ONLY_SOURCE_ROOTS
        ):
            continue
        raise SourceIndexError(
            f"compile database entry is outside every configured source-root: {raw}"
        )


def host_compile_database(
    repository: Path, database: Path, settings: Settings, roots: dict[str, tuple[str, ...]]
) -> Path:
    """Materialize a host-path compile database for native reccmp collection.

    The lint CMake projection runs inside the analysis image, so
    ``compile_commands.json`` names ``/repo``, ``/out``, and the vendor mounts.
    reccmp now indexes natively against the repository path it is given, so the
    guest prefixes have to become host paths before collection.
    """
    entries = json.loads(database.read_text(encoding="utf-8"))
    if not isinstance(entries, list):
        raise SourceIndexError(f"{database} is not a compile database")
    _reject_unowned_repo_entries(entries, roots)
    mapping = _guest_host_roots(repository, settings)
    rewritten = [rewrite_compile_entry(entry, mapping) for entry in entries]
    output = repository / "build" / "reccmp-source" / "compile_commands.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    content = json.dumps(rewritten, indent=2) + "\n"
    if not output.is_file() or output.read_text(encoding="utf-8") != content:
        output.write_text(content, encoding="utf-8")
    return output


def _analysis_indexer_binary() -> Path:
    import reccmp.source as source_package

    return Path(source_package.__file__).with_name("indexer.cpp")


def _compile_indexer_locally(source: Path, output: Path) -> None:
    """Compile reccmp's collector using the installed LLVM selection."""
    from reccmp.source.batch import _COMPILE, _pick_library, _run

    config = shutil.which("llvm-config-19")
    include = "/usr/lib/llvm-19/include"
    if config:
        probed = subprocess.run(
            [config, "--includedir"], capture_output=True, text=True, check=False
        )
        if probed.returncode == 0 and probed.stdout.strip():
            include = probed.stdout.strip()
    patterns = (
        "/usr/lib/llvm-19/lib/libclang-cpp.so.*",
        "/usr/lib/x86_64-linux-gnu/libclang-cpp.so.*",
        "/usr/lib/llvm-19/lib/libLLVM*.so*",
        "/usr/lib/x86_64-linux-gnu/libLLVM*.so*",
    )
    hits = [match for pattern in patterns for match in glob.glob(pattern)]
    clang_cpp = _pick_library([hit for hit in hits if "libclang-cpp" in hit])
    llvm = _pick_library([hit for hit in hits if "libclang-cpp" not in hit and "libLLVM" in hit])
    if clang_cpp is None or llvm is None:
        raise SourceIndexError(
            "no LLVM 19 development libraries found for the extended source indexer"
        )
    output.parent.mkdir(parents=True, exist_ok=True)
    _run(
        shlex.split(
            _COMPILE.format(
                include=include,
                clang_cpp=clang_cpp,
                llvm=llvm,
                source=shlex.quote(str(source)),
                output=shlex.quote(str(output)),
            )
        )
    )


def _compile_indexer_in_analysis_image(settings: Settings, source: Path, output: Path) -> None:
    """Build reccmp's Clang indexer once inside the lint image."""
    from .build import VC6_IMAGE
    from .subprocesses import resolve_executable, run

    docker = resolve_executable("docker") or "docker"
    output.parent.mkdir(parents=True, exist_ok=True)
    script = r"""
set -euo pipefail
config=$(command -v llvm-config-19 || command -v llvm-config || true)
include=/usr/lib/llvm-19/include
if [ -n "$config" ]; then
  probed=$("$config" --includedir)
  if [ -n "$probed" ]; then include=$probed; fi
fi
clang_cpp=$(ls /usr/lib/llvm-19/lib/libclang-cpp.so.* /usr/lib/x86_64-linux-gnu/libclang-cpp.so.* 2>/dev/null | tail -n1 || true)
llvm=$(ls /usr/lib/llvm-19/lib/libLLVM.so.* /usr/lib/x86_64-linux-gnu/libLLVM*.so* 2>/dev/null | grep -v libclang-cpp | tail -n1 || true)
if [ -z "$clang_cpp" ] || [ -z "$llvm" ]; then
  echo "no LLVM 19 development libraries in the analysis image" >&2
  exit 1
fi
clang++ -O2 -std=c++17 -fno-rtti -fno-exceptions \
  -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS \
  -I"$include" /src/indexer.cpp -o /out/indexer "$clang_cpp" "$llvm"
"""
    run(
        [
            docker,
            "run",
            "--rm",
            "--network",
            "none",
            *_ANALYSIS_LINUX_TEMP,
            "--volume",
            f"{source}:/src/indexer.cpp:ro",
            "--volume",
            f"{output.parent}:/out",
            "--entrypoint",
            "bash",
            VC6_IMAGE,
            "-lc",
            script,
        ],
        cwd=settings.repo_dir,
        log_path=settings.repo_dir / "build" / "logs" / "source-indexer-compile.json",
    )
    compiled = output.parent / "indexer"
    if compiled != output:
        compiled.replace(output)
    if not output.is_file():
        raise SourceIndexError("the analysis image did not produce a source indexer")


def _prepare_analysis_indexer(settings: Settings, cache: Path) -> None:
    """Point reccmp at an indexer that can see clang-cl and the MSVC headers.

    Collection Python runs on the host. The compile flags still name image
    paths such as ``/opt/msvc6-*`` and ``/usr/bin/clang-cl``, so the indexer
    binary itself has to run in the analysis image unless this process is
    already inside that image.
    """
    cache = cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    source = _analysis_indexer_binary()
    binary = cache / "indexer"
    stamp = cache / "indexer.sha256"
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if not binary.is_file() or not stamp.is_file() or stamp.read_text(encoding="utf-8") != digest:
        if Path("/usr/bin/clang-cl").is_file():
            _compile_indexer_locally(source, binary)
        else:
            _compile_indexer_in_analysis_image(settings, source, binary)
        stamp.write_text(digest, encoding="utf-8")

    if Path("/usr/bin/clang-cl").is_file():
        os.environ["RECCMP_SOURCE_INDEXER"] = str(binary)
        return

    from .build import LINT_BUILD_DIR, VC6_IMAGE, Mount
    from .subprocesses import resolve_executable

    docker = resolve_executable("docker") or "docker"
    repository = settings.repo_dir.resolve()
    sources = settings.work_dir / "fid" / "sources" / "unpacked"
    lint = (repository / LINT_BUILD_DIR).resolve()
    zlib = sources / "zlib-1.0.4" / "zlib-1.0.4"
    jpeg = sources / "ijg-jpeg-6" / "jpeg-6"
    infozip = sources / "infozip-unzip-5.4"
    mounts = (
        Mount(repository, str(repository), read_only=False),
        Mount(lint, str(lint), read_only=False),
        Mount(zlib, str(zlib.resolve())),
        Mount(jpeg, str(jpeg.resolve())),
        Mount(infozip, str(infozip.resolve())),
    )
    # -i: reccmp feeds each persistent collector its jobs over stdin.
    command = [docker, "run", "--rm", "-i", "--network", "none", *_ANALYSIS_LINUX_TEMP]
    for mount in mounts:
        command.extend(("--volume", mount.docker_argument()))
    command.extend(("-e", f"RECCMP_SOURCE_ROOT={repository}"))
    command.extend(("--entrypoint", str(binary), VC6_IMAGE))
    wrapper = cache / "docker-indexer"
    wrapper.write_text(
        f'#!/bin/sh\n# indexer {digest}\nexec {shlex.join(command)} "$@"\n',
        encoding="utf-8",
    )
    wrapper.chmod(0o755)
    os.environ["RECCMP_SOURCE_INDEXER"] = str(wrapper)


def _collect_source_index(
    repository: Path,
    database: Path,
    targets: dict[str, tuple[Path, ...]],
    settings: Settings,
    *,
    force: bool = False,
) -> SourceIndex:
    """Project adapter: host-path compile DB, then one reccmp collection."""
    cache = repository / "build" / "reccmp-source"
    host_database = host_compile_database(
        repository, database, settings, indexed_targets(repository, database)
    )
    _prepare_analysis_indexer(settings, cache)
    return SourceIndex.from_compile_database(
        repository,
        host_database,
        targets,
        clang="/usr/bin/clang-cl",
        # Each worker is one persistent collector (with the Docker wrapper,
        # one container) that takes jobs until none remain.
        jobs=min(8, os.cpu_count() or 1),
        cache_dir=cache,
        force=force,
    )


def _source_index_projections(
    index: SourceIndex,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Header declarations and per-unit repository dependencies, from the index."""
    seen: dict[tuple[str, str], dict[str, Any]] = {}
    records: list[tuple[str, Any, bool]] = [
        ("function", declaration, declaration.is_definition)
        for declaration in index.declarations.values()
    ]
    records.extend(
        ("global", variable, variable.definition_kind != "declaration")
        for variable in index.variables.values()
    )
    for kind, record, defined in records:
        if not record.source_file.startswith("include/wiz8/"):
            continue
        key = (record.source_file, record.semantic_id)
        previous = seen.get(key)
        if previous is None or (defined and not previous["defined"]):
            seen[key] = {
                "source_file": record.source_file,
                "semantic_id": record.semantic_id,
                "qualified_name": record.qualified_name,
                "kind": kind,
                "member": bool(getattr(record, "owning_class", None)),
                "defined": defined,
                "line": record.line,
            }
    header_declarations = [seen[key] for key in sorted(seen)]
    translation_unit_dependencies = [
        {"source_file": unit, "file_dependencies": list(paths)}
        for unit, paths in sorted(index.unit_dependencies.items())
    ]
    return header_declarations, translation_unit_dependencies


def _source_index_result(document: dict[str, Any], *, cached: bool) -> dict[str, Any]:
    return {
        "path": "build/source-index.json",
        "markers": len(document.get("markers") or ()),
        "declarations": len(document.get("declarations") or ()),
        "classes": len(document.get("classes") or ()),
        "variables": len(document.get("variables") or ()),
        "member_uses": len(document["member_uses"]),
        "conflicts": len(document.get("conflicts") or ()),
        "cached": cached,
    }


def write_source_index(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    from .build import LINT_BUILD_DIR, configure_clang

    repository = settings.repo_dir.resolve()
    validate_synthetic_marker_blocks(repository)
    database = repository / LINT_BUILD_DIR / "compile_commands.json"
    index_path = repository / "build/source-index.json"
    stamp = repository / "build/reccmp-source/source-index-inputs.sha256"
    configure_clang(settings)
    if not database.is_file():
        raise FileNotFoundError(f"clang configuration did not produce {database}")
    if not force and index_path.is_file() and stamp.is_file():
        digest = _source_index_input_digest(repository, database)
        if stamp.read_text(encoding="utf-8").strip() == digest:
            validate_cross_tu_declarations(repository)
            return _source_index_result(load_source_index(repository), cached=True)
    roots = indexed_targets(repository, database)
    targets = {
        target: tuple(
            sorted(
                path
                for source_root in source_roots
                for path in (repository / source_root).rglob("*")
                if path.suffix.lower() in _SOURCE_SUFFIXES
            )
        )
        for target, source_roots in roots.items()
    }
    index = _collect_source_index(repository, database, targets, settings, force=force)
    document = index.to_dict()
    header_declarations, dependencies = _source_index_projections(index)
    document["header_declarations"] = header_declarations
    document["translation_unit_dependencies"] = dependencies
    content = json.dumps(document, separators=(",", ":")) + "\n"
    if not index_path.is_file() or index_path.read_bytes() != content.encode("utf-8"):
        index_path.write_bytes(content.encode("utf-8"))
    else:
        # Its mtime records a successful explicit refresh even when the
        # compiler-backed projection is byte-for-byte unchanged.
        index_path.touch()
    validate_cross_tu_declarations(repository)
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(_source_index_input_digest(repository, database) + "\n", encoding="utf-8")
    return _source_index_result(document, cached=False)
