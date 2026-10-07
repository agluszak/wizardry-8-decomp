import subprocess

import pytest
from wiz8decomp.cast_lint import baseline_diff
from wiz8decomp.clang_tidy_lines import redundant_cast_line_filter
from wiz8decomp.comparison import changed_files, changed_source_files
from wiz8decomp.repository import tracked_paths
from wiz8decomp.source_oracle import extract_declaration_oracle


@pytest.fixture
def repository(tmp_path, monkeypatch):
    for name in (
        "GITHUB_ACTIONS",
        "GITHUB_BASE_REF",
        "GITHUB_EVENT_BEFORE",
        "WIZ8_REDUNDANT_CAST_LINES",
    ):
        monkeypatch.delenv(name, raising=False)
    git(tmp_path, "init", "-b", "main")
    git(tmp_path, "config", "user.name", "Test")
    git(tmp_path, "config", "user.email", "test@example.invalid")
    (tmp_path / "src/wiz8").mkdir(parents=True)
    (tmp_path / "src/wiz8/old.cpp").write_text("original();\n")
    (tmp_path / "released").mkdir()
    (tmp_path / "released/TIMER.H").write_text("typedef unsigned int TIMER;\n")
    git(tmp_path, "add", ".")
    git(tmp_path, "commit", "-m", "Baseline")
    git(tmp_path, "update-ref", "refs/remotes/origin/main", "HEAD")
    git(tmp_path, "switch", "-c", "work")
    return tmp_path


def git(repository, *args):
    return subprocess.run(
        ["git", *args], cwd=repository, check=True, capture_output=True, text=True
    ).stdout.strip()


def test_git_selection_includes_committed_staged_and_unstaged_changes(repository):
    original = repository / "src/wiz8/old.cpp"
    git(repository, "mv", "src/wiz8/old.cpp", "src/wiz8/renamed.cpp")
    git(repository, "commit", "-m", "Move")
    staged = repository / "src/wiz8/Two Words.h"
    staged.write_text("staged();\n")
    unusual = repository / 'src/wiz8/quote"and\nnewline.cpp'
    unusual.write_text("added();\n")
    git(repository, "add", ".")
    renamed = repository / "src/wiz8/renamed.cpp"
    renamed.write_text("original();\nworking();\n")
    untracked = repository / "src/wiz8/untracked.cpp"
    untracked.write_text("untracked();\n")

    assert set(changed_files(repository)) == {original, renamed, staged, unusual}
    assert set(changed_source_files(repository)) == {renamed, staged, unusual}
    assert set(tracked_paths(repository)) == {
        renamed,
        staged,
        unusual,
        repository / "released/TIMER.H",
    }
    base, diff = baseline_diff(repository)
    assert base == git(repository, "rev-parse", "origin/main")
    assert "+working();" in diff and "+staged();" in diff


def test_git_line_filter_includes_staged_and_working_lines(repository):
    source = repository / "src/wiz8/old.cpp"
    source.write_text("original();\nstaged();\n")
    git(repository, "add", ".")
    source.write_text("original();\nstaged();\nworking();\n")
    assert redundant_cast_line_filter(repository) == "src/wiz8/old.cpp@2-3"


def test_oracle_extraction_uses_pinned_git_bytes_in_a_worktree(repository, tmp_path):
    revision = git(repository, "rev-parse", "HEAD")
    blob = git(repository, "rev-parse", revision + ":released/TIMER.H")
    worktree = tmp_path / "linked-worktree"
    git(repository, "worktree", "add", "--detach", str(worktree), revision)
    (worktree / "released/TIMER.H").write_text("wrong current bytes\n")
    stage = tmp_path / "stage"
    result = extract_declaration_oracle(
        worktree, {"revision": revision, "source_root": "released"}, stage
    )
    assert (stage / "source/timer.h").read_bytes() == b"typedef unsigned int TIMER;\n"
    assert result["files"]["source/timer.h"]["blob"] == blob
