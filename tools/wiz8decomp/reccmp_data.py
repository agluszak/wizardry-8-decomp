"""Generate reccmp's disposable view of reviewed WIZ8 identities."""

from __future__ import annotations

import csv
import io
from dataclasses import dataclass
from pathlib import Path

from .evidence.claims import load_claims
from .paths import atomic_write
from .source_index import source_functions


@dataclass
class ReccmpEntity:
    address: int
    symbol: str
    size: int | None
    kind: str


def render_wiz8_data_source(repository: Path) -> str:
    """Project canonical evidence into reccmp's pipe-delimited schema."""

    entities: dict[int, ReccmpEntity] = {}
    model = source_functions(repository)
    for function in model.values():
        entities[function.address] = ReccmpEntity(
            address=function.address,
            symbol=function.name,
            size=None,
            kind="library" if function.marker_kind == "LIBRARY" else "function",
        )

    # Unrecovered functions have no owned declaration yet.  Their reviewed
    # identities remain evidence claims over Ghidra entities; a source claim
    # at the same address always wins.
    for claim in load_claims(repository):
        if claim["entity_kind"].strip() != "function":
            continue
        if claim["predicate"].strip() != "accepted-identity":
            continue
        address = int(claim["entity_key"], 16)
        if address in model:
            continue
        origin = set(claim["origin"].split("|"))
        entities[address] = ReccmpEntity(
            address=address,
            symbol=claim["value"].strip(),
            size=None,
            kind=("library" if origin & {"original-source", "sgp-source"} else "function"),
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
                entity.kind,
            )
        )
    return output.getvalue()


def write_wiz8_data_source(repository: Path) -> Path:
    destination = repository / "build/reccmp/wiz8-symbols.csv"
    atomic_write(destination, render_wiz8_data_source(repository))
    return destination
