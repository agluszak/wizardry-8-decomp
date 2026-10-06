"""Generate reccmp emission data without manufacturing C++ declarations.

The original-address inventory is evidence, not a recipe for code generation.
reccmp owns PDB ingestion and pairing; this adapter consumes its catalog rather
than implementing another matcher. Ambiguous or absent emissions retain their
inventory identity until independent evidence or an explicit override resolves it.
"""

from __future__ import annotations

import csv
import hashlib
import io
from collections import defaultdict
from dataclasses import dataclass, replace
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import TYPE_CHECKING

from reccmp.types import EntityType, ImageId

from .paths import atomic_write

if TYPE_CHECKING:
    from reccmp.compare import Compare

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
    """Read original identities; conflicting duplicate addresses are errors."""
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
    """Original inventory plus narrowly reviewed overrides, never source markers."""
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


def derive_emissions(rows: list[Emission], engine: Compare) -> list[Emission]:
    """Prefer exact PDB symbols from existing pairs and corroborated vtable slots.

    Never use function order, body resemblance, or a demangled-name rewrite to
    choose a symbol. A slot must agree across all paired tables exposing that
    original address. Conflicting slots (including ICF aliases) stay explicit.
    """
    slot_symbols: dict[int, set[str]] = defaultdict(set)
    symbol_addresses: dict[str, set[int]] = defaultdict(set)
    for table in engine.get_vtables():
        from reccmp.compare.vtables import compare_vtable

        comparison = compare_vtable(engine.db, engine.orig_bin, engine.recomp_bin, table)
        for slot in comparison.slots:
            if slot.orig_raw is None or slot.recomp_raw is None:
                continue
            # Use the raw slot's PDB symbol, not a thunk-resolved destination.
            entity = engine.get(ImageId.RECOMP, slot.recomp_raw)
            symbol = entity.fact(ImageId.RECOMP, "symbol") if entity is not None else None
            if symbol:
                slot_symbols[slot.orig_raw].add(str(symbol))
                symbol_addresses[str(symbol)].add(slot.orig_raw)

    result = []
    for row in rows:
        match = engine.get_match(row.address)
        symbol = (
            match.fact(ImageId.RECOMP, "symbol")
            if match is not None
            and match.entity_type
            == (EntityType.DATA if row.type == "global" else EntityType.FUNCTION)
            else None
        )
        candidates = slot_symbols.get(row.address, set())
        if symbol and candidates and candidates != {symbol}:
            # Do not silently overwrite a source/inventory match with a vtable guess.
            result.append(row)
            continue
        if not symbol and row.type == "synthetic" and len(candidates) == 1:
            candidate = next(iter(candidates))
            if candidate.startswith(("??_G", "??_E")) and symbol_addresses[candidate] == {
                row.address
            }:
                symbol = candidate
        if row.symbol and symbol and row.symbol != symbol:
            raise ValueError(f"PDB contradicts explicit emission symbol at {row.address:08x}")
        result.append(
            replace(row, recomp_selector=str(symbol), selector_is_symbol=True) if symbol else row
        )
    return result


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


def _output_signature(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def generate_emissions(
    repository: Path,
    *,
    derive: bool = False,
    targets: tuple[str, ...] | None = None,
) -> dict[str, object]:
    """Bootstrap baseline CSVs automatically; PDB enrichment is explicit.

    Reuse valid output for unchanged inventory, including explicitly enriched
    selectors. Removed, corrupt or outdated generated files get fresh baselines.
    Derivation uses temporary baselines rather than confirming its own previous
    generated selectors, and failures leave successful output intact.
    """
    rows = emission_inventory(repository)
    selected = set(TARGET_FILES) if targets is None else set(targets)
    if unknown := selected - TARGET_FILES.keys():
        raise ValueError(f"unknown emission targets: {sorted(unknown)}")
    rendered: dict[Path, tuple[str, str]] = {}
    with TemporaryDirectory(prefix="wiz8-emissions-") as temporary:
        for target, filename in TARGET_FILES.items():
            if target not in selected:
                continue
            target_rows = [row for row in rows if row.target == target]
            baseline_text = render_emissions(target_rows)
            seed = _output_signature(baseline_text)
            output = repository / OUTPUT / filename
            stamp = output.with_suffix(".sha256")
            if not derive and output.is_file() and stamp.is_file():
                expected = f"{seed} {_output_signature(output.read_text(encoding='utf-8'))}\n"
                if stamp.read_text(encoding="utf-8") == expected:
                    continue
            if derive and target_rows:
                from reccmp.compare import Compare

                from .comparison import comparison_target

                baseline = Path(temporary) / filename
                baseline.write_text(baseline_text, encoding="utf-8")
                configured = comparison_target(repository, target, bootstrap_emissions=False)
                prepared = replace(
                    configured,
                    data_sources=[
                        baseline if path.resolve() == output.resolve() else path
                        for path in configured.data_sources
                    ],
                )
                engine = Compare.from_target(prepared)
                target_rows = derive_emissions(target_rows, engine)
            rendered[output] = (seed, render_emissions(target_rows))
    for output, (seed, text) in rendered.items():
        atomic_write(output, text)
        atomic_write(output.with_suffix(".sha256"), f"{seed} {_output_signature(text)}\n")
    return {
        "path": str(OUTPUT),
        "emissions": sum(row.target in selected for row in rows),
        "derived": derive,
    }
