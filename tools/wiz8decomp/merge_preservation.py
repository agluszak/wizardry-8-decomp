"""Compare retail-address marker identities between two revisions.

Ordinary merging can silently turn a recovered FUNCTION back into a bare
declaration, duplicate an address under two names, or rename an identity
without moving its callers. The check reads the matching markers from
``src/`` and ``include/`` at a base and a head revision and reports, per
address: removed markers, changed identities, duplicated addresses,
FUNCTION/GLOBAL definitions demoted to bare declarations, and removed
FUNCTION addresses whose name is still referenced by the head tree
(newly unresolved call targets).

The comparison is revision-based so it runs without switching the checkout.
Known source-model refinements are recognized directly: recovered functions may
be reclassified as compiler/library emissions at the same retail address, and a
former GLOBAL may be absorbed by the known extent of a larger GLOBAL object.
Everything else remains a preservation failure; there is no waiver list.
"""

from __future__ import annotations

import hashlib
import io
import re
import subprocess
import tempfile
from collections import defaultdict
from functools import lru_cache
from pathlib import Path
from typing import Any

MARKER_KINDS = (
    "FUNCTION",
    "GLOBAL",
    "STRING",
    "VTABLE",
    "TEMPLATE",
    "SYNTHETIC",
    "LIBRARY",
    "STUB",
)
IDENTITY_KINDS = frozenset({"FUNCTION", "GLOBAL", "VTABLE"})
PRESERVING_RECLASSIFICATIONS = {
    "FUNCTION": frozenset({"TEMPLATE", "SYNTHETIC", "LIBRARY"}),
    "GLOBAL": frozenset({"STRING"}),
}
SOURCE_ROOTS = ("src", "include")
SOURCE_SUFFIXES = (".c", ".cpp", ".h", ".hpp")

_MARKER = re.compile(
    r"^\s*//\s*(?P<kind>FUNCTION|GLOBAL|STRING|VTABLE|TEMPLATE|SYNTHETIC|LIBRARY|STUB):\s*"
    r"(?P<target>[A-Za-z0-9_]+)\s+(?P<address>0x[0-9A-Fa-f]+)",
)
_DECLARATOR = re.compile(r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(?:\(|=|;|\[)")

Identity = tuple[str, str, int]


@lru_cache(maxsize=8)
def _git_prefix(repo_dir: Path) -> tuple[str, ...]:
    if (repo_dir / ".jj").is_dir():
        root = subprocess.run(
            ["jj", "git", "root"], cwd=repo_dir, capture_output=True, text=True, check=True
        ).stdout.strip()
        return ("git", f"--git-dir={root}")
    return ("git",)


def _git(repo_dir: Path, *args: str) -> str:
    return subprocess.run(
        [*_git_prefix(repo_dir), *args],
        cwd=repo_dir,
        capture_output=True,
        text=True,
        check=True,
        errors="replace",
    ).stdout


def _resolve_commit(repo_dir: Path, revision: str) -> str:
    """Resolve a Jujutsu revset or a Git revision to a commit id."""

    if (repo_dir / ".jj").is_dir():
        if revision.startswith("origin/"):
            revision = f"{revision.removeprefix('origin/')}@origin"
        return subprocess.run(
            ["jj", "log", "-r", revision, "--no-graph", "-T", "commit_id"],
            cwd=repo_dir,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    if revision.endswith("@origin"):
        revision = f"origin/{revision.removesuffix('@origin')}"
    return _git(repo_dir, "rev-parse", "--verify", f"{revision}^{{commit}}").strip()


def base_ancestry_report(repo_dir: Path, base: str, head: str | None = None) -> dict[str, Any]:
    """Verify ``base`` is an ancestor of ``head`` and report divergence.

    A stale recovery branch puts one new commit on an old base ancestor while
    current main advanced; comparing such a head against the fetched base would
    silently replay obsolete recoveries. The report carries the resolved base,
    merge base and ahead/behind counts, plus the files each side changed since
    the merge base when the branch has diverged.
    """

    if head is None:
        head = "@" if (repo_dir / ".jj").is_dir() else "HEAD"
    head_sha = _resolve_commit(repo_dir, head)
    base_sha = _resolve_commit(repo_dir, base)
    try:
        merge_base = _git(repo_dir, "merge-base", head_sha, base_sha).strip()
    except subprocess.CalledProcessError:
        return {
            "status": "failed",
            "base": base_sha,
            "head": head_sha,
            "merge_base": None,
            "error": (
                f"no common ancestor between base {base_sha[:12]} and head "
                f"{head_sha[:12]}; fetch deeper history before validating"
            ),
        }
    ancestor = merge_base == base_sha
    ahead = int(_git(repo_dir, "rev-list", "--count", f"{merge_base}..{head_sha}"))
    behind = int(_git(repo_dir, "rev-list", "--count", f"{merge_base}..{base_sha}"))
    report: dict[str, Any] = {
        "status": "passed" if ancestor else "failed",
        "base": base_sha,
        "head": head_sha,
        "merge_base": merge_base,
        "ahead": ahead,
        "behind": behind,
    }
    if not ancestor:
        report["error"] = (
            f"base {base_sha[:12]} is not an ancestor of head {head_sha[:12]} "
            f"(merge base {merge_base[:12]}; {ahead} ahead, {behind} behind); "
            "fetch and rebase onto the current base before validating"
        )
        report["changed_files_since_merge_base"] = {
            side: sorted(
                line
                for line in _git(
                    repo_dir, "diff", "--name-only", "--no-renames", f"{merge_base}..{tip}"
                ).splitlines()
                if line
            )
            for side, tip in (("head", head_sha), ("base", base_sha))
        }
    return report


def _tree_sources(repo_dir: Path, revision: str | None) -> dict[str, str]:
    if revision is None:
        listing = _git(
            repo_dir,
            "ls-files",
            "-z",
            "--cached",
            "--others",
            "--exclude-standard",
            "--",
            *SOURCE_ROOTS,
        )
        return {
            name: (repo_dir / name).read_text(encoding="utf-8", errors="replace")
            for name in sorted(set(listing.split("\0")))
            if name.endswith(SOURCE_SUFFIXES) and (repo_dir / name).is_file()
        }
    listing = _git(repo_dir, "ls-tree", "-r", "-z", revision, "--", *SOURCE_ROOTS)
    files: dict[str, str] = {}
    for entry in listing.split("\0"):
        if not entry:
            continue
        metadata, name = entry.split("\t", 1)
        _mode, kind, oid = metadata.split()
        if kind == "blob" and name.endswith(SOURCE_SUFFIXES):
            files[name] = oid
    if not files:
        return {}
    oids = sorted(set(files.values()))
    output = subprocess.run(
        [*_git_prefix(repo_dir), "cat-file", "--batch"],
        cwd=repo_dir,
        input=("\n".join(oids) + "\n").encode(),
        capture_output=True,
        check=True,
    ).stdout
    stream = io.BytesIO(output)
    blobs: dict[str, str] = {}
    for oid in oids:
        actual, kind, size = stream.readline().split()
        if actual.decode() != oid or kind != b"blob":
            raise ValueError(f"unexpected Git blob response for {oid}")
        blobs[oid] = stream.read(int(size)).decode("utf-8", errors="replace")
        if stream.read(1) != b"\n":
            raise ValueError("truncated Git blob batch")
    return {name: blobs[oid] for name, oid in files.items()}


def _entity_form(entity: str, kind: str) -> str:
    """Whether the marked entity is a definition or a bare declaration."""

    if kind == "FUNCTION":
        if "{" in entity:
            return "definition"
        if ";" in entity:
            return "declaration"
        return ""
    if kind == "GLOBAL":
        return "declaration" if re.match(r"\s*extern\b", entity) else "definition"
    return ""


def _owned_entity(lines: list[str], start: int, kind: str) -> tuple[str, str]:
    """The entity a marker binds to: normalized text plus declaration/definition form.

    The entity extends past a single line so that multiline signatures still
    compare whole and a definition body ``{`` versus a terminating ``;`` is
    visible.
    """

    collected: list[str] = []
    for index in range(start, min(start + 12, len(lines))):
        stripped = lines[index].strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            if kind == "STRING":
                return stripped, ""
            continue
        if stripped.startswith("//"):
            if kind in ("TEMPLATE", "SYNTHETIC") and not collected:
                return stripped, ""
            continue
        collected.append(stripped)
        joined = " ".join(collected)
        if ";" in joined or "{" in joined:
            text = re.sub(r"\s+", " ", joined)
            return text, _entity_form(text, kind)
    return re.sub(r"\s+", " ", " ".join(collected)), ""


def _entity_name(entity: str) -> str:
    cleaned = re.sub(r"/\*.*?\*/", " ", entity)
    match = _DECLARATOR.search(cleaned)
    return match.group(1).split("::")[-1] if match else ""


def collect_identities(
    repo_dir: Path, revision: str | None, *, sources: dict[str, str] | None = None
) -> dict[Identity, list[dict[str, str]]]:
    """Markers at ``revision`` keyed by (kind, target, address)."""

    identities: dict[Identity, list[dict[str, str]]] = defaultdict(list)
    for name, content in (
        sources if sources is not None else _tree_sources(repo_dir, revision)
    ).items():
        lines = content.splitlines()
        for index, line in enumerate(lines):
            marker = _MARKER.match(line)
            if marker is None:
                continue
            kind = marker.group("kind")
            key = (kind, marker.group("target"), int(marker.group("address"), 16))
            entity, form = _owned_entity(lines, index + 1, kind)
            identities[key].append(
                {
                    "file": name,
                    "entity": entity,
                    "form": form,
                    "name": _entity_name(entity),
                }
            )
    return identities


def _references(sources: dict[str, str], names: set[str]) -> dict[str, int]:
    if not names:
        return {}
    counts = dict.fromkeys(names, 0)
    pattern = re.compile(r"\b(" + "|".join(re.escape(name) for name in sorted(names)) + r")\b")
    for content in sources.values():
        for match in pattern.finditer(content):
            counts[match.group(1)] += 1
    return counts


def _format_key(key: Identity) -> str:
    kind, target, address = key
    return f"{kind} {target} 0x{address:08X}"


def _global_definitions_from_sources(sources: dict[str, str]) -> list[dict[str, Any]]:
    """Run the repository's existing global-size model over an arbitrary Git tree."""

    from .global_model import parse_global_definitions

    with tempfile.TemporaryDirectory(prefix="wiz8-merge-preservation-") as directory:
        root = Path(directory)
        for name, content in sources.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        return parse_global_definitions(root)


def _global_container(
    definitions: list[dict[str, Any]], target: str, address: int
) -> dict[str, Any] | None:
    """Return the unique current GLOBAL whose known extent contains address."""

    candidates = [
        item
        for item in definitions
        if str(item.get("target") or "").upper() == target.upper()
        and item.get("size")
        and int(item["address"]) < address < int(item["address"]) + int(item["size"])
    ]
    return candidates[0] if len(candidates) == 1 else None


def merge_preservation_report(
    repo_dir: Path,
    base: str,
    head: str | None = None,
) -> dict[str, Any]:
    """Compare marker identities between ``base`` and ``head``."""

    requested_head = head
    if head is None and (repo_dir / ".jj").is_dir():
        head = subprocess.run(
            ["jj", "log", "-r", "@", "--no-graph", "-T", "commit_id"],
            cwd=repo_dir,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    base = _git(repo_dir, "rev-parse", "--verify", f"{base}^{{commit}}").strip()
    if head is not None:
        head = _git(repo_dir, "rev-parse", "--verify", f"{head}^{{commit}}").strip()
    base_sources = _tree_sources(repo_dir, base)
    head_sources = _tree_sources(repo_dir, head)
    before = collect_identities(repo_dir, base, sources=base_sources)
    after = collect_identities(repo_dir, head, sources=head_sources)

    removed = [key for key in sorted(before) if key not in after]
    added = [key for key in sorted(after) if key not in before]
    changed = [
        key
        for key in sorted(before.keys() & after.keys())
        if key[0] in IDENTITY_KINDS
        and sorted(item["entity"] for item in before[key])
        != sorted(item["entity"] for item in after[key])
    ]
    duplicates = [key for key in sorted(after) if key[0] in IDENTITY_KINDS and len(after[key]) > 1]
    conflicts = [
        {"target": target, "address": f"0x{address:08X}", "kinds": ["FUNCTION", "STUB"]}
        for kind, target, address in sorted(after)
        if kind == "FUNCTION" and ("STUB", target, address) in after
    ]
    demoted = [
        key
        for key in sorted(before.keys() & after.keys())
        if key[0] in ("FUNCTION", "GLOBAL")
        and any(item["form"] == "definition" for item in before[key])
        and all(item["form"] == "declaration" for item in after[key])
    ]

    after_kinds: dict[tuple[str, int], set[str]] = defaultdict(set)
    for kind, target, address in after:
        after_kinds[target, address].add(kind)

    reclassified: list[dict[str, Any]] = []
    unresolved_losses: list[Identity] = []
    for key in removed:
        kind, target, address = key
        if kind not in IDENTITY_KINDS:
            continue
        kinds = after_kinds.get((target, address), set())
        replacements = kinds & PRESERVING_RECLASSIFICATIONS.get(kind, frozenset())
        if len(kinds) == 1 and len(replacements) == 1:
            replacement = next(iter(replacements))
            replacement_key = (replacement, target, address)
            reclassified.append(
                {
                    "identity": _format_key(key),
                    "replacement": _format_key(replacement_key),
                    "owners": after[replacement_key],
                }
            )
        else:
            unresolved_losses.append(key)

    global_definitions = (
        _global_definitions_from_sources(head_sources)
        if any(key[0] == "GLOBAL" for key in unresolved_losses)
        else []
    )
    subsumed: list[dict[str, Any]] = []
    lost: list[Identity] = []
    for key in unresolved_losses:
        kind, target, address = key
        if kind == "GLOBAL":
            container = _global_container(global_definitions, target, address)
            if container is not None:
                container_address = int(container["address"])
                subsumed.append(
                    {
                        "identity": _format_key(key),
                        "container": f"GLOBAL {target} 0x{container_address:08X}",
                        "name": str(container.get("name") or ""),
                        "offset": f"0x{address - container_address:X}",
                        "size": int(container["size"]),
                        "source_file": str(container.get("source_file") or ""),
                        "line": int(container.get("line") or 0),
                    }
                )
                continue
        lost.append(key)

    lost_function_names = {
        item["name"] for key in lost if key[0] == "FUNCTION" for item in before[key] if item["name"]
    }
    still_referenced = _references(head_sources, lost_function_names)
    unresolved = sorted(name for name, count in still_referenced.items() if count)

    failed = bool(lost or duplicates or demoted or conflicts)

    def describe(
        keys: list[Identity], source: dict[Identity, list[dict[str, str]]]
    ) -> list[dict[str, Any]]:
        return [{"identity": _format_key(key), "owners": source[key]} for key in keys]

    counts = {
        kind: {
            "base": sum(1 for key in before if key[0] == kind),
            "head": sum(1 for key in after if key[0] == kind),
        }
        for kind in MARKER_KINDS
    }
    return {
        "schema": "wiz8.merge-preservation-v2",
        "base": base,
        "head": head or "working-tree",
        "source_state": {
            "mode": "revision" if requested_head is not None else "current",
            "head_commit": head,
            "base_tree": _git(repo_dir, "rev-parse", f"{base}^{{tree}}").strip(),
            "head_tree": _git(repo_dir, "rev-parse", f"{head}^{{tree}}").strip() if head else None,
            "source_digest": hashlib.sha256(
                repr(sorted(head_sources.items())).encode("utf-8")
            ).hexdigest(),
            "identical_sources": base_sources == head_sources,
            "warning": "explicit revision mode excludes uncommitted working-tree edits"
            if requested_head is not None
            else None,
        },
        "status": "failed" if failed else "passed",
        "counts": counts,
        "removed": describe(removed, before),
        "added": describe(added, after),
        "changed": [
            {
                "identity": _format_key(key),
                "base": [item["entity"] for item in before[key]],
                "head": [item["entity"] for item in after[key]],
            }
            for key in changed
        ],
        "reclassified": reclassified,
        "subsumed": subsumed,
        "lost": describe(lost, before),
        "duplicates": describe(duplicates, after),
        "conflicts": conflicts,
        "demoted": [
            {
                "identity": _format_key(key),
                "base": [item["entity"] for item in before[key]],
                "head": [item["entity"] for item in after[key]],
            }
            for key in demoted
        ],
        "newly_unresolved": unresolved,
    }
