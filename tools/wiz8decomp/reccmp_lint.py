"""Use reccmp's parser/linter with the repository's explicit-order policy."""

from __future__ import annotations

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


class ReccmpLintError(RuntimeError):
    """reccmp found a non-waived annotation problem."""


def _configured_lint_targets(repository: Path) -> tuple[DecomplintTarget, ...]:
    """Translate every reccmp source target into decomplint's native scope."""

    project = RecCmpProject.from_directory(repository)
    project_file = project.project_config_path or repository / "reccmp-project.yml"
    return tuple(
        DecomplintTarget(
            paths=tuple(source_code_search(target.source_paths)),
            module=target.target_id,
            encoding=target.encoding or "utf-8",
            project_file_path=project_file,
            aliases=target.marker_aliases,
        )
        for target in project.targets.values()
        if target.source_paths
    )


def validate_reccmp_annotations(repository: Path) -> dict[str, Any]:
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
        "alerts": dict(sorted(counts.items())),
        "waived": sorted(code.name.lower() for code in ALLOWED_ALERTS),
    }
