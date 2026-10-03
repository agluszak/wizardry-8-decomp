"""Hard source-model invariants for recovered Wizardry C++.

These checks cover constructs that should never be reintroduced once the typed
source model can express them directly:

* compiler/template output must remain emission provenance, not authored bodies;
* calls and shared helpers belong to source; inlining belongs to the compiler;
* literal byte offsets into repository-typed objects must use named fields;
* recovered callables must use real declarations, not inline function-pointer
  reinterpret casts;
* semantic callable names must not encode retail addresses;
* an object allocated with `new T[n]` must not be released by scalar `delete`;
  on trivially destructible elements the spellings emit identical code, so a
  scalar delete is always the wrong authored form.

Unlike the diff-scoped cast hygiene gate, these are whole-tree invariants with
no source comment waiver.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .source_index import (
    declaration_for_marker,
    declarations_by_semantic_key,
    load_source_index,
)
from .source_units import load_source_unit_document

_RECOVERED_ROOTS = ("src/wiz8", "include/wiz8", "src/surrender", "include/surrender")
_CPP_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})
_MARKER_ONLY_KINDS = frozenset({"SYNTHETIC", "LIBRARY"})

_NOISE = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)
_PROJECT_TYPE = (
    r"(?:W8|sr|st)[A-Z][A-Za-z0-9_]*"
    r"(?:\s*<[^;{}()\n]*>)?"
    r"(?:::[A-Za-z_][A-Za-z0-9_]*)?"
)
_PROJECT_POINTER_DECL = re.compile(
    rf"\b(?:const\s+)?{_PROJECT_TYPE}\s*(?:const\s*)?\*\s*(?:const\s+)?"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
)
_PROJECT_REFERENCE_DECL = re.compile(
    rf"\b(?:const\s+)?{_PROJECT_TYPE}\s*(?:const\s*)?&\s*"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)"
)
_REINTERPRET_RAW_OFFSET = re.compile(
    r"reinterpret_cast\s*<\s*(?:const\s+)?(?:unsigned\s+)?char\s*\*\s*>\s*"
    r"\(\s*(?P<base>this|[A-Za-z_][A-Za-z0-9_]*|&\s*[A-Za-z_][A-Za-z0-9_]*)\s*\)"
    r"\s*(?:\+\s*(?:0[xX][0-9A-Fa-f]+|\d+)\b\s*)+"
)
_C_STYLE_RAW_OFFSET = re.compile(
    r"\(\s*(?:const\s+)?(?:unsigned\s+)?char\s*\*\s*\)\s*"
    r"(?P<base>this|[A-Za-z_][A-Za-z0-9_]*|&\s*[A-Za-z_][A-Za-z0-9_]*)"
    r"\s*(?:\+\s*(?:0[xX][0-9A-Fa-f]+|\d+)\b\s*)+"
)
_INLINE_FUNCTION_POINTER_CAST = re.compile(
    r"reinterpret_cast\s*<\s*"
    r"(?P<type>(?:(?!>).){0,600}\(\s*"
    r"(?:(?:__cdecl|__stdcall|__fastcall|__thiscall)\s*)?\*\s*\)"
    r"(?:(?!>).){0,600})>\s*\(",
    re.DOTALL,
)
_COMPILER_SYNTHETIC_NAME = re.compile(
    r"\`(?:scalar|vector) deleting destructor'|\`vtordisp\b|\`adjustor\{",
    re.IGNORECASE,
)
_ADDRESS_SUFFIXED_CALLABLE = re.compile(r"[A-Za-z_][A-Za-z0-9_:<>~]*[0-9][0-9A-Fa-f]{5,7}$")
_ADDRESS_PLACEHOLDER = re.compile(r"^Function[0-9A-Fa-f]{6,9}$")
_ARRAY_ALLOCATION = re.compile(r"\b(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*new\s+[^;=\n(\[]*\[")
_SCALAR_DELETE = re.compile(
    r"\bdelete\s+(?!\[)\s*(?:(?:this|[A-Za-z_][A-Za-z0-9_]*)\s*->\s*)?"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*;"
)
_INLINE_CONTROLS = (
    ("forceinline", re.compile(r"\b__forceinline\b")),
    ("noinline", re.compile(r"\b__declspec\s*\(\s*noinline\s*\)")),
    (
        "inline-control-pragma",
        re.compile(
            r"^[ \t]*#\s*pragma\s+(?:auto_inline|inline_depth|inline_recursion)\b", re.MULTILINE
        ),
    ),
    ("codegen-inline-name", re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*Inline\b")),
)


class SourceModelGateError(RuntimeError):
    """Recovered source contains compiler lowering or type-model escape hatches."""


def _mask_cpp_noise(source: str) -> str:
    def mask(match: re.Match[str]) -> str:
        return "".join("\n" if char == "\n" else " " for char in match.group())

    return _NOISE.sub(mask, source)


def _source_files(repository: Path, roots: tuple[str, ...] = _RECOVERED_ROOTS) -> list[Path]:
    files: list[Path] = []
    for root_name in roots:
        root = repository / root_name
        if not root.is_dir():
            continue
        files.extend(
            path
            for path in root.rglob("*")
            if path.is_file() and path.suffix.casefold() in _CPP_SUFFIXES
        )
    return sorted(set(files))


def _inline_control_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for path in _source_files(repository, _RECOVERED_ROOTS):
        source = path.read_text(encoding="utf-8", errors="ignore")
        masked = _mask_cpp_noise(source)
        for kind, pattern in _INLINE_CONTROLS:
            for match in pattern.finditer(masked):
                violations.append(
                    {
                        "kind": kind,
                        "file": path.relative_to(repository).as_posix(),
                        "line": _line(source, match.start()),
                        "detail": _snippet(source, match.start(), match.end()),
                    }
                )
    return violations


def _line(source: str, offset: int) -> int:
    return source.count("\n", 0, offset) + 1


def _snippet(source: str, start: int, end: int) -> str:
    return " ".join(source[start:end].split())[:220]


def _project_typed_names(masked: str) -> tuple[set[str], set[str]]:
    pointers = {match.group("name") for match in _PROJECT_POINTER_DECL.finditer(masked)}
    references = {match.group("name") for match in _PROJECT_REFERENCE_DECL.finditer(masked)}
    return pointers, references


def _typed_raw_offset_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for path in _source_files(repository):
        source = path.read_text(encoding="utf-8", errors="ignore")
        masked = _mask_cpp_noise(source)
        pointers, references = _project_typed_names(masked)
        for pattern in (_REINTERPRET_RAW_OFFSET, _C_STYLE_RAW_OFFSET):
            for match in pattern.finditer(masked):
                base = re.sub(r"\s+", "", match.group("base"))
                typed = base == "this"
                if base.startswith("&"):
                    typed = base[1:] in references
                elif base != "this":
                    typed = base in pointers
                if not typed:
                    continue
                violations.append(
                    {
                        "kind": "typed-object-raw-offset",
                        "file": path.relative_to(repository).as_posix(),
                        "line": _line(source, match.start()),
                        "detail": _snippet(source, match.start(), match.end()),
                    }
                )
    return violations


def _callable_cast_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for path in _source_files(repository):
        source = path.read_text(encoding="utf-8", errors="ignore")
        masked = _mask_cpp_noise(source)
        for match in _INLINE_FUNCTION_POINTER_CAST.finditer(masked):
            violations.append(
                {
                    "kind": "callable-reinterpret-cast",
                    "file": path.relative_to(repository).as_posix(),
                    "line": _line(source, match.start()),
                    "detail": _snippet(source, match.start(), match.end()),
                }
            )
    return violations


def _address_suffixed_callable_violations(repository: Path) -> list[dict[str, Any]]:
    """Semantic recovered callables use names; markers retain retail addresses."""

    index = load_source_index(repository)
    violations: list[dict[str, Any]] = []
    seen: set[tuple[str, str, int]] = set()
    for item in index.get("declarations") or ():
        if not item.get("is_definition"):
            continue
        if str(item.get("target") or "").upper() not in {"WIZ8", "SURRENDER"}:
            continue

        name = str(item.get("qualified_name") or "")
        leaf = name.rsplit("::", 1)[-1]
        if not _ADDRESS_SUFFIXED_CALLABLE.fullmatch(name) or _ADDRESS_PLACEHOLDER.fullmatch(leaf):
            continue

        owner = str(item.get("owning_class") or "")
        owner_leaf = owner.rsplit("::", 1)[-1]
        if owner_leaf and leaf in {owner_leaf, f"~{owner_leaf}"}:
            continue

        source_file = str(item.get("source_file") or "")
        line = int(item.get("line") or 0)
        key = (name, source_file, line)
        if key in seen:
            continue
        seen.add(key)
        violations.append(
            {
                "kind": "address-suffixed-callable",
                "file": source_file,
                "line": line,
                "detail": name,
            }
        )
    return violations


def _scalar_delete_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for path in _source_files(repository):
        source = path.read_text(encoding="utf-8", errors="ignore")
        masked = _mask_cpp_noise(source)
        array_allocated = {match.group("name") for match in _ARRAY_ALLOCATION.finditer(masked)}
        for match in _SCALAR_DELETE.finditer(masked):
            if match.group("name") not in array_allocated:
                continue
            violations.append(
                {
                    "kind": "scalar-delete-of-array-allocation",
                    "file": path.relative_to(repository).as_posix(),
                    "line": _line(source, match.start()),
                    "detail": _snippet(source, match.start(), match.end()),
                }
            )
    return violations


def _template_emission(declaration: dict[str, Any]) -> bool:
    owner = str(declaration.get("owning_class") or "")
    semantic_id = str(declaration.get("semantic_id") or "")
    return (
        "<" in owner or "type-parameter-" in semantic_id or semantic_id.startswith(("??$?", "??$"))
    )


def _compiler_emission_violations(repository: Path) -> list[dict[str, Any]]:
    index = load_source_index(repository)
    declarations_by_key = declarations_by_semantic_key(index)
    units = load_source_unit_document(repository)
    compiler_files = {str(path) for path in units.get("compiler-emission") or ()}
    violations: list[dict[str, Any]] = []

    for marker in index.get("markers") or ():
        kind = str(marker.get("marker_kind") or "")
        source_file = str(marker.get("source_file") or "")
        line = int(marker.get("line") or 0)
        declaration = declaration_for_marker(marker, declarations_by_key)
        name = str(marker.get("marker_name") or declaration.get("qualified_name") or "")

        if kind == "FUNCTION" and declaration and _template_emission(declaration):
            violations.append(
                {
                    "kind": "template-emission-as-function",
                    "file": source_file,
                    "line": line,
                    "detail": f"{name or declaration.get('semantic_id')} is template output",
                }
            )

        if name and _COMPILER_SYNTHETIC_NAME.search(name) and kind != "SYNTHETIC":
            violations.append(
                {
                    "kind": "compiler-helper-as-authored",
                    "file": source_file,
                    "line": line,
                    "detail": f"{name} is {kind or 'unmarked'}, expected SYNTHETIC",
                }
            )

        if kind in _MARKER_ONLY_KINDS and declaration:
            violations.append(
                {
                    "kind": "emission-marker-binds-declaration",
                    "file": source_file,
                    "line": line,
                    "detail": f"{kind} {name or marker.get('address')} binds an authored declaration",
                }
            )

        if source_file in compiler_files and kind in {"FUNCTION", "GLOBAL"}:
            violations.append(
                {
                    "kind": "authored-marker-in-compiler-unit",
                    "file": source_file,
                    "line": line,
                    "detail": f"compiler-emission TU contains {kind}",
                }
            )

    for declaration in index.get("declarations") or ():
        source_file = str(declaration.get("source_file") or "")
        if source_file not in compiler_files or not declaration.get("is_definition"):
            continue
        violations.append(
            {
                "kind": "authored-definition-in-compiler-unit",
                "file": source_file,
                "line": int(declaration.get("line") or 0),
                "detail": str(
                    declaration.get("qualified_name") or declaration.get("semantic_id") or ""
                ),
            }
        )

    for variable in index.get("variables") or ():
        source_file = str(variable.get("source_file") or "")
        if source_file not in compiler_files:
            continue
        if str(variable.get("definition_kind") or "") == "declaration":
            continue
        violations.append(
            {
                "kind": "authored-data-in-compiler-unit",
                "file": source_file,
                "line": int(variable.get("line") or 0),
                "detail": str(variable.get("qualified_name") or variable.get("semantic_id") or ""),
            }
        )

    return violations


def source_model_violations(repository: Path) -> list[dict[str, Any]]:
    return [
        *_compiler_emission_violations(repository),
        *_typed_raw_offset_violations(repository),
        *_callable_cast_violations(repository),
        *_address_suffixed_callable_violations(repository),
        *_scalar_delete_violations(repository),
        *_inline_control_violations(repository),
    ]


def validate_source_model(repository: Path) -> dict[str, Any]:
    violations = source_model_violations(repository)
    if violations:
        rendered = "\n  ".join(
            f"{item['file']}:{item['line']} {item['kind']}: {item['detail']}" for item in violations
        )
        raise SourceModelGateError(
            "source-model hard gate failed; fix the typed/authored model rather than "
            "waiving compiler lowering:\n  " + rendered
        )
    return {
        "ok": True,
        "gate": "source-model-hard-invariants",
        "rules": [
            "compiler-emission-ownership",
            "typed-object-raw-offsets",
            "callable-reinterpret-casts",
            "address-suffixed-callables",
            "scalar-delete-of-array-allocation",
            "compiler-owned-inlining",
        ],
    }
