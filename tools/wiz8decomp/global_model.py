"""Check retail storage ownership using reccmp's compiler-backed source index."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path
from typing import Any

from reccmp.parser.marker import MarkerType, match_marker
from reccmp.parser.reader import MarkerBlock

from .source_index import SourceIndexError, load_source_index

_DOCUMENTED_ALIAS = re.compile(r"alias(?:es)?\s+(?:of|for)\b|no separate definition", re.IGNORECASE)
_GLOBAL_LINE = re.compile(r"^\s*//\s*GLOBAL(?:\s*:.*)?\s*$", re.IGNORECASE)
_UNRESOLVED = re.compile(r"\bunresolved(?:-global)?\b", re.IGNORECASE)
_ADDRESS_SUFFIX = re.compile(r"_([0-9a-f]{8})$", re.IGNORECASE)


def indexed_global_definitions(document: dict[str, Any]) -> list[dict[str, Any]]:
    """Use compiler-bound marker anchors, including internal and static-local storage."""
    definitions = []
    seen = set()
    for raw in document["marker_blocks"]:
        block = MarkerBlock.from_dict(raw)
        if block.anchor is None:
            continue  # The marker linter diagnoses unbound annotations.
        if _DOCUMENTED_ALIAS.search(" ".join(c.text for c in block.comments)):
            continue
        candidates = block.anchor.of_kind("variable")
        for comment in block.comments:
            marker = match_marker(comment.text)
            if marker is None or marker.type != MarkerType.GLOBAL:
                continue
            for variable in candidates:
                if not variable.is_definition:
                    continue
                if variable.type is None:
                    raise SourceIndexError(
                        "global storage facts are missing; rebuild the source index"
                    )
                key = (
                    marker.module,
                    marker.offset,
                    block.source_file,
                    variable.semantic_id,
                    variable.type,
                    variable.size,
                )
                if key in seen:
                    continue
                seen.add(key)
                definitions.append(
                    {
                        "target": marker.module.upper(),
                        "address": marker.offset,
                        "name": variable.qualified_name,
                        "type": variable.type,
                        "size": variable.size,
                        "semantic_id": variable.semantic_id,
                        "source_file": block.source_file,
                        "line": variable.line,
                    }
                )
    return definitions


def overlapping_globals(definitions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Report each pair once; unknown extents still conflict at the same start."""
    by_target: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in definitions:
        by_target[item["target"]].append(item)
    violations = []
    for target, items in by_target.items():
        ordered = sorted(items, key=lambda item: (item["address"], item["name"]))
        for index, outer in enumerate(ordered):
            end = outer["address"] + (outer["size"] or 0)
            for inner in ordered[index + 1 :]:
                if inner["address"] != outer["address"] and inner["address"] >= end:
                    break
                offset = inner["address"] - outer["address"]
                violations.append(
                    {
                        "kind": "global-overlap",
                        "target": target,
                        "file": inner["source_file"],
                        "line": inner["line"],
                        "detail": f"{inner['name']} @ 0x{inner['address']:x} overlaps "
                        f"{outer['name']} + 0x{offset:x}.",
                    }
                )
    return violations


def global_violations(repository: Path) -> list[dict[str, Any]]:
    document = load_source_index(repository)
    definitions = indexed_global_definitions(document)
    violations = overlapping_globals(definitions)
    for raw in document["marker_blocks"]:
        block = MarkerBlock.from_dict(raw)
        comments = " ".join(c.text for c in block.comments)
        if _UNRESOLVED.search(comments):
            continue
        variables = block.anchor.of_kind("variable") if block.anchor else []
        if variables and not any(v.is_definition for v in variables):
            continue
        for comment in block.comments:
            if _GLOBAL_LINE.match(comment.text) and match_marker(comment.text) is None:
                violations.append(
                    {
                        "kind": "unaddressed-global",
                        "file": block.source_file,
                        "line": comment.line,
                        "detail": "GLOBAL has no retail address; resolve it or mark it unresolved",
                    }
                )

    owners: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
    for item in definitions:
        owners[item["target"], item["address"]].append(item)
    seen = set()
    for variable in document["variables"]:
        if variable["definition_kind"] == "declaration":
            continue
        match = _ADDRESS_SUFFIX.search(variable["qualified_name"])
        if match is None:
            continue
        target = variable["target"]
        address = int(match[1], 16)
        for owner in owners.get((target, address), []):
            location = (variable["source_file"], variable["line"])
            if location == (owner["source_file"], owner["line"]):
                continue
            key = (target, *location, address)
            if key in seen:
                continue
            seen.add(key)
            violations.append(
                {
                    "kind": "global-address-shadow",
                    "file": location[0],
                    "line": location[1],
                    "detail": f"{variable['qualified_name']} @ 0x{address:x} duplicates "
                    f"{owner['name']} ({owner['source_file']}:{owner['line']}) "
                    "without a GLOBAL marker",
                }
            )
    return violations
