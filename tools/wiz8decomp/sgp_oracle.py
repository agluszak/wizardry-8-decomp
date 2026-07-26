from __future__ import annotations

import csv
import hashlib
import io
import itertools
import re
import shutil
import struct
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pefile
import yaml

from .config import Settings
from .ghidra.fid_seeds import _docker_cmake_build, load_static_libraries
from .paths import atomic_write, sha256_file
from .subprocesses import run

RELOCATION_TYPES = frozenset({0x06, 0x07, 0x14})
CLASSIFICATION_RANK = {
    "unavailable": 0,
    "absent-or-stripped": 1,
    "ambiguous-generic": 2,
    "near-source-with-wiz8-modifications": 3,
    "relocation-equivalent": 4,
    "exact": 5,
}


@dataclass(frozen=True)
class CoffFunction:
    name: str
    body: bytes
    relocation_offsets: tuple[int, ...]

    @property
    def masked_body(self) -> bytes:
        return mask_relocations(self.body, self.relocation_offsets)


@dataclass(frozen=True)
class BuildText:
    identifier: str
    module_sha256: str
    base: int | None
    data: bytes | None
    unavailable_reason: str | None


def _coff_name(data: bytes, raw: bytes, string_table: int) -> str:
    if raw[:4] == b"\0\0\0\0":
        offset = struct.unpack_from("<I", raw, 4)[0]
        return data[string_table + offset :].split(b"\0", 1)[0].decode(
            "utf-8", errors="replace"
        )
    return raw[:8].rstrip(b"\0").decode("utf-8", errors="replace")


def _source_name(name: str) -> str:
    value = name.removeprefix("_")
    return re.sub(r"@\d+$", "", value)


def parse_coff_functions(path: Path) -> list[CoffFunction]:
    data = path.read_bytes()
    if len(data) < 20:
        raise RuntimeError(f"COFF object is too short: {path}")
    machine, section_count, _timestamp, symbol_table, symbol_count, optional_size, _flags = (
        struct.unpack_from("<HHIIIHH", data, 0)
    )
    if machine != 0x14C or optional_size != 0:
        raise RuntimeError(f"not an i386 COFF object: {path}")
    string_table = symbol_table + symbol_count * 18
    sections: dict[int, dict[str, Any]] = {}
    section_offset = 20
    for index in range(1, section_count + 1):
        header = data[section_offset : section_offset + 40]
        name = _coff_name(data, header[:8], string_table)
        size, raw_offset, relocation_offset, _lines, relocation_count, _line_count, _attrs = (
            struct.unpack_from("<IIIIHHI", header, 16)
        )
        sections[index] = {
            "name": name,
            "data": data[raw_offset : raw_offset + size] if raw_offset else b"",
            "relocations": [
                struct.unpack_from("<IIH", data, relocation_offset + position * 10)
                for position in range(relocation_count)
            ],
        }
        section_offset += 40

    symbols: list[tuple[str, int, int, int, int]] = []
    symbol_index = 0
    while symbol_index < symbol_count:
        raw = data[symbol_table + symbol_index * 18 : symbol_table + symbol_index * 18 + 18]
        name = _coff_name(data, raw, string_table)
        value, section, symbol_type, storage, auxiliary_count = struct.unpack_from(
            "<IhHBB", raw, 8
        )
        symbols.append((name, value, section, symbol_type, storage))
        symbol_index += 1 + auxiliary_count

    functions: list[CoffFunction] = []
    by_section: dict[int, list[tuple[str, int]]] = {}
    for name, value, section, symbol_type, storage in symbols:
        if (
            section > 0
            and storage == 2
            and symbol_type == 0x20
            and sections[section]["name"].startswith(".text")
        ):
            by_section.setdefault(section, []).append((name, value))
    for section_index, entries in sorted(by_section.items()):
        section = sections[section_index]
        entries.sort(key=lambda item: (item[1], item[0]))
        for position, (name, start) in enumerate(entries):
            end = entries[position + 1][1] if position + 1 < len(entries) else len(section["data"])
            body = section["data"][start:end].rstrip(b"\x90")
            if not body:
                continue
            offsets = tuple(
                sorted(
                    address - start
                    for address, _symbol, kind in section["relocations"]
                    if kind in RELOCATION_TYPES and start <= address <= end - 4
                )
            )
            functions.append(
                CoffFunction(
                    name=_source_name(name),
                    body=body,
                    relocation_offsets=offsets,
                )
            )
    if not functions:
        raise RuntimeError(f"COFF object exposes no external .text functions: {path}")
    return sorted(functions, key=lambda item: item.name.casefold())


def mask_relocations(body: bytes, offsets: tuple[int, ...]) -> bytes:
    masked = bytearray(body)
    for offset in offsets:
        masked[offset : offset + 4] = b"\0\0\0\0"
    return bytes(masked)


def _stable_ranges(length: int, offsets: tuple[int, ...]) -> list[tuple[int, int]]:
    holes = [False] * length
    for offset in offsets:
        for index in range(offset, min(offset + 4, length)):
            holes[index] = True
    ranges = []
    start = None
    for index, hole in enumerate([*holes, True]):
        if not hole and start is None:
            start = index
        elif hole and start is not None:
            ranges.append((start, index))
            start = None
    return ranges


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


def _load_config(settings: Settings) -> dict[str, Any]:
    path = settings.repo_dir / "config" / "analysis" / "sgp" / "harness.yml"
    config = yaml.safe_load(path.read_text(encoding="utf-8"))
    if config.get("schema") != "wiz8.sgp-harness":
        raise RuntimeError("invalid SGP harness schema")
    if not config.get("units") or not config.get("builds") or not config.get("flag_axes"):
        raise RuntimeError("SGP harness must configure units, builds, and flag axes")
    return config


def _load_builds(settings: Settings, config: dict[str, Any]) -> list[BuildText]:
    builds = []
    for record in config["builds"]:
        path = settings.work_dir / "variants" / record["variant"] / record["module"]
        if not path.is_file():
            builds.append(BuildText(record["id"], "", None, None, "module-missing"))
            continue
        module_hash = sha256_file(path)
        if record.get("static_matching") == "unavailable":
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


def _flag_combinations(config: dict[str, Any]) -> list[tuple[str, ...]]:
    axes = list(config["flag_axes"].values())
    return [tuple(items) for items in itertools.product(*axes)]


def _compile_units(
    settings: Settings,
    config: dict[str, Any],
    units: list[dict[str, Any]],
) -> dict[str, list[tuple[tuple[str, ...], list[CoffFunction]]]]:
    source = settings.work_dir / "oracles" / f"ja2-sgp-{config['source_revision'][:7]}" / "sgp"
    if not source.is_dir():
        raise RuntimeError("pinned SGP source is missing; run 'just _sgp-source'")
    head = run(
        ["git", "-C", source, "rev-parse", "HEAD"],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "sgp" / "source-revision.json",
    ).stdout.strip()
    if head != config["source_revision"]:
        raise RuntimeError(f"SGP source revision mismatch: expected {config['source_revision']}, got {head}")
    toolchains = {item.id: item for item in load_static_libraries(settings).toolchains}
    toolchain = toolchains.get(config["toolchain"])
    if toolchain is None or "compiler" not in toolchain.capabilities:
        raise RuntimeError(f"configured SGP toolchain is not compiler-capable: {config['toolchain']}")
    results = {unit["id"]: [] for unit in units}
    sweep_root = settings.work_dir / "sgp" / "sweeps"
    sweep_root.mkdir(parents=True, exist_ok=True)
    for index, flags in enumerate(_flag_combinations(config)):
        output = Path(tempfile.mkdtemp(prefix=f"flags-{index:02d}-", dir=sweep_root))
        try:
            definitions = {
                "WIZ8_BUILD_DECOMP": "OFF",
                "WIZ8_BUILD_FID_SEEDS": "OFF",
                "WIZ8_BUILD_SGP_PROBES": "ON",
                "SGP_SOURCE": "Z:/sources/sgp",
                "WIZ8_SGP_SWEEP_FLAGS": ";".join(flags),
            }
            for unit in units:
                _docker_cmake_build(
                    settings,
                    toolchain,
                    output=output,
                    target=unit["target"],
                    definitions=definitions,
                    source_mounts={"sgp": source},
                    log_name=f"sgp-{unit['id']}-flags-{index:02d}",
                )
                objects = sorted((output / "CMakeFiles" / f"{unit['target']}.dir").rglob("*.obj"))
                if len(objects) != 1:
                    raise RuntimeError(
                        f"{unit['target']} produced {len(objects)} objects; expected exactly one"
                    )
                results[unit["id"]].append((flags, parse_coff_functions(objects[0])))
        finally:
            shutil.rmtree(output, ignore_errors=True)
    return results


def _evaluate(
    variants: list[tuple[tuple[str, ...], list[CoffFunction]]],
    builds: list[BuildText],
    threshold: float,
) -> list[dict[str, Any]]:
    candidates: list[
        tuple[tuple[str, ...], list[tuple[CoffFunction, list[dict[str, Any]]]]]
    ] = []
    for flags, functions in variants:
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
        candidates.append((flags, evaluated))
    _best_index, (flags, evaluated) = max(
        enumerate(candidates),
        key=lambda indexed: (
            sum(
                CLASSIFICATION_RANK[match["classification"]]
                for _function, matches in indexed[1][1]
                for match in matches
            ),
            sum(
                match["classification"] in {"exact", "relocation-equivalent"}
                for _function, matches in indexed[1][1]
                for match in matches
            ),
            -indexed[0],
        ),
    )
    rows = []
    for function, matches in sorted(evaluated, key=lambda item: item[0].name.casefold()):
        for match in matches:
            build = match["build"]
            addresses = [build.base + position for position in match["positions"]] if build.base else []
            rows.append(
                {
                    "function": function.name,
                    "size": len(function.body),
                    "flags": " ".join(flags),
                    "build": build.identifier,
                    "module_sha256": build.module_sha256,
                    "classification": match["classification"],
                    "address": f"{addresses[0]:08x}" if len(addresses) == 1 else "",
                    "hit_count": len(addresses),
                    "similarity": f"{match['similarity']:.6f}",
                    "relocation_masked_sha256": hashlib.sha256(function.masked_body).hexdigest(),
                    "unavailable_reason": build.unavailable_reason or "",
                }
            )
    return rows


def _write_report(path: Path, rows: list[dict[str, Any]]) -> None:
    fields = [
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


def sweep_sgp_units(settings: Settings, unit_ids: list[str] | None = None) -> dict[str, Any]:
    config = _load_config(settings)
    by_id = {unit["id"]: unit for unit in config["units"]}
    unknown = sorted(set(unit_ids or ()) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown SGP unit(s): {', '.join(unknown)}")
    units = [by_id[item] for item in unit_ids] if unit_ids else list(config["units"])
    builds = _load_builds(settings, config)
    compiled = _compile_units(settings, config, units)
    summaries = []
    for unit in units:
        rows = _evaluate(compiled[unit["id"]], builds, float(config["near_match_threshold"]))
        report = settings.repo_dir / unit["report"]
        _write_report(report, rows)
        summaries.append(
            {
                "unit": unit["id"],
                "report": unit["report"],
                "functions": len({row["function"] for row in rows}),
                "rows": len(rows),
                "classifications": {
                    value: sum(row["classification"] == value for row in rows)
                    for value in CLASSIFICATION_RANK
                },
            }
        )
    return {
        "schema": "wiz8.sgp-harness-run",
        "flag_combinations": len(_flag_combinations(config)),
        "builds": [build.identifier for build in builds],
        "units": summaries,
    }
