"""Refuse to reuse a build directory that belongs to another checkout.

The CMake build directory lives at `build/decomp` inside the checkout, so each
working copy gets its own by construction and cannot overwrite another's cache
or linked `Wiz8.exe`/`Wiz8.pdb`. That is the structural fix; this module is the
belt to its braces, because a cache can still be stale after a checkout is moved
or copied, and the symptom of a mismatch is not an error -- it is a *wrong
measurement*. `just compare` will read an image linked from other sources and
report correct functions as 30-40% similar.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

CACHE_NAME = "CMakeCache.txt"
PROJECT_KEY = "RECCMP_PROJECT_DIR_HOST"
BUILD_KEY = "RECCMP_BUILD_DIR_HOST"


class BuildDirectoryConflict(RuntimeError):
    """The build directory was configured by a different checkout or work dir."""


@dataclass(frozen=True)
class BuildDirectoryOwner:
    """What a build directory's CMake cache says about who configured it."""

    cache: Path
    project_dir: str | None
    build_dir: str | None

    @property
    def configured(self) -> bool:
        return self.project_dir is not None


def read_owner(build_dir: Path) -> BuildDirectoryOwner:
    """Read the recorded host paths. An unconfigured directory is not an error."""

    cache = build_dir / CACHE_NAME
    if not cache.is_file():
        return BuildDirectoryOwner(cache=cache, project_dir=None, build_dir=None)
    values: dict[str, str] = {}
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        for key in (PROJECT_KEY, BUILD_KEY):
            prefix = f"{key}:PATH="
            if line.startswith(prefix):
                values[key] = line[len(prefix) :].strip()
    return BuildDirectoryOwner(
        cache=cache,
        project_dir=values.get(PROJECT_KEY),
        build_dir=values.get(BUILD_KEY),
    )


def _same_path(left: str | None, right: Path) -> bool:
    if not left:
        return False
    try:
        return Path(left).resolve() == right.resolve()
    except OSError:
        return False


def check_build_directory(build_dir: Path, repo_dir: Path) -> dict[str, object]:
    """Raise if `build_dir` was configured by a checkout other than `repo_dir`.

    A directory that has never been configured is fine: CMake is about to claim
    it. Only a cache naming a *different* checkout is refused.
    """

    owner = read_owner(build_dir)
    result: dict[str, object] = {
        "build_dir": str(build_dir),
        "cache": str(owner.cache),
        "configured": owner.configured,
        "recorded_project_dir": owner.project_dir,
        "recorded_build_dir": owner.build_dir,
        "repo_dir": str(repo_dir),
    }
    if not owner.configured:
        result["ok"] = True
        return result

    if not _same_path(owner.project_dir, repo_dir):
        raise BuildDirectoryConflict(
            f"{owner.cache} was configured by a different checkout.\n"
            f"  recorded {PROJECT_KEY}: {owner.project_dir}\n"
            f"  this checkout:          {repo_dir}\n"
            "Two checkouts must not share WIZ8_WORK_DIR: they overwrite each other's CMake cache "
            "and linked Wiz8.exe/Wiz8.pdb, and `just compare` then reports the other checkout's "
            "code as if it were yours. Point WIZ8_WORK_DIR at a directory of your own; see "
            ".env.example for how to share the large read-only trees with `cp -al`."
        )

    if owner.build_dir and not _same_path(owner.build_dir, build_dir):
        raise BuildDirectoryConflict(
            f"{owner.cache} records a different build directory.\n"
            f"  recorded {BUILD_KEY}: {owner.build_dir}\n"
            f"  this build directory:  {build_dir}\n"
            "The work directory has moved since it was configured, so reccmp would map addresses "
            "through stale paths. Delete the build directory and reconfigure."
        )

    result["ok"] = True
    return result
