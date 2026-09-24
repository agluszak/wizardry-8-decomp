"""Use reccmp's marker linter with the repository's explicit-order policy.

decomplint reads the marker blocks the compiler-backed source index
collected (build/source-index.json, refreshed by `wiz8 check`)."""

from __future__ import annotations

import re
from collections import Counter
from pathlib import Path
from typing import Any

from reccmp.dir import source_code_search
from reccmp.parser.error import AlertCode
from reccmp.project.detect import RecCmpProject
from reccmp.tools.decomplint import DecomplintTarget, check_aliases, lint_all_targets

# Many recovered translation units preserve reviewed source/link ordering that is
# not monotonically increasing by address. reccmp's generic order advice cannot
# be applied without changing emitted-code evidence. Marker names and line/name
# marker styles are also intentionally non-authoritative: the shared compiler
# index binds semantic identities. Duplicate offsets and stray markers remain
# fatal.
ALLOWED_ALERTS = frozenset(
    {
        AlertCode.BYNAME_FUNCTION_IN_CPP,
        AlertCode.FUNCTION_OUT_OF_ORDER,
        AlertCode.NOT_STRICT_FORMAT,
    }
)

_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx", ".inl"})
_FOLDED_MARKER = re.compile(
    r"^\s*//\s*(?:FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY|VTABLE|GLOBAL|STUB):"
    r"\s+\S+\s+0x[0-9a-f]+\s+FOLDED(?:\s|$)",
    re.IGNORECASE,
)
_IDENTITY_ALIAS_ANNOTATION = re.compile(r"\bidentity-alias\s*:", re.IGNORECASE)


def _folded_source_markers(repository: Path) -> list[str]:
    """Return source locations that turn retail ICF into source identity."""

    problems: list[str] = []
    for root_name in ("src", "include"):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                if _FOLDED_MARKER.match(line):
                    problems.append(f"{path.relative_to(repository)}:{line_number}")
    return problems


def _identity_alias_annotations(repository: Path) -> list[str]:
    """Reject the retired source-identity alias escape hatch."""

    problems: list[str] = []
    for root_name in ("src", "include"):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                if _IDENTITY_ALIAS_ANNOTATION.search(line):
                    problems.append(f"{path.relative_to(repository)}:{line_number}")
    return problems


class ReccmpLintError(RuntimeError):
    """reccmp found a non-waived annotation problem."""


def _configured_lint_targets(repository: Path) -> tuple[DecomplintTarget, ...]:
    """Translate every reccmp source target the source index covers into
    decomplint's native scope. Markers are read from the compiler, so a
    target this configuration does not compile (the JPEG importer without the
    IJG tree) cannot be linted here; `_unlinted_targets` names it."""

    project = RecCmpProject.from_directory(repository)
    project_file = project.project_config_path or repository / "reccmp-project.yml"
    covered = _indexed_target_ids(repository)
    return tuple(
        DecomplintTarget(
            paths=tuple(source_code_search(target.source_paths)),
            module=target.target_id,
            encoding=target.encoding or "utf-8",
            source_index=repository / "build" / "source-index.json",
            project_file_path=project_file,
            aliases=target.marker_aliases,
        )
        for target in project.targets.values()
        if target.source_paths and target.target_id in covered
    )


def _indexed_target_ids(repository: Path) -> set[str]:
    from .build import LINT_BUILD_DIR
    from .source_index import indexed_targets

    database = repository / LINT_BUILD_DIR / "compile_commands.json"
    return set(indexed_targets(repository, database))


def _unlinted_targets(repository: Path, linted: tuple[DecomplintTarget, ...]) -> list[str]:
    project = RecCmpProject.from_directory(repository)
    names = {target.module for target in linted}
    return sorted(
        target.target_id
        for target in project.targets.values()
        if target.source_paths and target.target_id not in names
    )


def validate_reccmp_annotations(repository: Path) -> dict[str, Any]:
    folded = _folded_source_markers(repository)
    if folded:
        raise ReccmpLintError(
            "FOLDED source markers are forbidden: retail ICF is comparison evidence, "
            "not source identity:\n  " + "\n  ".join(folded)
        )
    aliases = _identity_alias_annotations(repository)
    if aliases:
        raise ReccmpLintError(
            "identity-alias source annotations are forbidden; use ordinary source identities "
            "or a narrow ABI waiver:\n  " + "\n  ".join(aliases)
        )

    lint_targets = _configured_lint_targets(repository)
    alerts = lint_all_targets(lint_targets)
    alerts.extend(check_aliases(lint_targets))
    problems = [alert for alert in alerts if alert.code not in ALLOWED_ALERTS]
    if problems:
        rendered = [
            f"{alert.path}:{alert.line_number}: {alert.code.name.lower()}"
            + (f": {alert.detail}" if alert.detail else "")
            for alert in problems
        ]
        raise ReccmpLintError("reccmp decomplint failed:\n  " + "\n  ".join(sorted(rendered)))
    counts = Counter(alert.code.name.lower() for alert in alerts)
    return {
        "ok": True,
        "engine": "reccmp-decomplint",
        "targets": [target.module for target in lint_targets],
        "not_compiled_here": _unlinted_targets(repository, lint_targets),
        "alerts": dict(sorted(counts.items())),
        "waived": sorted(code.name.lower() for code in ALLOWED_ALERTS),
    }
