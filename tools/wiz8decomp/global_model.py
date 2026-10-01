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
_GLOBAL_LINE = re.compile(r"^\s*//\s*GLOBAL(?:\s*:\s*(?P<rest>.*))?\s*$", re.IGNORECASE)
_UNRESOLVED_GLOBAL = re.compile(r"\bunresolved(?:-global)?\b", re.IGNORECASE)
_SOURCE_MARKER = re.compile(r"^\s*//\s*(?:FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY|VTABLE|GLOBAL):\s+")
_SIZEOF_ASSERT = re.compile(
    r"static_assert\s*\(\s*sizeof\s*\(\s*([A-Za-z_][\w:]*)\s*\)\s*==\s*(0x[0-9a-fA-F]+|\d+)",
)
_ARRAY_EXTENT = re.compile(r"\[([^\]]*)\]")
_LOCAL_DEFINE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)(?:\s+(.+))?$")
_LOCAL_UNDEF = re.compile(r"^\s*#\s*undef\s+([A-Za-z_]\w*)\b")
_PREPROCESSOR_IF = re.compile(r"^\s*#\s*(?:if|ifdef|ifndef)\b")
_PREPROCESSOR_ENDIF = re.compile(r"^\s*#\s*endif\b")
_QUOTED_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
_NUMERIC_CONSTANT = re.compile(r"\b([A-Za-z_]\w*)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*(?:,|}|$)")
_DECL = re.compile(
    r"^(?P<prefix>.*?)(?:[A-Za-z_]\w*::)*(?P<name>[A-Za-z_]\w*)\s*"
    r"(?P<arrays>(?:\[[^\]]*\])*)\s*(?:=|;)"
)
_DIRECT_INIT = re.compile(
    r"^(?P<prefix>(?:static\s+)?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*<[^;]+>)\s+"
    r"(?P<name>[A-Za-z_]\w*)\s*\([^;]*\)\s*;"
)
_CALLBACK_DECL = re.compile(
    r"^(?P<result>(?:static\s+)?[A-Za-z_][\w:\s*<>]*)\(\s*\*\s*"
    r"(?P<name>[A-Za-z_]\w*)(?P<arrays>(?:\[[^\]]*\])*)\s*\)\s*"
    r"\((?P<args>[^)]*)\)\s*(?:=|;)"
)
_VTABLE_OR_FUNCTION = re.compile(r"^\s*//\s*(?:VTABLE|FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY):")
_DOCUMENTED_ALIAS = re.compile(r"alias(?:es)?\s+(?:of|for)\b|no separate definition", re.IGNORECASE)
# A file-scope variable definition whose name ends in a retail address. The
# name must be preceded by type text so in-body assignments (`g_x_... = 1;`)
# do not count as declarations.
_ADDRESS_SUFFIX_DECL = re.compile(
    r"^\s*(?P<prefix>(?:static|const|volatile|mutable|register)\s+)*"
    r"(?P<rtype>(?:[A-Za-z_][\w:<>,]*(?:::[\w<>]+)*[\s*&]+)+)"
    r"(?P<name>[A-Za-z_]\w*?_(?P<address>[0-9a-fA-F]{8}))\s*(?:\[[^\]]*\])*\s*(?:=|;)"
)

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
    # Accepted SGP/Win32 typedefs used as GLOBAL declaration elements.
    "CHAR": 1,
    "CHAR8": 1,
    "BOOLEAN": 1,
    "UINT8": 1,
    "INT8": 1,
    "UINT16": 2,
    "INT16": 2,
    "UINT32": 4,
    "INT32": 4,
    "TIMER": 4,
    "HVOBJECT": 4,
    "HVSURFACE": 4,
    "LPDIRECTDRAW": 4,
    "LPDIRECTDRAW2": 4,
    "LPDIRECTDRAWSURFACE": 4,
    "LPDIRECTDRAWSURFACE2": 4,
    "HHOOK": 4,
    "HINSTANCE": 4,
    "HWND": 4,
    "HANDLE": 4,
    "H3DPOBJECT": 4,
    "HDIGDRIVER": 4,
    "HPROVIDER": 4,
    "POINT": 8,
    "WNDPROC": 4,
}
_TEMPLATE_DEFAULT_SIZES = {
    "W8GrowableVector": 0x10,
    "W8Vector": 0x10,
    "srArray": 0x08,
    "srHeapBuffer": 0x08,
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
    "BOOLEAN": "uint8",
    "UINT8": "uint8",
    "INT8": "int8",
    "UINT16": "uint16",
    "INT16": "int16",
    "UINT32": "uint32",
    "INT32": "int32",
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
    for name, size in _TEMPLATE_DEFAULT_SIZES.items():
        sizes.setdefault(name, size)
    return sizes


def _eval_extent(expression: str) -> int | None:
    stripped = expression.strip()
    if not stripped:
        return None
    try:
        tree = ast.parse(stripped, mode="eval")
    except SyntaxError:
        return None

    def evaluate(node: ast.expr) -> int | None:
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = evaluate(node.operand)
            return (
                (value if isinstance(node.op, ast.UAdd) else -value) if value is not None else None
            )
        if isinstance(node, ast.BinOp) and isinstance(
            node.op, (ast.Add, ast.Sub, ast.Mult, ast.FloorDiv)
        ):
            left = evaluate(node.left)
            right = evaluate(node.right)
            if left is None or right is None:
                return None
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            if isinstance(node.op, ast.Mult):
                return left * right
            return left // right if right else None
        return None

    return evaluate(tree.body)


def _base_type_size(type_name: str, sizes: dict[str, int]) -> int | None:
    cleaned = re.sub(r"\s+", " ", type_name).strip()
    cleaned = re.sub(r"^(?:const|volatile|static|class|struct|enum)\s+", "", cleaned)
    cleaned = re.sub(r"\s+(?:const|volatile)$", "", cleaned)
    template_depth = 0
    for char in cleaned:
        if char == "<":
            template_depth += 1
        elif char == ">":
            template_depth -= 1
        elif char in "*&" and template_depth == 0:
            return 4
    template = re.match(
        r"^(W8GrowableVector|W8Vector|W8HashTable|srArray|srHeapBuffer)\s*<", cleaned
    )
    if template:
        return sizes.get(template.group(1), _TEMPLATE_DEFAULT_SIZES.get(template.group(1)))
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
        if extent is None or extent <= 0:
            return None
        size *= extent
    return size


def _array_bound_names(arrays: str) -> set[str]:
    return {
        name
        for bound in _ARRAY_EXTENT.findall(arrays)
        for name in re.findall(r"\b[A-Za-z_]\w*\b", bound)
    }


def _replace_array_bound_names(arrays: str, values: dict[str, int]) -> str:
    return _ARRAY_EXTENT.sub(
        lambda match: (
            "["
            + re.sub(
                r"\b[A-Za-z_]\w*\b",
                lambda name: str(values.get(name.group(0), name.group(0))),
                match.group(1),
            )
            + "]"
        ),
        arrays,
    )


def _local_numeric_array_bounds(lines: list[str], before: int, arrays: str) -> str:
    """Resolve only unconditional same-file integer defines preceding a GLOBAL."""

    names = _array_bound_names(arrays)
    if not names:
        return arrays
    values: dict[str, int | None] = {}
    depth = 0
    for line in lines[:before]:
        if _PREPROCESSOR_IF.match(line):
            depth += 1
            continue
        if _PREPROCESSOR_ENDIF.match(line):
            depth = max(0, depth - 1)
            continue
        defined = _LOCAL_DEFINE.match(line)
        if defined and defined.group(1) in names:
            value = (defined.group(2) or "").strip()
            values[defined.group(1)] = (
                _number(value)
                if depth == 0 and re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|\d+)", value)
                else None
            )
        undefined = _LOCAL_UNDEF.match(line)
        if undefined and undefined.group(1) in names:
            values[undefined.group(1)] = None
    return _replace_array_bound_names(
        arrays, {name: value for name, value in values.items() if value is not None}
    )


def _header_integer_constants(path: Path) -> dict[str, int]:
    """Read unconditional literal defines and enum members from one included header."""

    content = path.read_text(encoding="utf-8", errors="replace")
    lines = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL).splitlines()
    meaningful = [
        line.strip() for line in lines if line.strip() and not line.lstrip().startswith("//")
    ]
    guard = None
    if len(meaningful) > 1:
        opening = re.fullmatch(r"#\s*ifndef\s+([A-Za-z_]\w*)", meaningful[0])
        if opening and re.fullmatch(rf"#\s*define\s+{opening.group(1)}", meaningful[1]):
            guard = opening.group(1)
    values: dict[str, int] = {}
    depth = 0
    in_enum = False
    for line in lines:
        stripped = line.split("//", 1)[0].strip()
        if guard and re.fullmatch(rf"#\s*ifndef\s+{guard}", stripped):
            guard = None
            continue
        if _PREPROCESSOR_IF.match(stripped):
            depth += 1
            continue
        if _PREPROCESSOR_ENDIF.match(stripped):
            depth = max(0, depth - 1)
            continue
        if depth:
            continue
        defined = _LOCAL_DEFINE.match(stripped)
        if defined:
            value = (defined.group(2) or "").strip()
            if re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|\d+)", value):
                values[defined.group(1)] = _number(value)
            continue
        if re.search(r"\benum\b[^;]*\{", stripped):
            in_enum = True
        if in_enum:
            for name, value in _NUMERIC_CONSTANT.findall(stripped):
                values[name] = _number(value)
            if "}" in stripped:
                in_enum = False
    return values


def _included_numeric_array_bounds(
    repo_dir: Path,
    source_file: Path,
    lines: list[str],
    before: int,
    arrays: str,
    header_cache: dict[Path, dict[str, int]],
) -> str:
    """Use unique literal constants from headers directly included before the declaration."""

    names = _array_bound_names(arrays)
    if not names:
        return arrays
    candidates: dict[str, set[int]] = defaultdict(set)
    excluded: set[str] = set()
    prefix = "\n".join(lines[:before])
    for name in names:
        if re.search(rf"\b{re.escape(name)}\s*=", prefix):
            excluded.add(name)
    depth = 0
    for line in lines[:before]:
        defined = _LOCAL_DEFINE.match(line)
        undefined = _LOCAL_UNDEF.match(line)
        mutator = defined if defined is not None else undefined
        if mutator is not None:
            excluded.add(mutator.group(1))
        if _PREPROCESSOR_IF.match(line):
            depth += 1
            continue
        if _PREPROCESSOR_ENDIF.match(line):
            depth = max(0, depth - 1)
            continue
        if depth:
            continue
        included = _QUOTED_INCLUDE.match(line)
        if included is None:
            continue
        spelling = included.group(1)
        paths = [source_file.parent / spelling, repo_dir / "include" / spelling]
        found = [path.resolve() for path in paths if path.is_file()]
        if len(set(found)) != 1:
            continue
        path = found[0]
        if path not in header_cache:
            header_cache[path] = _header_integer_constants(path)
        for name in names:
            if name in header_cache[path]:
                candidates[name].add(header_cache[path][name])
    values = {
        name: next(iter(observed))
        for name, observed in candidates.items()
        if len(observed) == 1 and name not in excluded
    }
    return _replace_array_bound_names(arrays, values)


def _initializer_array_bound(lines: list[str], start: int) -> int | None:
    """Count outer brace elements when the declaration has an inferred first bound."""

    declaration = lines[start].split("//", 1)[0]
    if re.search(r"\[\s*\](?:\s*\[[^\]]+\])*\s*=\s*\{", declaration) is None:
        return None
    source = "\n".join(lines[start : start + 512])
    opening = source.find("{", source.find("="))
    if opening < 0:
        return None
    braces = parentheses = brackets = 0
    count = 0
    element = False
    quote = ""
    escaped = False
    line_comment = block_comment = False
    for index in range(opening, min(len(source), opening + 131072)):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if line_comment:
            if char == "\n":
                line_comment = False
            continue
        if block_comment:
            if char == "*" and following == "/":
                block_comment = False
            continue
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = ""
            continue
        if char == "/" and following == "/":
            line_comment = True
            continue
        if char == "/" and following == "*":
            block_comment = True
            continue
        if char == "#" and not source[source.rfind("\n", 0, index) + 1 : index].strip():
            # Conditional initializer branches do not give a unique bound.
            return None
        if char in "\"'":
            quote = char
            if braces == 1:
                element = True
            continue
        if char == "{":
            braces += 1
            if braces == 2:
                element = True
        elif char == "}":
            if braces == 1:
                return count + int(element) if parentheses == brackets == 0 else None
            braces -= 1
        elif braces == 1:
            if char == "(":
                parentheses += 1
            elif char == ")":
                parentheses -= 1
            elif char == "[":
                # Designated initializers can set a sparse bound.
                if not element and parentheses == 0:
                    return None
                brackets += 1
            elif char == "]":
                brackets -= 1
            elif char == "," and parentheses == brackets == 0:
                if not element:
                    return None
                count += 1
                element = False
            elif not char.isspace():
                element = True
            if parentheses < 0 or brackets < 0:
                return None
    return None


def _strip_comments_and_qualifiers(line: str) -> str:
    return line.split("//", 1)[0].strip()


def _scan_global_comment(lines: list[str], index: int) -> tuple[list[str], int, str] | None:
    """Comments and the first non-comment line after a GLOBAL marker."""

    look = index + 1
    comments: list[str] = []
    while look < len(lines):
        stripped = lines[look].strip()
        if not stripped:
            look += 1
            continue
        if stripped.startswith("//"):
            comments.append(stripped)
            if _SOURCE_MARKER.match(lines[look]) and not _GLOBAL_LINE.match(lines[look]):
                return None
            look += 1
            continue
        return comments, look, _strip_comments_and_qualifiers(lines[look])
    return None


def _extern_declaration(decl: str, parsed: re.Match[str] | None) -> bool:
    """``extern`` without an initializer is a declaration, not a definition."""

    if not decl.startswith("extern "):
        return False
    return parsed is None or not parsed.group(0).rstrip().endswith("=")


def parse_global_definitions(
    repo_dir: Path, sizes: dict[str, int] | None = None
) -> list[dict[str, Any]]:
    """Independently defined ``GLOBAL`` objects, excluding externs and aliases."""

    if sizes is None:
        sizes = known_type_sizes(repo_dir)
    definitions: list[dict[str, Any]] = []
    header_cache: dict[Path, dict[str, int]] = {}
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
                scanned = _scan_global_comment(lines, index)
                if scanned is None:
                    index += 1
                    continue
                comments, look, decl = scanned
                window = " ".join(comments)
                parsed = _DECL.match(decl) or _DIRECT_INIT.match(decl) or _CALLBACK_DECL.match(decl)
                if _extern_declaration(decl, parsed) or _DOCUMENTED_ALIAS.search(window):
                    index = look + 1
                    continue
                if depth > 0:
                    # Function-local static. Same ownership rules still apply when
                    # it carries an independent GLOBAL address, so keep it.
                    pass
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
                if parsed.re is _CALLBACK_DECL:
                    type_name = f"{parsed.group('result').strip()} (*)({parsed.group('args')})"
                else:
                    type_name = parsed.group("prefix").strip()
                name = parsed.group("name")
                arrays = parsed.groupdict().get("arrays") or ""
                numeric_arrays = _local_numeric_array_bounds(lines, look, arrays)
                numeric_arrays = _included_numeric_array_bounds(
                    repo_dir, path, lines, look, numeric_arrays, header_cache
                )
                if numeric_arrays.startswith("[]"):
                    inferred = _initializer_array_bound(lines, look)
                    if inferred is not None and inferred > 0:
                        numeric_arrays = f"[{inferred}]" + numeric_arrays[2:]
                definitions.append(
                    {
                        "name": name,
                        "address": address,
                        "size": _declaration_size(type_name, numeric_arrays, sizes),
                        "type": (type_name + arrays).strip(),
                        "projected_type": (type_name + numeric_arrays).strip(),
                        "source_file": relative,
                        "line": look + 1,
                        "target": target,
                        "location": location,
                        "extern": False,
                    }
                )
                index = look + 1
    return definitions


def unaddressed_globals(repo_dir: Path) -> list[dict[str, Any]]:
    """``GLOBAL`` markers that neither name a retail address nor say unresolved.

    An unaddressed definition can sit inside an addressed aggregate without the
    overlap checker seeing it. Either recover the address or mark the marker
    ``unresolved``.
    """

    violations: list[dict[str, Any]] = []
    roots = (repo_dir / "src", repo_dir / "include")
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp", ".c"}:
                continue
            relative = str(path.relative_to(repo_dir))
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for index, line in enumerate(lines):
                match = _GLOBAL_LINE.match(line)
                if match is None or _GLOBAL_MARKER.match(line):
                    continue
                rest = match.group("rest") or ""
                scanned = _scan_global_comment(lines, index)
                comments = scanned[0] if scanned is not None else []
                window = " ".join([rest, *comments])
                if _UNRESOLVED_GLOBAL.search(window):
                    continue
                name = ""
                decl = ""
                parsed = None
                if scanned is not None:
                    decl = scanned[2]
                    parsed = _DECL.match(decl)
                    if parsed is not None:
                        name = parsed.group("name")
                if _extern_declaration(decl, parsed):
                    continue
                detail = (
                    f"{name or 'global'} has a GLOBAL marker without a retail address; "
                    "resolve the address or mark the marker unresolved"
                )
                violations.append(
                    {
                        "kind": "unaddressed-global",
                        "file": relative,
                        "line": index + 1,
                        "name": name,
                        "detail": detail,
                    }
                )
    return violations


def shadowed_global_definitions(
    repo_dir: Path, definitions: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    """Unmarked definitions that claim a ``GLOBAL``-marked retail address.

    A file-scope variable named ``..._00652dc4`` asserts ownership of that
    address even without a marker. When the address already has a marked
    owner, the unmarked definition is a second source object for the same
    retail storage and must not survive review.
    """

    marked: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
    for item in definitions:
        if item.get("name"):
            marked[(str(item.get("target") or "WIZ8"), int(item["address"]))].append(item)

    from .source_index import project_targets

    roots_by_target: dict[str, tuple[str, ...]] = {}
    for name, config in project_targets(repo_dir).items():
        roots = config.get("source-root", ())
        if isinstance(roots, str):
            roots = (roots,)
        roots_by_target[name] = tuple(roots)

    def target_for(relative: str) -> str:
        for name, source_roots in roots_by_target.items():
            if any(relative.startswith(root.rstrip("/") + "/") for root in source_roots):
                return name
        return "WIZ8"

    violations: list[dict[str, Any]] = []
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
            in_block_comment = False
            for number, line in enumerate(lines, 1):
                text = line
                # Naive block-comment tracking: strings on comment lines are
                # not C++ tokens, so replace comment regions with spaces.
                cleaned = ""
                cursor = 0
                while cursor < len(text):
                    if in_block_comment:
                        end = text.find("*/", cursor)
                        if end < 0:
                            cursor = len(text)
                            continue
                        in_block_comment = False
                        cursor = end + 2
                        continue
                    start = text.find("/*", cursor)
                    line_comment = text.find("//", cursor)
                    if line_comment >= 0 and (start < 0 or line_comment < start):
                        cleaned += text[cursor:line_comment]
                        cursor = len(text)
                        continue
                    if start < 0:
                        cleaned += text[cursor:]
                        break
                    cleaned += text[cursor:start] + "  "
                    in_block_comment = True
                    cursor = start + 2
                code = cleaned
                depth += code.count("{") - code.count("}")
                stripped = code.strip()
                if depth != 0 or not stripped:
                    continue
                if _SOURCE_MARKER.match(stripped):
                    continue
                match = _ADDRESS_SUFFIX_DECL.match(stripped)
                if match is None:
                    continue
                if stripped.startswith(("extern ", "extern\t")):
                    continue
                target = target_for(relative)
                address = int(match.group("address"), 16)
                owners = [
                    owner
                    for owner in marked.get((target, address), [])
                    if not (
                        owner.get("source_file") == relative
                        and int(owner.get("line") or 0) == number
                    )
                ]
                if not owners:
                    continue
                owner = owners[0]
                violations.append(
                    {
                        "kind": "global-address-shadow",
                        "name": match.group("name"),
                        "address": f"0x{address:08x}",
                        "file": relative,
                        "line": number,
                        "detail": (
                            f"{match.group('name')} @ 0x{address:x} duplicates "
                            f"{owner['name']} ({owner['source_file']}:{owner['line']}) "
                            "without a GLOBAL marker"
                        ),
                    }
                )
    return violations


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
                        "file": member.get("source_file") or "",
                        "line": int(member.get("line") or 0),
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
    missing = unaddressed_globals(repo_dir)
    if missing:
        rendered = [f"{item['file']}:{item['line']} {item['detail']}" for item in missing]
        raise GlobalOverlapError(
            "GLOBAL markers have no retail address:\n  " + "\n  ".join(rendered)
        )
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


_STATUS_MEMBER = re.compile(r"\bg_status\.([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)")


def status_member_accesses(repo_dir: Path) -> list[dict[str, Any]]:
    """Source sites that name a g_status member."""

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
GXSTATUS_START = 0x006836B8


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
                kind = "member of g_status"
                container = "g_status"
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
