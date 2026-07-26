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
    if name.startswith(MIDDLEWARE_PREFIXES) or module["relative_path"].casefold().startswith("miles"):
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


def is_first_party(module: dict[str, Any]) -> bool:
    return module.get("classification") == "first-party-game"


def _module_diff(modules: list[dict[str, Any]]) -> dict[str, Any]:
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for module in modules:
        groups[module["module_name"].casefold()].append(module)
    comparisons = []
    for name, members in sorted(groups.items()):
        members = sorted(members, key=lambda item: item["variant"])
        if len(members) < 2:
            comparisons.append({"module_name": name, "present_only_in": [item["variant"] for item in members]})
            continue
        baseline = next((item for item in members if item["variant"] == "gog-base"), members[0])
        for other in members:
            if other is baseline:
                continue
            left_sections = {item["name"]: item for item in baseline["sections"]}
            right_sections = {item["name"]: item for item in other["sections"]}
            changed = sorted(name for name in left_sections.keys() & right_sections.keys() if left_sections[name]["sha256"] != right_sections[name]["sha256"])
            added = sorted(right_sections.keys() - left_sections.keys())
            removed = sorted(left_sections.keys() - right_sections.keys())
            exact = baseline["sha256"] == other["sha256"]
            metadata_only = not exact and baseline["metadata_normalized_sha256"] == other["metadata_normalized_sha256"]
            size_delta = other["size"] - baseline["size"]
            comparisons.append({
                "module_name": name,
                "baseline_variant": baseline["variant"],
                "other_variant": other["variant"],
                "exact": exact,
                "metadata_only": metadata_only,
                "size_delta": size_delta,
                "delta_scale": "identical" if exact else ("small" if abs(size_delta) < max(4096, baseline["size"] // 100) else "large"),
                "changed_sections": changed,
                "added_sections": added,
                "removed_sections": removed,
                "imports_added": sorted({item["module"] for item in other["imports"]} - {item["module"] for item in baseline["imports"]}, key=str.casefold),
                "imports_removed": sorted({item["module"] for item in baseline["imports"]} - {item["module"] for item in other["imports"]}, key=str.casefold),
                "injection_indicators": (["added PE sections: " + ", ".join(added)] if added else []) + (["new imported modules"] if {item["module"] for item in other["imports"]} - {item["module"] for item in baseline["imports"]} else []),
            })
    return {"schema": "wiz8.module-diff", "comparisons": comparisons}


def inventory(settings: Settings) -> dict[str, Any]:
    variants_root = settings.work_dir / "variants"
    if not variants_root.is_dir():
        raise RuntimeError("no materialized variants; run 'wiz8 variants materialize' first")
    modules: list[dict[str, Any]] = []
    for variant_dir in sorted((path for path in variants_root.iterdir() if path.is_dir()), key=lambda p: p.name):
        for path in sorted((p for p in variant_dir.rglob("*") if p.is_file()), key=lambda p: p.relative_to(variant_dir).as_posix().casefold()):
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
    diff = _module_diff(modules)
    atomic_json(settings.build_dir / "reports" / "module-diff.json", diff)
    compiler = {
        "schema": "wiz8.compiler-evidence",
        "modules": [
            {"identity": item["identity"], "hypothesis": item["compiler_hypothesis"], "rich_header": item["rich_header"]}
            for item in modules if item["classification"] in {"first-party-game", "renderer", "fan-patch", "setup"}
        ],
    }
    atomic_json(settings.build_dir / "reports" / "compiler-evidence.json", compiler)
    _write_markdown(settings, modules, diff, compiler)
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


def _write_markdown(settings: Settings, modules: list[dict[str, Any]], diff: dict[str, Any], compiler: dict[str, Any]) -> None:
    lines = ["# Wizardry 8 module inventory", "", "| Variant | Module | Class | SHA-256 | Version | Compiler |", "|---|---|---|---|---|---|"]
    for item in modules:
        versions = item["version_resources"]
        version = versions.get("FileVersion") or versions.get("FixedFileVersion") or "unknown"
        lines.append(f"| {item['variant']} | `{item['relative_path']}` | {item['classification']} | `{item['sha256'][:16]}…` | {version} | {item['compiler_hypothesis']['family']} ({item['compiler_hypothesis']['confidence']}) |")
    lines.extend(["", "Generated from PE structures and content; classifications retain evidence in `modules.json`.", ""])
    atomic_write(settings.build_dir / "reports" / "modules.md", "\n".join(lines))
    compiler_lines = ["# Compiler evidence", ""]
    for item in compiler["modules"]:
        hypothesis = item["hypothesis"]
        compiler_lines.append(f"## `{item['identity']}`")
        compiler_lines.append("")
        compiler_lines.append(f"Hypothesis: **{hypothesis['family']}**; confidence: **{hypothesis['confidence']}**.")
        compiler_lines.append("")
        compiler_lines.extend(f"- {evidence}" for evidence in hypothesis["evidence"])
        compiler_lines.append("")
    atomic_write(settings.build_dir / "reports" / "compiler-evidence.md", "\n".join(compiler_lines))


def load_inventory(settings: Settings) -> dict[str, Any]:
    path = settings.build_dir / "manifests" / "modules.json"
    if not path.is_file():
        return inventory(settings)
    return json.loads(path.read_text(encoding="utf-8"))
