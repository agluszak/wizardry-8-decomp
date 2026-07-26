from pathlib import Path

import pytest
from wiz8decomp.build_dir import (
    BuildDirectoryConflict,
    check_build_directory,
    read_owner,
)


def _cache(build_dir: Path, project_dir: Path | str, own_build_dir: Path | str | None) -> None:
    build_dir.mkdir(parents=True, exist_ok=True)
    lines = [
        "# This is the CMakeCache file.",
        "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo",
        f"RECCMP_PROJECT_DIR_HOST:PATH={project_dir}",
    ]
    if own_build_dir is not None:
        lines.append(f"RECCMP_BUILD_DIR_HOST:PATH={own_build_dir}")
    (build_dir / "CMakeCache.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def test_unconfigured_directory_is_accepted(tmp_path: Path) -> None:
    # CMake is about to claim it; nothing has been overwritten.
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    result = check_build_directory(build_dir, tmp_path / "checkout")

    assert result["ok"] is True
    assert result["configured"] is False


def test_directory_configured_by_this_checkout_is_accepted(tmp_path: Path) -> None:
    checkout = tmp_path / "checkout"
    checkout.mkdir()
    build_dir = tmp_path / "work" / "build"
    _cache(build_dir, checkout, build_dir)

    result = check_build_directory(build_dir, checkout)

    assert result["ok"] is True
    assert result["recorded_project_dir"] == str(checkout)


def test_directory_configured_by_another_checkout_is_refused(tmp_path: Path) -> None:
    mine = tmp_path / "wizardry-evidence"
    theirs = tmp_path / "wizardry-decomp-agent1"
    for path in (mine, theirs):
        path.mkdir()
    build_dir = tmp_path / "shared-work" / "build"
    _cache(build_dir, theirs, build_dir)

    with pytest.raises(BuildDirectoryConflict) as excinfo:
        check_build_directory(build_dir, mine)

    message = str(excinfo.value)
    # The message has to name both sides, or the reader cannot tell what to change.
    assert str(theirs) in message
    assert str(mine) in message
    assert "WIZ8_WORK_DIR" in message


def test_moved_work_directory_is_refused(tmp_path: Path) -> None:
    checkout = tmp_path / "checkout"
    checkout.mkdir()
    build_dir = tmp_path / "new-work" / "build"
    _cache(build_dir, checkout, tmp_path / "old-work" / "build")

    with pytest.raises(BuildDirectoryConflict, match="different build directory"):
        check_build_directory(build_dir, checkout)


def test_a_cache_without_the_build_key_still_validates_the_checkout(tmp_path: Path) -> None:
    checkout = tmp_path / "checkout"
    checkout.mkdir()
    build_dir = tmp_path / "work" / "build"
    _cache(build_dir, checkout, None)

    assert check_build_directory(build_dir, checkout)["ok"] is True

    other = tmp_path / "other"
    other.mkdir()
    with pytest.raises(BuildDirectoryConflict):
        check_build_directory(build_dir, other)


def test_read_owner_reports_an_absent_cache_without_raising(tmp_path: Path) -> None:
    owner = read_owner(tmp_path / "nowhere")

    assert owner.configured is False
    assert owner.project_dir is None
