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
from .paths import compile_database_relative
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
PRODUCT_GENERATOR = "NMake Makefiles"


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
            r"C:\cmake\bin\cmake.exe",
            "-E",
            "env",
            r"TEMP=Z:\out\tmp",
            r"TMP=Z:\out\tmp",
            r"C:\cmake\bin\cmake.exe",
            "--build",
            "Z:/out",
            "--target",
            target,
            "--parallel",
            str(jobs),
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

    manifest = load_manifest(settings)
    extraction = extract_role(settings, "gog-media")
    variants = materialize_variants(settings, only=["gog-base"])
    sources = fetch_seed_sources(settings)
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
        if not (build.build_dir / "CMakeCache.txt").is_file():
            _configure(settings)
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
        Mount(
            settings.work_dir / "fid/sources/unpacked/ijg-jpeg-6/jpeg-6",
            "/jpeg",
        ),
        Mount(
            settings.work_dir / "fid/sources/unpacked/infozip-unzip-5.4",
            "/infozip",
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
        "-DIJG_JPEG_SOURCE=/jpeg",
        "-DZLIB_SOURCE=/zlib",
        "-DINFOZIP_SOURCE=/infozip",
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


def run_clang_tidy(prefix: list[str], output: Path, repository: Path) -> None:
    """Gate first-party code with the narrow reconstruction-error profile.

    Only translation units under a reccmp source root are tidied: the compile
    database also covers the pristine zlib/Info-ZIP static libraries, which
    keep their upstream warnings by policy, and the retained SGP C library,
    whose C idioms are outside the reconstruction-error profile.
    """
    from .source_index import indexed_targets

    database = json.loads((output / "compile_commands.json").read_text(encoding="utf-8"))
    roots = {
        root.rstrip("/")
        for source_roots in indexed_targets(repository).values()
        for root in source_roots
    }

    def first_party(path: str) -> bool:
        relative = compile_database_relative(path, repository)
        if relative is None or relative.startswith("src/sgp/"):
            return False
        return any(relative == root or relative.startswith(root + "/") for root in roots)

    files = sorted({entry["file"] for entry in database if first_party(entry["file"])})
    if not files:
        raise RuntimeError("clang-tidy: compile database has no first-party sources")
    run(
        [
            *prefix,
            "--entrypoint",
            "clang-tidy",
            VC6_IMAGE,
            "--quiet",
            "-p",
            "/out",
            "--config-file",
            "/repo/.clang-tidy",
            *files,
        ],
        cwd=output,
        log_path=output.parent / "logs" / "clang-tidy.json",
    )


def lint(settings: Settings, *, full_diagnostics: bool = False) -> dict[str, Any]:
    """Compile recovered C++ with structural or full recovery diagnostics."""

    output, prefix = configure_clang(settings, full_diagnostics=full_diagnostics)
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
    if not full_diagnostics:
        run_clang_tidy(prefix, output, settings.repo_dir)
    return {
        "status": "ok",
        "mode": "full-diagnostics" if full_diagnostics else "gating",
        "log": str(
            Path("build/logs")
            / ("clang-full-diagnostics.json" if full_diagnostics else "clang-lint-build.json")
        ),
    }


def build_toolchain(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    from .ghidra.fid_seeds import build_toolchain_images

    return build_toolchain_images(settings, toolchain_ids)


def check(repository: Path) -> dict[str, Any]:
    """Fast public validation: Python/repository gates, no compiler lane."""

    from .cast_lint import validate_cast_markers
    from .global_model import validate_type_consistency
    from .identity_lint import validate_identity
    from .linkage_lint import validate_c_linkage
    from .placement import validate_source_placement
    from .reccmp_lint import validate_reccmp_annotations
    from .source_index import write_source_index
    from .source_units import validate_source_units
    from .structural_lint import validate_structures

    settings = load_settings()
    assert settings is not None
    # The repository suite and later comparisons read this projection; its
    # writer also validates synthetic markers and cross-TU declarations.
    source_index = write_source_index(settings)
    validators = (
        ("source-units", lambda: validate_source_units(repository)),
        ("type-consistency", lambda: validate_type_consistency(repository)),
        ("reccmp", lambda: validate_reccmp_annotations(repository)),
        ("casts", lambda: validate_cast_markers(repository)),
        ("c-linkage", lambda: validate_c_linkage(repository)),
        ("placement", lambda: validate_source_placement(settings)),
        ("identities", lambda: validate_identity(repository)),
        ("structures", lambda: validate_structures(repository)),
    )
    commands = (
        ("format", ["ruff", "format", "--check", "."]),
        ("ruff", ["ruff", "check", "."]),
        ("types", ["pyright"]),
        ("tests", ["pytest", "tests/unit", "tests/repository"]),
    )
    gates: list[dict[str, str]] = []
    for name, action in validators:
        action()
        gates.append({"name": name, "status": "passed"})
    for name, command in commands:
        log = Path("build/logs") / f"check-{name}.json"
        run(command, cwd=repository, log_path=repository / log)
        gates.append({"name": name, "status": "passed", "log": str(log)})
    return {
        "status": "passed",
        "source_index": source_index["path"],
        "gates": gates,
    }
