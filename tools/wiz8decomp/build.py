from __future__ import annotations

import json
import os
import sys
from contextlib import contextmanager
from dataclasses import asdict, dataclass
from datetime import UTC, datetime
from fcntl import LOCK_EX, LOCK_NB, LOCK_UN, flock
from pathlib import Path
from typing import Any

import yaml

from .build_dir import check_build_directory
from .config import Settings
from .paths import atomic_json, atomic_write
from .reccmp_data import write_wiz8_data_source
from .subprocesses import CommandResult, resolve_executable, run

VC6_IMAGE = "wizardry8-msvc600:sp5"
LINT_BUILD_DIR = "build/clang"
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
    return {
        "manifest_files": len(manifest.files),
        "repairs": repairs,
        "extractions": len(extractions),
        "variants": len(variants["variants"]),
        "sources": {
            "ready": sum(row["status"] == "ready" for row in sources["sources"]),
            "skipped": sum(row["status"] != "ready" for row in sources["sources"]),
        },
    }


def _write_reccmp_build(settings: Settings) -> None:
    build_dir = settings.repo_dir / "build" / "decomp"
    targets = {}
    for target, stem in (
        ("WIZ8", "Wiz8"),
        ("SURRENDER", "sr"),
        ("SREXT_JPEGIMPORTER", "srEXT_JPEGImporter"),
        ("SREXT_UNZIP", "srEXT_Unzip"),
    ):
        targets[target] = {
            "path": str(build_dir / (stem + (".exe" if target == "WIZ8" else ".dll"))),
            "pdb": str(build_dir / (stem + ".pdb")),
        }
    atomic_write(
        build_dir / "reccmp-build.yml",
        yaml.safe_dump(
            {"project": str(settings.repo_dir), "targets": targets},
            sort_keys=False,
        ),
    )


def _configure(settings: Settings) -> dict[str, Any]:
    prepare_result = prepare(settings)
    build = ContainerBuild.from_settings(settings)
    build.build_dir.mkdir(parents=True, exist_ok=True)
    (build.build_dir / "tmp").mkdir(parents=True, exist_ok=True)
    validate_build_directory(settings)
    write_wiz8_data_source(settings.repo_dir)
    detect = run(
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
    cache = build.build_dir / "CMakeCache.txt"
    fresh = cache.is_file() and not _product_cache_ready(build.build_dir)
    configured = run(build.configure_command(fresh=fresh), cwd=settings.repo_dir)
    atomic_json(
        _owner_path(build.build_dir),
        {"source_dir": str(settings.repo_dir), "build_dir": str(build.build_dir)},
    )
    _write_reccmp_build(settings)
    return {
        "prepare": prepare_result,
        "detect": _result(detect),
        "configure": _result(configured),
    }


def configure(settings: Settings) -> dict[str, Any]:
    with build_lock(settings):
        return _configure(settings)


def build_target(
    settings: Settings, target: str = "match", jobs: int | None = None
) -> dict[str, Any]:
    with build_lock(settings):
        build = ContainerBuild.from_settings(settings)
        write_wiz8_data_source(settings.repo_dir)
        validate_build_directory(settings)
        prerequisites_ready = all(
            mount.host.exists()
            for mount in build.mounts
            if mount.container not in {"/repo", "/out"}
        )
        if not prerequisites_ready or not _product_cache_ready(build.build_dir):
            _configure(settings)
        # Let CMake regenerate while its NMake serialization guards are intact,
        # then adapt only those generated guards for the parallel JOM build.
        run(build.check_build_system_command(), cwd=settings.repo_dir)
        parallel_makefiles = _enable_jom_parallelism(build.build_dir)
        resolved_target = TARGET_ALIASES.get(target, target)
        result = run(
            build.build_command(resolved_target, jobs or max(1, os.cpu_count() or 1)),
            cwd=settings.repo_dir,
        )
        return {
            "target": resolved_target,
            "parallel_makefiles": parallel_makefiles,
            "command": _result(result),
        }


def lint(settings: Settings) -> dict[str, Any]:
    """Compile recovered C++ with clang-cl and the real VC6 headers."""

    docker = resolve_executable("docker") or "docker"
    output = settings.repo_dir / LINT_BUILD_DIR
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

    configure_result = run(
        [
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
        ],
        cwd=settings.repo_dir,
    )
    build_result = run(
        [
            *prefix(),
            "--entrypoint",
            "cmake",
            VC6_IMAGE,
            "--build",
            "/out",
            "--",
            "-k",
            "0",
        ],
        cwd=settings.repo_dir,
        log_path=settings.repo_dir / "build" / "logs" / "clang-lint-build.json",
    )
    return {"configure": _result(configure_result), "build": _result(build_result)}


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
        ["ruff", "format", "--check", "."],
        ["ruff", "check", "."],
        ["pyright"],
        ["cmake", "-P", "cmake/ValidateWiz8Sources.cmake"],
        ["pytest", "tests/unit", "tests/repository"],
        ["wiz8", "check-reccmp"],
    )
    return {
        "lint": lint_result,
        "source_index": source_index,
        "commands": [_result(run(command, cwd=repository)) for command in commands],
    }


def verify(settings: Settings, *, compare_image: bool = True) -> dict[str, Any]:
    from .ghidra.index import export_index
    from .ghidra.reccmp_import import import_reccmp_source
    from .reccmp_workflows import compare_vtables
    from .runtime import run_runtime_suite
    from .source_index import write_source_index
    from .source_layouts import require_source_layouts, verify_source_layouts

    lint_result = lint(settings)
    source_index = write_source_index(settings)
    build_target(settings, "WIZ8")
    build_target(settings, "SURRENDER")
    build_target(settings, "WIZ8_RUNTIME_TEST")
    build_analysis_target(settings, "tools/sgp-oracle", "sgp-oracle", "WIZ8_SGP_PROBES")
    source_import = import_reccmp_source(settings, "wiz8")
    surrender_source_import = import_reccmp_source(settings, "wiz8--gog-base--sr--")
    ghidra_index = export_index(settings, "wiz8")
    surrender_ghidra_index = export_index(settings, "wiz8--gog-base--sr--")
    source_layouts = require_source_layouts(verify_source_layouts(settings))
    vtables = compare_vtables(settings.repo_dir, "WIZ8", None)
    if not vtables["ok"]:
        raise ValueError("source-owned vtable slots differ from the paired original")
    comparison = (
        {
            "wiz8": compare(settings, "WIZ8", build_first=False),
            "surrender": compare(settings, "SURRENDER", build_first=False),
        }
        if compare_image
        else None
    )
    decomplint = run(["wiz8", "check-reccmp"], cwd=settings.repo_dir)
    tests = run(["pytest"], cwd=settings.repo_dir)
    runtime_tests = run_runtime_suite(settings)
    return {
        "lint": lint_result,
        "source_index": source_index,
        "source_import": source_import,
        "surrender_source_import": surrender_source_import,
        "ghidra_index": ghidra_index,
        "surrender_ghidra_index": surrender_ghidra_index,
        "source_layouts": source_layouts,
        "vtables": vtables,
        "compare": comparison,
        "decomplint": _result(decomplint),
        "tests": _result(tests),
        "runtime_tests": runtime_tests,
    }
