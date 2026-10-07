"""Class-ABI rules over the Clang-backed source index that `wiz8 lint` cannot see."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from wiz8decomp.source_index import (
    declaration_for_marker,
    declarations_by_semantic_key,
    load_source_index,
)

REPOSITORY = Path(__file__).resolve().parents[2]
# clone is deliberately absent: a class holding state its base's assignment
# cannot carry overrides it for real (stTextureAnim, 0x004858B0).
IDENTITY_METHODS = ("getClassName", "getClassID", "getClassNode")
SUPPORT_BASE = "srClassSupport<"


def _index() -> dict[str, Any]:
    index = load_source_index(REPOSITORY)
    assert index.get("markers") and index.get("classes") and index.get("declarations"), (
        "source index is empty or truncated; regenerate it with `uv run wiz8 check`"
    )
    return index


def test_support_derived_classes_do_not_redeclare_template_methods() -> None:
    """A class whose own base is srClassSupport<ThatClass, ...> inherits the
    identity trio from the template instead of re-declaring it."""
    index = _index()
    support_derived: dict[str, str] = {}
    for record in index["classes"]:
        for base in record.get("bases") or ():
            # Only the class the specialization names owns those emissions.
            if (
                base.startswith(SUPPORT_BASE)
                and base[len(SUPPORT_BASE) :].split(",")[0].strip() == record["qualified_name"]
            ):
                support_derived[record["qualified_name"]] = base

    assert support_derived, "expected at least one srClassSupport-derived class"
    offenders = [
        f"{declaration['qualified_name']} "
        f"({declaration['source_file']}:{declaration['line']}) "
        f"duplicates {support_derived[declaration['owning_class']]}"
        for declaration in index["declarations"]
        if declaration.get("owning_class") in support_derived
        and declaration["qualified_name"].rsplit("::", 1)[-1] in IDENTITY_METHODS
    ]
    assert not offenders, "\n  ".join(["srClassSupport identity re-declared:", *sorted(offenders)])


def test_authored_lifecycle_markers_use_lifecycle_semantics() -> None:
    """A marker that names a constructor or destructor must resolve to that
    C++ entity, so the compiler emits the real lifecycle bundle rather than a
    look-alike ordinary method."""
    index = _index()
    by_key = declarations_by_semantic_key(index)
    offenders = []
    for marker in index["markers"]:
        if marker["marker_kind"] != "FUNCTION":
            continue
        declaration = declaration_for_marker(marker, by_key)
        owner = declaration.get("owning_class")
        if owner is None:
            continue
        name = declaration["qualified_name"]
        tail = name.rsplit("::", 1)[-1]
        if tail.startswith("~"):
            expected = "destructor"
        elif tail == owner.rsplit("::", 1)[-1]:
            expected = "constructor"
        else:
            continue
        if declaration["semantic_kind"] != expected:
            offenders.append(
                f"{marker['source_file']}:{marker['line']} {name} is "
                f"{declaration['semantic_kind']}, expected {expected}"
            )
    assert not offenders, "\n  ".join(["lifecycle marker defects:", *sorted(offenders)])
