from types import SimpleNamespace

import pytest
from wiz8decomp.config import repository_root
from wiz8decomp.ghidra import project


def _modules() -> list[dict[str, str]]:
    return [
        {
            "program_name": "wiz8--demo--wiz8--demo",
            "variant": "demo",
            "module_name": "Wiz8.exe",
        },
        {
            "program_name": "wiz8--gog-base--wiz8--canonical",
            "variant": "gog-base",
            "module_name": "Wiz8.exe",
        },
        {
            "program_name": "wiz8--gog-base--sr--renderer",
            "variant": "gog-base",
            "module_name": "Sr.dll",
        },
    ]


def test_wiz8_is_the_canonical_executable_alias(monkeypatch) -> None:
    modules = _modules()
    monkeypatch.setattr(project, "configured_modules", lambda *_args, **_kwargs: modules)

    assert (
        project.resolve_program_name(SimpleNamespace(), "wiz8") == "wiz8--gog-base--wiz8--canonical"
    )
    assert (
        project.resolve_program_name(SimpleNamespace(), None) == "wiz8--gog-base--wiz8--canonical"
    )


def test_variant_qualified_alias_still_selects_a_noncanonical_build(monkeypatch) -> None:
    modules = _modules()
    monkeypatch.setattr(project, "configured_modules", lambda *_args, **_kwargs: modules)

    assert (
        project.resolve_program_name(SimpleNamespace(), "demo/Wiz8.exe") == "wiz8--demo--wiz8--demo"
    )


def test_all_shipped_surrender_dll_families_are_independent_ghidra_programs(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    names = [
        "srDD_DirectX6.dll",
        "srDD_DirectX7.dll",
        "srDD_Glide2x.dll",
        "srDD_OpenGL.dll",
        "srDD_Software.dll",
        "srEXT_AVI.dll",
        "srEXT_default.dll",
        "srEXT_FLIC.dll",
        "srEXT_HTTP.dll",
        "srEXT_Inspector.dll",
        "srEXT_JPEGImporter.dll",
        "srEXT_LWOImporter.dll",
        "srEXT_MPEG.dll",
        "srEXT_Unzip.dll",
        "srHXImporter.dll",
        "srVP_486.dll",
        "srVP_AMD3DNow.dll",
        "srVP_Generic.dll",
        "srVP_KNI.dll",
        "srVP_Pentium.dll",
        "srVP_PentiumII.dll",
        "srVP_PentiumMMX.dll",
        "srVP_PentiumPro.dll",
        "srVP_x86.dll",
    ]
    modules = [
        {
            "variant": "gog-base",
            "module_name": name,
            "relative_path": f"Dll/{name}",
            "sha256": str(index) * 64,
        }
        for index, name in enumerate(names, 1)
    ]
    monkeypatch.setattr(project, "load_inventory", lambda _settings: {"modules": modules})

    selected = project.configured_modules(
        SimpleNamespace(repo_dir=repository_root()), all_modules=True
    )

    assert [item["module_name"] for item in selected] == names


def test_default_ghidra_selection_does_not_claim_renderer_modules_as_wiz8(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    modules = [
        {
            "variant": "gog-base",
            "module_name": name,
            "relative_path": name if name == "Wiz8.exe" else f"Dll/{name}",
            "sha256": str(index) * 64,
        }
        for index, name in enumerate(("Wiz8.exe", "srEXT_AVI.dll", "srVP_KNI.dll"), 1)
    ]
    monkeypatch.setattr(project, "load_inventory", lambda _settings: {"modules": modules})

    selected = project.configured_modules(SimpleNamespace(repo_dir=repository_root()))

    assert [item["module_name"] for item in selected] == ["Wiz8.exe"]
