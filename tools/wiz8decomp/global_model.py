"""Ownership and type identity for recovered globals.

One retail address has one source object. ``// GLOBAL: WIZ8 0x...`` markers
name independently defined storage; a second definition at the same start, or
inside another known extent, is a model defect.
"""

from __future__ import annotations

import ast
import re
from collections import defaultdict
from pathlib import Path
from typing import Any

_GLOBAL_MARKER = re.compile(
    r"^\s*//\s*GLOBAL:\s+(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9a-fA-F]+)\s*$",
    re.IGNORECASE,
)
_SOURCE_MARKER = re.compile(r"^\s*//\s*(?:FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY|VTABLE|GLOBAL):\s+")
_SIZEOF_ASSERT = re.compile(
    r"static_assert\s*\(\s*sizeof\s*\(\s*([A-Za-z_][\w:]*)\s*\)\s*==\s*(0x[0-9a-fA-F]+|\d+)",
)
_ARRAY_EXTENT = re.compile(r"\[([^\]]*)\]")
_DECL = re.compile(
    r"^(?P<prefix>.*?)(?P<name>[A-Za-z_]\w*)\s*(?P<arrays>(?:\[[^\]]*\])*)\s*(?:=|;)"
)
_VTABLE_OR_FUNCTION = re.compile(r"^\s*//\s*(?:VTABLE|FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY):")
_IDENTITY_ALIAS = re.compile(r"identity-alias\s*:", re.IGNORECASE)
_DOCUMENTED_ALIAS = re.compile(r"alias(?:es)?\s+(?:of|for)\b|no separate definition", re.IGNORECASE)

PRIMITIVE_SIZES = {
    "bool": 1,
    "char": 1,
    "signed char": 1,
    "unsigned char": 1,
    "short": 2,
    "unsigned short": 2,
    "wchar_t": 2,
    "int": 4,
    "unsigned": 4,
    "unsigned int": 4,
    "long": 4,
    "unsigned long": 4,
    "float": 4,
    "double": 8,
    "long long": 8,
    "unsigned long long": 8,
    "size_t": 4,
    "int8_t": 1,
    "uint8_t": 1,
    "int16_t": 2,
    "uint16_t": 2,
    "int32_t": 4,
    "uint32_t": 4,
}

# Win32 ABI-equivalent spellings of the same storage.
_EQUIVALENT = {
    "int": "int32",
    "long": "int32",
    "unsigned": "uint32",
    "unsigned int": "uint32",
    "unsigned long": "uint32",
    "short": "int16",
    "unsigned short": "uint16",
    "char": "int8",
    "signed char": "int8",
    "unsigned char": "uint8",
    "wchar_t": "uint16",
}


class GlobalOverlapError(RuntimeError):
    """Two independently defined globals occupy the same retail storage."""


class TypeConsistencyError(RuntimeError):
    """One retail address is read or written through incompatible types."""


def _number(text: str) -> int:
    return int(text, 16) if text.lower().startswith("0x") else int(text)


def known_type_sizes(repo_dir: Path) -> dict[str, int]:
    sizes = dict(PRIMITIVE_SIZES)
    roots = (repo_dir / "include", repo_dir / "src")
    for root in roots:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp", ".c"}:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for name, value in _SIZEOF_ASSERT.findall(text):
                sizes[name] = _number(value)
    sizes.setdefault("W8GrowableVector", 0x10)
    return sizes


def _eval_extent(expression: str) -> int | None:
    stripped = expression.strip()
    if not stripped:
        return None
    try:
        tree = ast.parse(stripped, mode="eval")
    except SyntaxError:
        return None
    allowed = (
        ast.Expression,
        ast.BinOp,
        ast.UnaryOp,
        ast.Constant,
        ast.Mult,
        ast.Add,
        ast.Sub,
        ast.FloorDiv,
        ast.USub,
        ast.UAdd,
    )
    if not all(isinstance(node, allowed) for node in ast.walk(tree)):
        return None
    try:
        value = ast.literal_eval(stripped)
    except (ValueError, SyntaxError):
        if not isinstance(tree.body, ast.BinOp) or not isinstance(tree.body.op, ast.Mult):
            return None
        try:
            value = ast.literal_eval(tree.body.left) * ast.literal_eval(tree.body.right)
        except (ValueError, SyntaxError):
            return None
    return int(value) if isinstance(value, int) else None


def _base_type_size(type_name: str, sizes: dict[str, int]) -> int | None:
    cleaned = re.sub(r"\s+", " ", type_name).strip()
    cleaned = re.sub(r"^(?:const|volatile|static|class|struct|enum)\s+", "", cleaned)
    cleaned = re.sub(r"\s+(?:const|volatile)$", "", cleaned)
    if "*" in cleaned or "&" in cleaned:
        return 4
    template = re.match(r"^(W8GrowableVector|W8HashTable)\s*<", cleaned)
    if template:
        return sizes.get(
            template.group(1), 0x10 if template.group(1) == "W8GrowableVector" else None
        )
    return sizes.get(cleaned)


def _declaration_size(type_name: str, arrays: str, sizes: dict[str, int]) -> int | None:
    extents = [_eval_extent(item) for item in _ARRAY_EXTENT.findall(arrays)]
    if any(item is None for item in extents):
        # Incomplete `[]` or unparsable bound: start address only.
        return None
    element = _base_type_size(type_name, sizes)
    if element is None:
        return None
    size = element
    for extent in extents:
        size *= extent or 0
        if extent == 0:
            return None
    return size


def _strip_comments_and_qualifiers(line: str) -> str:
    return line.split("//", 1)[0].strip()


def parse_global_definitions(
    repo_dir: Path, sizes: dict[str, int] | None = None
) -> list[dict[str, Any]]:
    """Independently defined ``GLOBAL`` objects, excluding externs and aliases."""

    if sizes is None:
        sizes = known_type_sizes(repo_dir)
    definitions: list[dict[str, Any]] = []
    roots = (repo_dir / "src", repo_dir / "include")
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp", ".c"}:
                continue
            relative = str(path.relative_to(repo_dir))
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            depth = 0
            index = 0
            while index < len(lines):
                line = lines[index]
                depth += line.count("{") - line.count("}")
                match = _GLOBAL_MARKER.match(line)
                if not match:
                    index += 1
                    continue
                location = f"{relative}:{index + 1}"
                address = int(match.group("address"), 16)
                target = match.group("target").upper()
                look = index + 1
                comments: list[str] = []
                while look < len(lines):
                    stripped = lines[look].strip()
                    if not stripped:
                        look += 1
                        continue
                    if stripped.startswith("//"):
                        comments.append(stripped)
                        if _SOURCE_MARKER.match(lines[look]) and not _GLOBAL_MARKER.match(
                            lines[look]
                        ):
                            break
                        look += 1
                        continue
                    break
                if look >= len(lines):
                    index += 1
                    continue
                decl = _strip_comments_and_qualifiers(lines[look])
                window = " ".join(comments)
                if (
                    decl.startswith("extern ")
                    or _IDENTITY_ALIAS.search(window)
                    or _DOCUMENTED_ALIAS.search(window)
                ):
                    index = look + 1
                    continue
                if depth > 0:
                    # Function-local static. Same ownership rules still apply when
                    # it carries an independent GLOBAL address, so keep it.
                    pass
                parsed = _DECL.match(decl)
                if parsed is None:
                    definitions.append(
                        {
                            "name": "",
                            "address": address,
                            "size": None,
                            "type": "",
                            "source_file": relative,
                            "line": index + 1,
                            "target": target,
                            "location": location,
                            "extern": False,
                        }
                    )
                    index = look + 1
                    continue
                type_name = parsed.group("prefix").strip()
                name = parsed.group("name")
                arrays = parsed.group("arrays") or ""
                definitions.append(
                    {
                        "name": name,
                        "address": address,
                        "size": _declaration_size(type_name, arrays, sizes),
                        "type": (type_name + arrays).strip(),
                        "source_file": relative,
                        "line": look + 1,
                        "target": target,
                        "location": location,
                        "extern": False,
                    }
                )
                index = look + 1
    return definitions


def _end(item: dict[str, Any]) -> int | None:
    size = item.get("size")
    if not size:
        return None
    return int(item["address"]) + int(size)


def overlapping_globals(definitions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Independently defined globals that share retail storage."""

    by_target: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in definitions:
        if item.get("extern"):
            continue
        if not item.get("name"):
            continue
        by_target[str(item.get("target") or "WIZ8")].append(item)

    violations: list[dict[str, Any]] = []
    for target, items in by_target.items():
        ordered = sorted(items, key=lambda item: (item["address"], item["name"]))
        for index, inner in enumerate(ordered):
            for outer in ordered:
                if inner is outer:
                    continue
                inner_start = int(inner["address"])
                outer_start = int(outer["address"])
                outer_end = _end(outer)
                inner_end = _end(inner)
                same_start = inner_start == outer_start
                contained = outer_end is not None and outer_start < inner_start < outer_end
                partial = (
                    outer_end is not None
                    and inner_end is not None
                    and inner_start < outer_start < inner_end < outer_end
                )
                if not (same_start or contained or partial):
                    continue
                if same_start:
                    names = sorted((inner["name"], outer["name"]))
                    if inner["name"] != names[0]:
                        continue
                    container, member = outer, inner
                    if container["name"] != names[1]:
                        container, member = inner, outer
                elif outer_start < inner_start:
                    container, member = outer, inner
                else:
                    continue
                offset = int(member["address"]) - int(container["address"])
                detail = (
                    f"{member['name']} @ 0x{member['address']:x} overlaps "
                    f"{container['name']} + 0x{offset:x}."
                )
                key = (
                    target,
                    member["address"],
                    container["address"],
                    member["name"],
                    container["name"],
                )
                violations.append(
                    {
                        "kind": "global-overlap",
                        "target": target,
                        "name": member["name"],
                        "address": f"0x{member['address']:08x}",
                        "container": container["name"],
                        "offset": offset,
                        "detail": detail,
                        "key": key,
                    }
                )
    unique: dict[tuple[Any, ...], dict[str, Any]] = {}
    for item in violations:
        unique.setdefault(item["key"], item)
    return [unique[key] for key in sorted(unique)]


def _normalize_type(type_name: str) -> str:
    cleaned = re.sub(r"\s+", " ", type_name).strip()
    cleaned = cleaned.replace(" *", "*").replace("*", "*")
    if cleaned.endswith("*"):
        return "ptr:" + _normalize_type(cleaned[:-1].rstrip())
    arrays = "".join(f"[{item}]" for item in _ARRAY_EXTENT.findall(cleaned))
    base = _ARRAY_EXTENT.sub("", cleaned).strip()
    base = _EQUIVALENT.get(base, base)
    return base + arrays


def type_consistency_violations(
    definitions: list[dict[str, Any]], extra: list[dict[str, Any]] | None = None
) -> list[dict[str, Any]]:
    """Incompatible source-level types at one absolute address."""

    claims: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
    for item in list(definitions) + list(extra or ()):
        type_name = str(item.get("type") or "")
        if not type_name or not item.get("name"):
            continue
        claims[(str(item.get("target") or "WIZ8"), int(item["address"]))].append(item)
    violations: list[dict[str, Any]] = []
    for (target, address), entries in sorted(claims.items()):
        normalized = {_normalize_type(item["type"]) for item in entries}
        if len(normalized) <= 1:
            continue
        # Pointer vs integer, 1-byte vs 4-byte, unrelated class pointers.
        names = sorted({item["name"] for item in entries})
        types = sorted(normalized)
        violations.append(
            {
                "kind": "type-consistency",
                "target": target,
                "address": f"0x{address:08x}",
                "names": names,
                "types": types,
                "detail": (
                    f"0x{address:08x}: incompatible types {', '.join(types)} for {', '.join(names)}"
                ),
            }
        )
    return violations


def validate_global_ownership(repo_dir: Path) -> dict[str, Any]:
    sizes = known_type_sizes(repo_dir)
    definitions = parse_global_definitions(repo_dir, sizes)
    overlaps = overlapping_globals(definitions)
    if overlaps:
        rendered = [item["detail"] for item in overlaps]
        raise GlobalOverlapError(
            "independently defined globals overlap retail storage:\n  " + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "global-ownership",
        "globals": len(definitions),
        "sized": sum(item["size"] is not None for item in definitions),
    }


def validate_type_consistency(repo_dir: Path) -> dict[str, Any]:
    sizes = known_type_sizes(repo_dir)
    definitions = parse_global_definitions(repo_dir, sizes)
    violations = type_consistency_violations(definitions)
    if violations:
        rendered = [item["detail"] for item in violations]
        raise TypeConsistencyError(
            "one address has incompatible source types:\n  " + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "type-consistency", "globals": len(definitions)}


_STATUS_MEMBER = re.compile(r"g_status_685170\.([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)")


def status_member_accesses(repo_dir: Path) -> list[dict[str, Any]]:
    """Source sites that name a g_status_685170 member."""

    rows: list[dict[str, Any]] = []
    for root in (repo_dir / "src/wiz8", repo_dir / "include/wiz8"):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
                continue
            relative = str(path.relative_to(repo_dir))
            for number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                for match in _STATUS_MEMBER.finditer(line):
                    rows.append(
                        {
                            "file": relative,
                            "line": number,
                            "member": match.group(1),
                            "absolute": f"0x{GSTATUS_START:08x}",
                        }
                    )
    return rows


GSTATUS_START = 0x00685170
GSTATUS_SIZE = 0x49C2
GSTATUS_END = GSTATUS_START + GSTATUS_SIZE
GXSTATUS_START = 0x00683F78


def classify_status_region(definitions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Address-sorted globals around gStatus / gXStatus."""

    rows: list[dict[str, Any]] = []
    objects = [item for item in definitions if item.get("size") and item["name"]]
    for item in sorted(definitions, key=lambda row: row["address"]):
        address = int(item["address"])
        if address < 0x00685000 or address >= 0x00689C00:
            continue
        kind = "standalone"
        container = ""
        offset: int | None = None
        if GSTATUS_START <= address < GSTATUS_END:
            if address == GSTATUS_START and str(item["name"]).startswith("g_status"):
                kind = "object"
                container = str(item["name"])
                offset = 0
            else:
                kind = "member of g_status_685170"
                container = "g_status_685170"
                offset = address - GSTATUS_START
        elif address == GXSTATUS_START:
            kind = "object"
            container = str(item["name"])
            offset = 0
        else:
            for outer in objects:
                start = int(outer["address"])
                end = start + int(outer["size"])
                if start <= address < end and outer["name"] != item["name"]:
                    kind = f"member of {outer['name']}"
                    container = str(outer["name"])
                    offset = address - start
                    break
            else:
                kind = "unresolved" if item.get("size") is None else "standalone"
        rows.append(
            {
                "address": f"0x{address:08x}",
                "name": item["name"],
                "type": item.get("type") or "",
                "size": item.get("size"),
                "class": kind,
                "container": container,
                "offset": None if offset is None else f"0x{offset:x}",
                "source_file": item.get("source_file") or "",
            }
        )
    return rows
