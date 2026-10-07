"""Pre-portability queues composed into semantic debt, not a second source index.

Lexical hits locate review candidates. Compiler declarations and reviewed
classifications remain separate; none of these observations seed retail types.
"""

from __future__ import annotations

import hashlib
import json
import re
from collections import Counter
from pathlib import Path
from typing import Any

from reccmp.parser.marker import MarkerType, match_marker

ROOTS = (
    "src/wiz8",
    "include/wiz8",
    "src/surrender",
    "include/surrender",
    "src/sgp",
    "src/srext_jpegimporter",
    "src/srext_unzip",
    "include/bink.h",
)
SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".cxx", ".hxx"}
# Match literals before comments so a quoted URL or comment delimiter is not
# mistaken for prose. Blanking preserves offsets and therefore source locations.
_TRIVIA = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/', re.DOTALL)
_INCLUDE = re.compile(r'^[ \t]*#\s*include\s*[<"]([^>"\n]+)[>"]', re.MULTILINE)
_COMPARISON = re.compile(
    r"\b(?:reinterpret|c-style-cast|raw-offset|union|uninit|member-dtor|format-off)-ok:"
)
_TENTATIVE = re.compile(
    r"\bunresolved fragment\b|\bcurrently\b|\bfor now\b|\bprobably\b|"
    r"\bnot yet recovered\b|\b(?:needed|necessary) to match\b",
    re.IGNORECASE,
)
_WIDTH = {
    "long": r"\blong\b",
    "pointer_sized_integer": r"\b(?:size_t|ptrdiff_t|intptr_t|uintptr_t|LONG_PTR|ULONG_PTR|DWORD_PTR)\b",
    "wide_text": r"\b(?:wchar_t|CHAR16|STR16|WCHAR|LPWSTR|LPCWSTR)\b",
    "enum_representation": r"\benum\s+(?:class\s+)?[A-Za-z_]\w*\s*\{",
    "pointer_size_contract": r"\bsizeof\s*\(\s*(?:void\s*\*|[A-Za-z_]\w*\s*\*)\s*\)",
    "window_integer_transport": r"\b(?:GetWindowLongA?|SetWindowLongA?|GWL_USERDATA|GWL_HINSTANCE)\b",
    "integer_reinterpret_cast": r"\breinterpret_cast\s*<\s*(?:(?:unsigned|signed)\s+)?(?:int|long|DWORD|UINT32)\s*>",
}
_COMPILER = {
    "calling_convention": r"\b(?:__cdecl|__stdcall|__fastcall|__thiscall|WINAPI|CALLBACK|PASCAL)\b",
    "declspec": r"\b__declspec\s*\(",
    "pragma": r"^[ \t]*#\s*pragma\b",
    "packing": r"^[ \t]*#\s*pragma\s+pack\b",
    "crt_extension": r"\b_(?:stricmp|strnicmp|wcsicmp|wcsnicmp|snprintf|vsnprintf|splitpath|makepath|findfirst|findnext|access|mkdir|chdir|getcwd|itoa|ultoa|rotl|rotr)\b",
    "inline_assembly": r"\b(?:__asm|_asm)\b",
}
# Entry-point spellings, rather than all Windows API calls. Owners below are
# the existing source component; natural replacement owners live in review data.
_PLATFORM = {
    "window_events": r"\b(?:HWND|WNDCLASS\w*|MSG|WPARAM|LPARAM|LRESULT|CreateWindow\w*|PeekMessage\w*|GetMessage\w*|DispatchMessage\w*|DefWindowProc\w*)\b",
    "directdraw": r"\b(?:IDirectDraw\w*|LPDIRECTDRAW\w*|DDSURFACEDESC\w*|DirectDrawCreate\w*)\b",
    "renderer": r"\b(?:IDirect3D\w*|LPDIRECT3D\w*|Direct3DCreate\w*|srGERD|srVP)\b",
    "input": r"\b(?:GetAsyncKeyState|GetKeyboardState|GetCursorPos|SetCursorPos|DirectInput\w*|IDirectInput\w*)\b",
    "clock": r"\b(?:GetTickCount|QueryPerformanceCounter|QueryPerformanceFrequency|timeGetTime|timeBeginPeriod|Sleep)\b",
    "synchronization": r"\b(?:CRITICAL_SECTION|CreateThread|CreateMutex\w*|CreateEvent\w*|WaitForSingleObject|WaitForMultipleObjects|EnterCriticalSection|Interlocked\w*)\b",
    "filesystem": r"\b(?:FindFirstFile\w*|FindNextFile\w*|WIN32_FIND_DATA\w*|CreateFile\w*|GetFileAttributes\w*|GetFileTime|SetFilePointer|ReadFile|WriteFile|GetCurrentDirectory\w*)\b",
    "registry": r"\b(?:HKEY|RegOpenKey\w*|RegQueryValue\w*|RegSetValue\w*|GetPrivateProfile\w*|WritePrivateProfile\w*)\b",
    "dialogs_debugger": r"\b(?:MessageBox\w*|DialogBox\w*|OutputDebugString\w*|DebugBreak|IsDebuggerPresent)\b",
    "bink": r"\b(?:HBINK|Bink[A-Z]\w*)\b",
    "miles": r"\b(?:AIL_\w+|HSAMPLE|HDIGDRIVER|HSTREAM|HMUSIC|HSEQUENCE)\b",
    "jpeg": r"\b(?:jpeg_\w+|jpeglib)\b",
    "zlib_unzip": r"\b(?:inflate\w*|deflate\w*|z_stream|unz\w*|zlib)\b",
}
_PUBLIC_TOKEN = re.compile(
    r"\b(?:HWND|HINSTANCE|HANDLE|HDC|HKEY|RECT|POINT|DWORD|WORD|BYTE|BOOL|FILETIME|SYSTEMTIME|"
    r"WPARAM|LPARAM|LRESULT|IDirectDraw\w*|IDirect3D\w*|LPDIRECT\w*|DDSURFACEDESC\w*)\b"
)
_LAYOUT = re.compile(r"\b(?:sizeof|offsetof)\s*\(\s*([A-Za-z_]\w*(?:::\w+)*)")
_ASSERTION = re.compile(r"\bstatic_assert\s*\([^;]+;", re.DOTALL)
_BUILTIN_TYPES = frozenset(
    {
        "void",
        "bool",
        "char",
        "wchar_t",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
    }
)


def _blank(match: re.Match[str]) -> str:
    return re.sub(r"[^\n]", " ", match.group())


def _recovery_marker(comment: str) -> bool:
    """Use reccmp's grammar, including secondary-base vtable identities."""
    marker = match_marker(comment)
    return marker is not None and marker.type != MarkerType.UNKNOWN


def strip_recovery_markers(text: str) -> str:
    """Drop only standalone identity lines; retain prose and exception rationale.

    Not a portable exporter: source remains in this checkout. Used to establish
    the mechanical boundary without treating addresses in useful prose as trash.
    """
    spans = [
        match.span()
        for match in _TRIVIA.finditer(text)
        if _recovery_marker(match.group().strip())
        and not text[text.rfind("\n", 0, match.start()) + 1 : match.start()].strip()
    ]
    for start, end in reversed(spans):
        text = text[:start] + re.sub(r"[^\n]", "", text[start:end]) + text[end:]
    return text


def _owner(path: str) -> str:
    if path == "include/bink.h":
        return "bink"
    parts = path.split("/")
    return parts[1]


def _hits(path: str, code: str, patterns: dict[str, str]) -> list[dict[str, Any]]:
    rows = []
    for kind, pattern in patterns.items():
        for match in re.finditer(pattern, code, re.MULTILINE):
            rows.append(
                {
                    "source_file": path,
                    "line": code.count("\n", 0, match.start()) + 1,
                    "kind": kind,
                    "token": match.group().strip(),
                    "source_owner": _owner(path),
                    "status": "lexical_candidate",
                }
            )
    return sorted(rows, key=lambda row: (row["source_file"], row["line"], row["kind"]))


def _assembly_sites(path: str, code: str) -> list[dict[str, Any]]:
    rows = []
    for match in re.finditer(r"\b(?:__asm|_asm)\s*\{", code):
        end = code.find("}", match.end())
        body = code[match.end() : end] if end >= 0 else code[match.end() :]
        instructions = set(
            re.findall(
                r"\b(?:rdtsc|cpuid|pushfd|popfd|fistp?|fldcw|fstcw|fnstcw|mm\d|xmm\d)\b",
                body,
                re.IGNORECASE,
            )
        )
        families = []
        if instructions & {"rdtsc", "cpuid", "pushfd", "popfd"}:
            families.append("cpu_clock_or_feature_query")
        if instructions & {"fist", "fistp", "fldcw", "fstcw", "fnstcw"}:
            families.append("x87_rounding_or_control_word")
        if any(token.startswith(("mm", "xmm")) for token in instructions):
            families.append("simd")
        rows.append(
            {
                "source_file": path,
                "line": code.count("\n", 0, match.start()) + 1,
                "source_owner": _owner(path),
                "instruction_families": families,
                "replacement_status": "requires_instruction_and_behavior_review",
            }
        )
    return rows


def _lifetime_reviews(
    repository: Path, index: dict[str, Any], reviewed: dict[str, Any]
) -> dict[tuple[str, str, str], dict[str, Any]]:
    """Bind source-model contracts to exact declarations and frozen source inputs.

    Matching a source snapshot does not independently prove retail ownership.
    Missing/stale/conflicting observations remain in the review queue.
    """
    declarations: dict[tuple[str, str, str], set[str]] = {}
    for record in index.get("classes", []):
        for field in record.get("fields", []):
            key = (
                str(record.get("qualified_name") or ""),
                str(field.get("name") or ""),
                str(field.get("source_file") or ""),
            )
            declarations.setdefault(key, set()).add(str(field.get("type") or ""))
    evidence = reviewed.get("lifetime_source_snapshots", {})
    snapshot_status = {}
    for path, expected in evidence.items():
        file = repository / path
        snapshot_status[path] = (
            file.is_file() and hashlib.sha256(file.read_bytes()).hexdigest() == expected
        )
    results = {}
    for review in reviewed.get("lifetime_fields", []):
        key = (review["record"], review["field"], review["source_file"])
        if key in results:
            raise ValueError(f"Duplicate lifetime field review: {key}")
        types = declarations.get(key, set())
        inputs = review.get("source_evidence", [])
        status = (
            "declaration_not_observed"
            if not types
            else "conflicting_declarations"
            if len(types) != 1
            else "declaration_changed"
            if types != {review["type"]}
            else "source_evidence_changed"
            if key[2] not in inputs or any(not snapshot_status.get(path, False) for path in inputs)
            else "source_model_review"
        )
        results[key] = {
            **review,
            "status": status,
            "observed_types": sorted(types),
            "provenance": "recovered_source_contract",
            "retail_equivalence": "not_established_by_this_report",
        }
    return results


def portability_queues(repository: Path, index: dict[str, Any]) -> dict[str, Any]:
    """Read existing source/projection; never compile, capture or guess layouts."""
    config_path = repository / "config/pre-portability.json"
    reviewed = json.loads(config_path.read_text()) if config_path.is_file() else {}
    width, compiler, platforms, headers, comments, layouts, assembly = [], [], [], [], [], [], []
    source_count = 0
    seen_layouts = set()
    layout_reviews = {
        (row["source_file"], row["record"]): row for row in reviewed.get("layouts", [])
    }
    declarations: dict[str, dict[str, dict[str, Any]]] = {}
    for record in index.get("classes", []):
        declarations.setdefault(str(record.get("qualified_name") or ""), {})[
            str(record.get("source_file") or "")
        ] = record
    for root_name in ROOTS:
        root = repository / root_name
        for file in sorted(root.rglob("*")) if root.is_dir() else [root]:
            if not file.is_file() or file.suffix.lower() not in SUFFIXES:
                continue
            source_count += 1
            path = file.relative_to(repository).as_posix()
            text = file.read_text(encoding="utf-8", errors="replace")
            code = _TRIVIA.sub(_blank, text)
            width.extend(_hits(path, code, _WIDTH))
            compiler.extend(_hits(path, code, _COMPILER))
            platforms.extend(_hits(path, code, _PLATFORM))
            assembly.extend(_assembly_sites(path, code))
            if file.suffix.lower() in {".h", ".hpp", ".hxx"}:
                tokens = sorted(set(_PUBLIC_TOKEN.findall(code)))
                # Includes need their string literal, but comments must be blank.
                includes = _INCLUDE.findall(
                    _TRIVIA.sub(
                        lambda m: _blank(m) if m.group().startswith(("/",)) else m.group(),
                        text,
                    )
                )
                if tokens or any(
                    re.search(
                        r"windows|ddraw|d3d|wiz8_windows|wiz8_directdraw", item, re.IGNORECASE
                    )
                    for item in includes
                ):
                    headers.append(
                        {
                            "source_file": path,
                            "source_owner": _owner(path),
                            "platform_tokens": tokens,
                            "direct_includes": includes,
                            "status": "public_header_review",
                        }
                    )
            for match in _TRIVIA.finditer(text):
                value = match.group()
                if not value.startswith(("/",)):
                    continue
                category = (
                    "recovery_metadata"
                    if _recovery_marker(value.strip())
                    else "comparison_metadata_with_rationale"
                    if _COMPARISON.search(value)
                    else "tentative_prose"
                    if _TENTATIVE.search(value)
                    else None
                )
                if category:
                    comments.append(
                        {
                            "source_file": path,
                            "line": text.count("\n", 0, match.start()) + 1,
                            "category": category,
                            "text": value.strip(),
                        }
                    )
            layout_matches = [
                (contract.start() + match.start(), match.group(1))
                for contract in _ASSERTION.finditer(code)
                for match in _LAYOUT.finditer(contract.group())
            ]
            for offset, name in layout_matches:
                if name in _BUILTIN_TYPES:
                    continue
                # The assertion's file need not declare its record. Use only
                # exact compiler names with one visible declaration owner;
                # lexical aliases, nested names and templates remain candidates.
                visible = set(index.get("unit_dependencies", {}).get(path, [])) | {path}
                owners = declarations.get(name, {})
                resolved = [record for owner, record in owners.items() if owner in visible]
                declaration = resolved[0] if len(resolved) == 1 else None
                owner = str(declaration["source_file"]) if declaration else path
                if (owner, name) in seen_layouts:
                    continue
                seen_layouts.add((owner, name))
                review = layout_reviews.get((owner, name))
                layouts.append(
                    {
                        "record": name,
                        "source_file": owner,
                        "line": declaration.get("line")
                        if declaration
                        else code.count("\n", 0, offset) + 1,
                        "assertion_location": f"{path}:{code.count(chr(10), 0, offset) + 1}",
                        "evidence": "source_index_declaration_owner"
                        if declaration
                        else "lexical_layout_candidate",
                        "classification": review["classification"] if review else "unclassified",
                        "review": review,
                    }
                )
    lifetime_reviews = _lifetime_reviews(repository, index, reviewed)
    pointers = []
    seen_fields = set()
    for record in index.get("classes", []):
        # The source index owns contracts declared in included evidence fragments.
        # Keep their canonical declaration owner instead of guessing it from .inc names.
        path = str(record.get("source_file") or "")
        name = str(record.get("qualified_name") or "")
        if (
            record.get("asserted_size") is not None
            and path.startswith(tuple(root + "/" for root in ROOTS))
            and (path, name) not in seen_layouts
        ):
            seen_layouts.add((path, name))
            review = layout_reviews.get((path, name))
            layouts.append(
                {
                    "record": name,
                    "source_file": path,
                    "line": record.get("line"),
                    "classification": review["classification"] if review else "unclassified",
                    "review": review,
                    "evidence": "source_index_layout_contract",
                }
            )
        for field in record.get("fields", []):
            path = str(field.get("source_file") or "")
            if not path.startswith(tuple(root + "/" for root in ROOTS)):
                continue
            spelling = str(field.get("type") or "")
            if "*" not in spelling and not re.search(r"\bsrPtr\s*<", spelling):
                continue
            key = (record.get("qualified_name"), field.get("name"), path)
            if key in seen_fields:
                continue
            seen_fields.add(key)
            review = lifetime_reviews.get(key)
            active_review = review if review and review["status"] == "source_model_review" else None
            pointers.append(
                {
                    "record": record.get("qualified_name"),
                    "field": field.get("name"),
                    "type": spelling,
                    "source_file": path,
                    "line": field.get("line"),
                    "ownership": active_review["ownership"] if active_review else "requires_review",
                    "lifetime_review": review,
                }
            )
    return {
        "coverage": {
            "source_files": source_count,
            "roots": list(ROOTS),
            "limitations": "lexical candidates plus supplied source-index fields; field contracts validate frozen recovered-source inputs, not retail equivalence; aliases and indirect calls need review",
        },
        "summary": {
            "reviewed_lifetime_fields": sum(
                row["status"] == "source_model_review" for row in lifetime_reviews.values()
            ),
            "abi_width_sites": len(width),
            "compiler_boundary_sites": len(compiler),
            "platform_dependency_sites": len(platforms),
            "platform_public_headers": len(headers),
            "tentative_comments": sum(row["category"] == "tentative_prose" for row in comments),
            "unclassified_layout_candidates": sum(
                row["classification"] == "unclassified" for row in layouts
            ),
            "pointer_fields_requiring_review": sum(
                row["ownership"] == "requires_review" for row in pointers
            ),
        },
        "abi_width_sites": width,
        "compiler_boundary_sites": compiler,
        "inline_assembly_blocks": assembly,
        "platform_dependency_sites": platforms,
        "platform_subsystems": dict(sorted(Counter(row["kind"] for row in platforms).items())),
        "platform_owners": {
            kind: {
                "source_owners": dict(
                    sorted(
                        Counter(
                            row["source_owner"] for row in platforms if row["kind"] == kind
                        ).items()
                    )
                ),
                "replacement_boundary": reviewed.get("platform_boundaries", {}).get(
                    kind, "requires_review"
                ),
            }
            for kind in sorted({row["kind"] for row in platforms})
        },
        "public_headers": headers,
        "comment_metadata": comments,
        "layout_candidates": layouts,
        "pointer_fields": pointers,
        "lifetime_field_reviews": list(lifetime_reviews.values()),
        "reviewed_boundaries": reviewed,
        "fork_status": "blocked",
        "fork_blockers": {
            "inactive_lifetime_reviews": sum(
                row["status"] != "source_model_review" for row in lifetime_reviews.values()
            ),
            "unreviewed_abi_sites": len(width),
            "unclassified_layout_candidates": sum(
                row["classification"] == "unclassified" for row in layouts
            ),
            "pointer_fields_requiring_review": sum(
                row["ownership"] == "requires_review" for row in pointers
            ),
            "missing_behavioral_references": [
                row["observation"]
                for row in reviewed.get("behavioral_references", [])
                if not row.get("reference")
            ],
            "undecided_components": [
                row["component"]
                for row in reviewed.get("components", [])
                if row["decision"] == "undecided"
            ],
            "review_required": "candidate counts are not confirmed defects; the fork gate also needs the semantic/TU queues and reviewed mismatch causes",
        },
    }
