from __future__ import annotations

import re
from pathlib import Path

from .io import read_table


def load_boundary_rows(mapping_path: Path) -> list[dict[str, str]]:
    try:
        rows = list(read_table(mapping_path, schema_name="wiz8-gameplay-boundaries.csv").rows)
    except ValueError as error:
        duplicate = re.search(r"duplicate identity \('([^']+)',\)", str(error))
        if duplicate:
            raise RuntimeError(
                f"boundary map {mapping_path} repeats address {duplicate.group(1)}"
            ) from error
        raise RuntimeError(str(error)) from error
    if not rows:
        raise RuntimeError(f"boundary map is empty: {mapping_path}")
    for line, row in enumerate(rows, start=2):
        address = row["address"].strip().lower().removeprefix("0x")
        try:
            int(address, 16)
        except ValueError as error:
            raise RuntimeError(
                f"boundary map {mapping_path}:{line} has invalid address {row['address']!r}"
            ) from error
    return rows
