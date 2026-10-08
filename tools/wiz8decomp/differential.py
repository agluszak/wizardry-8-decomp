"""Differential execution of exported SurRender APIs (vector processor, probes, surfaces).

The VC6 runner (tests/differential/sr_difftest.cpp) is staged once per
SR.DLL variant in its own directory, so Windows DLL search binds the runner to
that variant, and each variant runs in a separate Wine process. The first
variant is the reference (normally retail); every other variant is compared
with it line by line. Only "info" lines, which carry process-relative
addresses, are excluded from comparison.

A difference in any output word is a divergence: outputs are compared as exact
32-bit patterns, and float differences are additionally reported in ULPs. A
divergent elementwise case is re-run on the first divergent element alone
(`--window`) to produce a minimized reproducer.
"""

from __future__ import annotations

import json
import shutil
import struct
import subprocess
from collections.abc import Sequence
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .build import VC6_PRODUCT_IMAGE
from .subprocesses import resolve_executable

RUNNER_NAME = "sr_difftest.exe"
# The CRT the retail game ships; staged identically next to every variant so
# only SR.DLL differs between runs.
RUNTIME_DLLS = ("MSVCRT.DLL", "MSVCP60.DLL")
WINE_OVERRIDES = "msvcrt,msvcp60=n,b"
# Operations whose outputs are not one output element per input element; a
# first-element window does not reproduce their behavior.
NON_ELEMENTWISE = frozenset(
    {
        "sum",
        "minMaxF",
        "minMax3",
        "minMax4",
        "isZero",
        "isNeg",
        "isPos",
        "collectPos",
        "collectNeg",
        "collectNonZero",
        "cullNoClip",
        "remapInverse",
        "reverse",
        "maxDW",
        "minF",
        "isEqualK",
        "mulMatrix",
        "testBoundingBox",
    }
)


@dataclass(frozen=True)
class Variant:
    name: str
    dll: Path


@dataclass
class Trace:
    header: list[str] = field(default_factory=list)
    cases: dict[str, list[str]] = field(default_factory=dict)
    order: list[str] = field(default_factory=list)
    info: list[str] = field(default_factory=list)
    footer: list[str] = field(default_factory=list)
    returncode: int = 0
    stderr: str = ""


def parse_trace(text: str) -> Trace:
    trace = Trace()
    current: str | None = None
    for raw in text.splitlines():
        line = raw.rstrip("\r")
        if not line:
            continue
        if line.startswith("info "):
            trace.info.append(line)
            continue
        if line.startswith("case "):
            current = line[5:]
            trace.cases[current] = []
            trace.order.append(current)
            continue
        if current is not None:
            if line.startswith("end "):
                current = None
            else:
                trace.cases[current].append(line)
            continue
        (trace.footer if trace.order else trace.header).append(line)
    return trace


def stage(variant: Variant, runner: Path, runtime_dir: Path, output: Path) -> Path:
    directory = output / variant.name
    directory.mkdir(parents=True, exist_ok=True)
    shutil.copy2(runner, directory / RUNNER_NAME)
    shutil.copy2(variant.dll, directory / "sr.dll")
    for name in RUNTIME_DLLS:
        source = runtime_dir / name
        if source.is_file():
            shutil.copy2(source, directory / name)
    return directory


def run_variant(directory: Path, arguments: Sequence[str], *, timeout: int = 600) -> Trace:
    docker = resolve_executable("docker") or "docker"
    command = [
        docker,
        "run",
        "--rm",
        "--init",
        "--network",
        "none",
        "-e",
        f"WINEDLLOVERRIDES={WINE_OVERRIDES}",
        "--volume",
        f"{directory}:/variant:ro",
        "-w",
        "/variant",
        VC6_PRODUCT_IMAGE,
        RUNNER_NAME,
        *arguments,
    ]
    completed = subprocess.run(
        command, capture_output=True, text=True, timeout=timeout, check=False
    )
    trace = parse_trace(completed.stdout)
    trace.returncode = completed.returncode
    trace.stderr = "\n".join(
        line for line in completed.stderr.splitlines() if "XDG_RUNTIME_DIR" not in line
    )
    return trace


def _word(line: str) -> tuple[str, int | None, float | None]:
    parts = line.split()
    key = " ".join(parts[:2])
    if len(parts) >= 3 and parts[0] in {"in", "out"} and len(parts[2]) == 8:
        word = int(parts[2], 16)
        value = struct.unpack("<f", struct.pack("<I", word))[0] if len(parts) >= 4 else None
        return key, word, value
    return key, None, None


def _ulp_distance(left: int, right: int) -> int:
    def ordered(bits: int) -> int:
        return bits if bits < 0x80000000 else 0x80000000 - bits

    return abs(ordered(left) - ordered(right))


def first_divergence(reference: list[str], candidate: list[str]) -> dict[str, Any] | None:
    for index in range(max(len(reference), len(candidate))):
        left = reference[index] if index < len(reference) else "<missing>"
        right = candidate[index] if index < len(candidate) else "<missing>"
        if left == right:
            continue
        divergence: dict[str, Any] = {"line": index, "reference": left, "candidate": right}
        left_key, left_word, left_value = _word(left)
        right_key, right_word, right_value = _word(right)
        if (
            left_key == right_key
            and left_word is not None
            and right_word is not None
            and left_value is not None
            and right_value is not None
        ):
            divergence["ulps"] = _ulp_distance(left_word, right_word)
        return divergence
    return None


def _case_operation(lines: list[str]) -> str | None:
    for line in lines:
        if line.startswith("op "):
            return line.split()[1]
    return None


def _divergent_element(divergence: dict[str, Any]) -> int | None:
    key = divergence["reference"].split()
    if len(key) < 2 or key[0] != "out" or not key[1].startswith("d["):
        return None
    return int(key[1][2 : key[1].index("]")])


def _blob_lines(trace: Trace) -> list[str]:
    return [line for name in trace.order for line in trace.cases[name] if line.startswith("blob ")]


def cross_read(
    variants: Sequence[Variant],
    traces: Sequence[Trace],
    directories: Sequence[Path],
    cases: Sequence[str],
) -> dict[str, Any]:
    """Decode every variant's written stream blobs with every other variant.

    Each variant's ``blob`` lines go to ``blobs-<source>.txt`` in every variant
    directory. For each source, every variant runs the ``stream.xread.*`` cases
    on that file, and each candidate's decode is compared with the reference's
    decode of the same bytes. Retail-written bytes are thus read by the rebuilt
    DLL and the other way round.
    """
    empty: dict[str, Any] = {"cases": 0, "divergences": [[] for _ in variants[1:]]}
    selected = [case for case in cases if case.startswith(("stream.xread", "stream.*", "stream.x"))]
    if cases and not selected:
        return empty
    blobs = [_blob_lines(trace) for trace in traces]
    if not any(blobs):
        return empty
    for directory in directories:
        for variant, lines in zip(variants, blobs, strict=True):
            (directory / f"blobs-{variant.name}.txt").write_text(
                "\n".join(lines) + "\n", encoding="utf-8"
            )
    jobs = [(source, index) for source in range(len(variants)) for index in range(len(variants))]

    def run(job: tuple[int, int]) -> Trace:
        source, index = job
        arguments = ["--blobs", f"blobs-{variants[source].name}.txt", "stream.xread.*"]
        return run_variant(directories[index], arguments)

    with ThreadPoolExecutor(max_workers=min(6, len(jobs))) as pool:
        decoded = dict(zip(jobs, pool.map(run, jobs), strict=True))
    for (source, index), trace in decoded.items():
        (directories[index] / f"xread-{variants[source].name}.txt").write_text(
            "\n".join(f"case {name}\n" + "\n".join(trace.cases[name]) for name in trace.order)
            + "\n",
            encoding="utf-8",
        )
    divergences: list[list[dict[str, Any]]] = [[] for _ in variants[1:]]
    count = 0
    for source, variant in enumerate(variants):
        reference = decoded[(source, 0)]
        count += len(reference.order)
        for index in range(1, len(variants)):
            trace = decoded[(source, index)]
            for name in reference.order:
                divergence = first_divergence(
                    reference.cases[name], trace.cases.get(name, ["<missing case>"])
                )
                if divergence is not None:
                    divergences[index - 1].append(
                        {"case": f"{name}@{variant.name}-written", **divergence}
                    )
    return {"cases": count, "divergences": divergences}


def compare(
    variants: Sequence[Variant],
    runner: Path,
    runtime_dir: Path,
    output: Path,
    *,
    seed: int = 1,
    generated: int = 2,
    cases: Sequence[str] = (),
    minimize: bool = True,
) -> dict[str, Any]:
    if len(variants) < 2:
        raise ValueError("differential comparison needs a reference and at least one candidate")
    directories = [stage(variant, runner, runtime_dir, output) for variant in variants]
    arguments = ["--seed", str(seed), "--generated", str(generated), *cases]
    with ThreadPoolExecutor(max_workers=len(variants)) as pool:
        traces = list(pool.map(lambda directory: run_variant(directory, arguments), directories))
    for variant, trace, directory in zip(variants, traces, directories, strict=True):
        (directory / "trace.txt").write_text(
            "\n".join(
                [
                    *trace.header,
                    *(f"case {name}\n" + "\n".join(trace.cases[name]) for name in trace.order),
                    *trace.footer,
                ]
            )
            + "\n",
            encoding="utf-8",
        )
    if traces[0].returncode != 0 or "done" not in traces[0].footer:
        raise RuntimeError(
            f"{variants[0].name} reference runner did not complete "
            f"(exit {traces[0].returncode}): {traces[0].stderr[-2000:]}"
        )

    reference = traces[0]
    results: list[dict[str, Any]] = []
    pending: list[tuple[dict[str, Any], Path, str, int]] = []
    for variant, trace, directory in zip(variants[1:], traces[1:], directories[1:], strict=True):
        header = first_divergence(reference.header, trace.header)
        divergent: list[dict[str, Any]] = []
        missing = [name for name in reference.order if name not in trace.cases]
        for name in reference.order:
            if name not in trace.cases:
                continue
            divergence = first_divergence(reference.cases[name], trace.cases[name])
            if divergence is None:
                continue
            entry: dict[str, Any] = {"case": name, **divergence}
            operation = _case_operation(reference.cases[name])
            element = _divergent_element(divergence)
            if (
                minimize
                and operation is not None
                and operation not in NON_ELEMENTWISE
                and element is not None
            ):
                pending.append((entry, directory, name, element))
            divergent.append(entry)
        results.append(
            {
                "candidate": variant.name,
                "dll": str(variant.dll),
                "completed": trace.returncode == 0 and "done" in trace.footer,
                "exit": trace.returncode,
                "header_divergence": header,
                "cases": len(reference.order),
                "divergent": len(divergent),
                "missing": missing,
                "divergences": divergent,
            }
        )

    def reduce(item: tuple[dict[str, Any], Path, str, int]) -> None:
        entry, directory, name, element = item
        window = ["--seed", str(seed), "--window", str(element), "1", name]
        left = run_variant(directories[0], window)
        right = run_variant(directory, window)
        if left.order and right.order:
            left_lines = left.cases[left.order[0]]
            right_lines = right.cases[right.order[0]]
            entry["minimized"] = {
                "case": left.order[0],
                "reproduces": first_divergence(left_lines, right_lines) is not None,
                "inputs": [line for line in left_lines if line.startswith(("op ", "in "))],
                "reference": [line for line in left_lines if line.startswith("out ")],
                "candidate": [line for line in right_lines if line.startswith("out ")],
            }

    cross = cross_read(variants, traces, directories, cases)
    for result, entries in zip(results, cross["divergences"], strict=True):
        result["cases"] += cross["cases"]
        result["divergent"] += len(entries)
        result["divergences"].extend(entries)

    # Each reduction is two short container runs; bound the parallelism.
    with ThreadPoolExecutor(max_workers=6) as pool:
        list(pool.map(reduce, pending))
    report = {
        "reference": {"name": variants[0].name, "dll": str(variants[0].dll)},
        "seed": seed,
        "generated_rounds": generated,
        "cases": len(reference.order),
        "results": results,
    }
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def render(report: dict[str, Any], *, limit: int = 20) -> str:
    lines = [
        f"reference {report['reference']['name']}: {report['reference']['dll']}",
        f"cases {report['cases']} (seed {report['seed']}, {report['generated_rounds']} generated rounds)",
    ]
    for result in report["results"]:
        lines.append("")
        lines.append(
            f"{result['candidate']}: {result['divergent']} divergent / {result['cases']} cases"
            + (f", {len(result['missing'])} missing" if result["missing"] else "")
            + (
                ""
                if result.get("completed", True)
                else f", runner incomplete (exit {result.get('exit')})"
            )
        )
        families: dict[str, int] = {}
        for entry in result["divergences"]:
            family = ".".join(entry["case"].split(".")[:2])
            families[family] = families.get(family, 0) + 1
        if families:
            lines.append(
                "  by family: "
                + ", ".join(f"{name} {count}" for name, count in sorted(families.items()))
            )
        if result["header_divergence"]:
            header = result["header_divergence"]
            lines.append(f"  header: {header['reference']!r} vs {header['candidate']!r}")
        for entry in result["divergences"][:limit]:
            ulps = f" ({entry['ulps']} ulp)" if "ulps" in entry else ""
            lines.append(f"  {entry['case']}")
            lines.append(f"    reference: {entry['reference']}")
            lines.append(f"    candidate: {entry['candidate']}{ulps}")
            minimized = entry.get("minimized")
            if minimized and minimized["reproduces"]:
                lines.append(f"    minimized: {minimized['case']}")
                lines.extend(f"      {line}" for line in minimized["inputs"])
                for left, right in zip(
                    minimized["reference"], minimized["candidate"], strict=False
                ):
                    marker = "  " if left == right else "!="
                    lines.append(f"      {marker} ref {left} | cand {right}")
        if len(result["divergences"]) > limit:
            lines.append(f"  ... {len(result['divergences']) - limit} more in report.json")
    return "\n".join(lines)
