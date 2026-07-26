from __future__ import annotations

import os

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
        raise RuntimeError(f"PyGhidra {REQUIRED_PYGHIDRA_VERSION} is required; found {pyghidra.__version__}")
    return {"ghidra_version": version, "ghidra_release": release, "pyghidra_version": pyghidra.__version__}


def start_pyghidra(settings: Settings, *, max_heap: str | None = None) -> None:
    validate_environment(settings)
    import pyghidra

    if pyghidra.started():
        return
    launcher = pyghidra.HeadlessPyGhidraLauncher(install_dir=settings.ghidra_install_dir)
    if max_heap is not None:
        launcher.add_vmargs(f"-Xmx{max_heap}")
    launcher.start()
