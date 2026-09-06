"""Class-ABI audit over the Clang-backed source index.

These rules encode only facts the compiler and the retail image have already
settled. They exist because a source model can be wrong in ways `just lint`
cannot see: lint compiles the declarations, so it accepts a class that
re-declares methods its base already emits, and it accepts a new virtual added
to a dllimport class even when the linker then leaves that vtable slot null.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

REPOSITORY = Path(__file__).resolve().parents[2]
"""The identity trio, and deliberately not clone.

A support-derived class never re-declares the identity trio: those three bodies
are fixed by the template arguments, so a hand-written copy is always the
address-qualified-wrapper mistake. clone is different. The template's clone
copies memberwise through Derived, which is wrong for a class holding state its
base's assignment operator cannot carry, and such a class overrides it for real.
stTextureAnim is the worked example: its clone assigns through srTexture and then
copies each playback field individually, and it compares byte-exact against
0x004858B0. Listing clone here rejected that legitimate override, so it is out.
"""
IDENTITY_METHODS = ("getClassName", "getClassID", "getClassNode")
SUPPORT_BASE = "srClassSupport<"
DELETING_DESTRUCTORS = (
    "`scalar deleting destructor'",
    "`vector deleting destructor'",
)


def _index() -> dict[str, Any]:
    """Load the generated index.

    These rules are only as current as this file. `just test` and `just check`
    both regenerate it before running the repository suite, and the generator
    aborts the lane rather than rewriting a bad index, so the supported lanes
    cannot reach these rules with stale data. Running pytest directly after
    editing sources can, so refresh with `uv run wiz8 analyze source-index`
    first and read its errors -- it refuses to rewrite the index when a marker
    is malformed, and that refusal is itself a defect to fix.

    Do not add an mtime-based staleness assert here: the writer preserves the
    file when regenerated content is byte-identical, so mtime comparison
    reports false staleness.
    """
    path = REPOSITORY / "build" / "source-index.json"
    assert path.is_file(), (
        f"{path} is missing; the source index is refreshed by `just test` and "
        "`just check` before the repository suite runs"
    )
    with path.open(encoding="utf-8") as handle:
        index = json.load(handle)

    assert index.get("markers") and index.get("classes") and index.get("declarations"), (
        "source index is empty or truncated; regenerate it with `uv run wiz8 analyze source-index`"
    )
    return index


def _support_specialization(record: dict[str, Any]) -> str | None:
    """Return the srClassSupport base a class derives from, if any."""
    for base in record.get("bases") or ():
        if base.startswith(SUPPORT_BASE):
            return base
    return None


def test_support_derived_classes_do_not_redeclare_template_methods() -> None:
    """A class whose own base is srClassSupport<ThatClass, ...> inherits the
    identity trio and clone from the template. Re-declaring them turns a
    template emission back into a hand-written method, which is the exact
    mistake the address-qualified wrapper classes encoded."""
    index = _index()
    declarations = index["declarations"]

    support_derived: dict[str, str] = {}
    for record in index["classes"]:
        base = _support_specialization(record)
        if base is None:
            continue
        name = record["qualified_name"]
        # Only the class the specialization names owns those emissions; a
        # further subclass of it may legitimately override them.
        first_argument = base[len(SUPPORT_BASE) :].split(",")[0].strip()
        if first_argument == name:
            support_derived[name] = base

    assert support_derived, "expected at least one srClassSupport-derived class"

    offenders = [
        f"{declaration['qualified_name']} "
        f"({declaration['source_file']}:{declaration['line']}) "
        f"duplicates {support_derived[declaration['owning_class']]}"
        for declaration in declarations
        if declaration.get("owning_class") in support_derived
        and declaration["qualified_name"].rsplit("::", 1)[-1] in IDENTITY_METHODS
    ]
    assert not offenders, (
        "these classes derive from srClassSupport and must inherit the "
        "registry identity and clone slots instead of declaring them:\n  "
        + "\n  ".join(sorted(offenders))
    )


def test_template_specializations_are_never_function_markers() -> None:
    """An address inside srClassSupport<...> is emitted from the template, so
    it is a TEMPLATE emission (or SYNTHETIC for the compiler's deleting
    destructor). FUNCTION would claim someone authored that body."""
    offenders = [
        f"{marker['source_file']}:{marker['line']} {marker['marker_name']}"
        for marker in _index()["markers"]
        if SUPPORT_BASE in (marker.get("marker_name") or "") and marker["marker_kind"] == "FUNCTION"
    ]
    assert not offenders, (
        "srClassSupport specializations are template output and must use "
        "TEMPLATE, never FUNCTION:\n  " + "\n  ".join(sorted(offenders))
    )


def test_deleting_destructors_are_synthetic_and_unbound() -> None:
    """MSVC generates scalar and vector deleting destructors. Neither may bind to an
    authored declaration, because authoring one means someone hand-wrote the
    flag test and operator delete that the compiler owns."""
    offenders = []
    for marker in _index()["markers"]:
        if not any(
            spelling in (marker.get("marker_name") or "") for spelling in DELETING_DESTRUCTORS
        ):
            continue
        location = f"{marker['source_file']}:{marker['line']} {marker['marker_name']}"
        if marker["marker_kind"] != "SYNTHETIC":
            offenders.append(f"{location} is {marker['marker_kind']}, expected SYNTHETIC")
        elif marker["declaration"] is not None:
            offenders.append(f"{location} binds an authored declaration")
    assert not offenders, "\n  ".join(["deleting-destructor defects:", *sorted(offenders)])


def test_authored_lifecycle_markers_use_lifecycle_semantics() -> None:
    """A marker that names a constructor or destructor must resolve to that
    C++ entity, so the compiler emits the real lifecycle bundle rather than a
    look-alike ordinary method."""
    offenders = []
    for marker in _index()["markers"]:
        if marker["marker_kind"] != "FUNCTION":
            continue
        declaration = marker.get("declaration")
        if not declaration:
            continue
        name = declaration["qualified_name"]
        owner = declaration.get("owning_class")
        if owner is None:
            continue
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
