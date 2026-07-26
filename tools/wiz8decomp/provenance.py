"""Closed vocabulary for where a reviewed name came from and how far it may be trusted.

The model is documented in ``docs/wiz8-evidence-model.md``. Two properties matter here:

* ``name_origin`` is a ``|``-joined set drawn from :data:`NAME_ORIGIN_CEILING`;
* ``authority`` is *derived*: it is the strongest ceiling among the row's origins.

Materialising the derived value in the tracked CSVs keeps them readable, and validating equality
keeps them from drifting. Under-claiming is expressed by dropping an origin token, never by
weakening ``authority`` while keeping a strong origin.
"""

from __future__ import annotations

from types import MappingProxyType

AUTHORITY_RANK = MappingProxyType(
    {
        "descriptive": 0,
        "external-semantic": 1,
        "string-backed": 2,
        "abi-backed": 3,
        "source-backed": 4,
    }
)

NAME_ORIGIN_CEILING = MappingProxyType(
    {
        # An original source body proves the name.
        "original-source": "source-backed",
        # Specifically the pinned, licence-restricted SGP oracle tree.
        "sgp-source": "source-backed",
        # An original decorated symbol: export table, import table, or library symbol table.
        "original-export": "abi-backed",
        # The program names the entity at runtime, e.g. an assertion or class registration string.
        "original-runtime-string": "string-backed",
        # A __FILE__, assertion, or logging path assigns translation-unit ownership.
        "original-source-path": "string-backed",
        # Official builds are boundary oracles. They locate and date code; they do not name it.
        "official-demo": "descriptive",
        "official-cross-build": "descriptive",
        # Third parties assigning meaningful names Sir-Tech never necessarily used.
        "fan-patch-signature": "external-semantic",
        "cosmic-forge": "external-semantic",
        # We named it from observed behaviour.
        "descriptive": "descriptive",
    }
)

NAME_ORIGINS = frozenset(NAME_ORIGIN_CEILING)
AUTHORITIES = frozenset(AUTHORITY_RANK)

ORIGIN_SEPARATOR = "|"


class ProvenanceError(ValueError):
    """A reviewed row claims provenance outside the model."""


def parse_name_origin(value: str) -> tuple[str, ...]:
    """Split and validate a ``|``-joined origin set, returning it in canonical order."""

    raw = [token.strip() for token in value.split(ORIGIN_SEPARATOR)]
    if any(not token for token in raw):
        raise ProvenanceError(f"empty name_origin token in {value!r}")
    unknown = sorted(set(raw) - NAME_ORIGINS)
    if unknown:
        raise ProvenanceError(f"unknown name_origin: {', '.join(unknown)}")
    origins = sorted(set(raw))
    if len(origins) != len(raw):
        raise ProvenanceError(f"duplicate name_origin token in {value!r}")
    if len(origins) > 1 and "descriptive" in origins:
        raise ProvenanceError(
            "descriptive is the absence of an external source and cannot be combined: "
            f"{format_name_origin(origins)}"
        )
    return tuple(origins)


def format_name_origin(origins: tuple[str, ...] | list[str]) -> str:
    return ORIGIN_SEPARATOR.join(sorted(origins))


def derive_authority(origins: tuple[str, ...]) -> str:
    """Return the strongest authority the given origins can support."""

    if not origins:
        raise ProvenanceError("name_origin must not be empty")
    return max(
        (NAME_ORIGIN_CEILING[origin] for origin in origins),
        key=lambda authority: AUTHORITY_RANK[authority],
    )


def validate_provenance(name_origin: str, authority: str) -> tuple[tuple[str, ...], str]:
    """Validate one row's provenance pair and return the canonical form."""

    origins = parse_name_origin(name_origin)
    authority = authority.strip()
    if authority not in AUTHORITIES:
        raise ProvenanceError(f"unknown authority: {authority!r}")
    expected = derive_authority(origins)
    if authority != expected:
        raise ProvenanceError(
            f"authority {authority!r} is not derivable from {format_name_origin(origins)}; "
            f"expected {expected!r}"
        )
    return origins, authority


def is_original(authority: str) -> bool:
    """True when the name is evidence about Sir-Tech's own naming, not a third party's."""

    return AUTHORITY_RANK[authority] >= AUTHORITY_RANK["string-backed"]


# A FID seed's build provenance determines where its symbol names came from. A
# precompiled archive carries the original library's COFF symbol table; a
# source-built object carries names the pinned source itself declares. Both are
# original evidence, and neither is a guess we made.
FID_SOURCE_KIND_ORIGIN = MappingProxyType(
    {
        "precompiled-archive": "original-export",
        "cmake-object-library": "original-source",
    }
)


def origin_for_fid_source_kind(source_kind: str | None) -> str:
    """Map a FID seed's build provenance onto this model's name provenance.

    A match with no recorded seed provenance is not authoritative evidence about
    anyone's naming, so it degrades to ``descriptive`` rather than inheriting the
    authority of the database it happened to come from.
    """

    if source_kind is None:
        return "descriptive"
    origin = FID_SOURCE_KIND_ORIGIN.get(source_kind)
    if origin is None:
        raise ProvenanceError(f"unknown FID seed source_kind: {source_kind!r}")
    return origin
