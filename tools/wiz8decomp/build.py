from __future__ import annotations

import json
import os
import sys
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import UTC, datetime
from fcntl import LOCK_EX, LOCK_NB, LOCK_UN, flock
from pathlib import Path
from typing import Any

from .binary.coff_archive import named_iat_archive
from .config import Settings, load_settings
from .paths import atomic_write, compile_database_relative
from .reccmp_data import write_wiz8_data_source
from .subprocesses import resolve_executable, run

VC6_IMAGE = "wizardry8-msvc600:sp5"
LINT_BUILD_DIR = "build/clang"
DIAGNOSTICS_BUILD_DIR = "build/clang-diagnostics"
TARGET_ALIASES = {
    "match": "WIZ8",
    "runtime": "WIZ8_RUNTIME",
    "runtime-test": "WIZ8_RUNTIME_TEST",
}
_PRODUCT_OUTPUTS = {
    "WIZ8": "Wiz8",
    "WIZ8_RUNTIME": "Wiz8Runtime",
    "WIZ8_RUNTIME_TEST": "Wiz8RuntimeTest",
}
_PRODUCT_INPUT_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx", ".inc", ".rc", ".def", ".asm"}
)
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
    """One pinned-VC6 product build over the checkout's prepared inputs."""

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
                Mount(settings.product_build_dir, "/out", read_only=False),
            ),
            source_dir=settings.repo_dir,
            build_dir=settings.product_build_dir,
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

    def configure_command(self) -> list[str]:
        command = self.docker_prefix()
        command.extend(
            (
                r"C:\cmake\bin\cmake.exe",
                "-S",
                self.container_source_dir,
                "-B",
                "Z:/out",
                "-G",
                PRODUCT_GENERATOR,
                "-DIJG_JPEG_SOURCE=Z:/jpeg",
                "-DZLIB_SOURCE=Z:/zlib",
                "-DINFOZIP_SOURCE=Z:/infozip",
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
                rf"cd /d Z:\out&& {JOM_PROGRAM} -j {jobs} {target}"
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


@contextmanager
def build_lock(settings: Settings):
    """Fail fast when another command owns this checkout's product build."""

    build_dir = settings.product_build_dir
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
    """Materialize the primary game, its variant, and pinned source dependencies.

    Optional corpus inputs stay behind explicit `wiz8 corpus` operations; this
    path only builds what ordinary development requires.
    """

    from .extract.variants import extract_role, materialize_variants
    from .ghidra.fid_seeds import fetch_seed_sources
    from .inputs.scan import load_manifest
    from .source_index import write_source_index

    manifest = load_manifest(settings)
    extraction = extract_role(settings, "gog-media")
    variants = materialize_variants(settings, only=["gog-base"])
    sources = fetch_seed_sources(settings)
    write_source_index(settings)
    write_wiz8_data_source(settings.repo_dir)
    run(
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
        log_path=settings.build_dir / "logs" / "reccmp-detect.json",
    )
    return {
        "status": "ok",
        "manifest_files": len(manifest.files),
        "extraction": extraction["role"],
        "variants": [row["variant"] for row in variants["variants"]],
        "sources": {
            "ready": sum(row["status"] == "ready" for row in sources["sources"]),
            "skipped": sum(row["status"] != "ready" for row in sources["sources"]),
        },
        "detect": "ok",
    }


def _ensure_sr_assert_import(settings: Settings) -> Path:
    """The srAssertFail consumer descriptor the canonical product links."""

    path = settings.product_build_dir / "sr-assert-import.lib"
    payload = named_iat_archive(
        "SR.dll", "?srAssertFail@@YAXPBD0J0@Z", "?srAssertFail@@YAXPBD0J0ZZ"
    )
    if not path.is_file() or path.read_bytes() != payload:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)
    return path


def _configure(settings: Settings) -> None:
    build = ContainerBuild.from_settings(settings)
    build.build_dir.mkdir(parents=True, exist_ok=True)
    (build.build_dir / "tmp").mkdir(parents=True, exist_ok=True)
    run(
        build.configure_command(),
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "product-configure.json",
    )


def _product_cache_ready(build_dir: Path) -> bool:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return False
    content = cache.read_text(encoding="utf-8", errors="replace").replace("\r", "")
    return f"CMAKE_GENERATOR:INTERNAL={PRODUCT_GENERATOR}\n" in content


def _enable_jom_parallelism(build_dir: Path) -> list[str]:
    """Remove only CMake's NMake serialization guards after regeneration.

    CMake runs in the VC6 container and may leave generated files owned by the
    container user. Replacing the file atomically only needs write permission on
    the host-owned build directory, unlike truncating the generated file in
    place.
    """

    updated: list[str] = []
    for path in (build_dir / "Makefile", build_dir / "CMakeFiles/Makefile2"):
        content = path.read_bytes()
        replacement = content.replace(b".NOTPARALLEL:\r\n", b"# .NOTPARALLEL removed for JOM\r\n")
        replacement = replacement.replace(b".NOTPARALLEL:\n", b"# .NOTPARALLEL removed for JOM\n")
        if replacement != content:
            atomic_write(path, replacement)
            updated.append(str(path))
    return updated


def require_product(settings: Settings, target: str) -> tuple[Path, Path]:
    """Require an existing executable/PDB pair without repairing it."""

    resolved = TARGET_ALIASES.get(target, target)
    stem = _PRODUCT_OUTPUTS[resolved]
    artifacts = (
        settings.product_build_dir / f"{stem}.exe",
        settings.product_build_dir / f"{stem}.pdb",
    )
    if any(not path.is_file() for path in artifacts):
        raise FileNotFoundError(
            f"{stem} build artifacts are missing; run `uv run wiz8 build {target}`"
        )
    return artifacts


def warn_if_product_may_be_stale(settings: Settings, target: str) -> None:
    """Warn when a runnable product predates relevant checked-in inputs."""

    import logging

    artifacts = require_product(settings, target)
    built_at = min(path.stat().st_mtime_ns for path in artifacts)
    repository = settings.repo_dir
    candidates = [repository / "CMakeLists.txt"]
    candidates.extend((repository / "cmake").rglob("*.cmake"))
    candidates.extend(repository.glob("src/*/CMakeLists.txt"))
    candidates.extend(repository.glob("src/*/sources.cmake"))
    for root in ("src/wiz8", "src/sgp", "include/wiz8"):
        candidates.extend(
            path
            for path in (repository / root).rglob("*")
            if path.suffix.lower() in _PRODUCT_INPUT_SUFFIXES
        )
    if TARGET_ALIASES.get(target, target) == "WIZ8_RUNTIME_TEST":
        candidates.extend(
            path
            for path in (repository / "tests/runtime").rglob("*")
            if path.suffix.lower() in _PRODUCT_INPUT_SUFFIXES
        )
    if any(path.is_file() and path.stat().st_mtime_ns > built_at for path in candidates):
        logging.getLogger(__name__).warning(
            "runtime build may be stale; relevant inputs are newer than the current build\n"
            "         run `uv run wiz8 build %s` for fresh runtime results",
            target,
        )


def build_target(
    settings: Settings, target: str = "match", jobs: int | None = None
) -> dict[str, Any]:
    with build_lock(settings):
        build = ContainerBuild.from_settings(settings)
        resolved_target = TARGET_ALIASES.get(target, target)
        missing = [
            mount.host
            for mount in build.mounts
            if mount.container not in {"/repo", "/out"} and not mount.host.exists()
        ]
        if missing:
            rendered = ", ".join(str(path) for path in missing)
            raise RuntimeError(
                f"prepared build inputs are missing ({rendered}); run `wiz8 prepare`"
            )
        _ensure_sr_assert_import(settings)
        if not _product_cache_ready(build.build_dir):
            _configure(settings)
        run(
            build.check_build_system_command(),
            cwd=settings.repo_dir,
            log_path=settings.repo_dir / "build/logs/product-regenerate.json",
        )
        _enable_jom_parallelism(build.build_dir)
        if resolved_target in {"WIZ8_RUNTIME", "WIZ8_RUNTIME_TEST"}:
            # The runnable products must link without /FORCE:UNRESOLVED, so the
            # comparison MAP and the generated trap thunks must be current
            # before their link runs. The comparison product keeps /FORCE.
            run(
                build.build_command("WIZ8", jobs or max(1, os.cpu_count() or 1)),
                cwd=settings.repo_dir,
                log_path=settings.repo_dir / "build" / "logs" / "runtime-prereq.json",
            )
            from .runtime_stubs import write_runtime_stubs
            from .source_index import write_source_index

            write_source_index(settings)
            write_runtime_stubs(settings)
        run(
            build.build_command(resolved_target, jobs or max(1, os.cpu_count() or 1)),
            cwd=settings.repo_dir,
            log_path=settings.repo_dir / "build" / "logs" / "product-build.json",
        )
        return {
            "status": "ok",
            "target": resolved_target,
            "log": str(Path("build/logs/product-build.json")),
        }


def configure_clang(
    settings: Settings, *, full_diagnostics: bool = False, force: bool = False
) -> tuple[Path, list[str]]:
    """Configure the compiler-backed source projection when it is missing."""
    output = settings.repo_dir / (DIAGNOSTICS_BUILD_DIR if full_diagnostics else LINT_BUILD_DIR)
    output.mkdir(parents=True, exist_ok=True)
    prefix = clang_container_prefix(settings, output)
    if not force and (output / "CMakeCache.txt").is_file() and (output / "build.ninja").is_file():
        return output, prefix

    configure_command = [
        *prefix,
        "cmake",
        "-S",
        "/repo",
        "-B",
        "/out",
        "-G",
        "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DWIZ8_BUILD_MODE=lint",
    ]
    run(
        configure_command,
        cwd=settings.repo_dir,
        log_path=settings.repo_dir
        / "build/logs"
        / ("clang-diagnostics-configure.json" if full_diagnostics else "clang-configure.json"),
    )
    return output, prefix


def _clang_target(settings: Settings, target: str, *, full_diagnostics: bool = False) -> dict[str, Any]:
    output, prefix = configure_clang(settings, full_diagnostics=full_diagnostics)
    log_name = "clang-diagnostics-build.json" if full_diagnostics else "clang-lint-build.json"
    result = run(
        [*prefix, "cmake", "--build", "/out", "--target", target, "--", "-j2"],
        cwd=settings.repo_dir,
        log_path=settings.repo_dir / "build/logs" / log_name,
    )
    return {
        "status": "ok",
        "mode": "diagnostics" if full_diagnostics else "gating",
        "log": str(Path("build/logs") / log_name),
    }


def _compile_database(settings: Settings) -> list[dict[str, Any]]:
    database = settings.repo_dir / LINT_BUILD_DIR / "compile_commands.json"
    if not database.is_file():
        configure_clang(settings)
    return json.loads(database.read_text(encoding="utf-8"))


def _clang_sources(settings: Settings, target: str) -> list[str]:
    from .source_index import project_targets

    config = project_targets(settings.repo_dir)[target]
    roots = config.get("source-root", ())
    if isinstance(roots, str):
        roots = (roots,)
    owned = {
        path.as_posix()
        for root in roots
        for path in (settings.repo_dir / root).rglob("*")
        if path.suffix.lower() in _PRODUCT_INPUT_SUFFIXES
    }
    return sorted(owned)


def _compile_database_files(settings: Settings, target: str) -> list[str]:
    return [
        relative
        for row in _compile_database(settings)
        if (relative := compile_database_relative(str(row["file"]), settings.repo_dir)) is not None
        and relative in _clang_sources(settings, target)
    ]


def lint(settings: Settings, *, full_diagnostics: bool = False) -> dict[str, Any]:
    """Compile recovered C++ with clang-cl diagnostics and project source gates."""
    output, _ = configure_clang(settings, full_diagnostics=full_diagnostics)
    return _clang_target(settings, "wiz8-clang-lint", full_diagnostics=full_diagnostics)


def build_toolchain(settings: Settings, toolchains: list[str] | None = None) -> dict[str, Any]:
    from .toolchain import build_toolchain_images

    toolchain_ids = toolchains or ["vc6-sp5"]
    return build_toolchain_images(settings, toolchain_ids)


def check(repository: Path) -> dict[str, Any]:
    """Fast public validation: Python/repository gates, no compiler lane."""

    from .cast_lint import validate_cast_markers
    from .c_linkage_lint import validate_c_linkage
    from .header_architecture import validate_header_architecture
    from .identity_lint import validate_identity
    from .linkage_lint import validate_c_linkage as validate_linkage
    from .placement import validate_source_placement
    from .reccmp_lint import validate_reccmp_annotations
    from .source_index import write_source_index
    from .source_units import validate_source_units
    from .structural_lint import validate_structures
    from .type_consistency import validate_types

    gates = [
        ("source-units", validate_source_units),
        ("header-architecture", validate_header_architecture),
        ("type-consistency", validate_types),
        ("reccmp", validate_reccmp_annotations),
        ("casts", validate_cast_markers),
        ("c-linkage", validate_c_linkage),
        ("placement", validate_source_placement),
        ("identities", validate_identity),
        ("structures", validate_structures),
    ]
    source_index = write_source_index(load_settings(repository))
    for name, validate in gates:
        validate(repository)
    logs = repository / "build/logs"
    format_result = run(
        ["uv", "run", "ruff", "format", "--check", "tools", "tests"],
        cwd=repository,
        log_path=logs / "check-format.json",
    )
    ruff_result = run(
        ["uv", "run", "ruff", "check", "tools", "tests"],
        cwd=repository,
        log_path=logs / "check-ruff.json",
    )
    types_result = run(
        ["uv", "run", "pyright", "tools"],
        cwd=repository,
        log_path=logs / "check-types.json",
    )
    tests_result = run(
        ["uv", "run", "pytest", "-q", "tests/unit"],
        cwd=repository,
        log_path=logs / "check-tests.json",
    )
    return {
        "status": "passed",
        "source_index": str(source_index.relative_to(repository)),
        "gates": [
            *({"name": name, "status": "passed"} for name, _ in gates),
            {"name": "format", "status": "passed", "log": str(format_result.log_path.relative_to(repository)) if hasattr(format_result, "log_path") and format_result.log_path else "build/logs/check-format.json"},
            {"name": "ruff", "status": "passed", "log": str(ruff_result.log_path.relative_to(repository)) if hasattr(ruff_result, "log_path") and ruff_result.log_path else "build/logs/check-ruff.json"},
            {"name": "types", "status": "passed", "log": str(types_result.log_path.relative_to(repository)) if hasattr(types_result, "log_path") and types_result.log_path else "build/logs/check-types.json"},
            {"name": "tests", "status": "passed", "log": str(tests_result.log_path.relative_to(repository)) if hasattr(tests_result, "log_path") and tests_result.log_path else "build/logs/check-tests.json"},
        ],
    }
