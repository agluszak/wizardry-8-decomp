from __future__ import annotations

import fnmatch
import re
from pathlib import Path
from typing import Any

import yaml

from ..binary.inventory import load_inventory
from ..config import Settings


def program_name(module: dict[str, Any]) -> str:
    stem = re.sub(r"[^a-z0-9]+", "-", Path(module["relative_path"]).stem.casefold()).strip("-")
    return f"wiz8--{module['variant']}--{stem}--{module['sha256'][:12]}"


def _rules(settings: Settings) -> list[dict[str, Any]]:
    data = yaml.safe_load(
        (settings.repo_dir / "config" / "modules.yml").read_text(encoding="utf-8")
    )
    return data["selection_rules"]


def configured_modules(
    settings: Settings,
    *,
    all_modules: bool = False,
    variant: str | None = None,
    requested_program: str | None = None,
) -> list[dict[str, Any]]:
    modules = load_inventory(settings)["modules"]
    rules = _rules(settings)
    selected = []
    for module in modules:
        module["program_name"] = program_name(module)
        import_enabled = any(
            fnmatch.fnmatch(module["module_name"].casefold(), rule["match"].casefold())
            and rule.get("import", False)
            and module["variant"] in rule.get("variants", [module["variant"]])
            for rule in rules
        )
        if not import_enabled:
            continue
        if variant and module["variant"] != variant:
            continue
        if requested_program and requested_program not in {
            module["program_name"],
            module["module_name"],
            f"{module['variant']}/{module['module_name']}",
        }:
            continue
        selected.append(module)
    if not all_modules and variant is None and requested_program is None:
        selected = [
            item
            for item in selected
            if item["variant"] == "gog-base" and item["module_name"].casefold() == "wiz8.exe"
        ]
    selected.sort(key=lambda item: item["program_name"])
    if requested_program and not selected:
        raise ValueError(f"no configured module matches program selector: {requested_program}")
    return selected


def resolve_program_name(settings: Settings, selector: str | None) -> str:
    # Reproducible FID seed programs are project-owned analysis inputs rather
    # than configured game modules.  Their complete content-addressed names may
    # be queried directly for compiler comparison.
    if selector and selector.startswith("fid--"):
        return selector
    modules = configured_modules(settings, all_modules=True)
    names = [item["program_name"] for item in modules]
    canonical = [
        item["program_name"]
        for item in modules
        if item["variant"] == "gog-base" and item["module_name"].casefold() == "wiz8.exe"
    ]
    if selector is not None and selector.casefold() == "wiz8" and len(canonical) == 1:
        return canonical[0]
    if selector:
        exact = [name for name in names if name == selector]
        prefixes = [name for name in names if name.startswith(selector)]
        aliases = [
            item["program_name"]
            for item in modules
            if selector in {item["module_name"], f"{item['variant']}/{item['module_name']}"}
        ]
        matches = sorted(set(exact or aliases or prefixes))
        if len(matches) != 1:
            raise ValueError(
                f"program selector {selector!r} matched {len(matches)} programs: {', '.join(matches)}"
            )
        return matches[0]
    if len(canonical) == 1:
        return canonical[0]
    if names:
        return names[0]
    raise ValueError("no configured Ghidra programs; run inventory first")


def module_for_program(settings: Settings, name: str) -> dict[str, Any]:
    matches = [
        item
        for item in configured_modules(settings, all_modules=True)
        if item["program_name"] == name
    ]
    if len(matches) != 1:
        raise ValueError(f"program has no unique module record: {name}")
    return matches[0]
