"""Warm the reusable Ghidra project for a comparison target."""

from __future__ import annotations

import argparse
import json

from wiz8decomp.comparison import all_source_addresses, compare_selected
from wiz8decomp.config import load_settings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("target")
    parser.add_argument("--count", type=int, default=33)
    args = parser.parse_args()

    settings = load_settings()
    target = args.target.upper()
    addresses = all_source_addresses(settings.repo_dir, target)[: args.count]
    if not addresses:
        raise RuntimeError(f"no recovered functions for {target}")

    result = compare_selected(
        settings.repo_dir,
        target,
        addresses,
        settings.ghidra_install_dir,
        classify_source_non_emissions=True,
    )
    print(
        json.dumps(
            {
                "target": target,
                "requested": len(addresses),
                "ok": result["ok"],
                "counts": result["counts"],
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
