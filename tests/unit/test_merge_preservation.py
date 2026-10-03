from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest
import yaml
from wiz8decomp.merge_preservation import base_ancestry_report, merge_preservation_report


@pytest.mark.parametrize("job", ["scope", "repository"])
def test_pr_event_revisions_survive_main_advancing(tmp_path: Path, job: str) -> None:
    upstream = _repo(tmp_path / "upstream")
    path = "src/surrender/node.cpp"
    base = _commit(upstream, {path: "int value = 0;\n"}, "base")
    head = _commit(upstream, {path: "int value = 1;\n"}, "PR head")
    subprocess.run(["git", "-C", str(upstream), "update-ref", "refs/pull/1/head", head], check=True)
    subprocess.run(["git", "-C", str(upstream), "checkout", "-q", base], check=True)
    merged = _commit(upstream, {path: "int value = 1;\n"}, "squash merge")
    subprocess.run(
        ["git", "-C", str(upstream), "update-ref", "refs/heads/main", merged], check=True
    )
    runner = _repo(tmp_path / "runner")
    subprocess.run(
        ["git", "-C", str(runner), "remote", "add", "origin", upstream.as_uri()], check=True
    )
    root = Path(__file__).resolve().parents[2]
    workflow = yaml.safe_load((root / ".github/workflows/ci.yml").read_text())
    step_name = "Classify changed paths" if job == "scope" else "Fetch PR comparison baseline"
    step = next(s for s in workflow["jobs"][job]["steps"] if s["name"] == step_name)
    script = step["run"].replace("${{ github.event.pull_request.base.sha }}", base)
    script = script.replace("${{ github.event.pull_request.head.sha }}", head)
    script = script.replace("${{ github.event.pull_request.number }}", "1")
    classifier = runner / ".github/scripts/classify-ci-changes.py"
    classifier.parent.mkdir(parents=True)
    classifier.write_text((root / ".github/scripts/classify-ci-changes.py").read_text())
    output = tmp_path / "output"
    subprocess.run(
        ["bash", "-e", "-c", script],
        cwd=runner,
        env={
            **os.environ,
            "GITHUB_EVENT_NAME": "pull_request",
            "GITHUB_BASE_REF": "main",
            "RUNNER_TEMP": str(tmp_path),
            "GITHUB_OUTPUT": str(output),
        },
        check=True,
        capture_output=True,
        text=True,
    )
    report = base_ancestry_report(runner, "origin/main", "origin/pr-head")
    assert report["status"] == "passed"
    assert report["base"] == base
    assert report["head"] == head
    if job == "scope":
        assert "surrender=true" in output.read_text()


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
    assert report["demoted"][0]["identity"] == "FUNCTION WIZ8 0x00401000"


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
    assert report["demoted"][0]["identity"] == "FUNCTION WIZ8 0x00401000"


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


def test_function_may_be_reclassified_as_synthetic(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Foo(void) { return 1; }\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {
            "src/foo.cpp": "",
            "evidence/observations/compiler-emissions.csv": (
                "target|address|symbol|name|type\n"
                "WIZ8|00401000||compiler-owned teardown emission|synthetic\n"
            ),
        },
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["lost"] == []
    assert report["reclassified"][0]["replacement"] == "EMISSION WIZ8 0x00401000"


def test_function_may_be_reclassified_as_template(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/foo.cpp": "// FUNCTION: WIZ8 0x00401000\nint Grow(void) { return 1; }\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {
            "src/foo.cpp": "",
            "config/reccmp/emission_overrides.csv": (
                "target|address|symbol|name|type\nWIZ8|00401000||Hash<int>::Grow|template\n"
            ),
        },
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["reclassified"][0]["replacement"] == "EMISSION WIZ8 0x00401000"


def test_global_may_be_reclassified_as_string(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/foo.cpp": '// GLOBAL: WIZ8 0x00601000\nchar* g_file = "foo.cpp";\n'},
        "base",
    )
    head = _commit(
        tmp_path,
        {"src/foo.cpp": '// STRING: WIZ8 0x00601000\n#define FILE_NAME "foo.cpp"\n'},
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["reclassified"][0]["replacement"] == "STRING WIZ8 0x00601000"


def test_interior_global_may_be_absorbed_by_known_aggregate_extent(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {"src/foo.cpp": "// GLOBAL: WIZ8 0x00601004\nint g_top;\n"},
        "base",
    )
    head = _commit(
        tmp_path,
        {
            "src/foo.cpp": (
                "struct Rect { int left; int top; int right; int bottom; };\n"
                'static_assert(sizeof(Rect) == 16, "Rect_size");\n'
                "// GLOBAL: WIZ8 0x00601000\n"
                "Rect g_rect;\n"
            )
        },
        "head",
    )

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "passed"
    assert report["lost"] == []
    assert report["subsumed"] == [
        {
            "identity": "GLOBAL WIZ8 0x00601004",
            "container": "GLOBAL WIZ8 0x00601000",
            "name": "g_rect",
            "offset": "0x4",
            "size": 16,
            "source_file": "src/foo.cpp",
            "line": 4,
        }
    ]


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
    assert report["demoted"][0]["identity"] == "GLOBAL WIZ8 0x00601000"


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


def test_function_replaced_by_stub_fails(tmp_path: Path) -> None:
    _repo(tmp_path)
    body = "// FUNCTION: WIZ8 0x00401000\nint Foo() { return 1; }\n"
    base = _commit(tmp_path, {"src/foo.cpp": body}, "base")
    head = _commit(tmp_path, {"src/foo.cpp": "// STUB: WIZ8 0x00401000\nint Stub() {}\n"}, "stub")

    report = merge_preservation_report(tmp_path, base, head)

    assert report["status"] == "failed"
    assert report["lost"][0]["identity"] == "FUNCTION WIZ8 0x00401000"


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
    assert report["duplicates"][0]["identity"] == "FUNCTION WIZ8 0x00401000"


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
    assert report["duplicates"][0]["identity"] == "VTABLE WIZ8 0x00601000"


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


def test_removing_binary_emission_identity_is_a_preservation_failure(tmp_path: Path) -> None:
    _repo(tmp_path)
    base = _commit(
        tmp_path,
        {
            "evidence/observations/compiler-emissions.csv": (
                "target|address|symbol|name|type\nWIZ8|00401000|??_GFoo@@UAEPAXI@Z||synthetic\n"
            )
        },
        "base",
    )
    head = _commit(
        tmp_path,
        {"evidence/observations/compiler-emissions.csv": "target|address|symbol|name|type\n"},
        "head",
    )
    report = merge_preservation_report(tmp_path, base, head)
    assert report["status"] == "failed"
    assert report["lost"][0]["identity"] == "EMISSION WIZ8 0x00401000"


@pytest.mark.parametrize(
    "target,metadata", [("WIZ8", "wiz8-msvc-runtime.csv"), ("SURRENDER", "surrender-libraries.csv")]
)
@pytest.mark.parametrize("preserve", [True, False])
def test_library_identity_migration_is_audited(
    tmp_path: Path, preserve: bool, target: str, metadata: str
) -> None:
    _repo(tmp_path)
    source = "src/wiz8/vc6_runtime.cpp"
    base = _commit(tmp_path, {source: f"// LIBRARY: {target} 0x005e1c30\n// __aulldiv\n"}, "base")
    rows = "address|symbol|name|type\n"
    if preserve:
        rows += "005e1c30||__aulldiv|library\n"
    head = _commit(tmp_path, {source: "", f"config/reccmp/{metadata}": rows}, "head")
    report = merge_preservation_report(tmp_path, base, head)
    assert report["status"] == ("passed" if preserve else "failed")
    assert report["counts"]["LIBRARY"] == {"base": 1, "head": int(preserve)}
    if preserve:
        assert report["changed"] == []
    else:
        assert report["lost"][0]["identity"] == f"LIBRARY {target} 0x005E1C30"
