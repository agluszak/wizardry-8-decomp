from __future__ import annotations

import hashlib
import json
import os
import shlex
import subprocess
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
_LINT_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})
_LINT_HEADER_SUFFIXES = frozenset({".h", ".hpp", ".hxx"})
PRODUCT_GENERATOR = "NMake Makefiles"
_PRODUCT_MOUNT_SENTINELS = {
    "/jpeg": "jpeglib.h",
    "/zlib": "zlib.h",
    "/infozip": "unzip.c",
}
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
    makefile = build_dir / "Makefile"
    if not cache.is_file() or not makefile.is_file():
        return False
    content = cache.read_text(encoding="utf-8", errors="replace").replace("\r", "")
    return f"CMAKE_GENERATOR:INTERNAL={PRODUCT_GENERATOR}\n" in content


def prepared_mount_ready(mount: Mount) -> bool:
    """True when a product mount contains its required source tree, not just a directory."""

    if mount.container in {"/repo", "/out"}:
        return mount.host.exists()
    sentinel = _PRODUCT_MOUNT_SENTINELS.get(mount.container)
    if sentinel is None:
        return mount.host.exists()
    return (mount.host / sentinel).is_file()


def _enable_jom_parallelism(build_dir: Path) -> list[str]:
    """Remove only CMake's NMake serialization guards after regeneration.

    CMake runs in the VC6 container and may leave generated files unwritable by
    the host runner. Replace them atomically instead of truncating them in place.
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
            if mount.container not in {"/repo", "/out"} and not prepared_mount_ready(mount)
        ]
        if missing:
            rendered = ", ".join(str(path) for path in missing)
            raise RuntimeError(
                f"prepared build inputs are missing ({rendered}); run `uv run wiz8 prepare`"
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


def clang_configure_inputs(repository: Path) -> tuple[Path, ...]:
    """Return the CMake inputs that shape the Clang compile database."""

    candidates: list[Path] = [repository / "CMakeLists.txt"]
    candidates.extend(sorted((repository / "cmake").glob("*.cmake")))
    candidates.extend(sorted(repository.glob("src/*/CMakeLists.txt")))
    candidates.extend(sorted(repository.glob("src/*/sources.cmake")))
    return tuple(path for path in candidates if path.is_file())


def _clang_configuration_current(repository: Path, output: Path) -> bool:
    required = (
        output / "CMakeCache.txt",
        output / "build.ninja",
        output / "compile_commands.json",
    )
    if any(not path.is_file() for path in required):
        return False
    configured_at = (output / "compile_commands.json").stat().st_mtime_ns
    return not any(
        path.stat().st_mtime_ns > configured_at for path in clang_configure_inputs(repository)
    )


def configure_clang(
    settings: Settings, *, full_diagnostics: bool = False, force: bool = False
) -> tuple[Path, list[str]]:
    """Configure the compiler-backed source projection when stale or missing."""
    output = settings.repo_dir / (DIAGNOSTICS_BUILD_DIR if full_diagnostics else LINT_BUILD_DIR)
    output.mkdir(parents=True, exist_ok=True)
    prefix = clang_container_prefix(settings, output)
    if not force and _clang_configuration_current(settings.repo_dir, output):
        return output, prefix

    configure_command = [
        *prefix,
        "--entrypoint",
        "cmake",
        VC6_IMAGE,
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
    return output, prefix


def clang_container_prefix(settings: Settings, output: Path) -> list[str]:
    """Return the analysis-image invocation for an existing Clang build tree."""

    docker = resolve_executable("docker") or "docker"
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

    command = [docker, "run", "--rm", "--init", "--network", "none"]
    for mount in mounts:
        command.extend(("--volume", mount.docker_argument()))
    return command


def _repository_relative(repository: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(repository.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def _full_lint_change(repository: Path, path: Path) -> bool:
    relative = _repository_relative(repository, path)
    if relative in {".clang-tidy", "CMakeLists.txt", "reccmp-project.yml"}:
        return True
    if relative.startswith(("docker/msvc600/", "tools/lint/")):
        return True
    if relative.startswith("cmake/") and relative.endswith(".cmake"):
        return True
    if relative.startswith("src/") and Path(relative).name in {"CMakeLists.txt", "sources.cmake"}:
        return True
    return relative.startswith("src/sgp/") and path.suffix.casefold() in _LINT_HEADER_SUFFIXES


def lint_required(repository: Path, changed_paths: list[Path]) -> bool:
    """Whether a PR diff needs the compiler-backed lint lane."""

    return any(
        path.suffix.casefold() in _LINT_SOURCE_SUFFIXES or _full_lint_change(repository, path)
        for path in changed_paths
    )


def _lint_selection(
    settings: Settings, changed_paths: list[Path]
) -> tuple[list[Path] | None, list[Path], list[Path]]:
    from .clang_tidy_lines import FILTER_ENV
    from .comparison import header_dependent_files
    from .source_index import indexed_targets, source_index_freshness, write_source_index

    repository = settings.repo_dir
    changed = [
        path
        for path in changed_paths
        if path.suffix.casefold() in _LINT_SOURCE_SUFFIXES and path.is_file()
    ]
    deleted_source = any(
        path.suffix.casefold() in _LINT_SOURCE_SUFFIXES and not path.is_file()
        for path in changed_paths
    )
    if (
        os.environ.get(FILTER_ENV) == "*"
        or deleted_source
        or any(_full_lint_change(repository, path) for path in changed_paths)
    ):
        return None, changed, []

    headers = [path for path in changed if path.suffix.casefold() in _LINT_HEADER_SUFFIXES]
    dependent: set[Path] = set()
    if headers:
        database = repository / LINT_BUILD_DIR / "compile_commands.json"
        targets = list(indexed_targets(repository, database))
        if any(
            source_index_freshness(repository, target)["state"] in {"missing", "invalid", "stale"}
            for target in targets
        ):
            write_source_index(settings)
        for target in targets:
            dependent.update(header_dependent_files(settings, target, headers))
    selected = set(changed) | dependent
    return sorted(selected), changed, sorted(dependent - set(changed))


def _lint_compile_files(
    output: Path, repository: Path, selected: list[Path] | None
) -> tuple[list[str], list[str]]:
    from .source_index import indexed_targets

    database_path = output / "compile_commands.json"
    database = json.loads(database_path.read_text(encoding="utf-8"))
    roots = {
        root.rstrip("/")
        for source_roots in indexed_targets(repository, database_path).values()
        for root in source_roots
    }
    selected_relative = (
        None if selected is None else {_repository_relative(repository, path) for path in selected}
    )
    recovered: set[str] = set()
    vendor: set[str] = set()
    for entry in database:
        raw = str(entry.get("file") or "")
        relative = compile_database_relative(raw, repository)
        if relative is None:
            continue
        if selected_relative is not None and relative not in selected_relative:
            continue
        if relative.startswith("src/sgp/"):
            vendor.add(raw)
            continue
        if any(relative == root or relative.startswith(root + "/") for root in roots):
            recovered.add(raw)
    return sorted(recovered), sorted(vendor)


def _docker_image_id() -> str:
    docker = resolve_executable("docker") or "docker"
    result = subprocess.run(
        [docker, "image", "inspect", "--format={{.Id}}", VC6_IMAGE],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.stdout.strip() if result.returncode == 0 and result.stdout.strip() else VC6_IMAGE


def _lint_cache_digest(
    output: Path,
    repository: Path,
    image_id: str,
    cast_lines: str,
    recovered: list[str],
    vendor: list[str],
    inputs: set[Path],
) -> str:
    digest = hashlib.sha256(b"wiz8-clang-lint-v1\0")
    digest.update(image_id.encode() + b"\0")
    digest.update(cast_lines.encode() + b"\0")
    digest.update((output / "compile_commands.json").read_bytes())
    digest.update((repository / ".clang-tidy").read_bytes())
    for profile, files in (("recovered", recovered), ("vendor", vendor)):
        for filename in files:
            digest.update(profile.encode() + b"\0" + filename.encode() + b"\0")
    for path in sorted(inputs, key=lambda item: _repository_relative(repository, item)):
        relative = _repository_relative(repository, path)
        digest.update(relative.encode() + b"\0")
        if path.is_file():
            digest.update(path.read_bytes())
        else:
            digest.update(b"<missing>")
        digest.update(b"\0")
    return digest.hexdigest()


def _lint_cache_inputs(
    repository: Path,
    selected: list[Path] | None,
    changed_paths: list[Path],
    recovered: list[str],
    vendor: list[str],
) -> set[Path]:
    if selected is not None:
        return set(selected)

    inputs = {
        path
        for path in changed_paths
        if path.suffix.casefold() in _LINT_SOURCE_SUFFIXES or _full_lint_change(repository, path)
    }
    for filename in (*recovered, *vendor):
        relative = compile_database_relative(filename, repository)
        if relative is not None:
            inputs.add(repository / relative)
    return inputs


def _lint_file_names(repository: Path, files: list[str]) -> list[str]:
    names = []
    for filename in files:
        names.append(compile_database_relative(filename, repository) or filename)
    return sorted(names)


def _write_lint_selection_log(repository: Path, result: dict[str, Any]) -> Path:
    path = repository / "build/logs/clang-tidy-selection.json"
    atomic_write(path, (json.dumps(result, indent=2, sort_keys=True) + "\n").encode())
    return path


def run_clang_tidy(
    prefix: list[str],
    output: Path,
    repository: Path,
    recovered: list[str],
    vendor: list[str],
    cast_lines: str,
) -> None:
    """Compile and tidy the selected first-party translation units in one container."""
    from .clang_tidy_lines import FILTER_ENV

    commands: list[str] = []
    if recovered:
        commands.append(
            shlex.join(
                [
                    "clang-tidy",
                    "--quiet",
                    "-p",
                    "/out",
                    "--config-file",
                    "/repo/.clang-tidy",
                    *recovered,
                ]
            )
        )
    if vendor:
        # Retained SGP C participates only in compiler diagnostics; the
        # reconstruction-specific clang-tidy checks deliberately exclude it.
        # LLVM 19 refuses Checks:'-*' without --allow-no-checks.
        commands.append(
            shlex.join(
                [
                    "clang-tidy",
                    "--quiet",
                    "--allow-no-checks",
                    "-p",
                    "/out",
                    "--config={Checks: '-*'}",
                    *vendor,
                ]
            )
        )
    if not commands:
        return
    run(
        [
            *prefix,
            "-e",
            f"{FILTER_ENV}={cast_lines}",
            "--entrypoint",
            "bash",
            VC6_IMAGE,
            "-lc",
            "set -e\n" + "\n".join(commands),
        ],
        cwd=output,
        log_path=output.parent / "logs" / "clang-tidy.json",
    )


def lint(
    settings: Settings,
    *,
    full_diagnostics: bool = False,
    since: str | None = None,
    changed_paths: list[Path] | None = None,
) -> dict[str, Any]:
    """Compile changed C/C++ with structural or full recovery diagnostics."""

    if full_diagnostics:
        output, prefix = configure_clang(settings, full_diagnostics=True)
        run(
            [
                *prefix,
                "--entrypoint",
                "cmake",
                VC6_IMAGE,
                "--build",
                "/out",
                "--target",
                "WIZ8_CLANG_DIAGNOSTICS",
                "--",
                "-k",
                "0",
            ],
            cwd=settings.repo_dir,
            log_path=settings.repo_dir / "build/logs/clang-full-diagnostics.json",
        )
        return {
            "status": "ok",
            "mode": "full-diagnostics",
            "log": str(Path("build/logs/clang-full-diagnostics.json")),
        }

    from .clang_tidy_lines import redundant_cast_line_filter
    from .comparison import changed_files

    repository = settings.repo_dir
    changes = list(changed_paths) if changed_paths is not None else changed_files(repository, since)
    output = repository / LINT_BUILD_DIR
    configured = _clang_configuration_current(repository, output)
    output, prefix = configure_clang(settings)
    selected, changed, dependent = _lint_selection(settings, changes)
    recovered, vendor = _lint_compile_files(output, repository, selected)
    cast_lines = redundant_cast_line_filter(repository)
    image_id = _docker_image_id()
    inputs = _lint_cache_inputs(repository, selected, changes, recovered, vendor)
    digest = _lint_cache_digest(output, repository, image_id, cast_lines, recovered, vendor, inputs)
    stamp = output / ".lint-success.sha256"
    cached = stamp.is_file() and stamp.read_text(encoding="utf-8").strip() == digest
    scope = "full" if selected is None else "changed"
    files = _lint_file_names(repository, recovered + vendor)
    docker_runs = 0 if configured else 1
    if not cached and files:
        run_clang_tidy(prefix, output, repository, recovered, vendor, cast_lines)
        docker_runs += 1
    if not cached:
        atomic_write(stamp, (digest + "\n").encode())

    log_result = {
        "status": "cached" if cached else "passed",
        "scope": scope,
        "cached": cached,
        "docker_runs": docker_runs,
        "file_count": len(files),
        "recovered_file_count": len(recovered),
        "vendor_file_count": len(vendor),
        "changed_file_count": len(changed),
        "dependent_file_count": len(dependent),
        "files": files,
    }
    log = _write_lint_selection_log(repository, log_result)
    return {
        "status": "ok",
        "mode": "gating",
        "scope": scope,
        "cached": cached,
        "files": len(files),
        "docker_runs": docker_runs,
        "log": str(log.relative_to(repository)),
    }


def build_toolchain(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    from .ghidra.fid_seeds import build_toolchain_images

    return build_toolchain_images(settings, toolchain_ids)


def check(repository: Path) -> dict[str, Any]:
    """Fast public validation: cheap host gates before compiler-backed indexing."""

    from .cast_lint import validate_cast_markers
    from .global_model import validate_type_consistency
    from .header_architecture import validate_header_architecture
    from .identity_lint import validate_identity
    from .linkage_lint import validate_c_linkage
    from .placement import validate_source_placement
    from .reccmp_lint import validate_reccmp_annotations
    from .source_index import write_source_index
    from .source_oracle import validate_source_oracle_ownership
    from .source_units import validate_source_units
    from .structural_lint import validate_structures

    settings = load_settings()
    assert settings is not None
    cheap_commands = (
        ("format", ["ruff", "format", "--check", "."]),
        ("ruff", ["ruff", "check", "."]),
        ("types", ["pyright"]),
    )
    gates: list[dict[str, str]] = []
    for name, command in cheap_commands:
        log = Path("build/logs") / f"check-{name}.json"
        run(command, cwd=repository, log_path=repository / log)
        gates.append({"name": name, "status": "passed", "log": str(log)})

    # The repository suite and later comparisons read this projection; its
    # writer also validates synthetic markers and cross-TU declarations.
    source_index = write_source_index(settings)
    validators = (
        ("source-units", lambda: validate_source_units(repository)),
        ("header-architecture", lambda: validate_header_architecture(repository)),
        ("type-consistency", lambda: validate_type_consistency(repository)),
        ("reccmp", lambda: validate_reccmp_annotations(repository)),
        ("casts", lambda: validate_cast_markers(repository)),
        ("c-linkage", lambda: validate_c_linkage(repository)),
        ("placement", lambda: validate_source_placement(settings)),
        ("identities", lambda: validate_identity(repository)),
        ("source-oracle", lambda: validate_source_oracle_ownership(repository)),
        ("structures", lambda: validate_structures(repository)),
    )
    for name, action in validators:
        action()
        gates.append({"name": name, "status": "passed"})

    tests_log = Path("build/logs/check-tests.json")
    run(
        ["pytest", "tests/unit", "tests/repository"],
        cwd=repository,
        log_path=repository / tests_log,
    )
    gates.append({"name": "tests", "status": "passed", "log": str(tests_log)})
    return {
        "status": "passed",
        "source_index": source_index["path"],
        "source_index_cached": bool(source_index.get("cached")),
        "gates": gates,
    }
