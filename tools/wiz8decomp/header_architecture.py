"""Header role classification against recovered translation-unit ownership.

Every header under ``include/wiz8`` has a role:

- ``shared-layout``: record layouts, constants and shared-state ``extern``
  declarations; no behavioral API. May not include an interface header.
- ``tu-interface``: declarations owned by one original translation unit,
  including member functions and globals.
- ``reconstructed-declarations``: declarations merged from several original
  units; ``implementation-tus`` names the allowed owners.
- ``provisional-interface``: ownership genuinely unresolved because the
  implementing source is an ``unresolved-fragment`` entry in
  ``source_units.json``.
- ``header-implementation``: the header itself carries the implementation
  (template/inline bodies) rather than declaring a TU's interface.
- ``compat-aggregate``: temporary include-only migration façade; it may not
  declare anything and nothing may include it.
- ``bridge``: the SGP C bridge.

Path convention covers ``include/wiz8/layouts/`` (shared-layout) and
``include/wiz8/sgp_bridge.h`` (bridge); other explicit roles come from
``src/wiz8/header_architecture.json``. Without a configured role the role is
inferred from resolved ownership: zero declarations is shared-layout, only
in-header definitions is header-implementation, a single original unit is
tu-interface, and anything else fails as unclassified.

Declaration ownership comes from the compiler-backed source index:
definitions record their defining TU, and declarations never defined are
joined through the marker stream's ``declaration_key`` to a retail address
the assertion layout places. The textual scan only enumerates what each
header declares — functions (free and member) and ``extern`` globals — plus
``// FUNCTION:``/``// GLOBAL:`` marker addresses. Placeholder names that
encode an address (``Function50ABF0``, ``UpdateNpcEvents0050D530``) resolve
through the layout as well.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .ghidra.unit_intervals import TranslationUnitLayout, assertion_anchors, read_assertions
from .paths import atomic_json
from .source_units import (
    CLASSIFICATION_PATH,
    ORIGINAL_TU,
    UNRESOLVED_FRAGMENT,
    SourceUnitError,
    load_source_unit_document,
    mapped_repository_source_file,
    source_unit_records,
)

BRIDGE = "bridge"
SHARED_LAYOUT = "shared-layout"
TU_INTERFACE = "tu-interface"
RECONSTRUCTED = "reconstructed-declarations"
HEADER_IMPLEMENTATION = "header-implementation"
PROVISIONAL_INTERFACE = "provisional-interface"
COMPAT_AGGREGATE = "compat-aggregate"
UNCLASSIFIED = "unclassified"
ROLES = frozenset(
    {
        BRIDGE,
        SHARED_LAYOUT,
        TU_INTERFACE,
        RECONSTRUCTED,
        HEADER_IMPLEMENTATION,
        PROVISIONAL_INTERFACE,
        COMPAT_AGGREGATE,
        UNCLASSIFIED,
    }
)
INTERFACE_ROLES = frozenset({TU_INTERFACE, RECONSTRUCTED, PROVISIONAL_INTERFACE})
PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})
OWNED_ATTRIBUTIONS = PLACED_ATTRIBUTIONS | {"recovered-original-tu"}
ARCHITECTURE_PATH = Path("src/wiz8/header_architecture.json")
HEADER_ROOT = Path("include/wiz8")
BRIDGE_HEADER = "include/wiz8/sgp_bridge.h"
SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
HEADER_SUFFIXES = frozenset({".h", ".hpp"})
SKIP_DIRECTORIES = frozenset({"sgp-compat"})

_FUNCTION_MARKER = re.compile(
    r"^\s*//\s*FUNCTION:\s+(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9a-fA-F]+)\s*$",
    re.IGNORECASE,
)
_GLOBAL_MARKER = re.compile(
    r"^\s*//\s*GLOBAL:\s+(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9a-fA-F]+)\s*$",
    re.IGNORECASE,
)
_ADDRESS = re.compile(r"0x[0-9a-fA-F]{6,8}")
# Placeholder names carry their retail address: Function50ABF0 and the trailing
# 00-prefixed eight digits in UpdateNpcEvents0050D530.
_PLACEHOLDER_ADDRESS = re.compile(r"(?:Function([0-9A-Fa-f]{6})|00([0-9A-Fa-f]{6}))$")
_DECL_NAME = re.compile(
    r"(?P<name>operator[^\s(]*|~?[A-Za-z_]\w*(?:::[A-Za-z_~]\w*)*)"
    r"\s*\((?P<args>[^;{}]*?)\)[^;{]*;"
)
_EXTERN_GLOBAL = re.compile(
    r"^\s*extern\s+(?!\"C\")[^;()]*?\b(?P<name>[A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*;"
)
_SKIP_DECL_PREFIX = re.compile(
    r"^\s*(?:typedef|using|friend|static_assert|enum|struct|class|namespace|#)\b"
)
_RECORD_SCOPE = re.compile(r"\b(?:struct|class)\s+([A-Za-z_]\w*)?[^;{}]*$")
_TRANSPARENT_SCOPE = re.compile(r'^\s*(?:extern\s+"C"|namespace\b)')
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//.*?$", re.MULTILINE)


class HeaderArchitectureError(RuntimeError):
    """A classified header violates its architectural role."""


def _posix(path: Path, repo_dir: Path) -> str:
    return path.relative_to(repo_dir).as_posix()


def load_header_architecture_document(repo_dir: Path) -> dict[str, Any]:
    path = repo_dir / ARCHITECTURE_PATH
    if not path.is_file():
        return {
            "schema": "wiz8.header-architecture-v1",
            "headers": {},
            "proven-original-headers": {},
        }
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "wiz8.header-architecture-v1":
        raise HeaderArchitectureError(f"{ARCHITECTURE_PATH} has an unsupported schema")
    for relative, configured in (document.get("headers") or {}).items():
        role = str((configured or {}).get("role") or "")
        if role not in ROLES:
            raise HeaderArchitectureError(
                f"{ARCHITECTURE_PATH}: {relative} has unknown role {role!r}"
            )
    return document


def _assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    units, headers = assertion_anchors(read_assertions(repo_dir))
    return TranslationUnitLayout(units, header_anchors=headers)


def _strip_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub(" ", text))


def _next_code_line(lines: list[str], start: int) -> str:
    collected: list[str] = []
    for index in range(start, min(start + 8, len(lines))):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith("//"):
            continue
        collected.append(stripped)
        joined = " ".join(collected)
        if ";" in joined or "{" in joined:
            return joined
    return " ".join(collected)


def _declarator_name(text: str) -> str | None:
    cleaned = _strip_comments(text).replace("\n", " ")
    match = _DECL_NAME.search(cleaned)
    if match is None:
        return None
    return match.group("name")


def scan_function_definitions(repo_dir: Path) -> dict[int, dict[str, Any]]:
    """Map FUNCTION marker addresses onto the recovered defining file and name."""

    definitions: dict[int, dict[str, Any]] = {}
    roots = [repo_dir / "src/wiz8", repo_dir / "include/wiz8"]
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in SOURCE_SUFFIXES or not path.is_file():
                continue
            relative = _posix(path, repo_dir)
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for index, line in enumerate(lines):
                marker = _FUNCTION_MARKER.match(line)
                if marker is None:
                    continue
                address = int(marker.group("address"), 16)
                following = _next_code_line(lines, index + 1)
                name = _declarator_name(
                    following + (";" if "(" in following and ";" not in following else "")
                )
                if name is None:
                    fallback = re.search(r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\(", following)
                    name = fallback.group(1) if fallback else ""
                definitions[address] = {
                    "address": address,
                    "name": name,
                    "source_file": relative,
                    "line": index + 1,
                }
    return definitions


def _header_paths(repo_dir: Path) -> list[Path]:
    root = repo_dir / HEADER_ROOT
    if not root.is_dir():
        return []
    paths: list[Path] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in HEADER_SUFFIXES:
            continue
        relative = path.relative_to(root).parts
        if relative and relative[0] in SKIP_DIRECTORIES:
            continue
        paths.append(path)
    return paths


def _code_line(line: str, in_block: bool) -> tuple[str, bool]:
    """Return the compilable span of one source line and the block-comment state."""

    if in_block:
        end = line.find("*/")
        if end < 0:
            return "", True
        line = line[end + 2 :]
        in_block = False
    pieces: list[str] = []
    index = 0
    while index < len(line):
        block = line.find("/*", index)
        slash = line.find("//", index)
        if block < 0 and slash < 0:
            pieces.append(line[index:])
            break
        if slash >= 0 and (block < 0 or slash < block):
            pieces.append(line[index:slash])
            break
        pieces.append(line[index:block])
        end = line.find("*/", block + 2)
        if end < 0:
            return "".join(pieces), True
        index = end + 2
    return "".join(pieces), in_block


_RECORD_KEYWORDS = re.compile(r"\b(?:struct|class)\s+([A-Za-z_]\w*)")
_TEMPLATE_PREFIX = re.compile(r"\btemplate\s*<[^;{}>]*>")
_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')

_SCOPE_RECORD = "record"
_SCOPE_TRANSPARENT = "transparent"
_SCOPE_OPAQUE = "opaque"


def scan_header_function_declarations(path: Path, repo_dir: Path) -> list[dict[str, Any]]:
    """Function declarations and in-header definitions in one header.

    Both namespace-scope declarations and member declarations are collected;
    member names are qualified with the enclosing record (``Class::method``).
    ``member`` marks declarations inside a class body and ``defined`` marks
    entries the header itself emits (in-class or out-of-class bodies), so role
    checks can distinguish a header's interface from the code it owns.
    ``extern "C"`` and ``namespace`` blocks are transparent: their contents
    count as namespace-scope declarations. Function bodies, enums, unions and
    brace initializers are opaque and their contents are ignored.
    """

    relative = _posix(path, repo_dir)
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    declarations: list[dict[str, Any]] = []
    scopes: list[dict[str, Any]] = []
    pending_record: str | None = None
    pending_transparent = False
    pending_address: int | None = None
    buffer = ""
    buffer_line = 0
    in_block = False

    def scope_names() -> str:
        names = [s["name"] for s in scopes if s["kind"] == _SCOPE_RECORD and s["name"]]
        return "::".join(names)

    def flush(defined: bool, member: bool, index: int) -> None:
        nonlocal buffer, pending_address
        head = buffer.split("{", 1)[0]
        text = head + ";" if defined else buffer
        # Function-pointer members such as `void (*frame)(void)` are fields,
        # not declarations of behavior.
        if "(*" in text:
            buffer = ""
            pending_address = None
            return
        name = _declarator_name(text)
        addresses = [int(item, 16) for item in _ADDRESS.findall(buffer)]
        address = pending_address or (addresses[-1] if addresses else None)
        if name is not None:
            params = text.find("(")
            param_count = None
            if params >= 0:
                depth = 0
                end = len(text)
                for pos in range(params, len(text)):
                    if text[pos] == "(":
                        depth += 1
                    elif text[pos] == ")":
                        depth -= 1
                        if depth == 0:
                            end = pos
                            break
                inside = text[params + 1 : end].strip()
                param_count = inside.count(",") + 1 if inside and inside != "void" else 0
            member = member or "::" in name or "::" in head
            if member and "::" not in name:
                prefix = scope_names()
                name = f"{prefix}::{name}" if prefix else name
            declarations.append(
                {
                    "name": name,
                    "address": address,
                    "header": relative,
                    "line": buffer_line or index + 1,
                    "member": member,
                    "defined": defined,
                    "kind": "function",
                    "param_count": param_count,
                }
            )
        buffer = ""
        pending_address = None

    for index, line in enumerate(lines):
        marker = _FUNCTION_MARKER.match(line) or _GLOBAL_MARKER.match(line)
        if marker is not None:
            pending_address = int(marker.group("address"), 16)
        code, in_block = _code_line(line, in_block)
        stripped = code.strip()
        if stripped and pending_address is None:
            # Trailing `/* 0x… */` evidence comments travel with the
            # declaration on the same raw line.
            commented = _ADDRESS.findall(line)
            if commented:
                pending_address = int(commented[-1], 16)

        scope_text = _TEMPLATE_PREFIX.sub(" ", stripped)
        brace_pos = stripped.find("{")
        equals_pos = stripped.find("=")
        record_names = _RECORD_KEYWORDS.findall(scope_text)
        if record_names:
            record_opens = brace_pos >= 0 and (equals_pos < 0 or brace_pos < equals_pos)
            record_continues = brace_pos < 0 and ";" not in stripped
            if record_opens or record_continues:
                pending_record = record_names[-1]
        if _TRANSPARENT_SCOPE.match(stripped):
            pending_transparent = True

        scope = scopes[-1] if scopes else None
        at_member = scope is not None and scope["kind"] == _SCOPE_RECORD
        at_namespace = scope is None or scope["kind"] == _SCOPE_TRANSPARENT
        collectible = (at_member or at_namespace) and pending_record is None

        if collectible:
            if buffer:
                buffer = f"{buffer} {stripped}"
            elif stripped and not stripped.startswith("#"):
                global_decl = _EXTERN_GLOBAL.match(stripped)
                if global_decl is not None:
                    declarations.append(
                        {
                            "name": global_decl.group("name"),
                            "address": pending_address,
                            "header": relative,
                            "line": index + 1,
                            "member": at_member,
                            "defined": False,
                            "kind": "global",
                        }
                    )
                    pending_address = None
                elif _SKIP_DECL_PREFIX.match(stripped):
                    pending_address = None
                elif "(" in stripped and not stripped.startswith("static_assert"):
                    buffer = stripped
                    buffer_line = index + 1

        if buffer:
            if "{" in buffer:
                flush(defined=True, member=at_member, index=index)
            elif ";" in buffer:
                head = buffer.split("(", 1)[0]
                if "=" not in head or "operator" in head:
                    flush(defined=False, member=at_member, index=index)
                else:
                    buffer = ""
                    pending_address = None

        for character in stripped:
            if character == "{":
                if pending_record is not None:
                    scopes.append({"kind": _SCOPE_RECORD, "name": pending_record})
                    pending_record = None
                elif pending_transparent:
                    scopes.append({"kind": _SCOPE_TRANSPARENT, "name": None})
                    pending_transparent = False
                else:
                    scopes.append({"kind": _SCOPE_OPAQUE, "name": None})
            elif character == "}":
                if scopes:
                    scopes.pop()
            elif character == ";":
                pending_record = None
                pending_transparent = False
    return declarations


def _configured_role(relative: str, document: dict[str, Any]) -> tuple[str | None, list[str]]:
    headers = document.get("headers") or {}
    configured = headers.get(relative) or {}
    if configured.get("role"):
        return str(configured["role"]), [
            str(item) for item in configured.get("implementation-tus") or ()
        ]
    if relative == BRIDGE_HEADER:
        return BRIDGE, []
    if relative.startswith("include/wiz8/layouts/"):
        return SHARED_LAYOUT, []
    return None, []


def _infer_role(declared: list[dict[str, Any]], definitions_in_header: int) -> str:
    """Infer the role of a header the document does not configure.

    A header with no declarations or in-header definitions is shared
    representation. A header that only carries in-header definitions is a
    header-implementation. A header whose declarations all resolve to one
    original TU is that unit's interface; unresolved declarations do not veto
    the inference, but a second resolved unit does. Anything else needs an
    explicit role in ``header_architecture.json``.
    """

    if not declared and not definitions_in_header:
        return SHARED_LAYOUT
    if not declared:
        return HEADER_IMPLEMENTATION
    units = {
        str(owner["original_unit"])
        for item in declared
        for owner in item.get("owners", [])
        if owner.get("attribution") in OWNED_ATTRIBUTIONS and owner.get("original_unit")
    }
    return TU_INTERFACE if len(units) == 1 else UNCLASSIFIED


def _original_unit_for_file(repo_dir: Path, source_file: str) -> str | None:
    if not source_file.endswith((".c", ".cpp")):
        return None
    try:
        records = source_unit_records(repo_dir)
    except (SourceUnitError, OSError):
        records = {}
    record = records.get(source_file)
    if record and record.get("class") == ORIGINAL_TU:
        return record.get("original_path")
    return None


def _normalize_qualified(name: str) -> str:
    """Drop template arguments so index names match scanned declarators."""

    return re.sub(r"<[^<>]*>", "", name)


class _IndexContext:
    """Compiler-backed ownership data from ``build/source-index.json``.

    The index keeps one entry per entity: a definition records the TU that
    defines it, and a declaration that has no definition records the header it
    was seen in plus the ``semantic_id`` the marker stream links to a retail
    address. That makes it the ownership oracle for functions and globals
    alike, including member functions the textual scanner enumerates.
    """

    def __init__(self, index: dict[str, Any]):
        self.definition_units: dict[str, set[str]] = {}
        self.declaration_semantics: dict[str, dict[str, str]] = {}
        self.variable_units: dict[str, set[str]] = {}
        self.header_definitions: dict[str, set[str]] = {}
        marker_addresses = {
            str(marker["declaration_key"][1]): int(marker["address"])
            for marker in index.get("markers") or []
            if marker.get("declaration_key") and marker.get("address") is not None
        }
        self.definition_params: dict[str, dict[str, set[int]]] = {}
        for entry in index.get("declarations") or []:
            name = _normalize_qualified(str(entry.get("qualified_name") or ""))
            unit = str(entry.get("unit_id") or "")
            source = str(entry.get("source_file") or "")
            if entry.get("is_definition"):
                if name and source.endswith((".h", ".hpp")):
                    self.header_definitions.setdefault(name, set()).add(source)
                elif name and unit:
                    self.definition_units.setdefault(name, set()).add(unit)
                    params = entry.get("parameter_types")
                    if isinstance(params, list):
                        self.definition_params.setdefault(name, {}).setdefault(unit, set()).add(
                            len(params)
                        )
            else:
                semantic = str(entry.get("semantic_id") or "")
                if name and source:
                    self.declaration_semantics.setdefault(source, {})[name] = semantic
        for variable in index.get("variables") or []:
            if variable.get("definition_kind") != "definition":
                continue
            name = str(variable.get("qualified_name") or "")
            unit = str(variable.get("unit_id") or "")
            source = str(variable.get("source_file") or "")
            if name and unit and not source.endswith((".h", ".hpp")):
                self.variable_units.setdefault(name, set()).add(unit)
        self.marker_addresses = marker_addresses

    def defined_in_header(self, source_file: str, name: str) -> bool:
        """Whether the index saw this entity's definition inside a header.

        The compiler emits one definition record per TU that instantiates an
        inline/template; ``source_file`` still points at the header carrying
        the body. Those entities are header-implemented, not TU-owned.
        """

        normalized = _normalize_qualified(name)
        return source_file in self.header_definitions.get(normalized, set())

    def address_for_declaration(self, source_file: str, name: str) -> int | None:
        semantic = self.declaration_semantics.get(source_file, {}).get(name)
        if semantic is None:
            return None
        return self.marker_addresses.get(semantic)


def _load_index_context(repo_dir: Path) -> _IndexContext | None:
    from reccmp.source import SourceIndexError

    try:
        from .source_index import load_source_index

        return _IndexContext(load_source_index(repo_dir))
    except (SourceIndexError, OSError, ValueError):
        return None


def _unit_owner(repo_dir: Path, source_file: str) -> dict[str, Any]:
    unit = _original_unit_for_file(repo_dir, source_file)
    if unit:
        return {
            "original_unit": unit,
            "attribution": "recovered-original-tu",
            "source_file": source_file,
        }
    return {
        "original_unit": source_file,
        "attribution": "recovered-file",
        "source_file": source_file,
    }


def _decl_owners(
    layout: TranslationUnitLayout,
    repo_dir: Path,
    declaration: dict[str, Any],
    by_name: dict[str, list[dict[str, Any]]],
    by_tail: dict[str, list[dict[str, Any]]],
    index: _IndexContext | None,
) -> list[dict[str, Any]]:
    """Resolve every unit a declaration could belong to.

    Retail placement wins: an explicit ``// FUNCTION:``/``// GLOBAL:`` marker,
    a marker the source index links to this declaration, or the address the
    placeholder name encodes all bind the decl to the original TU the
    assertion layout reports. Otherwise the recovered definition's file —
    from the source index when available, else the marker scan — is mapped
    back to its original TU.
    """

    name = str(declaration.get("name") or "")
    address = declaration.get("address")
    if not isinstance(address, int):
        embedded = _PLACEHOLDER_ADDRESS.search(name)
        if embedded is not None:
            digits = next(group for group in embedded.groups() if group)
            address = int(digits, 16)
    if not isinstance(address, int) and index is not None:
        address = index.address_for_declaration(str(declaration.get("header") or ""), name)
    if isinstance(address, int):
        owner = layout.owner(address)
        attribution = str(owner.get("attribution") or "")
        unit = str(owner.get("source_path") or "")
        if attribution in PLACED_ATTRIBUTIONS and unit:
            return [{"original_unit": unit, "attribution": attribution, "source_file": ""}]

    units: set[str] = set()
    if index is not None:
        pool = (
            index.variable_units if declaration.get("kind") == "global" else index.definition_units
        )
        units |= pool.get(name, set()) | pool.get(_normalize_qualified(name), set())
        if len(units) > 1:
            # Overloads and same-named functions in different TUs collapse to
            # one index name; narrow by declared parameter count when known.
            param_count = declaration.get("param_count")
            by_params = index.definition_params.get(name) or index.definition_params.get(
                _normalize_qualified(name)
            )
            if param_count is not None and by_params:
                narrowed = {unit for unit, counts in by_params.items() if param_count in counts}
                if narrowed:
                    units = narrowed
    if not units:
        matches = by_name.get(name, [])
        if len(matches) != 1 and "::" in name:
            tail = name.rsplit("::", 1)[-1]
            candidates = by_tail.get(tail, [])
            if len(candidates) > 1:
                prefix = name.rsplit("::", 1)[0]
                narrowed = [
                    item
                    for item in candidates
                    if str(item["name"]).startswith(f"{prefix}::") or str(item["name"]) == tail
                ]
                if narrowed:
                    candidates = narrowed
            if len(candidates) == 1:
                matches = candidates
        units = {str(item["source_file"]) for item in matches if item.get("source_file")}
    owners = [_unit_owner(repo_dir, unit) for unit in sorted(units)]
    return owners or [{"original_unit": "", "attribution": "unknown", "source_file": ""}]


def _definitions_by_name(definitions: dict[int, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    by_name: dict[str, list[dict[str, Any]]] = {}
    for item in definitions.values():
        name = str(item.get("name") or "")
        if name:
            by_name.setdefault(name, []).append(item)
    return by_name


def _definitions_by_tail(definitions: dict[int, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    by_tail: dict[str, list[dict[str, Any]]] = {}
    for item in definitions.values():
        name = str(item.get("name") or "")
        if name:
            by_tail.setdefault(name.rsplit("::", 1)[-1], []).append(item)
    return by_tail


def _header_includes(path: Path) -> list[str]:
    return [
        match.group(1)
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines()
        if (match := _INCLUDE.match(line))
    ]


def _resolve_include(
    relative: str, include: str, scanned: dict[str, list[dict[str, Any]]]
) -> str | None:
    """Resolve a quoted include to a scanned ``include/wiz8`` header."""

    candidates = [
        (Path(relative).parent / include).as_posix(),
        (Path("include") / include).as_posix(),
        (HEADER_ROOT / include).as_posix(),
    ]
    for candidate in candidates:
        if candidate in scanned:
            return candidate
    return None


def _unresolved_fragments(repo_dir: Path) -> set[str]:
    try:
        document = load_source_unit_document(repo_dir)
    except SourceUnitError:
        return set()
    return {str(item) for item in document.get("unresolved-fragment") or ()}


def _source_includes(repo_dir: Path) -> dict[str, list[str]]:
    """Map every recovered source file to its quoted includes."""

    consumers: dict[str, list[str]] = {}
    for root in (repo_dir / "src/wiz8", repo_dir / "include/wiz8"):
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in SOURCE_SUFFIXES or not path.is_file():
                continue
            relative = _posix(path, repo_dir)
            if relative.startswith("include/wiz8/") and relative.split("/")[2] in SKIP_DIRECTORIES:
                continue
            consumers[relative] = _header_includes(path)
    return consumers


def analyze_header_architecture(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> dict[str, Any]:
    document = load_header_architecture_document(repo_dir)
    if layout is None:
        layout = _assertion_layout(repo_dir)
    definitions = scan_function_definitions(repo_dir)
    by_name = _definitions_by_name(definitions)
    by_tail = _definitions_by_tail(definitions)
    headers: list[dict[str, Any]] = []
    violations: list[dict[str, Any]] = []

    proven = document.get("proven-original-headers") or {}
    for relative, original in proven.items():
        path = repo_dir / relative
        stem = Path(relative)
        if stem.suffix.lower() == ".hpp":
            legacy = stem.with_suffix(".h")
            if (repo_dir / legacy).is_file() and not path.is_file():
                violations.append(
                    {
                        "kind": "proven-header-spelling",
                        "file": legacy.as_posix(),
                        "detail": (
                            f"{legacy.as_posix()} is the proven original header "
                            f"{original}; rename it to {relative}"
                        ),
                    }
                )
        if path.suffix.lower() != ".hpp" and str(original).lower().endswith(".hpp"):
            violations.append(
                {
                    "kind": "proven-header-spelling",
                    "file": relative,
                    "detail": f"{relative} is proven as {original}",
                }
            )

    index = _load_index_context(repo_dir)
    scanned: dict[str, list[dict[str, Any]]] = {}
    for path in _header_paths(repo_dir):
        scanned[_posix(path, repo_dir)] = scan_header_function_declarations(path, repo_dir)

    unresolved_fragments = _unresolved_fragments(repo_dir)
    include_consumers = _source_includes(repo_dir)

    declared_by_header: dict[str, list[dict[str, Any]]] = {}
    defined_counts: dict[str, int] = {}
    header_roles: dict[str, str] = {}
    configured_roles: dict[str, str] = {}
    implementation_map: dict[str, list[str]] = {}
    header_units: dict[str, dict[str, int]] = {}

    for relative, declarations in scanned.items():
        configured_role, implementation_tus = _configured_role(relative, document)
        units: dict[str, int] = {}
        declared: list[dict[str, Any]] = []
        defined_in_header = 0
        for declaration in declarations:
            if declaration.get("defined") or (
                index is not None
                and index.defined_in_header(relative, str(declaration.get("name") or ""))
            ):
                declaration["defined"] = True
                defined_in_header += 1
                continue
            owners = _decl_owners(layout, repo_dir, declaration, by_name, by_tail, index)
            declaration["resolved_units"] = sorted(
                {str(owner["original_unit"]) for owner in owners if owner.get("original_unit")}
            )
            declaration["owners"] = owners
            for owner in owners:
                unit = str(owner.get("original_unit") or "")
                if unit and owner.get("attribution") in OWNED_ATTRIBUTIONS | {"recovered-file"}:
                    units[unit] = units.get(unit, 0) + 1
            declared.append(declaration)
        role = configured_role or _infer_role(declared, defined_in_header)
        declared_by_header[relative] = declared
        defined_counts[relative] = defined_in_header
        header_roles[relative] = role
        configured_roles[relative] = configured_role or ""
        implementation_map[relative] = implementation_tus
        header_units[relative] = units

    for relative, declarations in scanned.items():
        path = repo_dir / relative
        role = header_roles[relative]
        implementation_tus = implementation_map[relative]
        units = header_units[relative]
        declared = declared_by_header[relative]
        original_placed = sorted(
            {
                str(owner["original_unit"])
                for item in declared
                for owner in item.get("owners", [])
                if owner.get("attribution") in OWNED_ATTRIBUTIONS and owner.get("original_unit")
            }
        )
        record = {
            "file": relative,
            "role": role,
            "configured": bool(configured_roles[relative]),
            "implementation_tus": implementation_tus,
            "functions": sum(1 for item in declarations if item["kind"] == "function"),
            "globals": sum(1 for item in declarations if item["kind"] == "global"),
            "original_units": original_placed,
            "all_units": sorted(units),
        }
        headers.append(record)

        if role == UNCLASSIFIED:
            violations.append(
                {
                    "kind": "unclassified-header",
                    "file": relative,
                    "detail": (
                        f"{relative} has no resolvable role ({len(declared)} declarations"
                        f" across {sorted(units) or 'no'} units); classify it in "
                        f"{ARCHITECTURE_PATH}"
                    ),
                    "original_units": sorted(units),
                }
            )
        if role == SHARED_LAYOUT and declarations:
            functions = [item for item in declarations if item["kind"] == "function"]
            if functions:
                violations.append(
                    {
                        "kind": "layout-declares-functions",
                        "file": relative,
                        "detail": (
                            f"{relative} is shared-layout but declares or defines "
                            f"{len(functions)} function(s)"
                        ),
                        "functions": [item["name"] for item in functions[:8]],
                    }
                )
        if role == SHARED_LAYOUT:
            for include in _header_includes(path):
                target = _resolve_include(relative, include, scanned)
                if target is None:
                    continue
                target_role = header_roles.get(target)
                exposed = [
                    item["name"]
                    for item in scanned[target]
                    if item["kind"] == "function" and not item["member"] and not item.get("defined")
                ]
                # A layout may depend on a type that carries member functions —
                # a by-value field needs the complete class — and may see
                # shared-state externs, but must not pull in namespace-scope
                # function API through the include.
                if target_role == COMPAT_AGGREGATE or exposed:
                    violations.append(
                        {
                            "kind": "layout-includes-interface",
                            "file": relative,
                            "detail": (
                                f"{relative} includes {target}, which declares "
                                f"{len(exposed)} namespace-scope declaration(s)"
                            ),
                            "include": target,
                            "functions": exposed[:8],
                        }
                    )
        if role == TU_INTERFACE and not implementation_tus and len(original_placed) > 1:
            violations.append(
                {
                    "kind": "tu-interface-mixed-units",
                    "file": relative,
                    "detail": (
                        f"{relative} is tu-interface but declares entities from "
                        + ", ".join(original_placed)
                    ),
                    "original_units": original_placed,
                }
            )
        if role in INTERFACE_ROLES and implementation_tus:
            allowed = {item.casefold() for item in implementation_tus}
            foreign_units = {
                str(owner["original_unit"])
                for item in declared
                for owner in item.get("owners", [])
                if owner.get("attribution") in OWNED_ATTRIBUTIONS
                and owner.get("original_unit")
                and str(owner["original_unit"]).casefold() not in allowed
            }
            if foreign_units:
                units_found = sorted(foreign_units)
                violations.append(
                    {
                        "kind": f"{role}-foreign-unit",
                        "file": relative,
                        "detail": (
                            f"{relative} lists implementation TUs "
                            f"{implementation_tus} but declares {units_found}"
                        ),
                        "original_units": units_found,
                    }
                )
        if role == PROVISIONAL_INTERFACE:
            placed = [
                item["name"]
                for item in declared
                for owner in item.get("owners", [])
                if owner.get("attribution") in PLACED_ATTRIBUTIONS
                or (
                    owner.get("attribution") in OWNED_ATTRIBUTIONS | {"recovered-file"}
                    and owner.get("source_file")
                    and str(owner["source_file"]) not in unresolved_fragments
                )
            ]
            if placed:
                violations.append(
                    {
                        "kind": "provisional-interface-resolved",
                        "file": relative,
                        "detail": (
                            f"{relative} is provisional-interface but "
                            f"{len(placed)} declaration(s) resolve to known "
                            "units; use an explicit role instead"
                        ),
                        "functions": placed[:8],
                    }
                )
        if role == COMPAT_AGGREGATE and declarations:
            violations.append(
                {
                    "kind": "compat-aggregate-declares",
                    "file": relative,
                    "detail": (
                        f"{relative} is compat-aggregate but carries "
                        f"{len(declarations)} declaration(s); aggregates are "
                        "include-only façades"
                    ),
                }
            )
        if role == BRIDGE and relative != BRIDGE_HEADER:
            violations.append(
                {
                    "kind": "unexpected-bridge",
                    "file": relative,
                    "detail": f"{relative} is classified as bridge; only {BRIDGE_HEADER} is",
                }
            )

    forbidden = {str(item) for item in document.get("forbidden-includes") or ()}
    aggregate_headers = {file for file, role in header_roles.items() if role == COMPAT_AGGREGATE}
    if forbidden:
        for consumer, includes in include_consumers.items():
            for include in includes:
                target = _resolve_include(consumer, include, scanned)
                if target is None:
                    target = include if include.startswith("include/") else f"include/{include}"
                if target in forbidden:
                    violations.append(
                        {
                            "kind": "forbidden-include",
                            "file": consumer,
                            "detail": f"{consumer} includes forbidden header {include}",
                            "include": include,
                        }
                    )
    if aggregate_headers:
        for consumer, includes in include_consumers.items():
            for include in includes:
                target = _resolve_include(consumer, include, scanned)
                if target in aggregate_headers:
                    violations.append(
                        {
                            "kind": "compat-aggregate-consumed",
                            "file": consumer,
                            "detail": f"{consumer} still includes compat aggregate {target}",
                            "include": target,
                        }
                    )

    fragments = _fragment_ownership(repo_dir, layout, definitions)
    report = {
        "schema": "wiz8.header-architecture-report-v1",
        "headers": headers,
        "violations": violations,
        "unclassified_mixed": [
            item
            for item in headers
            if item["role"] == UNCLASSIFIED and len(item["original_units"]) > 1
        ],
        "unresolved_fragments": fragments,
    }
    return report


def _fragment_ownership(
    repo_dir: Path,
    layout: TranslationUnitLayout,
    definitions: dict[int, dict[str, Any]],
) -> list[dict[str, Any]]:
    if not (repo_dir / CLASSIFICATION_PATH).is_file():
        return []
    try:
        records = source_unit_records(repo_dir)
    except (SourceUnitError, OSError):
        return []
    unresolved = {
        path for path, record in records.items() if record["class"] == UNRESOLVED_FRAGMENT
    }
    rows: list[dict[str, Any]] = []
    for path in sorted(unresolved):
        owned = [item for item in definitions.values() if item["source_file"] == path]
        units: dict[str, int] = {}
        unknown = 0
        for item in owned:
            owner = layout.owner(int(item["address"]))
            if str(owner.get("attribution") or "") in PLACED_ATTRIBUTIONS and owner.get(
                "source_path"
            ):
                unit = str(owner["source_path"])
                units[unit] = units.get(unit, 0) + 1
            else:
                unknown += 1
        suggestion = "keep-fragment"
        if owned and not unknown and len(units) == 1:
            suggestion = "merge"
        elif owned and len(units) > 1:
            suggestion = "split"
        rows.append(
            {
                "file": path,
                "functions": len(owned),
                "original_units": sorted(units),
                "unplaced": unknown,
                "suggestion": suggestion,
            }
        )
    return rows


def header_architecture_violations(
    repo_dir: Path, layout: TranslationUnitLayout | None = None
) -> list[dict[str, Any]]:
    return list(analyze_header_architecture(repo_dir, layout=layout)["violations"])


def validate_header_architecture(repo_dir: Path) -> dict[str, Any]:
    report = analyze_header_architecture(repo_dir)
    destination = repo_dir / "build/reports/header-architecture/report.json"
    atomic_json(destination, report)
    violations = list(report["violations"])
    if violations:
        rendered = [f"{item['file']}: {item['kind']}: {item['detail']}" for item in violations]
        raise HeaderArchitectureError("header architecture failed:\n  " + "\n  ".join(rendered))
    return {
        "ok": True,
        "gate": "header-architecture",
        "headers": len(report["headers"]),
        "unclassified_mixed": len(report["unclassified_mixed"]),
        "report": str(destination.relative_to(repo_dir)),
    }


def write_header_architecture_report(repo_dir: Path) -> dict[str, Any]:
    """Write the ownership report even when the gate would fail."""

    report = analyze_header_architecture(repo_dir)
    destination = repo_dir / "build/reports/header-architecture/report.json"
    atomic_json(destination, report)
    return {
        "schema": "wiz8.header-architecture-summary",
        "headers": len(report["headers"]),
        "violations": len(report["violations"]),
        "unclassified_mixed": len(report["unclassified_mixed"]),
        "unresolved_fragments": len(report["unresolved_fragments"]),
        "mergeable_fragments": sum(
            1 for item in report["unresolved_fragments"] if item["suggestion"] == "merge"
        ),
        "report": str(destination.relative_to(repo_dir)),
        "violation_kinds": sorted({item["kind"] for item in report["violations"]}),
    }


def expected_recovered_source(repo_dir: Path, unit: str) -> str | None:
    """Expose original-TU mapping for callers that rehome fragments."""

    return mapped_repository_source_file(repo_dir, unit)
