from types import SimpleNamespace

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
