"""Aggregate translation-unit and header debt may drain, never grow.

Several files and headers group declarations by how their contents were
discovered -- a registry mechanism, an address range, a boundary list -- rather
than by the original translation unit that owned them. Draining them is ordinary
recovery work; what this file prevents is the opposite, new bodies and new
includes accumulating in them because they are convenient.

There is deliberately no rule here that infers an original owner. That was
tried and removed: deriving a file's owning unit from the `srAssertFail` call
sites inside it is not sound, because one recovered file can hold a single
asserting function among dozens of silent ones, and a single function can carry
assertions from several units after inlining. It produced confident-looking
attributions that were artifacts of both. If an original owner is ever recorded,
it should come from something that actually identifies a unit, not from a vote.
"""

from __future__ import annotations

import re
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]

# Files grouped by discovery rather than by original owner. They may lose
# markers as their contents find real homes; they may never gain any.
# The light unit's file was 00497af0_004adb20.cpp, whose bounds spanned eight
# units that carry their own assertion-backed intervals. It is now bounded to
# the single unnamed unit it actually held, so its ceiling is the count at that
# rename rather than the ten the wider bucket had; from here it may only drain.
AGGREGATE_MARKER_CEILING = {
    "src/wiz8/engine_code/registry_classes.cpp": 21,
    "src/wiz8/unattributed/0049ba51_0049e5cf.cpp": 14,
}

# Headers that aggregate unrelated declarations. Existing includers are the
# shrinking baseline; no unit may newly include one.
AGGREGATE_HEADER_CEILING = {
    "registry_classes.h": 12,
    "quarantine_common.h": 32,
}


def test_aggregate_units_do_not_grow() -> None:
    """A file grouped by discovery may drain, never accumulate."""
    marker = re.compile(r"^//\s*(?:FUNCTION|TEMPLATE):\s*WIZ8", re.MULTILINE)

    offenders = []
    for relative, ceiling in AGGREGATE_MARKER_CEILING.items():
        path = REPOSITORY / relative
        if not path.is_file():
            continue
        count = len(marker.findall(path.read_text(encoding="utf-8", errors="replace")))
        if count > ceiling:
            offenders.append(
                f"{relative} holds {count} markers, above its {ceiling} ceiling; "
                "put the new body in the unit that owns it instead"
            )
    assert not offenders, "\n  ".join(["aggregate unit grew:", *offenders])


def test_aggregate_headers_gain_no_new_includers() -> None:
    """A mechanism is not a domain owner, so these headers only lose users."""
    sources = [
        path
        for root in ("src", "include")
        for path in (REPOSITORY / root).rglob("*")
        if path.suffix in {".cpp", ".c", ".h"}
    ]

    offenders = []
    for header, ceiling in AGGREGATE_HEADER_CEILING.items():
        pattern = re.compile(rf"#\s*include\s*[\"<][^\">]*{re.escape(header)}[\">]")
        count = sum(
            1
            for path in sources
            if pattern.search(path.read_text(encoding="utf-8", errors="replace"))
        )
        if count > ceiling:
            offenders.append(
                f"{header} has {count} includers, above its {ceiling} ceiling; "
                "declare what you need in a subsystem header instead"
            )
    assert not offenders, "\n  ".join(["aggregate header gained an includer:", *offenders])
