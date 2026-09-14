from __future__ import annotations

import subprocess
from pathlib import Path

from wiz8decomp.merge_preservation import merge_preservation_report, parse_allowed


def _commit(repo: Path, files: dict[str, str], message: str) -> str:
    for name, content in files.items():
        path = repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
    subprocess.run(["git", "-C", str(repo), "add", "."], check=True)
    subprocess.run(["git", "-C", str(repo), "commit", "-qm", message], check=True)
    return subprocess.run(
        ["git", "-C", str(repo), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def _repo(tmp_path: Path) -> Path:
    subprocess.run(["git", "init", "-q", "-b", "main", str(tmp_path)], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.email", "test@invalid"], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.name", "test"], check=True)
    return tmp_path


def test_function_definition_demoted_to_declaration_fails(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo(void) { return 1; }\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo(void);\n"},
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "failed"
    assert report["unexplained_demotions"] == ["FUNCTION WIZ8 0x00401000"]


def test_multiline_signature_demotion_fails(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {
            "src/wiz8/foo.cpp": (
                "// FUNCTION: WIZ8 0x00401000\n"
                "int Foo(\n"
                "    int first,\n"
                "    int second)\n"
                "{\n"
                "    return first + second;\n"
                "}\n"
            )
        },
        "base",
    )
    head = _commit(
        tmp_path,
        {
            "src/wiz8/foo.cpp": (
                "// FUNCTION: WIZ8 0x00401000\nint Foo(\n    int first,\n    int second);\n"
            )
        },
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "failed"
    assert report["unexplained_demotions"] == ["FUNCTION WIZ8 0x00401000"]


def test_function_declaration_staying_a_declaration_passes(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/wiz8/foo.h": "// FUNCTION: WIZ8 0x00401000\nint Foo(void);\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {"src/wiz8/foo.h": "// FUNCTION: WIZ8 0x00401000\nint Foo(int unused);\n"},
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["demoted"] == []


def test_demotion_may_be_allowed_with_a_reason(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo(void) { return 1; }\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo(void);\n"},
        "head",
    )

    report = merge_preservation_report(
        tmp_path,
        base,
        head,
        allowed={("FUNCTION", "WIZ8", 0x401000, "demotion"): "moved to runtime stub"},
    )

    assert report["status"] == "passed"
    assert report["demoted"][0]["identity"] == "FUNCTION WIZ8 0x00401000"


def test_global_definition_demoted_to_extern_fails(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// GLOBAL: WIZ8 0x00601000\nint g_foo = 3;\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {"src/wiz8/foo.cpp": "// GLOBAL: WIZ8 0x00601000\nextern int g_foo;\n"},
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "failed"
    assert report["unexplained_demotions"] == ["GLOBAL WIZ8 0x00601000"]


def test_current_tree_catches_uncommitted_loss(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path, {"src/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo() { return 1; }\n"}, "base"
    )
    (tmp_path / "src/foo.cpp").write_text("")
    current = merge_preservation_report(tmp_path, base)
    assert current["status"] == "failed"
    assert current["source_state"]["mode"] == "current"
    revision = merge_preservation_report(tmp_path, base, base)
    assert revision["status"] == "passed"
    assert revision["source_state"]["mode"] == "revision"
    assert revision["source_state"]["identical_sources"]
    assert revision["source_state"]["warning"]


def test_function_stub_conflict_cannot_be_waived_as_loss(tmp_path: Path) -> None:
    _repo(tmp_path)
    body = "// FUNCTION: WIZ8 0x00401000\nint Foo() { return 1; }\n"
    base = _commit(tmp_path, {"src/foo.cpp": body}, "base")
    head = _commit(
        tmp_path, {"src/stub.cpp": "// STUB: WIZ8 0x00401000\nint Stub() {}\n"}, "conflict"
    )
    report = merge_preservation_report(
        tmp_path, base, head, parse_allowed(["WIZ8:FUNCTION:0x00401000:loss=withdrawal"])
    )
    assert report["status"] == "failed"
    assert report["conflicts"] == [
        {"target": "WIZ8", "address": "0x00401000", "kinds": ["FUNCTION", "STUB"]}
    ]


def test_allow_is_scoped_to_target_kind_and_transition(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path, {"src/foo.cpp": "// FUNCTION: DEMO 0x00401000\nint Foo() {}\n"}, "base"
    )
    head = _commit(
        tmp_path, {"src/foo.cpp": "// FUNCTION: DEMO 0x00401000\nint Foo();\n"}, "demotion"
    )
    for selector in (
        "WIZ8:FUNCTION:0x00401000:demotion",
        "DEMO:GLOBAL:0x00401000:demotion",
        "DEMO:FUNCTION:0x00401000:loss",
    ):
        report = merge_preservation_report(
            tmp_path, base, head, parse_allowed([selector + "=reviewed"])
        )
        assert report["status"] == "failed"
