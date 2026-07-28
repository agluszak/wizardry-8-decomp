from __future__ import annotations

import json
import os
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from .build_dir import check_build_directory
from .config import Settings
from .paths import atomic_json, atomic_write
from .subprocesses import CommandResult, resolve_executable, run

VC6_IMAGE = "wizardry8-msvc600:sp5"
TARGET_ALIASES = {"match": "WIZ8_MATCHING", "runtime": "WIZ8_RUNTIME"}


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
        command = [docker, "run", "--rm", "--network", "none"]
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
                "NMake Makefiles",
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


def _result(result: CommandResult) -> dict[str, Any]:
    return asdict(result)


def _owner_path(build_dir: Path) -> Path:
    return build_dir / ".wiz8-build-owner.json"


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
    lines = [f"project: '{settings.repo_dir}'", "targets:"]
    for target, stem in (
        ("WIZ8", "Wiz8"),
        ("SREXT_JPEGIMPORTER", "srEXT_JPEGImporter"),
        ("SREXT_UNZIP", "srEXT_Unzip"),
    ):
        lines.extend(
            (
                f"  {target}:",
                f"    path: '{build_dir / (stem + ('.exe' if target == 'WIZ8' else '.dll'))}'",
                f"    pdb: '{build_dir / (stem + '.pdb')}'",
            )
        )
    atomic_write(build_dir / "reccmp-build.yml", "\n".join(lines) + "\n")


def configure(settings: Settings) -> dict[str, Any]:
    prepare_result = prepare(settings)
    build = ContainerBuild.from_settings(settings)
    build.build_dir.mkdir(parents=True, exist_ok=True)
    (build.build_dir / "tmp").mkdir(parents=True, exist_ok=True)
    validate_build_directory(settings)
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
    fresh = cache.is_file() and "CMAKE_GENERATOR:INTERNAL=NMake Makefiles" not in cache.read_text(
        encoding="utf-8", errors="replace"
    ).replace("\r", "")
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


def build_target(
    settings: Settings, target: str = "match", jobs: int | None = None
) -> dict[str, Any]:
    build = ContainerBuild.from_settings(settings)
    validate_build_directory(settings)
    prerequisites_ready = all(
        mount.host.exists() for mount in build.mounts if mount.container not in {"/repo", "/out"}
    )
    if not prerequisites_ready or not (build.build_dir / "CMakeCache.txt").is_file():
        configure(settings)
    resolved_target = TARGET_ALIASES.get(target, target)
    result = run(
        build.build_command(resolved_target, jobs or max(1, os.cpu_count() or 1)),
        cwd=settings.repo_dir,
    )
    return {"target": resolved_target, "command": _result(result)}


def build_analysis_target(
    settings: Settings, source_dir: str, build_name: str, target: str, jobs: int | None = None
) -> dict[str, Any]:
    output = settings.repo_dir / "build" / "decomp" / "CMakeFiles" / build_name
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
    return {
        "target": target,
        "configure": _result(configure_result),
        "build": _result(build_result),
    }


def compare(
    settings: Settings, target: str = "WIZ8", arguments: list[str] | None = None
) -> dict[str, Any]:
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


def build_toolchain(settings: Settings) -> dict[str, Any]:
    from .ghidra.fid_seeds import build_toolchain_images

    return build_toolchain_images(settings)


def check(repository: Path) -> dict[str, Any]:
    commands = (
        ["ruff", "check", "."],
        ["pyright"],
        ["pytest", "tests/unit"],
        ["wiz8", "evidence", "validate"],
        ["wiz8", "check-markers"],
    )
    return {"commands": [_result(run(command, cwd=repository)) for command in commands]}


def verify(settings: Settings, *, compare_image: bool = True) -> dict[str, Any]:
    build_target(settings, "WIZ8")
    build_target(settings, "WIZ8_GAMEPLAY_BOUNDARIES")
    build_analysis_target(settings, "tools/sgp-oracle", "sgp-oracle", "WIZ8_SGP_PROBES")
    boundaries = run(["wiz8", "verify-boundaries"], cwd=settings.repo_dir)
    comparison = compare(settings, "WIZ8") if compare_image else None
    tests = run(["pytest"], cwd=settings.repo_dir)
    return {
        "boundaries": _result(boundaries),
        "compare": comparison,
        "tests": _result(tests),
    }
