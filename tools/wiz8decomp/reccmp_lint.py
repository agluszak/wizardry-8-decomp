"""Use reccmp's parser/linter with the repository's explicit-order policy."""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
from typing import Any

from reccmp.parser.error import AlertCode
from reccmp.tools.decomplint import decomplint_parse_args, lint_all_targets

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


def validate_reccmp_annotations(repository: Path) -> dict[str, Any]:
    arguments = argparse.Namespace(
        paths=[repository / "src/wiz8", repository / "include/wiz8"],
        target="WIZ8",
        encoding="utf-8",
    )
    alerts = [
        alert
        for alert in lint_all_targets(decomplint_parse_args(arguments))
        if alert.target in {None, "WIZ8"}
    ]
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
        "alerts": dict(sorted(counts.items())),
        "waived": sorted(code.name.lower() for code in ALLOWED_ALERTS),
    }
