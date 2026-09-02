"""One project-opening path for every live Wizardry Ghidra operation."""

from __future__ import annotations

import fcntl
import os
from collections.abc import Iterator
from contextlib import contextmanager
from typing import Any

from ..config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
    ghidra_version,
)


def validate_environment(settings: Settings) -> dict[str, str]:
    version, release = ghidra_version(settings.ghidra_install_dir)
    if version != REQUIRED_GHIDRA_VERSION or release != REQUIRED_GHIDRA_RELEASE:
        raise RuntimeError(
            f"Ghidra {REQUIRED_GHIDRA_VERSION} {REQUIRED_GHIDRA_RELEASE} is required; "
            f"found {version or 'missing'} {release or 'missing'} at {settings.ghidra_install_dir}"
        )
    os.environ["GHIDRA_INSTALL_DIR"] = str(settings.ghidra_install_dir)
    import pyghidra

    if pyghidra.__version__ != REQUIRED_PYGHIDRA_VERSION:
        raise RuntimeError(
            f"PyGhidra {REQUIRED_PYGHIDRA_VERSION} is required; found {pyghidra.__version__}"
        )
    return {
        "ghidra_version": version,
        "ghidra_release": release,
        "pyghidra_version": pyghidra.__version__,
    }


def start_pyghidra(settings: Settings, *, max_heap: str | None = None) -> None:
    validate_environment(settings)
    import pyghidra

    if pyghidra.started():
        return
    launcher = pyghidra.HeadlessPyGhidraLauncher(install_dir=settings.ghidra_install_dir)
    if max_heap is not None:
        launcher.add_vmargs(f"-Xmx{max_heap}")
    launcher.start()


@contextmanager
def project_lock(settings: Settings) -> Iterator[None]:
    """Exclude every competing process for an entire project ownership interval."""

    lock_path = settings.build_dir / "ghidra" / "project-open.lock"
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with lock_path.open("a+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


@contextmanager
def open_project(settings: Settings, *, create: bool = False) -> Iterator[Any]:
    """Open this checkout's project while excluding every competing owner."""

    from .workspace import check_project_owner

    check_project_owner(settings)
    with project_lock(settings):
        start_pyghidra(settings)
        import pyghidra

        project = pyghidra.open_project(settings.project_dir, settings.project_name, create=create)
        try:
            yield project
        finally:
            project.close()


@contextmanager
def open_program(settings: Settings, selector: str = "wiz8") -> Iterator[Any]:
    """Ensure and open one reviewed program through the canonical project owner."""

    from .workspace import ensure_seed

    settings.project_dir.mkdir(parents=True, exist_ok=True)
    with open_project(settings, create=True) as project:
        program_name = ensure_seed(settings, project, selector)
        import pyghidra

        with pyghidra.program_context(project, "/" + program_name) as program:
            yield program
