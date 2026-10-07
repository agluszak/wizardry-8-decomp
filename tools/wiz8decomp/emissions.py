"""Generate deterministic reccmp emission data from reviewed inventory.

Compiler-generated identities live outside authored C++. The reviewed inventory
and narrow overrides are the source of truth; generated CSVs are disposable build
metadata consumed by reccmp.
"""

from __future__ import annotations

import csv
import io
from dataclasses import dataclass, replace
from pathlib import Path

from .paths import atomic_write

INVENTORY = Path("evidence/observations/compiler-emissions.csv")
OVERRIDES = Path("config/reccmp/emission_overrides.csv")
OUTPUT = Path("build/generated/reccmp")
TARGET_FILES = {
    "WIZ8": "wiz8-emissions.csv",
    "SURRENDER": "surrender-emissions.csv",
    "SREXT_JPEGIMPORTER": "srext-jpegimporter-emissions.csv",
    "SREXT_UNZIP": "srext-unzip-emissions.csv",
}


@dataclass(frozen=True)
class Emission:
    target: str
    address: int
    symbol: str
    name: str
    type: str
    source_files: tuple[str, ...] = ()
    recomp_selector: str = ""
    selector_is_symbol: bool = False


def read_emissions(text: str) -> list[Emission]:
    rows: dict[tuple[str, int], Emission] = {}
    for row in csv.DictReader(io.StringIO(text), delimiter="|"):
        flag = row.get("selector_is_symbol", "").strip().lower()
        if flag not in {"", "true", "false", "1", "0"}:
            raise ValueError(f"invalid emission selector flag: {flag}")
        emission = Emission(
            target=row["target"].upper(),
            address=int(row["address"], 16),
            symbol=row["symbol"],
            name=row["name"],
            type=row["type"].lower(),
            source_files=tuple(filter(None, row.get("source_files", "").split(";"))),
            recomp_selector=row.get("recomp_selector", ""),
            selector_is_symbol=flag in {"true", "1"},
        )
        if emission.target not in TARGET_FILES or emission.type not in {
            "synthetic",
            "template",
            "global",
        }:
            raise ValueError(f"invalid emission target/type: {emission}")
        if emission.address <= 0 or not (emission.symbol or emission.name):
            raise ValueError(f"emission requires an address and symbol or name: {emission}")
        key = (emission.target, emission.address)
        if key in rows:
            raise ValueError(f"duplicate emission identity: {key}")
        rows[key] = emission
    return list(rows.values())


def emission_inventory(repository: Path, target: str | None = None) -> list[Emission]:
    path = repository / INVENTORY
    rows = read_emissions(path.read_text(encoding="utf-8")) if path.is_file() else []
    identities = {(row.target, row.address): row for row in rows}
    overrides = repository / OVERRIDES
    if overrides.is_file():
        for row in read_emissions(overrides.read_text(encoding="utf-8")):
            old = identities.get((row.target, row.address))
            identities[row.target, row.address] = replace(
                row, source_files=row.source_files or (old.source_files if old else ())
            )
    return sorted(
        (row for row in identities.values() if target is None or row.target == target.upper()),
        key=lambda row: (row.target, row.address),
    )


def render_emissions(rows: list[Emission]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, delimiter="|", lineterminator="\n")
    writer.writerow(("address", "symbol", "name", "type", "recomp_selector", "selector_is_symbol"))
    for row in sorted(rows, key=lambda item: item.address):
        writer.writerow(
            (
                f"{row.address:08x}",
                row.symbol,
                row.name,
                row.type,
                row.recomp_selector,
                ("true" if row.selector_is_symbol else "false") if row.recomp_selector else "",
            )
        )
    return stream.getvalue()


def generate_emissions(
    repository: Path,
    *,
    targets: tuple[str, ...] | None = None,
) -> dict[str, object]:
    rows = emission_inventory(repository)
    selected = set(TARGET_FILES) if targets is None else set(targets)
    if unknown := selected - TARGET_FILES.keys():
        raise ValueError(f"unknown emission targets: {sorted(unknown)}")

    count = 0
    for target, filename in TARGET_FILES.items():
        if target not in selected:
            continue
        target_rows = [row for row in rows if row.target == target]
        count += len(target_rows)
        text = render_emissions(target_rows)
        output = repository / OUTPUT / filename
        if not output.is_file() or output.read_text(encoding="utf-8") != text:
            atomic_write(output, text)

    return {"path": str(OUTPUT), "emissions": count}
