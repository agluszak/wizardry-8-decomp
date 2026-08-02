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

# GhidraClassLoader.addPath cannot replace classes the long-lived JVM already
# loaded: rebuilding the jar and adding it again would silently keep running
# the old code. The digest of the jar this process attached is therefore
# remembered, and a rebuild inside one process is a hard error demanding a
# fresh process instead of a wrong measurement.
_loaded_jar_digest: str | None = None


def _attach_jar(jar_path: Path) -> None:
    global _loaded_jar_digest

    digest = hashlib.sha256(jar_path.read_bytes()).hexdigest()
    if _loaded_jar_digest == digest:
        return
    if _loaded_jar_digest is not None:
        raise RuntimeError(
            "the exporter jar changed after this process already loaded an "
            "earlier build; the JVM cannot reload classes in place. Rerun the "
            "command in a fresh process."
        )
    from java.lang import ClassLoader

    ClassLoader.getSystemClassLoader().addPath(str(jar_path))
    _loaded_jar_digest = digest


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
        for name in ("javac", "javac.exe"):
            candidate = Path(java_home) / "bin" / name
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


def _reference_forms(repo_dir: Path, entries: list[int]) -> list[list[str]]:
    """Compiler-owned full reference form for every source parameter slot."""

    from ..source_model import load_source_index

    declarations = {
        marker["address"]: marker.get("declaration") or {}
        for marker in load_source_index(repo_dir)["markers"]
    }
    result: list[list[str]] = []
    for entry in entries:
        declaration = declarations.get(entry, {})
        forms = declaration.get("parameter_reference_forms")
        if isinstance(forms, list) and forms:
            result.append(
                [
                    str(form.get("kind", "value")) if isinstance(form, dict) else "value"
                    for form in forms
                ]
            )
            continue
        # Backward-compatible checked-tree indexes are conservative: a legacy
        # true bit proves only that some reference existed, not what it referred
        # to, so it cannot authorize pointer-to-object syntax rewriting.
        result.append(["value" for _ in declaration.get("parameter_references", [])])
    return result


def _resolve_data_addresses(selections: list[str]) -> list[int]:
    addresses: list[int] = []
    for selection in selections:
        start, end = parse_selection(selection)
        if end is not None:
            raise ValueError(f"--data takes plain addresses, not ranges: {selection!r}")
        addresses.append(start)
    return list(dict.fromkeys(addresses))


def _packet_result(entry: int, packet: Any) -> dict[str, Any]:
    """Convert one Java single-decompile packet into stable Python data."""

    body_owner = int(packet.getBodyOwner())
    canonical = int(packet.getCanonicalTarget())
    passes = [
        {
            "status": str(item.getStatus()),
            "pass": str(item.getPass()),
            "detail": str(item.getDetail()),
        }
        for item in packet.getPasses()
    ]
    calls = [
        {
            "site": None if int(item.getSite()) < 0 else f"0x{int(item.getSite()):08x}",
            "referenced": f"0x{int(item.getReferenced()):08x}",
            "canonical": f"0x{int(item.getCanonical()):08x}",
            "referenced_name": str(item.getReferencedName()),
            "canonical_name": str(item.getCanonicalName()),
            "origin": str(item.getOrigin()),
            "thunk_kind": str(item.getThunkKind()),
            "this_adjustment": int(item.getThisAdjustment()),
            "return_adjustment": int(item.getReturnAdjustment()),
            "evidence": str(item.getEvidence()),
        }
        for item in packet.getCalls()
    ]
    vtables = []
    for table in packet.getVtables():
        slots = [int(value) for value in table.getSlots()]
        names = [str(value) for value in table.getSlotNames()]
        vtables.append(
            {
                "address": f"0x{int(table.getAddress()):08x}",
                "name": str(table.getName()),
                "slots": [
                    {"address": f"0x{address:08x}", "name": name}
                    for address, name in zip(slots, names, strict=True)
                ],
            }
        )
    return {
        "entry": f"0x{entry:08x}",
        "text": str(packet.getText()),
        "body": str(packet.getBody()),
        "recovery": {
            "source_kind": str(packet.getSourceKind()),
            "emission_kind": str(packet.getEmissionKind()),
            "body_owner": None if body_owner < 0 else f"0x{body_owner:08x}",
            "canonical_target": None if canonical < 0 else f"0x{canonical:08x}",
            "origin": str(packet.getOrigin()),
            "evidence": str(packet.getEvidence()),
            "passes": passes,
            "calls": calls,
            "vtables": vtables,
            "defects": [str(item) for item in packet.getDefects()],
        },
    }


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
    _attach_jar(jar_path)

    import jpype
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    # Each function remains independently bounded across the Java/Python
    # boundary; ``text`` is only the user-facing concatenated presentation.
    exports: list[dict[str, Any]] = []
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            exporter = jpype.JClass(_EXPORTER_CLASS)
            from ghidra.util.task import TaskMonitor

            if class_name is not None:
                text = str(exporter.exportClass(program, class_name, TaskMonitor.DUMMY))
                if text.startswith("// error:"):
                    raise ValueError(text.strip().removeprefix("// error: "))
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
                markers = unit_markers(settings.repo_dir, unit)
                entries = [
                    marker["address"] for marker in markers if marker["marker_kind"] == "FUNCTION"
                ]
                string_array = jpype.JArray(jpype.JString)
                reference_forms = jpype.JArray(string_array)(
                    [string_array(forms) for forms in _reference_forms(settings.repo_dir, entries)]
                )
                java_packets = exporter.exportFunctionPackets(
                    program,
                    jpype.JArray(jpype.JLong)(entries),
                    reference_forms,
                    TaskMonitor.DUMMY,
                )
                exports = [
                    _packet_result(entry, packet)
                    for entry, packet in zip(entries, java_packets, strict=True)
                ]
                blocks = {entry: item["text"] for entry, item in zip(entries, exports, strict=True)}
                text = assemble_unit(markers, blocks)
                functions = [
                    {
                        "entry": f"0x{marker['address']:08x}",
                        "kind": marker["marker_kind"].lower(),
                    }
                    for marker in markers
                ]
            else:
                entries = _resolve_entries(program, selections)
                string_array = jpype.JArray(jpype.JString)
                reference_forms = jpype.JArray(string_array)(
                    [string_array(forms) for forms in _reference_forms(settings.repo_dir, entries)]
                )
                java_packets = exporter.exportFunctionPackets(
                    program,
                    jpype.JArray(jpype.JLong)(entries),
                    reference_forms,
                    TaskMonitor.DUMMY,
                )
                exports = [
                    _packet_result(entry, packet)
                    for entry, packet in zip(entries, java_packets, strict=True)
                ]
                text = "\n".join(item["text"] for item in exports)
                functions = [
                    {
                        "entry": f"0x{entry:08x}",
                        "kind": item["recovery"]["emission_kind"],
                    }
                    for entry, item in zip(entries, exports, strict=True)
                ]
    finally:
        project.close()

    result: dict[str, Any] = {
        "program": program_name,
        "functions": functions,
        "exports": exports,
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


def explain_function(
    settings: Settings,
    selection: str | None = None,
    *,
    program_selector: str = "wiz8",
    class_name: str | None = None,
) -> dict[str, Any]:
    """Trace one function's passes or report a class's complete ABI family."""

    if (selection is None) == (class_name is None):
        raise ValueError("pass one function address or --class (exactly one)")

    jar_path = ensure_exporter_jar(settings)

    from .environment import start_pyghidra
    from .workspace import ensure_seed

    program_name = ensure_seed(settings, program_selector)
    start_pyghidra(settings)
    _attach_jar(jar_path)

    import jpype
    import pyghidra

    start: int | None = None
    if selection is not None:
        start, end = parse_selection(selection)
        if end is not None:
            raise ValueError("explain takes one plain address")

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            exporter = jpype.JClass(_EXPORTER_CLASS)
            from ghidra.util.task import TaskMonitor

            if class_name is not None:
                text = str(exporter.explainClass(program, class_name, TaskMonitor.DUMMY))
            else:
                assert start is not None
                string_array = jpype.JArray(jpype.JString)
                packets = exporter.exportFunctionPackets(
                    program,
                    jpype.JArray(jpype.JLong)([start]),
                    jpype.JArray(string_array)(
                        [
                            string_array(forms)
                            for forms in _reference_forms(settings.repo_dir, [start])
                        ]
                    ),
                    TaskMonitor.DUMMY,
                )
                packet = _packet_result(start, packets[0])
                recovery = packet["recovery"]
                facts = recovery["passes"]
                lines = [f"0x{start:08x} {recovery['emission_kind']} ({recovery['source_kind']})"]
                lines.extend(
                    f"{fact['status']:<12} {fact['pass']}: {fact['detail']}" for fact in facts
                )
                if not facts:
                    lines.append("no recognizer applied or declined; verbatim rendering")
                lines.extend(f"defect      {defect}" for defect in recovery["defects"])
                text = "\n".join(lines) + "\n"
    finally:
        project.close()
    result = {"program": program_name, "text": text}
    if class_name is not None:
        result["class"] = class_name
    else:
        assert start is not None
        result["address"] = f"0x{start:08x}"
    return result
