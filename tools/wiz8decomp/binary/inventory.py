from __future__ import annotations

import csv
import io
import json
from collections import defaultdict
from collections.abc import Callable
from typing import Any

from ..config import Settings
from ..manifest_models import (
    VariantModuleInventory,
    VariantModuleSummary,
    variant_module_inventory_path,
    write_generated_document,
)
from ..paths import atomic_json, atomic_write
from .pe import inspect_pe, is_pe

MIDDLEWARE_PREFIXES = ("mss", "bink", "smack", "mp3dec", "lua", "d3dim", "secdrv", "drvmgt")


def classify_module(module: dict[str, Any]) -> tuple[str, list[str]]:
    name = module["module_name"].casefold()
    reasons: list[str] = []
    if name.startswith(MIDDLEWARE_PREFIXES) or module["relative_path"].casefold().startswith(
        "miles"
    ):
        reasons.append("known middleware/system module name")
        return "middleware", reasons
    if name in {"wiz8.exe", "wiz8new.exe"}:
        reasons.append("official or unofficial-patch main executable")
        return "first-party-game", reasons
    if name == "wiz8_v128.exe" or name in {"wiz8.dll", "cfagent1.28.dll"}:
        reasons.append("fan-patch executable/support module")
        return "fan-patch", reasons
    if name.startswith("srdd_") or name == "sr.dll":
        reasons.append("Wizardry renderer module naming")
        return "renderer", reasons
    if "setup" in name or "launcher" in name or name == "3dsetup.exe":
        reasons.append("setup/configuration executable naming")
        return "setup", reasons
    return "unclassified", ["no curated classification rule matched"]


def representative_modules(
    settings: Settings, predicate: Callable[[dict[str, Any]], bool]
) -> tuple[list[dict[str, Any]], dict[str, str]]:
    """One module per distinct payload, preferring the canonical variant.

    Several variants ship byte-identical modules. Emitting each would multiply
    every row without adding an observation, and attributing rows to whichever
    variant happened to sort first would bury the canonical matching target under
    an incidental name. Returns the chosen modules and the aliases collapsed.
    """
    import yaml

    from ..ghidra.project import program_name

    modules = [module for module in load_inventory(settings)["modules"] if predicate(module)]
    if not modules:
        raise RuntimeError("no matching modules in the inventory; run 'wiz8 inventory' first")
    canonical = yaml.safe_load(
        (settings.repo_dir / "config" / "variants.yml").read_text(encoding="utf-8")
    )["canonical_matching_target"]["variant"]

    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for module in modules:
        groups[module["sha256"]].append(module)
    chosen: list[dict[str, Any]] = []
    aliases: dict[str, str] = {}
    for members in groups.values():
        members.sort(
            key=lambda item: (item["variant"] != canonical, item["variant"], item["relative_path"])
        )
        chosen.append(members[0])
        for other in members[1:]:
            aliases[program_name(other)] = program_name(members[0])
    chosen.sort(key=lambda item: (item["variant"], item["relative_path"]))
    return chosen, dict(sorted(aliases.items()))


def inventory(settings: Settings) -> dict[str, Any]:
    variants_root = settings.work_dir / "variants"
    if not variants_root.is_dir():
        raise RuntimeError("no materialized variants; run 'wiz8 corpus materialize' first")
    modules: list[dict[str, Any]] = []
    for variant_dir in sorted(
        (path for path in variants_root.iterdir() if path.is_dir()), key=lambda p: p.name
    ):
        for path in sorted(
            (p for p in variant_dir.rglob("*") if p.is_file()),
            key=lambda p: p.relative_to(variant_dir).as_posix().casefold(),
        ):
            if not is_pe(path):
                continue
            module = inspect_pe(path, variant_dir.name, path.relative_to(variant_dir).as_posix())
            classification, reasons = classify_module(module)
            module["classification"] = classification
            module["classification_evidence"] = reasons
            modules.append(module)
    result = {"schema": "wiz8.modules", "modules": modules}
    atomic_json(settings.build_dir / "manifests" / "modules.json", result)
    variants = VariantModuleInventory(
        variants=[
            VariantModuleSummary(
                id=variant,
                module_count=sum(item["variant"] == variant for item in modules),
            )
            for variant in sorted({item["variant"] for item in modules})
        ]
    )
    write_generated_document(variant_module_inventory_path(settings), variants)
    _write_source_evidence(settings, modules)
    return result


def _write_source_evidence(settings: Settings, modules: list[dict[str, Any]]) -> None:
    source_buffer = io.StringIO(newline="")
    assertion_buffer = io.StringIO(newline="")
    source_writer = csv.writer(source_buffer, lineterminator="\n")
    assertion_writer = csv.writer(assertion_buffer, lineterminator="\n")
    source_writer.writerow(["variant", "module", "source_path"])
    assertion_writer.writerow(["variant", "module", "assertion"])
    for module in modules:
        for value in module["source_paths"]:
            source_writer.writerow([module["variant"], module["relative_path"], value])
        for value in module["assertion_strings"]:
            assertion_writer.writerow([module["variant"], module["relative_path"], value])
    atomic_write(settings.build_dir / "evidence" / "source-paths.csv", source_buffer.getvalue())
    atomic_write(settings.build_dir / "evidence" / "assertions.csv", assertion_buffer.getvalue())


def load_inventory(settings: Settings) -> dict[str, Any]:
    path = settings.build_dir / "manifests" / "modules.json"
    if not path.is_file():
        return inventory(settings)
    return json.loads(path.read_text(encoding="utf-8"))
