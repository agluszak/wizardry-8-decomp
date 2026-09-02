"""One project-opening path for every live Wizardry Ghidra operation."""

from __future__ import annotations

import fcntl
from collections.abc import Iterator
from contextlib import contextmanager
from typing import Any

from ..config import Settings
from .environment import start_pyghidra


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

    program_name = ensure_seed(settings, selector)
    with open_project(settings) as project:
        import pyghidra

        with pyghidra.program_context(project, "/" + program_name) as program:
            yield program
