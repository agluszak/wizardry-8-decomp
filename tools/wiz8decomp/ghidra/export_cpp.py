"""Export recovered-style C++ for selected functions via the Java extension.

The semantic engine lives in ``tools/ghidra-extension`` and runs inside the
existing PyGhidra JVM. This module owns the disposable jar build, the
classloader wiring, selection resolution, and output handling. The jar is
attached with ``GhidraClassLoader.addPath`` after the JVM starts;
``launcher.add_classpaths`` would put it on the parent loader, which cannot
see the Ghidra module jars.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import zipfile
from pathlib import Path
from typing import Any

from .. import subprocesses
from ..config import REQUIRED_GHIDRA_VERSION, Settings
from ..paths import atomic_json, atomic_write

_EXPORTER_CLASS = "wiz8.exporter.Wiz8RecoveryExporter"
_MINIMUM_JDK_MAJOR = 21


def parse_selection(text: str) -> tuple[int, int | None]:
    """Parse ``0xADDR`` or an inclusive ``0xSTART:0xEND`` entry range."""

    raw = text.strip()
    if not raw:
        raise ValueError("empty selection")
    start_text, separator, end_text = raw.partition(":")
    try:
        start = int(start_text, 0)
        end = int(end_text, 0) if separator else None
    except ValueError as error:
        raise ValueError(f"invalid selection {text!r}: {error}") from error
    if start < 0 or (end is not None and end < 0):
        raise ValueError(f"invalid selection {text!r}: addresses must be non-negative")
    if end is not None and end < start:
        raise ValueError(f"invalid selection {text!r}: range end precedes start")
    return start, end


def source_directory(settings: Settings) -> Path:
    return settings.repo_dir / "tools" / "ghidra-extension" / "src"


def _source_files(settings: Settings) -> list[Path]:
    sources = sorted(source_directory(settings).rglob("*.java"))
    if not sources:
        raise RuntimeError(f"no Java sources under {source_directory(settings)}")
    return sources


def _sources_digest(settings: Settings, sources: list[Path]) -> str:
    digest = hashlib.sha256()
    for source in sources:
        digest.update(source.relative_to(source_directory(settings)).as_posix().encode())
        digest.update(b"\0")
        digest.update(source.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def _find_javac() -> Path:
    java_home = os.environ.get("JAVA_HOME")
    if java_home:
        candidate = Path(java_home) / "bin" / "javac"
        if candidate.is_file():
            return candidate
    found = shutil.which("javac")
    if found:
        return Path(found)
    raise RuntimeError(
        "javac not found; building the Ghidra extension requires a JDK "
        f"(Ghidra {REQUIRED_GHIDRA_VERSION} runs on JDK {_MINIMUM_JDK_MAJOR}). "
        "Install JDK 21 or set JAVA_HOME."
    )


def _check_javac_version(settings: Settings, javac: Path) -> None:
    result = subprocesses.run([javac, "-version"], cwd=settings.repo_dir)
    reported = (result.stdout + result.stderr).strip()
    version = reported.split()[-1] if reported.split() else ""
    major_text = version.split(".")[0]
    if not major_text.isdigit() or int(major_text) < _MINIMUM_JDK_MAJOR:
        raise RuntimeError(
            f"javac {version or 'unknown'} at {javac} is too old; "
            f"JDK {_MINIMUM_JDK_MAJOR} or newer is required."
        )


def _ghidra_classpath(settings: Settings) -> str:
    jars = sorted(settings.ghidra_install_dir.glob("Ghidra/*/*/lib/*.jar"))
    if not jars:
        raise RuntimeError(f"no Ghidra module jars under {settings.ghidra_install_dir}")
    return os.pathsep.join(str(jar) for jar in jars)


def ensure_exporter_jar(settings: Settings) -> Path:
    """Compile the extension when stale and return the jar path."""

    output_dir = settings.build_dir / "ghidra-extension"
    jar_path = output_dir / "wiz8-recovery.jar"
    stamp_path = output_dir / "stamp.json"
    sources = _source_files(settings)
    stamp = {
        "sources_sha256": _sources_digest(settings, sources),
        "ghidra_install_dir": str(settings.ghidra_install_dir),
        "ghidra_version": REQUIRED_GHIDRA_VERSION,
    }
    if jar_path.is_file() and stamp_path.is_file():
        try:
            if json.loads(stamp_path.read_text(encoding="utf-8")) == stamp:
                return jar_path
        except (OSError, json.JSONDecodeError):
            pass

    javac = _find_javac()
    _check_javac_version(settings, javac)
    classes_dir = output_dir / "classes"
    if classes_dir.exists():
        shutil.rmtree(classes_dir)
    classes_dir.mkdir(parents=True)
    subprocesses.run(
        [
            javac,
            "--release",
            str(_MINIMUM_JDK_MAJOR),
            "-encoding",
            "UTF-8",
            "-cp",
            _ghidra_classpath(settings),
            "-d",
            classes_dir,
            *sources,
        ],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "ghidra-extension-javac.json",
    )

    temp_jar = jar_path.with_suffix(".jar.tmp")
    with zipfile.ZipFile(temp_jar, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for class_file in sorted(classes_dir.rglob("*.class")):
            archive.write(class_file, class_file.relative_to(classes_dir).as_posix())
    os.replace(temp_jar, jar_path)
    atomic_json(stamp_path, stamp)
    return jar_path


def _resolve_entries(program: Any, selections: list[str]) -> list[int]:
    from .query import _address, _function

    entries: list[int] = []
    for selection in selections:
        start, end = parse_selection(selection)
        if end is None:
            function = _function(program, f"0x{start:08x}")
            entries.append(int(function.getEntryPoint().getOffset()))
            continue
        from ghidra.program.model.address import AddressSet

        address_set = AddressSet(
            _address(program, f"0x{start:08x}"), _address(program, f"0x{end:08x}")
        )
        found = [
            int(function.getEntryPoint().getOffset())
            for function in program.getFunctionManager().getFunctions(address_set, True)
            if address_set.contains(function.getEntryPoint())
        ]
        if not found:
            raise ValueError(f"no function entry points in {selection}")
        entries.extend(found)
    seen: set[int] = set()
    ordered = [entry for entry in entries if not (entry in seen or seen.add(entry))]
    return ordered


def unit_markers(repo_dir: Path, unit: str) -> list[dict[str, Any]]:
    """The unit's markers in file order.

    Git owns the matching markers, so for whole-unit export the source
    index's marker kinds override Ghidra's name-based classification:
    SYNTHETIC/TEMPLATE blocks are reconstructed from the recorded symbol and
    only FUNCTION-marked addresses are decompiled.
    """

    from ..source_model import load_source_index

    markers = [
        marker for marker in load_source_index(repo_dir)["markers"] if marker["source_file"] == unit
    ]
    if not markers:
        raise ValueError(f"no markers recorded for translation unit {unit}")
    markers.sort(key=lambda marker: marker["line"])
    return markers


def assemble_unit(markers: list[dict[str, Any]], blocks: dict[int, str]) -> str:
    """Join per-marker blocks in file order into the unit's export text."""

    parts: list[str] = []
    for marker in markers:
        address = marker["address"]
        kind = marker["marker_kind"]
        if kind == "FUNCTION":
            parts.append(blocks.get(address, f"// error: no export for 0x{address:08x}\n"))
        elif kind in {"SYNTHETIC", "TEMPLATE"}:
            parts.append(f"// {kind}: WIZ8 0x{address:08x}\n// {marker['marker_name']}\n")
        else:
            parts.append(f"// {kind}: WIZ8 0x{address:08x}\n")
    return "\n".join(parts)


def _resolve_data_addresses(selections: list[str]) -> list[int]:
    addresses: list[int] = []
    for selection in selections:
        start, end = parse_selection(selection)
        if end is not None:
            raise ValueError(f"--data takes plain addresses, not ranges: {selection!r}")
        addresses.append(start)
    return addresses


def export_cpp(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str = "wiz8",
    class_name: str | None = None,
    unit: str | None = None,
    data: bool = False,
    output: Path | None = None,
) -> dict[str, Any]:
    if sum([class_name is not None, unit is not None, bool(selections)]) != 1:
        raise ValueError("pass address selections, --class, or --unit (exactly one)")
    if data and not selections:
        raise ValueError("--data takes plain addresses only")
    jar_path = ensure_exporter_jar(settings)

    from .environment import start_pyghidra
    from .workspace import ensure_seed

    program_name = ensure_seed(settings, program_selector)
    start_pyghidra(settings)

    from java.lang import ClassLoader

    ClassLoader.getSystemClassLoader().addPath(str(jar_path))

    import jpype
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            exporter = jpype.JClass(_EXPORTER_CLASS)
            from ghidra.util.task import TaskMonitor

            if class_name is not None:
                text = str(exporter.exportClass(program, class_name, TaskMonitor.DUMMY))
                functions = []
            elif data:
                entries = _resolve_data_addresses(selections)
                text = str(
                    exporter.exportData(
                        program, jpype.JArray(jpype.JLong)(entries), TaskMonitor.DUMMY
                    )
                )
                functions = [{"entry": f"0x{entry:08x}", "kind": "data"} for entry in entries]
            elif unit is not None:
                from ..recover import split_export_blocks

                markers = unit_markers(settings.repo_dir, unit)
                entries = [
                    marker["address"] for marker in markers if marker["marker_kind"] == "FUNCTION"
                ]
                raw = str(
                    exporter.export(program, jpype.JArray(jpype.JLong)(entries), TaskMonitor.DUMMY)
                )
                text = assemble_unit(markers, split_export_blocks(raw))
                functions = [
                    {
                        "entry": f"0x{marker['address']:08x}",
                        "kind": marker["marker_kind"].lower(),
                    }
                    for marker in markers
                ]
            else:
                entries = _resolve_entries(program, selections)
                text = str(
                    exporter.export(
                        program,
                        jpype.JArray(jpype.JLong)(entries),
                        TaskMonitor.DUMMY,
                    )
                )
                functions = [
                    {
                        "entry": f"0x{entry:08x}",
                        "kind": _kind_name(program, entry),
                    }
                    for entry in entries
                ]
    finally:
        project.close()

    result: dict[str, Any] = {
        "program": program_name,
        "functions": functions,
        "text": text,
    }
    if class_name is not None:
        result["class"] = class_name
    if unit is not None:
        result["unit"] = unit
    if output is not None:
        atomic_write(output, text)
        result["outputs"] = [str(output)]
    return result


def _kind_name(program: Any, entry: int) -> str:
    import jpype

    function = program.getFunctionManager().getFunctionAt(
        program.getAddressFactory().getDefaultAddressSpace().getAddress(entry)
    )
    if function is None:
        return "missing"
    kind = jpype.JClass("wiz8.exporter.FunctionKind").classify(function)
    return str(kind.name()).lower()
