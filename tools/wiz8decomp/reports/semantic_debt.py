"""Rank non-gating recovery debt from the existing source model."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from ..ghidra.unit_intervals import TranslationUnitLayout, assertion_anchors, read_assertions
from ..source_index import load_source_index, source_functions
from ..source_units import UNMAPPED_SOURCE, source_unit_records

_PLACEHOLDER = re.compile(r"^Function[0-9A-Fa-f]{6,9}$")
_ADDRESS_SUFFIX = re.compile(r"[A-Za-z_][A-Za-z0-9_:<>]*[0-9][0-9A-Fa-f]{5,7}$")
_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})

_STALE_CLAIM = re.compile(
    r"unrecovered|unported|not\s+yet\s+identified|not\s+yet\s+ported|"
    r"not\s+yet\s+recovered|not\s+identified|body\s+is\s+not\s+ported|"
    r"unresolved\s+at\s+link|stays\s+unresolved|named\s+by\s+address|"
    r"identity\s+ceiling",
    re.IGNORECASE,
)
_EXPLICIT_ADDRESS = re.compile(r"\b0x([0-9A-Fa-f]{6,8})\b")
_ADDRESS_SUFFIXED_TOKEN = re.compile(r"\b[A-Za-z_][A-Za-z0-9_:<>]*[0-9A-Fa-f]{6,8}\b")
_TRAILING_HEX = re.compile(r"([0-9A-Fa-f]{6,8})$")
_FIELD_FLOW_CATEGORY_LIMIT = 64

_SOURCE_SHAPING_PATTERNS = (
    ("forceinline", re.compile(r"\b__forceinline\b")),
    ("noinline", re.compile(r"\b__declspec\s*\(\s*noinline\s*\)")),
    (
        "optimizer_pragma",
        re.compile(
            r"^\s*#\s*pragma\s+(?:optimize|inline_depth|inline_recursion|auto_inline)\b",
            re.IGNORECASE,
        ),
    ),
)
_SOURCE_ROOTS = {
    "WIZ8": ("src/wiz8", "include/wiz8"),
    "SURRENDER": ("src/surrender", "include/surrender"),
}


def _source_shaping_directives(repository: Path, target: str) -> list[dict[str, Any]]:
    """Return compiler controls that may encode codegen instead of authored design."""

    rows: list[dict[str, Any]] = []
    for root_name in _SOURCE_ROOTS.get(target.upper(), ()):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            relative = str(path.relative_to(repository))
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                for kind, pattern in _SOURCE_SHAPING_PATTERNS:
                    if not pattern.search(line):
                        continue
                    rows.append(
                        {
                            "source_file": relative,
                            "line": line_number,
                            "kind": kind,
                            "spelling": line.strip(),
                            "status": "investigation_candidate",
                            "reason": (
                                "compiler source-shaping must be justified by authored-source "
                                "evidence, not comparison score"
                            ),
                        }
                    )
                    break
    return rows


_COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
_STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')
_IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
_ADDRESS_STORAGE = re.compile(r"(?:^|_)(?:0x)?[0-9A-Fa-f]{5,8}$")
_OFFSET_STORAGE = re.compile(
    r"^(?:m_)?(?:field|offset|off|dword|word|byte|flag|float|ptr|value)_[0-9A-Fa-f]{1,4}$"
)
_UNKNOWN_STORAGE = re.compile(r"^(?:m_)?(?:unknown|unk)_[0-9A-Fa-f]+")
_PADDING_STORAGE = re.compile(r"^(?:m_)?(?:padding|pad)_[0-9A-Fa-f]+")
_WRITE_AFTER = r"\s*(?:[-+*/%&|^]|<<|>>)?=(?!=)|\s*(?:\+\+|--)"
_WRITE_BEFORE = r"(?:\+\+|--)\s*(?:[A-Za-z_][A-Za-z0-9_]*\s*(?:->|\.)\s*)*"
_INITIALIZER = r"[:,]\s*"
_CAST_ESCAPES = ("reinterpret-ok", "c-style-cast-ok")


def _target_sources(repository: Path, target: str) -> dict[str, str]:
    sources: dict[str, str] = {}
    for root_name in _SOURCE_ROOTS.get(target.upper(), ()):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix.casefold() in _SOURCE_SUFFIXES:
                sources[path.relative_to(repository).as_posix()] = path.read_text(
                    encoding="utf-8", errors="replace"
                )
    return sources


_LAYOUT_ASSERTION = re.compile(
    r"^.*\b(?:static_assert|offsetof|W8_ASSERT_\w*OFFSET)\b.*$", re.MULTILINE
)


def _code_only(text: str) -> str:
    """Comment- and string-free code, without layout assertions."""

    return _LAYOUT_ASSERTION.sub("", _STRING.sub('""', _COMMENT.sub(" ", text)))


class _Usage:
    """Name-level identifier occurrences in comment-free selected-product source.

    Occurrence counts are by spelling: distinct records sharing one member
    name are aggregated, so counts bound rather than attribute usage."""

    def __init__(self, sources: dict[str, str]):
        self.code = {path: _code_only(text) for path, text in sources.items()}
        self.counts: dict[str, dict[str, int]] = {}
        for path, code in self.code.items():
            for match in _IDENTIFIER.finditer(code):
                per_file = self.counts.setdefault(match.group(0), {})
                per_file[path] = per_file.get(path, 0) + 1

    def __call__(self, name: str, declarations: int) -> dict[str, Any]:
        per_file = self.counts.get(name, {})
        token = re.escape(name)
        writes = re.compile(
            rf"\b{token}\b(?:\s*\[[^\]]*\])*(?:{_WRITE_AFTER})|{_WRITE_BEFORE}\b{token}\b|"
            rf"{_INITIALIZER}\b{token}\s*\("
        )
        write_count = sum(len(writes.findall(self.code[path])) for path in per_file)
        references = max(sum(per_file.values()) - declarations, 0)
        return {
            "references": references,
            "writes": min(write_count, references),
            "reads": max(references - write_count, 0),
            "source_files": sorted(per_file),
        }


def _storage_kind(name: str) -> str | None:
    leaf = name.rsplit("::", 1)[-1]
    if _PADDING_STORAGE.match(leaf):
        return "padding"
    if _UNKNOWN_STORAGE.match(leaf):
        return "unknown"
    if _OFFSET_STORAGE.match(leaf):
        return "offset"
    if _ADDRESS_STORAGE.search(leaf):
        return "address"
    return None


def _target_path(path: str, target: str) -> bool:
    return path.startswith(tuple(f"{root}/" for root in _SOURCE_ROOTS.get(target.upper(), ())))


def _storage_debt(
    index: dict[str, Any], usage: _Usage, target: str = "WIZ8"
) -> dict[str, list[dict[str, Any]]]:
    """Globals and members whose identifiers still encode address/offset/unknown."""

    globals_by_name: dict[str, dict[str, Any]] = {}
    for variable in index.get("variables", []):
        name = str(variable.get("qualified_name") or "")
        path = str(variable.get("source_file") or "")
        kind = _storage_kind(name)
        if variable.get("target") != target.upper() or "::" in name or kind is None:
            continue
        if not _target_path(path, target):
            continue
        row = globals_by_name.setdefault(
            name,
            {"name": name, "kind": kind, "type": variable.get("type"), "declarations": []},
        )
        location = f"{path}:{variable.get('line')}"
        if location not in row["declarations"]:
            row["declarations"].append(location)
    global_rows = [
        {**row, **usage(row["name"], len(row["declarations"]))} for row in globals_by_name.values()
    ]

    members: dict[str, dict[str, Any]] = {}
    seen_records: set[str] = set()
    for record in index.get("classes", []):
        record_name = str(record.get("qualified_name") or "")
        if record_name in seen_records:
            continue
        seen_records.add(record_name)
        for field in record.get("fields", []):
            name = str(field.get("name") or "")
            path = str(field.get("source_file") or "")
            kind = _storage_kind(name)
            if kind is None or not _target_path(path, target):
                continue
            row = members.setdefault(name, {"name": name, "kind": kind, "fields": []})
            row["fields"].append(
                {
                    "record": record_name,
                    "offset": field.get("offset"),
                    "type": field.get("type"),
                    "location": f"{path}:{field.get('line')}",
                }
            )
    # This index has declarations, not receiver bindings for token occurrences.
    # Even a unique indexed field may share a spelling with a local or vendor
    # field. Never present these aggregate counts as owner-filtered accesses.
    member_rows = [
        {
            **row,
            **usage(row["name"], len(row["fields"])),
            "usage_scope": "identifier_spelling",
            "receiver_verified": False,
            "possible_nonmember_matches": True,
        }
        for row in members.values()
    ]
    padding_accessed = [
        row for row in member_rows if row["kind"] == "padding" and row["references"] > 0
    ]
    order = lambda row: (-row["references"], -row["writes"], row["name"])
    return {
        "address_named_globals": sorted(global_rows, key=order),
        "address_named_members": sorted(
            (row for row in member_rows if row["kind"] != "padding"), key=order
        ),
        "accessed_padding_members": sorted(padding_accessed, key=order),
    }


def _void_storage(index: dict[str, Any], target: str = "WIZ8") -> list[dict[str, Any]]:
    """Project-owned members typed ``void*``: candidate typed holes."""

    rows: list[dict[str, Any]] = []
    seen: set[str] = set()
    for record in index.get("classes", []):
        record_name = str(record.get("qualified_name") or "")
        if record_name in seen:
            continue
        seen.add(record_name)
        for field in record.get("fields", []):
            path = str(field.get("source_file") or "")
            spelling = re.sub(r"\s+", "", str(field.get("type") or ""))
            if not _target_path(path, target) or not spelling.startswith("void*"):
                continue
            rows.append(
                {
                    "record": record_name,
                    "field": field.get("name"),
                    "type": field.get("type"),
                    "offset": field.get("offset"),
                    "location": f"{path}:{field.get('line')}",
                }
            )
    rows.sort(key=lambda row: (row["location"], row["field"] or ""))
    return rows


_REINTERPRET_TARGET = re.compile(r"\breinterpret_cast<\s*(?:const\s+)?([A-Za-z_][\w:]*)")


def _cast_escapes(sources: dict[str, str], owned_types: set[str]) -> list[dict[str, Any]]:
    """Files ranked by reviewed cast escapes, owned-type reinterpretations first.

    A reinterpret_cast whose target is a repository-owned record is the
    strongest type-model candidate: both representations are ours to fix."""

    rows = []
    for path, text in sources.items():
        counts = {marker: text.count(marker) for marker in _CAST_ESCAPES}
        total = sum(counts.values())
        if not total:
            continue
        owned = sorted(
            {name for name in _REINTERPRET_TARGET.findall(_code_only(text)) if name in owned_types}
        )
        rows.append(
            {
                "source_file": path,
                "total": total,
                **counts,
                "owned_reinterpret_targets": owned,
            }
        )
    rows.sort(
        key=lambda row: (-len(row["owned_reinterpret_targets"]), -row["total"], row["source_file"])
    )
    return rows


_ENUM_PARAMETER = re.compile(r"^enum\s+([A-Za-z_][\w:]*)$")
_INTEGER_LITERAL = re.compile(r"^(?:0[xX][0-9A-Fa-f]+|\d+)$")


def _enum_literal_arguments(index: dict[str, Any], sources: dict[str, str]) -> list[dict[str, Any]]:
    """Integer literals passed where the callee's declared parameter is an enum."""

    enum_positions: dict[str, set[int]] = {}
    plain_positions: dict[str, set[int]] = {}
    for item in index.get("declarations", []):
        name = str(item.get("qualified_name") or "").rpartition("::")[2]
        for position, parameter in enumerate(item.get("parameter_types") or []):
            positions = enum_positions if _ENUM_PARAMETER.match(str(parameter)) else plain_positions
            positions.setdefault(name, set()).add(position)
    # Calls are matched by unqualified name, so a position that is an enum in
    # one same-named declaration and plain in another is ambiguous.
    for name in list(enum_positions):
        enum_positions[name] -= plain_positions.get(name, set())
        if not enum_positions[name]:
            del enum_positions[name]
    if not enum_positions:
        return []
    call = re.compile(r"\b(" + "|".join(map(re.escape, sorted(enum_positions))) + r")\s*\(")
    rows: list[dict[str, Any]] = []
    for path, text in sources.items():
        code = _code_only(text)
        for match in call.finditer(code):
            depth, start, arguments = 1, match.end(), []
            position = start
            while position < len(code) and depth:
                char = code[position]
                if char in "([{":
                    depth += 1
                elif char in ")]}":
                    depth -= 1
                if (char == "," and depth == 1) or depth == 0:
                    arguments.append(code[start:position].strip())
                    start = position + 1
                position += 1
            for argument_index in enum_positions[match.group(1)]:
                if argument_index < len(arguments) and _INTEGER_LITERAL.match(
                    arguments[argument_index]
                ):
                    rows.append(
                        {
                            "location": f"{path}:{code.count(chr(10), 0, match.start()) + 1}",
                            "callee": match.group(1),
                            "argument": argument_index,
                            "literal": arguments[argument_index],
                        }
                    )
    rows.sort(key=lambda row: (row["callee"], row["location"]))
    return rows


_NARRATION_EVIDENCE = re.compile(
    r"0x[0-9A-Fa-f]{4,}|retail|bug|\bUB\b|undefined|evidence|unresolved|owner|lifetime"
    r"|ABI|layout|assert|TODO|unknown|why|because|despite|emits?|inlin|ICF|thunk",
    re.IGNORECASE,
)
_FUNCTION_HEAD = re.compile(r"^[A-Za-z_][\w:<>,*&~ ]*\([^;]*\)\s*(?:const\s*)?\{?\s*$")
_NARRATION_MIN_COMMENT_LINES = 3
_NARRATION_MAX_BODY_LINES = 12


def _narration_candidates(sources: dict[str, str]) -> list[dict[str, Any]]:
    """Multi-line comments over short functions that cite no evidence.

    These usually paraphrase the visible body; comments that mention retail
    behaviour, addresses, ownership, layout or unresolved decisions are kept
    out of the queue."""

    rows: list[dict[str, Any]] = []
    for path, text in sources.items():
        if not path.endswith((".c", ".cpp")):
            continue
        lines = text.splitlines()
        for start, end, comment in _comment_regions(lines):
            if end - start < _NARRATION_MIN_COMMENT_LINES or _NARRATION_EVIDENCE.search(comment):
                continue
            head = end - 1
            while head < len(lines) and (
                not lines[head].strip() or lines[head].lstrip().startswith("//")
            ):
                head += 1
            if head >= len(lines) or not _FUNCTION_HEAD.match(lines[head].strip()):
                continue
            depth, opened, finish = 0, False, head
            for finish in range(head, min(len(lines), head + _NARRATION_MAX_BODY_LINES + 2)):
                depth += lines[finish].count("{") - lines[finish].count("}")
                opened = opened or "{" in lines[finish]
                if opened and depth <= 0:
                    break
            else:
                continue
            if not opened or depth > 0:
                continue
            rows.append(
                {
                    "location": f"{path}:{start}",
                    "comment_lines": end - start,
                    "function": lines[head].strip(),
                    "body_lines": finish - head + 1,
                }
            )
    rows.sort(key=lambda row: (-row["comment_lines"], row["location"]))
    return rows


_DUPLICATE_MIN_FIELDS = 3


def _duplicate_layouts(index: dict[str, Any], target: str = "WIZ8") -> list[dict[str, Any]]:
    """Distinct repository records with identical field type/offset sequences.

    Identical layouts are only candidates: two owners may legitimately share a
    shape. They are where independently reconstructed copies of one original
    abstraction tend to hide."""

    groups: dict[tuple[Any, ...], dict[str, str]] = {}
    for record in index.get("classes", []):
        fields = record.get("fields") or []
        if len(fields) < _DUPLICATE_MIN_FIELDS or record.get("bases"):
            continue
        name = str(record.get("qualified_name") or "")
        path = str(fields[0].get("source_file") or "")
        if "<" in name or not _target_path(path, target):
            continue
        key = tuple((field.get("type"), field.get("offset"), field.get("size")) for field in fields)
        groups.setdefault(key, {})[name] = f"{path}:{fields[0].get('line')}"
    rows = [
        {"field_count": len(key), "records": dict(sorted(members.items()))}
        for key, members in groups.items()
        if len(members) > 1
    ]
    rows.sort(key=lambda row: (-row["field_count"], sorted(row["records"])))
    return rows


_SIZE_ASSERTION = re.compile(
    r"static_assert\(\s*sizeof\(\s*([A-Za-z_][\w:]*)\s*\)\s*==\s*(0[xX][0-9A-Fa-f]+|\d+)"
)
_BYTE_STRIDE = re.compile(
    r"\bmalloc\((?:[^;()]*\*)?\s*(0[xX][0-9A-Fa-f]+)\s*\)"
    r"|\bmem(?:set|cpy|move)\([^;]*,(?:[^;,()]*\*)?\s*(0[xX][0-9A-Fa-f]+)\s*\)"
    r"|\b(?:FileRead|FileWrite|fread|fwrite)\([^;,]*,[^;,]*,(?:[^;,()]*\*)?\s*(0[xX][0-9A-Fa-f]+)\s*,"
)
_MIN_STRIDE_RECORD_SIZE = 0x10
_MAX_STRIDE_CANDIDATES = 2


def _byte_strides(sources: dict[str, str]) -> list[dict[str, Any]]:
    """Byte literals in allocation/stride positions equal to an asserted record size."""

    sizes: dict[int, set[str]] = {}
    for text in sources.values():
        for name, size in _SIZE_ASSERTION.findall(text):
            value = int(size, 0)
            if value >= _MIN_STRIDE_RECORD_SIZE:
                sizes.setdefault(value, set()).add(name)
    rows: list[dict[str, Any]] = []
    for path, text in sources.items():
        if not path.endswith((".c", ".cpp")):
            continue
        code = _STRING.sub(
            '""', _COMMENT.sub(lambda match: re.sub(r"[^\n]", " ", match.group(0)), text)
        )
        for match in _BYTE_STRIDE.finditer(code):
            literal = next(group for group in match.groups() if group)
            types = sizes.get(int(literal, 0))
            if not types or len(types) > _MAX_STRIDE_CANDIDATES:
                continue
            rows.append(
                {
                    "location": f"{path}:{code.count(chr(10), 0, match.start()) + 1}",
                    "literal": literal,
                    "candidate_types": sorted(types),
                }
            )
    rows.sort(key=lambda row: (len(row["candidate_types"]), row["location"]))
    return rows


_GENERIC_WORDS = frozenset(
    [
        "m",
        "g",
        "s",
        "field",
        "value",
        "unknown",
        "unk",
        "flag",
        "flags",
        "byte",
        "bytes",
        "dword",
        "word",
        "float",
        "int",
        "ptr",
        "data",
        "var",
        "bits",
        "short",
        "long",
        "double",
    ]
)
_STORAGE_WORDS = frozenset(
    [
        "field",
        "value",
        "unknown",
        "unk",
        "flag",
        "byte",
        "dword",
        "word",
        "float",
        "int",
        "ptr",
        "data",
        "var",
        "bits",
        "short",
        "long",
        "double",
    ]
)
_HEX_PART = re.compile(r"^(?:0x)?[0-9A-Fa-f]*[0-9][0-9A-Fa-f]*$")
_MARKER_COMMENT = re.compile(
    r"^//\s*(?:GLOBAL|LIBRARY|FUNCTION|SYNTHETIC|STRING|VTABLE|TEMPLATE|LINE)\b"
)
_COMMENT_STOP_WORDS = frozenset(
    [
        "the",
        "and",
        "for",
        "this",
        "that",
        "from",
        "with",
        "when",
        "only",
        "each",
        "used",
        "retail",
        "stored",
        "into",
        "than",
        "then",
        "also",
        "have",
        "does",
        "were",
        "which",
        "while",
    ]
)


def _placeholder_identifier(name: str) -> bool:
    """True when every word of ``name`` is a storage placeholder or an address/offset."""

    parts = [part for part in name.rsplit("::", 1)[-1].split("_") if part]
    words = [part.lower() for part in parts if not _HEX_PART.match(part)]
    return all(word in _GENERIC_WORDS for word in words) and (
        not words or any(word in _STORAGE_WORDS for word in words)
    )


_DISCLAIMED_SEMANTICS = re.compile(
    r"\b(?:unknown|unresolved|unread|never (?:read|consumed)|"
    r"no (?:recovered |retail )?(?:consumer|reader|writer)s?)\b",
    re.IGNORECASE,
)


def _adjacent_comment_text(lines: list[str], line: int) -> str:
    """Raw text of the trailing or immediately preceding authored comment."""

    if not 0 < line <= len(lines):
        return ""
    trailing = re.search(r"/\*(.*?)\*/|//(.*)$", lines[line - 1])
    text = (trailing.group(1) or trailing.group(2) or "") if trailing else ""
    if not text.strip():
        for previous in reversed(lines[max(line - 4, 0) : line - 1]):
            stripped = previous.strip()
            if _MARKER_COMMENT.match(stripped):
                continue
            closes_comment = stripped.endswith("*/") and ";" not in stripped.split("/*")[0]
            if stripped.startswith(("//", "/*", "*")) or closes_comment:
                text = stripped
            break
    return text


def _adjacent_comment(lines: list[str], line: int) -> str:
    """Words of the trailing or immediately preceding authored comment."""

    text = _adjacent_comment_text(lines, line)
    words = re.sub(r"0x[0-9A-Fa-f]+|[^A-Za-z ]", " ", text).lower().split()
    return " ".join(word for word in words if len(word) > 3 and word not in _COMMENT_STOP_WORDS)


def _known_semantics_bad_spelling(
    index: dict[str, Any], sources: dict[str, str], usage: _Usage, target: str = "WIZ8"
) -> list[dict[str, Any]]:
    """Placeholder-named storage whose adjacent comment already states a meaning."""

    lines = {path: text.split("\n") for path, text in sources.items()}
    candidates: dict[tuple[str, str], dict[str, Any]] = {}

    def consider(kind: str, owner: str, name: str, path: str, line: int, type_: Any) -> None:
        if path not in sources or _PADDING_STORAGE.match(name) or not _placeholder_identifier(name):
            return
        source_lines = lines.get(path, [])
        if _DISCLAIMED_SEMANTICS.search(_adjacent_comment_text(source_lines, line)):
            return
        comment = _adjacent_comment(source_lines, line)
        if comment:
            candidates.setdefault(
                (owner, name),
                {
                    "kind": kind,
                    "name": f"{owner}::{name}" if owner else name,
                    "type": type_,
                    "location": f"{path}:{line}",
                    "comment": comment,
                    "identifier": name,
                },
            )

    for variable in index.get("variables", []):
        name = str(variable.get("qualified_name") or "")
        if variable.get("target") == target.upper() and "::" not in name:
            consider(
                "global",
                "",
                name,
                str(variable.get("source_file") or ""),
                int(variable.get("line") or 0),
                variable.get("type"),
            )
    for record in index.get("classes", []):
        for field in record.get("fields", []):
            consider(
                "member",
                str(record.get("qualified_name") or ""),
                str(field.get("name") or ""),
                str(field.get("source_file") or ""),
                int(field.get("line") or 0),
                field.get("type"),
            )
    rows = []
    for row in candidates.values():
        identifier = row.pop("identifier")
        rows.append({**row, **usage(identifier, 1)})
    rows.sort(key=lambda row: (-row["references"], row["name"]))
    return rows


def _external_single_unit_definitions(
    index: dict[str, Any], usage: _Usage, target: str = "WIZ8"
) -> list[dict[str, Any]]:
    """Externally linked free functions/globals referenced only from their defining file.

    Static linkage still needs proof that no import/export, marker-established
    cross-unit caller or retail symbol requires external visibility."""

    rows: list[dict[str, Any]] = []
    seen: set[str] = set()
    entities = [
        (item, "function")
        for item in index.get("declarations", [])
        if item.get("semantic_kind") == "free_function" and item.get("is_definition")
    ] + [
        (item, "global")
        for item in index.get("variables", [])
        if item.get("target") == target.upper() and item.get("definition_kind") == "definition"
    ]
    for item, kind in entities:
        name = str(item.get("qualified_name") or "")
        path = str(item.get("source_file") or "")
        if (
            "::" in name
            or name in seen
            or item.get("linkage") != "external"
            or not _target_path(path, target)
            or not path.startswith("src/")
        ):
            continue
        files = set(usage.counts.get(name, {}))
        if files != {path}:
            continue
        seen.add(name)
        rows.append({"kind": kind, "name": name, "location": f"{path}:{item.get('line')}"})
    rows.sort(key=lambda row: row["location"])
    return rows


_EMPTY_BODY = re.compile(r"\)\s*(?::[^{;]*)?\{\s*\}")


def _empty_special_members(index: dict[str, Any], sources: dict[str, str]) -> list[dict[str, Any]]:
    """Hand-written empty constructors/destructors in the selected source.

    An empty authored body is faithful when a declaration requires it; one that
    only claims an implicit emission belongs in binary emission metadata."""

    rows: list[dict[str, Any]] = []
    for item in index.get("declarations", []):
        path = str(item.get("source_file") or "")
        if (
            item.get("semantic_kind") not in {"constructor", "destructor"}
            or not item.get("is_definition")
            or path not in sources
        ):
            continue
        start, end = int(item.get("line") or 0), int(item.get("end_line") or 0)
        body = "\n".join(sources[path].split("\n")[start - 1 : end])
        body = _COMMENT.sub(" ", body)
        if _EMPTY_BODY.search(body) and "(" in body:
            initialized = bool(re.search(r"\)\s*:", body))
            rows.append(
                {
                    "name": item.get("qualified_name"),
                    "kind": item.get("semantic_kind"),
                    "location": f"{path}:{start}",
                    "has_initializer_list": initialized,
                }
            )
    rows.sort(key=lambda row: row["location"])
    return rows


_BASE_ASSIGNMENT = re.compile(r"\b([A-Za-z_]\w*)::operator=\s*\(")


def _explicit_base_assignments(
    index: dict[str, Any], sources: dict[str, str]
) -> list[dict[str, Any]]:
    """Authored assignment operators that call a base assignment explicitly."""

    rows: list[dict[str, Any]] = []
    for item in index.get("declarations", []):
        path = str(item.get("source_file") or "")
        name = str(item.get("qualified_name") or "")
        if not name.endswith("::operator=") or not item.get("is_definition"):
            continue
        if path not in sources:
            continue
        start, end = int(item.get("line") or 0), int(item.get("end_line") or 0)
        body = _COMMENT.sub(" ", "\n".join(sources[path].split("\n")[start:end]))
        bases = sorted(set(_BASE_ASSIGNMENT.findall(body)))
        if bases:
            rows.append(
                {
                    "name": name,
                    "location": f"{path}:{start}",
                    "base_assignments": bases,
                    "member_assignments": len(re.findall(r"\bother\.\w+", body)),
                }
            )
    rows.sort(key=lambda row: row["location"])
    return rows


def _comment_regions(lines: list[str]) -> list[tuple[int, int, str]]:
    """Return (start_line, end_line, text) for ``/* */`` blocks and ``//`` runs.

    Lines are 1-based and the end line is exclusive. ``//``-only consecutive
    lines merge into one region; a trailing ``//`` after code forms its own
    single-line region. Only the first block comment on a line is captured.
    """

    regions: list[tuple[int, int, str]] = []
    index = 0
    total = len(lines)
    while index < total:
        line = lines[index]
        if line.lstrip().startswith("//"):
            start = index
            parts = []
            while index < total and lines[index].lstrip().startswith("//"):
                parts.append(lines[index].lstrip()[2:])
                index += 1
            regions.append((start + 1, index, "\n".join(parts)))
            continue
        block = line.find("/*")
        slash = line.find("//")
        if slash != -1 and (block == -1 or slash < block):
            regions.append((index + 1, index + 1, line[slash:]))
            index += 1
            continue
        if block == -1:
            index += 1
            continue
        start = index
        parts = [line[block + 2 :]]
        closing = parts[0].find("*/")
        if closing != -1:
            parts[0] = parts[0][:closing]
        else:
            index += 1
            while index < total:
                closing = lines[index].find("*/")
                if closing != -1:
                    parts.append(lines[index][:closing])
                    break
                parts.append(lines[index])
                index += 1
        regions.append((start + 1, index + 1, "\n".join(parts)))
        index += 1
    return regions


def _candidate_addresses(text: str) -> set[int]:
    """Collect retail VAs cited by a comment: ``0x`` literals and the hex
    suffixes positional names embed, accepting dropped leading zeros."""

    addresses = {int(match.group(1), 16) for match in _EXPLICIT_ADDRESS.finditer(text)}
    for token in _ADDRESS_SUFFIXED_TOKEN.finditer(text):
        digits_match = _TRAILING_HEX.search(token.group(0))
        if not digits_match:
            continue
        digits = digits_match.group(1)
        addresses.add(int(digits, 16))
        if len(digits) > 6:
            addresses.add(int(digits[-6:], 16))
    return addresses


def _stale_recovery_claims(
    repository: Path, functions: dict[int, Any], target: str = "WIZ8"
) -> list[dict[str, Any]]:
    """Flag prose debt claims whose cited address is a defined FUNCTION.

    A claim phrase alone is not flagged: plenty of comments legitimately call
    out still-unrecovered fields or routines. A claim becomes stale when the
    retail address it cites (or an address-suffixed name it mentions) already
    carries a FUNCTION marker in the source index.
    """

    rows: list[dict[str, Any]] = []
    for root_name in _SOURCE_ROOTS.get(target.upper(), ()):
        root = repository / root_name
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix.casefold() not in _SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository).as_posix()
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for start, end, comment in _comment_regions(lines):
                phrase = _STALE_CLAIM.search(comment)
                if not phrase:
                    continue
                context = [comment]
                for line in lines[end : min(end + 8, len(lines))]:
                    if not line.strip():
                        break
                    context.append(line)
                for address in sorted(_candidate_addresses("\n".join(context))):
                    function = functions.get(address)
                    if function is None:
                        continue
                    rows.append(
                        {
                            "source_file": relative,
                            "line": start,
                            "phrase": phrase.group(0),
                            "address": f"0x{address:08x}",
                            "resolved_name": function.name,
                            "resolved_source_file": function.source_file,
                        }
                    )
    rows.sort(key=lambda row: (row["source_file"], row["line"], row["address"]))
    return rows


def _unresolved_functions(index: dict[str, Any], target: str | None = None) -> list[dict[str, Any]]:
    """A recovered body does not resolve an address-shaped source identity."""

    rows: dict[str, dict[str, Any]] = {}
    for item in index.get("declarations", []):
        name = str(item.get("qualified_name") or "")
        semantic_id = str(item.get("semantic_id") or "")
        if not _PLACEHOLDER.fullmatch(name.rsplit("::", 1)[-1]):
            continue
        if target is not None and item.get("target") != target.upper():
            continue
        key = f"{item.get('target', '')}:{semantic_id}"
        if key not in rows or item.get("is_definition"):
            rows[key] = {
                "name": name,
                "semantic_id": semantic_id,
                "source_file": str(item.get("source_file") or ""),
                "line": int(item.get("line") or 0),
                "is_definition": bool(item.get("is_definition")),
            }
    return sorted(rows.values(), key=lambda row: (row["name"], row["source_file"], row["line"]))


def semantic_name_opportunity_report(repository: Path) -> dict[str, Any]:
    """Rank unresolved placeholder names by their checked-in call-site footprint."""

    index = load_source_index(repository)
    candidates = _unresolved_functions(index)
    sources = []
    for root_name in ("include/wiz8", "src/wiz8", "include/surrender", "src/surrender"):
        root = repository / root_name
        sources.extend(
            path.read_text(encoding="utf-8", errors="replace")
            for path in root.rglob("*")
            if path.is_file() and path.suffix.casefold() in _SOURCE_SUFFIXES
        )
    corpus = "\n".join(sources)
    rows = []
    for candidate in candidates:
        references = len(re.findall(rf"\b{re.escape(candidate['name'])}\s*\(", corpus))
        if references < 2:
            continue
        rows.append({**candidate, "references": references})
    rows.sort(key=lambda row: (-row["references"], row["name"]))
    return {
        "schema": "wiz8.semantic-name-opportunities-v1",
        "non_gating": True,
        "count": len(rows),
        "functions": rows,
    }


def _retail_identity_ownership(repository: Path, target: str) -> dict[str, Any]:
    """Classify reccmp's existing identities, independently of rebuild pairing.

    This does not discover retail objects or infer an owner from a nearby
    address. Catalog coverage and body/extent coverage need separate review.
    """
    from collections import Counter

    from reccmp.compare import Compare
    from reccmp.types import EntityType, ImageId

    from ..comparison import comparison_target
    from ..emissions import emission_inventory
    from ..source_index import address_bound_identities

    engine = Compare.from_target(comparison_target(repository, target))
    bindings = address_bound_identities(repository, target)
    emissions = {row.address: row for row in emission_inventory(repository, target)}
    aliases = {
        entity.orig_addr: canonical.orig_addr
        for entity, canonical in engine.db.get_aliases(ImageId.ORIG)
    }
    rows = []
    for entity in engine.get_all():
        if entity.orig is None:
            continue
        address = entity.orig_addr
        if address is None:
            continue
        facts = entity.orig.facts
        identities = bindings.get(address, ())
        emission = emissions.get(address)
        bucket = None
        basis = None
        if address in aliases:
            bucket, basis = "icf_folded_sibling", "native reccmp alias"
        elif entity.entity_type == EntityType.IMPORT:
            bucket, basis = "external_abi", "retail PE import"
        elif entity.entity_type in (EntityType.IMPORT_THUNK, EntityType.THUNK, EntityType.VTORDISP):
            bucket, basis = "thunk", "native reccmp thunk classification"
        elif facts.get("library") or any(
            identity.kind == "library" or identity.source_file.startswith("src/sgp/")
            for identity in identities
        ):
            bucket, basis = "library_owned", "library metadata or component source binding"
        elif emission is not None:
            bucket = (
                "template_inline_emission" if emission.type == "template" else "compiler_generated"
            )
            basis = "reviewed compiler emission inventory"
        elif (
            entity.entity_type == EntityType.LABEL
            and str(facts.get("name") or "").startswith(("__Unwind", "__ehhandler"))
            or entity.entity_type == EntityType.DATA
            and "seh_unwinds_orig" in facts
        ):
            bucket, basis = "compiler_generated", "native reccmp exception metadata"
        elif entity.entity_type == EntityType.VTABLE and any(
            identity.kind == "vtable" for identity in identities
        ):
            bucket, basis = "compiler_generated", "source-bound class vtable"
        elif any(identity.kind in ("definition", "global") for identity in identities):
            bucket, basis = "recovered_authored_source", "explicit source address binding"
        elif entity.entity_type in (EntityType.STRING, EntityType.WIDECHAR, EntityType.FLOAT):
            bucket, basis = "compiler_generated", "native reccmp literal classification"
        rows.append(
            {
                "address": f"0x{address:08x}",
                "entity_type": EntityType(entity.entity_type).name if entity.entity_type else None,
                "name": facts.get("name") or facts.get("source_name"),
                "bucket": bucket,
                "basis": basis,
                "paired": entity.recomp_addr is not None,
                "source_files": sorted(
                    {identity.source_file for identity in identities}
                    | set(emission.source_files if emission else ())
                ),
            }
        )
    counts = Counter(row["bucket"] for row in rows if row["bucket"] is not None)
    unresolved = [row for row in rows if row["bucket"] is None]
    return {
        "scope": "existing native reccmp catalog; not a retail object-discovery or byte-coverage proof",
        "inventory_complete": False,
        "entities": len(rows),
        "classified": len(rows) - len(unresolved),
        "unclassified": len(unresolved),
        "without_recomp_pair": sum(not row["paired"] for row in rows),
        "buckets": dict(sorted(counts.items())),
        "unclassified_entities": unresolved,
        "identities": rows,
    }


def semantic_debt_report(
    repository: Path,
    target: str = "WIZ8",
    *,
    retail_identities: bool = False,
) -> dict[str, Any]:
    """Return a read-only recovery queue; none of its rows are validation failures."""

    index = load_source_index(repository)
    units = source_unit_records(repository)
    unmapped_paths = [
        path
        for path, record in units.items()
        if record["mapping"] == UNMAPPED_SOURCE and _target_path(path, target)
    ]
    functions = source_functions(repository, target)
    suffixed = [
        {
            "address": f"0x{address:08x}",
            "name": function.name,
            "source_file": function.source_file,
        }
        for address, function in sorted(functions.items())
        if _ADDRESS_SUFFIX.fullmatch(function.name) and not _PLACEHOLDER.fullmatch(function.name)
    ]

    unit_set = set(unmapped_paths)
    anchors, headers = assertion_anchors(read_assertions(repository))
    layout = TranslationUnitLayout(anchors, header_anchors=headers)
    provisional_by_file: dict[str, dict[str, Any]] = {
        path: {"source_file": path, "function_count": 0, "candidate_original_units": set()}
        for path in unmapped_paths
    }
    for marker in index.get("markers", []):
        current = str(marker.get("source_file") or "")
        if marker.get("marker_kind") != "FUNCTION" or current not in unit_set:
            continue
        provisional_by_file[current]["function_count"] += 1
        address = int(marker["address"])
        owner = layout.owner(address)
        if owner.get("attribution") not in {"direct", "bounded", "cross-build"}:
            continue
        candidate = str(owner.get("source_path") or "")
        if candidate:
            provisional_by_file[current]["candidate_original_units"].add(candidate)

    provisional = [
        {**row, "candidate_original_units": sorted(row["candidate_original_units"])}
        for row in provisional_by_file.values()
    ]
    provisional.sort(key=lambda row: (-row["function_count"], row["source_file"]))

    source_shaping = _source_shaping_directives(repository, target)
    unresolved = _unresolved_functions(index, target)
    stale = _stale_recovery_claims(repository, functions, target)
    sources = _target_sources(repository, target)
    usage = _Usage(sources) if sources else None
    storage = (
        _storage_debt(index, usage, target)
        if usage
        else {
            "address_named_globals": [],
            "address_named_members": [],
            "accessed_padding_members": [],
        }
    )
    void_storage = _void_storage(index, target) if sources else []
    owned_types = {
        str(record.get("qualified_name") or "").rpartition("::")[2]
        for record in index.get("classes", [])
    }
    cast_escapes = _cast_escapes(sources, owned_types)
    enum_literals = _enum_literal_arguments(index, sources) if sources else []
    narration = _narration_candidates(sources)
    duplicate_layouts = _duplicate_layouts(index, target) if sources else []
    byte_strides = _byte_strides(sources)
    bad_spelling = _known_semantics_bad_spelling(index, sources, usage, target) if usage else []
    single_unit = _external_single_unit_definitions(index, usage, target) if usage else []
    empty_special = _empty_special_members(index, sources) if sources else []
    base_assignments = _explicit_base_assignments(index, sources) if sources else []
    from .portability import portability_queues

    portability = portability_queues(repository, index)
    ownership = _retail_identity_ownership(repository, target) if retail_identities else None
    return {
        **({"retail_identity_ownership": ownership} if ownership is not None else {}),
        "schema": "wiz8.semantic-debt-v2",
        "non_gating": True,
        "summary": {
            "unmapped_sources": len(unmapped_paths),
            "unresolved_function_identities": len(unresolved),
            "address_suffixed_names": len(suffixed),
            "source_shaping_directives": len(source_shaping),
            "provisional_tu_placements": len(provisional),
            "stale_recovery_claims": len(stale),
            "address_named_globals": len(storage["address_named_globals"]),
            "address_named_members": len(storage["address_named_members"]),
            "accessed_padding_members": len(storage["accessed_padding_members"]),
            "void_storage_members": len(void_storage),
            "cast_escape_sites": sum(row["total"] for row in cast_escapes),
            "byte_strides_matching_record_sizes": len(byte_strides),
            "enum_parameters_passed_literals": len(enum_literals),
            "narration_candidates": len(narration),
            "duplicate_record_layouts": len(duplicate_layouts),
            "known_semantics_bad_spelling": len(bad_spelling),
            "external_single_unit_definitions": len(single_unit),
            "empty_special_members": len(empty_special),
            "explicit_base_assignments": len(base_assignments),
        },
        "unmapped_sources": provisional,
        "unresolved_function_identities": unresolved,
        "address_suffixed_names": suffixed,
        "source_shaping_directives": source_shaping,
        "provisional_tu_placements": provisional,
        "stale_recovery_claims": stale,
        **storage,
        "void_storage_members": void_storage,
        "cast_escapes": cast_escapes,
        "byte_strides_matching_record_sizes": byte_strides,
        "enum_parameters_passed_literals": enum_literals,
        "narration_candidates": narration,
        "duplicate_record_layouts": duplicate_layouts,
        "known_semantics_bad_spelling": bad_spelling,
        "external_single_unit_definitions": single_unit,
        "empty_special_members": empty_special,
        "explicit_base_assignments": base_assignments,
        "pre_portability": portability,
    }
