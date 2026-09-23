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


def test_folded_alias_shares_an_address_without_becoming_a_duplicate(tmp_path: Path) -> None:
    """An ICF-folded alias names an identity owned elsewhere, not a second owner."""

    _repo(tmp_path)
    head = _commit(
        tmp_path,
        {
            "src/wiz8/owner.cpp": (
                "// FUNCTION: WIZ8 0x00401000\nint FoldedOwner() { return 1; }\n"
            ),
            "src/wiz8/alias.cpp": (
                "// FUNCTION: WIZ8 0x00401000 FOLDED\nint FoldedAlias() { return 1; }\n"
            ),
        },
        "folded alias",
    )

    report = merge_preservation_report(tmp_path, head, head)

    assert report["status"] == "passed"
    assert report["duplicates"] == []


def test_two_owning_claims_on_one_address_still_fail(tmp_path: Path) -> None:
    _repo(tmp_path)
    head = _commit(
        tmp_path,
        {
            "src/wiz8/first.cpp": "// FUNCTION: WIZ8 0x00401000\nint First() { return 1; }\n",
            "src/wiz8/second.cpp": "// FUNCTION: WIZ8 0x00401000\nint Second() { return 1; }\n",
        },
        "duplicate owners",
    )

    report = merge_preservation_report(tmp_path, head, head)

    assert report["status"] == "failed"
    assert report["unexplained_duplicates"] == ["FUNCTION WIZ8 0x00401000"]


def test_a_fold_only_address_keeps_its_claims_as_owners(tmp_path: Path) -> None:
    _repo(tmp_path)
    head = _commit(
        tmp_path,
        {
            "src/wiz8/first.cpp": "// FUNCTION: WIZ8 0x00401000 FOLDED\nint First();\n",
            "src/wiz8/second.cpp": "// FUNCTION: WIZ8 0x00401000 FOLDED\nint Second();\n",
        },
        "fold only",
    )

    report = merge_preservation_report(tmp_path, head, head)

    assert report["status"] == "failed"
    assert report["unexplained_duplicates"] == ["FUNCTION WIZ8 0x00401000"]


def test_folded_alias_cannot_demote_the_owning_definition(tmp_path: Path) -> None:
    """The owner stays a definition even though an alias of it is declaration-only."""

    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/wiz8/owner.cpp": "// FUNCTION: WIZ8 0x00401000\nint Owner() { return 1; }\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {
            "src/wiz8/owner.cpp": "// FUNCTION: WIZ8 0x00401000\nint Owner() { return 1; }\n",
            "src/wiz8/alias.cpp": "// FUNCTION: WIZ8 0x00401000 FOLDED\nint Alias();\n",
        },
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["demoted"] == []


def test_vtable_class_discriminator_is_not_an_alias(tmp_path: Path) -> None:
    """A trailing discriminator distinguishes co-located vtables; both own their address."""

    _repo(tmp_path)
    head = _commit(
        tmp_path,
        {
            "src/wiz8/widget.cpp": (
                "// VTABLE: WIZ8 0x00601000 W8TextControl::Listener\n"
                "W8TextControl::Listener v_table_a;\n"
            ),
            "src/wiz8/other.cpp": (
                "// VTABLE: WIZ8 0x00601000 W8ControlSelectionListener\n"
                "W8ControlSelectionListener v_table_b;\n"
            ),
        },
        "discriminators",
    )

    report = merge_preservation_report(tmp_path, head, head)

    assert report["status"] == "failed"
    assert report["unexplained_duplicates"] == ["VTABLE WIZ8 0x00601000"]


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


def test_base_ancestry_passes_when_base_is_ancestor(tmp_path: Path) -> None:
    from wiz8decomp.merge_preservation import base_ancestry_report

    _repo(tmp_path)
    base = _commit(tmp_path, {"src/foo.cpp": "int a;\n"}, "base")
    head = _commit(tmp_path, {"src/foo.cpp": "int a;\nint b;\n"}, "head")

    report = base_ancestry_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["base"] == base
    assert report["merge_base"] == base
    assert report["ahead"] == 1
    assert report["behind"] == 0


def test_jj_base_accepts_git_remote_branch_spelling(tmp_path: Path, monkeypatch) -> None:
    from wiz8decomp.merge_preservation import _resolve_commit

    (tmp_path / ".jj").mkdir()
    calls = []

    def fake_run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0, stdout="base-commit\n", stderr="")

    monkeypatch.setattr(subprocess, "run", fake_run)

    assert _resolve_commit(tmp_path, "origin/main") == "base-commit"
    assert calls == [["jj", "log", "-r", "main@origin", "--no-graph", "-T", "commit_id"]]


def test_base_ancestry_fails_on_diverged_branch(tmp_path: Path) -> None:
    from wiz8decomp.merge_preservation import base_ancestry_report

    _repo(tmp_path)
    stale = _commit(tmp_path, {"src/foo.cpp": "int a;\n"}, "stale-base")
    # Head carries one commit on the old base while main advances elsewhere.
    subprocess.run(["git", "-C", str(tmp_path), "checkout", "-qb", "branch"], check=True)
    _commit(tmp_path, {"src/stale.cpp": "int stale;\n"}, "branch-work")
    branch_head = subprocess.run(
        ["git", "-C", str(tmp_path), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    subprocess.run(["git", "-C", str(tmp_path), "checkout", "-q", "main"], check=True)
    base = _commit(tmp_path, {"src/new.cpp": "int fresh;\n"}, "main-advanced")

    report = base_ancestry_report(tmp_path, base, branch_head)

    assert report["status"] == "failed"
    assert report["base"] == base
    assert report["merge_base"] == stale
    assert report["ahead"] == 1
    assert report["behind"] == 1
    assert "not an ancestor" in report["error"]
    assert report["changed_files_since_merge_base"] == {
        "head": ["src/stale.cpp"],
        "base": ["src/new.cpp"],
    }
