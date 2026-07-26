from __future__ import annotations

import csv
import io
import json
import re
import shutil
import struct
import tempfile
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from .binary.pe import inspect_pe, is_pe
from .config import Settings
from .extract.variants import EXTRACTED_NAMES
from .inputs.scan import scan_inputs
from .paths import atomic_write, sha256_file
from .subprocesses import run

ARTIFACT_EXTENSIONS = {".pdb", ".obj", ".lib", ".cpp", ".c", ".h", ".dsp", ".dsw", ".map"}
_PATH_EXTENSIONS = r"pdb|obj|lib|cpp|c|h|dsp|dsw|map"
_ABSOLUTE_PATH_RE = re.compile(
    rf"(?i)(?:[a-z]:[\\/]|\\\\)[^\x00\r\n<>\"|?*]{{1,320}}?\.(?:{_PATH_EXTENSIONS})\b"
)
_RELATIVE_PATH_RE = re.compile(
    rf"(?i)(?<![a-z0-9_.])"
    rf"(?:(?:\.\.?|(?:(?!\.(?:{_PATH_EXTENSIONS})\b)[a-z0-9_.() -])+)[\\/])+"
    rf"[a-z0-9_.$() +@'-]+\.(?:{_PATH_EXTENSIONS})\b"
)
_BARE_STRONG_PATH_RE = re.compile(
    r"(?i)(?<![a-z0-9_.])"
    r"[a-z_][a-z0-9_.$@+-]{1,100}\.(?:pdb|obj|lib|cpp|cc|cxx|hpp|dsp|dsw|map)\b"
)
_COMPILER_VERSION_RE = re.compile(r"(?i)(?:Intel\(R\)|Microsoft).*?Compiler Version[^\r\n]{0,3900}")
_ASCII_RUN_RE = re.compile(rb"[\x20-\x7e]{4,}")
_UTF16_RUN_RE = re.compile(rb"(?:[\x20-\x7e]\x00){4,}")
_BINARY_EXTENSIONS = {".exe", ".dll"}
_REPORT_FILES = ("binaries.csv", "binary-records.csv", "containers.csv", "container-members.csv")


def extract_debug_records(strings: Iterable[tuple[str, str]]) -> list[dict[str, str]]:
    """Extract deliberate path-shaped records, excluding random bare .c/.h byte runs."""

    records: dict[tuple[str, str, str], dict[str, str]] = {}
    for encoding, value in strings:
        if len(value) > 4096:
            continue
        for pattern in (_ABSOLUTE_PATH_RE, _RELATIVE_PATH_RE, _BARE_STRONG_PATH_RE):
            for match in pattern.finditer(value):
                path = match.group(0).strip(" ,:;()[]{}")
                extension = Path(path.replace("\\", "/")).suffix.casefold()
                if extension not in ARTIFACT_EXTENSIONS and extension not in {
                    ".cc",
                    ".cxx",
                    ".hpp",
                }:
                    continue
                key = ("path", encoding, path.casefold())
                records[key] = {
                    "record_kind": "path",
                    "origin": "embedded-string",
                    "encoding": encoding,
                    "extension": extension,
                    "value": path,
                }
        for match in _COMPILER_VERSION_RE.finditer(value):
            command = match.group(0).strip()
            key = ("compiler-command", encoding, command.casefold())
            records[key] = {
                "record_kind": "compiler-command",
                "origin": "embedded-string",
                "encoding": encoding,
                "extension": "",
                "value": command,
            }
    values = list(records.values())
    path_values = [row for row in values if row["record_kind"] == "path"]

    def is_subsumed(row: dict[str, str]) -> bool:
        value = row["value"].replace("/", "\\").casefold()
        if "\\" in value:
            boundary = "\\" + value
        else:
            boundary = rf"(?:[\\/\s]){re.escape(value)}$"
        return any(
            other["encoding"] == row["encoding"]
            and other["value"] != row["value"]
            and (
                other["value"].replace("/", "\\").casefold().endswith(boundary)
                if "\\" in value
                else re.search(boundary, other["value"].replace("/", "\\").casefold()) is not None
            )
            for other in path_values
        )

    values = [row for row in values if row["record_kind"] != "path" or not is_subsumed(row)]
    return sorted(values, key=lambda row: (row["record_kind"], row["value"].casefold()))


def infer_build_configuration(records: list[dict[str, str]]) -> tuple[str, str]:
    values = [row["value"] for row in records]
    lowered = [value.casefold() for value in values]
    evidence: list[str] = []
    optimized = any(
        re.search(r"(?i)(?:^|\s)[-/](?:o[12x]|ob[012])(?:\s|$)", value) for value in values
    )
    unoptimized = any(re.search(r"(?i)(?:^|\s)[-/]od(?:\s|$)", value) for value in values)
    debug_info = any(re.search(r"(?i)(?:^|\s)[-/]z(?:i|7)(?:\s|$)", value) for value in values)
    release_path = any(re.search(r"(?i)(?:^|[\\/])release(?:[\\/]|$)", value) for value in values)
    debug_path = any(re.search(r"(?i)(?:^|[\\/])debug(?:[\\/]|$)", value) for value in values)
    ndebug = any("ndebug" in value for value in lowered)
    if optimized:
        evidence.append("compiler command contains an optimization flag")
    if unoptimized:
        evidence.append("compiler command contains /Od or -Od")
    if debug_info:
        evidence.append("compiler command contains /Zi, -Zi, /Z7, or -Z7")
    if release_path:
        evidence.append("artifact path contains a Release component")
    if debug_path:
        evidence.append("artifact path contains a Debug component")
    if ndebug:
        evidence.append("compiler command defines NDEBUG")

    if (release_path or ndebug) and (debug_path or unoptimized):
        configuration = "mixed"
    elif optimized and debug_info:
        configuration = "optimized-with-debug-info"
    elif release_path or ndebug:
        configuration = "release"
    elif debug_path or unoptimized:
        configuration = "debug"
    elif optimized:
        configuration = "optimized"
    else:
        configuration = "unknown"
    return configuration, "; ".join(evidence)


def _pe_scan_ranges(path: Path) -> list[tuple[int, int]]:
    """Return PE headers and mapped section bytes, excluding installer/patch overlays."""

    try:
        size = path.stat().st_size
        with path.open("rb") as stream:
            header = stream.read(0x40)
            if header[:2] != b"MZ" or len(header) < 0x40:
                return []
            pe_offset = struct.unpack_from("<I", header, 0x3C)[0]
            stream.seek(pe_offset)
            pe_header = stream.read(24)
            if pe_header[:4] != b"PE\0\0" or len(pe_header) != 24:
                return []
            section_count = struct.unpack_from("<H", pe_header, 6)[0]
            optional_size = struct.unpack_from("<H", pe_header, 20)[0]
            section_table = pe_offset + 24 + optional_size
            stream.seek(section_table)
            section_headers = stream.read(section_count * 40)
        ranges = [(0, min(size, section_table + len(section_headers)))]
        for index in range(section_count):
            section = section_headers[index * 40 : (index + 1) * 40]
            if len(section) != 40:
                break
            raw_size, raw_offset = struct.unpack_from("<II", section, 16)
            if raw_size and raw_offset < size:
                ranges.append((raw_offset, min(raw_size, size - raw_offset)))
        merged: list[tuple[int, int]] = []
        for offset, length in sorted(ranges):
            end = offset + length
            if merged and offset <= merged[-1][0] + merged[-1][1]:
                previous_offset, previous_length = merged[-1]
                merged[-1] = (
                    previous_offset,
                    max(previous_offset + previous_length, end) - previous_offset,
                )
            else:
                merged.append((offset, length))
        return merged
    except (OSError, struct.error):
        return []


def _printable_strings(path: Path, *, mapped_only: bool) -> Iterable[tuple[str, str]]:
    ranges = (
        (_pe_scan_ranges(path) or [(0, path.stat().st_size)])
        if mapped_only
        else [(0, path.stat().st_size)]
    )
    with path.open("rb") as stream:
        for offset, length in ranges:
            overlap = b""
            stream.seek(offset)
            remaining = length
            while remaining:
                chunk = stream.read(min(1024 * 1024, remaining))
                if not chunk:
                    break
                remaining -= len(chunk)
                data = overlap + chunk
                for match in _ASCII_RUN_RE.finditer(data):
                    yield "ascii", match.group(0).decode("ascii")
                for match in _UTF16_RUN_RE.finditer(data):
                    yield "utf-16le", match.group(0).decode("utf-16le")
                overlap = data[-4096:]


def _pe_linker_version(path: Path) -> str:
    try:
        with path.open("rb") as stream:
            header = stream.read(0x40)
            if header[:2] != b"MZ" or len(header) < 0x40:
                return ""
            offset = int.from_bytes(header[0x3C:0x40], "little")
            stream.seek(offset)
            pe_header = stream.read(0x1C)
        if pe_header[:4] != b"PE\0\0" or len(pe_header) < 0x1C:
            return ""
        return f"{pe_header[0x1A]}.{pe_header[0x1B]}"
    except OSError:
        return ""


def _infer_toolchain(records: list[dict[str, str]], pe_hypothesis: dict[str, Any] | None) -> str:
    for record in records:
        match = re.search(r"(?i)Intel\(R\).*?Compiler Version\s+([0-9.]+)", record["value"])
        if match:
            return f"Intel C/C++ Compiler {match.group(1)}"
    if pe_hypothesis and pe_hypothesis["family"] != "unknown":
        return f"{pe_hypothesis['family']} ({pe_hypothesis['confidence']} PE evidence)"
    return "unknown"


def _scan_binary(
    path: Path, sha256: str, size: int, *, mapped_only: bool
) -> tuple[dict[str, str], list[dict[str, str]]]:
    records = extract_debug_records(_printable_strings(path, mapped_only=mapped_only))
    linker_version = _pe_linker_version(path)
    pe_hypothesis: dict[str, Any] | None = None
    if size <= 128 * 1024 * 1024 and is_pe(path):
        metadata = inspect_pe(path, "debug-artifact-sweep", path.name)
        linker_version = metadata["linker_version"]
        pe_hypothesis = metadata["compiler_hypothesis"]
        pdbs = {value.casefold() for value in metadata["pdb_paths"]}
        for record in records:
            if record["record_kind"] == "path" and record["value"].casefold() in pdbs:
                record["origin"] = "codeview"
        present_paths = {record["value"].casefold() for record in records}
        records.extend(
            {
                "record_kind": "path",
                "origin": "codeview",
                "encoding": "ascii",
                "extension": ".pdb",
                "value": pdb,
            }
            for pdb in metadata["pdb_paths"]
            if pdb.casefold() not in present_paths
        )
        records.sort(key=lambda row: (row["record_kind"], row["value"].casefold()))
    configuration, inference_evidence = infer_build_configuration(records)
    return (
        {
            "sha256": sha256,
            "size": str(size),
            "scan_status": "scanned",
            "linker_version": linker_version,
            "toolchain_inference": _infer_toolchain(records, pe_hypothesis),
            "configuration_inference": configuration,
            "inference_evidence": inference_evidence,
            "record_count": str(len(records)),
        },
        records,
    )


def _parse_7z_slt(output: str) -> list[dict[str, str]]:
    _, separator, listing = output.partition("----------\n")
    if not separator:
        raise RuntimeError("7z did not return a technical member listing")
    members: list[dict[str, str]] = []
    for block in re.split(r"\n\s*\n", listing.strip()):
        fields: dict[str, str] = {}
        for line in block.splitlines():
            if " = " in line:
                key, value = line.split(" = ", 1)
                fields[key] = value
        if fields.get("Path") and fields.get("Folder") != "+":
            members.append(fields)
    return members


def _csv_text(fields: list[str], rows: list[dict[str, str]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def _snapshot_readme() -> str:
    return """# Debug-artifact corpus snapshot

These CSVs are the reviewed output of a complete sweep of every EXE/DLL available in the
configured Wizardry input containers and every container member name. They are tracked because
reproduction requires proprietary, untracked inputs. Container and binary SHA-256 identities are
recorded in the CSVs; any unavailable binary remains an explicit row rather than being omitted.

The producer is `wiz8decomp.debug_artifacts`. Normal runs write the same four CSVs under
`build/reports/debug-artifacts/` and fail when they differ from this snapshot. Refresh only after
reviewing a complete corpus sweep:

```sh
uv run wiz8 debug-artifacts --archive-password '<local-password>' --update-snapshot
```

Encrypted RAR inputs require `unrar` or `unrar-nonfree` on `PATH`. The password is used only for
the temporary extraction and is never written to these reports.

`container-members.csv` records only member names with the requested debug/source/project
extensions. `containers.csv` always has one row per input, including containers with zero hits.
No executable bytes or extracted game data are tracked here.
"""


def sweep_debug_artifacts(
    settings: Settings,
    *,
    update_snapshot: bool = False,
    archive_password: str | None = None,
) -> dict[str, Any]:
    manifest = scan_inputs(settings)
    container_rows: list[dict[str, str]] = []
    member_rows: list[dict[str, str]] = []
    binary_rows: list[dict[str, str]] = []
    record_rows: list[dict[str, str]] = []
    scan_cache: dict[tuple[str, bool], tuple[dict[str, str], list[dict[str, str]]]] = {}
    temporary_extractions: list[tempfile.TemporaryDirectory[str]] = []

    def add_binary(
        container: str,
        role: str,
        source_kind: str,
        member_path: str,
        path: Path | None,
        sha256: str,
        size: int,
        unavailable_reason: str = "",
    ) -> None:
        if unavailable_reason:
            metadata = {
                "sha256": sha256,
                "size": str(size),
                "scan_status": unavailable_reason,
                "linker_version": "",
                "toolchain_inference": "unknown",
                "configuration_inference": "unknown",
                "inference_evidence": "",
                "record_count": "0",
            }
            records: list[dict[str, str]] = []
        else:
            if path is None or not path.is_file():
                raise RuntimeError(f"binary payload is missing: {container}/{member_path}")
            mapped_only = source_kind == "input-container"
            cache_key = (sha256, mapped_only)
            if cache_key not in scan_cache:
                scan_cache[cache_key] = _scan_binary(path, sha256, size, mapped_only=mapped_only)
            metadata, records = scan_cache[cache_key]
        binary_rows.append(
            {
                "container": container,
                "role": role,
                "source_kind": source_kind,
                "member_path": member_path,
                **metadata,
            }
        )
        record_rows.extend(
            {
                "container": container,
                "binary_path": member_path,
                "binary_sha256": sha256,
                **record,
            }
            for record in records
        )

    for input_record in manifest.files:
        container = input_record.relative_path
        role = input_record.configured_role or "unassigned"
        source = settings.input_dir / container
        members: list[dict[str, str]]
        listing_method: str
        if input_record.configured_role:
            extracted_name = EXTRACTED_NAMES.get(input_record.configured_role)
            if not extracted_name:
                raise RuntimeError(f"no extraction mapping for configured role: {role}")
            extracted = settings.work_dir / "extracted" / extracted_name
            receipt_path = extracted / ".wiz8-extraction.json"
            if not receipt_path.is_file():
                raise RuntimeError(f"missing extraction receipt for {role}: {receipt_path}")
            receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
            if input_record.sha256 not in receipt.get("input_hashes", []):
                raise RuntimeError(f"extraction receipt is not tied to the current {role} input")
            members = [
                {
                    "Path": item["path"],
                    "Size": str(item["size"]),
                    "SHA256": item["sha256"],
                    "Encrypted": "-",
                }
                for item in receipt["files"]
            ]
            listing_method = "verified-extraction-receipt"
        else:
            result = run(["7z", "l", "-slt", source], cwd=source.parent)
            members = _parse_7z_slt(result.stdout)
            encrypted_members = [member for member in members if member.get("Encrypted") == "+"]
            if encrypted_members:
                if not archive_password:
                    raise RuntimeError(
                        f"{container} has encrypted members; pass --archive-password to complete "
                        "the binary sweep"
                    )
                temporary = tempfile.TemporaryDirectory(
                    prefix="wiz8-debug-artifacts-", dir=settings.work_dir
                )
                temporary_extractions.append(temporary)
                extracted = Path(temporary.name)
                if input_record.container_technology == "RAR":
                    extractor = shutil.which("unrar") or shutil.which("unrar-nonfree")
                    if not extractor:
                        raise RuntimeError(
                            f"{container} uses encrypted RAR data unsupported by 7z; install "
                            "unrar or put it on PATH"
                        )
                    run(
                        [extractor, "x", f"-p{archive_password}", "-y", source, extracted],
                        cwd=source.parent,
                    )
                    listing_method = "unrar-password-assisted-temporary-extraction"
                else:
                    run(
                        ["7z", "x", "-y", f"-p{archive_password}", f"-o{extracted}", source],
                        cwd=source.parent,
                    )
                    listing_method = "7z-password-assisted-temporary-extraction"
            else:
                extracted = None
                listing_method = "7z-technical-listing"

        suspicious = [
            member
            for member in members
            if Path(member["Path"]).suffix.casefold() in ARTIFACT_EXTENSIONS
        ]
        for member in suspicious:
            member_rows.append(
                {
                    "container": container,
                    "role": role,
                    "member_path": member["Path"],
                    "extension": Path(member["Path"]).suffix.casefold(),
                    "size": member.get("Size", ""),
                    "encrypted": "yes" if member.get("Encrypted") == "+" else "no",
                }
            )
        container_rows.append(
            {
                "container": container,
                "role": role,
                "size": str(input_record.size),
                "sha256": input_record.sha256,
                "detected_type": input_record.detected_type,
                "container_technology": input_record.container_technology or "",
                "installer_technology": input_record.installer_technology or "",
                "listing_method": listing_method,
                "member_count": str(len(members)),
                "artifact_member_count": str(len(suspicious)),
            }
        )

        if source.suffix.casefold() in _BINARY_EXTENSIONS:
            add_binary(
                container,
                role,
                "input-container",
                "<container>",
                source,
                input_record.sha256,
                input_record.size,
            )
        for member in members:
            if Path(member["Path"]).suffix.casefold() not in _BINARY_EXTENSIONS:
                continue
            member_path = member["Path"]
            add_binary(
                container,
                role,
                "container-member",
                member_path,
                None if extracted is None else extracted / member_path,
                member.get("SHA256", "")
                or (
                    sha256_file(extracted / member_path)
                    if extracted is not None and (extracted / member_path).is_file()
                    else ""
                ),
                int(member.get("Size", "0") or 0),
                "unavailable-not-extracted" if extracted is None else "",
            )

    container_rows.sort(key=lambda row: row["container"].casefold())
    member_rows.sort(key=lambda row: (row["container"].casefold(), row["member_path"].casefold()))
    binary_rows.sort(key=lambda row: (row["container"].casefold(), row["member_path"].casefold()))
    record_rows.sort(
        key=lambda row: (
            row["container"].casefold(),
            row["binary_path"].casefold(),
            row["record_kind"],
            row["value"].casefold(),
        )
    )
    outputs = {
        "containers.csv": _csv_text(
            [
                "container",
                "role",
                "size",
                "sha256",
                "detected_type",
                "container_technology",
                "installer_technology",
                "listing_method",
                "member_count",
                "artifact_member_count",
            ],
            container_rows,
        ),
        "container-members.csv": _csv_text(
            ["container", "role", "member_path", "extension", "size", "encrypted"], member_rows
        ),
        "binaries.csv": _csv_text(
            [
                "container",
                "role",
                "source_kind",
                "member_path",
                "sha256",
                "size",
                "scan_status",
                "linker_version",
                "toolchain_inference",
                "configuration_inference",
                "inference_evidence",
                "record_count",
            ],
            binary_rows,
        ),
        "binary-records.csv": _csv_text(
            [
                "container",
                "binary_path",
                "binary_sha256",
                "record_kind",
                "origin",
                "encoding",
                "extension",
                "value",
            ],
            record_rows,
        ),
    }
    report_dir = settings.build_dir / "reports" / "debug-artifacts"
    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / "debug-artifacts"
    for name, value in outputs.items():
        atomic_write(report_dir / name, value)
    if update_snapshot:
        for name, value in outputs.items():
            atomic_write(snapshot_dir / name, value)
        atomic_write(snapshot_dir / "README.md", _snapshot_readme())
    snapshot_fresh = all(
        (snapshot_dir / name).is_file()
        and (snapshot_dir / name).read_text(encoding="utf-8") == outputs[name]
        for name in _REPORT_FILES
    )
    if not update_snapshot and not snapshot_fresh:
        raise RuntimeError(
            "debug-artifact report differs from the tracked snapshot; review build/reports/"
            "debug-artifacts and rerun with --update-snapshot"
        )
    for temporary in temporary_extractions:
        temporary.cleanup()
    return {
        "schema": "wiz8.debug-artifact-sweep",
        "containers": len(container_rows),
        "artifact_container_members": len(member_rows),
        "binaries": len(binary_rows),
        "scanned_binaries": sum(row["scan_status"] == "scanned" for row in binary_rows),
        "unavailable_binaries": sum(row["scan_status"] != "scanned" for row in binary_rows),
        "records": len(record_rows),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
