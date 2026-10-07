"""Gate recovered Wizardry bodies against accepted source-oracle ownership."""

from __future__ import annotations

import csv
import json
import re
import struct
from collections import defaultdict
from collections.abc import Iterable, Mapping, Sequence
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any

from .config import load_settings
from .evidence.claims import load_claims
from .evidence.io import parse_hex
from .identity_lint import (
    _UNNAMED_FUNCTION,
    _declaration_address,
    _declaration_lines,
    _last_component,
)
from .paths import atomic_json
from .provenance import ORIGIN_SEPARATOR, ProvenanceError, parse_name_origin

RETAINED_IDENTITY_PREDICATES = frozenset({"accepted-identity", "accepted-alias"})
_MARKER_KINDS = frozenset({"FUNCTION", "LIBRARY"})
_ZLIB_LIBRARY_TOKENS = frozenset(
    {"inflate", "deflate", "adler", "zcfree", "zcalloc", "huft", "_tr_", "zlib"}
)

# FID false friend: byte-identical to IJG jzero_far but Sir-Tech-owned (docs/targets/srext-jpegimporter.md).
REJECTED_FID_ADDRESSES = frozenset({0x004146E0})

# Reviewed body sizes: coverage is [start, start+size).
ORACLE_BODY_SIZES: dict[int, tuple[str, int]] = {
    0x005E1C10: ("msvc-runtime", 12),
    0x005E1C30: ("msvc-runtime", 104),
    0x005E1CA0: ("msvc-runtime", 52),
    0x005E1CF0: ("msvc-runtime", 170),
    0x005E1DA0: ("msvc-runtime", 47),
    0x005E1DD0: ("msvc-runtime", 31),
    0x005E1DEF: ("msvc-runtime", 106),
    0x005E1E71: ("msvc-runtime", 104),
    0x005E1EF1: ("msvc-runtime", 81),
    0x00415F60: ("zlib", 265),
    0x004165C0: ("zlib", 752),
    0x00417960: ("zlib", 14),
}


@dataclass(frozen=True)
class OracleFamily:
    """One accepted available-source boundary the gate can enforce."""

    name: str
    target: str
    source_roots: tuple[str, ...] = ()
    name_origins: frozenset[str] = field(default_factory=frozenset)
    retained_predicates: frozenset[str] = RETAINED_IDENTITY_PREDICATES
    # Extra claim matching for CRT FID rows (origin token need not be a name_origin).
    claim_origins: frozenset[str] = field(default_factory=frozenset)
    claim_predicates: frozenset[str] = field(default_factory=frozenset)
    # Inclusive documented contribution ranges (start address .. last covered byte).
    address_ranges: tuple[tuple[int, int], ...] = ()
    # Markers under source_roots are oracle evidence, not merely ownership.
    # False for editable reconstructions whose oracle is an immutable baseline.
    owner_markers_are_evidence: bool = True
    # LIBRARY markers may own this family without living under source_roots.
    library_marker_ownership: bool = False
    # When set, only LIBRARY marker names containing these tokens join the family.
    library_name_tokens: frozenset[str] = field(default_factory=frozenset)
    # When library_marker_ownership is set and this is True, unmatched LIBRARY
    # markers on the target (not claimed by an earlier family) join this family.
    library_marker_remainder: bool = False
    reccmp_csv: str | None = None

    def owns_source(self, source_file: str) -> bool:
        if not self.source_roots:
            return False
        path = source_file.replace("\\", "/")
        return any(path == root.rstrip("/") or path.startswith(root) for root in self.source_roots)

    def contains_address(self, address: int) -> bool:
        return any(start <= address <= end for start, end in self.address_ranges)

    def matches_library_name(self, marker_name: str) -> bool:
        if not self.library_name_tokens:
            return False
        lowered = marker_name.casefold()
        return any(token in lowered for token in self.library_name_tokens)

    def accepts_owner_marker(self, source_file: str, kind: str) -> bool:
        if self.owns_source(source_file):
            return True
        return self.library_marker_ownership and kind == "LIBRARY"


# Family order matters for LIBRARY remainder assignment: specialised families
# first, then CRT remainder.
ORACLE_FAMILIES: tuple[OracleFamily, ...] = (
    OracleFamily(
        name="sgp",
        target="WIZ8",
        source_roots=("src/sgp/",),
        name_origins=frozenset({"sgp-source"}),
        owner_markers_are_evidence=False,
    ),
    OracleFamily(
        name="zlib",
        target="WIZ8",
        reccmp_csv="config/reccmp/wiz8-zlib.csv",
        address_ranges=((0x00415910, 0x0041A7ED),),
        library_marker_ownership=True,
        library_name_tokens=_ZLIB_LIBRARY_TOKENS,
    ),
    OracleFamily(
        name="msvc-runtime",
        target="WIZ8",
        reccmp_csv="config/reccmp/wiz8-msvc-runtime.csv",
        claim_origins=frozenset({"fid"}),
        claim_predicates=frozenset({"fid-variants"}),
        retained_predicates=frozenset({"fid-variants"}),
        address_ranges=(
            (0x00401000, 0x004011DF),
            (0x005E1C10, 0x005E1F41),
        ),
        library_marker_ownership=True,
        library_marker_remainder=True,
    ),
    OracleFamily(
        name="ijg-jpeg",
        target="SREXT_JPEGIMPORTER",
        reccmp_csv="config/reccmp/srext-jpegimporter.csv",
    ),
    OracleFamily(
        name="infozip-unzip",
        target="SREXT_UNZIP",
        reccmp_csv="config/reccmp/srext-unzip.csv",
    ),
)


class SourceOracleGateError(RuntimeError):
    """A recovered identity sits in oracle space or lacks oracle ownership."""


def _normalize_address(value: int | str) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def _format_address(address: int) -> str:
    return f"0x{address:08x}"


def _load_index(repo_dir: Path) -> dict[str, Any]:
    path = repo_dir / "build/source-index.json"
    return json.loads(path.read_text(encoding="utf-8"))


def _origin_tokens(origin: str) -> set[str]:
    """Split a claim origin set without requiring every token to be a name_origin."""

    try:
        return set(parse_name_origin(origin))
    except ProvenanceError:
        return {token.strip() for token in origin.split(ORIGIN_SEPARATOR) if token.strip()}


def _family_for_origin(
    origin: str,
    predicate: str,
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> OracleFamily | None:
    tokens = _origin_tokens(origin)
    pred = predicate.strip()
    for family in families:
        if (
            family.claim_predicates
            and pred in family.claim_predicates
            and (not family.claim_origins or tokens & family.claim_origins)
        ):
            return family
        if tokens & family.name_origins and pred in family.retained_predicates:
            return family
    return None


def _family_for_source(
    source_file: str, families: Sequence[OracleFamily] = ORACLE_FAMILIES
) -> OracleFamily | None:
    for family in families:
        if family.owns_source(source_file):
            return family
    return None


def _family_for_library_marker(
    *,
    address: int,
    marker_name: str,
    families: Sequence[OracleFamily],
) -> OracleFamily | None:
    for family in families:
        if family.contains_address(address) and family.library_marker_ownership:
            return family
        if family.matches_library_name(marker_name):
            return family
    for family in families:
        if family.library_marker_remainder:
            return family
    return None


@dataclass(frozen=True)
class OracleSymbol:
    """An address owned by an oracle family; ``proven`` when backed by oracle evidence."""

    address: int
    family: str
    target: str
    source_file: str
    evidence: str
    name: str = ""
    marker_kind: str = ""
    proven: bool = True


@dataclass(frozen=True)
class ContributionHull:
    """Inclusive start-address span proven for one oracle translation unit."""

    family: str
    target: str
    source_file: str
    start: int
    end: int

    def contains(self, address: int) -> bool:
        return self.start <= address <= self.end


@dataclass(frozen=True)
class AddressRangeOwner:
    family: str
    target: str
    start: int
    end: int

    def contains(self, address: int) -> bool:
        return self.start <= address <= self.end


@dataclass(frozen=True)
class SizedBodyOwner:
    """Inclusive-exclusive reviewed body span: [start, start+size)."""

    family: str
    target: str
    start: int
    size: int

    @property
    def end(self) -> int:
        return self.start + self.size - 1

    def contains(self, address: int) -> bool:
        return self.start <= address < self.start + self.size


def _range_owners(families: Sequence[OracleFamily]) -> list[AddressRangeOwner]:
    owners: list[AddressRangeOwner] = []
    for family in families:
        for start, end in family.address_ranges:
            owners.append(
                AddressRangeOwner(family=family.name, target=family.target, start=start, end=end)
            )
    return owners


def _sized_body_owners(
    families: Sequence[OracleFamily],
    *,
    body_sizes: Mapping[int, tuple[str, int]] = ORACLE_BODY_SIZES,
) -> list[SizedBodyOwner]:
    family_targets = {family.name: family.target for family in families}
    owners: list[SizedBodyOwner] = []
    for start, (family_name, size) in sorted(body_sizes.items()):
        target = family_targets.get(family_name)
        if target is None:
            continue
        owners.append(SizedBodyOwner(family=family_name, target=target, start=start, size=size))
    return owners


def _load_reccmp_library_symbols(
    repo_dir: Path, families: Sequence[OracleFamily]
) -> list[OracleSymbol]:
    symbols: list[OracleSymbol] = []
    for family in families:
        if not family.reccmp_csv:
            continue
        path = repo_dir / family.reccmp_csv
        if not path.is_file():
            continue
        with path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream, delimiter="|")
            for row in reader:
                if (row.get("type") or "").strip() != "library":
                    continue
                address = int((row.get("address") or "0").strip(), 16)
                if address in REJECTED_FID_ADDRESSES:
                    continue
                name = (row.get("name") or row.get("symbol") or "").strip()
                symbols.append(
                    OracleSymbol(
                        address=address,
                        family=family.name,
                        target=family.target,
                        source_file=family.reccmp_csv,
                        evidence="reccmp-csv:library",
                        name=name,
                        marker_kind="LIBRARY",
                    )
                )
    return symbols


def oracle_symbols(
    repo_dir: Path,
    *,
    index: Mapping[str, Any] | None = None,
    claims: Sequence[Mapping[str, str]] | None = None,
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> list[OracleSymbol]:
    """Collect oracle-owned retail addresses from markers and claims.

    Owner markers of a family whose markers are not evidence stay unproven
    until a retained claim for the same address supplies the provenance.
    """

    source_index = index if index is not None else _load_index(repo_dir)
    claim_rows = claims if claims is not None else load_claims(repo_dir)
    by_address: dict[tuple[str, int], OracleSymbol] = {}

    for marker in source_index.get("markers", ()):
        kind = str(marker.get("marker_kind") or "")
        if kind not in _MARKER_KINDS:
            continue
        source_file = str(marker.get("source_file") or "").replace("\\", "/")
        target = str(marker.get("target") or "")
        address = _normalize_address(marker["address"])
        marker_name = str(marker.get("marker_name") or "")

        family = _family_for_source(source_file, families)
        if family is None and kind == "LIBRARY":
            family = _family_for_library_marker(
                address=address, marker_name=marker_name, families=families
            )
        if family is None:
            continue
        if target and target != family.target:
            continue
        target = target or family.target
        key = (target, address)
        proven = kind == "LIBRARY" or family.owner_markers_are_evidence
        by_address[key] = OracleSymbol(
            address=address,
            family=family.name,
            target=target,
            source_file=source_file,
            evidence=f"marker:{kind}" if proven else f"owner-marker:{kind}",
            name=marker_name,
            marker_kind=kind,
            proven=proven,
        )

    # Retained library labels are the owner; claim values may describe matching
    # variants rather than spell the function's name.
    for symbol in _load_reccmp_library_symbols(repo_dir, families):
        key = (symbol.target, symbol.address)
        if key not in by_address:
            by_address[key] = symbol

    claims_path = repo_dir / "evidence/reviewed/wiz8/claims.csv"
    for claim in claim_rows:
        family = _family_for_origin(claim["origin"], claim["predicate"], families)
        if family is None:
            continue
        if claim["entity_kind"].strip() != "function":
            continue
        address = parse_hex(claim["entity_key"], field="entity_key", path=claims_path) or 0
        key = (family.target, address)
        existing = by_address.get(key)
        if existing is not None:
            if not existing.proven and existing.family == family.name:
                by_address[key] = replace(
                    existing, evidence=f"claim:{claim['predicate'].strip()}", proven=True
                )
            continue
        by_address[key] = OracleSymbol(
            address=address,
            family=family.name,
            target=family.target,
            source_file="",
            evidence=f"claim:{claim['predicate'].strip()}",
            name=claim.get("value", "").strip(),
        )

    return sorted(by_address.values(), key=lambda item: (item.target, item.address, item.family))


def contribution_hulls(
    symbols: Sequence[OracleSymbol],
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> list[ContributionHull]:
    """Convex start-address spans per owner TU with a real source root."""

    hullable = {family.name for family in families if family.source_roots}
    grouped: dict[tuple[str, str, str], list[int]] = defaultdict(list)
    for symbol in symbols:
        if symbol.family not in hullable or not symbol.source_file:
            continue
        grouped[(symbol.family, symbol.target, symbol.source_file)].append(symbol.address)

    hulls: list[ContributionHull] = []
    for (family, target, source_file), addresses in sorted(grouped.items()):
        hulls.append(
            ContributionHull(
                family=family,
                target=target,
                source_file=source_file,
                start=min(addresses),
                end=max(addresses),
            )
        )
    return hulls


def _owner_at(
    address: int,
    *,
    target: str,
    symbols_by_key: Mapping[tuple[str, int], OracleSymbol],
    hulls: Sequence[ContributionHull],
    ranges: Sequence[AddressRangeOwner],
    bodies: Sequence[SizedBodyOwner] = (),
) -> OracleSymbol | ContributionHull | AddressRangeOwner | SizedBodyOwner | None:
    if address in REJECTED_FID_ADDRESSES:
        return None
    direct = symbols_by_key.get((target, address))
    if direct is not None:
        return direct
    for body in bodies:
        if body.target == target and body.contains(address):
            return body
    for hull in hulls:
        if hull.target == target and hull.contains(address):
            return hull
    for owner in ranges:
        if owner.target == target and owner.contains(address):
            return owner
    return None


def _oracle_owned_markers(
    index: Mapping[str, Any], families: Sequence[OracleFamily]
) -> dict[tuple[str, int], tuple[str, str]]:
    """Map (target, address) -> (source_file, marker_kind) for oracle-owned markers."""

    owned: dict[tuple[str, int], tuple[str, str]] = {}
    for marker in index.get("markers", ()):
        kind = str(marker.get("marker_kind") or "")
        if kind not in _MARKER_KINDS:
            continue
        source_file = str(marker.get("source_file") or "").replace("\\", "/")
        address = _normalize_address(marker["address"])
        marker_name = str(marker.get("marker_name") or "")
        family = _family_for_source(source_file, families)
        if family is None and kind == "LIBRARY":
            family = _family_for_library_marker(
                address=address, marker_name=marker_name, families=families
            )
        if family is None:
            continue
        target = str(marker.get("target") or family.target)
        owned[(target, address)] = (source_file, kind)
    return owned


def _owner_detail(
    owner: OracleSymbol | ContributionHull | AddressRangeOwner | SizedBodyOwner,
) -> tuple[str, str]:
    if isinstance(owner, OracleSymbol):
        return owner.family, owner.source_file or owner.evidence
    if isinstance(owner, ContributionHull):
        return (
            owner.family,
            f"{owner.source_file} [{_format_address(owner.start)}-{_format_address(owner.end)}]",
        )
    if isinstance(owner, SizedBodyOwner):
        return (
            owner.family,
            f"body [{_format_address(owner.start)}-{_format_address(owner.end)}]",
        )
    return (
        owner.family,
        f"range [{_format_address(owner.start)}-{_format_address(owner.end)}]",
    )


def _retail_pe_path(repo_dir: Path) -> Path | None:
    settings = load_settings(require=False)
    if settings is not None and settings.repo_dir.resolve() == repo_dir.resolve():
        path = settings.work_dir / "variants/gog-base/Wiz8.exe"
        if path.is_file():
            return path
    candidates = (
        repo_dir / ".wiz8-work/extracted/gog-base/Wiz8.exe",
        repo_dir / ".wiz8-work/variants/gog-base/Wiz8.exe",
    )
    for path in candidates:
        if path.is_file():
            return path
    return None


def _pe_file_offset(data: bytes, address: int) -> int | None:
    if data[:2] != b"MZ":
        return None
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e_lfanew : e_lfanew + 4] != b"PE\0\0":
        return None
    coff = e_lfanew + 4
    num_sections = struct.unpack_from("<H", data, coff + 2)[0]
    opt_size = struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    image_base = struct.unpack_from("<I", data, opt + 28)[0]
    section_table = opt + opt_size
    rva = address - image_base
    for index in range(num_sections):
        offset = section_table + index * 40
        vsize, va, rawsize, rawptr = struct.unpack_from("<IIII", data, offset + 8)
        if va <= rva < va + max(vsize, rawsize):
            return rawptr + (rva - va)
    return None


def _iat_thunk_violations(repo_dir: Path, index: Mapping[str, Any]) -> list[dict[str, Any]]:
    """Fail FUNCTION markers whose retail bytes are a six-byte ``jmp [imm32]`` IAT thunk."""

    pe_path = _retail_pe_path(repo_dir)
    if pe_path is None:
        return []
    data = pe_path.read_bytes()
    violations: list[dict[str, Any]] = []
    for marker in index.get("markers", ()):
        kind = str(marker.get("marker_kind") or "")
        if kind not in {"FUNCTION", "LIBRARY"}:
            continue
        if str(marker.get("target") or "") != "WIZ8":
            continue
        address = _normalize_address(marker["address"])
        file_offset = _pe_file_offset(data, address)
        if file_offset is None or file_offset + 6 > len(data):
            continue
        chunk = data[file_offset : file_offset + 6]
        if chunk[:2] != b"\xff\x25":
            continue
        source_file = str(marker.get("source_file") or "").replace("\\", "/")
        violations.append(
            {
                "kind": "iat-thunk-function" if kind == "FUNCTION" else "iat-thunk-library",
                "family": "linker",
                "address": _format_address(address),
                "source": source_file,
                "line": int(marker.get("line") or 0),
                "detail": (
                    f"{source_file}:{marker.get('line')}: {kind} {_format_address(address)} "
                    "is a six-byte import thunk (jmp [iat]); record in emission metadata or leave unmarked"
                ),
            }
        )
    return violations


def source_oracle_violations(
    repo_dir: Path,
    *,
    index: Mapping[str, Any] | None = None,
    claims: Sequence[Mapping[str, str]] | None = None,
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> list[dict[str, Any]]:
    """Return ownership violations for the configured oracle families."""

    source_index = index if index is not None else _load_index(repo_dir)
    claim_rows = claims if claims is not None else load_claims(repo_dir)
    symbols = oracle_symbols(repo_dir, index=source_index, claims=claim_rows, families=families)
    hulls = contribution_hulls(symbols, families)
    ranges = _range_owners(families)
    bodies = _sized_body_owners(families)
    symbols_by_key = {(item.target, item.address): item for item in symbols}
    oracle_markers = _oracle_owned_markers(source_index, families)
    library_owners = {
        (item.target, item.address): item
        for item in _load_reccmp_library_symbols(repo_dir, families)
    }
    violations: list[dict[str, Any]] = []
    violations.extend(_iat_thunk_violations(repo_dir, source_index))

    for marker in source_index.get("markers", ()):
        if str(marker.get("marker_kind") or "") != "FUNCTION":
            continue
        source_file = str(marker.get("source_file") or "").replace("\\", "/")
        if _family_for_source(source_file, families) is not None:
            continue
        target = str(marker.get("target") or "")
        address = _normalize_address(marker["address"])
        owner = _owner_at(
            address,
            target=target,
            symbols_by_key=symbols_by_key,
            hulls=hulls,
            ranges=ranges,
            bodies=bodies,
        )
        if owner is None:
            continue
        family_name, owner_detail = _owner_detail(owner)
        violations.append(
            {
                "kind": "misplaced-oracle-function",
                "family": family_name,
                "address": _format_address(address),
                "source": source_file,
                "line": int(marker.get("line") or 0),
                "owner": owner_detail,
                "detail": (
                    f"{source_file}:{marker.get('line')}: FUNCTION {_format_address(address)} "
                    f"is owned by the {family_name} boundary ({owner_detail}); "
                    "recover it at that owner / mark LIBRARY, do not decompile under Wizardry TUs"
                ),
            }
        )

    seen_placeholders: set[tuple[str, int, str, int]] = set()
    source_lines: dict[str, list[str] | None] = {}
    for declaration in source_index.get("declarations", ()):
        qualified = str(declaration.get("qualified_name") or "")
        name = _last_component(qualified)
        if not _UNNAMED_FUNCTION.fullmatch(name):
            continue
        source_file = str(declaration.get("source_file") or "").replace("\\", "/")
        if _family_for_source(source_file, families) is not None:
            continue
        lines = _declaration_lines(repo_dir, declaration, source_lines)
        if lines is None:
            continue
        address_token = _declaration_address(
            lines, int(declaration["line"]), int(declaration["end_line"])
        )
        if address_token is None:
            match = re.fullmatch(r"Function([0-9a-fA-F]{6,8})", name, flags=re.IGNORECASE)
            if match is None or not declaration.get("is_definition"):
                continue
            address = int(match.group(1), 16)
        else:
            address = int(address_token, 16)
        owner = None
        target = ""
        for family in families:
            owner = _owner_at(
                address,
                target=family.target,
                symbols_by_key=symbols_by_key,
                hulls=hulls,
                ranges=ranges,
                bodies=bodies,
            )
            if owner is not None:
                target = family.target
                break
        if owner is None:
            continue
        family_name, _ = _owner_detail(owner)
        key = (target, address, source_file, int(declaration.get("line") or 0))
        if key in seen_placeholders:
            continue
        seen_placeholders.add(key)
        violations.append(
            {
                "kind": "oracle-placeholder",
                "family": family_name,
                "address": _format_address(address),
                "source": source_file,
                "line": int(declaration.get("line") or 0),
                "name": qualified,
                "detail": (
                    f"{source_file}:{declaration.get('line')}: {qualified} at "
                    f"{_format_address(address)} is owned by the {family_name} boundary; "
                    "recover it at that owner, do not decompile"
                ),
            }
        )

    claims_path = repo_dir / "evidence/reviewed/wiz8/claims.csv"
    for claim in claim_rows:
        family = _family_for_origin(claim["origin"], claim["predicate"], families)
        if family is None:
            continue
        if claim["entity_kind"].strip() != "function":
            continue
        address = parse_hex(claim["entity_key"], field="entity_key", path=claims_path) or 0
        owned = oracle_markers.get((family.target, address))
        if owned is not None and family.accepts_owner_marker(owned[0], owned[1]):
            continue
        library_owner = library_owners.get((family.target, address))
        if library_owner is not None and library_owner.family == family.name:
            continue
        if family.name_origins:
            origin_token = min(family.name_origins)
        elif family.claim_origins:
            origin_token = min(family.claim_origins)
        else:
            origin_token = family.name
        owner_hint = (
            f"under {', '.join(family.source_roots)}"
            if family.source_roots
            else "via a LIBRARY marker or library CSV row"
        )
        violations.append(
            {
                "kind": "missing-oracle-owner",
                "family": family.name,
                "address": _format_address(address),
                "claim_id": claim["claim_id"],
                "name": claim.get("value", "").strip(),
                "detail": (
                    f"{claim['claim_id']}: {origin_token} retained identity "
                    f"{_format_address(address)} requires a {family.name} owner {owner_hint}; "
                    + (
                        f"found {owned[1]} marker in {owned[0]}"
                        if owned
                        else "no oracle FUNCTION/LIBRARY marker or library CSV row"
                    )
                ),
            }
        )

    violations.sort(key=lambda item: (item["kind"], item.get("address", ""), item["detail"]))
    return violations


def source_oracle_report(
    repo_dir: Path,
    *,
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> dict[str, Any]:
    """Summarize oracle ownership and write the full artifact under ``build/``."""

    index = _load_index(repo_dir)
    claims = load_claims(repo_dir)
    symbols = oracle_symbols(repo_dir, index=index, claims=claims, families=families)
    hulls = contribution_hulls(symbols, families)
    ranges = _range_owners(families)
    bodies = _sized_body_owners(families)
    violations = source_oracle_violations(repo_dir, index=index, claims=claims, families=families)
    artifact = {
        "schema": "wiz8.source-oracle-v2",
        "families": [
            {
                "name": family.name,
                "target": family.target,
                "source_roots": list(family.source_roots),
                "name_origins": sorted(family.name_origins),
                "claim_origins": sorted(family.claim_origins),
                "reccmp_csv": family.reccmp_csv,
                "address_ranges": [
                    [_format_address(start), _format_address(end)]
                    for start, end in family.address_ranges
                ],
            }
            for family in families
        ],
        "symbols": [
            {
                "address": _format_address(item.address),
                "family": item.family,
                "target": item.target,
                "source_file": item.source_file,
                "evidence": item.evidence,
                "name": item.name,
                "marker_kind": item.marker_kind,
                "proven": item.proven,
            }
            for item in symbols
        ],
        "contribution_hulls": [
            {
                "family": hull.family,
                "target": hull.target,
                "source_file": hull.source_file,
                "start": _format_address(hull.start),
                "end": _format_address(hull.end),
            }
            for hull in hulls
        ],
        "address_ranges": [
            {
                "family": owner.family,
                "target": owner.target,
                "start": _format_address(owner.start),
                "end": _format_address(owner.end),
            }
            for owner in ranges
        ],
        "sized_bodies": [
            {
                "family": body.family,
                "target": body.target,
                "start": _format_address(body.start),
                "end": _format_address(body.end),
                "size": body.size,
            }
            for body in bodies
        ],
        "violations": violations,
    }
    path = repo_dir / "build/reports/source-oracle.json"
    atomic_json(path, artifact)
    counts: dict[str, int] = defaultdict(int)
    for item in violations:
        counts[str(item["kind"])] += 1
    by_family: dict[str, int] = defaultdict(int)
    owned_by_family: dict[str, int] = defaultdict(int)
    for item in symbols:
        (by_family if item.proven else owned_by_family)[item.family] += 1
    return {
        "status": "passed" if not violations else "failed",
        "proven_symbols": sum(by_family.values()),
        "proven_symbols_by_family": dict(by_family),
        "unproven_owned_symbols_by_family": dict(owned_by_family),
        "contribution_hulls": len(hulls),
        "address_ranges": len(ranges),
        "sized_bodies": len(bodies),
        "violations": len(violations),
        "violation_kinds": dict(counts),
        "artifact": str(path.relative_to(repo_dir)),
    }


def validate_source_oracle_ownership(
    repo_dir: Path,
    *,
    families: Sequence[OracleFamily] = ORACLE_FAMILIES,
) -> dict[str, Any]:
    report = source_oracle_report(repo_dir, families=families)
    if report["status"] != "passed":
        violations = json.loads((repo_dir / report["artifact"]).read_text(encoding="utf-8"))[
            "violations"
        ]
        rendered = [item["detail"] for item in violations]
        raise SourceOracleGateError(
            "source-oracle ownership gate failed:\n  " + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "source-oracle",
        "proven_symbols": report["proven_symbols"],
        "proven_symbols_by_family": report["proven_symbols_by_family"],
        "unproven_owned_symbols_by_family": report["unproven_owned_symbols_by_family"],
        "contribution_hulls": report["contribution_hulls"],
        "address_ranges": report["address_ranges"],
        "artifact": report["artifact"],
    }


def iter_oracle_families() -> Iterable[OracleFamily]:
    return ORACLE_FAMILIES


def extract_declaration_oracle(repository: Path, configuration: dict, destination: Path) -> dict:
    """Extract immutable source bytes for Clang; no reconstruction supplies types.

    Filename casing is normalized solely to reproduce Windows include lookup on
    Linux. Every byte stream, including required historical overlays, remains
    pinned to its Git blob and is recorded in the extraction manifest.
    """
    import hashlib
    import re
    import subprocess

    revision = configuration["revision"]
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("source oracle requires a full immutable commit ID")
    resolved = subprocess.run(
        ["git", "rev-parse", revision + "^{commit}"],
        cwd=repository,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()
    if resolved != revision:
        raise ValueError("oracle revision does not identify the exact commit")
    roots = [configuration["source_root"], *configuration.get("overlay_roots", [])]
    files = {}
    for root_index, root in enumerate(roots):
        names = subprocess.run(
            ["git", "ls-tree", "-r", "--name-only", revision, root],
            cwd=repository,
            capture_output=True,
            text=True,
            check=True,
        ).stdout.splitlines()
        if not names:
            raise ValueError("missing immutable oracle root: " + root)
        for name in names:
            relative = Path(name).relative_to(root).as_posix().lower()
            staged = ("source/" if root_index == 0 else f"overlay-{root_index}/") + relative
            if staged in files:
                raise ValueError("case-folded source oracle filename collision")
            data = subprocess.run(
                ["git", "show", revision + ":" + name],
                cwd=repository,
                capture_output=True,
                check=True,
            ).stdout
            blob = subprocess.run(
                ["git", "rev-parse", revision + ":" + name],
                cwd=repository,
                capture_output=True,
                text=True,
                check=True,
            ).stdout.strip()
            path = destination / staged
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            files[staged] = {"path": name, "blob": blob, "sha256": hashlib.sha256(data).hexdigest()}
    return {"revision": revision, "files": files}


def retained_declaration_mappings(
    repository: Path, configuration: dict, extraction: dict
) -> list[dict]:
    """Require explicit retained-contract correspondences, never infer from names."""
    rows = {row["claim_id"]: row for row in load_claims(repository)}
    mappings = []
    for correspondence in configuration["correspondences"]:
        claim = rows[correspondence["claim_id"]]
        if (
            claim["origin"] != "sgp-source"
            or claim["entity_kind"] != correspondence["current"]["kind"]
            or correspondence["original"]["kind"] != correspondence["current"]["kind"]
            or claim["value"] != correspondence["current"]["name"]
            or claim["authority"] != "source-backed"
            or claim["confidence"] not in {"strong", "high", "exact"}
            or claim["predicate"] != "accepted-identity"
            or correspondence.get("retained_declaration") is not True
        ):
            raise ValueError("source identity alone does not prove a retained declaration contract")
        original = dict(correspondence["original"])
        relative = (
            Path(original["file"]).relative_to(configuration["source_root"]).as_posix().lower()
        )
        entry = extraction["files"]["source/" + relative]
        original["file"] = "/oracle/source/" + relative
        mappings.append(
            {
                "original": {"selector": original},
                "current": {"selector": correspondence["current"]},
                "nominal_roles": correspondence.get("nominal_roles", {}),
                "basis": {
                    "kind": "source-oracle",
                    "reference": f"git:{extraction['revision']}:{entry['path']}#{claim['claim_id']}",
                    "reason": correspondence["reason"],
                    "revision": extraction["revision"],
                    "source_path": entry["path"],
                    "source_blob": entry["blob"],
                    "source_sha256": entry["sha256"],
                },
            }
        )
    return mappings
