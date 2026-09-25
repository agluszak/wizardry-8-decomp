import os
from pathlib import Path

import pytest
from wiz8decomp import build
from wiz8decomp.config import Settings


def _settings(repository: Path) -> Settings:
    return Settings.model_construct(
        repo_dir=repository,
        work_dir=repository / "work",
        input_dir=repository / "input",
        ghidra_install_dir=repository / "ghidra",
    )


def test_product_build_invokes_parallel_jom(tmp_path: Path) -> None:
    command = build.ContainerBuild.from_settings(_settings(tmp_path)).build_command("WIZ8", 7)

    assert command[-2:] == [
        "/c",
        "set TEMP=Z:\\out\\tmp&& set TMP=Z:\\out\\tmp&& cd /d Z:\\out&& C:\\jom\\jom.exe -j 7 WIZ8",
    ]


def test_jom_parallelism_removes_only_generated_guards(tmp_path: Path) -> None:
    makefile = tmp_path / "Makefile"
    nested = tmp_path / "CMakeFiles/Makefile2"
    nested.parent.mkdir()
    makefile.write_text(".NOTPARALLEL:\nall:\n\t@echo ok\n")
    nested.write_text(".NOTPARALLEL:\nrule:\n")
    makefile.chmod(0o444)
    nested.chmod(0o444)

    updated = build._enable_jom_parallelism(tmp_path)

    assert updated == [str(makefile), str(nested)]
    assert ".NOTPARALLEL:" not in makefile.read_text()
    assert "all:\n\t@echo ok" in makefile.read_text()


def test_clang_configuration_reuses_existing_ninja_tree(tmp_path: Path, monkeypatch) -> None:
    output = tmp_path / build.LINT_BUILD_DIR
    output.mkdir(parents=True)
    (output / "CMakeCache.txt").write_text("configured")
    (output / "build.ninja").write_text("ninja")
    (output / "compile_commands.json").write_text("[]")
    monkeypatch.setattr(
        build, "run", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError)
    )

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

    build.configure_clang(_settings(tmp_path))

    assert len(commands) == 1


def test_forced_clang_configuration_is_incremental_not_fresh(tmp_path: Path, monkeypatch) -> None:
    commands = []
    monkeypatch.setattr(build, "run", lambda command, **_kwargs: commands.append(command))

    build.configure_clang(_settings(tmp_path), force=True)

    assert len(commands) == 1
    assert "--fresh" not in commands[0]


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
    (tmp_path / "CMakeCache.txt").write_text("CMAKE_GENERATOR:INTERNAL=NMake Makefiles\n")
    assert build._product_cache_ready(tmp_path) is False
    (tmp_path / "Makefile").write_text("all:\n")
    assert build._product_cache_ready(tmp_path) is True


def test_empty_library_mount_is_not_ready(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    jpeg = settings.work_dir / "fid/sources/unpacked/ijg-jpeg-6/jpeg-6"
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


def test_product_inputs_names_stale_extraction_recipe(tmp_path: Path, monkeypatch) -> None:
    from wiz8decomp import doctor

    settings = _settings(tmp_path)
    extracted = settings.work_dir / "extracted" / "gog-base"
    extracted.mkdir(parents=True)
    (extracted / ".wiz8-extraction.json").write_text("{}", encoding="utf-8")
    monkeypatch.setattr(
        "wiz8decomp.build.prepared_mount_ready",
        lambda _mount: True,
    )
    monkeypatch.setattr(
        "wiz8decomp.extract.variants.verify_extraction",
        lambda *_args, **_kwargs: {
            "ok": False,
            "errors": ["receipt implementation_revision differs from the current recipe"],
        },
    )
    result = doctor._product_inputs_check(settings)
    assert result["ok"] is True
    assert result["status"] == "stale-recipe"
    assert "gog-media" in (result["detail"] or "")
    assert "corpus clean --stage extractions" in (result["detail"] or "")
    assert "corpus extract gog-media" in (result["detail"] or "")


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
        "wiz8decomp.ghidra.fid_seeds.fetch_seed_sources",
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
