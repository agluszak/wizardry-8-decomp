from pathlib import Path


def test_all_pyghidra_project_opens_use_the_shared_owner() -> None:
    root = Path(__file__).resolve().parents[2] / "tools" / "wiz8decomp" / "ghidra"
    direct_project_opens = []
    unguarded_program_opens = []
    for path in root.glob("*.py"):
        source = path.read_text(encoding="utf-8")
        # The recovery self-test owns a unique temporary project, never the
        # reviewed wizardry8 project managed by env.py.
        if path.name == "lifecycle_fixture.py":
            assert source.count("pyghidra.open_project(") == 1
            assert 'pyghidra.open_project(temporary / "project", "lifecycle-fixture")' in source
            assert 'tempfile.TemporaryDirectory(prefix=f"lifecycle-{label}-"' in source
        elif path.name != "env.py" and "pyghidra.open_project(" in source:
            direct_project_opens.append(path.name)
        if "pyghidra.open_program(" in source and "project_lock(settings)" not in source:
            unguarded_program_opens.append(path.name)
    assert direct_project_opens == []
    assert unguarded_program_opens == []
