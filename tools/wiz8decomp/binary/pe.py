from __future__ import annotations

import datetime as dt
import hashlib
import re
import struct
from pathlib import Path
from typing import Any

import pefile

from ..paths import sha256_file
from .fingerprints import bytes_entropy, metadata_normalized_pe_hash
from .rich_header import parse_rich_header


MACHINES = {0x14C: "x86", 0x8664: "x86-64", 0x1C0: "ARM", 0xAA64: "ARM64"}
SUBSYSTEMS = {
    1: "native",
    2: "windows-gui",
    3: "windows-console",
    9: "windows-ce-gui",
    10: "efi-application",
}
DEBUG_TYPES = {1: "COFF", 2: "CodeView", 3: "FPO", 4: "MISC", 12: "VC_FEATURE", 16: "REPRO"}
SOURCE_RE = re.compile(rb"[A-Za-z]:\\[^\x00\r\n]{3,240}\.(?:c|cc|cpp|cxx|h|hpp|pdb|obj|lib)", re.I)
REL_SOURCE_RE = re.compile(rb"(?:[A-Za-z0-9_.-]+[\\/]){1,8}[A-Za-z0-9_.-]+\.(?:c|cc|cpp|cxx|h|hpp)", re.I)
ASSERT_RE = re.compile(rb"[^\x00\r\n]{0,120}(?:assert(?:ion)?|failed)[^\x00\r\n]{0,160}", re.I)


def is_pe(path: Path) -> bool:
    try:
        with path.open("rb") as stream:
            head = stream.read(0x100)
        if head[:2] != b"MZ" or len(head) < 0x40:
            return False
        offset = struct.unpack_from("<I", head, 0x3C)[0]
        with path.open("rb") as stream:
            stream.seek(offset)
            return stream.read(4) == b"PE\0\0"
    except OSError:
        return False


def _decode(value: bytes | str | None) -> str:
    if value is None:
        return ""
    if isinstance(value, str):
        return value
    return value.decode("utf-8", errors="replace")


def _version_info(pe: pefile.PE) -> dict[str, str]:
    result: dict[str, str] = {}
    for group in getattr(pe, "FileInfo", []) or []:
        for entry in group:
            if getattr(entry, "Key", b"") == b"StringFileInfo":
                for table in entry.StringTable:
                    for key, value in table.entries.items():
                        result[_decode(key)] = _decode(value).strip()
    fixed = getattr(pe, "VS_FIXEDFILEINFO", []) or []
    if fixed:
        info = fixed[0]
        result.setdefault("FixedFileVersion", f"{info.FileVersionMS >> 16}.{info.FileVersionMS & 0xFFFF}.{info.FileVersionLS >> 16}.{info.FileVersionLS & 0xFFFF}")
        result.setdefault("FixedProductVersion", f"{info.ProductVersionMS >> 16}.{info.ProductVersionMS & 0xFFFF}.{info.ProductVersionLS >> 16}.{info.ProductVersionLS & 0xFFFF}")
    return dict(sorted(result.items()))


def _imports(directory: Any) -> list[dict[str, Any]]:
    result = []
    for module in directory or []:
        symbols = []
        for item in module.imports:
            symbols.append({"name": _decode(item.name) or None, "ordinal": item.ordinal, "address": f"0x{item.address:x}"})
        result.append({"module": _decode(module.dll), "symbols": symbols})
    return sorted(result, key=lambda item: item["module"].casefold())


def _exports(pe: pefile.PE) -> list[dict[str, Any]]:
    directory = getattr(pe, "DIRECTORY_ENTRY_EXPORT", None)
    if not directory:
        return []
    return [
        {"name": _decode(symbol.name) or None, "ordinal": symbol.ordinal, "rva": f"0x{symbol.address:x}"}
        for symbol in sorted(directory.symbols, key=lambda value: (value.ordinal, _decode(value.name)))
    ]


def _debug(pe: pefile.PE, data: bytes) -> tuple[list[dict[str, Any]], list[str]]:
    records = []
    pdbs = []
    for entry in getattr(pe, "DIRECTORY_ENTRY_DEBUG", []) or []:
        item = {
            "type": DEBUG_TYPES.get(entry.struct.Type, str(entry.struct.Type)),
            "timestamp": entry.struct.TimeDateStamp,
            "size": entry.struct.SizeOfData,
            "rva": f"0x{entry.struct.AddressOfRawData:x}",
        }
        offset = entry.struct.PointerToRawData
        blob = data[offset : offset + entry.struct.SizeOfData]
        if blob.startswith(b"RSDS") and len(blob) >= 24:
            path = blob[24:].split(b"\0", 1)[0].decode("utf-8", errors="replace")
            item["pdb_path"] = path
            pdbs.append(path)
        elif blob.startswith(b"NB10") and len(blob) >= 16:
            path = blob[16:].split(b"\0", 1)[0].decode("utf-8", errors="replace")
            item["pdb_path"] = path
            pdbs.append(path)
        records.append(item)
    return records, sorted(set(pdbs), key=str.casefold)


def _tls_callbacks(pe: pefile.PE) -> list[str]:
    tls = getattr(pe, "DIRECTORY_ENTRY_TLS", None)
    if not tls or not tls.struct.AddressOfCallBacks:
        return []
    address = tls.struct.AddressOfCallBacks
    rva = address - pe.OPTIONAL_HEADER.ImageBase
    callbacks = []
    width = 8 if pe.PE_TYPE == pefile.OPTIONAL_HEADER_MAGIC_PE_PLUS else 4
    for index in range(256):
        raw = pe.get_data(rva + index * width, width)
        if len(raw) != width:
            break
        value = int.from_bytes(raw, "little")
        if not value:
            break
        callbacks.append(f"0x{value:x}")
    return callbacks


def _compiler_hypothesis(rich: dict[str, Any] | None, imports: list[dict[str, Any]], sections: list[dict[str, Any]], linker_version: str) -> dict[str, Any]:
    evidence: list[str] = []
    family = "unknown"
    confidence = "low"
    if rich and rich.get("valid"):
        products = sorted({f"{item['product_id']}:{item['build']}" for item in rich["records"]})
        evidence.append("valid Microsoft Rich header product/build records: " + ", ".join(products))
        family, confidence = "Microsoft Visual C++ / LINK", "strong"
    imported = {item["module"].casefold() for item in imports}
    if any(name.startswith("msvc") for name in imported):
        runtime_names = sorted(name for name in imported if name.startswith("msvc"))
        evidence.append("imports Microsoft Visual C++ runtime DLLs: " + ", ".join(runtime_names))
        family, confidence = "Microsoft Visual C++", "strong"
    if linker_version.startswith("6."):
        evidence.append(f"PE optional header reports LINK {linker_version}")
        if any(name in imported for name in {"msvcp60.dll", "msvcirt.dll"}):
            family, confidence = "Microsoft Visual C++ 6.x-era toolchain", "strong"
    if any(section["name"] == ".rdata" for section in sections):
        evidence.append("MSVC-compatible .rdata layout (weak evidence alone)")
    return {"family": family, "confidence": confidence, "evidence": evidence}


def inspect_pe(path: Path, variant: str, relative_path: str) -> dict[str, Any]:
    data = path.read_bytes()
    pe = pefile.PE(data=data, fast_load=False)
    pe.parse_data_directories()
    sections = []
    for section in pe.sections:
        name = section.Name.rstrip(b"\0").decode("ascii", errors="replace")
        blob = section.get_data()
        characteristics = section.Characteristics
        sections.append({
            "name": name,
            "rva": f"0x{section.VirtualAddress:x}",
            "virtual_address": pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress,
            "virtual_size": section.Misc_VirtualSize,
            "raw_offset": section.PointerToRawData,
            "raw_size": section.SizeOfRawData,
            "permissions": "".join(("r" if characteristics & 0x40000000 else "-", "w" if characteristics & 0x80000000 else "-", "x" if characteristics & 0x20000000 else "-")),
            "entropy": round(bytes_entropy(blob), 4),
            "sha256": hashlib.sha256(blob).hexdigest(),
        })
    imports = _imports(getattr(pe, "DIRECTORY_ENTRY_IMPORT", []))
    delay_imports = _imports(getattr(pe, "DIRECTORY_ENTRY_DELAY_IMPORT", []))
    debug, pdb_paths = _debug(pe, data)
    rich = parse_rich_header(path)
    overlay_offset = pe.get_overlay_data_start_offset()
    source_paths = sorted({match.decode("latin-1", errors="replace") for regex in (SOURCE_RE, REL_SOURCE_RE) for match in regex.findall(data)}, key=str.casefold)
    assertions = sorted({match.decode("latin-1", errors="replace") for match in ASSERT_RE.findall(data)}, key=str.casefold)
    high_entropy_exec = [section["name"] for section in sections if "x" in section["permissions"] and section["entropy"] > 7.2]
    packed_indicators = []
    if high_entropy_exec:
        packed_indicators.append("high-entropy executable sections: " + ", ".join(high_entropy_exec))
    if len(imports) <= 2 and any(section["entropy"] > 7.4 for section in sections):
        packed_indicators.append("very small import table with high-entropy section")
    timestamp = pe.FILE_HEADER.TimeDateStamp
    try:
        timestamp_utc = dt.datetime.fromtimestamp(timestamp, dt.timezone.utc).isoformat()
    except (OSError, OverflowError, ValueError):
        timestamp_utc = None
    result = {
        "identity": f"{variant}/{relative_path}/{sha256_file(path)}",
        "variant": variant,
        "relative_path": relative_path,
        "module_name": path.name,
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "metadata_normalized_sha256": metadata_normalized_pe_hash(path),
        "machine": MACHINES.get(pe.FILE_HEADER.Machine, f"0x{pe.FILE_HEADER.Machine:x}"),
        "image_base": pe.OPTIONAL_HEADER.ImageBase,
        "entry_point_rva": pe.OPTIONAL_HEADER.AddressOfEntryPoint,
        "entry_point": pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.AddressOfEntryPoint,
        "pe_timestamp": timestamp,
        "pe_timestamp_utc": timestamp_utc,
        "subsystem": SUBSYSTEMS.get(pe.OPTIONAL_HEADER.Subsystem, str(pe.OPTIONAL_HEADER.Subsystem)),
        "linker_version": f"{pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.OPTIONAL_HEADER.MinorLinkerVersion}",
        "image_version": f"{pe.OPTIONAL_HEADER.MajorImageVersion}.{pe.OPTIONAL_HEADER.MinorImageVersion}",
        "os_version": f"{pe.OPTIONAL_HEADER.MajorOperatingSystemVersion}.{pe.OPTIONAL_HEADER.MinorOperatingSystemVersion}",
        "sections": sections,
        "version_resources": _version_info(pe),
        "imports": imports,
        "delay_imports": delay_imports,
        "exports": _exports(pe),
        "debug_directory": debug,
        "pdb_paths": pdb_paths,
        "rich_header": rich,
        "tls_callbacks": _tls_callbacks(pe),
        "overlay_size": len(data) - overlay_offset if overlay_offset is not None else 0,
        "packed": bool(packed_indicators),
        "packed_indicators": packed_indicators,
        "exception_sections": [section["name"] for section in sections if section["name"] in {".pdata", ".xdata"}],
        "msvc_rtti_signature_count": data.count(b".?AV") + data.count(b".?AU"),
        "source_paths": source_paths,
        "assertion_strings": assertions,
    }
    result["compiler_hypothesis"] = _compiler_hypothesis(rich, imports, sections, result["linker_version"])
    return result
