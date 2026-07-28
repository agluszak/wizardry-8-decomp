from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from ..config import Settings
from ..inputs.scan import load_manifest
from ..manifest_models import load_variant_module_inventory, load_variant_provenance
from ..paths import atomic_json, atomic_write


def _load(path: Path, default: Any) -> Any:
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else default


def bootstrap_report(settings: Settings) -> dict[str, Any]:
    inputs = load_manifest(settings).model_dump(mode="json", by_alias=True)
    variants = load_variant_provenance(settings).model_dump(mode="json", by_alias=True)
    variant_modules = load_variant_module_inventory(settings).model_dump(mode="json", by_alias=True)
    modules = _load(settings.build_dir / "manifests" / "modules.json", {"modules": []})
    module_diff = _load(settings.build_dir / "reports" / "module-diff.json", {"comparisons": []})
    compilers = _load(settings.build_dir / "reports" / "compiler-evidence.json", {"modules": []})
    imports = _load(settings.build_dir / "manifests" / "ghidra-import.json", {"programs": []})
    cross = _load(
        settings.build_dir / "reports" / "cross-build-summary.json", {"status": "not generated"}
    )
    first_party = [
        item
        for item in modules["modules"]
        if item.get("classification") in {"first-party-game", "renderer"}
    ]
    report = {
        "schema": "wiz8.bootstrap-report",
        "inputs": inputs["files"],
        "variants": variants.get("variants", []),
        "variant_module_inventory": variant_modules["variants"],
        "modules": modules["modules"],
        "module_diff": module_diff,
        "compiler_evidence": compilers,
        "ghidra_imports": imports["programs"],
        "cross_build": cross,
        "remaining_uncertainties": [
            "InstallShield demo extraction requires unshield unless a deterministic static alternative is available.",
            "Compiler version remains a ranked evidence conclusion until Rich records and runtime signatures are reviewed.",
            "Fan-patch modules are separate targets and must not be attributed to original Wizardry source.",
        ],
        "recommended_next_targets": [item["identity"] for item in first_party[:10]],
    }
    atomic_json(settings.build_dir / "reports" / "bootstrap.json", report)
    lines = [
        "# Wizardry 8 bootstrap report",
        "",
        f"Discovered **{len(inputs['files'])}** configured/local input files, **{len(variants.get('variants', []))}** variants, and **{len(modules['modules'])}** PE modules.",
        "",
        "## Inputs",
        "",
    ]
    for item in inputs["files"]:
        lines.append(
            f"- `{item['relative_path']}` — {item['detected_type']}, role `{item.get('configured_role') or 'unassigned'}`, SHA-256 `{item['sha256']}`"
        )
    lines.extend(["", "## Ghidra programs", ""])
    lines.extend(
        f"- `{item['program']}` — {item['status']}, {item.get('function_count', 'unknown')} functions"
        for item in imports["programs"]
    )
    lines.extend(["", "## Remaining uncertainties", ""])
    lines.extend(f"- {item}" for item in report["remaining_uncertainties"])
    lines.extend(["", "## Recommended next targets", ""])
    lines.extend(f"- `{item}`" for item in report["recommended_next_targets"])
    lines.append("")
    atomic_write(settings.build_dir / "reports" / "bootstrap.md", "\n".join(lines))
    return report
