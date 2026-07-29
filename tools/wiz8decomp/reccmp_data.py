"""Generate reccmp's disposable view of reviewed WIZ8 identities."""

from __future__ import annotations

import csv
import io
from dataclasses import dataclass
from pathlib import Path

from .evidence.functions import load_function_identities
from .paths import atomic_write

LIBRARY_OWNERS = frozenset(
    {
        "infozip-library",
        "msvc6-runtime",
        "sgp-compression",
        "sgp-shared",
        "zlib-library",
    }
)


@dataclass
class ReccmpEntity:
    address: int
    symbol: str
    size: int | None
    owner: str


def _boundary_entities(path: Path) -> dict[int, ReccmpEntity]:
    entities: dict[int, ReccmpEntity] = {}
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            address = int(row["address"], 16)
            entities[address] = ReccmpEntity(
                address=address,
                symbol=row["symbol"].strip(),
                size=int(row["size"], 0) if row["size"].strip() else None,
                owner=row["owner"].strip(),
            )
    return entities


def render_wiz8_data_source(repository: Path) -> str:
    """Project canonical evidence into reccmp's pipe-delimited schema."""

    entities = _boundary_entities(repository / "config/reccmp/wiz8-gameplay-boundaries.csv")
    identities = load_function_identities(
        repository / "evidence/reviewed/wiz8/function-provenance.csv",
        program="wiz8",
    )
    for identity in identities:
        existing = entities.get(identity.address)
        entities[identity.address] = ReccmpEntity(
            address=identity.address,
            symbol=identity.name,
            size=identity.size
            if identity.size is not None
            else (existing.size if existing else None),
            owner=identity.owner or (existing.owner if existing else ""),
        )

    output = io.StringIO(newline="")
    writer = csv.writer(output, delimiter="|", lineterminator="\n")
    writer.writerow(("address", "symbol", "size", "type"))
    for entity in sorted(entities.values(), key=lambda item: item.address):
        writer.writerow(
            (
                f"{entity.address:08x}",
                entity.symbol,
                "" if entity.size is None else entity.size,
                "library" if entity.owner in LIBRARY_OWNERS else "function",
            )
        )
    return output.getvalue()


def write_wiz8_data_source(repository: Path) -> Path:
    destination = repository / "build/reccmp/wiz8-symbols.csv"
    atomic_write(destination, render_wiz8_data_source(repository))
    return destination
