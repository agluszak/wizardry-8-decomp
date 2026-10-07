import importlib
import os
from pathlib import Path

import pytest
from wiz8decomp import build
from wiz8decomp.config import Settings


@pytest.mark.parametrize("failure", [None, "clang-format", "pyright", "pytest"])
def test_check_uses_completed_index_and_propagates_command_failures(
    tmp_path: Path, monkeypatch, failure: str | None
) -> None:
    settings = _settings(tmp_path)
    index = tmp_path / "build/source-index.json"
    monkeypatch.setattr(build, "load_settings", lambda: settings)

    def write_index(_settings):
        index.parent.mkdir(exist_ok=True)
        assert (tmp_path / "build/generated/reccmp/wiz8-emissions.csv").is_file()
        index.write_text("completed projection")
        return {"path": str(index), "cached": False}

    monkeypatch.setattr("wiz8decomp.source_index.write_source_index", write_index)
    validators = {
        "cast_lint": "validate_cast_markers",
        "global_model": "validate_type_consistency",
        "header_architecture": "validate_header_architecture",
        "identity_lint": "validate_identity",
        "linkage_lint": "validate_c_linkage",
        "placement": "validate_source_placement",
        "reccmp_lint": "validate_reccmp_annotations",
        "source_model_lint": "validate_source_model",
        "source_oracle": "validate_source_oracle_ownership",
        "source_units": "validate_source_units",
        "structural_lint": "validate_structures",
        "surrender_exports": "validate_surrender_exports",
        "template_model_lint": "validate_template_model",
    }
    for module, name in validators.items():
        monkeypatch.setattr(importlib.import_module(f"wiz8decomp.{module}"), name, lambda *_: None)

    def run(command, **_kwargs):
        if command[0] == "pytest":
            assert index.read_text() == "completed projection"
        if command[0] == failure:
            raise RuntimeError(f"failed {failure}")

    monkeypatch.setattr(build, "run", run)
    if failure is None:
        assert build.check(tmp_path)["status"] == "passed"
    else:
        with pytest.raises(RuntimeError, match=f"failed {failure}"):
            build.check(tmp_path)


def _settings(repository: Path) -> Settings:
    return Settings.model_construct(
        repo_dir=repository,
        work_dir=repository / "work",
        input_dir=repository / "input",
        ghidra_install_dir=repository / "ghidra",
    )


def test_cpp_format_files_owns_reconstruction_and_excludes_oracles(tmp_path: Path) -> None:
    owned = [
        "include/wiz8/example.h",
        "src/sgp/DEBUG.H",
        "src/wiz8/Combat Attack.cpp",
        "src/srext_unzip/plugin.cpp",
        "tests/runtime/example.cpp",
        "tools/lint/include/Windows.h",
    ]
    excluded = [
        "third_party/sgp/example.c",
        "vendor/example.h",
        "src/sgp/README.md",
        "src/sgp/ddraw.h",
        "src/sgp/Mss.h",
        "src/sgp/ZLIB.H",
        "src/sgp/ZCONF.H",
    ]
    for name in owned + excluded:
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
    assert build.cpp_format_files(tmp_path) == sorted(owned)


def _prepare_sources(settings: Settings) -> None:
    for mount in build.ContainerBuild.from_settings(settings).mounts:
        sentinel = build._SOURCE_MOUNT_SENTINELS.get(mount.container)
        if mount.container != "/out":
            mount.host.mkdir(parents=True, exist_ok=True)
        if sentinel:
            (mount.host / sentinel).write_text("/* prepared source */\n")


def test_product_build_uses_cmake_parallel_jom(tmp_path: Path) -> None:
    command = build.ContainerBuild.from_settings(_settings(tmp_path)).build_command("WIZ8", 7)
    assert command[-7:] == [
        build.CMAKE_PROGRAM,
        "--build",
        "Z:/out",
        "--target",
        "WIZ8",
        "--parallel",
        "7",
    ]


@pytest.mark.parametrize("target", ["runtime", "runtime-test"])
def test_runtime_build_needs_only_its_cmake_target(
    tmp_path: Path, monkeypatch, target: str
) -> None:
    settings = _settings(tmp_path)
    _prepare_sources(settings)
    output = settings.product_build_dir
    output.mkdir(parents=True)
    (output / "Makefile").write_text("all:\n")
    (output / "CMakeCache.txt").write_text(
        f"CMAKE_GENERATOR:INTERNAL={build.PRODUCT_GENERATOR}\nCMAKE_BUILD_TYPE:STRING=\n"
    )
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_: commands.append(command))
    monkeypatch.setattr(
        "wiz8decomp.source_index.write_source_index",
        lambda *_: pytest.fail("runtime link must not index sources"),
    )
    result = build.build_target(settings, target, jobs=2)
    assert result["target"] == build.TARGET_ALIASES[target]
    assert len(commands) == 1
    assert commands[0][-4:] == ["--target", build.TARGET_ALIASES[target], "--parallel", "2"]


def test_clang_configuration_reuses_existing_ninja_tree(tmp_path: Path, monkeypatch) -> None:
    output = tmp_path / build.LINT_BUILD_DIR
    output.mkdir(parents=True)
    (output / "CMakeCache.txt").write_text("configured")
    (output / "build.ninja").write_text("ninja")
    (output / "compile_commands.json").write_text("[]")
    monkeypatch.setattr(
        build, "run", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError)
    )

    _prepare_sources(_settings(tmp_path))
    actual, prefix = build.configure_clang(_settings(tmp_path))

    assert actual == output
    assert "--volume" in prefix


def test_clang_configuration_reruns_when_inputs_are_newer(tmp_path: Path, monkeypatch) -> None:
    output = tmp_path / build.LINT_BUILD_DIR
    output.mkdir(parents=True)
    (output / "CMakeCache.txt").write_text("configured")
    (output / "build.ninja").write_text("ninja")
    (output / "compile_commands.json").write_text("[]")
    inventory = tmp_path / "CMakeLists.txt"
    inventory.write_text("project(wiz8)\n")
    os.utime(output / "compile_commands.json", ns=(1_000_000_000, 1_000_000_000))
    os.utime(inventory, ns=(2_000_000_000, 2_000_000_000))
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_kwargs: commands.append(command))

    _prepare_sources(_settings(tmp_path))
    build.configure_clang(_settings(tmp_path))

    assert len(commands) == 1


def test_forced_clang_configuration_is_incremental_not_fresh(tmp_path: Path, monkeypatch) -> None:
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_kwargs: commands.append(command))

    _prepare_sources(_settings(tmp_path))
    build.configure_clang(_settings(tmp_path), force=True)

    assert len(commands) == 1
    assert "--fresh" not in commands[0]


def test_x64_configuration_is_isolated_from_lint_projection(tmp_path: Path, monkeypatch):
    settings = _settings(tmp_path)
    _prepare_sources(settings)
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_: commands.append(command))
    output, _ = build.configure_clang(settings, full_diagnostics=True, x64=True)
    assert output == tmp_path / build.X64_DIAGNOSTICS_BUILD_DIR
    assert "-DCMAKE_TOOLCHAIN_FILE=/repo/cmake/clang-cl-x86_64.cmake" in commands[0]
    assert "-DWIZ8_FULL_DIAGNOSTICS=ON" in commands[0]
    with pytest.raises(ValueError, match="not a source-index"):
        build.configure_clang(settings, x64=True)


def test_x64_diagnostics_separate_header_blockers_from_source_contracts():
    result = build.x64_diagnostic_summary(
        "/opt/msvc6-vc98-include/winnt.h(630,2): error: Must define a target architecture.\n"
        "/repo/include/surrender/srPtr.h(91,1): error: static assertion failed: size\n",
        "",
    )
    assert result["toolchain_blocked"]
    assert result["source_errors"] == 1
    assert result["toolchain_errors"] == 1


def test_missing_runtime_product_names_the_explicit_build(tmp_path: Path) -> None:
    with pytest.raises(FileNotFoundError, match=r"uv run wiz8 build runtime-test"):
        build.require_product(_settings(tmp_path), "runtime-test")


@pytest.mark.parametrize("source_newer, warns", [(True, True), (False, False)])
def test_runtime_product_freshness_uses_input_mtimes(
    tmp_path: Path, caplog, source_newer: bool, warns: bool
) -> None:
    settings = _settings(tmp_path)
    source = tmp_path / "src/wiz8/unit.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("void f() {}")
    settings.product_build_dir.mkdir(parents=True)
    executable = settings.product_build_dir / "Wiz8Runtime.exe"
    pdb = settings.product_build_dir / "Wiz8Runtime.pdb"
    executable.write_bytes(b"exe")
    pdb.write_bytes(b"pdb")
    old, new = 1_000_000_000, 2_000_000_000
    source_time, artifact_time = (new, old) if source_newer else (old, new)
    os.utime(source, ns=(source_time, source_time))
    os.utime(executable, ns=(artifact_time, artifact_time))
    os.utime(pdb, ns=(artifact_time, artifact_time))

    build.warn_if_product_may_be_stale(settings, "runtime")

    assert ("runtime build may be stale" in caplog.text) is warns


def test_product_cache_without_makefile_is_not_ready(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(
        f"CMAKE_GENERATOR:INTERNAL={build.PRODUCT_GENERATOR}\nCMAKE_BUILD_TYPE:STRING=\n"
    )
    assert build._product_cache_ready(tmp_path) is False
    (tmp_path / "Makefile").write_text("all:\n")
    assert build._product_cache_ready(tmp_path) is True


def test_product_cache_with_old_build_type_requires_reconfigure(tmp_path: Path) -> None:
    (tmp_path / "CMakeCache.txt").write_text(
        f"CMAKE_GENERATOR:INTERNAL={build.PRODUCT_GENERATOR}\nCMAKE_BUILD_TYPE:STRING=RelWithDebInfo\n"
    )
    (tmp_path / "Makefile").write_text("all:\n")
    assert build._product_cache_ready(tmp_path) is False


def test_empty_library_mount_is_not_ready(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    jpeg = settings.work_dir / "sources/unpacked/ijg-jpeg-6/jpeg-6"
    jpeg.mkdir(parents=True)
    mount = build.Mount(jpeg, "/jpeg")
    assert build.prepared_mount_ready(mount) is False
    (jpeg / "jpeglib.h").write_text("/* jpeg */\n")
    assert build.prepared_mount_ready(mount) is True


def test_lint_selection_refreshes_stale_source_index(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    header = tmp_path / "include/wiz8/item.h"
    header.parent.mkdir(parents=True)
    header.write_text("struct W8Item;\n")
    events: list[str] = []
    monkeypatch.setattr(
        "wiz8decomp.source_index.source_index_freshness",
        lambda *_args, **_kwargs: {"state": "stale", "detail": "newer source"},
    )
    monkeypatch.setattr(
        "wiz8decomp.source_index.indexed_targets",
        lambda *_args, **_kwargs: ["WIZ8"],
    )
    monkeypatch.setattr(
        "wiz8decomp.source_index.write_source_index",
        lambda *_args, **_kwargs: events.append("write") or {},
    )
    monkeypatch.setattr(
        "wiz8decomp.comparison.header_dependent_files",
        lambda *_args, **_kwargs: set(),
    )

    selected, changed, dependent = build._lint_selection(settings, [header])

    assert events == ["write"]
    assert changed == [header]
    assert selected == [header]
    assert dependent == []


def test_product_build_uses_product_only_vc6_image(tmp_path: Path) -> None:
    product = build.ContainerBuild.from_settings(_settings(tmp_path))
    assert product.image == build.VC6_PRODUCT_IMAGE
    assert product.image != build.VC6_IMAGE


def test_prepare_comparison_reuses_cached_original_without_installer(
    tmp_path: Path, monkeypatch
) -> None:
    import hashlib

    settings = _settings(tmp_path)
    settings.work_dir.mkdir(parents=True)
    settings.repo_dir.mkdir(exist_ok=True)
    original = settings.work_dir / "comparison/gog-base/Wiz8.exe"
    original.parent.mkdir(parents=True)
    payload = b"reviewed wiz8"
    original.write_bytes(payload)
    digest = hashlib.sha256(payload).hexdigest()
    (settings.repo_dir / "reccmp-project.yml").write_text(
        f"targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: {digest}\n"
    )

    events = []
    monkeypatch.setattr(
        "wiz8decomp.build.fetch_sources",
        lambda _settings: {"sources": []},
    )
    monkeypatch.setattr(
        build,
        "run",
        lambda command, **_kwargs: events.append(command),
    )

    result = build.prepare_comparison(settings, ["WIZ8"])

    assert result["extraction"] == "cached"
    assert result["targets"] == ["WIZ8"]
    assert events[0][:3] == ["reccmp-project", "detect", "--search-path"]


@pytest.mark.parametrize("mode", ["product", "lint", "diagnostics", "cached-lint"])
@pytest.mark.parametrize("dependency", ["/jpeg", "/zlib", "/infozip"])
def test_configuration_rejects_missing_sources_before_running_docker(
    tmp_path: Path, monkeypatch, mode: str, dependency: str
) -> None:
    settings = _settings(tmp_path)
    _prepare_sources(settings)
    mount = next(
        mount
        for mount in build.ContainerBuild.from_settings(settings).mounts
        if mount.container == dependency
    )
    (mount.host / build._SOURCE_MOUNT_SENTINELS[dependency]).unlink()
    monkeypatch.setattr(
        build, "run", lambda *_args, **_kwargs: pytest.fail("must validate before Docker")
    )
    if mode == "cached-lint":
        output = settings.repo_dir / build.LINT_BUILD_DIR
        output.mkdir(parents=True)
        for name in ("CMakeCache.txt", "build.ninja", "compile_commands.json"):
            (output / name).write_text("cached")
    with pytest.raises(RuntimeError, match="prepared build inputs are missing") as error:
        if mode == "product":
            build._configure(settings)
        else:
            build.configure_clang(settings, full_diagnostics=mode == "diagnostics")
    assert str(mount.host) in str(error.value)
    assert "uv run wiz8 prepare" in str(error.value)


def test_product_configuration_clears_cached_build_type(tmp_path: Path) -> None:
    command = build.ContainerBuild.from_settings(_settings(tmp_path)).configure_command()
    assert "-DCMAKE_BUILD_TYPE=" in command
    assert all("RelWithDebInfo" not in argument for argument in command)


def test_old_nmake_cache_requires_fresh_jom_configuration(tmp_path: Path) -> None:
    (tmp_path / "Makefile").write_text("all:\n")
    (tmp_path / "CMakeCache.txt").write_text(
        "CMAKE_GENERATOR:INTERNAL=NMake Makefiles\nCMAKE_BUILD_TYPE:STRING=\n"
    )
    assert not build._product_cache_ready(tmp_path)
    command = build.ContainerBuild.from_settings(_settings(tmp_path)).configure_command()
    assert "--fresh" in command
    assert "-DCMAKE_MAKE_PROGRAM=C:/jom/jom.exe" in command


def test_surrender_build_indexes_before_provider_validation_on_a_fresh_runner(
    tmp_path, monkeypatch
):
    settings = _settings(tmp_path)
    _prepare_sources(settings)
    output = settings.product_build_dir
    output.mkdir(parents=True)
    (output / "surrender-objects.txt").write_text("Z:/out/provider.obj\n")
    provider = output / "sr.dll"
    provider.write_bytes(b"cached DLL")
    os.utime(provider, (1, 1))
    events = []
    monkeypatch.setattr(build, "_product_cache_ready", lambda _: True)
    monkeypatch.setattr(build, "run", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(
        "wiz8decomp.source_index.write_source_index",
        lambda _, *, jobs: events.append(("index", jobs)),
    )

    def validate(*_args):
        assert events == [("index", 1)]
        events.append("provider")
        return {"ok": True, "compiler_exports_absent_from_retail": ["implicit"]}

    monkeypatch.setattr(build, "validate_surrender_provider_objects", validate)
    monkeypatch.setattr(
        "wiz8decomp.surrender_exports.validate_built_surrender_exports",
        lambda *_: events.append("exports") or {"ok": True},
    )
    result = build.build_target(settings, "SURRENDER", jobs=1)
    assert events == [("index", 1), "provider", "exports"]
    assert result["provider_objects"]["compiler_exports_absent_from_retail"] == ["implicit"]


@pytest.mark.parametrize("indexed", [False, True])
def test_product_build_never_opens_a_reccmp_catalog(tmp_path, monkeypatch, indexed):
    from reccmp.compare import Compare
    from wiz8decomp.emissions import OUTPUT

    settings = _settings(tmp_path)
    _prepare_sources(settings)
    output = settings.product_build_dir
    output.mkdir(parents=True)
    (output / "Makefile").write_text("all:\n")
    (output / "CMakeCache.txt").write_text(
        f"CMAKE_GENERATOR:INTERNAL={build.PRODUCT_GENERATOR}\nCMAKE_BUILD_TYPE:STRING=\n"
    )
    # A second product and a stale index must not change compilation behavior.
    for filename in ("Wiz8.exe", "Wiz8.pdb", "sr.dll", "sr.pdb"):
        (output / filename).write_bytes(b"existing product")
        os.utime(output / filename, ns=(1, 1))
    if indexed:
        (tmp_path / "build/source-index.json").write_text("stale projection")
    monkeypatch.setattr(Compare, "from_target", lambda *_: pytest.fail("build must not analyze"))
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_kwargs: commands.append(command))

    assert build.build_target(settings, "WIZ8", 2)["status"] == "ok"
    assert len(commands) == 1
    assert (tmp_path / OUTPUT / "wiz8-emissions.csv").is_file()


