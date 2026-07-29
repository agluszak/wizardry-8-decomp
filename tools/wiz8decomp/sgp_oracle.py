from __future__ import annotations

import csv
import hashlib
import io
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pefile
from reccmp.compare.exact import (
    CoffFunction,
    mask_relocations,
    parse_coff_functions,
)
from reccmp.compare.exact import (
    stable_ranges as _stable_ranges,
)

from .config import Settings
from .paths import atomic_write, sha256_file

PROJECT_FLAGS = ("/O2", "/Ob2", "/G5", "/MD")
NEAR_MATCH_THRESHOLD = 0.75
REPORT = Path("build/reports/sgp/harness.csv")

BUILDS = (
    {"id": "demo", "variant": "demo", "module": "Wiz8.EXE"},
    {"id": "gog_base", "variant": "gog-base", "module": "Wiz8.exe"},
    {"id": "gog_1261", "variant": "gog-1261", "module": "Wiz8.exe"},
    {"id": "gog_1261_new", "variant": "gog-1261", "module": "Wiz8New.exe"},
    {"id": "gog_128_base", "variant": "gog-128", "module": "Wiz8.exe"},
    {
        "id": "gog_128_patch",
        "variant": "gog-128",
        "module": "Wiz8_v128.exe",
    },
    {
        "id": "retail_2001_12_23",
        "variant": "retail-2001-12-23",
        "module": "Wiz8.exe",
        "unavailable_reason": "protected-executable",
    },
)

UNITS = (
    {"id": "directdraw", "source": "DirectDraw Calls.c", "target": "WIZ8_SGP_RUNTIME"},
    {"id": "random", "source": "Random.c", "target": "WIZ8_SGP_WHOLE"},
    {"id": "compression", "source": "Compression.c", "target": "WIZ8_SGP_ANALYSIS"},
    {"id": "fileman", "source": "FileMan.c", "target": "WIZ8_SGP_RUNTIME"},
    {"id": "librarydatabase", "source": "LibraryDataBase.c", "target": "WIZ8_SGP_RUNTIME"},
    {"id": "dbman", "source": "DbMan.c", "target": "WIZ8_SGP_ANALYSIS"},
    {"id": "container", "source": "Container.c", "target": "WIZ8_SGP_RUNTIME"},
    {"id": "debug", "source": "DEBUG.C", "target": "WIZ8_SGP_RUNTIME"},
    {
        "id": "exceptionhandling",
        "source": "ExceptionHandling.cpp",
        "target": "WIZ8_SGP_ANALYSIS",
        "expected_empty": True,
    },
    {
        "id": "sgp",
        "source": "sgp.c",
        "target": "WIZ8_SGP_RUNTIME",
        "functions": ("GetRuntimeSettings", "ProcessCommandLine"),
    },
    {"id": "timer", "source": "timer.c", "target": "WIZ8_SGP_WHOLE"},
    {
        "id": "input",
        "source": "input.c",
        "target": "WIZ8_SGP_RUNTIME",
        "functions": (
            "KeyboardHandler",
            "MouseHandler",
            "InitializeInputManager",
            "ShutdownInputManager",
            "QueueEvent",
            "DequeueEvent",
            "FreeMouseCursor",
            "GetMouseWheelDeltaValue",
        ),
    },
)

CLASSIFICATION_RANK = {
    "unavailable": 0,
    "absent-or-stripped": 1,
    "ambiguous-generic": 2,
    "near-source-with-wiz8-modifications": 3,
    "relocation-equivalent": 4,
    "exact": 5,
}


@dataclass(frozen=True)
class BuildText:
    identifier: str
    module_sha256: str
    base: int | None
    data: bytes | None
    unavailable_reason: str | None


def _candidate_positions(text: bytes, body: bytes, offsets: tuple[int, ...]) -> list[int]:
    ranges = _stable_ranges(len(body), offsets)
    if not ranges:
        return list(range(max(0, len(text) - len(body) + 1)))
    anchor_start, anchor_end = max(ranges, key=lambda item: item[1] - item[0])
    anchor = body[anchor_start:anchor_end]
    if len(anchor) < 4:
        return list(range(max(0, len(text) - len(body) + 1)))
    positions = []
    cursor = 0
    while True:
        found = text.find(anchor, cursor)
        if found < 0:
            break
        candidate = found - anchor_start
        if 0 <= candidate <= len(text) - len(body):
            positions.append(candidate)
        cursor = found + 1
    return positions


def classify_body(
    function: CoffFunction,
    text: bytes,
    *,
    near_threshold: float,
) -> dict[str, Any]:
    masked = function.masked_body
    positions = _candidate_positions(text, masked, function.relocation_offsets)
    hits = []
    raw_hits = []
    stable = [
        index
        for start, end in _stable_ranges(len(masked), function.relocation_offsets)
        for index in range(start, end)
    ]
    for position in positions:
        window = text[position : position + len(masked)]
        if mask_relocations(window, function.relocation_offsets) == masked:
            hits.append(position)
            if window == function.body:
                raw_hits.append(position)
    if len(hits) > 1:
        return {"classification": "ambiguous-generic", "positions": hits, "similarity": 1.0}
    if len(hits) == 1:
        classification = "exact" if hits == raw_hits else "relocation-equivalent"
        return {"classification": classification, "positions": hits, "similarity": 1.0}

    best_position = None
    best_similarity = 0.0
    denominator = len(stable) or len(masked)
    for position in positions:
        window = text[position : position + len(masked)]
        matches = sum(window[index] == masked[index] for index in stable)
        similarity = matches / denominator
        if similarity > best_similarity:
            best_position, best_similarity = position, similarity
    if best_position is not None and best_similarity >= near_threshold:
        return {
            "classification": "near-source-with-wiz8-modifications",
            "positions": [best_position],
            "similarity": best_similarity,
        }
    return {"classification": "absent-or-stripped", "positions": [], "similarity": best_similarity}


def _load_builds(settings: Settings) -> list[BuildText]:
    builds = []
    for record in BUILDS:
        path = settings.work_dir / "variants" / record["variant"] / record["module"]
        if not path.is_file():
            builds.append(BuildText(record["id"], "", None, None, "module-missing"))
            continue
        module_hash = sha256_file(path)
        if record.get("unavailable_reason"):
            builds.append(
                BuildText(
                    record["id"],
                    module_hash,
                    None,
                    None,
                    record.get("unavailable_reason", "configured-unavailable"),
                )
            )
            continue
        image = pefile.PE(str(path), fast_load=True)
        text = next(
            (section for section in image.sections if section.Name.rstrip(b"\0") == b".text"),
            None,
        )
        if text is None:
            builds.append(BuildText(record["id"], module_hash, None, None, "no-text-section"))
            continue
        builds.append(
            BuildText(
                record["id"],
                module_hash,
                image.OPTIONAL_HEADER.ImageBase + text.VirtualAddress,
                text.get_data(),
                None,
            )
        )
    return builds


def _compile_units(
    settings: Settings,
    units: list[dict[str, Any]],
) -> dict[str, list[tuple[tuple[str, ...], list[CoffFunction]]]]:
    from .build import build_target

    source = settings.repo_dir / "third_party/sfi-sgp/sgp"
    if not source.is_dir():
        raise RuntimeError(f"vendored SGP source is missing: {source}")
    if not (source / "SFI Source Code license agreement.txt").is_file():
        raise RuntimeError("vendored SGP source is missing its required SFI license")
    results = {unit["id"]: [] for unit in units}
    for target in sorted({unit["target"] for unit in units}):
        build_target(settings, target)
    output = settings.repo_dir / "build/decomp"
    for unit in units:
        target = unit["target"]
        target_root = output / "CMakeFiles" / f"{target}.dir"
        source_object = f"{Path(unit['source']).name.replace(' ', '_')}.obj".casefold()
        objects = sorted(
            obj for obj in target_root.rglob("*.obj") if obj.name.casefold() == source_object
        )
        if len(objects) != 1:
            raise RuntimeError(
                f"{target} produced {len(objects)} objects for "
                f"{unit['source']}; expected exactly one"
            )
        try:
            functions = parse_coff_functions(objects[0])
        except RuntimeError as error:
            if unit.get("expected_empty") and "exposes no external .text functions" in str(error):
                functions = []
            else:
                raise
        selected = unit.get("functions")
        if selected is not None:
            selected_names = set(selected)
            available_names = {function.name for function in functions}
            missing = sorted(selected_names - available_names)
            if missing:
                raise RuntimeError(
                    f"{target} did not expose selected functions: " + ", ".join(missing)
                )
            functions = [function for function in functions if function.name in selected_names]
        if unit.get("expected_empty") and functions:
            names = ", ".join(function.name for function in functions)
            raise RuntimeError(f"{target} was expected to emit no functions but exposed: {names}")
        results[unit["id"]].append((PROJECT_FLAGS, functions))
    return results


def _evaluate(
    variants: list[tuple[tuple[str, ...], list[CoffFunction]]],
    builds: list[BuildText],
    threshold: float,
    preferred_flags: tuple[str, ...] | None = None,
) -> list[dict[str, Any]]:
    if len(variants) != 1:
        raise RuntimeError("SGP evaluation requires exactly one settled project profile")
    flags, functions = variants[0]
    if preferred_flags is not None and flags != preferred_flags:
        raise RuntimeError(
            f"compiled SGP profile {flags} does not match configured project flags {preferred_flags}"
        )

    evaluated = []
    for function in functions:
        matches = []
        for build in builds:
            if build.data is None:
                matches.append(
                    {
                        "build": build,
                        "classification": "unavailable",
                        "positions": [],
                        "similarity": 0.0,
                    }
                )
            else:
                matches.append(
                    {
                        "build": build,
                        **classify_body(function, build.data, near_threshold=threshold),
                    }
                )
        evaluated.append((function, matches))

    # Relocation masking can make distinct source functions identical. A
    # single binary hit is not enough to assign either identity in that case:
    # Compression.c's CompressFini and DecompressFini differ only in the
    # relocated deflateEnd/inflateEnd call target.
    fingerprints: dict[bytes, int] = {}
    for function, _matches in evaluated:
        fingerprints[function.masked_body] = fingerprints.get(function.masked_body, 0) + 1
    for function, matches in evaluated:
        if fingerprints[function.masked_body] < 2:
            continue
        for match in matches:
            if match["classification"] in {"exact", "relocation-equivalent"}:
                match["classification"] = "ambiguous-generic"
    rows = []
    for function, matches in sorted(evaluated, key=lambda item: item[0].name.casefold()):
        for match in matches:
            build = match["build"]
            addresses = (
                [build.base + position for position in match["positions"]] if build.base else []
            )
            address = (
                f"{addresses[0]:08x}"
                if len(addresses) == 1 and match["classification"] != "ambiguous-generic"
                else ""
            )
            rows.append(
                {
                    "function": function.name,
                    "size": len(function.body),
                    "flags": " ".join(flags),
                    "build": build.identifier,
                    "module_sha256": build.module_sha256,
                    "classification": match["classification"],
                    "address": address,
                    "hit_count": len(addresses),
                    "similarity": f"{match['similarity']:.6f}",
                    "relocation_masked_sha256": hashlib.sha256(function.masked_body).hexdigest(),
                    "unavailable_reason": build.unavailable_reason or "",
                }
            )
    return rows


def _write_report(path: Path, rows: list[dict[str, Any]]) -> None:
    fields = [
        "unit",
        "function",
        "size",
        "flags",
        "build",
        "module_sha256",
        "classification",
        "address",
        "hit_count",
        "similarity",
        "relocation_masked_sha256",
        "unavailable_reason",
    ]
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    atomic_write(path, stream.getvalue())


def sweep_sgp_units(
    settings: Settings,
    unit_ids: list[str] | None = None,
) -> dict[str, Any]:
    by_id = {unit["id"]: unit for unit in UNITS}
    unknown = sorted(set(unit_ids or ()) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown SGP unit(s): {', '.join(unknown)}")
    units = [by_id[item] for item in unit_ids] if unit_ids else list(UNITS)
    builds = _load_builds(settings)
    compiled = _compile_units(settings, units)
    summaries = []
    generated_rows = []
    for unit in units:
        rows = _evaluate(
            compiled[unit["id"]],
            builds,
            NEAR_MATCH_THRESHOLD,
            PROJECT_FLAGS,
        )
        generated_rows.extend({"unit": unit["id"], **row} for row in rows)
        summaries.append(
            {
                "unit": unit["id"],
                "report": str(REPORT),
                "functions": len({row["function"] for row in rows}),
                "compiled_empty": bool(unit.get("expected_empty")) and not rows,
                "rows": len(rows),
                "classifications": {
                    value: sum(row["classification"] == value for row in rows)
                    for value in CLASSIFICATION_RANK
                },
            }
        )
    report = settings.repo_dir / REPORT
    selected_ids = {unit["id"] for unit in units}
    merge_source = report
    if unit_ids and merge_source.is_file():
        with merge_source.open(newline="", encoding="utf-8") as stream:
            generated_rows.extend(
                row for row in csv.DictReader(stream) if row["unit"] not in selected_ids
            )
    unit_order = {unit["id"]: index for index, unit in enumerate(UNITS)}
    build_order = {build.identifier: index for index, build in enumerate(builds)}
    generated_rows.sort(
        key=lambda row: (
            unit_order[row["unit"]],
            row["function"].casefold(),
            build_order[row["build"]],
        )
    )
    _write_report(report, generated_rows)
    return {
        "schema": "wiz8.sgp-harness-run",
        "project_flags": list(PROJECT_FLAGS),
        "builds": [build.identifier for build in builds],
        "units": summaries,
        "report": str(report.relative_to(settings.repo_dir)),
    }
