from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from ..subprocesses import run, tool_version
from .manifest import Evidence


@dataclass(frozen=True)
class Detection:
    detected_type: str
    container: str | None
    installer: str | None
    extraction_tool: str | None
    extraction_version: str | None
    evidence: list[Evidence]


def _contains(path: Path, needles: tuple[bytes, ...], limit: int = 4 * 1024 * 1024) -> bytes:
    with path.open("rb") as stream:
        head = stream.read(limit)
        if path.stat().st_size > limit:
            stream.seek(max(0, path.stat().st_size - limit))
            head += stream.read(limit)
    lower = head.lower()
    return next((needle for needle in needles if needle.lower() in lower), b"")


def _seven_zip_probe(path: Path) -> tuple[str | None, str]:
    info = tool_version("7z")
    if not info["executable"]:
        return None, ""
    result = run(["7z", "l", "-slt", path], cwd=path.parent, check=False)
    text = result.stdout + "\n" + result.stderr
    match = re.search(r"^Type = (.+)$", text, re.MULTILINE | re.IGNORECASE)
    return (match.group(1).strip() if match else None), text


def detect(path: Path) -> Detection:
    with path.open("rb") as stream:
        head = stream.read(0x9000)
    evidence: list[Evidence] = []
    detected = "unknown"
    container = None
    installer = None
    extractor = None

    if len(head) > 0x8006 and head[0x8001:0x8006] == b"CD001":
        detected, container, extractor = "iso-image", "ISO 9660", "7z"
        evidence.append(Evidence(kind="signature", value="CD001 at ISO volume descriptor"))
    elif head.startswith(b"7z\xbc\xaf\x27\x1c"):
        detected, container, extractor = "archive", "7z", "7z"
        evidence.append(Evidence(kind="signature", value="7z magic"))
    elif head.startswith((b"PK\x03\x04", b"PK\x05\x06", b"PK\x07\x08")):
        detected, container, extractor = "archive", "ZIP", "7z"
        evidence.append(Evidence(kind="signature", value="ZIP magic"))
    elif head.startswith((b"Rar!\x1a\x07\x00", b"Rar!\x1a\x07\x01\x00")):
        detected, container, extractor = "archive", "RAR", "7z"
        evidence.append(Evidence(kind="signature", value="RAR magic"))
    elif head.startswith(b"MSCF"):
        detected, container, extractor = "archive", "Microsoft CAB", "cabextract"
        evidence.append(Evidence(kind="signature", value="MSCF magic"))
    elif head.startswith(b"MZ"):
        detected = "pe-executable"
        evidence.append(Evidence(kind="signature", value="DOS MZ header"))
        marker = _contains(path, (b"Inno Setup", b"InstallShield", b"Nullsoft", b"7-Zip SFX"))
        archive_type, listing = _seven_zip_probe(path)
        if marker:
            installer = marker.decode("ascii", errors="replace")
            detected = "installer"
            evidence.append(Evidence(kind="embedded-string", value=installer))
        if archive_type and archive_type.casefold() not in {"pe", "coff"}:
            container = archive_type
            detected = "installer" if installer or archive_type.casefold() == "cab" else "self-extracting-archive"
            evidence.append(Evidence(kind="7z-probe", value=f"embedded {archive_type} container"))
        if "This installation was built with Inno Setup" in listing:
            installer = "Inno Setup"
        if installer == "Inno Setup":
            extractor = "innoextract"
        elif installer == "InstallShield" or (container or "").casefold() == "cab":
            extractor = "7z+unshield"
        else:
            extractor = "7z" if container else None
    else:
        evidence.append(Evidence(kind="signature", value=head[:16].hex()))

    version = None
    if extractor:
        primary = extractor.split("+")[0]
        if primary == "7z":
            primary = "7z"
        version = tool_version(primary, ("i",) if primary == "7z" else ("--version",))["version"]
    return Detection(detected, container, installer, extractor, version, evidence)


def extension_mismatch(path: Path, detected_type: str, container: str | None) -> bool:
    suffix = path.suffix.casefold()
    expected = {
        ".iso": {"iso-image"},
        ".zip": {"archive"},
        ".7z": {"archive"},
        ".rar": {"archive"},
        ".cab": {"archive"},
        ".exe": {"pe-executable", "installer", "self-extracting-archive"},
        ".dll": {"pe-executable"},
    }
    return suffix in expected and detected_type not in expected[suffix]
