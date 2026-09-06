from __future__ import annotations

import json
import os
import struct
import sys
from contextlib import contextmanager
from dataclasses import asdict, dataclass
from datetime import UTC, datetime
from fcntl import LOCK_EX, LOCK_NB, LOCK_UN, flock
from pathlib import Path
from typing import Any

import pefile
import yaml

from .binary.coff_archive import coff_member_kind, named_iat_archive, read_coff_archive
from .build_dir import check_build_directory
from .config import Settings
from .paths import atomic_json
from .reccmp_data import write_wiz8_data_source
from .subprocesses import CommandResult, resolve_executable, run

VC6_IMAGE = "wizardry8-msvc600:sp5"
LINT_BUILD_DIR = "build/clang"
DIAGNOSTICS_BUILD_DIR = "build/clang-diagnostics"
TARGET_ALIASES = {
    "match": "WIZ8",
    "runtime": "WIZ8_RUNTIME",
    "runtime-test": "WIZ8_RUNTIME_TEST",
}
PRODUCT_GENERATOR = "NMake Makefiles"
JOM_PROGRAM = r"C:\jom\jom.exe"


@dataclass(frozen=True)
class Mount:
    host: Path
    container: str
    read_only: bool = True

    def docker_argument(self) -> str:
        suffix = ":ro" if self.read_only else ""
        return f"{self.host}:{self.container}{suffix}"


@dataclass(frozen=True)
class ContainerBuild:
    image: str
    mounts: tuple[Mount, ...]
    source_dir: Path
    build_dir: Path
    container_source_dir: str = "Z:/repo"

    @classmethod
    def from_settings(cls, settings: Settings) -> ContainerBuild:
        sources = settings.work_dir / "fid" / "sources" / "unpacked"
        return cls(
            image=VC6_IMAGE,
            mounts=(
                Mount(settings.repo_dir, "/repo"),
                Mount(sources / "ijg-jpeg-6" / "jpeg-6", "/jpeg"),
                Mount(sources / "zlib-1.0.4" / "zlib-1.0.4", "/zlib"),
                Mount(sources / "infozip-unzip-5.4", "/infozip"),
                Mount(settings.repo_dir / "build" / "decomp", "/out", read_only=False),
            ),
            source_dir=settings.repo_dir,
            build_dir=settings.repo_dir / "build" / "decomp",
        )

    def docker_prefix(self) -> list[str]:
        docker = resolve_executable("docker") or "docker"
        # CMake's JOM generator creates nested Windows processes. Docker's init
        # shim must reap them; with Wine as PID 1 the compiler probes leave
        # zombies and CMake waits forever after a successful try-compile.
        command = [docker, "run", "--rm", "--init", "--network", "none"]
        for mount in self.mounts:
            command.extend(("--volume", mount.docker_argument()))
        command.append(self.image)
        return command

    def configure_command(self, *, fresh: bool = False) -> list[str]:
        command = self.docker_prefix()
        command.extend(
            (
                r"C:\cmake\bin\cmake.exe",
                *(("--fresh",) if fresh else ()),
                "-S",
                self.container_source_dir,
                "-B",
                "Z:/out",
                "-G",
                PRODUCT_GENERATOR,
                "-DIJG_JPEG_SOURCE=Z:/jpeg",
                "-DZLIB_SOURCE=Z:/zlib",
                "-DINFOZIP_SOURCE=Z:/infozip",
                "-DSGP_SOURCE=Z:/repo/third_party/sfi-sgp/sgp",
                "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
            )
        )
        return command

    def build_command(self, target: str, jobs: int) -> list[str]:
        return [
            *self.docker_prefix(),
            "cmd",
            "/c",
            (
                r"set TEMP=Z:\out\tmp&& set TMP=Z:\out\tmp&& "
                rf"cd /d Z:\out&& C:\jom\jom.exe -j {jobs} {target}"
            ),
        ]

    def check_build_system_command(self) -> list[str]:
        return [
            *self.docker_prefix(),
            "cmd",
            "/c",
            (
                r"set TEMP=Z:\out\tmp&& set TMP=Z:\out\tmp&& "
                rf"cd /d Z:\out&& {JOM_PROGRAM} cmake_check_build_system"
            ),
        ]


def _result(result: CommandResult) -> dict[str, Any]:
    return asdict(result)


def _owner_path(build_dir: Path) -> Path:
    return build_dir / ".wiz8-build-owner.json"


def _product_cache_ready(build_dir: Path) -> bool:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return False
    cached = cache.read_text(encoding="utf-8", errors="replace").replace("\r", "")
    return f"CMAKE_GENERATOR:INTERNAL={PRODUCT_GENERATOR}\n" in cached


def _enable_jom_parallelism(build_dir: Path) -> list[str]:
    """Remove only CMake's generated NMake serialization guards.

    CMake's native JOM generator does not return from its VC6 try-compile under
    Wine even with Docker's init shim.  The ordinary NMake generator is the
    stable configuration path; JOM understands those makefiles once the two
    top-level serialization directives are removed.
    """

    updated: list[str] = []
    for path in (build_dir / "Makefile", build_dir / "CMakeFiles" / "Makefile2"):
        content = path.read_bytes()
        replacement = content.replace(b".NOTPARALLEL:\r\n", b"# .NOTPARALLEL removed for JOM\r\n")
        replacement = replacement.replace(b".NOTPARALLEL:\n", b"# .NOTPARALLEL removed for JOM\n")
        if replacement != content:
            path.write_bytes(replacement)
            updated.append(str(path))
    return updated


def validate_build_directory(settings: Settings) -> dict[str, Any]:
    build_dir = settings.repo_dir / "build" / "decomp"
    marker = _owner_path(build_dir)
    if marker.is_file():
        owner = json.loads(marker.read_text(encoding="utf-8"))
        if Path(owner["source_dir"]).resolve() != settings.repo_dir.resolve():
            raise RuntimeError(f"{marker} belongs to another checkout: {owner['source_dir']}")
        if Path(owner["build_dir"]).resolve() != build_dir.resolve():
            raise RuntimeError(f"{marker} records a moved build directory: {owner['build_dir']}")
        return {"ok": True, "configured": True, **owner}
    return check_build_directory(build_dir, settings.repo_dir)


@contextmanager
def build_lock(settings: Settings):
    """Fail fast when another command owns this checkout's product build."""

    build_dir = settings.repo_dir / "build" / "decomp"
    build_dir.mkdir(parents=True, exist_ok=True)
    lock_path = build_dir / ".wiz8-build.lock"
    with lock_path.open("a+", encoding="utf-8") as stream:
        try:
            flock(stream.fileno(), LOCK_EX | LOCK_NB)
        except BlockingIOError as error:
            stream.seek(0)
            holder = stream.read().strip() or "unknown holder"
            raise RuntimeError(f"product build is already running: {holder}") from error
        record = {
            "pid": os.getpid(),
            "command": sys.argv,
            "cwd": str(Path.cwd()),
            "started_at": datetime.now(UTC).isoformat(),
        }
        stream.seek(0)
        stream.truncate()
        json.dump(record, stream)
        stream.flush()
        os.fsync(stream.fileno())
        try:
            yield
        finally:
            flock(stream.fileno(), LOCK_UN)


def prepare(settings: Settings) -> dict[str, Any]:
    from .extract.variants import extract_all, materialize_variants
    from .ghidra.fid_seeds import fetch_seed_sources
    from .inputs.scan import load_manifest
    from .pipeline import PipelineStage, clean_pipeline, verify_pipeline

    manifest = load_manifest(settings)
    pipeline = verify_pipeline(settings)
    repairs: list[dict[str, Any]] = []
    failed_stages = {check["stage"] for check in pipeline["checks"] if not check["ok"]}
    if "extractions" in failed_stages:
        repairs.append(clean_pipeline(settings, PipelineStage.extractions))
    elif "variants" in failed_stages:
        repairs.append(clean_pipeline(settings, PipelineStage.variants))
    extractions = extract_all(settings)
    variants = materialize_variants(settings)
    sources = fetch_seed_sources(settings)
    write_wiz8_data_source(settings.repo_dir)
    _write_sr_assert_import(settings.repo_dir)
    detected = run(
        [
            "reccmp-project",
            "detect",
            "--search-path",
            settings.work_dir / "variants" / "gog-base",
            settings.work_dir / "variants" / "gog-base" / "Dll",
            "--what",
            "original",
        ],
        cwd=settings.repo_dir,
    )
    return {
        "manifest_files": len(manifest.files),
        "repairs": repairs,
        "extractions": len(extractions),
        "variants": len(variants["variants"]),
        "sources": {
            "ready": sum(row["status"] == "ready" for row in sources["sources"]),
            "skipped": sum(row["status"] != "ready" for row in sources["sources"]),
        },
        "detect": _result(detected),
    }


def _write_sr_assert_import(repository: Path) -> None:
    path = repository / "build" / "decomp" / "sr-assert-import.lib"
    payload = named_iat_archive(
        "SR.dll", "?srAssertFail@@YAXPBD0J0@Z", "?srAssertFail@@YAXPBD0J0ZZ"
    )
    if not path.is_file() or path.read_bytes() != payload:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)


def _configure(settings: Settings) -> dict[str, Any]:
    build = ContainerBuild.from_settings(settings)
    build.build_dir.mkdir(parents=True, exist_ok=True)
    (build.build_dir / "tmp").mkdir(parents=True, exist_ok=True)
    validate_build_directory(settings)
    cache = build.build_dir / "CMakeCache.txt"
    fresh = cache.is_file() and not _product_cache_ready(build.build_dir)
    configured = run(build.configure_command(fresh=fresh), cwd=settings.repo_dir)
    atomic_json(
        _owner_path(build.build_dir),
        {"source_dir": str(settings.repo_dir), "build_dir": str(build.build_dir)},
    )
    return {"configure": _result(configured)}


def configure(settings: Settings) -> dict[str, Any]:
    with build_lock(settings):
        return _configure(settings)


def _check_product_imports(repository: Path, target: str) -> dict[str, object]:
    """Check the small consumer libraries against original PEs, not our sr.dll."""
    boundaries = {
        "WIZ8": (
            ("MSS32", "mss32.dll", "src/wiz8/imports/mss32.def", "wiz8_mss32.lib"),
            ("BINKW32", "binkw32.dll", "src/wiz8/imports/binkw32.def", "wiz8_binkw32.lib"),
        ),
        "SREXT_JPEGIMPORTER": (
            (
                "SURRENDER",
                "sr.dll",
                "src/srext_jpegimporter/sr-jpeg-imports.def",
                "srext_jpeg_sr.lib",
            ),
        ),
        "SREXT_UNZIP": (
            ("SURRENDER", "sr.dll", "src/srext_unzip/sr-unzip-imports.def", "srext_unzip_sr.lib"),
        ),
    }
    if target not in boundaries:
        return {}
    originals = yaml.safe_load((repository / "reccmp-user.yml").read_text())["targets"]
    products = yaml.safe_load((repository / "build/decomp/reccmp-build.yml").read_text())["targets"]

    def imports(path: Path) -> dict[str, set[str]]:
        with pefile.PE(str(path), fast_load=True) as pe:
            pe.parse_data_directories([pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
            return {
                entry.dll.decode("ascii").lower(): {
                    symbol.name.decode("ascii") if symbol.name else f"#{symbol.ordinal}"
                    for symbol in entry.imports
                }
                for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", [])
            }

    def exports(provider: str) -> set[str]:
        with pefile.PE(originals[provider]["path"], fast_load=True) as pe:
            pe.parse_data_directories([pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
            directory = getattr(pe, "DIRECTORY_ENTRY_EXPORT", None)
            if directory is None:
                return set()
            return {symbol.name.decode("ascii") for symbol in directory.symbols if symbol.name}

    original = imports(Path(originals[target]["path"]))
    product_path = Path(products[target]["path"])
    if not product_path.is_absolute():
        product_path = repository / "build/decomp" / product_path
    built = imports(product_path)
    checked: dict[str, object] = {}
    for provider, dll, definition, library in boundaries[target]:
        expected = original[dll].copy()
        # Retail's malformed name is not exported by its own MSS provider.
        if dll == "mss32.dll" and "_AIL_init_sample@" in expected:
            expected.remove("_AIL_init_sample@")
            expected.add("_AIL_init_sample@4")
        declared = set()
        for line in (repository / definition).read_text().splitlines():
            line = line.split(";", 1)[0].strip()
            if line and not line.startswith(("LIBRARY", "EXPORTS")):
                declared.add(line)
        imported = set()
        for member in read_coff_archive(repository / "build/decomp" / library):
            if coff_member_kind(member.data) != "coff-import":
                continue  # ordinary descriptor/terminator objects
            name = member.data[20:].split(b"\0", 1)[0].decode("ascii")
            name_type = (struct.unpack_from("<H", member.data, 18)[0] >> 2) & 7
            if name_type == 2:
                name = name[1:] if name.startswith(("_", "@", "?")) else name
            elif name_type != 1:
                raise RuntimeError(f"unexpected import encoding in {library}: {name_type}")
            imported.add(name)
        for owner, names in ((definition, declared), (library, imported)):
            if names != expected:
                raise RuntimeError(
                    f"{owner}: missing {sorted(expected - names)}, "
                    f"extra {sorted(names - expected)} against retail imports"
                )
        if not expected <= exports(provider) or not built[dll] <= expected:
            raise RuntimeError(f"{target}/{dll}: unsupported provider or consumer import names")
        checked[dll] = {
            "retail": len(original[dll]),
            "library": len(imported),
            "linked": len(built[dll]),
        }
    if target == "WIZ8":
        unsupported = built["sr.dll"] - exports("SURRENDER")
        if unsupported:
            raise RuntimeError(
                f"WIZ8/SR.dll: names absent from retail provider: {sorted(unsupported)}"
            )
        if "?srAssertFail@@YAXPBD0J0ZZ" not in built["sr.dll"]:
            raise RuntimeError("WIZ8/SR.dll: srAssertFail's provider import is missing")
        checked["sr.dll"] = {"linked": len(built["sr.dll"]), "provider_supported": True}
    return checked


def build_target(
    settings: Settings, target: str = "match", jobs: int | None = None
) -> dict[str, Any]:
    with build_lock(settings):
        build = ContainerBuild.from_settings(settings)
        resolved_target = TARGET_ALIASES.get(target, target)
        validate_build_directory(settings)
        missing = [
            mount.host
            for mount in build.mounts
            if mount.container not in {"/repo", "/out"} and not mount.host.exists()
        ]
        import_archive = build.build_dir / "sr-assert-import.lib"
        if not import_archive.is_file():
            missing.append(import_archive)
        if missing:
            rendered = ", ".join(str(path) for path in missing)
            raise RuntimeError(
                f"prepared build inputs are missing ({rendered}); run `wiz8 prepare`"
            )
        if not _product_cache_ready(build.build_dir):
            _configure(settings)
        # Let CMake regenerate while its NMake serialization guards are intact,
        # then adapt only those generated guards for the parallel JOM build.
        run(build.check_build_system_command(), cwd=settings.repo_dir)
        parallel_makefiles = _enable_jom_parallelism(build.build_dir)
        run(
            build.build_command(resolved_target, jobs or max(1, os.cpu_count() or 1)),
            cwd=settings.repo_dir,
            log_path=settings.repo_dir / "build" / "logs" / "product-build.json",
        )
        return {
            "target": resolved_target,
            "parallel_makefiles": parallel_makefiles,
            "status": "built",
            "log": str(Path("build/logs/product-build.json")),
        }


def configure_clang(
    settings: Settings, *, full_diagnostics: bool = False
) -> tuple[Path, list[str]]:
    """Configure the compiler-backed source projection and return its runner."""
    docker = resolve_executable("docker") or "docker"
    output = settings.repo_dir / (DIAGNOSTICS_BUILD_DIR if full_diagnostics else LINT_BUILD_DIR)
    output.mkdir(parents=True, exist_ok=True)
    mounts = (
        Mount(settings.repo_dir, "/repo"),
        Mount(output, "/out", read_only=False),
        Mount(
            settings.work_dir / "fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4",
            "/zlib",
        ),
    )

    def prefix() -> list[str]:
        command = [docker, "run", "--rm", "--init", "--network", "none"]
        for mount in mounts:
            command.extend(("--volume", mount.docker_argument()))
        return command

    configure_command = [
        *prefix(),
        "--entrypoint",
        "cmake",
        VC6_IMAGE,
        "--fresh",
        "-S",
        "/repo",
        "-B",
        "/out",
        "-G",
        "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_TOOLCHAIN_FILE=/repo/cmake/clang-cl-i686.cmake",
    ]
    if full_diagnostics:
        configure_command.append("-DWIZ8_FULL_DIAGNOSTICS=ON")
    run(
        configure_command,
        cwd=settings.repo_dir,
        log_path=settings.repo_dir
        / "build"
        / "logs"
        / ("clang-full-configure.json" if full_diagnostics else "clang-lint-configure.json"),
    )
    return output, prefix()


def lint(settings: Settings, *, full_diagnostics: bool = False) -> dict[str, Any]:
    """Compile recovered C++ with structural or full recovery diagnostics."""

    _output, prefix = configure_clang(settings, full_diagnostics=full_diagnostics)
    target = "WIZ8_CLANG_DIAGNOSTICS" if full_diagnostics else "WIZ8_CLANG_LINT"
    run(
        [
            *prefix,
            "--entrypoint",
            "cmake",
            VC6_IMAGE,
            "--build",
            "/out",
            "--target",
            target,
            "--",
            "-k",
            "0",
        ],
        cwd=settings.repo_dir,
        log_path=settings.repo_dir
        / "build"
        / "logs"
        / ("clang-full-diagnostics.json" if full_diagnostics else "clang-lint-build.json"),
    )
    return {
        "mode": "full-diagnostics" if full_diagnostics else "gating",
        "status": "passed",
        "log": str(
            Path("build/logs")
            / ("clang-full-diagnostics.json" if full_diagnostics else "clang-lint-build.json")
        ),
    }


def build_analysis_target(
    settings: Settings, source_dir: str, build_name: str, target: str, jobs: int | None = None
) -> dict[str, Any]:
    output = settings.repo_dir / "build" / "analysis" / build_name
    output.mkdir(parents=True, exist_ok=True)
    (output / "tmp").mkdir(exist_ok=True)
    build = ContainerBuild(
        image=VC6_IMAGE,
        mounts=(
            Mount(settings.repo_dir, "/repo"),
            Mount(output, "/out", read_only=False),
        ),
        source_dir=settings.repo_dir / source_dir,
        build_dir=output,
        container_source_dir=f"Z:/repo/{source_dir}",
    )
    configure_result = run(build.configure_command(), cwd=settings.repo_dir)
    build_result = run(
        # VC6 writes shared PDB state while compiling the released SGP units.
        # Keep this independent oracle build serial just as the old probe graph was.
        build.build_command(target, jobs or 1),
        cwd=settings.repo_dir,
    )
    atomic_json(
        output / "objects.json",
        {
            "schema": "wiz8.analysis-object-root",
            "name": build_name,
            "root": str((output / "CMakeFiles").relative_to(settings.repo_dir)),
        },
    )
    return {
        "target": target,
        "configure": _result(configure_result),
        "build": _result(build_result),
    }


def compare(
    settings: Settings,
    target: str = "WIZ8",
    arguments: list[str] | None = None,
    *,
    build_first: bool = True,
) -> dict[str, Any]:
    if build_first:
        build_target(settings, target)
    validate_build_directory(settings)
    build_dir = settings.repo_dir / "build" / "decomp"
    if (
        not (build_dir / "CMakeCache.txt").is_file()
        or not (build_dir / "reccmp-build.yml").is_file()
    ):
        configure(settings)
    result = run(
        ["reccmp-reccmp", "--target", target, "--no-color", *(arguments or [])],
        cwd=settings.repo_dir / "build" / "decomp",
    )
    return {"target": target, "command": _result(result)}


def build_toolchain(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    from .ghidra.fid_seeds import build_toolchain_images

    return build_toolchain_images(settings, toolchain_ids)


def check(repository: Path) -> dict[str, Any]:
    from .config import load_settings
    from .source_index import write_source_index

    settings = load_settings()
    assert settings is not None
    lint_result = lint(settings)
    source_index = write_source_index(settings)
    write_wiz8_data_source(repository)
    commands = (
        ("format", ["ruff", "format", "--check", "."]),
        ("ruff", ["ruff", "check", "."]),
        ("types", ["pyright"]),
        ("sources", ["cmake", "-P", "cmake/ValidateWiz8Sources.cmake"]),
        ("tests", ["pytest", "tests/unit", "tests/repository"]),
        ("reccmp", ["wiz8", "check-reccmp"]),
    )
    gates = []
    for name, command in commands:
        log = Path("build/logs") / f"check-{name}.json"
        run(command, cwd=repository, log_path=repository / log)
        gates.append({"name": name, "status": "passed", "log": str(log)})
    return {
        "status": "passed",
        "lint": lint_result,
        "source_index": source_index,
        "gates": gates,
    }


def verify(
    settings: Settings,
    *,
    compare_image: bool = True,
    against: Path | None = None,
) -> dict[str, Any]:
    from .reccmp_workflows import compare_vtables
    from .runtime import run_runtime_suite
    from .source_layouts import (
        require_source_layout_delta,
        verify_source_layout_delta,
        verify_source_layouts,
    )
    from .unresolved import require_unresolved_delta, unresolved_report, verify_unresolved_delta

    results: dict[str, Any] = {"schema": "wiz8.verification", "failures": []}

    def gate(name: str, action: Any) -> Any:
        try:
            value = action()
        except Exception as error:  # noqa: BLE001 - every independent gate must still run
            results["failures"].append({"gate": name, "error": f"{type(error).__name__}: {error}"})
            return None
        results[name] = value
        return value

    gate("check", lambda: check(settings.repo_dir))
    wiz8_build = gate("build_wiz8", lambda: build_target(settings, "WIZ8"))
    surrender_build = gate("build_surrender", lambda: build_target(settings, "SURRENDER"))
    runtime_build = gate("build_runtime_test", lambda: build_target(settings, "WIZ8_RUNTIME_TEST"))

    if wiz8_build is not None:
        gate("imports_wiz8", lambda: _check_product_imports(settings.repo_dir, "WIZ8"))
        current_unresolved = gate(
            "current_unresolved",
            lambda: unresolved_report(
                settings.repo_dir / "build/decomp/CMakeFiles/wiz8_recovered_objects.dir",
                settings.repo_dir / "build/decomp/Wiz8.map",
            ),
        )
        if current_unresolved is not None:
            gate(
                "unresolved",
                lambda: require_unresolved_delta(
                    verify_unresolved_delta(settings, current_unresolved)
                ),
            )
        current_source_layouts = gate(
            "current_source_layouts", lambda: verify_source_layouts(settings)
        )
        if current_source_layouts is not None:
            gate(
                "source_layouts",
                lambda: require_source_layout_delta(
                    verify_source_layout_delta(settings, current_source_layouts, against)
                ),
            )

        def vtable_gate() -> dict[str, Any]:
            value = compare_vtables(settings.repo_dir, "WIZ8", None)
            if not value["ok"]:
                raise ValueError("source-owned vtable slots differ from the paired original")
            return value

        gate("vtables", vtable_gate)
        if compare_image:
            gate("compare_wiz8", lambda: compare(settings, "WIZ8", build_first=False))
    if surrender_build is not None and compare_image:
        gate("compare_surrender", lambda: compare(settings, "SURRENDER", build_first=False))
    if runtime_build is not None:
        gate("runtime_tests", lambda: run_runtime_suite(settings))

    report = settings.build_dir / "reports" / "verify.json"
    results["ok"] = not results["failures"]
    results["report"] = str(report)
    atomic_json(report, results)
    if results["failures"]:
        names = ", ".join(item["gate"] for item in results["failures"])
        raise ValueError(f"verification failed at {names}; see {report}")
    return results
